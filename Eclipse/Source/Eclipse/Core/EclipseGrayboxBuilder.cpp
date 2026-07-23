#include "Core/EclipseGrayboxBuilder.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Eclipse.h"
#include "Animation/AnimSequence.h"
#include "Animation/SkeletalMeshActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Materials/MaterialInterface.h"
#include "Quests/EclipseObjectiveTrigger.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	struct FBlockDef { const TCHAR* Label; float X; float Y; float Z; float SX; float SY; float SZ; };
	struct FPointDef { const TCHAR* Id; float X; float Y; };

	// ~200x200 m district: perimeter, two buildings (control-post compound with a
	// west entrance gap; warehouse with an east gap), scattered cover field.
	constexpr FBlockDef Blocks[] = {
		{ TEXT("Floor"), 0, 0, -50, 200, 200, 1 },
		{ TEXT("Wall_N"), 0, 10000, 200, 200, 1, 5 },
		{ TEXT("Wall_S"), 0, -10000, 200, 200, 1, 5 },
		{ TEXT("Wall_E"), 10000, 0, 200, 1, 200, 5 },
		{ TEXT("Wall_W"), -10000, 0, 200, 1, 200, 5 },
		{ TEXT("BldgA_N"), 5000, -1200, 200, 16, 1, 4 },
		{ TEXT("BldgA_S"), 5000, -2800, 200, 16, 1, 4 },
		{ TEXT("BldgA_E"), 5800, -2000, 200, 1, 16, 4 },
		{ TEXT("BldgA_W"), 4200, -2400, 200, 1, 8, 4 },
		{ TEXT("BldgB_N"), -4000, 3800, 200, 12, 1, 4 },
		{ TEXT("BldgB_S"), -4000, 2200, 200, 12, 1, 4 },
		{ TEXT("BldgB_E"), -3200, 3400, 200, 1, 6, 4 },
		{ TEXT("BldgB_W"), -4800, 3000, 200, 1, 12, 4 },
	};

	constexpr FPointDef CoverPoints[] = {
		{ TEXT("Cover"), -6000, -4000 }, { TEXT("Cover"), -4500, -5500 }, { TEXT("Cover"), -2500, -3000 },
		{ TEXT("Cover"), -1000, -5000 }, { TEXT("Cover"), 500, -2500 },   { TEXT("Cover"), 2000, -4000 },
		{ TEXT("Cover"), 3500, -1500 },  { TEXT("Cover"), 1500, 500 },    { TEXT("Cover"), -500, 2000 },
		{ TEXT("Cover"), -2000, 500 },   { TEXT("Cover"), -3500, -500 },  { TEXT("Cover"), 2500, 2500 },
		{ TEXT("Cover"), 4000, 1000 },   { TEXT("Cover"), 5500, 3000 },   { TEXT("Cover"), -6500, 1500 },
		{ TEXT("Cover"), -5000, 4500 },  { TEXT("Cover"), 6500, -4500 },  { TEXT("Cover"), 7000, 500 },
		{ TEXT("Cover"), -7500, -2000 }, { TEXT("Cover"), 0, 6500 },
	};

	constexpr FPointDef Sites[] = {
		{ TEXT("Site_ControlPost"), 5000, -2000 },
		{ TEXT("Site_AlarmRelay"), 5000, 1500 },
		{ TEXT("Site_Crane"), -4000, 3000 },
		{ TEXT("Site_Pens"), -4000, 2600 },
		{ TEXT("Site_Extraction"), -8500, -8500 },
		{ TEXT("Spawn_Checkpoint"), 4600, -2000 },
		{ TEXT("Spawn_Reserve"), 6500, -3500 },
		{ TEXT("Spawn_Yard"), -3600, 3000 },
		{ TEXT("Spawn_Pens"), -4400, 2600 },
		{ TEXT("Spawn_Patrol"), 0, 0 },
	};

	/** Sites that double as objective triggers (SPEC-P1-05 objective primitives). */
	constexpr FPointDef TriggerSites[] = {
		{ TEXT("Site_ControlPost"), 5000, -2000 },
		{ TEXT("Site_AlarmRelay"), 5000, 1500 },
		{ TEXT("Site_Crane"), -4000, 3000 },
		{ TEXT("Site_Pens"), -4000, 2600 },
		{ TEXT("Site_Extraction"), -8500, -8500 },
	};

	constexpr FPointDef EntryPoints[] = {
		{ TEXT("Entry_Main"), -9000, 0 },
		{ TEXT("Entry_Sewer"), 0, -9000 },
		{ TEXT("Entry_Roof"), 8500, 8500 },
	};

	/**
	 * PLACEHOLDER(Part 15.5): early stylized dressing, owner-authorized ahead of
	 * the Phase 2 art pass. Bold cel-banded block colors (Borderlands-leaning:
	 * punchy, readable at command distance) as dynamic instances of the authored
	 * toon master (/Game/Art/M_EclipseToon); the real district replaces all of it
	 * with authored kits. Shade tones are hue-shifted cool, never just darker —
	 * that hue shift is what makes flat cel shading read as painted, not dimmed.
	 */
	// TexPath (optional): CC0 world-aligned albedo multiplied into the cel bands
	// (Content/Art/Textures/SOURCES.md — owner-authorized asset pass). Null =
	// flat cel, bit-identical to the pre-texture look.
	// TexGain normalizes each texture's average luminance back to ~1.0 so the
	// district's histogram — and with it the banked dusk auto-exposure — is
	// unchanged by texturing (first textured round: the darker floor re-metered
	// the whole scene into daylight). TexMix < 1 blends toward the flat cel
	// color where a texture's value range fights the palette.
	// FallbackTexPath (optional): machine-local Fab/Megascans primaries degrade
	// to a repo-tracked CC0 texture instead of flat cel on machines without the
	// pack (curation contract, phase0/ASSET_CURATION.md; GDD 14.3.5).
	struct FPaletteDef { const TCHAR* Prefix; FLinearColor Lit; FLinearColor Shade; const TCHAR* TexPath; float TexScale; float TexGain; float TexMix; const TCHAR* FallbackTexPath; float FallbackTexGain; };
	const FPaletteDef Palette[] = {
		// TexGain = 1/measured-linear-average (Tools/measure_albedo_gain.py —
		// exact sRGB EOTF + Rec.709; the toon HLSL clamps the per-pixel
		// multiplier at 2.5, so raw gains stay honest here).
		// Floor: Megascans 4K asphalt (A9, curation pass 2026-07-23, mean .081)
		// with the 2K Poly Haven asphalt as repo-tracked fallback (mean .059).
		{ TEXT("Floor"),  FLinearColor(0.165f, 0.150f, 0.160f), FLinearColor(0.055f, 0.052f, 0.080f), TEXT("/Game/Fab/Megascans/Surfaces/Asphalt_Surface_rmqlqkp0/High/rmqlqkp0_tier_1/Textures/T_rmqlqkp0_4K_B.T_rmqlqkp0_4K_B"), 700.0f, 12.41f, 0.5f, TEXT("/Game/Art/Textures/T_asphalt_03_diff.T_asphalt_03_diff"), 16.8f },  // asphalt — dark but never crushed
		{ TEXT("Wall_"),  FLinearColor(0.230f, 0.250f, 0.290f), FLinearColor(0.075f, 0.082f, 0.130f), TEXT("/Game/Art/Textures/T_concrete_block_wall_diff.T_concrete_block_wall_diff"), 500.0f, 20.5f, 0.5f },  // perimeter concrete, cold
		// Dominion post: oxide red, shade to maroon-purple (variation is
		// luminance-only, hue stays palette). West-facade banding (15.8 look-ronde
		// A/B, cam 1): under the -25/55 sun west faces sit at ndl +0.52 and land
		// in the MID band while south faces (+0.74) go lit — BldgA_W read
		// salmon-pink next to lit oxide, one asset as two color plates. OWNER
		// CALL (2026-07-23, A/B panel): variant B — BandHi stays 0.55 in the toon
		// masters and the affected west gevels take the per-gevel WestComp
		// mid-band warmte-compensatie (see WestCompLabels below; B's original
		// single-slab test is shot 00036), rolled out district-wide this round so
		// the warm look is consistent everywhere. REJECTED: (A) global BandHi
		// 0.55→0.50 in the masters (shot 00029, briefly committed as eccd6f2 and
		// reverted by this pass) — the owner overruled A's quantitative edge on
		// look. Also rejected: sun-yaw nudge — SunRotation is synced with the
		// hard-coded SunTravel in EclipseCharacter.cpp (out of scope this round).
		{ TEXT("BldgA"),  FLinearColor(0.560f, 0.160f, 0.085f), FLinearColor(0.200f, 0.045f, 0.085f), TEXT("/Game/Art/Textures/T_metal_plate_diff.T_metal_plate_diff"), 350.0f, 32.5f, 0.5f },
		{ TEXT("BldgB"),  FLinearColor(0.060f, 0.300f, 0.310f), FLinearColor(0.020f, 0.100f, 0.150f), TEXT("/Game/Art/Textures/T_CorrugatedSteel007A_diff.T_CorrugatedSteel007A_diff"), 300.0f, 2.72f, 0.45f },  // warehouse: worker teal over rusty corrugated sheet (ambientCG 007A, mean .367 — replaces corrugated_iron_02)
		// Yellow value hierarchy (15.8 look-ronde, cam 3): every yellow element in
		// the plaza midfield sat on the same value (screen 255/211/0 slabs next to
		// 245/216/1 lane paint) and the field read as one flat sheet of yellow.
		// One value step between the families now: Cover barriers (full hazard,
		// the primary cover read) > CoverB worn barriers (x0.72, alternating
		// blocks) > DecoLine route paint (saturated worn yellow, mid-band ~x0.55
		// of the barrier read). Hue stays in the hazard-amber family (15.5).
		// CoverB MUST precede Cover: PaletteForLabel prefix-matches in order.
		{ TEXT("CoverB"), FLinearColor(0.612f, 0.259f, 0.036f), FLinearColor(0.259f, 0.079f, 0.043f), nullptr, 0.0f, 1.0f, 0.0f },  // worn/older cover barriers, one value step down
		{ TEXT("Cover"),  FLinearColor(0.850f, 0.360f, 0.050f), FLinearColor(0.360f, 0.110f, 0.060f), nullptr, 0.0f, 1.0f, 0.0f },  // hazard orange, reads as cover
		{ TEXT("Skyline"), FLinearColor(0.048f, 0.044f, 0.058f), FLinearColor(0.018f, 0.017f, 0.028f), nullptr, 0.0f, 1.0f, 0.0f }, // graphite massing silhouetted in the smog (03.3); the haze adds the aerial fade
		{ TEXT("Glow"),   FLinearColor(2.200f, 1.000f, 0.300f), FLinearColor(2.200f, 1.000f, 0.300f), nullptr, 0.0f, 1.0f, 0.0f },  // sodium-orange window strips — bright enough to survive 15 km of smog
		{ TEXT("Outland"), FLinearColor(0.045f, 0.042f, 0.055f), FLinearColor(0.020f, 0.020f, 0.030f), nullptr, 0.0f, 1.0f, 0.0f }, // industrial plain under the skyline, darker than the district floor
		// Worn lane paint on the plaza asphalt. 15.8 look-ronde: the old
		// near-neutral cream (0.700/0.660/0.520) washed to the same screen value
		// as the Cover slabs (the x10 near-neutral pitfall) — now a saturated worn
		// yellow one step under CoverB, so ground routing info stays subordinate
		// to the cover read (bottom of the yellow hierarchy, see CoverB above).
		{ TEXT("DecoLine"), FLinearColor(0.420f, 0.345f, 0.095f), FLinearColor(0.180f, 0.147f, 0.058f), nullptr, 0.0f, 1.0f, 0.0f },
		{ TEXT("DecoStain"), FLinearColor(0.070f, 0.062f, 0.075f), FLinearColor(0.028f, 0.026f, 0.038f), TEXT("/Game/Art/Textures/T_Metal041B_diff.T_Metal041B_diff"), 400.0f, 3.44f, 0.7f }, // oil/rust staining — heavy-rust grunge grain (ambientCG Metal041B, mean .291; the CC0 stand-in for the scrapped Fab "Grungy Surface")
		// Plaza deck-plate apron under the well centerpiece: SciFi10_1 X-braced
		// plate (A1 recipe, mean .202), machine-local Fab pack — flat graphite
		// cel when absent. 15.8 art-fix: tint lifted x1.3 off the Wall_ pair so
		// the apron sits one value step off the asphalt (00011 read grey-on-grey);
		// hue unchanged — luminance-only, 15.5.
		{ TEXT("DecoPlaza"), FLinearColor(0.300f, 0.325f, 0.377f), FLinearColor(0.098f, 0.107f, 0.169f), TEXT("/Game/SciFi_Materials_10/Textures/1/T_4k_SciFi10_1_BaseColor.T_4k_SciFi10_1_BaseColor"), 200.0f, 4.96f, 0.7f },
		// Worker-row facade kit (the Blender gen_building_kit masonry). 15.8
		// look-ronde, cam 2: riding the near-neutral Wall_ tint the row washed to
		// chalk white next to the saturated containers (screen 153/154/163 — the
		// x10 near-neutral pitfall again, see the barrel lesson at the figures
		// below). Saturated worker-teal instead, one value step under the BldgB
		// warehouse so the yard's hierarchy holds: hue stays in the worker-teal
		// family (15.5 palette discipline), same concrete grain for the masonry
		// read (multiplicative luminance keeps saturation, so it cannot wash).
		{ TEXT("KitRow"), FLinearColor(0.070f, 0.225f, 0.235f), FLinearColor(0.026f, 0.080f, 0.115f), TEXT("/Game/Art/Textures/T_concrete_block_wall_diff.T_concrete_block_wall_diff"), 500.0f, 20.5f, 0.5f },
	};
	const FPaletteDef DefaultPalette = { TEXT(""), FLinearColor(0.35f, 0.35f, 0.38f), FLinearColor(0.12f, 0.12f, 0.16f), nullptr, 0.0f, 1.0f, 0.0f };

	const FPaletteDef& PaletteForLabel(const TCHAR* Label)
	{
		for (const FPaletteDef& Entry : Palette)
		{
			if (FCString::Strnicmp(Label, Entry.Prefix, FCString::Strlen(Entry.Prefix)) == 0)
			{
				return Entry;
			}
		}
		return DefaultPalette;
	}

	/**
	 * Per-gevel mid-band warmte-compensatie — variant B, OWNER CALL (2026-07-23,
	 * A/B panel, shot 00036 over variant A's 00029). Under the (-25,55) dusk sun
	 * west faces sit at ndl +0.52: inside the masters' 0.10..0.55 mid band, so
	 * they lerped 50% toward the shade tone and read salmon next to the lit
	 * south faces (+0.74) — one asset as two color plates. Compensated blocks
	 * get a MID whose LitColor' = 2*Lit − Shade: the mid-band lerp (halfway
	 * Shade→Lit') then lands EXACTLY on the palette's true lit tone, so the
	 * west gevel reads the same warm plate as the lit gevel. The shade band is
	 * untouched, and the Lit' overshoot only ever shows on a compensated
	 * block's thin lit end caps (wall slabs have no large lit face) or on the
	 * backdrop hulks, where 13+ km of smog eats it.
	 *
	 * Scope — exactly the gevels whose mid-band read is visible district-side:
	 *  - BldgA_W   the checkpoint-muur west gevel (B's original test slab);
	 *  - BldgA_E   the compound gevel seen through the west entrance gap (its
	 *              interior face points west — the 190/125/91 salmon in 00036);
	 *  - BldgB_W   the warehouse west gevel;
	 *  - Skyline*  backdrop hulks/chimneys/cranes, whose west sides carry the
	 *              same split at silhouette distance.
	 * Deliberately NOT compensated despite a mid-band west read: BldgB_E's
	 * interior west face (yard side) and Wall_E's interior face — cold,
	 * low-salience palettes where the mid band never produced the salmon-on-
	 * oxide split; compensating them would touch more MIDs for no visible win.
	 * NOT BldgA_N/S / BldgB_N/S: their big interior/exterior south faces sit in
	 * the LIT band — compensation would repaint the checkpoint-muur lit read
	 * itself (only their 1x1-unit west end caps stay mid, a sliver by design).
	 */
	constexpr const TCHAR* WestCompLabels[] = { TEXT("BldgA_W"), TEXT("BldgA_E"), TEXT("BldgB_W") };

	bool IsWestCompensated(const TCHAR* Label)
	{
		for (const TCHAR* CompLabel : WestCompLabels)
		{
			if (FCString::Stricmp(Label, CompLabel) == 0)
			{
				return true;
			}
		}
		return FCString::Strnicmp(Label, TEXT("Skyline"), 7) == 0;
	}

	/**
	 * One sun definition shared by the light actor AND the toon material's LightDir
	 * parameter — if these ever diverge, material banding and pawn lighting tell
	 * two different stories about where the sun is.
	 */
	// Low dusk sun: vertical faces split hard into lit/shade (the cel read), the
	// floor stays in the mid band (BandLo 0.10 < sin 25 deg = 0.423 < BandHi 0.55
	// in M_EclipseToon). West verticals (+0.52) sit in the mid band BY DESIGN —
	// the owner-chosen variant B keeps BandHi 0.55 and warms the named west
	// gevels via the per-gevel WestComp compensation instead (see WestCompLabels
	// and the BldgA palette comment for the A/B history). Also synced with the
	// hard-coded SunTravel in EclipseCharacter.cpp.
	const FRotator SunRotation(-25.0f, 55.0f, 0.0f);

	/**
	 * Cel luminance calibration for the dev box: the unlit toon emissive is scaled
	 * so the district occupies the same luminance range a lit surface had under
	 * the banked pass-19 sun (legacy intensity 8 + fill 2.5) — auto-exposure then
	 * lands on the same proven dusk grade, and the palette survives as ratios.
	 * Pinning exposure + physical lux was tried (passes 20-22 forensics) and
	 * fought three unit systems at once on SM5; the RTX pass redoes this in real
	 * physical units with Lumen.
	 */
	const float ToonEmissiveScale = 10.0f;
}

namespace EclipseGraybox
{

bool IsDistrictPresent(UWorld& World)
{
	for (TActorIterator<ATargetPoint> It(&World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Site_ControlPost")))
		{
			return true;
		}
	}
	return false;
}

void BuildDistrict(UWorld& World)
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh == nullptr)
	{
		UE_LOG(LogEclipse, Error, TEXT("Graybox: engine cube mesh missing — district not built."));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// The SM6 target (strong PC) runs full-fidelity extras the SM5 laptop
	// fallback cannot; computed here because the material choice below also
	// depends on it (15.2C + the 15.5 fidelity revision).
	const bool bFullFidelity = World.GetFeatureLevel() >= ERHIFeatureLevel::SM6;

	// One dynamic instance per palette entry (not per block) keeps the dressing cheap.
	// Toon master first (cel bands computed in-material — deterministic on the SM5
	// fallback where scene lights never reach horizontals); engine shape material
	// as the flat-color fallback when the authored asset is absent (GDD 14.3.5).
	// The lit-toon migration experiment (15.5 revision) rides behind
	// -EclipseLitToon on SM6: same banded color as BaseColor so VSM + software
	// Lumen paint real light on top. Never the default until an A/B locks it.
	const bool bLitToon = bFullFidelity && FParse::Param(FCommandLine::Get(), TEXT("EclipseLitToon"));
	UMaterialInterface* ToonMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToon.M_EclipseToon"));
	UMaterialInterface* ToonLitMaterial = bLitToon ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToonLit.M_EclipseToonLit")) : nullptr;
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	// 15.8 art-fix: ground stains ride a translucent variant of the unlit toon
	// master whose baked opacity mask fades them out organically — the opaque
	// path rendered them as hard dark rectangles ("carpet tiles", review shots
	// 00008–00013). Either asset missing = the opaque master fallback (14.3.5).
	UMaterialInterface* ToonDecalMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToonDecal.M_EclipseToonDecal"));
	UTexture* StainMask = LoadObject<UTexture>(nullptr, TEXT("/Game/Art/Decals/T_stain_mask.T_stain_mask"));
	if (ToonDecalMaterial == nullptr || StainMask == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Graybox: stain decal path incomplete (material %s, mask %s) — stains fall back to hard-edged opaque cel (run Tools/author_toon_material.py + generate_decals.py/import_generated_decals.py)."),
			ToonDecalMaterial != nullptr ? TEXT("ok") : TEXT("MISSING"), StainMask != nullptr ? TEXT("ok") : TEXT("MISSING"));
		ToonDecalMaterial = nullptr;
	}
	if (ToonMaterial == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Graybox: M_EclipseToon missing — falling back to flat engine-material dressing (GDD 14.3.5)."));
	}
	if (bLitToon && ToonLitMaterial == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Graybox: -EclipseLitToon set but M_EclipseToonLit missing — unlit toon fallback (run Tools/author_toon_material.py)."));
	}
	// Keyed by palette prefix + west-comp flag, not by color: DecoPlaza shares
	// Wall_'s graphite tint but carries its own deck-plate albedo — a color key
	// would collapse the two into whichever MID spawned first — and a
	// west-compensated gevel needs its own MID next to its uncompensated
	// siblings (BldgA_W warm vs BldgA_N/S true-lit, owner call 2026-07-23).
	TMap<uint32, UMaterialInstanceDynamic*> MidByPrefix;
	auto MidForPalette = [ToonMaterial, ToonLitMaterial, ToonDecalMaterial, StainMask, BaseMaterial, &MidByPrefix, &World](const FPaletteDef& Entry, bool bWestComp = false) -> UMaterialInstanceDynamic*
	{
		// Stains ride the translucent mask variant (15.8 art-fix) — unlit under
		// every mode, like the glow strips below.
		const bool bStainDecal = ToonDecalMaterial != nullptr && FCString::Stricmp(Entry.Prefix, TEXT("DecoStain")) == 0;
		// Glow strips stay unlit-emissive under every mode — they are light
		// sources, not lit surfaces.
		const bool bWantsLit = !bStainDecal && ToonLitMaterial != nullptr && FCString::Strnicmp(Entry.Prefix, TEXT("Glow"), 4) != 0;
		UMaterialInterface* Master = bStainDecal ? ToonDecalMaterial : (bWantsLit ? ToonLitMaterial : (ToonMaterial != nullptr ? ToonMaterial : BaseMaterial));
		if (Master == nullptr)
		{
			return nullptr; // both materials missing = plain blocks, never a crash (GDD 14.3.5)
		}
		UMaterialInstanceDynamic*& Mid = MidByPrefix.FindOrAdd(HashCombine(GetTypeHash(FStringView(Entry.Prefix)), bWestComp ? 1u : 0u));
		if (Mid == nullptr)
		{
			Mid = UMaterialInstanceDynamic::Create(Master, &World);
			if (Master != BaseMaterial)
			{
				// WestComp (variant B, owner call 2026-07-23): LitColor' =
				// 2*Lit − Shade lands the mid-band lerp exactly on the palette's
				// lit tone — see the IsWestCompensated doc above the palette.
				Mid->SetVectorParameterValue(TEXT("LitColor"), bWestComp ? Entry.Lit * 2.0f - Entry.Shade : Entry.Lit);
				Mid->SetVectorParameterValue(TEXT("ShadeColor"), Entry.Shade);
				// The material's L = -LightDir, so pass the travel direction of the sun.
				Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
				if (!bWantsLit)
				{
					// Lit variant keeps EmissiveScale 1: BaseColor is albedo,
					// the real lights supply the energy.
					Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
				}
				// CC0 albedo pass: opt in per entry; a missing asset degrades to
				// the declared fallback texture (machine-local Fab primaries →
				// repo-tracked CC0), then to flat cel — never a crash (14.3.5).
				if (Entry.TexPath != nullptr)
				{
					UTexture* Albedo = LoadObject<UTexture>(nullptr, Entry.TexPath);
					float Gain = Entry.TexGain;
					if (Albedo == nullptr && Entry.FallbackTexPath != nullptr)
					{
						UE_LOG(LogEclipse, Warning, TEXT("Graybox: albedo %s missing — falling back to %s (machine-local pack not pulled)."), Entry.TexPath, Entry.FallbackTexPath);
						Albedo = LoadObject<UTexture>(nullptr, Entry.FallbackTexPath);
						Gain = Entry.FallbackTexGain;
					}
					if (Albedo != nullptr)
					{
						Mid->SetTextureParameterValue(TEXT("AlbedoTex"), Albedo);
						Mid->SetScalarParameterValue(TEXT("TexWorldScale"), Entry.TexScale);
						Mid->SetScalarParameterValue(TEXT("AlbedoGain"), Gain);
						Mid->SetScalarParameterValue(TEXT("AlbedoMix"), Entry.TexMix);
					}
					else
					{
						UE_LOG(LogEclipse, Warning, TEXT("Graybox: albedo %s missing — flat cel fallback (run Tools/import_polyhaven_textures.py / import_cc0_albedos.py)."), Entry.TexPath);
					}
				}
				if (bStainDecal)
				{
					// Baked radial/noise falloff (Tools/generate_decals.py) —
					// mesh-UV mask over the world-aligned grunge grain.
					Mid->SetTextureParameterValue(TEXT("MaskTex"), StainMask);
				}
			}
			else
			{
				Mid->SetVectorParameterValue(TEXT("Color"), Entry.Lit);
			}
		}
		return Mid;
	};

	auto SpawnBlock = [&World, CubeMesh, &Params, &MidForPalette](const TCHAR* Label, const FVector& Location, const FVector& Scale, float YawDeg = 0.0f)
	{
		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, FRotator(0.0f, YawDeg, 0.0f), Params);
		if (Actor != nullptr)
		{
			Actor->SetMobility(EComponentMobility::Movable);
			Actor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			Actor->SetActorScale3D(Scale);
			Actor->Tags.Add(Label);
			if (UMaterialInstanceDynamic* Mid = MidForPalette(PaletteForLabel(Label), IsWestCompensated(Label)))
			{
				Actor->GetStaticMeshComponent()->SetMaterial(0, Mid);
			}
			// The engine cube's distance field breaks at these non-uniform scales
			// (up to 200:1) and poisons DFAO/DF-shadows into blanket blackness at
			// high scalability. Graybox blocks light via CSM only; the authored
			// district gets proper per-kit distance fields in the art pass.
			Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
			if (FCString::Stricmp(Label, TEXT("Floor")) == 0 ||
				FCString::Strnicmp(Label, TEXT("Skyline"), 7) == 0 ||
				FCString::Strnicmp(Label, TEXT("Glow"), 4) == 0 ||
				FCString::Strnicmp(Label, TEXT("Outland"), 7) == 0 ||
				FCString::Strnicmp(Label, TEXT("Deco"), 4) == 0)
			{
				// Ground slabs have nothing under them; skyline dressing sits
				// kilometers out — VSM pages for backdrop shadows are pure waste.
				Actor->GetStaticMeshComponent()->SetCastShadow(false);
			}
			if (FCString::Strnicmp(Label, TEXT("Deco"), 4) == 0 ||
				FCString::Strnicmp(Label, TEXT("Glow"), 4) == 0)
			{
				// Dressing never collides: lane paint, stains, and light strips
				// must not perturb nav, cover queries, or hitscan (SPEC-P1-05).
				Actor->SetActorEnableCollision(false);
			}
		}
	};

	for (const FBlockDef& Block : Blocks)
	{
		SpawnBlock(Block.Label, FVector(Block.X, Block.Y, Block.Z), FVector(Block.SX, Block.SY, Block.SZ));
	}

	int32 CoverIndex = 0;
	for (const FPointDef& Cover : CoverPoints)
	{
		const bool bRotated = (CoverIndex++ % 2) == 0;
		// Alternating blocks take the CoverB tint (one value step down): the
		// midfield's yellow gets hierarchy instead of twenty identical slabs
		// (15.8 look-ronde, cam 3). Same hazard hue, same collision, same cover
		// gameplay — nothing queries the label tag.
		SpawnBlock(bRotated ? TEXT("Cover") : TEXT("CoverB"), FVector(Cover.X, Cover.Y, 60.0f),
			FVector(bRotated ? 3.0f : 1.0f, bRotated ? 1.0f : 3.0f, 1.2f));
	}

	// PLACEHOLDER(15.4): first real prop meshes — CC0 Poly Haven FBX restyled
	// through the toon pipeline (mesh-UV albedo, luminance-only; provenance in
	// Content/Art/Textures/SOURCES.md). Dressing tier for now: no collision —
	// a later pass promotes correctly-sized props to real cover WITH the squad
	// scenario suite re-run (SPEC-P1-06); missing assets degrade to nothing.
	{
		struct FPropDef { const TCHAR* Label; const TCHAR* MeshPath; const TCHAR* TexPath; float TexGain; FLinearColor Lit; FLinearColor Shade; };
		const FPropDef Props[] = {
			{ TEXT("Prop_Barrel"), TEXT("/Game/Art/Props/Barrel_01.Barrel_01"), TEXT("/Game/Art/Textures/T_Barrel_01_diff.T_Barrel_01_diff"), 17.6f, FLinearColor(0.160f, 0.100f, 0.070f), FLinearColor(0.055f, 0.042f, 0.062f) },
			{ TEXT("Prop_Barrier"), TEXT("/Game/Art/Props/concrete_road_barrier.concrete_road_barrier"), TEXT("/Game/Art/Textures/T_concrete_road_barrier_diff.T_concrete_road_barrier_diff"), 6.7f, FLinearColor(0.26f, 0.27f, 0.30f), FLinearColor(0.085f, 0.090f, 0.140f) },
			{ TEXT("Prop_Crate"), TEXT("/Game/Art/Props/plastic_crate_03.plastic_crate_03"), TEXT("/Game/Art/Textures/T_plastic_crate_03_diff.T_plastic_crate_03_diff"), 8.0f, FLinearColor(0.080f, 0.280f, 0.300f), FLinearColor(0.030f, 0.100f, 0.150f) },
		};

		auto SpawnProp = [&World, &Params](UStaticMesh* Mesh, UMaterialInstanceDynamic* Mid, const FVector& Location, float YawDeg, float Scale)
		{
			AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, FRotator(0.0f, YawDeg, 0.0f), Params);
			if (Actor != nullptr)
			{
				Actor->SetMobility(EComponentMobility::Movable);
				Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
				Actor->SetActorScale3D(FVector(Scale));
				if (Mid != nullptr)
				{
					// Every slot: imported FBX props carry multiple material
					// slots; any slot left on the default engine material
					// renders as pale gray and breaks the palette (first prop
					// round, camera 2 — the "white barrels").
					for (int32 SlotIndex = 0; SlotIndex < Actor->GetStaticMeshComponent()->GetNumMaterials(); ++SlotIndex)
					{
						Actor->GetStaticMeshComponent()->SetMaterial(SlotIndex, Mid);
					}
				}
				Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
				Actor->SetActorEnableCollision(false);
			}
		};

		FRandomStream PropRng(211);
		for (int32 PropIndex = 0; PropIndex < static_cast<int32>(UE_ARRAY_COUNT(Props)); ++PropIndex)
		{
			const FPropDef& Prop = Props[PropIndex];
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Prop.MeshPath);
			if (Mesh == nullptr)
			{
				UE_LOG(LogEclipse, Warning, TEXT("Graybox: prop %s missing — skipped (run Tools/import_polyhaven_props.py)."), Prop.MeshPath);
				continue;
			}
			UTexture* Tex = LoadObject<UTexture>(nullptr, Prop.TexPath);
			UMaterialInstanceDynamic* Mid = nullptr;
			if (ToonMaterial != nullptr)
			{
				Mid = UMaterialInstanceDynamic::Create(ToonMaterial, &World);
				Mid->SetVectorParameterValue(TEXT("LitColor"), Prop.Lit);
				Mid->SetVectorParameterValue(TEXT("ShadeColor"), Prop.Shade);
				Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
				Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
				Mid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
				Mid->SetScalarParameterValue(TEXT("AlbedoGain"), Prop.TexGain);
				if (Tex != nullptr)
				{
					Mid->SetTextureParameterValue(TEXT("AlbedoTex"), Tex);
					Mid->SetScalarParameterValue(TEXT("AlbedoMix"), 0.85f);
				}
			}
			UE_LOG(LogEclipse, Display, TEXT("Graybox: prop %s slots=%d tex=%s mid=%s"),
				Prop.Label, Mesh->GetStaticMaterials().Num(),
				Tex != nullptr ? TEXT("ok") : TEXT("MISSING"),
				Mid != nullptr ? TEXT("ok") : TEXT("null"));

			if (PropIndex == 0)
			{
				// Barrel clusters: warehouse yard, control-post rear, road side.
				const FVector Centers[] = { FVector(-3300, 3000, 0), FVector(6300, -2600, 0), FVector(2100, 750, 0), FVector(-8600, 8300, 0) };
				for (const FVector& Center : Centers)
				{
					const int32 Count = 3 + (PropRng.RandRange(0, 1));
					for (int32 Index = 0; Index < Count; ++Index)
					{
						SpawnProp(Mesh, Mid,
							Center + FVector(PropRng.FRandRange(-200.0f, 200.0f), PropRng.FRandRange(-200.0f, 200.0f), 0.0f),
							PropRng.FRandRange(0.0f, 360.0f), 1.4f);
					}
				}
			}
			else if (PropIndex == 1)
			{
				// Checkpoint barriers flanking the EW artery near the compound approach.
				for (int32 Index = 0; Index < 6; ++Index)
				{
					const float X = -5500.0f + Index * 1700.0f;
					const float Y = (Index % 2 == 0) ? 640.0f : -640.0f;
					SpawnProp(Mesh, Mid, FVector(X, Y, 0), (Index % 2 == 0) ? 8.0f : 172.0f, 1.8f);
				}
			}
			else
			{
				// Crate stacks in the warehouse yard: two on the ground, one on top.
				const FVector Stacks[] = { FVector(-4300, 3350, 0), FVector(-3700, 2500, 0) };
				for (const FVector& Base : Stacks)
				{
					SpawnProp(Mesh, Mid, Base, PropRng.FRandRange(0.0f, 360.0f), 2.2f);
					SpawnProp(Mesh, Mid, Base + FVector(150, 40, 0), PropRng.FRandRange(0.0f, 360.0f), 2.2f);
					SpawnProp(Mesh, Mid, Base + FVector(75, 20, 92), PropRng.FRandRange(0.0f, 360.0f), 2.2f);
				}
			}
		}
	}

	// PLACEHOLDER(15.5): occupation decals — Pillow-generated luminance patterns
	// (Tools/generate_decals.py) tinted by the palette, as thin no-collision
	// planes proud of their host surfaces. Dominion propaganda on the compound,
	// hazard pads at the crossing, rebel stencils near the entries (03.3's
	// "the world is ruled, and that shows" + the resistance answering back).
	{
		// TexGain = 1/measured-linear-average per generated map (same discipline
		// as the surface textures: measured, never guessed).
		struct FDecalDef { const TCHAR* TexPath; float TexGain; FLinearColor Lit; FLinearColor Shade; FVector Location; FVector Scale; };
		const FDecalDef Decals[] = {
			// Dominion white-gold posters: compound north, east, and south walls.
			{ TEXT("/Game/Art/Decals/T_decal_poster_diff.T_decal_poster_diff"), 7.8f, FLinearColor(0.300f, 0.255f, 0.165f), FLinearColor(0.120f, 0.100f, 0.070f), FVector(4600, -1146, 210), FVector(1.6f, 0.04f, 2.4f) },
			{ TEXT("/Game/Art/Decals/T_decal_poster_diff.T_decal_poster_diff"), 7.8f, FLinearColor(0.300f, 0.255f, 0.165f), FLinearColor(0.120f, 0.100f, 0.070f), FVector(5854, -2000, 210), FVector(0.04f, 1.6f, 2.4f) },
			{ TEXT("/Game/Art/Decals/T_decal_poster_diff.T_decal_poster_diff"), 7.8f, FLinearColor(0.300f, 0.255f, 0.165f), FLinearColor(0.120f, 0.100f, 0.070f), FVector(4600, -2854, 210), FVector(1.6f, 0.04f, 2.4f) },
			// Hazard pads at the artery/cross-street corners, amber. Gain 3.08 =
			// 1/mean of the FIXED worn-stripe map (15.8 art-fix: the old
			// generator tiled its bands edge-to-edge, so the pads were solid
			// 225 plates reading as flat pure-yellow quads — shot 00013).
			{ TEXT("/Game/Art/Decals/T_decal_hazard_diff.T_decal_hazard_diff"), 3.08f, FLinearColor(0.300f, 0.200f, 0.030f), FLinearColor(0.120f, 0.080f, 0.020f), FVector(-3200, 700, 4), FVector(2.4f, 2.4f, 0.04f) },
			{ TEXT("/Game/Art/Decals/T_decal_hazard_diff.T_decal_hazard_diff"), 3.08f, FLinearColor(0.300f, 0.200f, 0.030f), FLinearColor(0.120f, 0.080f, 0.020f), FVector(-4800, -700, 4), FVector(2.4f, 2.4f, 0.04f) },
			// Rebel eclipse stencils: west wall by Entry_Main, warehouse south face.
			{ TEXT("/Game/Art/Decals/T_decal_stencil_diff.T_decal_stencil_diff"), 7.1f, FLinearColor(0.300f, 0.060f, 0.050f), FLinearColor(0.110f, 0.030f, 0.035f), FVector(-9944, 420, 240), FVector(0.04f, 2.0f, 2.0f) },
			{ TEXT("/Game/Art/Decals/T_decal_stencil_diff.T_decal_stencil_diff"), 7.1f, FLinearColor(0.300f, 0.060f, 0.050f), FLinearColor(0.110f, 0.030f, 0.035f), FVector(-4300, 2146, 220), FVector(1.8f, 0.04f, 1.8f) },
		};

		// Same master choice as the palette blocks and the well (15.8 look-ronde,
		// lit-A/B agendapunt): under -EclipseLitToon the poster/hazard/stencil
		// planes are lit surfaces like the walls they hang on, not unlit
		// hold-outs. Default (no flag) is bit-identical unlit.
		UMaterialInterface* DecalMaster = ToonLitMaterial != nullptr ? ToonLitMaterial : ToonMaterial;
		for (const FDecalDef& Decal : Decals)
		{
			UTexture* Tex = LoadObject<UTexture>(nullptr, Decal.TexPath);
			if (Tex == nullptr || DecalMaster == nullptr)
			{
				UE_LOG(LogEclipse, Warning, TEXT("Graybox: decal %s missing — skipped (run Tools/generate_decals.py + import)."), Decal.TexPath);
				continue;
			}
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(DecalMaster, &World);
			Mid->SetVectorParameterValue(TEXT("LitColor"), Decal.Lit);
			Mid->SetVectorParameterValue(TEXT("ShadeColor"), Decal.Shade);
			Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
			if (ToonLitMaterial == nullptr)
			{
				// Lit variant keeps EmissiveScale 1: BaseColor is albedo.
				Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
			}
			Mid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
			Mid->SetScalarParameterValue(TEXT("AlbedoGain"), Decal.TexGain);
			Mid->SetTextureParameterValue(TEXT("AlbedoTex"), Tex);
			Mid->SetScalarParameterValue(TEXT("AlbedoMix"), 1.0f);
			AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Decal.Location, FRotator::ZeroRotator, Params);
			if (Actor != nullptr)
			{
				Actor->SetMobility(EComponentMobility::Movable);
				Actor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
				Actor->SetActorScale3D(Decal.Scale);
				Actor->GetStaticMeshComponent()->SetMaterial(0, Mid);
				Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
				Actor->GetStaticMeshComponent()->SetCastShadow(false);
				Actor->SetActorEnableCollision(false);
				Actor->Tags.Add(TEXT("Deco_Decal"));
			}
		}
	}

	// PLACEHOLDER(15.8): warning-sign placards — FD_WarningSigns_V1 (Fab free
	// pack, machine-local) restyled through the toon pipeline like the car
	// wrecks: the sign's own albedo as AlbedoTex (UVMode 1) over amber/red cel
	// tints. The pack ships decal cutouts over green-screen photo backing
	// (headless audit 2026-07-23), so these ride the background-cleaned
	// placards from Tools/prepare_warning_signs.py (import via
	// Tools/import_warning_signs.py); gains are the measured 1/linear-mean per
	// placard — the car-block 3.2 assumes a full-frame albedo, these are
	// bright marks on a dark plate (poster-decal recipe). No collision;
	// missing textures = skipped (GDD 14.3.5).
	{
		struct FSignDef { const TCHAR* TexPath; float TexGain; FLinearColor Lit; FLinearColor Shade; FVector Location; FVector Scale; };
		// 15.8 art-fix (review shots 00008/00012/00013): the placards read as
		// near-black plates. Value-normalized albedos (prepare_warning_signs.py
		// NORM_TARGET pass) + re-measured gains, and the tints step up toward —
		// never onto — the sodium strips' emissive budget: lit x1.4, shade to
		// ~0.6x lit. Signs are man-made reflective placards, so their lit/shade
		// gap is deliberately shallower than the architecture's; hue unchanged
		// (15.5: the palette keeps hue authority, this is luminance-only).
		const FLinearColor SignRedLit(0.420f, 0.084f, 0.070f), SignRedShade(0.250f, 0.050f, 0.042f);      // checkpoint red (stencil family)
		const FLinearColor SignAmberLit(0.420f, 0.280f, 0.042f), SignAmberShade(0.250f, 0.167f, 0.025f);  // hazard amber (pad family)
		const FSignDef Signs[] = {
			// Gains: measured 1/linear-mean per normalized placard (tool output
			// 2026-07-23 fix round; ASSET_CURATION.md §8 table updated).
			// STOP hung under the gate portal's west beam, facing the Entry_Main approach.
			{ TEXT("/Game/Art/Decals/T_sign_stop_diff.T_sign_stop_diff"), 8.9f, SignRedLit, SignRedShade, FVector(-8850, 0, 320), FVector(0.04f, 1.0f, 1.0f) },
			// Radiation placard on the crossing lamp pole (artery x cross-street).
			{ TEXT("/Game/Art/Decals/T_sign_radiation_diff.T_sign_radiation_diff"), 10.2f, SignAmberLit, SignAmberShade, FVector(-4650, -675, 230), FVector(0.9f, 0.04f, 0.9f) },
			// TOXIC on the west wall inner face — the Dominion answer to the rebel stencil across the Entry_Main gap.
			{ TEXT("/Game/Art/Decals/T_sign_toxic_diff.T_sign_toxic_diff"), 5.9f, SignAmberLit, SignAmberShade, FVector(-9944, -350, 260), FVector(0.04f, 1.4f, 1.4f) },
			// Curation pass 2026-07-23, the four new placards (ASSET_CURATION.md §8):
			// ROUTE arrow on the second crossing lamp — the artery choke's checkpoint
			// routing, paired face-on with the radiation placard (review camera 6).
			{ TEXT("/Game/Art/Decals/T_sign_route_diff.T_sign_route_diff"), 12.7f, SignRedLit, SignRedShade, FVector(-4230, -675, 240), FVector(0.9f, 0.04f, 0.9f) },
			// LABOR beside the warehouse yard's east gate gap (Underworks labor
			// stories, art bible §2.2) — on BldgB_E's east face, toward Spawn_Yard.
			{ TEXT("/Game/Art/Decals/T_sign_labor_diff.T_sign_labor_diff"), 6.4f, SignAmberLit, SignAmberShade, FVector(-3146, 3250, 260), FVector(0.04f, 1.0f, 1.0f) },
			// BLAST on the Dominion post's west face — munitions fence warning on
			// the checkpoint approach (amber pops on the oxide-red facade).
			{ TEXT("/Game/Art/Decals/T_sign_blast_diff.T_sign_blast_diff"), 7.0f, SignAmberLit, SignAmberShade, FVector(4146, -2400, 250), FVector(0.04f, 0.9f, 0.9f) },
			// REACTOR exclusion triangle on the west perimeter wall north of the
			// gate — Dominion exclusion zone stacked over the rebel stencil story.
			{ TEXT("/Game/Art/Decals/T_sign_reactor_diff.T_sign_reactor_diff"), 14.8f, SignRedLit, SignRedShade, FVector(-9944, 700, 270), FVector(0.04f, 1.2f, 1.2f) },
		};

		// Sign placards follow the -EclipseLitToon master choice like the decals
		// above (well-fix pattern); default unlit path bit-identical.
		UMaterialInterface* SignMaster = ToonLitMaterial != nullptr ? ToonLitMaterial : ToonMaterial;
		for (const FSignDef& Sign : Signs)
		{
			UTexture* Tex = LoadObject<UTexture>(nullptr, Sign.TexPath);
			if (Tex == nullptr || SignMaster == nullptr)
			{
				UE_LOG(LogEclipse, Warning, TEXT("Graybox: sign %s missing — skipped (run Tools/import_warning_signs.py chain)."), Sign.TexPath);
				continue;
			}
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(SignMaster, &World);
			Mid->SetVectorParameterValue(TEXT("LitColor"), Sign.Lit);
			Mid->SetVectorParameterValue(TEXT("ShadeColor"), Sign.Shade);
			Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
			if (ToonLitMaterial == nullptr)
			{
				// Lit variant keeps EmissiveScale 1: BaseColor is albedo.
				Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
			}
			Mid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
			Mid->SetScalarParameterValue(TEXT("AlbedoGain"), Sign.TexGain);
			Mid->SetTextureParameterValue(TEXT("AlbedoTex"), Tex);
			Mid->SetScalarParameterValue(TEXT("AlbedoMix"), 0.9f);
			AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Sign.Location, FRotator::ZeroRotator, Params);
			if (Actor != nullptr)
			{
				Actor->SetMobility(EComponentMobility::Movable);
				Actor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
				Actor->SetActorScale3D(Sign.Scale);
				Actor->GetStaticMeshComponent()->SetMaterial(0, Mid);
				Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
				Actor->GetStaticMeshComponent()->SetCastShadow(false);
				Actor->SetActorEnableCollision(false);
				Actor->Tags.Add(TEXT("Deco_Sign"));
			}
		}
	}

	// PLACEHOLDER(15.5): plaza centerpiece — the curated Paragon basin ring (A2,
	// phase0/ASSET_CURATION.md: 11.8 m industrial vat / landing-pad rim) on a
	// SciFi10 deck-plate apron, in the open plaza north of the artery. Both ride
	// graphite toon MIDs, world-aligned luminance albedos (UVMode 0). Position is
	// tile-locked: (600, 1800) / TexWorldScale 1200 = frac (0.5, 0.5), so the
	// circular pad graphic lands centered on the ring instead of quartered at
	// the tile seams. Dressing tier: no collision; a missing machine-local pack
	// degrades to the apron alone (GDD 14.3.5).
	{
		const FVector PlazaCenter(600, 1800, 0);
		SpawnBlock(TEXT("DecoPlaza"), PlazaCenter + FVector(0, 0, 2), FVector(20.0f, 20.0f, 0.05f));

		UStaticMesh* Well = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/ParagonMinions/FX/Meshes/Environment/Maps/Agora/SM_Well_Center_FX.SM_Well_Center_FX"));
		// Same master choice as the palette blocks (code-review note, 15.8 fix
		// round): under -EclipseLitToon the ring is a lit surface like the rest
		// of the district, not a hard-coded unlit hold-out.
		UMaterialInterface* WellMaster = ToonLitMaterial != nullptr ? ToonLitMaterial : ToonMaterial;
		if (Well != nullptr && WellMaster != nullptr)
		{
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(WellMaster, &World);
			// 15.8 art-fix: the graphite ring read grey-on-grey in the aerial
			// (00011) — the centerpiece now carries an amber accent from the
			// hazard/sodium family (hue in-palette, 03.3) with a modest bump on
			// the unlit emissive path so the plaza's middelpunt actually reads.
			// Stays well under the Glow strips' budget (2.2 x10).
			Mid->SetVectorParameterValue(TEXT("LitColor"), FLinearColor(0.520f, 0.310f, 0.060f));   // amber centerpiece accent
			Mid->SetVectorParameterValue(TEXT("ShadeColor"), FLinearColor(0.210f, 0.115f, 0.040f));
			Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
			if (ToonLitMaterial == nullptr)
			{
				// Lit variant keeps EmissiveScale 1 (albedo path, real lights).
				Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale * 1.3f);
			}
			if (UTexture* PadTex = LoadObject<UTexture>(nullptr, TEXT("/Game/SciFi_Materials_10/Textures/2/T_4k_SciFi10_2_BaseColor.T_4k_SciFi10_2_BaseColor")))
			{
				// Circular pad graphic, measured mean-lin .742 -> gain 1.35.
				Mid->SetTextureParameterValue(TEXT("AlbedoTex"), PadTex);
				Mid->SetScalarParameterValue(TEXT("TexWorldScale"), 1200.0f);
				Mid->SetScalarParameterValue(TEXT("AlbedoGain"), 1.35f);
				Mid->SetScalarParameterValue(TEXT("AlbedoMix"), 0.8f);
			}
			AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(PlazaCenter, FRotator::ZeroRotator, Params);
			if (Actor != nullptr)
			{
				Actor->SetMobility(EComponentMobility::Movable);
				Actor->GetStaticMeshComponent()->SetStaticMesh(Well);
				for (int32 SlotIndex = 0; SlotIndex < Actor->GetStaticMeshComponent()->GetNumMaterials(); ++SlotIndex)
				{
					Actor->GetStaticMeshComponent()->SetMaterial(SlotIndex, Mid);
				}
				Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
				Actor->SetActorEnableCollision(false);
				Actor->Tags.Add(TEXT("Deco_Plaza"));
			}
		}
		else
		{
			// Raw pack material must never show (15.5) — without the toon master
			// the ring stays out entirely; the apron still marks the plaza.
			UE_LOG(LogEclipse, Display, TEXT("Graybox: plaza well skipped (ParagonMinions pack or toon master absent) — deck-plate apron only (14.3.5)."));
		}
	}

	// PLACEHOLDER(15.7/09): first inhabitants — Quaternius CC0 animated
	// characters (SOURCES.md) as looping-Idle dressing figures, toon-restyled
	// flat cel (palette hue, no albedo). Visual tier only: no collision, no AI,
	// no perception — the real crowd/NPC layer is Part 9 work; these prove the
	// character silhouette + palette read at command distance.
	{
		struct FFigureDef { const TCHAR* MeshPath; const TCHAR* AnimPath; FLinearColor Lit; FLinearColor Shade; FVector Location; float Yaw; };
		const FLinearColor EnforcerLit(0.300f, 0.235f, 0.095f), EnforcerShade(0.090f, 0.072f, 0.055f);   // Dominion white-gold (saturated — near-neutral washes to gray at x10, see the barrel lesson)
		const FLinearColor CivilianLit(0.060f, 0.130f, 0.160f), CivilianShade(0.024f, 0.050f, 0.070f);   // worker gray-teal
		const FFigureDef Figures[] = {
			{ TEXT("/Game/Art/Characters/BlueSoldier_Male/BlueSoldier_Male.BlueSoldier_Male"), TEXT("/Game/Art/Characters/BlueSoldier_Male/BlueSoldier_MaleCharacterArmature_Idle.BlueSoldier_MaleCharacterArmature_Idle"), EnforcerLit, EnforcerShade, FVector(4150, -1750, 0), 180.0f },
			{ TEXT("/Game/Art/Characters/BlueSoldier_Female/BlueSoldier_Female.BlueSoldier_Female"), TEXT("/Game/Art/Characters/BlueSoldier_Female/BlueSoldier_FemaleCharacterArmature_Idle.BlueSoldier_FemaleCharacterArmature_Idle"), EnforcerLit, EnforcerShade, FVector(-450, 630, 0), 250.0f },
			{ TEXT("/Game/Art/Characters/Casual_Bald/Casual_Bald.Casual_Bald"), TEXT("/Game/Art/Characters/Casual_Bald/Casual_BaldCharacterArmature_Idle.Casual_BaldCharacterArmature_Idle"), CivilianLit, CivilianShade, FVector(-4250, 3250, 0), 30.0f },
			{ TEXT("/Game/Art/Characters/Casual2_Male/Casual2_Male.Casual2_Male"), TEXT("/Game/Art/Characters/Casual2_Male/Casual2_MaleCharacterArmature_Idle.Casual2_MaleCharacterArmature_Idle"), CivilianLit, CivilianShade, FVector(2050, 850, 0), 300.0f },
			// 15.8 patrol pass: two enforcers looping the Walk cycle along the
			// EW artery (same visual tier — no AI, no nav; the real patrol
			// brain is Part 9). One paces the gate portal, one the crossing,
			// so the stride reads in the new review frames.
			{ TEXT("/Game/Art/Characters/BlueSoldier_Male/BlueSoldier_Male.BlueSoldier_Male"), TEXT("/Game/Art/Characters/BlueSoldier_Male/BlueSoldier_MaleCharacterArmature_Walk.BlueSoldier_MaleCharacterArmature_Walk"), EnforcerLit, EnforcerShade, FVector(-8700, -100, 0), 0.0f },
			{ TEXT("/Game/Art/Characters/BlueSoldier_Female/BlueSoldier_Female.BlueSoldier_Female"), TEXT("/Game/Art/Characters/BlueSoldier_Female/BlueSoldier_FemaleCharacterArmature_Walk.BlueSoldier_FemaleCharacterArmature_Walk"), EnforcerLit, EnforcerShade, FVector(-4100, -350, 0), 180.0f },
		};

		for (const FFigureDef& Figure : Figures)
		{
			USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, Figure.MeshPath);
			if (Mesh == nullptr)
			{
				UE_LOG(LogEclipse, Warning, TEXT("Graybox: figure %s missing — skipped (run Tools/import_quaternius_characters.py)."), Figure.MeshPath);
				continue;
			}
			ASkeletalMeshActor* Actor = World.SpawnActor<ASkeletalMeshActor>(Figure.Location, FRotator(0.0f, Figure.Yaw, 0.0f), Params);
			if (Actor == nullptr)
			{
				continue;
			}
			USkeletalMeshComponent* Component = Actor->GetSkeletalMeshComponent();
			Component->SetSkeletalMesh(Mesh);
			if (UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, Figure.AnimPath))
			{
				Component->SetAnimationMode(EAnimationMode::AnimationSingleNode);
				Component->PlayAnimation(Idle, /*bLooping*/ true);
			}
			if (ToonMaterial != nullptr)
			{
				UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(ToonMaterial, &World);
				Mid->SetVectorParameterValue(TEXT("LitColor"), Figure.Lit);
				Mid->SetVectorParameterValue(TEXT("ShadeColor"), Figure.Shade);
				Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
				Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
				for (int32 SlotIndex = 0; SlotIndex < Component->GetNumMaterials(); ++SlotIndex)
				{
					Component->SetMaterial(SlotIndex, Mid);
				}
			}
			Actor->SetActorEnableCollision(false);
			Actor->Tags.Add(TEXT("Deco_Figure"));
		}
	}

	// PLACEHOLDER(15.4): wrecked cars (RustyCarsFree Fab pack, machine-local)
	// as street dressing, toon-restyled — rust-tinted cel with the car's own
	// texture as luminance detail. No collision at this tier (promotion to nav
	// obstacles goes through the squad scenario suite). Missing pack = skipped.
	{
		auto FindBaseTexture = [](UMaterialInterface* Material) -> UTexture*
		{
			if (Material == nullptr) { return nullptr; }
			TArray<UTexture*> Textures;
			Material->GetUsedTextures(Textures, EMaterialQualityLevel::High, true, ERHIFeatureLevel::SM6, true);
			UTexture* Fallback = nullptr;
			for (UTexture* Texture : Textures)
			{
				const FString TexName = Texture->GetName();
				if (TexName.Contains(TEXT("Diff")) || TexName.Contains(TEXT("Base")) || TexName.Contains(TEXT("Alb")) || TexName.Contains(TEXT("Color")))
				{
					return Texture;
				}
				if (Fallback == nullptr) { Fallback = Texture; }
			}
			return Fallback;
		};

		struct FCarDef { const TCHAR* MeshPath; FVector Location; float Yaw; };
		const FCarDef Cars[] = {
			{ TEXT("/Game/RustyCarsFree/Geometries/SM_asset_00.SM_asset_00"), FVector(7600, 950, 0), 14.0f },
			{ TEXT("/Game/RustyCarsFree/Geometries/SM_asset_01.SM_asset_01"), FVector(8600, -820, 0), 188.0f },
			{ TEXT("/Game/RustyCarsFree/Geometries/SM_asset_02.SM_asset_02"), FVector(-8500, -1100, 0), 235.0f },
			{ TEXT("/Game/RustyCarsFree/Geometries/SM_asset_03.SM_asset_03"), FVector(-6800, 5600, 0), 75.0f },
			{ TEXT("/Game/RustyCarsFree/Geometries/SM_asset_04.SM_asset_04"), FVector(1500, -6900, 0), 320.0f },
		};
		const FLinearColor CarLit(0.190f, 0.115f, 0.070f), CarShade(0.070f, 0.045f, 0.055f); // rust wrecks
		int32 CarsPlaced = 0;
		for (const FCarDef& Car : Cars)
		{
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Car.MeshPath);
			if (Mesh == nullptr)
			{
				continue; // pack not pulled on this machine — dressing degrades silently (14.3.5)
			}
			AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Car.Location, FRotator(0.0f, Car.Yaw, 0.0f), Params);
			if (Actor == nullptr)
			{
				continue;
			}
			Actor->SetMobility(EComponentMobility::Movable);
			Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
			Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
			Actor->SetActorEnableCollision(false);
			Actor->Tags.Add(TEXT("Deco_Car"));
			if (ToonMaterial != nullptr)
			{
				for (int32 SlotIndex = 0; SlotIndex < Actor->GetStaticMeshComponent()->GetNumMaterials(); ++SlotIndex)
				{
					UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(ToonMaterial, &World);
					Mid->SetVectorParameterValue(TEXT("LitColor"), CarLit);
					Mid->SetVectorParameterValue(TEXT("ShadeColor"), CarShade);
					Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
					Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
					Mid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
					if (UTexture* BaseTexture = FindBaseTexture(Actor->GetStaticMeshComponent()->GetMaterial(SlotIndex)))
					{
						Mid->SetTextureParameterValue(TEXT("AlbedoTex"), BaseTexture);
						Mid->SetScalarParameterValue(TEXT("AlbedoGain"), 3.2f);
						Mid->SetScalarParameterValue(TEXT("AlbedoMix"), 0.9f);
					}
					Actor->GetStaticMeshComponent()->SetMaterial(SlotIndex, Mid);
				}
			}
			++CarsPlaced;
		}
		if (CarsPlaced > 0)
		{
			UE_LOG(LogEclipse, Display, TEXT("Graybox: %d wrecked cars dressed (RustyCarsFree)."), CarsPlaced);
		}
	}

	// Self-authored props (Tools/blender/gen_street_props.py — the first
	// hand-built ECLIPSE assets, 15.5 hero-asset ladder): sodium lamps with
	// separate glow planes, propaganda boards with poster planes, vents,
	// cable arcs, barricades. All no-collision dressing; missing assets =
	// skipped (agent output not imported on this machine yet, 14.3.5).
	{
		const FLinearColor MetalLit(0.105f, 0.105f, 0.120f), MetalShade(0.040f, 0.040f, 0.052f);
		const FLinearColor OliveLit(0.120f, 0.110f, 0.070f), OliveShade(0.050f, 0.045f, 0.035f);

		// bFollowLitMaster: poster planes follow the -EclipseLitToon master choice
		// like the wall decals (15.8 look-ronde, well-fix pattern). Metal/olive
		// prop tints stay on the unlit master for now (lit-A/B ronde agendapunt,
		// with the cars/figures); glow planes are light sources and never migrate.
		auto MakeMid = [ToonMaterial, ToonLitMaterial, &World](const FLinearColor& Lit, const FLinearColor& Shade, bool bFollowLitMaster = false) -> UMaterialInstanceDynamic*
		{
			const bool bLitMaster = bFollowLitMaster && ToonLitMaterial != nullptr;
			UMaterialInterface* Master = bLitMaster ? ToonLitMaterial : ToonMaterial;
			if (Master == nullptr) { return nullptr; }
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Master, &World);
			Mid->SetVectorParameterValue(TEXT("LitColor"), Lit);
			Mid->SetVectorParameterValue(TEXT("ShadeColor"), Shade);
			Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
			if (!bLitMaster)
			{
				// Lit variant keeps EmissiveScale 1: BaseColor is albedo.
				Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
			}
			return Mid;
		};
		UMaterialInstanceDynamic* MetalMid = MakeMid(MetalLit, MetalShade);
		UMaterialInstanceDynamic* OliveMid = MakeMid(OliveLit, OliveShade);
		UMaterialInstanceDynamic* GenGlowMid = MakeMid(FLinearColor(2.2f, 1.0f, 0.3f), FLinearColor(2.2f, 1.0f, 0.3f));
		UMaterialInstanceDynamic* PosterMid = MakeMid(FLinearColor(0.300f, 0.255f, 0.165f), FLinearColor(0.120f, 0.100f, 0.070f), /*bFollowLitMaster*/ true);
		if (PosterMid != nullptr)
		{
			if (UTexture* PosterTex = LoadObject<UTexture>(nullptr, TEXT("/Game/Art/Decals/T_decal_poster_diff.T_decal_poster_diff")))
			{
				PosterMid->SetTextureParameterValue(TEXT("AlbedoTex"), PosterTex);
				PosterMid->SetScalarParameterValue(TEXT("AlbedoGain"), 7.8f);
				PosterMid->SetScalarParameterValue(TEXT("AlbedoMix"), 1.0f);
				PosterMid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
			}
		}

		auto SpawnGen = [&World, &Params](UStaticMesh* Mesh, UMaterialInstanceDynamic* Mid, const FVector& Location, float Yaw, const FVector& Scale = FVector(1.0f))
		{
			if (Mesh == nullptr) { return; }
			AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, FRotator(0.0f, Yaw, 0.0f), Params);
			if (Actor == nullptr) { return; }
			Actor->SetMobility(EComponentMobility::Movable);
			Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
			Actor->SetActorScale3D(Scale);
			Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
			Actor->SetActorEnableCollision(false);
			Actor->Tags.Add(TEXT("Deco_Gen"));
			if (Mid != nullptr)
			{
				for (int32 SlotIndex = 0; SlotIndex < Actor->GetStaticMeshComponent()->GetNumMaterials(); ++SlotIndex)
				{
					Actor->GetStaticMeshComponent()->SetMaterial(SlotIndex, Mid);
				}
			}
		};

		UStaticMesh* Lamp = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Prop_SodiumLamp.SM_Prop_SodiumLamp"));
		UStaticMesh* LampGlow = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/GlowPlane.GlowPlane"));
		UStaticMesh* Board = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Prop_PropagandaBoard.SM_Prop_PropagandaBoard"));
		UStaticMesh* Poster = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/PosterPlane.PosterPlane"));
		UStaticMesh* Vent = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Prop_VentUnit.SM_Prop_VentUnit"));
		UStaticMesh* Cable = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Prop_CableArc.SM_Prop_CableArc"));
		UStaticMesh* Barricade = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Prop_Barricade.SM_Prop_Barricade"));

		const struct { FVector Loc; float Yaw; } Lamps[] = {
			{ FVector(-4650, -700, 0), 90.0f }, { FVector(-1250, 700, 0), 270.0f }, { FVector(2150, -700, 0), 90.0f },
			{ FVector(4150, -1500, 0), 200.0f }, { FVector(-4000, 2100, 0), 20.0f },
		};
		for (const auto& L : Lamps)
		{
			SpawnGen(Lamp, MetalMid, L.Loc, L.Yaw);
			SpawnGen(LampGlow, GenGlowMid, L.Loc, L.Yaw);
		}
		const struct { FVector Loc; float Yaw; } Boards[] = {
			{ FVector(-8300, 500, 0), 100.0f }, { FVector(-3800, -950, 0), 30.0f }, { FVector(6300, 1200, 0), 250.0f },
		};
		for (const auto& B : Boards)
		{
			SpawnGen(Board, MetalMid, B.Loc, B.Yaw);
			SpawnGen(Poster, PosterMid, B.Loc, B.Yaw);
		}
		SpawnGen(Vent, MetalMid, FVector(4600, -1200, 400), 15.0f);
		SpawnGen(Vent, MetalMid, FVector(5800, -2300, 400), 190.0f);
		SpawnGen(Vent, MetalMid, FVector(-4300, 3800, 400), 80.0f);
		SpawnGen(Vent, MetalMid, FVector(-3800, 2200, 400), 285.0f);
		SpawnGen(Barricade, OliveMid, FVector(-2900, 220, 0), 100.0f);
		SpawnGen(Barricade, OliveMid, FVector(-5200, -260, 0), 80.0f);
		SpawnGen(Barricade, OliveMid, FVector(3600, -1900, 0), 10.0f);

		// First hand-built structure (Tools/blender/gen_building_kit.py): a
		// worker-row facade in the empty NW zone + gantry portals at the gate.
		// Masonry rides the KitRow worker-teal cel MID (world-aligned albedo
		// needs no UVs) — NOT the Wall_ tint: near-neutral washed the row to
		// chalk white (15.8 look-ronde, cam 2; see the KitRow palette entry).
		// Metalwork rides the metal tint.
		UMaterialInstanceDynamic* MasonryMid = MidForPalette(PaletteForLabel(TEXT("KitRow")));
		UStaticMesh* KWall = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Kit_WallPanel.SM_Kit_WallPanel"));
		UStaticMesh* KWindow = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Kit_WallWindow.SM_Kit_WallWindow"));
		UStaticMesh* KDoor = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Kit_Doorway.SM_Kit_Doorway"));
		UStaticMesh* KPillar = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Kit_CornerPillar.SM_Kit_CornerPillar"));
		UStaticMesh* KTrim = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Kit_RoofTrim.SM_Kit_RoofTrim"));
		UStaticMesh* KChimney = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Kit_Chimney.SM_Kit_Chimney"));
		UStaticMesh* KGantry = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Generated/SM_Kit_GantryBeam.SM_Kit_GantryBeam"));

		// Facade row at (-6600, 6800), facing the plaza (south, yaw 270): the
		// module's +X facade turns to -Y. Modules step 400 units along X.
		{
			const FVector RowBase(-6600, 6800, 0);
			UStaticMesh* RowModules[] = { KDoor, KWindow, KWall, KWindow };
			for (int32 Index = 0; Index < 4; ++Index)
			{
				const FVector At = RowBase + FVector(Index * 400.0f, 0, 0);
				SpawnGen(RowModules[Index], MasonryMid, At, 270.0f);
				SpawnGen(KTrim, MasonryMid, At + FVector(0, 0, 350.0f), 270.0f);
				SpawnGen(KPillar, MasonryMid, At + FVector(-200.0f, 0, 0), 0.0f);
			}
			SpawnGen(KPillar, MasonryMid, RowBase + FVector(1400.0f, 0, 0), 0.0f);
			SpawnGen(KChimney, MasonryMid, RowBase + FVector(600.0f, 220.0f, 0), 0.0f);
			SpawnGen(Vent, MetalMid, RowBase + FVector(1000.0f, 60.0f, 350.0f), 250.0f);
		}
		// Scale audit (headless bounds pass, 2026-07-23): SM_Prop_CableArc
		// measures 422x30x79 with its sag at z 236..315 — the parked "~100x
		// oversized" QC eyeball was stale; the import is a sane 4.2 m catenary
		// hung at pole height. Strung at natural scale between a lamp pair
		// flanking the crossing; the pair's second lamp spawns here so the
		// 420-unit spacing matches the measured span.
		SpawnGen(Lamp, MetalMid, FVector(-4230, -700, 0), 90.0f);
		SpawnGen(LampGlow, GenGlowMid, FVector(-4230, -700, 0), 90.0f);
		SpawnGen(Cable, MetalMid, FVector(-4440, -700, 0), 0.0f);

		// Gate portal at the Entry_Main approach (measured kit: CornerPillar
		// 80x80x370, GantryBeam 50x600x50): two frames across the artery —
		// corner pillars as legs OUTSIDE the y=+-460 lane lines, beams scaled
		// 1.8 along their span so they bridge the 1080-unit gap flush with the
		// pillar outer faces. Metal tint; same no-collision dressing tier.
		for (const float GateX : { -8850.0f, -8550.0f })
		{
			SpawnGen(KPillar, MetalMid, FVector(GateX, -500.0f, 0), 0.0f);
			SpawnGen(KPillar, MetalMid, FVector(GateX, 500.0f, 0), 0.0f);
			SpawnGen(KGantry, MetalMid, FVector(GateX, 0, 370.0f), 0.0f, FVector(1.0f, 1.8f, 1.0f));
		}
	}

	// PLACEHOLDER(15.5/03.3): Kessara skyline massing OUTSIDE the playable
	// perimeter — "silhouetted crane forests" in the amber smog. Pure backdrop:
	// no nav, no cover, no mission space touched; the art pass replaces it with
	// authored kits. Deterministic seed so every machine builds the same city
	// (reproducible-from-code graybox, SPEC-P1-05).
	{
		FRandomStream SkylineRng(503); // 503 AE — the present year (00_INDEX)

		// Industrial plain under the backdrop, one slab: without it the massing
		// floats over sky beyond the district floor's ±10 km edge.
		SpawnBlock(TEXT("Outland"), FVector(0, 0, -80.0f), FVector(620.0f, 620.0f, 1.0f));

		// Mega-blocks: factory hulks ringing the district at 13–26 km.
		for (int32 Index = 0; Index < 56; ++Index)
		{
			const float Angle = SkylineRng.FRandRange(0.0f, 2.0f * UE_PI);
			const float Radius = SkylineRng.FRandRange(13000.0f, 26000.0f);
			const float Height = SkylineRng.FRandRange(9.0f, 34.0f);
			SpawnBlock(TEXT("Skyline"),
				FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Height * 50.0f - 80.0f),
				FVector(SkylineRng.FRandRange(9.0f, 26.0f), SkylineRng.FRandRange(9.0f, 26.0f), Height));

			// Sodium window strips on roughly a third of the hulks: worker light
			// against the graphite (03.3's sodium-orange vs. Dominion white-gold).
			if (SkylineRng.FRand() < 0.35f)
			{
				// 1600 units inward clears the widest hulk half-extent (26*50), so
				// strips sit proud of the facade instead of embedded in it.
				const float StripHeight = SkylineRng.FRandRange(150.0f, Height * 50.0f);
				SpawnBlock(TEXT("Glow"),
					FVector(FMath::Cos(Angle) * (Radius - 1600.0f), FMath::Sin(Angle) * (Radius - 1600.0f), StripHeight),
					FVector(0.4f, SkylineRng.FRandRange(3.0f, 7.0f), 0.45f));
			}
		}

		// Chimney stacks: the forge-world's vertical punctuation.
		for (int32 Index = 0; Index < 18; ++Index)
		{
			const float Angle = SkylineRng.FRandRange(0.0f, 2.0f * UE_PI);
			const float Radius = SkylineRng.FRandRange(12000.0f, 22000.0f);
			const float Height = SkylineRng.FRandRange(26.0f, 44.0f);
			SpawnBlock(TEXT("Skyline"),
				FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Height * 50.0f - 80.0f),
				FVector(SkylineRng.FRandRange(1.2f, 2.0f), SkylineRng.FRandRange(1.2f, 2.0f), Height));
		}

		// Street dressing + checkpoint light strips INSIDE the district: a
		// readable occupation story on the empty plaza (15.5 "occupation &
		// story"), all no-collision deco — nav/cover/missions untouched.
		{
			// East-west artery: Entry_Main to the control-post compound.
			SpawnBlock(TEXT("DecoLine"), FVector(0, 460, 3), FVector(190.0f, 0.16f, 0.06f));
			SpawnBlock(TEXT("DecoLine"), FVector(0, -460, 3), FVector(190.0f, 0.16f, 0.06f));
			for (int32 Index = 0; Index < 24; ++Index)
			{
				SpawnBlock(TEXT("DecoLine"), FVector(-9200.0f + Index * 800.0f, 0, 3), FVector(1.6f, 0.14f, 0.06f));
			}
			// North-south cross street toward the warehouse yard.
			SpawnBlock(TEXT("DecoLine"), FVector(-4460, 0, 3), FVector(0.16f, 190.0f, 0.06f));
			SpawnBlock(TEXT("DecoLine"), FVector(-3540, 0, 3), FVector(0.16f, 190.0f, 0.06f));
			// Oil and rust staining, biased toward the driven crossing.
			FRandomStream DecoRng(77);
			// Per-instance yaw rides its OWN stream (15.8 art-fix): the 14
			// banked placements draw position+scale from DecoRng in a fixed
			// order — new draws on that stream would reshuffle the layout, so
			// the variation stream is separate by contract.
			FRandomStream StainVarRng(770);
			for (int32 Index = 0; Index < 14; ++Index)
			{
				SpawnBlock(TEXT("DecoStain"),
					FVector(DecoRng.FRandRange(-8000.0f, 8000.0f), DecoRng.FRandRange(-7000.0f, 7000.0f), 2.0f),
					FVector(DecoRng.FRandRange(2.0f, 6.5f), DecoRng.FRandRange(2.0f, 6.5f), 0.04f),
					StainVarRng.FRandRange(0.0f, 360.0f));
			}
			// Sodium checkpoint strips on the inner wall faces (03.3: sodium
			// checkpoints vs. Dominion white-gold) — three per wall.
			for (int32 Index = -1; Index <= 1; ++Index)
			{
				const float Along = Index * 6000.0f;
				SpawnBlock(TEXT("Glow"), FVector(Along, 9935, 360), FVector(3.0f, 0.12f, 0.35f));
				SpawnBlock(TEXT("Glow"), FVector(Along, -9935, 360), FVector(3.0f, 0.12f, 0.35f));
				SpawnBlock(TEXT("Glow"), FVector(9935, Along, 360), FVector(0.12f, 3.0f, 0.35f));
				SpawnBlock(TEXT("Glow"), FVector(-9935, Along, 360), FVector(0.12f, 3.0f, 0.35f));
			}
		}

		// Crane gantries: two legs + a long beam, alternating axis — the layered
		// silhouette Kessara's identity hangs on. Axis-aligned is right for a
		// forge world's orthogonal yards.
		for (int32 Index = 0; Index < 12; ++Index)
		{
			const float Angle = SkylineRng.FRandRange(0.0f, 2.0f * UE_PI);
			const float Radius = SkylineRng.FRandRange(11500.0f, 17000.0f);
			const FVector Base(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
			const bool bAlongX = (Index % 2) == 0;
			const float LegHeight = SkylineRng.FRandRange(18.0f, 28.0f);
			const float Span = SkylineRng.FRandRange(1600.0f, 2600.0f);
			const FVector LegOffset = bAlongX ? FVector(Span * 0.5f, 0, 0) : FVector(0, Span * 0.5f, 0);
			SpawnBlock(TEXT("Skyline"), Base + LegOffset + FVector(0, 0, LegHeight * 50.0f - 80.0f), FVector(1.4f, 1.4f, LegHeight));
			SpawnBlock(TEXT("Skyline"), Base - LegOffset + FVector(0, 0, LegHeight * 50.0f - 80.0f), FVector(1.4f, 1.4f, LegHeight));
			SpawnBlock(TEXT("Skyline"), Base + FVector(0, 0, LegHeight * 100.0f - 140.0f),
				bAlongX ? FVector(Span / 100.0f + 6.0f, 1.6f, 1.6f) : FVector(1.6f, Span / 100.0f + 6.0f, 1.6f));
		}
	}

	for (const FPointDef& Site : Sites)
	{
		if (ATargetPoint* Point = World.SpawnActor<ATargetPoint>(FVector(Site.X, Site.Y, 120.0f), FRotator::ZeroRotator, Params))
		{
			Point->Tags.Add(Site.Id);
		}
	}

	for (const FPointDef& Site : TriggerSites)
	{
		if (AEclipseObjectiveTrigger* Trigger = World.SpawnActor<AEclipseObjectiveTrigger>(FVector(Site.X, Site.Y, 120.0f), FRotator::ZeroRotator, Params))
		{
			Trigger->SiteId = Site.Id;
		}
	}

	for (const FPointDef& Entry : EntryPoints)
	{
		if (APlayerStart* Start = World.SpawnActor<APlayerStart>(FVector(Entry.X, Entry.Y, 200.0f), FRotator::ZeroRotator, Params))
		{
			Start->Tags.Add(Entry.Id);
		}
	}

	// PLACEHOLDER(Part 15.3/15.5): the district's mood pass, owner-authorized ahead
	// of Phase 2 — a low warm sun through Kessara smog, punchy grade, ink outlines.
	// Everything below rides standard UE features behind scalability (15.10); the
	// authored art pass on target hardware replaces these numbers wholesale.

	// One authoritative mood: purge the host map's own sun/sky/fog first. Entry
	// ships a horizon-level sun that paints facades warm but leaves every
	// horizontal surface black — with two suns the district's look is luck.
	{
		TArray<AActor*> Stale;
		for (TActorIterator<ADirectionalLight> It(&World); It; ++It) { Stale.Add(*It); }
		for (TActorIterator<ASkyLight> It(&World); It; ++It) { Stale.Add(*It); }
		for (TActorIterator<ASkyAtmosphere> It(&World); It; ++It) { Stale.Add(*It); }
		for (TActorIterator<AExponentialHeightFog> It(&World); It; ++It) { Stale.Add(*It); }
		for (AActor* Actor : Stale)
		{
			Actor->Destroy();
		}
	}

	// Low industrial sun; drives the SkyAtmosphere so the horizon carries the mood.
	// Mid-afternoon sun: warm but high enough that shade sides stay readable —
	// the stylized look wants soft, lifted shadows, not noir silhouettes.
	if (ADirectionalLight* Sun = World.SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), SunRotation, Params))
	{
		if (UDirectionalLightComponent* SunComponent = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			// ADirectionalLight ships static-mobility; a runtime spawn keeps the
			// default horizontal direction unless made movable and re-rotated —
			// which lit facades and left every floor black (pass-8 forensics).
			// SunRotation is shared with the toon material's LightDir (see above).
			SunComponent->SetMobility(EComponentMobility::Movable);
			Sun->SetActorRotation(SunRotation);
			// Legacy intensity 8 pairs with the toon emissive x10 range under auto
			// exposure. Atmosphere re-linked: the old decouple protected LIT ground
			// from transmittance loss (pass-15) — the district is unlit now, so the
			// sun may power a real dusk sky and only lights pawns.
			SunComponent->SetIntensity(8.0f);
			SunComponent->SetLightColor(FLinearColor(1.0f, 0.87f, 0.70f));
			SunComponent->SetAtmosphereSunLight(true);
			SunComponent->SetVolumetricScatteringIntensity(2.0f);
			// SM5 laptop: the CSM path blankets the 200x-scaled ground slab in
			// shadow no matter the caster set (passes 5-16 forensics) — sun ships
			// shadowless there. SM6: VSM shadows return; the unlit district cannot
			// darken (emissive ignores shadowing), but the volumetric smog CAN —
			// buildings now cut real light shafts through the haze (15.5 revision).
			SunComponent->SetCastShadows(bFullFidelity);
			UE_LOG(LogEclipse, Display, TEXT("Graybox: sun direction %s (movable)."), *SunComponent->GetDirection().ToString());
		}
	}

	World.SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, Params);

	// SM6 only: the real captured skylight the fill light stands in for on SM5
	// (the laptop's realtime capture fed horizontal glare + a black zenith,
	// passes 3-14). Lights pawns/props; the unlit district ignores it.
	if (bFullFidelity)
	{
		if (ASkyLight* Sky = World.SpawnActor<ASkyLight>(FVector(0, 0, 400), FRotator::ZeroRotator, Params))
		{
			if (USkyLightComponent* SkyComponent = Sky->GetLightComponent())
			{
				SkyComponent->SetMobility(EComponentMobility::Movable);
				SkyComponent->SetRealTimeCaptureEnabled(true);
				// 2.2: lit PBR character bodies (step-2 pipeline) read at dusk;
				// the unlit district ignores this entirely, so the district
				// grade is untouched (first body-showcase round: near-black).
				SkyComponent->SetIntensity(2.2f);
			}
		}
	}

	// Fill light instead of a captured skylight: on this box's SM5 fallback the
	// realtime sky capture feeds horizontal glare and a black zenith (floors go
	// dark, facades blow out — passes 3-14 forensics). A soft cool shadowless
	// counter-sun gives the flat, readable stylized shade the direction wants;
	// the strong PC swaps this for a real captured skylight + Lumen bounce.
	if (ADirectionalLight* Fill = World.SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-35.0f, 235.0f, 0), Params))
	{
		if (UDirectionalLightComponent* FillComponent = Cast<UDirectionalLightComponent>(Fill->GetLightComponent()))
		{
			FillComponent->SetMobility(EComponentMobility::Movable);
			Fill->SetActorRotation(FRotator(-35.0f, 235.0f, 0));
			FillComponent->SetIntensity(2.5f); // pawn fill in the banked pass-19 units; the unlit district ignores it
			FillComponent->SetLightColor(FLinearColor(0.55f, 0.65f, 0.85f));
			FillComponent->SetCastShadows(false);
			FillComponent->SetAtmosphereSunLight(false);
		}
	}

	// Kessara smog: warm sodium haze with volumetrics so the sun shafts read.
	if (AExponentialHeightFog* Fog = World.SpawnActor<AExponentialHeightFog>(FVector(0, 0, -50), FRotator::ZeroRotator, Params))
	{
		if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
		{
			// Haze, not soup: the district must read across its full 200 m at
			// command distance (15.5); the smog hugs the ground via the falloff.
			FogComponent->SetFogDensity(0.006f);
			FogComponent->SetFogHeightFalloff(0.2f);
			FogComponent->SetFogInscatteringColor(FLinearColor(0.42f, 0.32f, 0.24f));
			// SM5 laptop: volumetric fog receives no sun on the fallback path and
			// extinguishes the whole ground plane to black — plain exponential
			// haze there. SM6: real volumetric smog, so the shadowed sun draws
			// shafts through the crane-and-compound silhouettes (Kessara identity
			// 03.3: amber smog; 15.5 revision: more atmosphere within the style).
			FogComponent->SetVolumetricFog(bFullFidelity);
		}
	}

	// Graphic-novel grade: locked-ish exposure (no graybox swim), saturation punch,
	// gentle vignette, and the authored ink-outline blendable when it exists.
	if (APostProcessVolume* Post = World.SpawnActor<APostProcessVolume>(FVector::ZeroVector, FRotator::ZeroRotator, Params))
	{
		Post->bUnbound = true;
		FPostProcessSettings& Settings = Post->Settings;
		// Default histogram auto-exposure (the banked pass-19 regime — pinned
		// EV100 + physical lux fought three unit systems at once on this SM5 box,
		// passes 20-22 forensics). Local exposure stays neutralized so it cannot
		// re-contrast the flat cel bands.
		// Pull the histogram key down: without it the mostly-bright emissive
		// district washes to pastel (pass-27 forensics) — the dusk look wants
		// saturated mids, not chalk.
		// Color calibration (owner pass, 2026-07-23): Borderlands-punch — open
		// the mids, saturate harder, tip the grade warm. The dusk mood stays;
		// the somber gray-out goes.
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = -0.7f;
		Settings.bOverride_ColorGain = true;
		Settings.ColorGain = FVector4(1.05f, 1.00f, 0.93f, 1.0f);
		Settings.bOverride_LocalExposureHighlightContrastScale = true;
		Settings.LocalExposureHighlightContrastScale = 1.0f;
		Settings.bOverride_LocalExposureShadowContrastScale = true;
		Settings.LocalExposureShadowContrastScale = 1.0f;
		Settings.bOverride_BloomIntensity = true;
		Settings.BloomIntensity = 0.45f; // 15.5 revision: punchier bloom within the graphic-novel look
		// Subtle film grain per the 15.5 fidelity revision — texture, not noise;
		// SSAO is deliberately absent: it is a no-op on the unlit emissive
		// district and returns with the lit-toon migration.
		Settings.bOverride_FilmGrainIntensity = true;
		Settings.FilmGrainIntensity = 0.07f;
		Settings.bOverride_ColorSaturation = true;
		Settings.ColorSaturation = FVector4(1.38f, 1.38f, 1.38f, 1.0f);
		Settings.bOverride_ColorContrast = true;
		Settings.ColorContrast = FVector4(1.06f, 1.06f, 1.06f, 1.0f);
		Settings.bOverride_VignetteIntensity = true;
		Settings.VignetteIntensity = 0.35f;

		// Ink lines: PP_EclipseInk is the Tools/author_outline_material.py build —
		// a verified scene passthrough (zero edges = untouched frame). The old
		// PP_EclipseOutline asset replaced the whole frame with its tint and masked
		// every material change from pass 20-26; it stays on disk unused. Until the
		// script has authored PP_EclipseInk, the district renders without lines.
		if (UMaterialInterface* Outline = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/PP_EclipseInk.PP_EclipseInk")))
		{
			Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, Outline));
		}
		else
		{
			UE_LOG(LogEclipse, Warning, TEXT("Graybox: PP_EclipseInk not authored yet — district renders without ink lines (GDD 14.3.5; run Tools/author_outline_material.py)."));
		}
	}

	UE_LOG(LogEclipse, Display, TEXT("Graybox: district built (%d blocks, %d cover, %d sites, %d entries)."),
		static_cast<int32>(UE_ARRAY_COUNT(Blocks)), static_cast<int32>(UE_ARRAY_COUNT(CoverPoints)),
		static_cast<int32>(UE_ARRAY_COUNT(Sites)), static_cast<int32>(UE_ARRAY_COUNT(EntryPoints)));
}

} // namespace EclipseGraybox
