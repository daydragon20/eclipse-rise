#include "Quests/EclipseMissionSubsystem.h"

#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "HAL/IConsoleManager.h"
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

	FString Error;
	if (!StartMission(Prep->SquadSoldierIds, Error))
	{
		UE_LOG(LogEclipse, Error, TEXT("Mission launch rejected: %s"), *Error);
	}
}

void UEclipseMissionSubsystem::ResolveMissionSpec(FName TemplateId, TArray<FEclipseObjectiveDef>& OutObjectives, bool& bOutProgressRegion) const
{
	const FString AssetPath = FString::Printf(TEXT("/Game/Data/Missions/%s.%s"), *TemplateId.ToString(), *TemplateId.ToString());
	if (const UEclipseMissionAsset* Asset = Cast<UEclipseMissionAsset>(FSoftObjectPath(AssetPath).TryLoad()))
	{
		OutObjectives = Asset->Objectives;
		bOutProgressRegion = Asset->bProgressRegionOnSuccess;
		return;
	}

	// Missing content = warning + playable default, never a crash (GDD 14.3.5):
	// one mandatory objective and an extraction keep the loop testable.
	UE_LOG(LogEclipse, Warning, TEXT("Mission template '%s' has no asset at %s — synthesizing the default spec."), *TemplateId.ToString(), *AssetPath);

	FEclipseObjectiveDef& Primary = OutObjectives.AddDefaulted_GetRef();
	Primary.ObjectiveId = TEXT("Obj_Primary");
	Primary.Type = EEclipseObjectiveType::ReachLocation;
	Primary.Description = FText::FromString(TEXT("Reach the objective site"));

	FEclipseObjectiveDef& Exfil = OutObjectives.AddDefaulted_GetRef();
	Exfil.ObjectiveId = TEXT("Obj_Exfil");
	Exfil.Type = EEclipseObjectiveType::ExtractSquad;
	Exfil.Description = FText::FromString(TEXT("Extract the squad"));

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
	ResolveMissionSpec(PendingTemplateId, ActiveObjectives, bProgressRegionOnSuccess);
	DeployedSoldierIds = SquadSoldierIds;

	Phase = EEclipseMissionPhase::Insertion;
	BroadcastMissionEvent(EclipseTags::Event_Mission_Started, NAME_None, false);

	// No level actors drive insertion yet: advance immediately so the loop is
	// playable headless. The graybox insertion trigger takes over this
	// transition when the district level lands.
	check(EclipseMissionLogic::CanAdvancePhase(EEclipseMissionPhase::Insertion, EEclipseMissionPhase::Objectives));
	Phase = EEclipseMissionPhase::Objectives;

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
	}
	return true;
}

void UEclipseMissionSubsystem::NotifySoldierDowned(const FGuid& SoldierId, FName Cause)
{
	if (Phase == EEclipseMissionPhase::None || Phase == EEclipseMissionPhase::Finished)
	{
		return;
	}
	DownedSoldiers.Add(SoldierId, Cause);
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
		DownedSoldiers, Campaign->GetState(), bSuccess && RosterTuning != nullptr, WoundedDaysOut);

	const FEclipseCampaignTransaction Consequences = EclipseMissionLogic::ComposeConsequences(
		LastOutcome, PendingRewards, Casualties, Campaign->GetState(),
		EclipseTags::Resource_Credits.GetTag(),
		EclipseTags::Resource_Materials.GetTag(),
		EclipseTags::Resource_Intel.GetTag(),
		bProgressRegionOnSuccess);

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

void UEclipseMissionSubsystem::ResetRuntime()
{
	ActiveObjectives.Reset();
	CompletedObjectiveIds.Reset();
	DeployedSoldierIds.Reset();
	DownedSoldiers.Reset();
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
