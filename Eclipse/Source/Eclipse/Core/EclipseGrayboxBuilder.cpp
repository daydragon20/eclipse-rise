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

	constexpr FPointDef Entries[] = {
		{ TEXT("Entry_Main"), -9000, 0 },
		{ TEXT("Entry_Sewer"), 0, -9000 },
		{ TEXT("Entry_Roof"), 8500, 8500 },
	};

	/**
	 * PLACEHOLDER(Part 15.5): early stylized dressing, owner-authorized ahead of
	 * the Phase 2 art pass. Bold flat block colors (Borderlands-leaning: punchy,
	 * readable at command distance) applied as dynamic instances of the engine
	 * shape material; the real district replaces all of it with authored kits.
	 */
	struct FPaletteDef { const TCHAR* Prefix; FLinearColor Color; };
	const FPaletteDef Palette[] = {
		{ TEXT("Floor"),  FLinearColor(0.120f, 0.110f, 0.120f) },  // asphalt — dark but never crushed
		{ TEXT("Wall_"),  FLinearColor(0.180f, 0.195f, 0.225f) },  // perimeter concrete, cold
		{ TEXT("BldgA"),  FLinearColor(0.420f, 0.130f, 0.075f) },  // Dominion post: oxide red
		{ TEXT("BldgB"),  FLinearColor(0.050f, 0.230f, 0.240f) },  // warehouse: worker teal
		{ TEXT("Cover"),  FLinearColor(0.760f, 0.310f, 0.045f) },  // hazard orange, reads as cover
	};

	FLinearColor ColorForLabel(const TCHAR* Label)
	{
		for (const FPaletteDef& Entry : Palette)
		{
			if (FCString::Strnicmp(Label, Entry.Prefix, FCString::Strlen(Entry.Prefix)) == 0)
			{
				return Entry.Color;
			}
		}
		return FLinearColor(0.35f, 0.35f, 0.38f);
	}
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

	// One dynamic instance per palette color (not per block) keeps the dressing cheap.
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	TMap<uint32, UMaterialInstanceDynamic*> MidByColor;
	auto MidForColor = [BaseMaterial, &MidByColor, &World](const FLinearColor& Color) -> UMaterialInstanceDynamic*
	{
		if (BaseMaterial == nullptr)
		{
			return nullptr; // engine material missing = plain blocks, never a crash (GDD 14.3.5)
		}
		UMaterialInstanceDynamic*& Mid = MidByColor.FindOrAdd(GetTypeHash(Color.ToFColor(true)));
		if (Mid == nullptr)
		{
			Mid = UMaterialInstanceDynamic::Create(BaseMaterial, &World);
			Mid->SetVectorParameterValue(TEXT("Color"), Color);
		}
		return Mid;
	};

	auto SpawnBlock = [&World, CubeMesh, &Params, &MidForColor](const TCHAR* Label, const FVector& Location, const FVector& Scale)
	{
		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, Params);
		if (Actor != nullptr)
		{
			Actor->SetMobility(EComponentMobility::Movable);
			Actor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
			Actor->SetActorScale3D(Scale);
			Actor->Tags.Add(Label);
			if (UMaterialInstanceDynamic* Mid = MidForColor(ColorForLabel(Label)))
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

	for (const FPointDef& Entry : Entries)
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
	if (ADirectionalLight* Sun = World.SpawnActor<ADirectionalLight>(FVector(0, 0, 5000), FRotator(-42.0f, 55.0f, 0), Params))
	{
		if (UDirectionalLightComponent* SunComponent = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
		{
			// ADirectionalLight ships static-mobility; a runtime spawn keeps the
			// default horizontal direction unless made movable and re-rotated —
			// which lit facades and left every floor black (pass-8 forensics).
			SunComponent->SetMobility(EComponentMobility::Movable);
			Sun->SetActorRotation(FRotator(-42.0f, 55.0f, 0));
			SunComponent->SetIntensity(8.0f);
			SunComponent->SetLightColor(FLinearColor(1.0f, 0.87f, 0.70f));
			// Not the atmosphere's sun: with legacy-unit intensities the atmosphere
			// transmittance eats the ground contribution (pass-15 forensics) — the
			// sky keeps its own gradient, the district keeps predictable light.
			SunComponent->SetAtmosphereSunLight(false);
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
			FillComponent->SetIntensity(2.5f);
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
		// Physical sun + atmosphere: default histogram auto-exposure handles the
		// range correctly — clamping it in legacy units blew the frame to white
		// the moment the light became physical (pass-11 forensics).
		Settings.bOverride_BloomIntensity = true;
		Settings.BloomIntensity = 0.35f;
		Settings.bOverride_ColorSaturation = true;
		Settings.ColorSaturation = FVector4(1.22f, 1.22f, 1.22f, 1.0f);
		Settings.bOverride_ColorContrast = true;
		Settings.ColorContrast = FVector4(1.04f, 1.04f, 1.04f, 1.0f);
		Settings.bOverride_VignetteIntensity = true;
		Settings.VignetteIntensity = 0.35f;

		if (UMaterialInterface* Outline = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/PP_EclipseOutline.PP_EclipseOutline")))
		{
			Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, Outline));
		}
		else
		{
			UE_LOG(LogEclipse, Warning, TEXT("Graybox: PP_EclipseOutline missing — district renders without ink lines (GDD 14.3.5)."));
		}
	}

	UE_LOG(LogEclipse, Display, TEXT("Graybox: district built (%d blocks, %d cover, %d sites, %d entries)."),
		static_cast<int32>(UE_ARRAY_COUNT(Blocks)), static_cast<int32>(UE_ARRAY_COUNT(CoverPoints)),
		static_cast<int32>(UE_ARRAY_COUNT(Sites)), static_cast<int32>(UE_ARRAY_COUNT(Entries)));
}

} // namespace EclipseGraybox
