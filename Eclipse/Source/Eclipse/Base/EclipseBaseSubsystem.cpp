#include "Base/EclipseBaseSubsystem.h"

#include "Base/EclipseBaseTypes.h"
#include "Base/EclipseVaultBuilder.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Characters/EclipseCharacter.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
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

#if !UE_BUILD_SHIPPING
	// The review-frame rig, and only when explicitly asked for on the command
	// line. It arms on the loaded map rather than here, because at Initialize
	// there is no world, no pawn and nothing to photograph.
	if (FParse::Param(FCommandLine::Get(), TEXT("EclipseVaultShot")))
	{
		VaultShotMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddWeakLambda(this, [this](UWorld* World)
		{
			SetupVaultShotRig(World);
		});
	}
#endif
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

#if !UE_BUILD_SHIPPING
	// Unsubscribed for the same reason the game mode's shot-fired handle is:
	// a leak you can see and leave is a choice, not an inheritance.
	if (VaultShotMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(VaultShotMapHandle);
		VaultShotMapHandle.Reset();
	}
#endif

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

EclipseBaseView::FEclipseBaseView UEclipseBaseSubsystem::ComposeBaseView() const
{
	const UEclipseCampaignSubsystem* Campaign = GetCampaign();
	if (Campaign == nullptr)
	{
		// Geen campagne = geen basis. De view komt terug op zijn eigen default
		// (`Absent`), en dat is de eerlijke lezing: er is geen leeg raster, er
		// is geen raster.
		return EclipseBaseView::FEclipseBaseView();
	}

	const UEclipseBaseLayoutAsset* Layout = ResolveLayout();
	const UDataTable* Table = ResolveFacilitiesTable();

	// Lokale lvalues: TFunctionRef bewaart alleen een verwijzing, en een
	// tijdelijke lambda zou al dood zijn tegen de tijd dat Compose hem aanroept.
	auto RowResolver = [Table](FName FacilityId) -> const FEclipseFacilityRow*
	{
		return Table != nullptr
			? Table->FindRow<FEclipseFacilityRow>(FacilityId, TEXT("EclipseBaseView"), /*bWarnIfMissing*/ false)
			: nullptr;
	};
	const FEclipseCampaignState& State = Campaign->GetState();
	auto SoldierResolver = [&State](const FGuid& SoldierId) -> const FEclipseSoldierRecord*
	{
		return State.FindSoldier(SoldierId);
	};

	// DE BEREIKTE UITBREIDINGSTRAP IS 1, EN DAT IS EEN GEMETEN FEIT GEEN AANNAME.
	// GDD 5.2 geeft de ladder 4 -> 8 -> 12 -> 16, maar nergens staat WAT hem
	// ophoogt; de verscheepte layout authort dan ook alleen trap-1-slots. Hier
	// een hoger getal invullen zou slots ontgrendelen die niemand heeft
	// ontworpen. Zodra de trigger beslist is, komt hij hier binnen en verandert
	// er niets aan de logica of het scherm.
	constexpr int32 AvailableSlotTier = 1;

	return EclipseBaseView::ComposeBaseView(
		State,
		Layout != nullptr ? TConstArrayView<FEclipseBaseSlotDef>(Layout->Slots) : TConstArrayView<FEclipseBaseSlotDef>(),
		ResolveTuningParams(),
		RowResolver,
		SoldierResolver,
		AvailableSlotTier);
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

#if !UE_BUILD_SHIPPING
void UEclipseBaseSubsystem::SetupVaultShotRig(UWorld* World)
{
	const UEclipseBaseLayoutAsset* Layout = ResolveLayout();
	if (World == nullptr || Layout == nullptr)
	{
		UE_LOG(LogEclipse, Error, TEXT("VaultShot: no world or no DA_BaseLayout — no frames (GDD 14.3.5)."));
		return;
	}

	// The review vault: all four Act 1 facilities operational, Workshop at L2.
	const FEclipseBaseState Review = EclipseVault::MakeReviewState(Layout->Slots);
	EclipseVault::RebuildVault(*World, Layout, Review);
	EclipseVault::SetSurfaceFogSuppressed(*World, /*bSuppressed*/ true, SurfaceFogDensity);

	// Eenmalige doorlichting van wat er ECHT hangt. "175 dozen geplaatst" en "175
	// dozen getekend" zijn twee verschillende beweringen, en de eerste ronde
	// leverde zes frames lucht op terwijl de eerste bewering klopte.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (!It->ActorHasTag(FName(TEXT("HP_VaultAnchor"))))
		{
			continue;
		}
		TArray<UInstancedStaticMeshComponent*> Isms;
		It->GetComponents<UInstancedStaticMeshComponent>(Isms);
		UE_LOG(LogEclipse, Display, TEXT("VaultShot: anker '%s' verborgen=%d · %d ISM-componenten"),
			*It->GetName(), It->IsHidden() ? 1 : 0, Isms.Num());
		for (const UInstancedStaticMeshComponent* Ism : Isms)
		{
			UE_LOG(LogEclipse, Display, TEXT("VaultShot:   %s instances=%d geregistreerd=%d zichtbaar=%d verborgenInGame=%d straal=%.0f mesh=%s materiaal=%s"),
				*Ism->GetName(), Ism->GetInstanceCount(), Ism->IsRegistered() ? 1 : 0,
				Ism->GetVisibleFlag() ? 1 : 0, Ism->bHiddenInGame ? 1 : 0,
				Ism->Bounds.SphereRadius, *GetNameSafe(Ism->GetStaticMesh()), *GetNameSafe(Ism->GetMaterial(0)));
		}
	}

	const TArray<EclipseVault::FEclipseVaultSlotPlan> Plans = EclipseVault::PlanSlots(Layout->Slots, Review);
	VaultShotEyes.Reset();
	VaultShotRotations.Reset();
	VaultShotLabels.Reset();

	// One frame per chamber from its doorway - the pose a player has walking
	// past - plus one down the Spine so the corridor that ties them together is
	// on record too.
	for (const EclipseVault::FEclipseVaultSlotPlan& Plan : Plans)
	{
		const float Side = Plan.bNorthSide ? 1.0f : -1.0f;
		const float MouthY = Plan.ChamberCenter.Y - Side * EclipseVault::ChamberDepthCm * 0.5f;
		const FVector Eye(Plan.ChamberCenter.X, MouthY - Side * 40.0f, Plan.ChamberCenter.Z + 170.0f);
		const FVector Look(Plan.ChamberCenter.X, MouthY + Side * 600.0f, Plan.ChamberCenter.Z + 140.0f);
		VaultShotEyes.Add(Eye);
		VaultShotRotations.Add((Look - Eye).Rotation());
		VaultShotLabels.Add(FString::Printf(TEXT("%s / %s"), *Plan.SlotId.ToString(), *Plan.FacilityId.ToString()));
	}
	if (Plans.Num() > 0)
	{
		const FVector SpineEye(Plans[0].ChamberCenter.X - 900.0f, 0.0f, Plans[0].ChamberCenter.Z + 175.0f);
		VaultShotEyes.Add(SpineEye);
		VaultShotRotations.Add(FRotator(-3.0f, 0.0f, 0.0f));
		VaultShotLabels.Add(TEXT("de Spine — de gang die de vier verbindt"));
	}

	// Index 0 repeats the first camera as a sacrificial warm-up: the first
	// capture of a session carries streaming/history artifacts (the district
	// rig's lesson, and it cost a shot round before it was written down).
	// Via een KOPIE: Insert met een element uit dezelfde array is in UE een
	// harde assert (de array kan herallokeren terwijl hij het argument nog
	// nodig heeft), en dat is precies wat de eerste rig-run deed.
	if (VaultShotEyes.Num() > 0)
	{
		const FVector FirstEye = VaultShotEyes[0];
		const FRotator FirstRotation = VaultShotRotations[0];
		VaultShotEyes.Insert(FirstEye, 0);
		VaultShotRotations.Insert(FirstRotation, 0);
		VaultShotLabels.Insert(TEXT("warm-up (weggooien)"), 0);
	}

	World->GetTimerManager().SetTimer(VaultShotTimer, this, &UEclipseBaseSubsystem::AdvanceVaultShotRig,
		2.0f, /*bLoop*/ true, /*FirstDelay*/ 6.0f);
	UE_LOG(LogEclipse, Display, TEXT("VaultShot: armed — %d frames of the Hollow Point review vault."), VaultShotEyes.Num());
}

void UEclipseBaseSubsystem::AdvanceVaultShotRig()
{
	UWorld* World = GetVaultWorld();
	APlayerController* Controller = World != nullptr ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = Controller != nullptr ? Controller->GetPawn() : nullptr;
	if (Pawn == nullptr)
	{
		return;
	}

	const int32 ShotIndex = VaultShotStep / 2;
	if (!VaultShotEyes.IsValidIndex(ShotIndex))
	{
		Controller->ConsoleCommand(TEXT("quit"));
		return;
	}

	if ((VaultShotStep % 2) == 0)
	{
		Pawn->SetActorLocation(VaultShotEyes[ShotIndex], /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
		Controller->SetControlRotation(VaultShotRotations[ShotIndex]);
		if (AEclipseCharacter* Character = Cast<AEclipseCharacter>(Pawn))
		{
			// Flying + zeroed velocity, or a walking character drifts during the
			// two-second settle and photographs a different room than the one
			// the pose named.
			Character->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			Character->GetCharacterMovement()->StopMovementImmediately();
			// EERSTE PERSOON, en dat is geen smaakkeuze. De derdepersoonsboom
			// hangt 300 cm ACHTER het personage: een camera die in de deuropening
			// hoort te staan, staat dan drie meter terug in de rots van de
			// overkant. De pose die dit script uitrekent is een OOGpositie, dus
			// het oog moet er ook echt staan - en de eerstepersoons-FOV (90) is
			// precies de FOV waarmee de headless meting rekent, zodat het beeld
			// en het getal naar hetzelfde kijken.
			Character->SetFirstPerson(true);
			if (USkeletalMeshComponent* Mesh = Character->GetMesh())
			{
				Mesh->SetVisibility(false);
			}
		}
		UE_LOG(LogEclipse, Display, TEXT("VAULTSHOT %d: %s"), ShotIndex, *VaultShotLabels[ShotIndex]);
	}
	else
	{
		// MEET DE CAMERA, niet de bedoeling. De eerste ronde leverde zes frames
		// met lucht en zwart op terwijl de pose-regel keurig "Slot_A" meldde: de
		// pose die dit script ZET en de plek waar de lens dan STAAT zijn twee
		// verschillende dingen, en alleen de tweede maakt een frame.
		FVector ViewLocation;
		FRotator ViewRotation;
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
		UE_LOG(LogEclipse, Display, TEXT("VAULTSHOT %d camera: bedoeld %s · pawn %s · lens %s · vault aanwezig=%d · geplaatste dozen=%d"),
			ShotIndex, *VaultShotEyes[ShotIndex].ToString(), *Pawn->GetActorLocation().ToString(), *ViewLocation.ToString(),
			EclipseVault::IsVaultPresent(*World) ? 1 : 0, EclipseVault::ReadPlacedBoxes(*World).Num());
		Controller->ConsoleCommand(TEXT("HighResShot 1920x1080"));
	}
	++VaultShotStep;
}
#endif

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
		TEXT("Eclipse.Base.Enter"),
		TEXT("Render the vault and put the player inside it (SPEC-P2-03 §2d: the base is a place you stand in, not a menu)."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
		{
			// The vault stood behind a console command that only BUILT it, and
			// nothing ever moved the player to it - so a walkable base existed
			// that nobody could reach on foot. That is the gap §2d names, and
			// this is the smallest honest closing of it until the P1-08 menu-hub
			// retirement lands a real level seam.
			EnsureVaultPresent();
			UWorld* World = GetVaultWorld();
			APlayerController* Controller = World != nullptr ? World->GetFirstPlayerController() : nullptr;
			APawn* Pawn = Controller != nullptr ? Controller->GetPawn() : nullptr;
			if (Pawn == nullptr)
			{
				UE_LOG(LogEclipse, Error, TEXT("Base.Enter: no player pawn to move into the vault."));
				return;
			}
			FVector Entry;
			if (!EclipseVault::FindVaultPoint(*World, TEXT("Entry_Vault"), Entry))
			{
				UE_LOG(LogEclipse, Error, TEXT("Base.Enter: the vault has no Entry_Vault marker — is a DA_BaseLayout linked? (GDD 14.3.5)"));
				return;
			}
			// Zonder dit sta je in bruine soep: zie SetSurfaceFogSuppressed.
			EclipseVault::SetSurfaceFogSuppressed(*World, /*bSuppressed*/ true, SurfaceFogDensity);
			Pawn->SetActorLocation(Entry + FVector(0.0f, 0.0f, 120.0f), /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				Character->GetCharacterMovement()->StopMovementImmediately();
			}
			// Facing the airlock, so the first thing on screen is the corridor in
			// and not the rock behind you.
			Controller->SetControlRotation(FRotator(0.0f, -90.0f, 0.0f));
			UE_LOG(LogEclipse, Display, TEXT("Base.Enter: standing at the Hollow Point airlock (%s). Walk south down the Spine; the four chambers open off it. Eclipse.Base.Leave puts the district back."), *Entry.ToString());
		}),
		ECVF_Default));

	ConsoleCommands.Add(Console.RegisterConsoleCommand(
		TEXT("Eclipse.Base.Leave"),
		TEXT("Back up to the district and restore its surface fog (the other half of Eclipse.Base.Enter)."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
		{
			// Enter without Leave would leave the district permanently fogless,
			// and a debug entry point that quietly wrecks the look of the place
			// you came from is worse than no entry point at all.
			UWorld* World = GetVaultWorld();
			if (World == nullptr)
			{
				return;
			}
			EclipseVault::SetSurfaceFogSuppressed(*World, /*bSuppressed*/ false, SurfaceFogDensity);
			APlayerController* Controller = World->GetFirstPlayerController();
			APawn* Pawn = Controller != nullptr ? Controller->GetPawn() : nullptr;
			if (Pawn == nullptr)
			{
				return;
			}
			FVector Surface(0.0f, 0.0f, 200.0f);
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				if (It->ActorHasTag(FName(TEXT("Entry_Main"))))
				{
					Surface = It->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f);
					break;
				}
			}
			Pawn->SetActorLocation(Surface, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
			if (ACharacter* Character = Cast<ACharacter>(Pawn))
			{
				Character->GetCharacterMovement()->StopMovementImmediately();
			}
			UE_LOG(LogEclipse, Display, TEXT("Base.Leave: back on the surface at %s, fog restored."), *Surface.ToString());
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
