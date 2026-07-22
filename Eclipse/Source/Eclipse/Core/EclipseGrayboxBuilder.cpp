#include "Core/EclipseGrayboxBuilder.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Eclipse.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInstanceDynamic.h"
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
	struct FPaletteDef { const TCHAR* Prefix; FLinearColor Lit; FLinearColor Shade; };
	const FPaletteDef Palette[] = {
		{ TEXT("Floor"),  FLinearColor(0.165f, 0.150f, 0.160f), FLinearColor(0.055f, 0.052f, 0.080f) },  // asphalt — dark but never crushed
		{ TEXT("Wall_"),  FLinearColor(0.230f, 0.250f, 0.290f), FLinearColor(0.075f, 0.082f, 0.130f) },  // perimeter concrete, cold
		{ TEXT("BldgA"),  FLinearColor(0.560f, 0.160f, 0.085f), FLinearColor(0.200f, 0.045f, 0.085f) },  // Dominion post: oxide red, shade to maroon-purple
		{ TEXT("BldgB"),  FLinearColor(0.060f, 0.300f, 0.310f), FLinearColor(0.020f, 0.100f, 0.150f) },  // warehouse: worker teal, shade to deep sea
		{ TEXT("Cover"),  FLinearColor(0.850f, 0.360f, 0.050f), FLinearColor(0.360f, 0.110f, 0.060f) },  // hazard orange, reads as cover
	};
	const FPaletteDef DefaultPalette = { TEXT(""), FLinearColor(0.35f, 0.35f, 0.38f), FLinearColor(0.12f, 0.12f, 0.16f) };

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
	 * One sun definition shared by the light actor AND the toon material's LightDir
	 * parameter — if these ever diverge, material banding and pawn lighting tell
	 * two different stories about where the sun is.
	 */
	// Low dusk sun: vertical faces split hard into lit/shade (the cel read), the
	// floor stays in the mid band (BandLo < sin 25 deg < BandHi in M_EclipseToon).
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

	// One dynamic instance per palette entry (not per block) keeps the dressing cheap.
	// Toon master first (cel bands computed in-material — deterministic on the SM5
	// fallback where scene lights never reach horizontals); engine shape material
	// as the flat-color fallback when the authored asset is absent (GDD 14.3.5).
	UMaterialInterface* ToonMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToon.M_EclipseToon"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ToonMaterial == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Graybox: M_EclipseToon missing — falling back to flat engine-material dressing (GDD 14.3.5)."));
	}
	TMap<uint32, UMaterialInstanceDynamic*> MidByColor;
	auto MidForPalette = [ToonMaterial, BaseMaterial, &MidByColor, &World](const FPaletteDef& Entry) -> UMaterialInstanceDynamic*
	{
		UMaterialInterface* Master = ToonMaterial != nullptr ? ToonMaterial : BaseMaterial;
		if (Master == nullptr)
		{
			return nullptr; // both materials missing = plain blocks, never a crash (GDD 14.3.5)
		}
		UMaterialInstanceDynamic*& Mid = MidByColor.FindOrAdd(GetTypeHash(Entry.Lit.ToFColor(true)));
		if (Mid == nullptr)
		{
			Mid = UMaterialInstanceDynamic::Create(Master, &World);
			if (ToonMaterial != nullptr)
			{
				Mid->SetVectorParameterValue(TEXT("LitColor"), Entry.Lit);
				Mid->SetVectorParameterValue(TEXT("ShadeColor"), Entry.Shade);
				// The material's L = -LightDir, so pass the travel direction of the sun.
				Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
				Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
			}
			else
			{
				Mid->SetVectorParameterValue(TEXT("Color"), Entry.Lit);
			}
		}
		return Mid;
	};

	auto SpawnBlock = [&World, CubeMesh, &Params, &MidForPalette](const TCHAR* Label, const FVector& Location, const FVector& Scale)
	{
		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
		if (Actor != nullptr)
		{
			Actor->SetMobility(EComponentMobility::Movable);
			Actor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			Actor->SetActorScale3D(Scale);
			Actor->Tags.Add(Label);
			if (UMaterialInstanceDynamic* Mid = MidForPalette(PaletteForLabel(Label)))
			{
				Actor->GetStaticMeshComponent()->SetMaterial(0, Mid);
			}
			// The engine cube's distance field breaks at these non-uniform scales
			// (up to 200:1) and poisons DFAO/DF-shadows into blanket blackness at
			// high scalability. Graybox blocks light via CSM only; the authored
			// district gets proper per-kit distance fields in the art pass.
			Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
			if (FCString::Stricmp(Label, TEXT("Floor")) == 0)
			{
				Actor->GetStaticMeshComponent()->SetCastShadow(false); // nothing is ever under the ground slab
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
		SpawnBlock(TEXT("Cover"), FVector(Cover.X, Cover.Y, 60.0f),
			FVector(bRotated ? 3.0f : 1.0f, bRotated ? 1.0f : 3.0f, 1.2f));
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
			// PLACEHOLDER(15.3, strong PC): this box's SM5 CSM path blankets the
			// 200x-scaled ground slab in shadow no matter the caster set (passes
			// 5-16 forensics). Key light ships shadowless here — the flat two-tone
			// + ink outline IS the stylized look; VSM shadows return on SM6 target.
			SunComponent->SetCastShadows(false);
			UE_LOG(LogEclipse, Display, TEXT("Graybox: sun direction %s (movable)."), *SunComponent->GetDirection().ToString());
		}
	}

	World.SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator, Params);

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
			// PLACEHOLDER(15.3, strong PC): volumetric fog receives no sun on this
			// box's SM5 fallback and extinguishes the whole ground plane to black —
			// plain exponential haze until target hardware (SM6) takes over.
			FogComponent->SetVolumetricFog(false);
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
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = -1.0f;
		Settings.bOverride_LocalExposureHighlightContrastScale = true;
		Settings.LocalExposureHighlightContrastScale = 1.0f;
		Settings.bOverride_LocalExposureShadowContrastScale = true;
		Settings.LocalExposureShadowContrastScale = 1.0f;
		Settings.bOverride_BloomIntensity = true;
		Settings.BloomIntensity = 0.35f;
		Settings.bOverride_ColorSaturation = true;
		Settings.ColorSaturation = FVector4(1.22f, 1.22f, 1.22f, 1.0f);
		Settings.bOverride_ColorContrast = true;
		Settings.ColorContrast = FVector4(1.04f, 1.04f, 1.04f, 1.0f);
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
