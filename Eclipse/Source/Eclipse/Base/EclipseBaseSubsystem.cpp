#include "Base/EclipseBaseSubsystem.h"

#include "Base/EclipseBaseTypes.h"
#include "Base/EclipseVaultBuilder.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"

void UEclipseBaseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The campaign ledger must exist before any order can commit; the bus
	// before that so commit facts are never dropped (same chain as Economy).
	UEclipseEventBusSubsystem* Bus = Collection.InitializeDependency<UEclipseEventBusSubsystem>();
	Collection.InitializeDependency<UEclipseCampaignSubsystem>();

	if (Bus != nullptr)
	{
		// One family subscription carries all four Event.Base.* facts (the
		// strategy-map-widget / economy-family pattern). Every one of them can
		// change what a chamber must show - started (scaffold on), built/upgraded
		// (state swap + global growth tier), staff assigned/released (idlers) -
		// so the vault re-renders on the family and never on a tick (GDD 12.4).
		// No ExpectedPayloadType: this consumer reads the committed state, not the
		// payload, so it stays uncoupled from the payload struct (like the other
		// family subscriptions in the project).
		const FGameplayTag BaseFamily = EclipseTags::Event_Base_FacilityBuilt.GetTag().RequestDirectParent();
		BaseEventsHandle = Bus->Subscribe(
			BaseFamily,
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseBaseSubsystem::OnBaseFact));
	}

	RegisterConsoleCommands();
}

void UEclipseBaseSubsystem::Deinitialize()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
		{
			Bus->Unsubscribe(BaseEventsHandle);
		}
	}

	UnregisterConsoleCommands();
	Super::Deinitialize();
}

UEclipseCampaignSubsystem* UEclipseBaseSubsystem::GetCampaign() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseCampaignSubsystem>() : nullptr;
}

const UEclipseCampaignSetupAsset* UEclipseBaseSubsystem::ResolveSetup() const
{
	const UEclipseCampaignSubsystem* Campaign = GetCampaign();
	return Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
}

const UEclipseBaseLayoutAsset* UEclipseBaseSubsystem::ResolveLayout() const
{
	const UEclipseCampaignSetupAsset* Setup = ResolveSetup();
	const UEclipseBaseLayoutAsset* Layout = Setup != nullptr ? Setup->BaseLayout.LoadSynchronous() : nullptr;
	if (Layout == nullptr && !bWarnedMissingLayout)
	{
		bWarnedMissingLayout = true;
		UE_LOG(LogEclipse, Warning, TEXT("Base: no layout asset linked in the campaign setup — build orders reject, the loop never gates (GDD 14.3.5)."));
	}
	return Layout;
}

const UDataTable* UEclipseBaseSubsystem::ResolveFacilitiesTable() const
{
	const UEclipseCampaignSetupAsset* Setup = ResolveSetup();
	const UDataTable* Table = Setup != nullptr ? Setup->Facilities.LoadSynchronous() : nullptr;
	if (Table == nullptr && !bWarnedMissingFacilities)
	{
		bWarnedMissingFacilities = true;
		UE_LOG(LogEclipse, Warning, TEXT("Base: no DT_Facilities linked in the campaign setup — facilities have no data (GDD 14.3.5)."));
	}
	return Table;
}

const FEclipseFacilityRow* UEclipseBaseSubsystem::FindFacilityRow(FName FacilityId) const
{
	const UDataTable* Table = ResolveFacilitiesTable();
	return Table != nullptr ? Table->FindRow<FEclipseFacilityRow>(FacilityId, TEXT("EclipseBase"), /*bWarnIfMissing*/ false) : nullptr;
}

EclipseBaseLogic::FEclipseBaseTuningParams UEclipseBaseSubsystem::ResolveTuningParams() const
{
	EclipseBaseLogic::FEclipseBaseTuningParams Params; // defaults = SPEC-P2-03 spec values
	const UEclipseCampaignSetupAsset* Setup = ResolveSetup();
	if (const UEclipseBaseTuningAsset* Tuning = Setup != nullptr ? Setup->BaseTuning.LoadSynchronous() : nullptr)
	{
		Params.RushCostCreditsPerDay = Tuning->RushCostCreditsPerDay;
		Params.CrewDayReduction = Tuning->CrewDayReduction;
		Params.AnalystIntelBonusPerDay = Tuning->AnalystIntelBonusPerDay;
		Params.MaxCrewPerSite = Tuning->MaxCrewPerSite;
		Params.AnalystBonusResource = Tuning->AnalystBonusResource;
		if (!Params.AnalystBonusResource.IsValid() && !bWarnedNoAnalystResource)
		{
			bWarnedNoAnalystResource = true;
			UE_LOG(LogEclipse, Warning, TEXT("Base: DA_BaseTuning has no AnalystBonusResource — analysts grant no bonus (GDD 14.3.5)."));
		}
	}
	else
	{
		// No asset: the analyst bonus keys off Intel per the spec table.
		Params.AnalystBonusResource = EclipseTags::Resource_Intel.GetTag();
	}
	return Params;
}

EclipseBaseLogic::FEclipseFacilityYieldParams UEclipseBaseSubsystem::ComputeTodaysFacilityYields() const
{
	const UEclipseCampaignSubsystem* Campaign = GetCampaign();
	if (Campaign == nullptr)
	{
		return EclipseBaseLogic::FEclipseFacilityYieldParams();
	}

	const UDataTable* Table = ResolveFacilitiesTable();
	return EclipseBaseLogic::ComputeFacilityYields(
		Campaign->GetState().BaseState, ResolveTuningParams(),
		[Table](FName FacilityId) -> const FEclipseFacilityRow*
		{
			return Table != nullptr ? Table->FindRow<FEclipseFacilityRow>(FacilityId, TEXT("EclipseBaseYields"), /*bWarnIfMissing*/ false) : nullptr;
		});
}

bool UEclipseBaseSubsystem::TryStartConstruction(FName FacilityId, FString& OutError)
{
	const UEclipseBaseLayoutAsset* Layout = ResolveLayout();
	if (Layout == nullptr)
	{
		OutError = TEXT("Build: no base layout data");
		return false;
	}

	// Placement is authored per slot; order is free (locked decision 2). Try
	// each slot that allows this facility and keep the most useful rejection.
	OutError = FString::Printf(TEXT("Build: no slot allows facility '%s'"), *FacilityId.ToString());
	for (const FEclipseBaseSlotDef& Slot : Layout->Slots)
	{
		if (!Slot.AllowedFacilityRows.Contains(FacilityId))
		{
			continue;
		}
		FString SlotError;
		if (TryStartConstructionAtSlot(Slot.SlotId, FacilityId, SlotError))
		{
			return true;
		}
		OutError = SlotError;
	}
	return false;
}

bool UEclipseBaseSubsystem::TryStartConstructionAtSlot(FName SlotId, FName FacilityId, FString& OutError)
{
	UEclipseCampaignSubsystem* Campaign = GetCampaign();
	const UEclipseBaseLayoutAsset* Layout = ResolveLayout();
	if (Campaign == nullptr || Layout == nullptr)
	{
		OutError = TEXT("Build: no campaign or base layout data");
		return false;
	}

	const FEclipseCampaignState& State = Campaign->GetState();
	const FEclipseFacilityRow* Row = FindFacilityRow(FacilityId);

	int32 TargetLevel = 0;
	if (!EclipseBaseLogic::ValidateBuildOrder(State.BaseState, Layout->Slots, SlotId, FacilityId, Row,
		State.GetBalance(EclipseTags::Resource_Materials), State.GetBalance(EclipseTags::Resource_Credits),
		TargetLevel, OutError))
	{
		return false;
	}

	const FEclipseFacilityLevelData* LevelData = EclipseBaseLogic::GetLevelData(Row, TargetLevel);
	check(LevelData != nullptr); // ValidateBuildOrder guarantees the level entry

	// Spend + start, atomic: the ledger's insufficient-funds rejection stays
	// the hard gate (P1-03 pattern), and Event.Base.ConstructionStarted plus
	// the wallet facts all leave this one commit (GDD 14.3.3).
	FEclipseCampaignTransaction Transaction;
	Transaction.Source = TEXT("BaseBuild");
	const FName Reason(*FString::Printf(TEXT("Build_%s"), *FacilityId.ToString()));
	if (LevelData->CostMaterials > 0)
	{
		FEclipseCampaignMutation& Spend = Transaction.Mutations.AddDefaulted_GetRef();
		Spend.Type = EEclipseCampaignMutationType::AdjustResource;
		Spend.ResourceType = EclipseTags::Resource_Materials.GetTag();
		Spend.Amount = -LevelData->CostMaterials;
		Spend.Reason = Reason;
	}
	if (LevelData->CostCredits > 0)
	{
		FEclipseCampaignMutation& Spend = Transaction.Mutations.AddDefaulted_GetRef();
		Spend.Type = EEclipseCampaignMutationType::AdjustResource;
		Spend.ResourceType = EclipseTags::Resource_Credits.GetTag();
		Spend.Amount = -LevelData->CostCredits;
		Spend.Reason = Reason;
	}
	FEclipseCampaignMutation& Start = Transaction.Mutations.AddDefaulted_GetRef();
	Start.Type = EEclipseCampaignMutationType::StartConstruction;
	Start.SlotId = SlotId;
	Start.FacilityId = FacilityId;
	Start.EtaDays = LevelData->BuildDays;
	Start.Reason = Reason;

	return Campaign->CommitTransaction(Transaction, OutError);
}

bool UEclipseBaseSubsystem::TryRushConstruction(FName SlotId, FString& OutError)
{
	UEclipseCampaignSubsystem* Campaign = GetCampaign();
	if (Campaign == nullptr)
	{
		OutError = TEXT("Rush: no campaign subsystem");
		return false;
	}

	const FEclipseCampaignState& State = Campaign->GetState();
	const FEclipseFacilityState* Facility = State.BaseState.FindBySlot(SlotId);
	const EclipseBaseLogic::FEclipseBaseTuningParams Tuning = ResolveTuningParams();
	if (!EclipseBaseLogic::ValidateRush(Facility, Tuning, State.GetBalance(EclipseTags::Resource_Credits), OutError))
	{
		return false;
	}

	// Spend + instant completion in ONE commit: FacilityBuilt/Upgraded fires
	// with the rush mutation (SPEC-P2-03 clock rules, spec lines 100-105).
	FEclipseCampaignTransaction Transaction;
	Transaction.Source = TEXT("BaseRush");
	const int32 Cost = EclipseBaseLogic::ComputeRushCost(Facility, Tuning);
	if (Cost > 0)
	{
		FEclipseCampaignMutation& Spend = Transaction.Mutations.AddDefaulted_GetRef();
		Spend.Type = EEclipseCampaignMutationType::AdjustResource;
		Spend.ResourceType = EclipseTags::Resource_Credits.GetTag();
		Spend.Amount = -Cost;
		Spend.Reason = FName(*FString::Printf(TEXT("Rush_%s"), *Facility->FacilityId.ToString()));
	}
	FEclipseCampaignMutation& Rush = Transaction.Mutations.AddDefaulted_GetRef();
	Rush.Type = EEclipseCampaignMutationType::RushConstruction;
	Rush.SlotId = SlotId;

	return Campaign->CommitTransaction(Transaction, OutError);
}

bool UEclipseBaseSubsystem::TryAssignStaff(FName SlotId, const FGuid& SoldierId, FString& OutError)
{
	UEclipseCampaignSubsystem* Campaign = GetCampaign();
	if (Campaign == nullptr)
	{
		OutError = TEXT("Staff: no campaign subsystem");
		return false;
	}

	const FEclipseCampaignState& State = Campaign->GetState();
	const FEclipseFacilityState* Facility = State.BaseState.FindBySlot(SlotId);
	const EclipseBaseLogic::FEclipseBaseTuningParams Tuning = ResolveTuningParams();

	// The cap is data (DA_BaseTuning.MaxCrewPerSite). This is the friendly
	// pre-check; the hard gate is the mutation-layer validation running on the
	// stamped cap inside the commit (step-3 review) — over-assignment past the
	// cap would lock a soldier for zero effect.
	if (Facility != nullptr && Facility->AssignedSoldierIds.Num() >= FMath::Max(0, Tuning.MaxCrewPerSite))
	{
		OutError = FString::Printf(TEXT("Staff: slot '%s' is fully staffed (%d)"), *SlotId.ToString(), Facility->AssignedSoldierIds.Num());
		return false;
	}
	if (!EclipseBaseLogic::ValidateStaffChange(State.BaseState, SlotId, SoldierId, /*bAssign*/ true, OutError))
	{
		return false;
	}

	FEclipseCampaignTransaction Transaction;
	Transaction.Source = TEXT("BaseStaff");
	FEclipseCampaignMutation& Assign = Transaction.Mutations.AddDefaulted_GetRef();
	Assign.Type = EEclipseCampaignMutationType::AssignStaff;
	Assign.SlotId = SlotId;
	Assign.SoldierId = SoldierId;
	// Intent tag = the positional role the site holds right now; the commit
	// re-derives it at apply time (the mutation's validity flag is the switch).
	Assign.StaffRoleTag = (Facility != nullptr && Facility->DaysRemaining > 0)
		? EclipseTags::Base_Staff_Crew.GetTag()
		: EclipseTags::Base_Staff_Analyst.GetTag();

	return Campaign->CommitTransaction(Transaction, OutError);
}

bool UEclipseBaseSubsystem::TryUnassignStaff(FName SlotId, const FGuid& SoldierId, FString& OutError)
{
	UEclipseCampaignSubsystem* Campaign = GetCampaign();
	if (Campaign == nullptr)
	{
		OutError = TEXT("Staff: no campaign subsystem");
		return false;
	}
	if (!EclipseBaseLogic::ValidateStaffChange(Campaign->GetState().BaseState, SlotId, SoldierId, /*bAssign*/ false, OutError))
	{
		return false;
	}

	FEclipseCampaignTransaction Transaction;
	Transaction.Source = TEXT("BaseStaff");
	FEclipseCampaignMutation& Unassign = Transaction.Mutations.AddDefaulted_GetRef();
	Unassign.Type = EEclipseCampaignMutationType::AssignStaff;
	Unassign.SlotId = SlotId;
	Unassign.SoldierId = SoldierId;
	// Empty role tag = unassign (SPEC-P2-03: "none = unassign").

	return Campaign->CommitTransaction(Transaction, OutError);
}

UWorld* UEclipseBaseSubsystem::GetVaultWorld() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance != nullptr ? GameInstance->GetWorld() : nullptr;
}

void UEclipseBaseSubsystem::EnsureVaultPresent()
{
	RefreshVault(/*bForceRebuild*/ true);
}

void UEclipseBaseSubsystem::OnBaseFact(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	// A fact is never an invitation to spawn a vault (bForceRebuild = false):
	// only a vault that already stands in this world re-renders.
	RefreshVault(/*bForceRebuild*/ false);
}

void UEclipseBaseSubsystem::RefreshVault(bool bForceRebuild)
{
	UWorld* World = GetVaultWorld();
	if (World == nullptr)
	{
		if (bForceRebuild)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Vault: no world to render into — the vault stays unbuilt (GDD 14.3.5)."));
		}
		return;
	}

	// Cheapest gate first, and deliberately BEFORE any asset resolve: a mission
	// world (or a menu world) carries no vault, so a Base fact must not even
	// touch the layout asset there - no work, no warnings, nothing to leak.
	if (!bForceRebuild && !EclipseVault::IsVaultPresent(*World))
	{
		RenderedVaultPlanHash = 0;
		return;
	}

	const UEclipseCampaignSubsystem* Campaign = GetCampaign();
	if (Campaign == nullptr)
	{
		if (bForceRebuild)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Vault: no campaign subsystem — nothing to render the vault from (GDD 14.3.5)."));
		}
		return;
	}

	// ResolveLayout() logs the missing-asset warning once; BuildVault then stands
	// the vault up empty (anchor only) instead of crashing (GDD 14.3.5).
	const FEclipseBaseState& BaseState = Campaign->GetState().BaseState;
	const UEclipseBaseLayoutAsset* Layout = ResolveLayout();

	// The coalescer: one commit emits up to four Event.Base.* facts against the
	// same post-commit state, so the first fact renders the final vault and the
	// rest hash identically and cost nothing. A null layout hashes to 0 - the
	// empty-vault render - so the loud warning above stays a once-per-change log.
	const uint32 PlanHash = Layout != nullptr
		? EclipseVault::ComputePlanHash(EclipseVault::PlanSlots(Layout->Slots, BaseState))
		: 0u;
	if (!bForceRebuild && PlanHash == RenderedVaultPlanHash)
	{
		return;
	}

	EclipseVault::RebuildVault(*World, Layout, BaseState);
	RenderedVaultPlanHash = PlanHash;
}

void UEclipseBaseSubsystem::LogBaseReport() const
{
	const UEclipseCampaignSubsystem* Campaign = GetCampaign();
	if (Campaign == nullptr)
	{
		return;
	}
	const FEclipseCampaignState& State = Campaign->GetState();
	const EclipseBaseLogic::FEclipseBaseTuningParams Tuning = ResolveTuningParams();
	const UEclipseBaseLayoutAsset* Layout = ResolveLayout();

	UE_LOG(LogEclipse, Display, TEXT("Hollow Point report — day %d (M %d / C %d / I %d):"),
		State.Day,
		State.GetBalance(EclipseTags::Resource_Materials),
		State.GetBalance(EclipseTags::Resource_Credits),
		State.GetBalance(EclipseTags::Resource_Intel));

	// Layout slots first (authored order); state-only rows (layout missing or
	// stale save) still print below so the report never hides state.
	TArray<FName> Reported;
	if (Layout != nullptr)
	{
		for (const FEclipseBaseSlotDef& Slot : Layout->Slots)
		{
			Reported.Add(Slot.SlotId);
			const FEclipseFacilityState* Facility = State.BaseState.FindBySlot(Slot.SlotId);
			if (Facility == nullptr || (Facility->Level < 1 && Facility->DaysRemaining <= 0))
			{
				const FString Allowed = FString::JoinBy(Slot.AllowedFacilityRows, TEXT(", "), [](const FName& N) { return N.ToString(); });
				UE_LOG(LogEclipse, Display, TEXT("  %s: empty (allowed: %s)"), *Slot.SlotId.ToString(), *Allowed);
				continue;
			}
			if (Facility->DaysRemaining > 0)
			{
				UE_LOG(LogEclipse, Display, TEXT("  %s: %s L%d -> L%d, %d day(s) left (ETA day %d, rush %d C), staff %d"),
					*Slot.SlotId.ToString(), *Facility->FacilityId.ToString(), Facility->Level, Facility->Level + 1,
					Facility->DaysRemaining, State.Day + Facility->DaysRemaining,
					EclipseBaseLogic::ComputeRushCost(Facility, Tuning), Facility->AssignedSoldierIds.Num());
			}
			else
			{
				UE_LOG(LogEclipse, Display, TEXT("  %s: %s L%d operational, staff %d"),
					*Slot.SlotId.ToString(), *Facility->FacilityId.ToString(), Facility->Level, Facility->AssignedSoldierIds.Num());
			}
		}
	}
	for (const FEclipseFacilityState& Facility : State.BaseState.Facilities)
	{
		if (!Reported.Contains(Facility.SlotId))
		{
			UE_LOG(LogEclipse, Display, TEXT("  %s (no layout slot!): %s L%d, %d day(s) left, staff %d"),
				*Facility.SlotId.ToString(), *Facility.FacilityId.ToString(), Facility.Level, Facility.DaysRemaining, Facility.AssignedSoldierIds.Num());
		}
	}
}

void UEclipseBaseSubsystem::RegisterConsoleCommands()
{
#if !UE_BUILD_SHIPPING
	IConsoleManager& Console = IConsoleManager::Get();

	// Same multi-instance guard as the campaign subsystem: first instance wins.
	if (Console.FindConsoleObject(TEXT("Eclipse.Base.Report")) != nullptr)
	{
		return;
	}

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Base.Report"),
		TEXT("Log the Hollow Point slots, construction states, ETAs and staff (SPEC-P2-03 Gauntlet surface)."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
		{
			LogBaseReport();
		}),
		ECVF_Default));

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Base.Vault"),
		TEXT("Render the walkable Hollow Point vault from today's campaign state (SPEC-P2-03 step 4-5 parity loop); re-renders an existing one."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
		{
			EnsureVaultPresent();
		}),
		ECVF_Default));

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Base.Build"),
		TEXT("Usage: Eclipse.Base.Build <FacilityId> [SlotId] — validated build/upgrade order (SPEC-P2-03)."),
		FConsoleCommandWithArgsDelegate::CreateWeakLambda(this, [this](const TArray<FString>& Args)
		{
			if (Args.Num() < 1 || Args.Num() > 2)
			{
				UE_LOG(LogEclipse, Error, TEXT("Usage: Eclipse.Base.Build <FacilityId> [SlotId]"));
				return;
			}
			FString Error;
			const bool bOk = Args.Num() == 2
				? TryStartConstructionAtSlot(FName(*Args[1]), FName(*Args[0]), Error)
				: TryStartConstruction(FName(*Args[0]), Error);
			if (!bOk)
			{
				UE_LOG(LogEclipse, Error, TEXT("Base.Build failed: %s"), *Error);
			}
		}),
		ECVF_Default));
#endif
}

void UEclipseBaseSubsystem::UnregisterConsoleCommands()
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
