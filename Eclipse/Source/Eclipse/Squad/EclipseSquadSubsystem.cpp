#include "Squad/EclipseSquadSubsystem.h"

#include "AI/EclipseEnemyController.h"
#include "AI/EclipseSquadmateController.h"
#include "Characters/EclipseCharacter.h"
#include "Characters/EclipsePlayerController.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "EngineUtils.h"
#include "Squad/EclipseBreachPoint.h"
#include "Squad/EclipseCommandModeTuning.h"
#include "TimerManager.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Squad/EclipseSquadOrderLogic.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "StructUtils/InstancedStruct.h"

void UEclipseSquadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if !UE_BUILD_SHIPPING
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Squad.DumpOrders")) == nullptr)
	{
		DumpCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("Eclipse.Squad.DumpOrders"),
			TEXT("Log each squadmate's current order state (SPEC-P1-06 debug)."),
			FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
			{
				UE_LOG(LogEclipse, Display, TEXT("Squad: %d registered squadmates."), Squadmates.Num());
				for (const FSquadmateEntry& Entry : Squadmates)
				{
					if (const AEclipseSquadmateController* Controller = Entry.Controller.Get())
					{
						UE_LOG(LogEclipse, Display, TEXT("  %s -> order %d"),
							*Entry.SoldierId.ToString(), static_cast<int32>(Controller->GetCurrentOrder()));
					}
				}

				// R3 criterion 1 read-out, so the number survives a session where
				// nobody had the debug HUD open (feel-gauntlet telemetry).
				UE_LOG(LogEclipse, Display, TEXT("  round trip: %d/%d answers <= %.2f s wall clock (worst %.3f s, avg %.3f s)"),
					OrderRoundTrip.WithinBarCount, OrderRoundTrip.SampleCount,
					EclipseSquadOrderLogic::FEclipseOrderRoundTripStats::BarSeconds,
					OrderRoundTrip.WorstSeconds, OrderRoundTrip.GetAverageSeconds());

				// Stage B-stand erbij (SPEC-P2-02): markeringen, breekpunten en wie
				// er stil is. Zonder deze regels is een geweigerde sync strike van
				// buitenaf niet te onderscheiden van een kapotte — en dat is precies
				// het onderscheid dat een weigering hoort te maken.
				const UEclipseCommandModeTuningAsset* CommandTuning = ResolveCommandTuning();
				UE_LOG(LogEclipse, Display, TEXT("  sync-marks: %d/%d - de vijand is %s van ons op de hoogte"),
					SyncMarks.Num(),
					CommandTuning != nullptr ? CommandTuning->MaxSyncStrikeMarks : 4,
					IsEnemyAwareOfSquad() ? TEXT("WEL") : TEXT("niet"));
				for (AActor* Target : GetSyncStrikeTargets())
				{
					UE_LOG(LogEclipse, Display, TEXT("    mark -> %s"), *GetNameSafe(Target));
				}
				for (const FSquadmateEntry& Soldier : Squadmates)
				{
					const AEclipseSquadmateController* Controller = Soldier.Controller.Get();
					if (Controller == nullptr)
					{
						continue;
					}
					UE_LOG(LogEclipse, Display, TEXT("    %s  doctrine %s  flank %s  %s"),
						*Soldier.SoldierId.ToString(EGuidFormats::Digits).Right(8),
						EclipseSquad::StanceLabel(Controller->GetDoctrine()),
						Controller->GetFlankStateLabel(),
						IsBodyConcealed(Controller->GetPawn()) ? TEXT("(ongezien)") : TEXT("(gezien)"));
				}
				int32 BreachPointCount = 0;
				for (TActorIterator<AEclipseBreachPoint> It(GetWorld()); It; ++It)
				{
					++BreachPointCount;
				}
				UE_LOG(LogEclipse, Display, TEXT("  breekpunten in de wereld: %d"), BreachPointCount);
			}),
			ECVF_Default);
	}

	RegisterStageBConsole();
#endif
}

void UEclipseSquadSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	if (DumpCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(DumpCommand);
		DumpCommand = nullptr;
	}
	for (IConsoleObject* Command : StageBCommands)
	{
		IConsoleManager::Get().UnregisterConsoleObject(Command);
	}
	StageBCommands.Reset();
#endif
	if (GetWorld() != nullptr)
	{
		GetWorld()->GetTimerManager().ClearTimer(MarkPruneTimer);
	}
	Super::Deinitialize();
}

const UEclipseSquadTuningAsset* UEclipseSquadSubsystem::ResolveTuning() const
{
	const UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	const UEclipseCampaignSubsystem* Campaign = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseCampaignSubsystem>() : nullptr;
	const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
	return Setup != nullptr ? Setup->SquadTuning.LoadSynchronous() : nullptr;
}

void UEclipseSquadSubsystem::RegisterSquadmate(AEclipseSquadmateController* Controller, const FGuid& SoldierId)
{
	if (Controller == nullptr)
	{
		return;
	}

	FSquadmateEntry& Entry = Squadmates.AddDefaulted_GetRef();
	Entry.Controller = Controller;
	Entry.SoldierId = SoldierId;

	// The body reports downs; the mission runtime resolves them at debrief
	// (SPEC-P1-06/07 pipeline) — wired here so spawning stays one call.
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(Controller->GetPawn()))
	{
		Body->SetSoldierId(SoldierId);
		// Weak controller capture: the body can outlive its controller during
		// teardown, and a raw pointer would dangle (GC never nulls captures).
		TWeakObjectPtr<AEclipseSquadmateController> WeakController(Controller);
		Body->OnDowned.AddWeakLambda(this, [this, WeakController](AEclipseCharacter* Downed, FName Cause)
		{
			if (AEclipseSquadmateController* AliveController = WeakController.Get())
			{
				AliveController->HandlePawnDowned();
			}

			UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
			if (GameInstance == nullptr)
			{
				return;
			}

			if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
			{
				FEclipseSquadEventPayload Payload;
				Payload.SoldierId = Downed->GetSoldierId();
				Payload.Cause = Cause;
				Bus->Broadcast(EclipseTags::Event_Squad_SoldierDowned, FInstancedStruct::Make(Payload));
			}
			if (UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>())
			{
				Mission->NotifySoldierDowned(Downed->GetSoldierId(), Cause);
			}

			// Auto-triage dispatch (SPEC-P2-01): the first standing squadmate
			// whose *data* says triage responds — the code never asks for a
			// Medic by name (GDD 14.2). Event-driven: the down is the trigger,
			// arrival is the next event, no per-frame scanning (GDD 12.4).
			DispatchTriage(Downed);
		});
	}
}

void UEclipseSquadSubsystem::DispatchTriage(AEclipseCharacter* DownedBody)
{
	if (DownedBody == nullptr || !DownedBody->IsDowned())
	{
		return;
	}
	for (const FSquadmateEntry& Candidate : Squadmates)
	{
		AEclipseSquadmateController* Responder = Candidate.Controller.Get();
		if (Responder != nullptr && Responder->BeginTriage(DownedBody))
		{
			UE_LOG(LogEclipse, Display, TEXT("[SQUAD Triage] %s moving to stabilize %s."),
				*Candidate.SoldierId.ToString().Left(8), *DownedBody->GetSoldierId().ToString().Left(8));
			break;
		}
	}
}

float UEclipseSquadSubsystem::WidestTriageWindowSeconds() const
{
	float Widest = 0.0f;
	for (const FSquadmateEntry& Entry : Squadmates)
	{
		const AEclipseSquadmateController* Controller = Entry.Controller.Get();
		if (Controller != nullptr && Controller->GetClassKit().bAutoTriage)
		{
			Widest = FMath::Max(Widest, Controller->GetClassKit().StabilizeWindowSeconds);
		}
	}
	return Widest;
}

void UEclipseSquadSubsystem::DispatchPendingTriage()
{
	UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	UEclipseMissionSubsystem* Mission = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseMissionSubsystem>() : nullptr;
	if (Mission == nullptr)
	{
		return;
	}

	const float PeekWindow = WidestTriageWindowSeconds();
	for (const FSquadmateEntry& Entry : Squadmates)
	{
		AEclipseSquadmateController* Controller = Entry.Controller.Get();
		AEclipseCharacter* Body = Controller != nullptr ? Cast<AEclipseCharacter>(Controller->GetPawn()) : nullptr;
		if (Body == nullptr || !Body->IsDowned())
		{
			continue;
		}
		// The peek uses the widest window on the squad; the attempt itself
		// still runs on the arriver's own kit (SPEC-P2-01). Saved or expired
		// casualties fail the peek — that is the anti-shuttle guarantee.
		if (!Mission->CanStabilizeSoldier(Body->GetSoldierId(), PeekWindow))
		{
			continue;
		}
		DispatchTriage(Body);
	}
}

void UEclipseSquadSubsystem::NotifyTriageArrived(AEclipseSquadmateController* Arriver, AEclipseCharacter* DownedBody)
{
	if (Arriver == nullptr || DownedBody == nullptr || !DownedBody->IsDowned())
	{
		return; // the patient recovered or despawned before arrival — nothing to report
	}

	UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	UEclipseMissionSubsystem* Mission = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseMissionSubsystem>() : nullptr;
	if (Mission == nullptr)
	{
		return;
	}

	const EclipseClassLogic::FEclipseResolvedClassKit& Kit = Arriver->GetClassKit();
	const AEclipseCharacter* ArriverBody = Cast<AEclipseCharacter>(Arriver->GetPawn());
	const FGuid StabilizerId = ArriverBody != nullptr ? ArriverBody->GetSoldierId() : FGuid();
	const FGuid PatientId = DownedBody->GetSoldierId();

	// The window check is pure timing against the arriving kit's data — a late
	// arrival is simply a death the squad will talk about (GDD 4.2.5).
	if (!Mission->TryStabilizeSoldier(PatientId, Kit.StabilizeWindowSeconds))
	{
		UE_LOG(LogEclipse, Display, TEXT("[SQUAD Triage] Too late for %s — the window closed (SPEC-P2-01)."),
			*PatientId.ToString().Left(8));
		return;
	}

	UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>();
	if (Bus == nullptr)
	{
		// The save is committed but nobody will hear it — a broken 9.5 promise
		// worth a loud log even though a missing game-instance bus is teardown-only.
		UE_LOG(LogEclipse, Warning, TEXT("[SQUAD Stabilize] Save of %s committed with no event bus to announce it (GDD 9.5)."),
			*PatientId.ToString().Left(8));
		return;
	}

	// Bark from data: the "Stabilize" row of the order-def table carries the
	// save lines; a missing row still speaks (silence is forbidden — GDD 9.5).
	const UEclipseSquadTuningAsset* Tuning = ResolveTuning();
	const UDataTable* OrderDefs = Tuning != nullptr ? Tuning->OrderDefs.LoadSynchronous() : nullptr;
	const FEclipseSquadOrderDefRow* BarkRow = OrderDefs != nullptr
		? OrderDefs->FindRow<FEclipseSquadOrderDefRow>(TEXT("Stabilize"), TEXT("SquadTriage"), /*bWarnIfMissing*/ false)
		: nullptr;
	const FString Bark = EclipseSquadOrderLogic::PickBarkLine(
		BarkRow != nullptr ? BarkRow->AcknowledgeLines : TArray<FString>(), StabilizerId, /*Salt*/ 500u);

	FEclipseSquadEventPayload Stabilized;
	Stabilized.SoldierId = PatientId;
	Stabilized.StabilizerId = StabilizerId;
	Stabilized.BarkLine = Bark;
	Bus->Broadcast(EclipseTags::Event_Squad_SoldierStabilized, FInstancedStruct::Make(Stabilized));

	// The stabilize *is* the signature verb firing (Class.Verb.Stabilize data).
	FEclipseSquadEventPayload Ability;
	Ability.SoldierId = StabilizerId;
	Ability.StabilizerId = StabilizerId;
	Ability.AbilityVerb = Kit.SignatureVerb;
	Bus->Broadcast(EclipseTags::Event_Squad_ClassAbilityUsed, FInstancedStruct::Make(Ability));

	UE_LOG(LogEclipse, Display, TEXT("[SQUAD Stabilize] %s"), *Bark);
}

TArray<FGuid> UEclipseSquadSubsystem::GetAliveSquadmateIds() const
{
	TArray<FGuid> Ids;
	Ids.Reserve(Squadmates.Num());
	for (const FSquadmateEntry& Entry : Squadmates)
	{
		const AEclipseSquadmateController* Controller = Entry.Controller.Get();
		const AEclipseCharacter* Body = Controller != nullptr ? Cast<AEclipseCharacter>(Controller->GetPawn()) : nullptr;
		if (Body != nullptr && !Body->IsDowned())
		{
			Ids.Add(Entry.SoldierId);
		}
	}
	return Ids;
}

bool UEclipseSquadSubsystem::IsRegisteredSquadmate(const FGuid& SoldierId) const
{
	for (const FSquadmateEntry& Entry : Squadmates)
	{
		if (Entry.SoldierId == SoldierId)
		{
			return true;
		}
	}
	return false;
}

void UEclipseSquadSubsystem::UnregisterAll()
{
	Squadmates.Reset();

	// The round-trip tally is run-scoped: mission teardown calls this, so the
	// next run's gauntlet numbers never inherit the previous run's worst case.
	OrderRoundTrip.Reset();

	// Markeringen zijn net zo goed run-scoped: doelen uit de vorige missie zijn
	// weg, en een set die de teardown overleeft zou de volgende run laten
	// beginnen met pips op lichamen die niet meer bestaan.
	ClearSyncStrikeMarks();
	WarnedMissingRefusalPools.Reset();
}

// ==========================================================================
// SPEC-P2-02 Stage B
// ==========================================================================

const UEclipseCommandModeTuningAsset* UEclipseSquadSubsystem::ResolveCommandTuning() const
{
	// HETZELFDE PAD als de mode-component, uit één constante. Twee letterlijke
	// paden zouden precies één keer uit elkaar lopen, en dan stelt de owner de
	// dilatatie af op asset A terwijl zijn sync-strike-cap uit asset B komt.
	static const FSoftObjectPath TuningPath(EclipseCommandMode::DefaultTuningPath);
	return Cast<UEclipseCommandModeTuningAsset>(TuningPath.TryLoad());
}

bool UEclipseSquadSubsystem::IsEnemyAwareOfSquad() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	// Het ALARM telt mee en niet alleen het zien: een missie die loud gaat door
	// een objective of een console-commando heeft de squad net zo goed verraden,
	// en de stealth-doctrine hoort daar hetzelfde op te reageren (SPEC-P2-04).
	const UGameInstance* GameInstance = World->GetGameInstance();
	if (const UEclipseMissionSubsystem* Mission = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseMissionSubsystem>() : nullptr)
	{
		if (Mission->IsAlarmRaised())
		{
			return true;
		}
	}

	for (TActorIterator<AEclipseEnemyController> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const AEclipseEnemyController* Enemy = *It;
		if (Enemy != nullptr && Enemy->HasSeenPlayerSide())
		{
			return true;
		}
	}
	return false;
}

bool UEclipseSquadSubsystem::IsBodyConcealed(const AActor* Body) const
{
	const UWorld* World = GetWorld();
	if (Body == nullptr || World == nullptr)
	{
		return false; // geen lichaam = geen belofte over onzichtbaarheid
	}
	for (TActorIterator<AEclipseEnemyController> It(const_cast<UWorld*>(World)); It; ++It)
	{
		const AEclipseEnemyController* Enemy = *It;
		if (Enemy == nullptr)
		{
			continue;
		}
		// Alleen vijanden die ons AL door hebben tellen mee. Een patrouille die
		// nog nergens van weet en toevallig in je richting kijkt, is precies de
		// situatie waar een sync strike voor bestaat.
		if (Enemy->HasSeenPlayerSide() && Enemy->CanSeeBody(Body))
		{
			return false;
		}
	}
	return true;
}

bool UEclipseSquadSubsystem::IsSyncStrikeTargetStillValid(const AActor* Target) const
{
	const AEclipseCharacter* Body = Cast<AEclipseCharacter>(Target);
	if (Body == nullptr || Body->IsDowned() || Body->IsPlayerSide())
	{
		return false;
	}
	// ONWETEND, per SPEC-P2-02 ("valid vs. Unaware enemies only"). Wie ons heeft
	// gezien, gaat niet meer stil om: dat is een vuurgevecht en geen strike.
	const AEclipseEnemyController* Controller = Cast<AEclipseEnemyController>(Body->GetController());
	return Controller == nullptr || !Controller->HasSeenPlayerSide();
}

bool UEclipseSquadSubsystem::ToggleSyncStrikeMark(AActor* Target, bool& bOutMarked)
{
	bOutMarked = false;
	if (!IsSyncStrikeTargetStillValid(Target))
	{
		return false;
	}

	// De id van een markering is van de MARKERING en niet van het lichaam:
	// vijanden hebben geen roster-id, en er een op plakken zou ze in de debrief
	// laten opduiken als soldaten. Eén guid per gemarkeerd lichaam, hergebruikt
	// zodat nog eens aanwijzen dezelfde markering uitzet.
	FGuid MarkId;
	for (const TPair<FGuid, TWeakObjectPtr<AActor>>& Pair : MarkedBodies)
	{
		if (Pair.Value.Get() == Target)
		{
			MarkId = Pair.Key;
			break;
		}
	}
	if (!MarkId.IsValid())
	{
		MarkId = FGuid::NewGuid();
	}

	const UEclipseCommandModeTuningAsset* Tuning = ResolveCommandTuning();
	const int32 MaxMarks = Tuning != nullptr ? Tuning->MaxSyncStrikeMarks : 4;

	bool bMarked = false;
	const bool bChanged = SyncMarks.ToggleMark(MarkId, MaxMarks, bMarked);
	bOutMarked = bMarked;

	if (!bChanged)
	{
		// De cap zat vol. Dat is het enige geval waarin een druk op de knop niets
		// doet, en dus het enige dat een zin verdient — stilte hier is precies de
		// stilte waarvan de speler denkt dat het spel hem negeert.
		BroadcastOrderQueued(FGuid(), TEXT("SyncStrike.MarkRejected"),
			ResolveQueuedBark(TEXT("SyncStrikeFull"), MarkId, /*Salt*/ 930u), FName(*GetNameSafe(Target)));
		return false;
	}

	if (bMarked)
	{
		MarkedBodies.Add(MarkId, Target);
		BroadcastOrderQueued(FGuid(), TEXT("SyncStrike.Marked"),
			ResolveQueuedBark(TEXT("SyncStrikeMark"), MarkId, /*Salt*/ 900u), FName(*GetNameSafe(Target)));

		// De snoeiklok loopt alleen zolang er markeringen staan (12.4).
		if (UWorld* World = GetWorld(); World != nullptr && !World->GetTimerManager().IsTimerActive(MarkPruneTimer))
		{
			World->GetTimerManager().SetTimer(MarkPruneTimer, this,
				&UEclipseSquadSubsystem::PruneSyncStrikeMarks, 0.5f, /*bLoop*/ true);
		}
	}
	else
	{
		MarkedBodies.Remove(MarkId);
		BroadcastOrderQueued(FGuid(), TEXT("SyncStrike.Unmarked"), FString(), FName(*GetNameSafe(Target)));
	}
	return true;
}

TArray<AActor*> UEclipseSquadSubsystem::GetSyncStrikeTargets() const
{
	TArray<AActor*> Targets;
	Targets.Reserve(SyncMarks.Num());
	for (const FGuid& Mark : SyncMarks.Marks)
	{
		if (const TWeakObjectPtr<AActor>* Body = MarkedBodies.Find(Mark))
		{
			if (AActor* Alive = Body->Get())
			{
				Targets.Add(Alive);
			}
		}
	}
	return Targets;
}

TArray<AActor*> UEclipseSquadSubsystem::GetAssignedSyncStrikeTargets(const FGuid& SoldierId) const
{
	TArray<AActor*> Assigned;
	const TArray<FGuid> Alive = GetAliveSquadmateIds();
	const int32 SoldierIndex = Alive.IndexOfByKey(SoldierId);
	if (SoldierIndex == INDEX_NONE)
	{
		return Assigned;
	}

	const TArray<AActor*> Targets = GetSyncStrikeTargets();
	for (const int32 MarkIndex : EclipseSquadOrderLogic::AssignSyncStrikeMarkIndices(SoldierIndex, Alive.Num(), Targets.Num()))
	{
		Assigned.Add(Targets[MarkIndex]);
	}
	return Assigned;
}

void UEclipseSquadSubsystem::ClearSyncStrikeMarks()
{
	SyncMarks.Reset();
	MarkedBodies.Reset();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MarkPruneTimer);
	}
}

void UEclipseSquadSubsystem::PruneSyncStrikeMarks()
{
	TArray<FGuid> StillValid;
	StillValid.Reserve(SyncMarks.Num());
	for (const FGuid& Mark : SyncMarks.Marks)
	{
		const TWeakObjectPtr<AActor>* Body = MarkedBodies.Find(Mark);
		if (Body != nullptr && IsSyncStrikeTargetStillValid(Body->Get()))
		{
			StillValid.Add(Mark);
		}
	}

	const int32 Pruned = SyncMarks.PruneMarks(StillValid);
	if (Pruned > 0)
	{
		// EEN PIP DIE VERDWIJNT IS EEN GEBEURTENIS. De debug-UI tekent de pips uit
		// deze stroom, dus hij moet ze er ook uit kunnen wissen — en de speler die
		// vier doelen dacht te hebben, hoort te merken dat er nog twee staan.
		for (auto It = MarkedBodies.CreateIterator(); It; ++It)
		{
			if (!SyncMarks.IsMarked(It.Key()))
			{
				BroadcastOrderQueued(FGuid(), TEXT("SyncStrike.Pruned"),
					ResolveQueuedBark(TEXT("SyncStrikePruned"), It.Key(), /*Salt*/ 910u),
					FName(*GetNameSafe(It.Value().Get())));
				It.RemoveCurrent();
			}
		}
	}

	if (SyncMarks.Num() == 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(MarkPruneTimer);
		}
	}
}

int32 UEclipseSquadSubsystem::GetSquadmateSlot(const FGuid& SoldierId) const
{
	return Squadmates.IndexOfByPredicate(
		[&SoldierId](const FSquadmateEntry& Candidate) { return Candidate.SoldierId == SoldierId; });
}

void UEclipseSquadSubsystem::NotifyBreachStacked(AEclipseSquadmateController* Arriver)
{
	const AEclipseBreachPoint* Point = Arriver != nullptr ? Arriver->GetBreachPoint() : nullptr;
	if (Point == nullptr)
	{
		return;
	}

	// SAMEN NAAR BINNEN. Zolang er nog iemand onderweg is op dit breekpunt,
	// gebeurt er niets — dat is de hele betekenis van "synchronized entry" op
	// sliceschaal: geen choreografie, wel dezelfde tel.
	TArray<AEclipseSquadmateController*> OnThisPoint;
	for (const FSquadmateEntry& Entry : Squadmates)
	{
		AEclipseSquadmateController* Controller = Entry.Controller.Get();
		if (Controller != nullptr && Controller->GetBreachPoint() == Point)
		{
			if (!Controller->IsBreachStacked())
			{
				return; // nog niet iedereen staat
			}
			OnThisPoint.Add(Controller);
		}
	}

	for (AEclipseSquadmateController* Controller : OnThisPoint)
	{
		const AEclipseCharacter* Body = Cast<AEclipseCharacter>(Controller->GetPawn());
		BroadcastOrderQueued(Body != nullptr ? Body->GetSoldierId() : FGuid(), TEXT("Breach.Entry"),
			ResolveQueuedBark(TEXT("BreachEntry"), Body != nullptr ? Body->GetSoldierId() : FGuid(), /*Salt*/ 940u));
		Controller->ExecuteBreachEntry();
	}
}

FString UEclipseSquadSubsystem::ResolveQueuedBark(FName RowName, const FGuid& SoldierId, uint32 Salt) const
{
	const UEclipseSquadTuningAsset* Tuning = ResolveTuning();
	const UDataTable* OrderDefs = Tuning != nullptr ? Tuning->OrderDefs.LoadSynchronous() : nullptr;
	const FEclipseSquadOrderDefRow* Row = OrderDefs != nullptr
		? OrderDefs->FindRow<FEclipseSquadOrderDefRow>(RowName, TEXT("SquadOrderQueued"), /*bWarnIfMissing*/ false)
		: nullptr;
	// Klaarstaande orders spreken uit de ACK-pool: er is niets geweigerd, er staat
	// iets klaar. Ontbrekende rij = de stockzin uit PickBarkLine, nooit stilte.
	return EclipseSquadOrderLogic::PickBarkLine(
		Row != nullptr ? Row->AcknowledgeLines : TArray<FString>(), SoldierId, Salt);
}

void UEclipseSquadSubsystem::BroadcastOrderQueued(const FGuid& SoldierId, FName Transition, const FString& BarkLine, FName TargetId)
{
	UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	UEclipseEventBusSubsystem* Bus = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr;
	if (Bus == nullptr)
	{
		return;
	}

	FEclipseSquadEventPayload Payload;
	Payload.SoldierId = SoldierId;
	Payload.Order = Transition;
	Payload.TargetId = TargetId;
	Payload.BarkLine = BarkLine;
	Bus->Broadcast(EclipseTags::Event_Squad_OrderQueued, FInstancedStruct::Make(Payload));

	if (!BarkLine.IsEmpty())
	{
		UE_LOG(LogEclipse, Display, TEXT("[SQUAD %s] %s"), *Transition.ToString(), *BarkLine);
	}
}

TArray<FString> UEclipseSquadSubsystem::GetOrderStateLines() const
{
	// DE NAAM KOMT UIT DE ROSTER, en dat is de enige bron die hem heeft.
	// FSquadmateEntry draagt bewust alleen een id + controller: wie de naam hier
	// zou kopiëren, heeft hem twee keer en dus vroeg of laat twee verschillende.
	const UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	const UEclipseCampaignSubsystem* Campaign = GameInstance != nullptr
		? GameInstance->GetSubsystem<UEclipseCampaignSubsystem>() : nullptr;

	TArray<FString> Lines;
	for (const FSquadmateEntry& Entry : Squadmates)
	{
		const AEclipseSquadmateController* Controller = Entry.Controller.Get();
		const FString Order = Controller != nullptr
			? UEnum::GetValueAsString(Controller->GetCurrentOrder()).RightChop(FString(TEXT("EEclipseSquadOrder::")).Len())
			: TEXT("(lost)");

		const FEclipseSoldierRecord* Record = Campaign != nullptr
			? Campaign->GetState().FindSoldier(Entry.SoldierId) : nullptr;
		Lines.Add(EclipseSquadOrderLogic::ComposeOrderStateLine(
			Record != nullptr ? Record->Name : FString(), Entry.SoldierId, Order));
	}
	return Lines;
}

bool UEclipseSquadSubsystem::IssueOrder(const FGuid& SoldierId, EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor, EEclipseSquadStance Stance)
{
	FSquadmateEntry* Entry = Squadmates.FindByPredicate(
		[&SoldierId](const FSquadmateEntry& Candidate) { return Candidate.SoldierId == SoldierId; });
	AEclipseSquadmateController* Controller = Entry != nullptr ? Entry->Controller.Get() : nullptr;
	if (Controller == nullptr)
	{
		// Warning, not Error: a Command Mode selection that outlived its soldier
		// lands here by design and falls back to the broadcast path (SPEC-P2-02).
		UE_LOG(LogEclipse, Warning, TEXT("IssueOrder: no squadmate with id %s — caller falls back to all"), *SoldierId.ToString());
		return false;
	}

	// R3 criterion 1 (the ≤1 s answer bar, GDD 8.4): stamped on the WALL clock —
	// Command Mode dilates the world to 0.30, and a game-time stamp would flatter
	// the measurement by the dilation factor (SPEC-P2-02 locked decision 2).
	// Unknown-id early-outs above never reach this: they are caller bugs, not
	// orders, and must not enter the tally as free successes.
	const double IssuedWallSeconds = EclipseSquadOrderLogic::NowWallSeconds();

	BroadcastOrderEvent(EclipseTags::Event_Squad_OrderIssued, SoldierId, Order, FString(), EEclipseOrderRefusalReason::None);

	const EclipseSquadOrderLogic::FEclipseOrderDecision Decision = Controller->ExecuteOrder(Order, TargetLocation, TargetActor, Stance);

	// Bark pools from data; the order id doubles as the row key.
	const UEclipseSquadTuningAsset* Tuning = ResolveTuning();
	const UDataTable* OrderDefs = Tuning != nullptr ? Tuning->OrderDefs.LoadSynchronous() : nullptr;
	const FName OrderRowName(*UEnum::GetValueAsString(Order).RightChop(FString(TEXT("EEclipseSquadOrder::")).Len()));
	const FEclipseSquadOrderDefRow* Row = OrderDefs != nullptr
		? OrderDefs->FindRow<FEclipseSquadOrderDefRow>(OrderRowName, TEXT("SquadOrder"), /*bWarnIfMissing*/ false)
		: nullptr;

	if (Decision.bAccepted)
	{
		const FString Bark = EclipseSquadOrderLogic::PickBarkLine(
			Row != nullptr ? Row->AcknowledgeLines : TArray<FString>(), SoldierId, static_cast<uint32>(Order));
		BroadcastOrderEvent(EclipseTags::Event_Squad_OrderAcknowledged, SoldierId, Order, Bark, EEclipseOrderRefusalReason::None);
	}
	else
	{
		// SOMMIGE REDENEN HEBBEN EEN EIGEN POOL, en dat begon met Downed
		// (owner-beslissing 26-07, optie 1): een neergeschoten soldaat weigerde een
		// MoveTo met "No route, boss." — een routeprobleem dat er niet was, terwijl
		// hij op de grond lag.
		//
		// SPEC-P2-02 Stage B maakt daar de REGEL van in plaats van de uitzondering,
		// want de drie nieuwe redenen lopen exact in dezelfde val: "geen breekpunt"
		// is geen "ik kom er niet", en "ze zien me" is geen "er staat niets
		// gemarkeerd". Welke redenen een eigen pool hebben, staat puur in
		// RefusalPoolRowName — één plek, testbaar, en niet vier ifs die uit elkaar
		// groeien.
		const FEclipseSquadOrderDefRow* PoolRow = Row;
		const FName ReasonRowName = EclipseSquadOrderLogic::RefusalPoolRowName(Decision.Reason);
		if (!ReasonRowName.IsNone())
		{
			const FEclipseSquadOrderDefRow* ReasonRow = OrderDefs != nullptr
				? OrderDefs->FindRow<FEclipseSquadOrderDefRow>(ReasonRowName, TEXT("SquadOrderReason"), /*bWarnIfMissing*/ false)
				: nullptr;
			if (ReasonRow != nullptr)
			{
				PoolRow = ReasonRow;
			}
			else if (!WarnedMissingRefusalPools.Contains(ReasonRowName))
			{
				// Terugval op de orderzin is beter dan zwijgen, maar het is precies
				// de verwarrende zin waar dit voor gebouwd is — dus luid (14.3.5).
				// Eén keer per reden, niet per order: een waarschuwing die per
				// weigering herhaalt, begraaft zichzelf.
				WarnedMissingRefusalPools.Add(ReasonRowName);
				UE_LOG(LogEclipse, Warning,
					TEXT("Squad: DT_SquadOrderDefs mist de rij '%s' — die weigering leent nu de zin van het ORDERTYPE, ")
					TEXT("en die wijst de speler op het verkeerde probleem. Draai Tools/create_phase1_content.py."),
					*ReasonRowName.ToString());
			}
		}

		const FString Bark = EclipseSquadOrderLogic::PickBarkLine(
			PoolRow != nullptr ? PoolRow->RefusalLines : TArray<FString>(), SoldierId, static_cast<uint32>(Order) + 100u);

		// A refusal is an answer (GDD 8.4), and it is the answer most worth
		// reading back: acknowledgements are the expected case, refusals are the
		// ones that explain why an order did nothing. Only the refusal branch
		// logs — logging every ack would bury it.
		UE_LOG(LogEclipse, Display, TEXT("Squad: %s REFUSED order %s (reason: %s) — \"%s\""),
			*SoldierId.ToString(EGuidFormats::DigitsWithHyphensLower).Left(8), *OrderRowName.ToString(),
			*UEnum::GetValueAsString(Decision.Reason).RightChop(FString(TEXT("EEclipseOrderRefusalReason::")).Len()),
			*Bark);

		BroadcastOrderEvent(EclipseTags::Event_Squad_OrderRefused, SoldierId, Order, Bark, Decision.Reason);
	}

	// Measured after the answer broadcast, not after the decision: the criterion
	// is "the tester hears an answer", and the bark rides that broadcast. Both
	// branches pass through here — a refusal is an answer (GDD 8.4).
	OrderRoundTrip.NoteRoundTrip(EclipseSquadOrderLogic::NowWallSeconds() - IssuedWallSeconds);
	return true;
}

void UEclipseSquadSubsystem::IssueOrderToAll(EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor, EEclipseSquadStance Stance)
{
	// Copy: entries never mutate mid-issue today, but order handlers may re-enter.
	const TArray<FSquadmateEntry> EntrySnapshot = Squadmates;
	for (const FSquadmateEntry& Entry : EntrySnapshot)
	{
		IssueOrder(Entry.SoldierId, Order, TargetLocation, TargetActor, Stance);
	}
}

void UEclipseSquadSubsystem::BroadcastOrderEvent(const FGameplayTag& Tag, const FGuid& SoldierId, EEclipseSquadOrder Order, const FString& BarkLine, EEclipseOrderRefusalReason Reason)
{
	UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	UEclipseEventBusSubsystem* Bus = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr;
	if (Bus == nullptr)
	{
		return;
	}

	FEclipseSquadEventPayload Payload;
	Payload.SoldierId = SoldierId;
	Payload.Order = FName(*UEnum::GetValueAsString(Order));
	Payload.BarkLine = BarkLine;
	Payload.Reason = Reason == EEclipseOrderRefusalReason::None ? NAME_None : FName(*UEnum::GetValueAsString(Reason));
	Bus->Broadcast(Tag, FInstancedStruct::Make(Payload));

	if (!BarkLine.IsEmpty())
	{
		// Text-on-screen stub for VO (SPEC-P1-06 debug bark surface).
		UE_LOG(LogEclipse, Display, TEXT("[SQUAD %s] %s"), *Payload.Order.ToString(), *BarkLine);
	}
}

// ==========================================================================
// SPEC-P2-02 Stage B — de debug-driver (14.5 stap 4).
//
// Console en geen toetsen, en dat is een GRENS en geen luiheid: de definitieve
// bindings horen in de Enhanced Input foot/command-contextstapel (SPEC-P2-07),
// en de invoerlaag zelf ligt bij een andere bouwer. Wat hier moet kunnen is dat
// alle acht 8.4-rijen in PIE echt uitgeven, bevestigen of gesproken weigeren —
// en dat kan hiermee, vandaag, zonder een toets te claimen die morgen verhuist.
// ==========================================================================

AEclipsePlayerController* UEclipseSquadSubsystem::GetPlayerController() const
{
	UWorld* World = GetWorld();
	return World != nullptr ? Cast<AEclipsePlayerController>(World->GetFirstPlayerController()) : nullptr;
}

void UEclipseSquadSubsystem::RegisterStageBConsole()
{
#if !UE_BUILD_SHIPPING
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Squad.Order")) != nullptr)
	{
		return; // een tweede wereld (PIE + editor) registreert niet nog eens
	}

	StageBCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Eclipse.Squad.Order"),
		TEXT("Geef een order uit de 8.4-tabel op je richtpunt: Eclipse.Squad.Order <MoveTo|FocusTarget|Hold|Regroup|Suppress|Flank|Breach|UseAbility|SyncStrike> [soldaatindex]. Zonder index: iedereen (SPEC-P2-02)."),
		FConsoleCommandWithArgsDelegate::CreateWeakLambda(this, [this](const TArray<FString>& Args)
		{
			if (Args.Num() < 1)
			{
				UE_LOG(LogEclipse, Warning, TEXT("Eclipse.Squad.Order <verb> [soldaatindex]"));
				return;
			}

			const UEnum* OrderEnum = StaticEnum<EEclipseSquadOrder>();
			const int64 Value = OrderEnum != nullptr ? OrderEnum->GetValueByNameString(Args[0]) : INDEX_NONE;
			if (Value == INDEX_NONE)
			{
				UE_LOG(LogEclipse, Warning, TEXT("Onbekende order '%s'."), *Args[0]);
				return;
			}
			const EEclipseSquadOrder Order = static_cast<EEclipseSquadOrder>(Value);

			// Het RICHTPUNT van de speler, via dezelfde dubbele trace die Command
			// Mode gebruikt — geen tweede doelbepaling naast de bestaande.
			FVector AimLocation = FVector::ZeroVector;
			AActor* AimActor = nullptr;
			if (AEclipsePlayerController* Controller = GetPlayerController())
			{
				Controller->GetAimPoint(AimLocation, AimActor);
			}

			if (Args.Num() >= 2)
			{
				const int32 Index = FCString::Atoi(*Args[1]);
				const TArray<FGuid> Alive = GetAliveSquadmateIds();
				if (!Alive.IsValidIndex(Index))
				{
					UE_LOG(LogEclipse, Warning, TEXT("Soldaatindex %d bestaat niet (%d overeind)."), Index, Alive.Num());
					return;
				}
				IssueOrder(Alive[Index], Order, AimLocation, AimActor);
				return;
			}
			IssueOrderToAll(Order, AimLocation, AimActor);
		}),
		ECVF_Default));

	StageBCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Eclipse.Squad.SetDoctrine"),
		TEXT("Zet de doctrine van de hele squad: recon | ready | overwatch | aggressive | stealth (SPEC-P2-02 Stage B)."),
		FConsoleCommandWithArgsDelegate::CreateWeakLambda(this, [this](const TArray<FString>& Args)
		{
			EEclipseSquadStance Stance = EEclipseSquadStance::Ready;
			if (Args.Num() < 1 || !EclipseSquad::ParseStance(Args[0], Stance))
			{
				UE_LOG(LogEclipse, Warning, TEXT("Eclipse.Squad.SetDoctrine <recon|ready|overwatch|aggressive|stealth>"));
				return;
			}
			for (const FSquadmateEntry& Entry : Squadmates)
			{
				if (AEclipseSquadmateController* Controller = Entry.Controller.Get())
				{
					Controller->SetDoctrine(Stance);
				}
			}
			UE_LOG(LogEclipse, Display, TEXT("Squad: doctrine %s bij %d man."),
				EclipseSquad::StanceLabel(Stance), Squadmates.Num());
		}),
		ECVF_Default));

	StageBCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Eclipse.Squad.MarkTarget"),
		TEXT("Markeer/ontmarkeer het doel onder je vizier voor een sync strike (SPEC-P2-02 Stage B)."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
		{
			FVector AimLocation = FVector::ZeroVector;
			AActor* AimActor = nullptr;
			if (AEclipsePlayerController* Controller = GetPlayerController())
			{
				Controller->GetAimPoint(AimLocation, AimActor);
			}
			bool bMarked = false;
			if (!ToggleSyncStrikeMark(AimActor, bMarked))
			{
				UE_LOG(LogEclipse, Display, TEXT("Sync strike: %s is geen stil doel (dood, van ons, of hij ziet ons al)."),
					*GetNameSafe(AimActor));
				return;
			}
			UE_LOG(LogEclipse, Display, TEXT("Sync strike: %s %s (%d gemarkeerd)."),
				*GetNameSafe(AimActor), bMarked ? TEXT("gemarkeerd") : TEXT("losgelaten"), SyncMarks.Num());
		}),
		ECVF_Default));

	StageBCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Eclipse.Squad.SpawnBreachPoint"),
		TEXT("Zet een breekpunt op je richtpunt, gericht van jou af. Debug-authoring tot er deurframes in de graybox staan (SPEC-P2-02 Stage B)."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
		{
			UWorld* World = GetWorld();
			AEclipsePlayerController* Controller = GetPlayerController();
			if (World == nullptr || Controller == nullptr)
			{
				return;
			}
			FVector AimLocation = FVector::ZeroVector;
			AActor* AimActor = nullptr;
			Controller->GetAimPoint(AimLocation, AimActor);

			// De doorgang wijst VAN de speler AF: je breekt in waar je naar kijkt.
			const APawn* Pawn = Controller->GetPawn();
			const FRotator Facing = Pawn != nullptr
				? (AimLocation - Pawn->GetActorLocation()).GetSafeNormal2D().Rotation()
				: FRotator::ZeroRotator;

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (const AEclipseBreachPoint* Point = World->SpawnActor<AEclipseBreachPoint>(
					AEclipseBreachPoint::StaticClass(), AimLocation, Facing, Params))
			{
				UE_LOG(LogEclipse, Display, TEXT("Breekpunt geplaatst op %s."), *Point->GetActorLocation().ToCompactString());
			}
		}),
		ECVF_Default));
#endif
}
