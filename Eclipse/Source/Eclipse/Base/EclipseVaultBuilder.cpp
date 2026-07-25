#include "Base/EclipseVaultBuilder.h"

#include "Base/EclipseBaseLogic.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Eclipse.h"
#include "Engine/StaticMesh.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	/** Every vault actor carries this tag - teardown and presence checks key off it. */
	const FName VaultTag(TEXT("HP_Vault"));
	const FName VaultAnchorTag(TEXT("HP_VaultAnchor"));
	const FName VaultSlotMarkerTag(TEXT("HP_VaultSlot"));
	const FName VaultOrphanTag(TEXT("VaultOrphan"));

	/** Marker-tag prefixes encoding the parity fields (parsed by ReadSlotMarkers). */
	const TCHAR* SlotTagPrefix = TEXT("VaultSlot:");
	const TCHAR* FacilityTagPrefix = TEXT("VaultFacility:");
	const TCHAR* LevelTagPrefix = TEXT("VaultLevel:");
	const TCHAR* VisualTagPrefix = TEXT("VaultVisual:");
	const TCHAR* StaffTagPrefix = TEXT("VaultStaff:");

	/**
	 * The vault sits 150 m under the district origin: the geothermal vault of
	 * 5.2 is literally beneath Kessara, and no graybox/skyline actor reaches
	 * this depth, so district and base coexist in one world without overlap.
	 * Geometry coordinates are level content, not gameplay tunables (GDD 14.2).
	 */
	const FVector VaultOrigin(0.0f, 0.0f, -15000.0f);

	// Shell dimensions (cm). Chunky, confident proportions (15.5): a 7 m wide
	// Spine, 9 x 9 m chambers, 4.2 m ceiling - corridors read heavy, not cramped.
	constexpr float WallThk = 60.0f;
	constexpr float CeilH = 420.0f;
	constexpr float SpineHalfW = 350.0f;
	constexpr float SlotSpacing = 1400.0f;
	constexpr float ChamberW = 900.0f;
	constexpr float ChamberD = 900.0f;
	constexpr float DoorW = 320.0f;
	constexpr float EndPad = 700.0f;
	constexpr float AirlockDepth = 600.0f;
	constexpr float AirlockW = 360.0f;
	constexpr float AnnexW = 620.0f;
	constexpr float AnnexD = 520.0f;

	/**
	 * Interior cel key: a steep fixed work-light direction for the unlit toon
	 * master's banding (floors land in the lit band, walls split by orientation).
	 * There is no sun underground, so this is the vault's own light story -
	 * deterministic, SM5-safe, same regime as the district's SunRotation.
	 */
	const FRotator VaultKeyLight(-55.0f, 130.0f, 0.0f);

	/** Same unlit-emissive luminance calibration as the district (see EclipseGrayboxBuilder). */
	constexpr float ToonEmissiveScale = 10.0f;

	/**
	 * Vault palette - cel tones in the established 15.5 hue families (cold
	 * concrete, worker teal, hazard amber, oxide red, sodium glow); shades are
	 * hue-shifted cool, never just darker. bCollides: the shell is walkable,
	 * dressing never perturbs nav/cover/hitscan (the SPEC-P1-05 lesson).
	 */
	struct FVaultPaletteDef { const TCHAR* Label; FLinearColor Lit; FLinearColor Shade; bool bCollides; };
	const FVaultPaletteDef VaultPalette[] = {
		{ TEXT("VaultRock"),   FLinearColor(0.170f, 0.185f, 0.225f), FLinearColor(0.052f, 0.058f, 0.100f), true },  // excavated basalt shell, colder than district walls
		{ TEXT("VaultFloor"),  FLinearColor(0.150f, 0.140f, 0.150f), FLinearColor(0.050f, 0.047f, 0.074f), true },  // poured graphite deck
		{ TEXT("VaultBlast"),  FLinearColor(0.190f, 0.230f, 0.250f), FLinearColor(0.062f, 0.080f, 0.125f), true },  // blast doors + sealed excavation faces, gunmetal
		{ TEXT("VaultRubble"), FLinearColor(0.115f, 0.110f, 0.135f), FLinearColor(0.038f, 0.036f, 0.060f), true },  // spoil heaps at the sealed faces
		{ TEXT("FacCommand"),  FLinearColor(0.560f, 0.160f, 0.085f), FLinearColor(0.200f, 0.045f, 0.085f), true },  // the stolen Dominion map table keeps Dominion oxide (5.2)
		{ TEXT("FacBarracks"), FLinearColor(0.070f, 0.225f, 0.235f), FLinearColor(0.026f, 0.080f, 0.115f), true },  // worker teal - the bunk family
		{ TEXT("FacWorkshop"), FLinearColor(0.612f, 0.259f, 0.036f), FLinearColor(0.259f, 0.079f, 0.043f), true },  // hazard amber workbenches
		{ TEXT("FacIntel"),    FLinearColor(0.100f, 0.320f, 0.420f), FLinearColor(0.035f, 0.110f, 0.210f), true },  // signal cyan consoles (teal family, one step brighter)
		{ TEXT("Scaffold"),    FLinearColor(0.850f, 0.360f, 0.050f), FLinearColor(0.360f, 0.110f, 0.060f), false }, // construction read = the district's cover orange
		{ TEXT("SurveyPost"),  FLinearColor(0.420f, 0.345f, 0.095f), FLinearColor(0.180f, 0.147f, 0.058f), true },  // worn yellow survey pillars (DecoLine family)
		{ TEXT("Idler"),       FLinearColor(0.200f, 0.230f, 0.240f), FLinearColor(0.070f, 0.085f, 0.120f), false }, // stand-in staff figures, rebel gray-teal
		{ TEXT("Memorial"),    FLinearColor(0.300f, 0.255f, 0.165f), FLinearColor(0.120f, 0.100f, 0.070f), true },  // pale gold - the wall (5.3.2)
		{ TEXT("GlowStrip"),   FLinearColor(2.200f, 1.000f, 0.300f), FLinearColor(2.200f, 1.000f, 0.300f), false }, // sodium work-light emissive (global growth read)
	};
	const FVaultPaletteDef DefaultVaultPalette = { TEXT("VaultDefault"), FLinearColor(0.35f, 0.35f, 0.38f), FLinearColor(0.12f, 0.12f, 0.16f), true };

	const FVaultPaletteDef& VaultPaletteForLabel(const FString& Label)
	{
		for (const FVaultPaletteDef& Entry : VaultPalette)
		{
			if (Label.Equals(Entry.Label, ESearchCase::IgnoreCase))
			{
				return Entry;
			}
		}
		return DefaultVaultPalette;
	}

	/**
	 * Facility id -> presentation binding (graybox tier). Presentation glue like
	 * the district's site table, keyed on the shared data ids: the CC row is
	 * EclipseBaseDefaults, the other three are the SPEC-P2-03 slice facility
	 * rows. An unknown facility id (e.g. a Phase 3 Medbay in an old save)
	 * renders the neutral blockout with no site marker - graceful (GDD 14.3.5).
	 */
	struct FFacilityPresentationDef { FName FacilityId; const TCHAR* PaletteLabel; const TCHAR* SiteTag; };
	const FFacilityPresentationDef* FindFacilityPresentation(FName FacilityId)
	{
		static const FFacilityPresentationDef Defs[] = {
			{ EclipseBaseDefaults::CommandCenterFacilityId, TEXT("FacCommand"),  TEXT("Site_MapTable") },
			{ FName(TEXT("Barracks")),                      TEXT("FacBarracks"), TEXT("Site_MusterBoard") },
			{ FName(TEXT("Workshop")),                      TEXT("FacWorkshop"), TEXT("Site_Workbench") },
			{ FName(TEXT("IntelligenceCenter")),            TEXT("FacIntel"),    TEXT("Site_IntelConsole") },
		};
		for (const FFacilityPresentationDef& Def : Defs)
		{
			if (Def.FacilityId == FacilityId)
			{
				return &Def;
			}
		}
		return nullptr;
	}

	/** Fill the state-derived plan fields from one facility state row (null = empty slot). */
	void FillPlanFromState(EclipseVault::FEclipseVaultSlotPlan& Plan, const FEclipseFacilityState* Facility, const FEclipseBaseSlotDef* SlotDef)
	{
		using namespace EclipseVault;
		if (Facility != nullptr)
		{
			Plan.FacilityId = Facility->FacilityId;
			Plan.Level = Facility->Level;
			Plan.DaysRemaining = Facility->DaysRemaining;
			Plan.StaffCount = Facility->AssignedSoldierIds.Num();
		}
		Plan.Visual = Plan.DaysRemaining > 0 ? EEclipseVaultSlotVisual::Constructing
			: (Plan.Level >= 1 ? EEclipseVaultSlotVisual::Built : EEclipseVaultSlotVisual::Empty);

		// The GDD 12.3 state-swap contract: which streaming layer this read
		// would stream in. The graybox blockout stands in for it either way.
		if (SlotDef != nullptr)
		{
			switch (Plan.Visual)
			{
			case EEclipseVaultSlotVisual::Empty:
				Plan.StreamingId = SlotDef->EmptyStateStreamingId;
				break;
			case EEclipseVaultSlotVisual::Constructing:
				Plan.StreamingId = SlotDef->ConstructionStreamingId;
				break;
			case EEclipseVaultSlotVisual::Built:
				Plan.StreamingId = SlotDef->LevelStreamingIds.IsValidIndex(Plan.Level - 1)
					? SlotDef->LevelStreamingIds[Plan.Level - 1] : NAME_None;
				break;
			}
		}
	}

	/** Deterministic chamber placement: index-alternating south/north off the Spine, centered on the layout slot count. */
	void PlaceChamber(EclipseVault::FEclipseVaultSlotPlan& Plan, int32 Index, int32 NumLayoutSlots)
	{
		const float Centering = NumLayoutSlots > 0 ? (NumLayoutSlots - 1) * 0.5f : 0.0f;
		const float X = (Index - Centering) * SlotSpacing;
		const float SideSign = (Index % 2 == 0) ? -1.0f : 1.0f; // even = south, odd = north
		Plan.bNorthSide = SideSign > 0.0f;
		Plan.ChamberCenter = VaultOrigin + FVector(X, SideSign * (SpineHalfW + WallThk + ChamberD * 0.5f), 0.0f);
	}
}

namespace EclipseVault
{

const TCHAR* VisualToString(EEclipseVaultSlotVisual Visual)
{
	switch (Visual)
	{
	case EEclipseVaultSlotVisual::Constructing: return TEXT("Constructing");
	case EEclipseVaultSlotVisual::Built: return TEXT("Built");
	default: return TEXT("Empty");
	}
}

TArray<FEclipseVaultSlotPlan> PlanSlots(TConstArrayView<FEclipseBaseSlotDef> Slots, const FEclipseBaseState& BaseState)
{
	TArray<FEclipseVaultSlotPlan> Plans;
	Plans.Reserve(Slots.Num() + BaseState.Facilities.Num());

	// Layout slots in authored order - the authored order IS the spatial order.
	int32 Index = 0;
	for (const FEclipseBaseSlotDef& Slot : Slots)
	{
		FEclipseVaultSlotPlan& Plan = Plans.AddDefaulted_GetRef();
		Plan.SlotId = Slot.SlotId;
		Plan.bAuthoredEmptyState = !Slot.EmptyStateStreamingId.IsNone();
		Plan.bHasMemorialAnnex = Slot.AdjacentSlotIds.Contains(MemorialNodeId);
		Plan.bHasGeneratorAnnex = Slot.AdjacentSlotIds.Contains(GeneratorNodeId);
		FillPlanFromState(Plan, BaseState.FindBySlot(Slot.SlotId), &Slot);
		PlaceChamber(Plan, Index++, Slots.Num());
	}

	// Orphaned state rows (stale save / trimmed layout) are still shown, tagged
	// as orphans - the report never hides state, neither does the vault.
	for (const FEclipseFacilityState& Facility : BaseState.Facilities)
	{
		if (EclipseBaseLogic::FindSlotDef(Slots, Facility.SlotId) == nullptr)
		{
			FEclipseVaultSlotPlan& Plan = Plans.AddDefaulted_GetRef();
			Plan.SlotId = Facility.SlotId;
			Plan.bOrphan = true;
			FillPlanFromState(Plan, &Facility, nullptr);
			PlaceChamber(Plan, Index++, Slots.Num());
		}
	}
	return Plans;
}

uint32 ComputePlanHash(TConstArrayView<FEclipseVaultSlotPlan> Plans)
{
	// The count seeds the hash because it drives the centering (and therefore
	// every chamber position); the per-plan fields are exactly the parity
	// contract the markers carry, so two equal hashes render the same vault.
	uint32 Hash = GetTypeHash(Plans.Num());
	for (const FEclipseVaultSlotPlan& Plan : Plans)
	{
		Hash = HashCombine(Hash, GetTypeHash(Plan.SlotId));
		Hash = HashCombine(Hash, GetTypeHash(Plan.FacilityId));
		Hash = HashCombine(Hash, GetTypeHash(Plan.Level));
		Hash = HashCombine(Hash, GetTypeHash(Plan.DaysRemaining));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Plan.Visual)));
		Hash = HashCombine(Hash, GetTypeHash(Plan.StaffCount));
		Hash = HashCombine(Hash, GetTypeHash(Plan.StreamingId));
		Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(
			(Plan.bAuthoredEmptyState ? 1 : 0) | (Plan.bHasMemorialAnnex ? 2 : 0) |
			(Plan.bHasGeneratorAnnex ? 4 : 0) | (Plan.bOrphan ? 8 : 0))));
	}
	return Hash;
}

bool IsVaultPresent(UWorld& World)
{
	for (TActorIterator<ATargetPoint> It(&World); It; ++It)
	{
		if (It->ActorHasTag(VaultAnchorTag))
		{
			return true;
		}
	}
	return false;
}

void BuildVault(UWorld& World, const UEclipseBaseLayoutAsset* Layout, const FEclipseBaseState& BaseState)
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh == nullptr)
	{
		UE_LOG(LogEclipse, Error, TEXT("Vault: engine cube mesh missing — vault not built."));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// The anchor owns every instanced mesh component; markers are separate
	// tagged points (the FindSiteLocation pattern). All carry VaultTag so the
	// re-render teardown is one sweep.
	ATargetPoint* Anchor = World.SpawnActor<ATargetPoint>(VaultOrigin, FRotator::ZeroRotator, Params);
	if (Anchor == nullptr)
	{
		UE_LOG(LogEclipse, Error, TEXT("Vault: anchor spawn failed — vault not built."));
		return;
	}
	Anchor->Tags.Add(VaultTag);
	Anchor->Tags.Add(VaultAnchorTag);

	if (Layout == nullptr)
	{
		// GDD 14.3.5 (and the SPEC-P2-03 integration rule): loud log, empty
		// vault, never a crash. Build orders already reject without a layout,
		// so the state cannot drift while the vault stands empty.
		UE_LOG(LogEclipse, Warning, TEXT("Vault: no DA_BaseLayout linked — the vault presents empty (anchor only) until the layout asset lands (GDD 14.3.5)."));
		return;
	}

	const TArray<FEclipseVaultSlotPlan> Plans = PlanSlots(Layout->Slots, BaseState);

	// Toon master first, engine shape material as the flat fallback (14.3.5) —
	// the same regime as the district builder. The vault stays on the UNLIT
	// master under every mode: underground there is no sun for the lit variant,
	// and the emissive calibration keeps the palette readable deterministically.
	UMaterialInterface* ToonMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToon.M_EclipseToon"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ToonMaterial == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Vault: M_EclipseToon missing — flat engine-material fallback (run Tools/author_toon_material.py; GDD 14.3.5)."));
	}

	// One ISM component per palette entry (GDD 12.4: instanced meshes, one draw
	// call per material family, zero per-tick work). Explicit component names
	// keep rebuilds deterministic; no shadows underground (unlit emissive).
	TMap<FString, UInstancedStaticMeshComponent*> IsmByLabel;
	auto IsmForLabel = [&](const TCHAR* Label) -> UInstancedStaticMeshComponent*
	{
		if (UInstancedStaticMeshComponent** Found = IsmByLabel.Find(Label))
		{
			return *Found;
		}
		const FVaultPaletteDef& Entry = VaultPaletteForLabel(Label);
		UInstancedStaticMeshComponent* Ism = NewObject<UInstancedStaticMeshComponent>(Anchor, FName(*FString::Printf(TEXT("VaultIsm_%s"), Label)));
		Ism->SetupAttachment(Anchor->GetRootComponent());
		Ism->SetMobility(EComponentMobility::Movable);
		Ism->SetStaticMesh(CubeMesh);
		Ism->SetCastShadow(false);
		Ism->SetAffectDistanceFieldLighting(false); // the district's non-uniform-scale DF lesson applies unchanged
		if (Entry.bCollides)
		{
			Ism->SetCollisionProfileName(TEXT("BlockAll"));
		}
		else
		{
			Ism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		UMaterialInterface* Master = ToonMaterial != nullptr ? ToonMaterial : BaseMaterial;
		if (Master != nullptr)
		{
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Master, &World);
			if (Master != BaseMaterial)
			{
				Mid->SetVectorParameterValue(TEXT("LitColor"), Entry.Lit);
				Mid->SetVectorParameterValue(TEXT("ShadeColor"), Entry.Shade);
				Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(VaultKeyLight.Vector(), 0.0f)));
				Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
			}
			else
			{
				Mid->SetVectorParameterValue(TEXT("Color"), Entry.Lit);
			}
			Ism->SetMaterial(0, Mid);
		}
		Ism->RegisterComponent();
		IsmByLabel.Add(Label, Ism);
		return Ism;
	};

	// Axis-aligned box instance: local center (vault space) + full size in cm.
	auto AddBox = [&](const TCHAR* Label, const FVector& LocalCenter, const FVector& Size)
	{
		if (UInstancedStaticMeshComponent* Ism = IsmForLabel(Label))
		{
			Ism->AddInstance(FTransform(FQuat::Identity, VaultOrigin + LocalCenter, Size / 100.0f), /*bWorldSpace*/ true);
		}
	};

	auto AddMarker = [&](const FVector& LocalCenter, std::initializer_list<FName> InTags) -> ATargetPoint*
	{
		ATargetPoint* Marker = World.SpawnActor<ATargetPoint>(VaultOrigin + LocalCenter, FRotator::ZeroRotator, Params);
		if (Marker != nullptr)
		{
			Marker->Tags.Add(VaultTag);
			for (const FName& Tag : InTags)
			{
				Marker->Tags.Add(Tag);
			}
		}
		return Marker;
	};

	// ---- Shell ------------------------------------------------------------
	// Spine half length covers every chamber plus the sealed end faces.
	float MaxAbsX = 0.0f;
	for (const FEclipseVaultSlotPlan& Plan : Plans)
	{
		MaxAbsX = FMath::Max(MaxAbsX, FMath::Abs(Plan.ChamberCenter.X - VaultOrigin.X));
	}
	const float HalfLen = (Plans.Num() > 0 ? MaxAbsX + SlotSpacing * 0.5f : SlotSpacing) + EndPad;
	const float DepthY = SpineHalfW + WallThk + ChamberD + WallThk + AnnexD + WallThk + 100.0f;

	// One floor and one ceiling slab across the whole footprint (2 instances).
	AddBox(TEXT("VaultFloor"), FVector(0, 0, -30.0f), FVector(2.0f * (HalfLen + WallThk), 2.0f * DepthY, 60.0f));
	AddBox(TEXT("VaultRock"), FVector(0, 0, CeilH + 30.0f), FVector(2.0f * (HalfLen + WallThk), 2.0f * DepthY, 60.0f));

	// Sealed excavation faces at both Spine ends (the 5.2 slots-5-8 tease):
	// blast-door slab + deterministic rubble spill in front of each.
	const FVector RubbleOffsets[] = {
		FVector(-90, -120, 40), FVector(-20, 60, 50), FVector(-140, 140, 35),
		FVector(-60, -40, 110), FVector(-170, -10, 45), FVector(-30, 180, 38),
	};
	const float RubbleSizes[] = { 180.0f, 140.0f, 120.0f, 100.0f, 160.0f, 90.0f };
	for (const float EndSign : { -1.0f, 1.0f })
	{
		AddBox(TEXT("VaultBlast"), FVector(EndSign * (HalfLen + WallThk * 0.5f), 0, CeilH * 0.5f),
			FVector(WallThk, 2.0f * (SpineHalfW + WallThk), CeilH));
		for (int32 RubbleIndex = 0; RubbleIndex < static_cast<int32>(UE_ARRAY_COUNT(RubbleOffsets)); ++RubbleIndex)
		{
			const FVector& Offset = RubbleOffsets[RubbleIndex];
			AddBox(TEXT("VaultRubble"),
				FVector(EndSign * (HalfLen - 140.0f + Offset.X * -EndSign), Offset.Y, Offset.Z * 0.5f),
				FVector(RubbleSizes[RubbleIndex], RubbleSizes[RubbleIndex] * 0.8f, Offset.Z));
		}
	}

	// Entry airlock (north side): pick an X clear of every north chamber mouth.
	float AirlockX = 0.0f;
	{
		bool bBlocked = false;
		for (const FEclipseVaultSlotPlan& Plan : Plans)
		{
			if (Plan.bNorthSide && FMath::Abs(Plan.ChamberCenter.X - VaultOrigin.X) < (ChamberW + DoorW) * 0.5f + 50.0f)
			{
				bBlocked = true;
				break;
			}
		}
		if (bBlocked)
		{
			AirlockX = SlotSpacing * 0.5f;
		}
	}

	// Spine walls with door gaps: chambers on that side, plus the airlock north.
	auto BuildWallWithGaps = [&](float SideSign, TArray<float> GapXs)
	{
		GapXs.Sort();
		const float WallY = SideSign * (SpineHalfW + WallThk * 0.5f);
		float Cursor = -HalfLen;
		auto EmitSegment = [&](float FromX, float ToX)
		{
			if (ToX - FromX > 10.0f)
			{
				AddBox(TEXT("VaultRock"), FVector((FromX + ToX) * 0.5f, WallY, CeilH * 0.5f), FVector(ToX - FromX, WallThk, CeilH));
			}
		};
		for (const float GapX : GapXs)
		{
			EmitSegment(Cursor, GapX - DoorW * 0.5f);
			Cursor = GapX + DoorW * 0.5f;
		}
		EmitSegment(Cursor, HalfLen);
	};
	TArray<float> NorthGaps, SouthGaps;
	NorthGaps.Add(AirlockX);
	for (const FEclipseVaultSlotPlan& Plan : Plans)
	{
		(Plan.bNorthSide ? NorthGaps : SouthGaps).Add(Plan.ChamberCenter.X - VaultOrigin.X);
	}
	BuildWallWithGaps(1.0f, NorthGaps);
	BuildWallWithGaps(-1.0f, SouthGaps);

	// Airlock corridor + the slid-open vault door (SPEC-P2-03 slot-graph entry).
	{
		const float CorridorY0 = SpineHalfW + WallThk;
		for (const float WallSign : { -1.0f, 1.0f })
		{
			AddBox(TEXT("VaultRock"),
				FVector(AirlockX + WallSign * (AirlockW * 0.5f + WallThk * 0.5f), CorridorY0 + AirlockDepth * 0.5f, CeilH * 0.5f),
				FVector(WallThk, AirlockDepth, CeilH));
		}
		// The massive door slab sits beside its opening - the vault reads open.
		AddBox(TEXT("VaultBlast"),
			FVector(AirlockX - AirlockW * 0.9f, CorridorY0 + AirlockDepth + WallThk * 0.5f, CeilH * 0.45f),
			FVector(AirlockW * 1.2f, WallThk, CeilH * 0.9f));
		AddMarker(FVector(AirlockX, CorridorY0 + AirlockDepth - 100.0f, 0), { FName(TEXT("Site_VaultDoor")) });
		AddMarker(FVector(AirlockX, CorridorY0 + AirlockDepth + 200.0f, 0), { FName(TEXT("Entry_Vault")) });
	}

	// ---- Chambers (one per plan entry) ------------------------------------
	int32 OrphanCount = 0;
	int32 DeclaredStreamingIds = 0;
	for (const FEclipseVaultSlotPlan& Plan : Plans)
	{
		const FVector Local = Plan.ChamberCenter - VaultOrigin;
		const float S = Plan.bNorthSide ? 1.0f : -1.0f;
		const float BackY = S * (SpineHalfW + WallThk + ChamberD + WallThk * 0.5f);
		const float MidY = S * (SpineHalfW + WallThk + ChamberD * 0.5f);
		const bool bAnnex = Plan.bHasMemorialAnnex || Plan.bHasGeneratorAnnex;

		// Back wall (split around a door when an annex hangs behind), side walls.
		if (bAnnex)
		{
			const float SegW = (ChamberW + 2.0f * WallThk - DoorW) * 0.5f;
			for (const float SegSign : { -1.0f, 1.0f })
			{
				AddBox(TEXT("VaultRock"), FVector(Local.X + SegSign * (DoorW + SegW) * 0.5f, BackY, CeilH * 0.5f), FVector(SegW, WallThk, CeilH));
			}
		}
		else
		{
			AddBox(TEXT("VaultRock"), FVector(Local.X, BackY, CeilH * 0.5f), FVector(ChamberW + 2.0f * WallThk, WallThk, CeilH));
		}
		for (const float SideSign : { -1.0f, 1.0f })
		{
			AddBox(TEXT("VaultRock"), FVector(Local.X + SideSign * (ChamberW * 0.5f + WallThk * 0.5f), MidY, CeilH * 0.5f), FVector(WallThk, ChamberD, CeilH));
		}

		// Annex room behind the chamber (memorial alcove / generator room).
		if (bAnnex)
		{
			const float AnnexMidY = S * (SpineHalfW + WallThk + ChamberD + WallThk + AnnexD * 0.5f);
			const float AnnexBackY = S * (SpineHalfW + WallThk + ChamberD + WallThk + AnnexD + WallThk * 0.5f);
			AddBox(TEXT("VaultRock"), FVector(Local.X, AnnexBackY, CeilH * 0.5f), FVector(AnnexW + 2.0f * WallThk, WallThk, CeilH));
			for (const float SideSign : { -1.0f, 1.0f })
			{
				AddBox(TEXT("VaultRock"), FVector(Local.X + SideSign * (AnnexW * 0.5f + WallThk * 0.5f), AnnexMidY, CeilH * 0.5f), FVector(WallThk, AnnexD, CeilH));
			}
			if (Plan.bHasMemorialAnnex)
			{
				// The memorial wall: auto-grown, never a purchase (5.3.2). Name
				// plaques are the dressing pass; the alcove itself stands now.
				AddBox(TEXT("Memorial"), FVector(Local.X, AnnexBackY - S * (WallThk * 0.5f + 12.0f), 130.0f), FVector(AnnexW * 0.7f, 20.0f, 240.0f));
				AddMarker(FVector(Local.X, AnnexMidY, 0), { FName(TEXT("Site_Memorial")), Plan.SlotId });
			}
			if (Plan.bHasGeneratorAnnex)
			{
				// The coughing geothermal generator - ambient story, no Energy
				// meter in Phase 2 (locked decision 5). PLACEHOLDER(GDD 6.2 Energy).
				AddBox(TEXT("VaultBlast"), FVector(Local.X, AnnexMidY, 110.0f), FVector(320.0f, 260.0f, 220.0f));
				AddBox(TEXT("GlowStrip"), FVector(Local.X, AnnexMidY, 240.0f), FVector(120.0f, 60.0f, 18.0f));
				AddMarker(FVector(Local.X - 260.0f, AnnexMidY, 0), { FName(TEXT("Site_Generator")), Plan.SlotId });
			}
		}

		// --- Per-slot state read (the 5.4 visible-growth table) ---
		if (!Plan.StreamingId.IsNone())
		{
			++DeclaredStreamingIds; // counted for the summary line below
		}
		if (Plan.bOrphan)
		{
			++OrphanCount;
		}

		const FFacilityPresentationDef* Presentation = Plan.FacilityId.IsNone() ? nullptr : FindFacilityPresentation(Plan.FacilityId);
		const TCHAR* FacilityPalette = Presentation != nullptr ? Presentation->PaletteLabel : DefaultVaultPalette.Label;

		if (Plan.Visual == EEclipseVaultSlotVisual::Empty)
		{
			// Raw rock + survey post -> build menu (SPEC-P2-03 interaction points);
			// a dead cable run along the floor sells "waiting for power".
			AddBox(TEXT("SurveyPost"), FVector(Local.X - ChamberW * 0.25f, S * (SpineHalfW + WallThk + ChamberD * 0.3f), 120.0f), FVector(60.0f, 60.0f, 240.0f));
			AddBox(TEXT("VaultRubble"), FVector(Local.X + ChamberW * 0.15f, MidY, 8.0f), FVector(ChamberW * 0.7f, 40.0f, 12.0f));
			AddMarker(FVector(Local.X - ChamberW * 0.25f, S * (SpineHalfW + WallThk + ChamberD * 0.3f), 0), { FName(TEXT("Site_Survey")), Plan.SlotId });
		}

		if (Plan.bAuthoredEmptyState && Plan.Level == 0)
		{
			// The 5.2 bunk camp (Slot B's authored empty state): 11 bunks, the
			// crate table, and the muster board - working from day 1, before and
			// during Barracks construction (facilities never gate the loop).
			for (int32 BunkIndex = 0; BunkIndex < 11; ++BunkIndex)
			{
				const int32 Col = BunkIndex % 4;
				const int32 Row = BunkIndex / 4;
				AddBox(TEXT("FacBarracks"),
					FVector(Local.X + (Col - 1.5f) * 190.0f, MidY + S * (Row - 1) * 230.0f, 30.0f),
					FVector(170.0f, 80.0f, 60.0f));
			}
			AddBox(TEXT("VaultRubble"), FVector(Local.X, MidY, 45.0f), FVector(90.0f, 90.0f, 90.0f)); // crate table
			AddBox(TEXT("SurveyPost"), FVector(Local.X + ChamberW * 0.35f, S * (SpineHalfW + WallThk + 60.0f), 110.0f), FVector(20.0f, 160.0f, 200.0f)); // muster board
			AddMarker(FVector(Local.X + ChamberW * 0.35f, S * (SpineHalfW + WallThk + 140.0f), 0), { FName(TEXT("Site_MusterBoard")), Plan.SlotId });
		}

		if (Plan.Visual == EEclipseVaultSlotVisual::Constructing)
		{
			// Scaffold blockout: four posts, two beams, a material pile. The
			// spark/drill loop is the audio/FX dressing pass.
			for (const float PX : { -1.0f, 1.0f })
			{
				for (const float PY : { -1.0f, 1.0f })
				{
					AddBox(TEXT("Scaffold"), FVector(Local.X + PX * ChamberW * 0.3f, MidY + PY * ChamberD * 0.3f, CeilH * 0.35f), FVector(40.0f, 40.0f, CeilH * 0.7f));
				}
				AddBox(TEXT("Scaffold"), FVector(Local.X, MidY + PX * ChamberD * 0.3f, CeilH * 0.7f), FVector(ChamberW * 0.6f, 40.0f, 40.0f));
			}
			AddBox(TEXT("VaultRubble"), FVector(Local.X + ChamberW * 0.28f, MidY - S * ChamberD * 0.22f, 40.0f), FVector(220.0f, 180.0f, 80.0f));
		}

		if (Plan.Level >= 1)
		{
			// Operational facility blockout + its own light temperature (the
			// light bar reuses the facility palette - unlit emissive makes it
			// read as the room's key) + the interaction point marker.
			AddBox(FacilityPalette, FVector(Local.X, MidY, 55.0f), FVector(300.0f, 200.0f, 110.0f));
			AddBox(FacilityPalette, FVector(Local.X, MidY, 130.0f), FVector(260.0f, 160.0f, 20.0f));
			AddBox(FacilityPalette, FVector(Local.X, MidY, CeilH - 60.0f), FVector(200.0f, 30.0f, 20.0f));
			if (Plan.Level >= 2)
			{
				// The Workshop L2 read: second workbench + powered rack (5.4).
				AddBox(FacilityPalette, FVector(Local.X + 220.0f, MidY + S * 160.0f, 50.0f), FVector(200.0f, 140.0f, 100.0f));
				AddBox(FacilityPalette, FVector(Local.X - 240.0f, MidY + S * 120.0f, 90.0f), FVector(80.0f, 160.0f, 180.0f));
			}
			if (Presentation != nullptr)
			{
				AddMarker(FVector(Local.X, S * (SpineHalfW + WallThk + 120.0f), 0), { FName(Presentation->SiteTag), Plan.SlotId });
			}
		}

		// Staff idlers (crew on a building site, analyst on an operational one -
		// positional, like the state): chunky two-block stand-ins, capped at 4.
		for (int32 IdlerIndex = 0; IdlerIndex < FMath::Min(Plan.StaffCount, 4); ++IdlerIndex)
		{
			const float IdlerX = Local.X + IdlerIndex * 140.0f - 70.0f;
			const float IdlerY = MidY - S * ChamberD * 0.25f;
			AddBox(TEXT("Idler"), FVector(IdlerX, IdlerY, 70.0f), FVector(55.0f, 40.0f, 95.0f));
			AddBox(TEXT("Idler"), FVector(IdlerX, IdlerY, 135.0f), FVector(32.0f, 32.0f, 32.0f));
		}

		// The parity marker: everything the state says about this slot, encoded
		// as tags the Gauntlet (and future debug tools) can read back.
		ATargetPoint* SlotMarker = AddMarker(FVector(Local.X, MidY, 140.0f), { VaultSlotMarkerTag });
		if (SlotMarker != nullptr)
		{
			SlotMarker->Tags.Add(FName(*FString::Printf(TEXT("%s%s"), SlotTagPrefix, *Plan.SlotId.ToString())));
			SlotMarker->Tags.Add(FName(*FString::Printf(TEXT("%s%s"), FacilityTagPrefix, *Plan.FacilityId.ToString())));
			SlotMarker->Tags.Add(FName(*FString::Printf(TEXT("%s%d"), LevelTagPrefix, Plan.Level)));
			SlotMarker->Tags.Add(FName(*FString::Printf(TEXT("%s%s"), VisualTagPrefix, VisualToString(Plan.Visual))));
			SlotMarker->Tags.Add(FName(*FString::Printf(TEXT("%s%d"), StaffTagPrefix, Plan.StaffCount)));
			if (Plan.bOrphan)
			{
				SlotMarker->Tags.Add(VaultOrphanTag);
			}
		}
	}

	// ---- Global growth (per completed facility count, 5.4) ----------------
	// Counts what the player added on top of the pre-built Command Center.
	int32 BuiltCount = 0;
	for (const FEclipseVaultSlotPlan& Plan : Plans)
	{
		if (Plan.Level >= 1 && Plan.FacilityId != EclipseBaseDefaults::CommandCenterFacilityId)
		{
			++BuiltCount;
		}
	}
	if (BuiltCount >= 1)
	{
		// Hanging work-lights in the Spine.
		AddBox(TEXT("GlowStrip"), FVector(-HalfLen * 0.4f, 0, CeilH - 80.0f), FVector(120.0f, 120.0f, 40.0f));
		AddBox(TEXT("GlowStrip"), FVector(HalfLen * 0.4f, 0, CeilH - 80.0f), FVector(120.0f, 120.0f, 40.0f));
	}
	if (BuiltCount >= 2)
	{
		// Wall sconces on the Spine's inner faces.
		for (const float SconceX : { -0.65f, -0.15f, 0.15f, 0.65f })
		{
			AddBox(TEXT("GlowStrip"), FVector(HalfLen * SconceX, (SconceX < 0.0f ? 1.0f : -1.0f) * (SpineHalfW - 14.0f), CeilH * 0.6f), FVector(60.0f, 20.0f, 90.0f));
		}
	}
	if (BuiltCount >= 3)
	{
		// Full strip lighting down the Spine.
		AddBox(TEXT("GlowStrip"), FVector(0, 0, CeilH - 30.0f), FVector(HalfLen * 1.8f, 40.0f, 16.0f));
	}

	UE_LOG(LogEclipse, Display, TEXT("Vault: built %d slot chamber(s) (%d orphan), %d player-built facilities, %d declared streaming layer(s) presented as graybox blockouts (state swap lands with the art pass — GDD 12.3)."),
		Plans.Num(), OrphanCount, BuiltCount, DeclaredStreamingIds);
}

void RebuildVault(UWorld& World, const UEclipseBaseLayoutAsset* Layout, const FEclipseBaseState& BaseState)
{
	// Collect first, then destroy: never mutate the actor list mid-iteration.
	TArray<AActor*> VaultActors;
	for (TActorIterator<AActor> It(&World); It; ++It)
	{
		if (It->ActorHasTag(VaultTag))
		{
			VaultActors.Add(*It);
		}
	}
	for (AActor* Actor : VaultActors)
	{
		Actor->Destroy();
	}
	BuildVault(World, Layout, BaseState);
}

TArray<FEclipseVaultSlotView> ReadSlotMarkers(UWorld& World)
{
	TArray<FEclipseVaultSlotView> Views;
	for (TActorIterator<ATargetPoint> It(&World); It; ++It)
	{
		if (!It->ActorHasTag(VaultSlotMarkerTag))
		{
			continue;
		}
		FEclipseVaultSlotView& View = Views.AddDefaulted_GetRef();
		View.bOrphan = It->ActorHasTag(VaultOrphanTag);
		for (const FName& Tag : It->Tags)
		{
			const FString TagString = Tag.ToString();
			if (TagString.StartsWith(SlotTagPrefix))
			{
				View.SlotId = FName(*TagString.RightChop(FCString::Strlen(SlotTagPrefix)));
			}
			else if (TagString.StartsWith(FacilityTagPrefix))
			{
				View.FacilityId = FName(*TagString.RightChop(FCString::Strlen(FacilityTagPrefix)));
			}
			else if (TagString.StartsWith(LevelTagPrefix))
			{
				View.Level = FCString::Atoi(*TagString.RightChop(FCString::Strlen(LevelTagPrefix)));
			}
			else if (TagString.StartsWith(StaffTagPrefix))
			{
				View.StaffCount = FCString::Atoi(*TagString.RightChop(FCString::Strlen(StaffTagPrefix)));
			}
			else if (TagString.StartsWith(VisualTagPrefix))
			{
				const FString Value = TagString.RightChop(FCString::Strlen(VisualTagPrefix));
				View.Visual = Value == TEXT("Built") ? EEclipseVaultSlotVisual::Built
					: (Value == TEXT("Constructing") ? EEclipseVaultSlotVisual::Constructing : EEclipseVaultSlotVisual::Empty);
			}
		}
	}
	Views.Sort([](const FEclipseVaultSlotView& A, const FEclipseVaultSlotView& B)
	{
		return A.SlotId.LexicalLess(B.SlotId);
	});
	return Views;
}

TArray<FString> ComputeVaultSignature(UWorld& World)
{
	TArray<FString> Lines;
	for (TActorIterator<ATargetPoint> It(&World); It; ++It)
	{
		if (!It->ActorHasTag(VaultTag))
		{
			continue;
		}
		TArray<FString> TagStrings;
		for (const FName& Tag : It->Tags)
		{
			TagStrings.Add(Tag.ToString());
		}
		TagStrings.Sort();
		Lines.Add(FString::Printf(TEXT("MARKER %s @ %s"), *FString::Join(TagStrings, TEXT("+")), *It->GetActorLocation().ToString()));

		if (It->ActorHasTag(VaultAnchorTag))
		{
			TArray<UInstancedStaticMeshComponent*> Isms;
			It->GetComponents<UInstancedStaticMeshComponent>(Isms);
			for (const UInstancedStaticMeshComponent* Ism : Isms)
			{
				for (int32 InstanceIndex = 0; InstanceIndex < Ism->GetInstanceCount(); ++InstanceIndex)
				{
					FTransform InstanceTransform;
					Ism->GetInstanceTransform(InstanceIndex, InstanceTransform, /*bWorldSpace*/ true);
					Lines.Add(FString::Printf(TEXT("ISM %s #%d %s"), *Ism->GetName(), InstanceIndex, *InstanceTransform.ToString()));
				}
			}
		}
	}
	Lines.Sort();
	return Lines;
}

} // namespace EclipseVault
