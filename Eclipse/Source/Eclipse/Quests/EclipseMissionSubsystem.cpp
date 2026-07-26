#include "Quests/EclipseMissionSubsystem.h"

#include "Characters/EclipseClassLogic.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
#include "Quests/EclipseStoryTypes.h"
#include "Squad/EclipseRosterLogic.h"
#include "Squad/EclipseRosterTypes.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseStrategySubsystem.h"
#include "StructUtils/InstancedStruct.h"

void UEclipseMissionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UEclipseEventBusSubsystem* Bus = Collection.InitializeDependency<UEclipseEventBusSubsystem>();
	Collection.InitializeDependency<UEclipseCampaignSubsystem>();
	Collection.InitializeDependency<UEclipseStrategySubsystem>();

	if (Bus != nullptr)
	{
		MissionSelectedHandle = Bus->Subscribe(
			EclipseTags::Event_Strategy_MissionSelected,
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseMissionSubsystem::OnMissionSelected),
			FEclipseStrategyEventPayload::StaticStruct());

		LaunchRequestedHandle = Bus->Subscribe(
			EclipseTags::Event_Prep_MissionLaunchRequested,
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseMissionSubsystem::OnLaunchRequested),
			FEclipsePrepEventPayload::StaticStruct());
	}

	RegisterConsoleCommands();
}

void UEclipseMissionSubsystem::Deinitialize()
{
	UnregisterConsoleCommands();

	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		Bus->Unsubscribe(MissionSelectedHandle);
		Bus->Unsubscribe(LaunchRequestedHandle);
	}

	Super::Deinitialize();
}

void UEclipseMissionSubsystem::OnMissionSelected(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseStrategyEventPayload* Strategy = Payload.GetPtr<FEclipseStrategyEventPayload>();
	if (Strategy == nullptr)
	{
		return;
	}

	// A mid-mission selection must not repoint the pending offer: the debrief
	// resolves rewards AND the story completion beat by PendingTemplateId, and
	// beats are permanent campaign state — the wrong row would commit the
	// wrong story fact. Selection re-opens once the runtime is at rest.
	if (Phase != EEclipseMissionPhase::None && Phase != EEclipseMissionPhase::Finished)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Mission: selection for region '%s' ignored — a mission is in flight (GDD 14.3.5)."), *Strategy->RegionId.ToString());
		return;
	}

	PendingRegionId = Strategy->RegionId;
	PendingTemplateId = Strategy->TemplateId;
	PendingRewards = FEclipseMissionRewards();
	bHasPendingOffer = true;

	FEclipseMissionOfferView Offer;
	if (const UEclipseStrategySubsystem* StrategySubsystem = GetGameInstance()->GetSubsystem<UEclipseStrategySubsystem>();
		StrategySubsystem != nullptr && StrategySubsystem->TryGetOffer(Strategy->RegionId, Offer))
	{
		PendingRewards.Credits = Offer.RewardCredits;
		PendingRewards.Materials = Offer.RewardMaterials;
		PendingRewards.Intel = Offer.RewardIntel;
	}
}

void UEclipseMissionSubsystem::OnLaunchRequested(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipsePrepEventPayload* Prep = Payload.GetPtr<FEclipsePrepEventPayload>();
	if (Prep == nullptr)
	{
		return;
	}

	// DE GEKOZEN LOADOUT ONTHOUDEN (26-07 avond, punt 5). Hij zat al in dit feit
	// en werd door niemand gelezen: je koos netjes, de keuze werd netjes
	// gevalideerd en verzonden, en het wapen in je handen was altijd de eerste rij
	// van DT_Weapons. Dit is de ontbrekende schakel, niet een nieuw systeem.
	ActiveLoadoutTag = Prep->LoadoutTag;

	FString Error;
	if (!StartMission(Prep->SquadSoldierIds, Error))
	{
		UE_LOG(LogEclipse, Error, TEXT("Mission launch rejected: %s"), *Error);
	}
}

void UEclipseMissionSubsystem::ResolveMissionSpec(FName TemplateId, TArray<FEclipseObjectiveDef>& OutObjectives, bool& bOutProgressRegion,
	TArray<FEclipseEnemySpawnSet>& OutEnemySpawns) const
{
	OutEnemySpawns.Reset();
	const FString AssetPath = FString::Printf(TEXT("/Game/Data/Missions/%s.%s"), *TemplateId.ToString(), *TemplateId.ToString());
	if (const UEclipseMissionAsset* Asset = Cast<UEclipseMissionAsset>(FSoftObjectPath(AssetPath).TryLoad()))
	{
		OutObjectives = Asset->Objectives;
		bOutProgressRegion = Asset->bProgressRegionOnSuccess;
		OutEnemySpawns = Asset->EnemySpawns;
		return;
	}

	// Missing content = warning + playable default, never a crash (GDD 14.3.5):
	// one mandatory objective and an extraction keep the loop testable.
	UE_LOG(LogEclipse, Warning, TEXT("Mission template '%s' has no asset at %s — synthesizing the default spec."), *TemplateId.ToString(), *AssetPath);

	FEclipseObjectiveDef& Primary = OutObjectives.AddDefaulted_GetRef();
	Primary.ObjectiveId = TEXT("Obj_Primary");
	Primary.Type = EEclipseObjectiveType::ReachLocation;
	Primary.Description = FText::FromString(TEXT("Reach the objective site"));
	Primary.TargetId = TEXT("Site_ControlPost"); // graybox site the trigger reports (SPEC-P1-05)

	FEclipseObjectiveDef& Exfil = OutObjectives.AddDefaulted_GetRef();
	Exfil.ObjectiveId = TEXT("Obj_Exfil");
	Exfil.Type = EEclipseObjectiveType::ExtractSquad;
	Exfil.Description = FText::FromString(TEXT("Extract the squad"));
	Exfil.TargetId = TEXT("Site_Extraction"); // reaching this ends the run

	bOutProgressRegion = true;
}

bool UEclipseMissionSubsystem::StartMission(const TArray<FGuid>& SquadSoldierIds, FString& OutError)
{
	if (Phase != EEclipseMissionPhase::None && Phase != EEclipseMissionPhase::Finished)
	{
		OutError = TEXT("A mission is already running");
		return false;
	}
	if (!bHasPendingOffer)
	{
		OutError = TEXT("No mission selected on the strategy map");
		return false;
	}

	ResetRuntime();
	ResolveMissionSpec(PendingTemplateId, ActiveObjectives, bProgressRegionOnSuccess, ActiveEnemySpawns);
	DeployedSoldierIds = SquadSoldierIds;

	Phase = EEclipseMissionPhase::Insertion;
	BroadcastMissionEvent(EclipseTags::Event_Mission_Started, NAME_None, false);
	BroadcastPhaseChanged(EclipseMissionLogic::GetPhaseName(Phase), /*bAuthoredSubPhase*/ false);

	// No level actors drive insertion yet: advance immediately so the loop is
	// playable headless. The graybox insertion trigger takes over this
	// transition when the district level lands.
	check(EclipseMissionLogic::CanAdvancePhase(EEclipseMissionPhase::Insertion, EEclipseMissionPhase::Objectives));
	Phase = EEclipseMissionPhase::Objectives;
	BroadcastPhaseChanged(EclipseMissionLogic::GetPhaseName(Phase), /*bAuthoredSubPhase*/ false);

	UE_LOG(LogEclipse, Display, TEXT("Mission '%s' started at region '%s' (%d objectives, %d squad)."),
		*PendingTemplateId.ToString(), *PendingRegionId.ToString(), ActiveObjectives.Num(), DeployedSoldierIds.Num());
	return true;
}

bool UEclipseMissionSubsystem::CompleteObjective(FName ObjectiveId, FString& OutError)
{
	if (Phase != EEclipseMissionPhase::Objectives && Phase != EEclipseMissionPhase::Extraction)
	{
		OutError = TEXT("No mission objectives are active");
		return false;
	}

	const FEclipseObjectiveDef* Objective = ActiveObjectives.FindByPredicate(
		[ObjectiveId](const FEclipseObjectiveDef& O) { return O.ObjectiveId == ObjectiveId; });
	if (Objective == nullptr)
	{
		OutError = FString::Printf(TEXT("Unknown objective '%s'"), *ObjectiveId.ToString());
		return false;
	}
	if (CompletedObjectiveIds.Contains(ObjectiveId))
	{
		OutError = FString::Printf(TEXT("Objective '%s' already complete"), *ObjectiveId.ToString());
		return false;
	}

	CompletedObjectiveIds.Add(ObjectiveId);
	BroadcastMissionEvent(EclipseTags::Event_Mission_ObjectiveCompleted, ObjectiveId, false);

	if (Phase == EEclipseMissionPhase::Objectives
		&& EclipseMissionLogic::AreMandatoryObjectivesComplete(ActiveObjectives, CompletedObjectiveIds))
	{
		Phase = EEclipseMissionPhase::Extraction;
		UE_LOG(LogEclipse, Display, TEXT("All mandatory objectives complete — extraction is open."));
		BroadcastPhaseChanged(EclipseMissionLogic::GetPhaseName(Phase), /*bAuthoredSubPhase*/ false);
	}
	return true;
}

void UEclipseMissionSubsystem::NotifySiteEntered(FName SiteId)
{
	const FEclipseObjectiveDef* Objective = ActiveObjectives.FindByPredicate(
		[SiteId](const FEclipseObjectiveDef& O) { return O.TargetId == SiteId; });
	if (Objective == nullptr)
	{
		return; // unbound site — sites outlive missions, that is not an error
	}
	if (Objective->Type == EEclipseObjectiveType::DestroyTarget)
	{
		// Aanwezigheid vervult dit type niet. Luid en één keer per site, want een
		// speler die hier staat en niets ziet gebeuren verdient een reden — dit is
		// precies het soort stilte dat de speelronde moet vinden (GDD 14.3.5).
		if (!SitesReportedAsPresenceOnly.Contains(SiteId))
		{
			SitesReportedAsPresenceOnly.Add(SiteId);
			UE_LOG(LogEclipse, Verbose,
				TEXT("Objective '%s' op site '%s' is een DestroyTarget: aanwezigheid vinkt hem niet af, het doel moet neer."),
				*Objective->ObjectiveId.ToString(), *SiteId.ToString());
		}
		return;
	}
	CompleteObjectiveByTarget(SiteId);
}

void UEclipseMissionSubsystem::CompleteObjectiveByTarget(FName TargetId)
{
	const FEclipseObjectiveDef* Objective = ActiveObjectives.FindByPredicate(
		[TargetId](const FEclipseObjectiveDef& O) { return O.TargetId == TargetId; });
	if (Objective == nullptr || CompletedObjectiveIds.Contains(Objective->ObjectiveId))
	{
		return; // unbound site or repeat entry — not an error, sites outlive missions
	}

	const bool bWasExtraction = Objective->Type == EEclipseObjectiveType::ExtractSquad;
	FString Error;
	CompleteObjective(Objective->ObjectiveId, Error);

	if (bWasExtraction)
	{
		// Reaching extraction ends the run (SPEC-P1-05 loop): success if the
		// mandatory set is now complete, else fail-forward (GDD 11.4 — never a wall).
		const bool bSuccess = EclipseMissionLogic::AreMandatoryObjectivesComplete(ActiveObjectives, CompletedObjectiveIds);
		if (!ResolveDebrief(bSuccess, Error))
		{
			UE_LOG(LogEclipse, Warning, TEXT("Extraction debrief rejected: %s"), *Error);
		}
	}
}

void UEclipseMissionSubsystem::NotifyAlarmRaised()
{
	if (Phase == EEclipseMissionPhase::None || Phase == EEclipseMissionPhase::Finished)
	{
		return; // no run to alarm — alert sources outlive missions, mirroring NotifySoldierDowned
	}
	if (bAlarmRaised)
	{
		return; // latched: the site is already loud — a second horn is not a new fact (idempotent by spec)
	}

	bAlarmRaised = true;
	UE_LOG(LogEclipse, Display, TEXT("Mission: alarm raised — ghost optionals are lost, the mission is not (GDD 11.4)."));
	BroadcastPhaseChanged(EclipseMissionLogic::AlarmSubPhaseName, /*bAuthoredSubPhase*/ true);
}

void UEclipseMissionSubsystem::NotifySoldierDowned(const FGuid& SoldierId, FName Cause)
{
	NotifySoldierDownedAt(SoldierId, Cause, -1.0);
}

void UEclipseMissionSubsystem::NotifySoldierDownedAt(const FGuid& SoldierId, FName Cause, double AtSeconds)
{
	if (Phase == EEclipseMissionPhase::None || Phase == EEclipseMissionPhase::Finished)
	{
		return;
	}
	DownedSoldiers.Add(SoldierId, Cause);

	// The down opens its stabilize window (SPEC-P2-01). Only the first down of
	// a run starts the clock for that soldier — a re-report cannot extend it.
	if (!DownedAtSeconds.Contains(SoldierId))
	{
		const UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
		DownedAtSeconds.Add(SoldierId, AtSeconds >= 0.0 ? AtSeconds : (World != nullptr ? World->GetTimeSeconds() : 0.0));
	}
}

bool UEclipseMissionSubsystem::TryStabilizeSoldier(const FGuid& SoldierId, float WindowSeconds, double AtSeconds)
{
	if (!CanStabilizeSoldier(SoldierId, WindowSeconds, AtSeconds))
	{
		return false;
	}
	StabilizedSoldiers.Add(SoldierId);
	return true;
}

bool UEclipseMissionSubsystem::CanStabilizeSoldier(const FGuid& SoldierId, float WindowSeconds, double AtSeconds) const
{
	if (Phase == EEclipseMissionPhase::None || Phase == EEclipseMissionPhase::Finished)
	{
		return false;
	}
	const double* DownedAt = DownedAtSeconds.Find(SoldierId);
	if (DownedAt == nullptr || StabilizedSoldiers.Contains(SoldierId))
	{
		return false; // not down, or already saved — one save per down
	}

	const UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	const double Now = AtSeconds >= 0.0 ? AtSeconds : (World != nullptr ? World->GetTimeSeconds() : 0.0);
	return EclipseClassLogic::IsStabilizeWindowOpen(*DownedAt, Now, WindowSeconds);
}

bool UEclipseMissionSubsystem::ResolveDebrief(bool bSuccess, FString& OutError)
{
	if (Phase != EEclipseMissionPhase::Objectives && Phase != EEclipseMissionPhase::Extraction)
	{
		OutError = TEXT("No mission to debrief");
		return false;
	}

	// Success requires the mandatory set; ForceEnd-win with objectives open is a
	// scripting error worth rejecting loudly rather than committing a lie.
	if (bSuccess && !EclipseMissionLogic::AreMandatoryObjectivesComplete(ActiveObjectives, CompletedObjectiveIds))
	{
		OutError = TEXT("Cannot resolve success: mandatory objectives incomplete");
		return false;
	}

	Phase = EEclipseMissionPhase::Debrief;
	BroadcastPhaseChanged(EclipseMissionLogic::GetPhaseName(Phase), /*bAuthoredSubPhase*/ false);

	UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	if (Campaign == nullptr)
	{
		OutError = TEXT("No campaign subsystem");
		return false;
	}

	LastOutcome = FEclipseMissionOutcome();
	LastOutcome.TemplateId = PendingTemplateId;
	LastOutcome.RegionId = PendingRegionId;
	LastOutcome.bSuccess = bSuccess;
	LastOutcome.CompletedObjectiveIds = CompletedObjectiveIds;
	LastOutcome.DeployedSoldierIds = DeployedSoldierIds;
	DownedSoldiers.GenerateKeyArray(LastOutcome.DownedSoldierIds);
	LastOutcome.bAlarmRaised = bAlarmRaised;

	// Voided optionals stay visible as "missed" (SPEC-P2-04) via the same pure
	// evaluation the payout uses, so debrief UI and wallet can never disagree.
	// DownedSoldierIds retains stabilized soldiers by design — the zero-
	// casualty latch reads "ever went down", the save only changes resolution.
	TArray<FEclipseObjectiveDef> PaidOptionals;
	EclipseMissionLogic::EvaluateOptionalObjectives(ActiveObjectives, CompletedObjectiveIds,
		bAlarmRaised, !LastOutcome.DownedSoldierIds.IsEmpty(), PaidOptionals, LastOutcome.MissedOptionalObjectiveIds);

	// Casualty resolution policy (SPEC-P1-07): downed on a won mission comes home
	// Wounded (days from data); downed on a failed mission is dead — the
	// extraction-without-body stub. Missing tuning degrades to the conservative
	// all-dead reading with a warning (GDD 14.3.5).
	int32 WoundedDaysOut = 0;
	const UEclipseCampaignSetupAsset* Setup = Campaign->GetActiveSetup();
	const UEclipseRosterTuningAsset* RosterTuning = Setup != nullptr ? Setup->RosterTuning.LoadSynchronous() : nullptr;
	if (RosterTuning != nullptr)
	{
		WoundedDaysOut = RosterTuning->WoundedDaysOut;
	}
	else if (!DownedSoldiers.IsEmpty())
	{
		UE_LOG(LogEclipse, Warning, TEXT("Debrief: no roster tuning — downed soldiers resolve as dead (GDD 14.3.5)."));
	}

	const TArray<FEclipseResolvedCasualty> Casualties = EclipseRosterLogic::ResolveCasualties(
		DownedSoldiers, Campaign->GetState(), bSuccess && RosterTuning != nullptr, WoundedDaysOut, StabilizedSoldiers);

	// Story completion beat (SPEC-P2-04): a won story mission's row supplies
	// the beat the debrief commits; generic missions simply have no row, so a
	// miss here is the common case and stays silent — table-health warnings
	// (wrong struct, orphan pins, empty beats) live in the strategy resolver.
	FGameplayTag CompletionBeat;
	if (bSuccess && Setup != nullptr)
	{
		const UDataTable* StoryTable = Setup->StoryMissions.LoadSynchronous();
		if (StoryTable != nullptr && StoryTable->GetRowStruct() != nullptr &&
			StoryTable->GetRowStruct()->IsChildOf(FEclipseStoryMissionRow::StaticStruct()))
		{
			// First-wins mirrors the resolver's tie rule (table order decides);
			// the None-guard keeps a half-authored row from matching a generic
			// offer that carries no template id.
			bool bBeatRowFound = false;
			StoryTable->ForeachRow<FEclipseStoryMissionRow>(TEXT("DebriefBeat"),
				[&CompletionBeat, &bBeatRowFound, this](const FName&, const FEclipseStoryMissionRow& Row)
				{
					if (!bBeatRowFound && !Row.MissionId.IsNone() && Row.MissionId == PendingTemplateId)
					{
						CompletionBeat = Row.CompletionBeatTag;
						bBeatRowFound = true;
					}
				});
		}
	}

	const FEclipseCampaignTransaction Consequences = EclipseMissionLogic::ComposeConsequences(
		LastOutcome, PendingRewards, Casualties, Campaign->GetState(),
		EclipseTags::Resource_Credits.GetTag(),
		EclipseTags::Resource_Materials.GetTag(),
		EclipseTags::Resource_Intel.GetTag(),
		bProgressRegionOnSuccess,
		CompletionBeat,
		ActiveObjectives);

	if (!Consequences.Mutations.IsEmpty())
	{
		if (!Campaign->CommitTransaction(Consequences, OutError))
		{
			// A rejected debrief means composition produced an invalid proposal —
			// surface it hard; silently dropping consequences is the divergence bug
			// this architecture exists to kill (GDD 12.2 rule 4).
			UE_LOG(LogEclipse, Error, TEXT("Debrief transaction rejected: %s"), *OutError);
			return false;
		}
	}

	BroadcastMissionEvent(bSuccess ? EclipseTags::Event_Mission_Completed : EclipseTags::Event_Mission_Failed, NAME_None, bSuccess);

	// The start of a mission is logged; the end was not, and that asymmetry hides
	// the half that matters most. A self-playing round found it the hard way: the
	// player walked into the checkpoint, went down, and the mission failed at
	// 31.5 s with the event bus as the only witness — no log line, no reason. A
	// run that ends says why it ended (GDD 14.3.5: degrade loudly).
	UE_LOG(LogEclipse, Display, TEXT("Mission '%s' %s at region '%s' (%d/%d objectives, %d/%d squad down%s)."),
		*PendingTemplateId.ToString(), bSuccess ? TEXT("SUCCEEDED") : TEXT("FAILED"), *PendingRegionId.ToString(),
		CompletedObjectiveIds.Num(), ActiveObjectives.Num(), DownedSoldiers.Num(), DeployedSoldierIds.Num(),
		bAlarmRaised ? TEXT(", alarm raised") : TEXT(""));

	Phase = EEclipseMissionPhase::Finished;
	bHasPendingOffer = false;
	return true;
}

void UEclipseMissionSubsystem::BroadcastMissionEvent(const FGameplayTag& Tag, FName ObjectiveId, bool bSuccess)
{
	UEclipseEventBusSubsystem* Bus = GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>();
	if (Bus == nullptr)
	{
		return;
	}

	FEclipseMissionEventPayload Payload;
	Payload.MissionId = PendingTemplateId;
	Payload.ObjectiveId = ObjectiveId;
	Payload.bSuccess = bSuccess;
	Bus->Broadcast(Tag, FInstancedStruct::Make(Payload));
}

void UEclipseMissionSubsystem::BroadcastPhaseChanged(FName PhaseName, bool bAuthoredSubPhase)
{
	UEclipseEventBusSubsystem* Bus = GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>();
	if (Bus == nullptr)
	{
		return;
	}

	FEclipseMissionEventPayload Payload;
	Payload.MissionId = PendingTemplateId;
	Payload.PhaseName = PhaseName;
	Payload.bAuthoredSubPhase = bAuthoredSubPhase;
	Bus->Broadcast(EclipseTags::Event_Mission_PhaseChanged, FInstancedStruct::Make(Payload));
}

void UEclipseMissionSubsystem::ResetRuntime()
{
	ActiveObjectives.Reset();
	CompletedObjectiveIds.Reset();
	DeployedSoldierIds.Reset();
	DownedSoldiers.Reset();
	DownedAtSeconds.Reset();
	StabilizedSoldiers.Reset();
	SitesReportedAsPresenceOnly.Reset();
	bAlarmRaised = false;
	bProgressRegionOnSuccess = true;
	Phase = EEclipseMissionPhase::None;
}

void UEclipseMissionSubsystem::RegisterConsoleCommands()
{
#if !UE_BUILD_SHIPPING
	IConsoleManager& Console = IConsoleManager::Get();
	if (Console.FindConsoleObject(TEXT("Eclipse.Mission.CompleteObjective")) != nullptr)
	{
		return;
	}

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Mission.CompleteObjective"),
		TEXT("Usage: Eclipse.Mission.CompleteObjective <ObjectiveId> — Gauntlet scripting surface (SPEC-P1-05)."),
		FConsoleCommandWithArgsDelegate::CreateWeakLambda(this, [this](const TArray<FString>& Args)
		{
			FString Error;
			if (Args.Num() != 1 || !CompleteObjective(FName(*Args[0]), Error))
			{
				UE_LOG(LogEclipse, Error, TEXT("CompleteObjective: %s"), Args.Num() == 1 ? *Error : TEXT("usage: <ObjectiveId>"));
			}
		}),
		ECVF_Default));

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Mission.RaiseAlarm"),
		TEXT("Usage: Eclipse.Mission.RaiseAlarm — trip the run's alarm latch (SPEC-P2-04; idempotent, Gauntlet surface until enemy alert code lands)."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
		{
			NotifyAlarmRaised();
		}),
		ECVF_Default));

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Mission.ForceEnd"),
		TEXT("Usage: Eclipse.Mission.ForceEnd <win|lose> — resolve the debrief by script (SPEC-P1-05)."),
		FConsoleCommandWithArgsDelegate::CreateWeakLambda(this, [this](const TArray<FString>& Args)
		{
			FString Error;
			if (Args.Num() != 1 || !ResolveDebrief(Args[0].Equals(TEXT("win"), ESearchCase::IgnoreCase), Error))
			{
				UE_LOG(LogEclipse, Error, TEXT("ForceEnd: %s"), Args.Num() == 1 ? *Error : TEXT("usage: <win|lose>"));
			}
		}),
		ECVF_Default));
#endif
}

void UEclipseMissionSubsystem::UnregisterConsoleCommands()
{
#if !UE_BUILD_SHIPPING
	for (IConsoleObject* Command : ConsoleCommands)
	{
		if (Command != nullptr)
		{
			IConsoleManager::Get().UnregisterConsoleObject(Command);
		}
	}
	ConsoleCommands.Reset();
#endif
}
