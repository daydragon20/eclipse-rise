#include "Squad/EclipseSquadSubsystem.h"

#include "AI/EclipseSquadmateController.h"
#include "Characters/EclipseCharacter.h"
#include "Core/EclipseEventBusSubsystem.h"
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
			}),
			ECVF_Default);
	}
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
#endif
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
}

TArray<FString> UEclipseSquadSubsystem::GetOrderStateLines() const
{
	TArray<FString> Lines;
	for (const FSquadmateEntry& Entry : Squadmates)
	{
		const AEclipseSquadmateController* Controller = Entry.Controller.Get();
		const FString Order = Controller != nullptr
			? UEnum::GetValueAsString(Controller->GetCurrentOrder()).RightChop(FString(TEXT("EEclipseSquadOrder::")).Len())
			: TEXT("(lost)");
		Lines.Add(FString::Printf(TEXT("%s  ->  %s"), *Entry.SoldierId.ToString().Left(8), *Order));
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
		const FString Bark = EclipseSquadOrderLogic::PickBarkLine(
			Row != nullptr ? Row->RefusalLines : TArray<FString>(), SoldierId, static_cast<uint32>(Order) + 100u);

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
