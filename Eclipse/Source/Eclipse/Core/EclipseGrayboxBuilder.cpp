#include "Core/EclipseGrayboxBuilder.h"

#include "Components/AudioComponent.h"
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
#include "Components/BoxComponent.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Quests/EclipseObjectiveTrigger.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundBase.h"
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
		// Dressing-iteratie 2, step 1 (DRESSING_ITERATIE_2.md; art-review
		// 00056-00062): the old warm-neutral tint (0.165/0.150/0.160) read as
		// full-daylight concrete under the dusk sky — the root of the
		// "stickers" verdict, and the biggest surface in every frame. Dusk fix:
		// the floor joins the cool perimeter family at exactly one value step
		// under Wall_ (x0.72 — the same step the CoverB hierarchy banked), so
		// asphalt sits below the walls in value and in the same dusk hue lane.
		// Measured gains untouched (12.41/16.8 = 1/linear-mean; the toon HLSL
		// clamps the per-pixel multiplier at 2.5) — the mean multiplier stays
		// 1.0, so this retint moves the histogram only by the tint delta.
		// Dressing-iteratie 3, step 2: two value steps down (x0.518 = 0.72^2) from
		// the iteration-2 tint. Iteration 2 anchored the floor to Wall_ and so made
		// it LIGHTER; the anchor is the SKY, because that is what dusk means. Mid
		// 0.1198 -> 0.0621, which lands ~1.9x the measured horizon sky (0.0595 lin)
		// once step 1 pins the key. Wall_ stays put, so the ground/wall value step
		// finally exists on screen (the review measured band and floor at an
		// identical 0.1237 in the overview frame). Measured gains untouched.
		// Dressing-iteratie 3, de blokkerende stijlovertreding. De art-review waardeerde
		// de vloer op naar BLOKKEREND onder de EEN-STIJL-WET: hij is rauw fotorealisme
		// in een cel/ink-frame en hij is 60-80% van elk frame. Gemeten op shot 00209,
		// drie schone asfaltvakken: sd/gem 29,0% / 29,0% / 30,1% eigen korrel. Dat is
		// niet alleen een stijlbreuk maar ook de reden dat contactschaduwen er niet in
		// leesbaar zijn: een blob van 0,62x is een dip van 38% op een oppervlak dat
		// zelf 30% ruist, oftewel een 1-sigma-signaal.
		// De textuur levert via de toon-master alleen LUMINANTIE, dus de mix is precies
		// de knop die bepaalt hoeveel fotografische korrel er doorkomt. Doel uit de
		// review: sd/gem onder ~12%, de waarde die het overzichtsframe al haalt.
		// 0.50 -> 0.20 omdat de korrel lineair met de mix hoort te schalen (0.12/0.60),
		// maar dat is een voorspelling: de mapping loopt door de tonemapper, dus dit
		// wordt gemeten en zo nodig gebisecteerd - niet uitgerekend en geloofd.
		// De vloer draait nu op een HAND-GEAUTHORDE tegel in plaats van een 4K-foto.
		// Dat was de blokkerende stijlovertreding: rauw fotorealisme in een cel/ink-
		// frame, op 60-80% van elk beeld. Gemeten eigen korrel van de foto ~60%, van
		// de tegel 11,2% (Tools/generate_toon_asphalt.py meet dat op zijn eigen
		// uitvoer). De tegel is laagfrequent, gekwantiseerd in vijf waardestappen en
		// heeft vertakkende scheuren waar de inkpass op kan reageren.
		// Gain 2.56 = 1/gemeten-lineair-gemiddelde, zoals elke andere gain hier.
		// En de MIX gaat terug van 0.20 naar 0.50: die 0.20 was een noodgreep om een
		// foto te dempen, en met een getekende tegel is dempen niet meer nodig - juist
		// omgekeerd, de vlakken en scheuren moeten LEZEN. Voorspelde framevariantie
		// ~6%; dat wordt gemeten, niet geloofd.
		// De Megascans-foto blijft als FALLBACK staan: valt de tegel weg, dan degradeert
		// het district naar de oude look in plaats van naar flat cel (14.3.5).
		// WAARDE-CORRECTIE na de tegelwissel, als eigen stap zodat er maar één ding
		// tegelijk beweegt. De tegel bracht de korrel naar 8,6% (doel gehaald) maar
		// de vloer zakte van 0.0296 naar 0.0217 - 27% donkerder, en daarmee onder het
		// niveau waarop stap 2 hem bewust verankerde. De gain 2.56 is correct gemeten
		// op de tegel; dat het frame toch zakt is dezelfde authored->frame
		// niet-lineariteit die deze iteratie al drie keer liet zien.
		// x1.30 op lit en shade is de eerste bisectiestap terug naar 0.0296. Bewust
		// gebisecteerd en niet uitgerekend, want die mapping is precies wat hier
		// onbetrouwbaar is.
		{ TEXT("Floor"),  FLinearColor(0.112f, 0.121f, 0.140f), FLinearColor(0.036f, 0.040f, 0.064f), TEXT("/Game/Art/Textures/T_toon_asphalt_diff.T_toon_asphalt_diff"), 700.0f, 2.56f, 0.50f, TEXT("/Game/Fab/Megascans/Surfaces/Asphalt_Surface_rmqlqkp0/High/rmqlqkp0_tier_1/Textures/T_rmqlqkp0_4K_B.T_rmqlqkp0_4K_B"), 12.41f },  // dusk asphalt — anchored under the SKY, never crushed
		// Dressing-iteratie 3, step 7 (nu prioriteit 1) — THE VALUE CEILING.
		// The metering pin of step 1 only moved the FLOOR: everything above ~0.25
		// lum sits on the tonemapper shoulder, so it returned just 20-45% of the
		// expected drop. Measured across the same boxes: floor /4.21 but barrier
		// /1.11, so barrier-over-floor went from 3.72x to 14.1x — the ceiling did
		// not come down with the floor, and dressing now stands FURTHER from its
		// ground than before the fix. The bottom half of the frame is bimodal with
		// an empty midband (mid-tones 66.5% -> 9.3% in cam 7, 2.0% in cam 4), and
		// an empty midband IS "stickers on a plate", numerically: no object shares
		// a value with the ground it stands on.
		// Wall_ is the biggest single offender by area — 4.1-4.8x the floor as an
		// unbroken band across every frame, right on the horizon. Iteration 2's
		// "Wall_ stays put" was the wrong call in hindsight.
		// Rule being banked: nothing that is not a light source may exceed the pool
		// core in frame. Method per the review: authored->frame is NON-linear
		// (~2.5x at this level, ~0.8x at floor level), so do NOT compute a factor —
		// start at authored x0.35 and bisect with Tools/measure_frame_values.py
		// against a frame target of <=0.15 lum in box (470,640,600,700).
		// BISECTED on measurement, not computed: x0.35 (the barrier's starting
		// point) overshot HERE. A located vertical scan of cam 5 — found by
		// scanning for the brightest row instead of guessing a box, the mistake
		// that cost two reverts earlier — put the band at 0.0942 before and 0.0245
		// after, i.e. /3.8, which left the wall DARKER than its own ground (0.67x
		// the floor). That is the inverted failure. One value step (x0.72) instead
		// lands ~1.4x the floor: a gentle, unmistakable step ABOVE the ground,
		// which is what "value separation" was asking for. Wall_ carries a huge
		// area, so it sets the frame's whole midband — it belongs just above the
		// floor, nowhere near the pool.
		{ TEXT("Wall_"),  FLinearColor(0.166f, 0.180f, 0.209f), FLinearColor(0.054f, 0.059f, 0.094f), TEXT("/Game/Art/Textures/T_concrete_block_wall_diff.T_concrete_block_wall_diff"), 500.0f, 20.5f, 0.5f },  // perimeter concrete, cold
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
		// Dressing-iteratie 3 — the teal DESATURATED, not re-hued, and that split is
		// the whole point. Two sources disagreed: this palette calls it "worker
		// teal" (deliberate Kessara colour), the second art-review called it Shroud
		// neon-cyan and flagged "one planet per frame" as blocking. Both are right
		// about different things — the HUE belongs to Kessara, the SATURATION does
		// not. Measured 0.81 (R at 20% of G/B), which is neon territory; raising R
		// to 0.150 drops it to ~0.50 and the warehouse reads as weathered painted
		// steel instead of a lightstrip. Value is deliberately left alone: it sits
		// under the ceiling rule already, so this changes colour only.
		// Cyan, bisection step 1 (owner call 2026-07-25 "doe het voorstel"). The
		// art-review's proposal, taken as-is: do NOT lower ColorSaturation 1.38 —
		// that is the owner's Borderlands punch and it would desaturate ten other
		// families to fix one — but push THIS family far up in red and re-measure.
		// Authored 0.150/0.290/0.300 is saturation 0.50 and still renders at frame
		// saturation 0.82 mean / 1.00 worst over 101 075 px (4.87% of cam 3), so
		// authored saturation does not translate 1:1 any more than authored value
		// did. Hence bisect, do not compute: red to the midpoint of its own range
		// (0.150 -> 0.225 lit, 0.050 -> 0.095 shade) and measure what comes out.
		// Target: frame saturation <= 0.55 for the family.
		// Cyaan, owner-keuze A (2026-07-25): de WAARDE omhoog in plaats van de grade
		// omlaag. Onderbouwing uit par. 1j: de grade werkt om de luminantie heen en
		// versterkt verzadiging het hardst bij LAGE waarden - geauthord staat deze
		// wand op verzadiging 0.27 en hij meet 0.99 in frame. Hem lichter maken haalt
		// hem uit die versterkingszone; nog een rood-stap doet dat niet.
		// RUIMTE GEMETEN VOORDAT ER IETS BEWOOG, want dit botst met het waardeplafond
		// uit stap 7 (niets niet-emissief boven de lichtplek-kern): de wand meet in
		// shot 00226 lum 0.0529 tegen een plafond van ~0.115. Ruim twee keer ruimte,
		// dus x1.5 op lit en shade past er comfortabel onder.
		// Bewust GEBISECTEERD en niet uitgerekend: authored -> frame is in dit project
		// herhaaldelijk niet-lineair gemeten. x1.5 is de eerste stap; als de
		// verzadiging nog boven 0.55 meet gaat er nog een stap bij, en de bovengrens
		// is het plafond, niet mijn geduld.
		// STENCIL-UITSLAG (magenta-proefronde, shot 00247): BldgB schildert 93 408
		// pixels in cam 3, maar slechts 1 490 van de 31 500 in het meetvak
		// (900-1250, 470-560) dat drie ronden lang als "de BldgB-wand" gold. Dat vak
		// is dus voor 95% een ANDER oppervlak, en dat verklaart alles: rood omhoog
		// deed niets, de Graphite-regel deed niets, en de waarde x1.5 gaf +5,7%
		// omdat ik telkens de verkeerde ingang draaide voor het vlak dat ik mat.
		// De cyaan-vraag is hiermee NIET opgelost maar wel eerlijk teruggebracht tot
		// een gerichte vervolgstap: dezelfde stencil per kandidaat-ingang draaien tot
		// het frame aanwijst welke dat oppervlak schildert. Drie keer raden heeft
		// verloren van een keer meten.
		{ TEXT("BldgB"),  FLinearColor(0.338f, 0.435f, 0.450f), FLinearColor(0.143f, 0.147f, 0.210f), TEXT("/Game/Art/Textures/T_CorrugatedSteel007A_diff.T_CorrugatedSteel007A_diff"), 300.0f, 2.72f, 0.45f },  // warehouse: weathered worker teal over rusty corrugated sheet (ambientCG 007A, mean .367)
		// Yellow value hierarchy (15.8 look-ronde, cam 3): every yellow element in
		// the plaza midfield sat on the same value (screen 255/211/0 slabs next to
		// 245/216/1 lane paint) and the field read as one flat sheet of yellow.
		// One value step between the families now: Cover barriers (full hazard,
		// the primary cover read) > CoverB worn barriers (x0.72, alternating
		// blocks) > DecoLine route paint (saturated worn yellow, mid-band ~x0.55
		// of the barrier read). Hue stays in the hazard-amber family (15.5).
		// CoverB MUST precede Cover: PaletteForLabel prefix-matches in order.
		// Dressing-iteratie 2, step 4 (graybox-restanten door de toon-master):
		// the flat hazard slabs read as raw graybox boxes next to the dressed
		// props (art-review: "geel barrier-blok", checkpoint frame). The whole
		// yellow family now carries the worn hazard-paint grain, world-aligned
		// (UVMode 0, these are scaled engine cubes) at the MEASURED gain 3.08
		// (worn-stripe map mean-lin .325, SOURCES.md) — mean multiplier 1.0,
		// so the banked Cover > CoverB > DecoLine value hierarchy is untouched;
		// mix stays low so the hazard hue keeps its palette authority (15.5).
		{ TEXT("CoverB"), FLinearColor(0.612f, 0.259f, 0.036f), FLinearColor(0.259f, 0.079f, 0.043f), TEXT("/Game/Art/Decals/T_decal_hazard_diff.T_decal_hazard_diff"), 450.0f, 3.08f, 0.35f },  // worn/older cover barriers, one value step down
		{ TEXT("Cover"),  FLinearColor(0.850f, 0.360f, 0.050f), FLinearColor(0.360f, 0.110f, 0.060f), TEXT("/Game/Art/Decals/T_decal_hazard_diff.T_decal_hazard_diff"), 450.0f, 3.08f, 0.35f },  // hazard orange, reads as cover
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
		{ TEXT("DecoPlaza"), FLinearColor(0.300f, 0.325f, 0.377f), FLinearColor(0.098f, 0.107f, 0.169f), TEXT("/Game/Art/Imported/Textures/T_4k_SciFi10_1_BaseColor.T_4k_SciFi10_1_BaseColor"), 200.0f, 4.96f, 0.7f },
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
	 * The district's sodium lamp posts — ONE list consumed twice (dressing-
	 * iteratie 2): the generated-prop pass spawns the pole + glow head here,
	 * and the luminance-decal pass drops the warm lamp-pool disc under each
	 * entry. The district is unlit (the iteratie-2 architecture anchor), so a
	 * "light pool" can only exist as a decal — a diverging second position
	 * list would strand pools without lamps. Yaw = the arm direction: the
	 * Blender lamp (gen_street_props.py) hangs its head ~0.6-0.9 m along
	 * local +X, so pools offset ~80 units along the yaw vector to sit under
	 * the head, not the pole base. Includes the crossing cable-arc pair
	 * (-4650/-4230): the pair's 420-unit spacing matches the measured
	 * catenary span (headless bounds pass 2026-07-23).
	 *
	 * PoolSize = the pool disc's diameter in units. 700 is the street default:
	 * one asphalt tile period (Floor TexWorldScale 700), so the pool's borrowed
	 * grain continues the floor's own phase seamlessly. The yard lamp at
	 * (-4000, 2100) stands 50 units off BldgB's south slab and hangs its head
	 * toward it, so a 700-disc would print warm light on the warehouse's INSIDE
	 * floor — a leak through an opaque wall. 300 keeps its coverage on the yard
	 * side (the fringe that crosses the slab is under 10% and lands inside the
	 * wall volume itself).
	 */
	struct FLampPostDef { float X; float Y; float Yaw; float PoolSize; };
	constexpr FLampPostDef LampPosts[] = {
		{ -4650.0f, -700.0f, 90.0f, 700.0f }, { -1250.0f, 700.0f, 270.0f, 700.0f }, { 2150.0f, -700.0f, 90.0f, 700.0f },
		{ 4150.0f, -1500.0f, 200.0f, 700.0f }, { -4000.0f, 2100.0f, 20.0f, 300.0f },
		{ -4230.0f, -700.0f, 90.0f, 700.0f },  // crossing pair, second pole (cable spans to -4440)
	};

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

	/**
	 * Layer-1 ambient bed volume (GDD 16.7): the never-silent floor sits low so
	 * barks, dialogue, and the 16.12 stingers always ride on top. Debug-tier
	 * named constant by choice — the real mix (with ducking) moves into the
	 * coming 16.7 audio-layer DataAsset; a one-value DA now would be ceremony
	 * (14.2 governs balance data, and a placeholder mix level is not balance yet).
	 */
	constexpr float AmbientBedVolume = 0.35f;
	const TCHAR* AmbientBedCuePath = TEXT("/Game/Audio/SFX/Cue_SFX_Amb_Kessara_Industrial_Loop_01.Cue_SFX_Amb_Kessara_Industrial_Loop_01");
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

	// NAVIGATIEGRENZEN. Zonder deze volume bestaat er GEEN navmesh, en dan faalt
	// elke MoveToLocation — de squad weigert dus elke verplaatsingsorder met
	// "no route" en staat de hele missie stil. Gemeten door de speelronde
	// (2026-07-25): navigatiesysteem aanwezig, 1 nav-data-actor, en nav-bounds 0.
	//
	// De config zei het omgekeerde. bGenerateNavigationOnlyAroundNavigationInvokers
	// stond aan met het commentaar "no authored bounds volume needed", en dat is
	// een misverstand: invoker-modus bepaalt WELKE tegels binnen de grenzen
	// gebouwd worden, hij vervangt de grenzen niet. Nul grenzen is nul navmesh,
	// invokers of niet — en dat was ook na een synchrone Build() nog zo.
	//
	// Het district bouwt zichzelf uit code (SPEC-P1-05), dus de grens hoort daar
	// ook vandaan te komen. De brush van ANavMeshBoundsVolume is een kubus van
	// 200 uu, dus schaal = gewenste halve maat / 100. Movable, want een statieke
	// brush is na registratie niet meer te schalen.
	if (ANavMeshBoundsVolume* NavBounds = World.SpawnActor<ANavMeshBoundsVolume>(FVector(-2000.0f, -2500.0f, 0.0f), FRotator::ZeroRotator, Params))
	{
		if (USceneComponent* Root = NavBounds->GetRootComponent())
		{
			Root->SetMobility(EComponentMobility::Movable);
		}
		// EEN BOX-COMPONENT en niet de actor-schaal, en dat is het verschil tussen
		// werken en niet werken. Het navigatiesysteem leest de grens uit
		// GetComponentsBoundingBox van de volume, en de brush van een
		// ANavMeshBoundsVolume wordt door de brush-BUILDER gevuld — die draait
		// alleen in de editor. Een runtime-gespawnde volume heeft dus geen
		// geometrie, en schalen schaalt niets: de grens registreert zich netjes en
		// is 0 x 0 x 0 uu groot. Gemeten in een echte -game-run, en het verklaart
		// waarom er wél "1 grens" stond en tóch nooit een tegel gebouwd werd.
		//
		// Een gewone box-component telt wél mee in die bounding box, kost niets en
		// heeft geen editor-code nodig. Geen collision: dit ding mag niets raken,
		// het bakent alleen af.
		UBoxComponent* Extent = NewObject<UBoxComponent>(NavBounds);
		Extent->SetupAttachment(NavBounds->GetRootComponent());
		Extent->SetBoxExtent(FVector(14000.0f, 14000.0f, 2000.0f), /*bUpdateOverlaps*/ false);
		Extent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Extent->RegisterComponent();
		if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(&World))
		{
			// Expliciet aanmelden: een runtime-gespawnde volume komt niet vanzelf
			// in de grenzenlijst zoals een in de map geplaatste dat wel doet.
			Nav->OnNavigationBoundsUpdated(NavBounds);
		}
		else
		{
			UE_LOG(LogEclipse, Warning, TEXT("Graybox: geen navigatiesysteem — de squad kan nergens heen (GDD 14.3.5)."));
		}
	}

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
	// OPPERVLAKTETYPES (26-07 avond). Deze twee zeggen niets over hoe iets ERUIT
	// ziet — dat doet de toon-master — maar waar je OP staat. De voetstapcode
	// leest ze af met een streep omlaag; zonder deze regels traceert hij naar een
	// vloer die niets over zichzelf te melden heeft.
	UPhysicalMaterial* MetalSurface = LoadObject<UPhysicalMaterial>(nullptr, TEXT("/Game/Art/Physics/PM_Metal.PM_Metal"));
	UPhysicalMaterial* ConcreteSurface = LoadObject<UPhysicalMaterial>(nullptr, TEXT("/Game/Art/Physics/PM_Concrete.PM_Concrete"));
	if (MetalSurface == nullptr || ConcreteSurface == nullptr)
	{
		UE_LOG(LogEclipse, Warning,
			TEXT("Graybox: physical materials ontbreken (metaal %s, beton %s) — voetstappen vallen terug op de straatbank (14.3.5)."),
			MetalSurface != nullptr ? TEXT("ok") : TEXT("MISSING"),
			ConcreteSurface != nullptr ? TEXT("ok") : TEXT("MISSING"));
	}

	UMaterialInterface* ToonDecalMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToonDecal.M_EclipseToonDecal"));
	// Shadow-variant van dezelfde master, BLEND_Modulate: contactverdonkering moet
	// de grond VERMENIGVULDIGEN in plaats van hem te bedekken (par. 1h/1i).
	UMaterialInterface* ToonShadowMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToonDecalShadow.M_EclipseToonDecalShadow"));
	UTexture* StainMask = LoadObject<UTexture>(nullptr, TEXT("/Game/Art/Decals/T_stain_mask.T_stain_mask"));
	// Dressing-iteratie 2: a missing STAIN mask no longer disqualifies the decal
	// MASTER — the light pass below rides that same master with its own pool/blob
	// masks, so the two cues degrade independently (14.3.5). The stain path still
	// needs both, which MidForPalette re-checks per instance.
	if (ToonDecalMaterial == nullptr || StainMask == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("Graybox: stain decal path incomplete (material %s, mask %s) — stains fall back to hard-edged opaque cel (run Tools/author_toon_material.py + generate_decals.py/import_generated_decals.py)."),
			ToonDecalMaterial != nullptr ? TEXT("ok") : TEXT("MISSING"), StainMask != nullptr ? TEXT("ok") : TEXT("MISSING"));
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
		// every mode, like the glow strips below. Both halves required: without
		// the baked mask the translucent master would render a hard quad.
		const bool bStainDecal = ToonDecalMaterial != nullptr && StainMask != nullptr && FCString::Stricmp(Entry.Prefix, TEXT("DecoStain")) == 0;
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

	// Welk oppervlak hoort bij welk label. Metaal waar het district metaal IS en
	// waar je er ook OP kunt staan; beton op de vloer en de vlakte eromheen.
	//
	// Ik had hier eerst ook DecoPlaza (de dekplaat van het plein) en "Crate"
	// staan. Allebei fout, en om verschillende redenen die het onthouden waard
	// zijn: Deco* krijgt met opzet geen botsing (dressing mag nav, dekking en
	// hitscan niet verstoren), dus daar loopt niemand op — en "Crate" bestaat
	// helemaal niet in dit district. Een oppervlak toewijzen aan iets waar je niet
	// op kunt staan is precies zo dood als een asset dat niemand laadt.
	//
	// Modder, gras, zand en hout hebben cues én een oppervlaktetype, maar nog geen
	// plek in Kessara waar ze liggen. Een vloer taggen als modder omdat het kan
	// zou een leugen zijn die je HOORT.
	auto SurfaceForLabel = [MetalSurface, ConcreteSurface](const TCHAR* Label) -> UPhysicalMaterial*
	{
		// De dekkingsblokken: hazard-oranje industriële barriers, 120 cm hoog. Dat
		// is het enige metaal in het district waar een speler bovenop kan komen.
		if (FCString::Strnicmp(Label, TEXT("Cover"), 5) == 0)
		{
			return MetalSurface;
		}
		if (FCString::Stricmp(Label, TEXT("Floor")) == 0 ||
			FCString::Strnicmp(Label, TEXT("Outland"), 7) == 0)
		{
			return ConcreteSurface;
		}
		return nullptr; // muren en dressing: daar loopt niemand op
	};

	auto SpawnBlock = [&World, CubeMesh, &Params, &MidForPalette, &SurfaceForLabel](const TCHAR* Label, const FVector& Location, const FVector& Scale, float YawDeg = 0.0f)
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
			if (UPhysicalMaterial* Surface = SurfaceForLabel(Label))
			{
				// Als OVERRIDE op de component en niet op het materiaal: de toon-master
				// is één materiaal voor het hele district, dus daar een oppervlak in
				// zetten zou betekenen dat alles hetzelfde klinkt. Het oppervlak hoort
				// bij het BLOK, niet bij zijn kleur.
				Actor->GetStaticMeshComponent()->SetPhysMaterialOverride(Surface);
			}
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

	// PLACEHOLDER(15.8): THE LIGHT PASS — dressing-iteratie 2's answer to the
	// art-review wish "light pools under the lamp posts + contact shadows"
	// (phase0/DRESSING_ITERATIE_2.md, architecture anchor). The district renders
	// UNLIT: M_EclipseToon computes its bands in-shader from LightDir, so real
	// point lights would light nothing here and real shadow maps never reach the
	// horizontals — adding either would cost VSM pages for no pixels (12.4).
	// Both cues therefore ride the translucent toon-decal master the ground
	// stains proved, with a baked radial falloff so a disc fades out organically
	// instead of reading as a "carpet tile" (review shots 00008-00013):
	//   * lamp pools — warm sodium discs under every LampPosts[] entry (the SAME
	//     list the prop pass spawns its poles from, so a pool can never drift
	//     away from its lamp), offset along the arm direction;
	//   * blob shadows — the floor tint at x0.4 under each dressing mass,
	//     spawned from that mass's OWN coordinates further down.
	// Dressing tier throughout: no collision, no shadow casting, no per-tick
	// work, one MID per cue, deterministic on every machine.
	UMaterialInstanceDynamic* PoolMid = nullptr;
	UMaterialInstanceDynamic* BlobMid = nullptr;
	// Ground-decal Z stack, chosen against the dressing already on the asphalt
	// (stains 0..4, lane paint 0..6, hazard pads 2..6): blobs 3..7 (centre 5),
	// pools 6..10 (centre 8), both 4 units thick. Both must sit OVER the paint —
	// a shadow the lane stripes shine through, or a pool that stops at them,
	// reads as a sticker — and both stay as low as that allows, because every
	// ground plane in this district cuts a faint band across a figure standing on
	// it (already true of the 6-unit lane paint; 10 is the ceiling here).
	constexpr float BlobDecalZ = 5.0f;
	constexpr float PoolDecalZ = 8.0f;
	{
		UTexture* PoolMask = LoadObject<UTexture>(nullptr, TEXT("/Game/Art/Decals/T_pool_mask.T_pool_mask"));
		UTexture* BlobMask = LoadObject<UTexture>(nullptr, TEXT("/Game/Art/Decals/T_blob_mask.T_blob_mask"));
		if (BlobMask == nullptr)
		{
			// Existing art before new art (and 14.3.5): the stain falloff is the
			// same radial/noise disc one generator round older — a serviceable
			// blob until the new mask is imported.
			UE_LOG(LogEclipse, Warning, TEXT("Graybox: blob-shadow mask T_blob_mask missing — falling back to the stain falloff mask (run Tools/generate_decals.py + import_generated_decals.py)."));
			BlobMask = StainMask;
		}
		// Loud per piece (14.3.5): a missing master kills both cues, a missing
		// mask only its own — the district then keeps its flat unlit ground read
		// instead of crashing or half-lighting in silence.
		if (ToonDecalMaterial == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Graybox: light pass skipped — M_EclipseToonDecal missing, so lamp pools and blob shadows cannot render (run Tools/author_toon_material.py)."));
		}
		else if (PoolMask == nullptr || BlobMask == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Graybox: light pass partial (pool mask %s, blob mask %s) — the missing cue is skipped (run Tools/generate_decals.py + import_generated_decals.py)."),
				PoolMask != nullptr ? TEXT("ok") : TEXT("MISSING"),
				BlobMask != nullptr ? TEXT("ok") : TEXT("MISSING"));
		}

		// One MID per cue (never per instance, 12.4). GrainSource opts a cue into
		// a palette entry's MEASURED albedo — the pool borrows the floor's own
		// asphalt so a light pool reads as lit asphalt instead of a painted disc,
		// with the same primary/fallback/flat degradation chain as the blocks.
		//
		// MixOverride exists because that borrowing was doing two jobs at once. The
		// pool wants the floor's TEXTURE (same asphalt, same tile phase); it does
		// not necessarily want the floor's MIX. When the floor's mix dropped from
		// 0.50 to 0.20 to kill its photographic grain (par. 1i), the pools silently
		// flattened with it — one knob, two intentions, and the second one moved by
		// accident. Negative = follow the palette entry, as before.
		auto MakeGroundDecalMid = [ToonDecalMaterial, ToonShadowMaterial, &World](const FLinearColor& Lit, const FLinearColor& Shade, UTexture* Mask, float OpacityScale, const FPaletteDef* GrainSource, float MixOverride = -1.0f, UMaterialInterface* Master = nullptr) -> UMaterialInstanceDynamic*
		{
			UMaterialInterface* Parent = Master != nullptr ? Master : ToonDecalMaterial;
			if (Parent == nullptr || Mask == nullptr)
			{
				return nullptr;
			}
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Parent, &World);
			Mid->SetVectorParameterValue(TEXT("LitColor"), Lit);
			Mid->SetVectorParameterValue(TEXT("ShadeColor"), Shade);
			Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
			Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
			// Mask rides MeshUV (an engine-cube face spans 0..1); the grain below
			// stays world-aligned (UVMode 0) so it keeps the floor's tile phase.
			Mid->SetTextureParameterValue(TEXT("MaskTex"), Mask);
			Mid->SetScalarParameterValue(TEXT("OpacityScale"), OpacityScale);
			if (GrainSource != nullptr && GrainSource->TexPath != nullptr)
			{
				UTexture* Albedo = LoadObject<UTexture>(nullptr, GrainSource->TexPath);
				float Gain = GrainSource->TexGain;
				if (Albedo == nullptr && GrainSource->FallbackTexPath != nullptr)
				{
					Albedo = LoadObject<UTexture>(nullptr, GrainSource->FallbackTexPath);
					Gain = GrainSource->FallbackTexGain;
				}
				if (Albedo != nullptr)
				{
					// Measured gain straight off the palette entry — the light
					// pass never invents a number (15.5).
					Mid->SetTextureParameterValue(TEXT("AlbedoTex"), Albedo);
					Mid->SetScalarParameterValue(TEXT("TexWorldScale"), GrainSource->TexScale);
					Mid->SetScalarParameterValue(TEXT("AlbedoGain"), Gain);
					Mid->SetScalarParameterValue(TEXT("AlbedoMix"),
						MixOverride >= 0.0f ? MixOverride : GrainSource->TexMix);
				}
				else
				{
					UE_LOG(LogEclipse, Warning, TEXT("Graybox: light-pass grain %s missing — pool discs run flat cel (14.3.5)."), GrainSource->TexPath);
				}
			}
			return Mid;
		};

		// Sodium pool tint. Hue = the palette's Glow entry (2.2/1.0/0.3), the
		// district's sodium authority and literally what the lamp heads emit, so
		// the pool introduces no new colour (15.5: palette keeps hue authority).
		// Luminance is DERIVED, not eyeballed — three banked value steps (x0.72
		// each, the step the floor/CoverB hierarchy uses) over the dusk asphalt:
		//   floor mid   = (Lit+Shade)/2 -> Rec709 luminance 0.1198
		//   pool mid    = 0.1198 / 0.72^3            = 0.3209
		//   pool lit    = pool mid / ((1+0.338)/2)   = 0.4797   (0.338 = the
		//                 floor entry's own shade/lit luminance ratio, so a pool
		//                 stays a floor-family tone in both bands)
		//   on the Glow hue (luminance 1.2046) -> x0.398
		// A flat pool renders its MID band (horizontals sit at ndl 0.423), so the
		// composite over the asphalt at peak opacity 0.70 lands at 2.18x the
		// floor: a pool that reads at command distance without a hot core.
		// Dressing-iteratie 3, step 4 — RE-DERIVED on measurement. The derivation
		// above still holds as a method, but its inputs moved: the floor dropped two
		// value steps in step 2 while these absolute values stayed, so the pool
		// drifted to 6.25x its own floor (measured core 0.2907 over floor 0.0465)
		// where the design says 2.18x. The second art-review pinned that from three
		// independent directions: pool-over-bulb must fall from 40% to 14-16%; the
		// lane marking inside the pool must keep >=2x its asphalt (it had INVERTED
		// to 0.79x, so the yellow line read DARKER than the road it is painted on);
		// and 2.18 x floor 0.0465 = 0.101. All three land on a frame target of
		// 0.10-0.12 lum, i.e. authored x0.35-0.40. The Glow hue factor (x0.398), the
		// shade/lit ratio and opacity 0.70 are unchanged — only the level moves.
		// STILL OPEN after this: the pool composites OVER the ground instead of
		// multiplying INTO it, which is why it can overwrite the lane marking's
		// hierarchy at all. Lowering the level makes the inversion smaller, not
		// impossible; the compositing change is its own step.
		const FPaletteDef& FloorPalette = PaletteForLabel(TEXT("Floor"));
		// Mix 0.50 expliciet: dat is de waarde waarop de pool op zijn gemeten doel
		// (0.1151 kern) is gebisecteerd, en hij hoort niet mee te bewegen als de
		// vloer zijn eigen korrel bijstelt. De TEXTUUR blijft geleend — zelfde
		// asfalt, zelfde tegelfase — alleen de mix is nu van de pool zelf.
		PoolMid = MakeGroundDecalMid(FLinearColor(0.333f, 0.151f, 0.045f), FLinearColor(0.112f, 0.051f, 0.015f), PoolMask, 0.70f, &FloorPalette, 0.50f);
		// Blob shadow: the floor tint at x0.4 — luminance only, no hue shift
		// (spec). Peak opacity 0.85 puts the core at ~0.49x the asphalt, dark
		// enough to ground a mass, far off the "silhouette black" the boulder
		// stain was flagged for.
		// Dressing-iteratie 3 — same numbers, no longer carried by hand. These were
		// authored as absolutes back when the floor was two value steps lighter, so
		// they were one more floor move away from the silent drift that put the pool
		// at 6.25x its target in step 4. x0.775 of the CURRENT floor entry reproduces
		// (0.066,0.072,0.084)/(0.022,0.024,0.038) to within 1%, so this changes no
		// pixel today and cannot drift tomorrow.
		// The LEVEL is deliberately left where it is. DRESSING_ITERATIE_3 par. 5 sets
		// the target at a core of ~0.62x the ground beside it and warns that going
		// darker turns the blob into the silhouette black the boulder stain was
		// already flagged for. Measured, cam 7, 7 sites / 5248 px against a 14px ring
		// of their own surrounding ground (Tools/measure_masked_region.py):
		//   as authored (x0.775)  0.0742 in / 0.1193 ring = 0.62x  — ON target
		//   trial at x0.40        0.0576 in / 0.1190 ring = 0.48x  — reverted, under
		//                                                            the par. 5 bound
		// So the read defect the art-review reported ("zero contact anchoring in all
		// six frames") is NOT this level: it is coverage, fixed in the loops below,
		// plus the skirt still being derived from footprint instead of height, which
		// is par. 5's own explanation for why no container blob was findable in 00065
		// and is its own measured step.
		// VERSE STENCIL-METING op de rustiger vloer (shots 00236 stencil -> 00229
		// echt): drie plekken op KAAL asfalt geven binnen 0.0303 / ring 0.0320 =
		// 0.95x. De contactschaduw is daar dus vrijwel afwezig, en dat bevestigt de
		// art-review: de eerdere 0.62x werd volledig gedragen door lamppaalvoeten
		// die in een lichtplek staan. Het rustiger maken van de vloer maakt de blob
		// beter ZICHTBAAR ten opzichte van de ruis, maar verandert zijn contrast
		// niet — een stap van 5% blijft een stap van 5%.
		// De oorzaak zit in de compositing, niet in dit getal: de blob ligt met
		// opacity 0.85 OVER de grond met een eigen waarde vlak bij die van de vloer,
		// dus tegen donker asfalt landt hij bijna op hetzelfde niveau. De
		// modulate-variant uit par. 1h is daarvoor de fix, en die moet samen met de
		// sorteervolgorde van de pools gebeuren — een eigen stap, niet nog een tint.
		BlobMid = MakeGroundDecalMid(FLinearColor(0.60f, 0.62f, 0.66f), FLinearColor(0.60f, 0.62f, 0.66f), BlobMask, 0.85f, nullptr, -1.0f, ToonShadowMaterial);
	}

	// Thin no-collision ground quad for the two light-pass cues. Non-uniform
	// Size turns the mask's disc into the ellipse a rectangular mass needs, and
	// Yaw carries the mass's own rotation so the ellipse lies along it.
	auto SpawnGroundDecal = [&World, CubeMesh, &Params](UMaterialInstanceDynamic* Mid, const FVector2D& Center, const FVector2D& Size, float Yaw, float Z, const TCHAR* Tag, int32 SortPriority = 0)
	{
		if (Mid == nullptr)
		{
			return; // masks/master absent — the warning above already said so
		}
		AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(FVector(Center.X, Center.Y, Z), FRotator(0.0f, Yaw, 0.0f), Params);
		if (Actor == nullptr)
		{
			return;
		}
		Actor->SetMobility(EComponentMobility::Movable);
		Actor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
		Actor->SetActorScale3D(FVector(Size.X / 100.0f, Size.Y / 100.0f, 0.04f));
		Actor->GetStaticMeshComponent()->SetMaterial(0, Mid);
		Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
		Actor->GetStaticMeshComponent()->SetCastShadow(false);
		// Doorschijnende quads sorteren op afstand, en deze twee cues liggen 3 units
		// uit elkaar op dezelfde vloer - dus welke wint werd feitelijk door de camera
		// bepaald. Zo verloor de modulate-schaduw de vorige poging: de lichtplek
		// tekende erna en veegde de verdonkering weg (gemeten cam 6: 0.57x voor de
		// multiply, 0.82x erna - de fix maakte het slechter). Een expliciete
		// prioriteit beslist het op bedoeling: de schaduw tekent als laatste, zodat
		// een massa die IN een lichtplek staat zijn contactschaduw OP die plek werpt,
		// wat hij buiten ook doet.
		Actor->GetStaticMeshComponent()->SetTranslucentSortPriority(SortPriority);
		Actor->SetActorEnableCollision(false);
		Actor->Tags.Add(Tag);
	};
	// Tekenvolgorde van de twee grond-cues; laag tekent eerst.
	constexpr int32 PoolSortPriority = 1;
	constexpr int32 BlobSortPriority = 2;

	// Lamp pools — consumer #2 of LampPosts (the prop pass below spawns pole +
	// glow head from the same rows). Pool centre = pole + 80 units along the arm
	// yaw, exactly as the list documents: the Blender lamp hangs its head along
	// local +X, so the light lands ahead of the pole base, not around it.
	// Disc diameter comes from the row (see FLampPostDef: 700 = one asphalt tile
	// period, the yard lamp runs smaller so it cannot print light through a
	// wall). A pool whose pole failed to import still spawns by design: the
	// ground read is the cue, and the missing mesh logs from the prop pass
	// (14.3.5).
	for (const FLampPostDef& Lamp : LampPosts)
	{
		const float YawRad = FMath::DegreesToRadians(Lamp.Yaw);
		SpawnGroundDecal(PoolMid,
			FVector2D(Lamp.X + FMath::Cos(YawRad) * 80.0f, Lamp.Y + FMath::Sin(YawRad) * 80.0f),
			FVector2D(Lamp.PoolSize, Lamp.PoolSize), Lamp.Yaw, PoolDecalZ, TEXT("Deco_LampPool"), PoolSortPriority);
	}
	// Grounding coverage (dressing-iteratie 3, review item 6). Blobs used to spawn
	// from four mass classes only — rubble, machine banks, containers, the bunker —
	// none of which is the cover field, so the art-review's verdict on the overview
	// frame was "the whole cover field, every prop, the poles and the figures float".
	// These two loops add the 20 cover blocks and the 6 lamp poles, the two classes
	// that stand in the middle of the street where the camera looks. With the value
	// ceiling down, contact darkening is the ONLY cue left that ties an object to the
	// ground it stands on, so the gap matters more now than it did before that fix.
	// These two classes are spawned from their OWN authored coordinate lists — the
	// same discipline the lamp pools follow — so a blob can never drift away from
	// the thing that casts it, and moving a cover block moves its shadow.
	// Cover-block size, corrected after the art-review pulled two authoring errors
	// out of the first version of this loop:
	//  1. The blocks ALTERNATE their long axis (see the CoverPoints spawn above:
	//     bRotated flips 3x1 to 1x3), and this loop spawned a fixed 420x240 at
	//     yaw 0. So ten of the twenty blobs lay crosswise under their own block —
	//     4.2x too wide across it and 60 units SHORT at each end, i.e. the block
	//     stuck out of its own shadow at both tips. The lamp loop below already
	//     passed Lamp.Yaw; this one simply forgot.
	//  2. The old comment read "cover blocks are 300x120, so 1.4x the footprint".
	//     120 is the HEIGHT (Z 1.2), not a footprint dimension — the footprint is
	//     300x100, which made the authored 240 a 2.4x skirt on the short axis
	//     while claiming to be 1.4x. A number that documents itself wrongly is
	//     worse than no number, because the next reader trusts it.
	// So the skirt is derived, per par. 5's rule, from the HEIGHT rather than the
	// footprint: a tall mass hides more of its own contact shadow behind itself
	// from eye height, so it needs a wider ring to show anything at all.
	// Blocks are 1.2 units tall = 120, so skirt = 0.35 x 120 = 42 units per side.
	constexpr float CoverBlockHeight = 120.0f;
	constexpr float CoverBlockSkirt = 0.35f * CoverBlockHeight;
	int32 BlobCoverIndex = 0;
	for (const FPointDef& Cover : CoverPoints)
	{
		// Same alternation as the block spawn, read from the same counter parity,
		// so the blob turns with the block it belongs to instead of being told a
		// size twice.
		const bool bRotated = (BlobCoverIndex++ % 2) == 0;
		const FVector2D Footprint = bRotated ? FVector2D(300.0f, 100.0f) : FVector2D(100.0f, 300.0f);
		SpawnGroundDecal(BlobMid, FVector2D(Cover.X, Cover.Y),
			Footprint + FVector2D(2.0f * CoverBlockSkirt, 2.0f * CoverBlockSkirt),
			0.0f, BlobDecalZ, TEXT("Deco_Blob"), BlobSortPriority);
	}
	for (const FLampPostDef& Lamp : LampPosts)
	{
		SpawnGroundDecal(BlobMid, FVector2D(Lamp.X, Lamp.Y), FVector2D(150.0f, 150.0f), Lamp.Yaw, BlobDecalZ, TEXT("Deco_Blob"), BlobSortPriority);
	}

	UE_LOG(LogEclipse, Display, TEXT("Graybox: light pass — %d lamp pools (%s) from the same LampPosts[] the poles use; blob shadows %s, now on cover blocks and lamp feet as well as the dressing masses."),
		PoolMid != nullptr ? static_cast<int32>(UE_ARRAY_COUNT(LampPosts)) : 0,
		PoolMid != nullptr ? TEXT("ok") : TEXT("SKIPPED"),
		BlobMid != nullptr ? TEXT("ok") : TEXT("SKIPPED"));

	// PLACEHOLDER(15.4): first real prop meshes — CC0 Poly Haven FBX restyled
	// through the toon pipeline (mesh-UV albedo, luminance-only; provenance in
	// Content/Art/Textures/SOURCES.md). Dressing tier for now: no collision —
	// a later pass promotes correctly-sized props to real cover WITH the squad
	// scenario suite re-run (SPEC-P1-06); missing assets degrade to nothing.
	{
		struct FPropDef { const TCHAR* Label; const TCHAR* MeshPath; const TCHAR* TexPath; float TexGain; FLinearColor Lit; FLinearColor Shade; };
		const FPropDef Props[] = {
			{ TEXT("Prop_Barrel"), TEXT("/Game/Art/Props/Barrel_01.Barrel_01"), TEXT("/Game/Art/Textures/T_Barrel_01_diff.T_Barrel_01_diff"), 17.6f, FLinearColor(0.160f, 0.100f, 0.070f), FLinearColor(0.055f, 0.042f, 0.062f) },
			// Value ceiling, offender #2 by excess (iteratie 3 step 7): this face
			// measured 0.6542 lum in frame = 14.1x the floor and 2.25x the pool
			// core, i.e. the brightest non-emissive thing in the district and
			// brighter than the light itself. Same method as Wall_: authored x0.35
			// as the starting point, then bisect on the measured frame value.
			// Second bisection step: x0.35 landed the face at 0.2909 in frame where
			// the target is <=0.15, so halve again. The mapping is measured
			// non-linear (authored 0.2701 -> 0.654 = 2.42x; authored 0.0945 ->
			// 0.2909 = 3.08x — the ratio RISES as the value falls), which is exactly
			// why this is bisected against the probe instead of computed.
			{ TEXT("Prop_Barrier"), TEXT("/Game/Art/Props/concrete_road_barrier.concrete_road_barrier"), TEXT("/Game/Art/Textures/T_concrete_road_barrier_diff.T_concrete_road_barrier_diff"), 6.7f, FLinearColor(0.046f, 0.048f, 0.053f), FLinearColor(0.015f, 0.016f, 0.025f) },
			// Same desaturation as BldgB: the review read this crate as a neon-cyan
			// "tech krate" from another planet's palette (measured hue 191, sat 0.77).
			// Keep it in the worker-teal family, take the neon out.
			// Same bisection step as BldgB above — the crate is the same family and
			// must move with it, or the warehouse and its crates drift into two
			// different teals.
			{ TEXT("Prop_Crate"), TEXT("/Game/Art/Props/plastic_crate_03.plastic_crate_03"), TEXT("/Game/Art/Textures/T_plastic_crate_03_diff.T_plastic_crate_03_diff"), 8.0f, FLinearColor(0.205f, 0.260f, 0.270f), FLinearColor(0.093f, 0.096f, 0.135f) },
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

		// ---- ARMATUREN (owner-besluit 26-07 avond: ja op het lichtplan) ---------
	//
	// VEERTIEN, niet honderd. Vijf bakens op de doelsites, acht wandarmaturen op
	// de routes ertussen, en de extractie als sterkste bron. Daarna een ronde
	// vaste-camera screenshots door de art-review, en pas dan breed.
	//
	// De opbouw komt uit het Epic-leerproject over cinematische belichting - de
	// OPBOUW en niet de techniek, want ray tracing blijft uit: POELEN met donker
	// ertussen, een helderste bron, en elk licht heeft een zichtbare armatuur.
	// Wat het district mist is contrast, niet helderheid.
	//
	// GEEN EIGEN KLEUR voor de bakens (owner-besluit). Dat volgt de regel die het
	// district al heeft: het palet is de enige kleur-autoriteit (15.5), en de
	// hierarchie komt uit HELDERHEID.
	{
		struct FFixtureDef
		{
			const TCHAR* Label;
			const TCHAR* MeshPath;
			const TCHAR* BaseColorPath;
			const TCHAR* EmissivePath;
			FVector Location;
			float Yaw;
			float GlowGain;
		};

		const FFixtureDef Fixtures[] = {
			// De vijf doelsites. Een doel dat oplicht is leesbaar vanaf de andere
			// kant van het plein; dat is navigatie, geen dressing.
			{ TEXT("Beacon_ControlPost"), TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Heavy_Duty_Linear_Beacon/Mesh/SM_Sci_Fi_Heavy_Duty_Linear_Beacon.SM_Sci_Fi_Heavy_Duty_Linear_Beacon"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Heavy_Duty_Linear_Beacon/Textures/T_Sci_Fi_Heavy_Duty_Linear_Beacon_BaseColor.T_Sci_Fi_Heavy_Duty_Linear_Beacon_BaseColor"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Heavy_Duty_Linear_Beacon/Textures/T_Sci_Fi_Heavy_Duty_Linear_Beacon_Emissive.T_Sci_Fi_Heavy_Duty_Linear_Beacon_Emissive"),
			  FVector(5000, -2000, 0), 0.0f, 10.0f },
			{ TEXT("Beacon_AlarmRelay"), TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Monolith_Light/Mesh/SM_Sci_Fi_Monolith_Light.SM_Sci_Fi_Monolith_Light"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Monolith_Light/Textures/T_Sci_Fi_Monolith_Light_BaseColor.T_Sci_Fi_Monolith_Light_BaseColor"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Monolith_Light/Textures/T_Sci_Fi_Monolith_Light_Emissive.T_Sci_Fi_Monolith_Light_Emissive"),
			  FVector(5000, 1500, 0), 0.0f, 10.0f },
			{ TEXT("Beacon_Crane"), TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Monolith_Light/Mesh/SM_Sci_Fi_Monolith_Light.SM_Sci_Fi_Monolith_Light"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Monolith_Light/Textures/T_Sci_Fi_Monolith_Light_BaseColor.T_Sci_Fi_Monolith_Light_BaseColor"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Monolith_Light/Textures/T_Sci_Fi_Monolith_Light_Emissive.T_Sci_Fi_Monolith_Light_Emissive"),
			  FVector(-4000, 3000, 0), 0.0f, 10.0f },
			{ TEXT("Beacon_Pens"), TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Bollard_Light/Mesh/SM_Sci_Fi_Bollard_Light.SM_Sci_Fi_Bollard_Light"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Bollard_Light/Textures/T_Sci_Fi_Bollard_Light_BaseColor.T_Sci_Fi_Bollard_Light_BaseColor"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Bollard_Light/Textures/T_Sci_Fi_Bollard_Light_Emissive.T_Sci_Fi_Bollard_Light_Emissive"),
			  FVector(-4000, 2600, 0), 0.0f, 10.0f },
			// DE STERKSTE BRON van het district: een punt dat feller is dan al het
			// andere. Dat is de hierarchie waar de referentie op draait, en het is
			// ook het punt waar je heen moet.
			{ TEXT("Beacon_Extraction"), TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Heavy_Duty_Linear_Beacon/Mesh/SM_Sci_Fi_Heavy_Duty_Linear_Beacon.SM_Sci_Fi_Heavy_Duty_Linear_Beacon"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Heavy_Duty_Linear_Beacon/Textures/T_Sci_Fi_Heavy_Duty_Linear_Beacon_BaseColor.T_Sci_Fi_Heavy_Duty_Linear_Beacon_BaseColor"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Heavy_Duty_Linear_Beacon/Textures/T_Sci_Fi_Heavy_Duty_Linear_Beacon_Emissive.T_Sci_Fi_Heavy_Duty_Linear_Beacon_Emissive"),
			  FVector(-8500, -8500, 0), 0.0f, 16.0f },
		};

		// De acht wandarmaturen op de routes ertussen. Poelen; het donker tussen
		// twee poelen hoort net zo goed bij het plan als de poelen zelf.
		const FVector RoutePoints[] = {
			FVector(3200, -1400, 0), FVector(1500, -600, 0), FVector(0, 300, 0), FVector(-1800, 1200, 0),
			FVector(-3000, 2200, 0), FVector(2600, 900, 0), FVector(-5600, -2600, 0), FVector(-7200, -5600, 0),
		};

		UMaterialInterface* FixtureMaster = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Game/Art/M_EclipseToonFixture.M_EclipseToonFixture"));
		if (FixtureMaster == nullptr)
		{
			UE_LOG(LogEclipse, Warning,
				TEXT("Graybox: M_EclipseToonFixture ontbreekt - armaturen blijven ongelicht (draai Tools/create_fixture_material.py)."));
		}

		int32 Placed = 0;
		const FLinearColor SunVector(FVector4(SunRotation.Vector(), 0.0f));
		auto SpawnFixture = [&World, &Params, FixtureMaster, SunVector, &Placed](
			const TCHAR* Label, const TCHAR* MeshPath, const TCHAR* BasePath, const TCHAR* EmissivePath,
			const FVector& Location, float Yaw, float GlowGain)
		{
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
			if (Mesh == nullptr)
			{
				UE_LOG(LogEclipse, Warning, TEXT("Graybox: armatuur %s ontbreekt (%s)."), Label, MeshPath);
				return;
			}
			AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, FRotator(0.0f, Yaw, 0.0f), Params);
			if (Actor == nullptr)
			{
				return;
			}
			Actor->SetMobility(EComponentMobility::Movable);
			Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
			Actor->Tags.Add(Label);
			// EEN GEDEELDE TAG NAAST HET LABEL. Het label zegt WELKE armatuur dit
			// is; deze zegt DAT het er een is, en dat is wat een isolatie-opname
			// nodig heeft — zonder gemeenschappelijke tag kun je ze niet als groep
			// overhouden.
			//
			// Deze regel stond eerst in SpawnBlock, de generieke blokkenspawner:
			// 226 districtblokken kregen daardoor het label "armatuur" en de
			// isolatie-opname liet het halve district staan. Gevonden doordat het
			// GETAL niet klopte (226 tegen 13 geplaatste armaturen) en niet doordat
			// het beeld er raar uitzag — dat zag er namelijk plausibel uit.
			Actor->Tags.Add(TEXT("EclipseFixture"));

			if (FixtureMaster != nullptr)
			{
				UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(FixtureMaster, &World);
				// Behuizing door de toon-master, met de graphite-tint van het
				// district. Het PALET bepaalt de kleur, ook hier.
				Mid->SetVectorParameterValue(TEXT("LitColor"), FLinearColor(0.300f, 0.325f, 0.377f));
				Mid->SetVectorParameterValue(TEXT("ShadeColor"), FLinearColor(0.098f, 0.107f, 0.169f));
				Mid->SetVectorParameterValue(TEXT("LightDir"), SunVector);
				Mid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
				if (UTexture* Base = LoadObject<UTexture>(nullptr, BasePath))
				{
					Mid->SetTextureParameterValue(TEXT("AlbedoTex"), Base);
					Mid->SetScalarParameterValue(TEXT("AlbedoMix"), 0.85f);
				}
				// HET LICHTVLAK. De emissive-map van het pack zegt WAAR het licht
				// zit; GlowColor zegt welke kleur (palet), GlowGain hoe fel.
				if (UTexture* Emissive = LoadObject<UTexture>(nullptr, EmissivePath))
				{
					Mid->SetTextureParameterValue(TEXT("EmissiveMaskTex"), Emissive);
				}
				Mid->SetScalarParameterValue(TEXT("GlowGain"), GlowGain);

				// WELKE INGANG KREEG DEZE ARMATUUR ECHT. Op de isolatie-opname van
				// 26-07 stonden alle dertien als DONKERE dozen in beeld, terwijl
				// een emissive met gain 6-16 juist had moeten uitslaan. "Ze
				// gloeien niet" is dan nog geen bevinding: het kan de textuur
				// zijn, de gain, of de materiaalgraaf, en die drie vragen om
				// verschillende reparaties.
				//
				// Zelfde vorm als de proplogregel ernaast (slots/tex/mid): zeg per
				// object welke ingang aankwam, zodat een ontbrekende niet als
				// "ziet er donker uit" hoeft te worden geraden.
				// EN OF DE PARAMETERS ÜBERHAUPT BESTAAN op deze master.
				// SetTextureParameterValue en SetScalarParameterValue doen STIL
				// NIETS als de naam er niet in zit: geen fout, geen waarschuwing,
				// gewoon een no-op. Alle regels hierboven "slagen" dan terwijl er
				// niets aankomt — en dat is precies de vorm die verklaart waarom
				// dertien armaturen met emissive=ok toch pikdonker zijn.
				// HOE GROOT IS DIT DING EIGENLIJK. Op de isolatie-opname zijn de
				// armaturen stipjes, en bij gain 200 licht alleen een flinterdun
				// randje op. Dat past net zo goed bij "de emissive is stuk" als
				// bij "de armatuur is klein en de lichtstrip is een fractie
				// daarvan", en die twee vragen om totaal verschillende
				// reparaties.
				//
				// Een beacon uit dit pack is behuizing met EEN lichtstrip; dat de
				// behuizing donker blijft is correct. De vraag is of die strip op
				// commando-afstand nog bestaat.
				const FVector FixtureSize = Actor->GetStaticMeshComponent()->Bounds.BoxExtent * 2.0f;
				UE_LOG(LogEclipse, Display, TEXT("Graybox: armatuur %s meet %.0f x %.0f x %.0f cm"),
					Label, FixtureSize.X, FixtureSize.Y, FixtureSize.Z);

				UTexture* ProbeTex = nullptr;
				float ProbeGain = -1.0f;
				const bool bHasEmissiveParam = Mid->GetTextureParameterValue(FMaterialParameterInfo(TEXT("EmissiveMaskTex")), ProbeTex);
				const bool bHasGainParam = Mid->GetScalarParameterValue(FMaterialParameterInfo(TEXT("GlowGain")), ProbeGain);

				UE_LOG(LogEclipse, Display,
					TEXT("Graybox: armatuur %s  mesh=ok  basis=%s  emissive=%s  gain=%.1f  |  master kent EmissiveMaskTex=%s GlowGain=%s"),
					Label,
					LoadObject<UTexture>(nullptr, BasePath) != nullptr ? TEXT("ok") : TEXT("ONTBREEKT"),
					LoadObject<UTexture>(nullptr, EmissivePath) != nullptr ? TEXT("ok") : TEXT("ONTBREEKT"),
					GlowGain,
					bHasEmissiveParam ? TEXT("ja") : TEXT("NEE"),
					bHasGainParam ? TEXT("ja") : TEXT("NEE"));

				if (!bHasEmissiveParam || !bHasGainParam)
				{
					UE_LOG(LogEclipse, Warning,
						TEXT("Graybox: M_EclipseToonFixture mist een lichtparameter — de armaturen blijven donker hoe hoog de gain ook staat (14.3.5)."));
				}
				for (int32 Slot = 0; Slot < Actor->GetStaticMeshComponent()->GetNumMaterials(); ++Slot)
				{
					Actor->GetStaticMeshComponent()->SetMaterial(Slot, Mid);
				}
			}
			Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
			// Armaturen zijn dressing: ze mogen nav, dekking en hitscan niet
			// verstoren - dezelfde regel als de lane paint en de vlekken.
			Actor->SetActorEnableCollision(false);
			++Placed;
		};

		for (const FFixtureDef& Def : Fixtures)
		{
			SpawnFixture(Def.Label, Def.MeshPath, Def.BaseColorPath, Def.EmissivePath, Def.Location, Def.Yaw, Def.GlowGain);
		}
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(RoutePoints)); ++Index)
		{
			// Een stap onder de bakens (gain 6 tegen 10): de routes wijzen de weg,
			// de doelen trekken je erheen.
			SpawnFixture(TEXT("RouteLight"), TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Patterned_Wall_Light/Mesh/SM_Sci_Fi_Patterned_Wall_Light.SM_Sci_Fi_Patterned_Wall_Light"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Patterned_Wall_Light/Textures/T_Sci_Fi_Patterned_Wall_Light_BaseColor.T_Sci_Fi_Patterned_Wall_Light_BaseColor"),
			  TEXT("/Game/Sci_Fi_Light/Static_Meshes/SM_Sci_Fi_Patterned_Wall_Light/Textures/T_Sci_Fi_Patterned_Wall_Light_Emissive.T_Sci_Fi_Patterned_Wall_Light_Emissive"),
				RoutePoints[Index] + FVector(0.0f, 0.0f, 260.0f), Index * 45.0f, 6.0f);
		}
		UE_LOG(LogEclipse, Display, TEXT("Graybox: %d armaturen geplaatst (5 bakens, 8 routes)."), Placed);
	}

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

		// Pack-slim (ASSET_CLEANUP §10): loads from the repo-tracked Imported copy;
	// the old ParagonMinions path only survives as a redirector stub on this
	// machine and may be deleted once a shot round verifies this line.
	UStaticMesh* Well = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Imported/Meshes/SM_Well_Center_FX.SM_Well_Center_FX"));
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
			if (UTexture* PadTex = LoadObject<UTexture>(nullptr, TEXT("/Game/Art/Imported/Textures/T_4k_SciFi10_2_BaseColor.T_4k_SciFi10_2_BaseColor")))
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

			// OP MAAT BRENGEN. Deze packs zijn niet op de schaal van de mannequin
			// geauthord: gemeten op het eerste echte spelbeeld stond er een
			// BlueSoldier van 328,4 cm naast een speler van 189,6 cm — 1,7x, een
			// reus van ruim drie meter die het halve frame vulde.
			//
			// Geen enkele meting op de speler had dit kunnen vinden: de speler
			// klopte. De fout bestaat pas als je twee lichamen NAAST elkaar ziet,
			// en dus pas op een beeld. Precies het gat waar de owner op wees.
			//
			// Genormaliseerd en niet met een vast getal: elk pack heeft zijn eigen
			// maat, en de volgende die erbij komt hoort zichzelf te corrigeren in
			// plaats van deze regel opnieuw te breken.
			const float AuthoredHeight = Component->Bounds.BoxExtent.Z * 2.0f;
			if (AuthoredHeight > KINDA_SMALL_NUMBER)
			{
				const float TargetHeight = 180.0f;
				const float Correction = TargetHeight / AuthoredHeight;
				// Alleen ingrijpen bij een ECHT verschil; een figuur die al klopt
				// hoort niet door afrondruis een schaal te krijgen.
				if (FMath::Abs(1.0f - Correction) > 0.05f)
				{
					Component->SetWorldScale3D(FVector(Correction));
					UE_LOG(LogEclipse, Display,
						TEXT("Graybox: figuur %s stond op %.1f cm — op maat gebracht naar %.0f cm (schaal %.3f)."),
						*Mesh->GetName(), AuthoredHeight, TargetHeight, Correction);
				}
			}

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

		// 14.3.5 gap closed (art-review of iteratie 2): this lambda used to
		// swallow a missing generated mesh silently, so a district built without
		// Tools/blender output looked "fine" while props were simply absent —
		// exactly the quiet degradation the ladder forbids. One warning per
		// missing mesh, then carry on.
		int32 MissingGenMeshes = 0;
		auto SpawnGen = [&World, &Params, &MissingGenMeshes](UStaticMesh* Mesh, UMaterialInstanceDynamic* Mid, const FVector& Location, float Yaw, const FVector& Scale = FVector(1.0f))
		{
			if (Mesh == nullptr)
			{
				++MissingGenMeshes;
				return;
			}
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

		// Lamp posts — consumer #1 of LampPosts (the light pass above drops each
		// pool from the same rows). The old local Lamps[] table here was that
		// second position list: five poles listed twice over, with the crossing
		// pair's sixth pole hand-placed 60 lines further down next to the cable.
		// One list means a pool can never end up without its lamp, and moving a
		// lamp moves its light (dressing-iteratie 2, spec step 2).
		// Dressing-iteratie 3, step 3 — make the SOURCE visible. The art-review
		// measured every lamp head at 0.038-0.10 linear while its own pool reads
		// 0.3867: the bulb is ~4x DARKER than the light it casts, which is the
		// strongest remaining reason a pool reads as a painted disc rather than
		// sodium light. Two wrong diagnoses were tried first, both measured and
		// reverted, so the cause is now known rather than guessed:
		//   1. "the plane sits edge-on because it carries the lamp's yaw" — a
		//      cross-billboard at Yaw+90 changed nothing (brightest pixel in cam 7
		//      went 225.3 -> 215.6, and in both shots that pixel is the neutral
		//      white barrier, not the lamp; an image diff found no new bright
		//      cluster). Reverted: double the actors, zero gain (12.4).
		//   2. "the glow is spawned at Z = 0" — false: gen_street_props.py authors
		//      GlowPlane at (head_x, 0, cap_z - 0.13), i.e. the height IS baked in.
		// The ACTUAL cause, from that same script: the plane is HORIZONTAL and
		// rotated pi about X so it faces straight DOWN into the pool. A horizontal
		// quad is edge-on to every eye-level camera no matter its yaw — which is
		// exactly why rotating it did nothing — and what little a camera does catch
		// is its culled back face. The lamp therefore has no side-visible source.
		// Fix: a small emissive bulb block in the hood mouth, on the existing Glow
		// palette (2.2,1.0,0.3 — the same entry the window strips use), sized to the
		// 0.8 m hood and placed at the measured head offset rotated into the post's
		// yaw. Visible from every angle, one actor per lamp, no new mesh and no
		// Blender re-run. The downward plane stays: it is the light leaving the hood.
		constexpr float LampHeadLocalX = 122.6f; // head_x = top.x + 0.60 m, in cm
		constexpr float LampHeadLocalZ = 436.6f; // cap_z - 0.13 m, the glow plane's own height
		for (const FLampPostDef& Post : LampPosts)
		{
			SpawnGen(Lamp, MetalMid, FVector(Post.X, Post.Y, 0.0f), Post.Yaw);
			// The GlowPlane spawn is GONE, and the review that killed it also
			// proved my earlier revert wrong. I had dismissed "the glow sits at
			// Z=0" because gen_street_props.py authors it at (head_x, 0,
			// cap_z-0.13) — but authored height is not shipped height: the
			// art-review found the quad lying flat in the street at the mast foot
			// in shot 00092 (1105,628)-(1215,655), 0.2745 lum, hue 34.8, with a
			// hard silver edge and an ink outline around it. A literal sticker in
			// the road. Lesson: verify what survives export/import, never trust the
			// authoring script's local transform.
			// Not re-placed at the head either: the emissive bulb below already is
			// the visible source, and the pool decal already is the light on the
			// ground, so a third element would only risk double-placement.
			const FVector HeadOffset = FRotator(0.0f, Post.Yaw, 0.0f).RotateVector(FVector(LampHeadLocalX, 0.0f, LampHeadLocalZ));
			SpawnBlock(TEXT("Glow"), FVector(Post.X, Post.Y, 0.0f) + HeadOffset, FVector(0.45f, 0.28f, 0.14f), Post.Yaw);
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
		// hung at pole height. Strung at natural scale between the crossing lamp
		// pair; both poles of that pair now live in LampPosts (rows 1 and 6, 420
		// units apart = the measured span), so only the cable spans them here.
		SpawnGen(Cable, MetalMid, FVector(-4440, -700, 0), 0.0f);

		// Gate portal at the Entry_Main approach (measured kit: CornerPillar
		// 80x80x370, GantryBeam 50x600x50): two frames across the artery —
		// corner pillars as legs OUTSIDE the y=+-460 lane lines, beams scaled
		// 1.8 along their span so they bridge the 1080-unit gap flush with the
		// pillar outer faces. Metal tint; same no-collision dressing tier.
		// 15.8 dressing round: the beams carry the SciFi10 dense-grid plating
		// (slot 10, curation §5 destination, measured gain 2.35) over the same
		// metal tint — world-aligned (UVMode 0), so the kit beam needs no
		// authored UVs. Missing albedo = flat metal cel (14.3.5).
		UMaterialInstanceDynamic* GantryMid = MakeMid(MetalLit, MetalShade);
		if (GantryMid != nullptr)
		{
			if (UTexture* GridTex = LoadObject<UTexture>(nullptr, TEXT("/Game/Art/Imported/Textures/T_4k_SciFi10_10_BaseColor.T_4k_SciFi10_10_BaseColor")))
			{
				GantryMid->SetTextureParameterValue(TEXT("AlbedoTex"), GridTex);
				GantryMid->SetScalarParameterValue(TEXT("TexWorldScale"), 150.0f);
				GantryMid->SetScalarParameterValue(TEXT("AlbedoGain"), 2.35f);
				GantryMid->SetScalarParameterValue(TEXT("AlbedoMix"), 0.6f);
			}
			else
			{
				UE_LOG(LogEclipse, Warning, TEXT("Graybox: SciFi10 grid albedo missing — gantry beams stay flat metal cel (14.3.5)."));
			}
		}
		for (const float GateX : { -8850.0f, -8550.0f })
		{
			SpawnGen(KPillar, MetalMid, FVector(GateX, -500.0f, 0), 0.0f);
			SpawnGen(KPillar, MetalMid, FVector(GateX, 500.0f, 0), 0.0f);
			SpawnGen(KGantry, GantryMid != nullptr ? GantryMid : MetalMid, FVector(GateX, 0, 370.0f), 0.0f, FVector(1.0f, 1.8f, 1.0f));
		}

		if (MissingGenMeshes > 0)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Graybox: %d generated prop placement(s) skipped — their meshes are missing (GDD 14.3.5; run Tools/blender/gen_street_props.py + gen_building_kit.py, then import_generated_meshes.py). Lamps, boards, vents, barricades and the kit row degrade to absent, not to raw materials."), MissingGenMeshes);
		}
	}

	// PLACEHOLDER(15.8): dressing round on the migrated curation accepts
	// (/Game/Art/Imported, pack-slim ASSET_CLEANUP §7–10) plus the three parked
	// ambientCG albedos — the last "accepted, not placed" rows of
	// phase0/ASSET_CURATION.md go live here. Dressing tier throughout:
	// deterministic placements, no collision, every surface through the toon
	// masters with the MEASURED curation gains (never estimated). Missing
	// asset = log + skip (GDD 14.3.5).
	{
		UMaterialInterface* DressMaster = ToonLitMaterial != nullptr ? ToonLitMaterial : ToonMaterial;
		if (DressMaster == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Graybox: toon masters missing — 15.8 dressing skipped (raw materials never render, 15.5)."));
		}
		else
		{
			const bool bUnlitMaster = ToonLitMaterial == nullptr;
			// Standard curation restyle contract: palette tint + optional
			// world-aligned albedo (UVMode 0 default — none of these meshes
			// needs authored UVs; luminance only, hue stays with the palette).
			auto MakeDressMid = [DressMaster, bUnlitMaster, &World](const FLinearColor& Lit, const FLinearColor& Shade, const TCHAR* TexPath = nullptr, float TexGain = 1.0f, float TexScale = 300.0f, float TexMix = 0.6f) -> UMaterialInstanceDynamic*
			{
				UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(DressMaster, &World);
				Mid->SetVectorParameterValue(TEXT("LitColor"), Lit);
				Mid->SetVectorParameterValue(TEXT("ShadeColor"), Shade);
				Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunRotation.Vector(), 0.0f)));
				if (bUnlitMaster)
				{
					// Lit variant keeps EmissiveScale 1: BaseColor is albedo.
					Mid->SetScalarParameterValue(TEXT("EmissiveScale"), ToonEmissiveScale);
				}
				if (TexPath != nullptr)
				{
					if (UTexture* Albedo = LoadObject<UTexture>(nullptr, TexPath))
					{
						Mid->SetTextureParameterValue(TEXT("AlbedoTex"), Albedo);
						Mid->SetScalarParameterValue(TEXT("TexWorldScale"), TexScale);
						Mid->SetScalarParameterValue(TEXT("AlbedoGain"), TexGain);
						Mid->SetScalarParameterValue(TEXT("AlbedoMix"), TexMix);
					}
					else
					{
						UE_LOG(LogEclipse, Warning, TEXT("Graybox: dressing albedo %s missing — flat cel fallback (14.3.5)."), TexPath);
					}
				}
				return Mid;
			};
			// Masses keep shadows (physical presence); thin panels/pads pass
			// bCastShadow=false like the decal planes (VSM budget, 12.4).
			auto SpawnDress = [&World, &Params](UStaticMesh* Mesh, UMaterialInstanceDynamic* Mid, const FVector& Location, const FRotator& Rotation, const FVector& Scale, const TCHAR* Tag, bool bCastShadow)
			{
				if (Mesh == nullptr) { return; }
				AStaticMeshActor* Actor = World.SpawnActor<AStaticMeshActor>(Location, Rotation, Params);
				if (Actor == nullptr) { return; }
				Actor->SetMobility(EComponentMobility::Movable);
				Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
				Actor->SetActorScale3D(Scale);
				if (Mid != nullptr)
				{
					// Every slot (the "white barrels" lesson, first prop round).
					for (int32 SlotIndex = 0; SlotIndex < Actor->GetStaticMeshComponent()->GetNumMaterials(); ++SlotIndex)
					{
						Actor->GetStaticMeshComponent()->SetMaterial(SlotIndex, Mid);
					}
				}
				Actor->GetStaticMeshComponent()->SetAffectDistanceFieldLighting(false);
				Actor->GetStaticMeshComponent()->SetCastShadow(bCastShadow);
				Actor->SetActorEnableCollision(false);
				Actor->Tags.Add(Tag);
			};

			// Palette tints (curation tint table; liner = BldgB worker-teal
			// x0.8 so the interior sits one value step under the exterior —
			// luminance-only, hue authority stays with the palette, 15.5).
			// Slag: the DecoStain oil tint was too dark for a MASS — a stain is
			// read against asphalt, a boulder against the sky, and at 0.070 it
			// went silhouette-black (art-review 25-07, "boulder-stain 30-40%
			// lichter"). Lifted x1.35, hue untouched (luminance-only, 15.5); the
			// ground stains themselves keep the original DecoStain values.
			// Same rule, same round: the boulders measured hue 279 — violet — because
		// B sat above both R and G here too. The art-review called them "a
		// mauve-violet potato". Rust/graphite family means B <= G, so R >= G >= B.
		const FLinearColor RubbleLit(0.098f, 0.088f, 0.078f), RubbleShade(0.040f, 0.036f, 0.031f);
			// Cyan, and this is where it actually came from (owner call 2026-07-25).
			// The art-review called the neon "the teal family" and I bisected BldgB
			// and the crates for it — measured, that only removed 29 600 of the
			// 101 075 neon pixels in cam 3, and pushing red further did NOTHING
			// (71 453 -> 71 267, 0.3%). Cropping the worst pixel showed why: it is a
			// CONTAINER, which runs on Graphite, not on the teal palette entry.
			// GraphiteShade was (0.075, 0.082, 0.130) — blue at 1.73x red — which
			// breaks the rule this same iteration already banked in par. 8: every
			// dark/shade tint keeps B <= G, or the grade's ColorSaturation 1.38
			// screams the excess out. So the fix is the project's own rule applied
			// to the entry that was exempt from it, not a new number.
			const FLinearColor GraphiteLit(0.230f, 0.250f, 0.250f), GraphiteShade(0.075f, 0.082f, 0.082f);
			// Dressing-iteratie 3, step 8 — the magenta container, third attempt, and
		// the first one aimed at the actual cause. Round 1 blamed the texture mix
		// (0.75 -> 0.45): wrong variable, the container stayed magenta. The second
		// art-review measured it properly: hue 336.6 at saturation 0.77-0.81 on
		// three separate patches, sitting at value 0.51 — so it is a MID, not a
		// shade, and it inherits from OxideShade, which was itself authored at hue
		// 344.5 with B = 1.9x G. Blue above green in a rust family is magenta by
		// construction, and ColorSaturation 1.38 then shouts it.
		// Banked rule for Kessara, now covering MIDTONES and not just shades:
		// every dark/shade tint keeps B <= G. New shade is hue 9.3 — deep rust.
		// BldgA shares this shade, so its facade warms with the container.
		const FLinearColor OxideLit(0.560f, 0.160f, 0.085f), OxideShade(0.200f, 0.062f, 0.038f);
			const FLinearColor MachineLit(0.105f, 0.105f, 0.120f), MachineShade(0.040f, 0.040f, 0.052f);
			const FLinearColor LinerLit(0.048f, 0.240f, 0.248f), LinerShade(0.016f, 0.080f, 0.120f);

			// --- Contested rubble: A3 slag boulder + C1 chunk filler (curation
			// §3) in pockets against the perimeter wall — the wall is where the
			// occupation grinds ("Contested" pockets, 03.3). Stain tint, NO
			// albedo: the mossy source hue must never enter (15.5); flat cel +
			// ink outline reads as stylized slag. C1 only ever instanced 3–5x
			// overlapping under/around A3 — solo it reads low-poly (forbidden).
			UStaticMesh* Boulder = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Imported/Meshes/Granite_Large_Grey_Mossy_Rough.Granite_Large_Grey_Mossy_Rough"));
			UStaticMesh* Chunk = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Imported/Meshes/SM_Rock_Chunk_LowPoly.SM_Rock_Chunk_LowPoly"));
			if (Boulder != nullptr)
			{
				UMaterialInstanceDynamic* RubbleMid = MakeDressMid(RubbleLit, RubbleShade);
				// Pocket centres clear the wall by ONE rule (art-review 00061:
				// the gate-side boulder pushed through the wall face instead of
				// bedding into the ground). The perimeter slabs' inner faces sit
				// at ±9950 (Blocks: centre ±10000, 100-unit slab), so every
				// pocket centre keeps 400 units of clearance — the slag still
				// hugs the wall, no mesh half-extent can reach through it, and
				// the chunk scatter (±170 below) stays inside that margin.
				struct FPocketDef { FVector Center; float BoulderYaw; float BoulderScale; };
				const FPocketDef Pockets[] = {
					{ FVector(-9550.0f, -1600.0f, 0.0f), 20.0f, 0.85f },  // west wall south of the gate (cam 5 left frame)
					{ FVector(-9550.0f, 3800.0f, 0.0f), 140.0f, 1.0f },   // west wall north section (cam 2 backdrop)
					{ FVector(-2800.0f, 9550.0f, 0.0f), 260.0f, 0.75f },  // north wall (cam 4 overview)
				};
				// Deterministic pockets on every machine; the variation stream
				// is private by contract (the DecoRng lesson, 15.8 art-fix).
				FRandomStream RubbleRng(158);
				if (Chunk == nullptr)
				{
					UE_LOG(LogEclipse, Warning, TEXT("Graybox: C1 rock chunk missing — rubble pockets run boulder-only (14.3.5)."));
				}
				for (const FPocketDef& Pocket : Pockets)
				{
					// Grounding (iteratie 2): one blob shadow per pocket, sized
					// from the pocket's OWN chunk-scatter radius (±170 below)
					// plus a 90-unit skirt for the boulder — derived from the
					// numbers that place the slag, never a second position list.
					SpawnGroundDecal(BlobMid, FVector2D(Pocket.Center.X, Pocket.Center.Y),
						FVector2D(520.0f, 520.0f), Pocket.BoulderYaw, BlobDecalZ, TEXT("Deco_Blob"), BlobSortPriority);
					// Measured bounds (headless pass 2026-07-24): the debris
					// pivot sits 84.4 units above the mesh base — grounded at
					// 84.4*scale minus a settle-sink so slag reads bedded into
					// the asphalt, never floating. Sink deepened 18 -> 24 with
					// the pocket move: the review wanted the slag bedded in the
					// ground rather than shoved through the wall face.
					SpawnDress(Boulder, RubbleMid, Pocket.Center + FVector(0, 0, 84.4f * Pocket.BoulderScale - 24.0f),
						FRotator(0.0f, Pocket.BoulderYaw, 0.0f), FVector(Pocket.BoulderScale), TEXT("Deco_Rubble"), true);
					if (Chunk != nullptr)
					{
						const int32 ChunkCount = 3 + RubbleRng.RandRange(0, 2);
						for (int32 Index = 0; Index < ChunkCount; ++Index)
						{
							const float ChunkScale = RubbleRng.FRandRange(0.35f, 0.55f);
							SpawnDress(Chunk, RubbleMid,
								Pocket.Center + FVector(RubbleRng.FRandRange(-170.0f, 170.0f), RubbleRng.FRandRange(-170.0f, 170.0f), 84.4f * ChunkScale - 15.0f * RubbleRng.FRand()),
								FRotator(0.0f, RubbleRng.FRandRange(0.0f, 360.0f), 0.0f), FVector(ChunkScale), TEXT("Deco_Rubble"), true);
						}
					}
				}
			}
			else
			{
				UE_LOG(LogEclipse, Warning, TEXT("Graybox: A3 slag boulder missing — Contested rubble skipped (14.3.5)."));
			}

			// --- Warehouse-yard loading dock: A1 plinth pair (curation §1) at
			// the BldgB east gap, SciFi10_1 deck plate (gain 4.96, the plinth's
			// curation recipe) on graphite. Measured: 200x200x10 plate, pivot at
			// base — Z x2 makes it a 20 cm dock step without breaking the
			// extruded-disc silhouette.
			// Dressing-iteratie 3 — SM_AssetPlatform is GONE, and this one was
			// blocking. It is the marketplace asset-SHOWCASE turntable: the second
			// art-review read its radial spokes, its measuring ticks and its
			// checker scale-ramp at 1080p, twice over, in the middle of a banked
			// review frame (00093). That is the ÉÉN-STIJL-WET question — "does it
			// betray its source?" — answered yes with legible text on it. No amount
			// of toon-mastering fixes a mesh whose geometry IS a product photo.
			// Replacement: scaled engine cubes on the same graphite tint and the
			// same measured SciFi10_1 deck-plate gain (4.96), keeping the 20 cm
			// dock step the tread-ramp is built against (140 units of run at -8.5°
			// rises 21). Squarer than the extruded disc, which is what a loading
			// dock should be anyway.
			{
				UMaterialInstanceDynamic* PlinthMid = MakeDressMid(GraphiteLit, GraphiteShade, TEXT("/Game/Art/Imported/Textures/T_4k_SciFi10_1_BaseColor.T_4k_SciFi10_1_BaseColor"), 4.96f, 200.0f, 0.7f);
				SpawnDress(CubeMesh, PlinthMid, FVector(-2900.0f, 2500.0f, 10.0f), FRotator(0.0f, 15.0f, 0.0f), FVector(2.8f, 2.8f, 0.2f), TEXT("Deco_Dock"), true);
				SpawnDress(CubeMesh, PlinthMid, FVector(-2900.0f, 2820.0f, 10.0f), FRotator::ZeroRotator, FVector(2.8f, 2.8f, 0.2f), TEXT("Deco_Dock"), true);
			}

			// --- SciFi10_9 diamond tread plate (gain 2.70): ramp onto the dock
			// plus an entrance pad at the compound west gap (curation §5:
			// "ramps, catwalks"). Thin pads: no shadow, like the lane paint.
			UMaterialInstanceDynamic* TreadMid = MakeDressMid(GraphiteLit, GraphiteShade, TEXT("/Game/Art/Imported/Textures/T_4k_SciFi10_9_BaseColor.T_4k_SciFi10_9_BaseColor"), 2.70f, 200.0f, 0.7f);
			// Dock seam (art-review 25-07, "zweefspleet"): the ramp geometry was
			// right — 140 units of run at -8.5° pitch rises 21, exactly the
			// plinth step (10-unit plate x scale 2, spawned at Z 1) — but the Z
			// offset was not. At centre Z 12 the high end's top face landed at
			// 25.3, i.e. 4.3 ABOVE the dock, with a 1.2-unit air slit against the
			// plinth's side face. Z 12 -> 7.7 puts that face on 21.0 and the low
			// end's on 0.3 (asphalt); X -2690 -> -2700 laps the ramp 8.8 units
			// onto the plinth, so the seam is closed instead of bridged.
			// Pad 2 y-scale 2.0 -> 1.7 (code-review #5): at 200 half-length the
			// pad ran to -1500 and pierced the lamp base at (4150,-1500); 170
			// clears the pole by 15 units.
			SpawnDress(CubeMesh, TreadMid, FVector(-2700.0f, 2820.0f, 7.7f), FRotator(-8.5f, 0.0f, 0.0f), FVector(1.4f, 1.2f, 0.06f), TEXT("Deco_Ramp"), false);
			SpawnDress(CubeMesh, TreadMid, FVector(4100.0f, -1600.0f, 3.0f), FRotator::ZeroRotator, FVector(1.6f, 1.7f, 0.04f), TEXT("Deco_Ramp"), false);

			// --- SciFi10_5 perforated sheet (gain 1.11): warehouse interior
			// liners proud of the BldgB inner faces — read through the east gap
			// (cam 2 sightline). Interior sits one value step under the
			// exterior corrugated read (LinerLit above).
			UMaterialInstanceDynamic* LinerMid = MakeDressMid(LinerLit, LinerShade, TEXT("/Game/Art/Imported/Textures/T_4k_SciFi10_5_BaseColor.T_4k_SciFi10_5_BaseColor"), 1.11f, 300.0f, 0.5f);
			// "Proud of" must mean touching (the teal-fin watch item): at ±10 from
			// the wall centre these 5-unit panels stood in 7.5 units of AIR, and
			// with ink outlines a floating plate reads as a loose teal fin rather
			// than a wall lining. Each liner now laps 1 unit INTO its wall slab
			// (BldgB inner faces: W -4750, S 2250, N 3750 from the 100-unit slabs
			// at -4800/2200/3800) — no gap to see, no coplanar faces to fight.
			SpawnDress(CubeMesh, LinerMid, FVector(-4748.5f, 3000.0f, 200.0f), FRotator::ZeroRotator, FVector(0.05f, 11.0f, 3.6f), TEXT("Deco_Liner"), false);
			SpawnDress(CubeMesh, LinerMid, FVector(-4000.0f, 2251.5f, 200.0f), FRotator::ZeroRotator, FVector(11.0f, 0.05f, 3.6f), TEXT("Deco_Liner"), false);
			SpawnDress(CubeMesh, LinerMid, FVector(-4000.0f, 3748.5f, 200.0f), FRotator::ZeroRotator, FVector(11.0f, 0.05f, 3.6f), TEXT("Deco_Liner"), false);

			// --- SciFi10_7 cross-braced plating (gain 1.78): Dominion post trim
			// bands proud of the BldgA west + north facades (curation §5), oxide
			// family — the compound reads authored, not extruded.
			UMaterialInstanceDynamic* TrimMid = MakeDressMid(OxideLit, OxideShade, TEXT("/Game/Art/Imported/Textures/T_4k_SciFi10_7_BaseColor.T_4k_SciFi10_7_BaseColor"), 1.78f, 250.0f, 0.6f);
			// Band Z 350 -> 365 (code-review #2, poster clash): at 350 the band
			// spanned 322.5..377.5 and the compound's north poster (Z 210, half
			// height 120 -> top 330) pushed 7.5 units through it. 365 spans
			// 337.5..392.5 — clear of the poster and still under the 400-unit
			// facade crest.
			SpawnDress(CubeMesh, TrimMid, FVector(4145.0f, -2400.0f, 365.0f), FRotator::ZeroRotator, FVector(0.05f, 8.0f, 0.55f), TEXT("Deco_Trim"), false);
			SpawnDress(CubeMesh, TrimMid, FVector(5000.0f, -1145.0f, 365.0f), FRotator::ZeroRotator, FVector(16.0f, 0.05f, 0.55f), TEXT("Deco_Trim"), false);

			// --- Machine banks: dark mottled steel bodies (ambientCG Metal046B,
			// gain 19.76 — curation §9 "machine blocks") with a SciFi10_6
			// control-face graphic (gain 1.39, curation §5) on the working side.
			// Two at the checkpoint's north wall, three as the Site_AlarmRelay
			// bank — the relay finally has machinery to sabotage.
			// Art-review 25-07: the control faces read as noise — at TexWorldScale
			// 180 the SciFi10_6 panel graphic tiled ~1.5x across a 130-unit face,
			// so the console detail dissolved at command distance. Projection
			// 180 -> 450 (2.5x, the review's "2-3x") puts a bit over a quarter of
			// the sheet on each face: one readable console, not a pattern. Gain
			// stays the MEASURED 1.39 and the body stays dark (MachineLit
			// unchanged, review: "body donker houden") — the read comes from the
			// face plate and the selective readout below, not from lifting mass.
			UMaterialInstanceDynamic* MachineBodyMid = MakeDressMid(MachineLit, MachineShade, TEXT("/Game/Art/Textures/T_Metal046B_diff.T_Metal046B_diff"), 19.76f, 250.0f, 0.6f);
			UMaterialInstanceDynamic* MachineFaceMid = MakeDressMid(FLinearColor(0.300f, 0.325f, 0.377f), FLinearColor(0.098f, 0.107f, 0.169f), TEXT("/Game/Art/Imported/Textures/T_4k_SciFi10_6_BaseColor.T_4k_SciFi10_6_BaseColor"), 1.39f, 450.0f, 0.85f);
			// "Emissive selectief": ONE sodium readout strip per machine, on the
			// palette's Glow entry — the district's existing emissive authority
			// (the same tint the window strips and lamp heads carry), so the
			// accent introduces no new colour and no new MID family (15.5). Small
			// by design: a lit console edge, never a glowing appliance.
			UMaterialInstanceDynamic* ReadoutMid = MidForPalette(PaletteForLabel(TEXT("Glow")));
			struct FMachineDef { FVector BodyLoc; FVector BodyScale; FVector FaceLoc; FVector FaceScale; };
			const FMachineDef Machines[] = {
				{ FVector(4450.0f, -1080.0f, 68.0f), FVector(1.5f, 0.8f, 1.35f), FVector(4450.0f, -1039.0f, 75.0f), FVector(1.3f, 0.05f, 1.1f) },
				{ FVector(4640.0f, -1080.0f, 68.0f), FVector(1.5f, 0.8f, 1.35f), FVector(4640.0f, -1039.0f, 75.0f), FVector(1.3f, 0.05f, 1.1f) },
				{ FVector(5075.0f, 1350.0f, 68.0f), FVector(0.8f, 1.4f, 1.35f), FVector(5034.0f, 1350.0f, 75.0f), FVector(0.05f, 1.2f, 1.1f) },
				{ FVector(5075.0f, 1500.0f, 68.0f), FVector(0.8f, 1.4f, 1.35f), FVector(5034.0f, 1500.0f, 75.0f), FVector(0.05f, 1.2f, 1.1f) },
				{ FVector(5075.0f, 1650.0f, 68.0f), FVector(0.8f, 1.4f, 1.35f), FVector(5034.0f, 1650.0f, 75.0f), FVector(0.05f, 1.2f, 1.1f) },
			};
			for (const FMachineDef& Machine : Machines)
			{
				SpawnDress(CubeMesh, MachineBodyMid, Machine.BodyLoc, FRotator::ZeroRotator, Machine.BodyScale, TEXT("Deco_Machine"), true);
				SpawnDress(CubeMesh, MachineFaceMid, Machine.FaceLoc, FRotator::ZeroRotator, Machine.FaceScale, TEXT("Deco_Machine"), false);
				// Readout strip + blob shadow, both DERIVED from this row: the
				// working side is wherever the face already sits outside the body
				// (FaceLoc - BodyLoc), and the thin face axis says which way the
				// strip runs. No second coordinate list anywhere in the bank.
				const FVector FaceOut = Machine.FaceLoc - Machine.BodyLoc;
				const bool bFaceAlongX = Machine.FaceScale.X > Machine.FaceScale.Y;
				const FVector ReadoutOffset = bFaceAlongX
					? FVector(0.0f, FMath::Sign(FaceOut.Y) * 4.0f, 25.0f)
					: FVector(FMath::Sign(FaceOut.X) * 4.0f, 0.0f, 25.0f);
				const FVector ReadoutScale = bFaceAlongX ? FVector(0.60f, 0.04f, 0.09f) : FVector(0.04f, 0.55f, 0.09f);
				SpawnDress(CubeMesh, ReadoutMid, Machine.FaceLoc + ReadoutOffset, FRotator::ZeroRotator, ReadoutScale, TEXT("Deco_Machine"), false);
				// Grounding blob: the body's own footprint x1.25, so the ellipse
				// spills a quarter of the mass on every side. Neighbours in a
				// bank stand 10 units apart, so the overlap lands UNDER the
				// bodies where no camera sees the double darkening.
				SpawnGroundDecal(BlobMid, FVector2D(Machine.BodyLoc.X, Machine.BodyLoc.Y),
					FVector2D(Machine.BodyScale.X * 125.0f, Machine.BodyScale.Y * 125.0f), 0.0f, BlobDecalZ, TEXT("Deco_Blob"), BlobSortPriority);
			}

			// --- Cargo containers: ambientCG Metal063 rust-speckled steel
			// (gain 6.42 — curation §9 "containers"). Two stacked-yard units by
			// the dock approach, one askew at the cross-street choke; the
			// Dominion supply unit rides oxide red (palette hue, story tell).
			UMaterialInstanceDynamic* ContainerMid = MakeDressMid(GraphiteLit, GraphiteShade, TEXT("/Game/Art/Textures/T_Metal063_diff.T_Metal063_diff"), 6.42f, 300.0f, 0.75f);
			// Art-review 25-07 (shot 00058): at mix 0.75 Metal063's cool rust
			// chroma pushed the oxide tint to magenta — palette hue must win
			// (15.5), so the Dominion unit runs a lower texture mix; the next
			// shot round verifies the read.
			UMaterialInstanceDynamic* ContainerRedMid = MakeDressMid(OxideLit, OxideShade, TEXT("/Game/Art/Textures/T_Metal063_diff.T_Metal063_diff"), 6.42f, 300.0f, 0.45f);
			// One row per unit (was three hand-written spawns): the grounding blob
			// below has to read the SAME location, yaw and footprint the unit
			// spawns with, and a table is the only way that stays true after the
			// next nudge. All three share the measured 20-ft-container box.
			const FVector ContainerScale(6.1f, 2.44f, 2.6f);
			const struct { UMaterialInstanceDynamic* Mid; FVector Loc; float Yaw; } Containers[] = {
				{ ContainerMid, FVector(-2560.0f, 3500.0f, 130.0f), 90.0f },     // dock approach, stacked yard
				{ ContainerRedMid, FVector(-2290.0f, 3460.0f, 130.0f), 96.0f },  // Dominion supply unit (oxide)
				{ ContainerMid, FVector(-3450.0f, 1500.0f, 130.0f), 100.0f },    // askew at the cross-street choke
			};
			for (const auto& Container : Containers)
			{
				SpawnDress(CubeMesh, Container.Mid, Container.Loc, FRotator(0.0f, Container.Yaw, 0.0f), ContainerScale, TEXT("Deco_Container"), true);
				// Blob = the container's own footprint x1.25, turned with it.
				SpawnGroundDecal(BlobMid, FVector2D(Container.Loc.X, Container.Loc.Y),
					FVector2D(ContainerScale.X * 125.0f, ContainerScale.Y * 125.0f), Container.Yaw, BlobDecalZ, TEXT("Deco_Blob"), BlobSortPriority);
			}

			// --- Perimeter-wall variant: ambientCG Concrete042A shuttered
			// concrete (gain 8.94 — curation §9 "perimeter-wall variant,
			// bunkers"). Buttresses break the wall's unbroken band read (the
			// cam-4 perimeterband watch-item) + a low checkpoint bunker at the
			// gate approach. Positions clear the placed signs/stencils/strips.
			UMaterialInstanceDynamic* BunkerMid = MakeDressMid(GraphiteLit, GraphiteShade, TEXT("/Game/Art/Textures/T_Concrete042A_diff.T_Concrete042A_diff"), 8.94f, 400.0f, 0.6f);
			// Buttresses, three ways at once (code-review #4 + art-review "dieper
			// + donkerder — anders onzichtbaar"):
			//  1. FLUSH: the perimeter slabs' inner faces sit at ±9950 (centre
			//     ±10000, 100-unit slab), so at ±9890 the buttresses floated 10
			//     units off the wall. Flush is the invariant here, not the literal
			//     number the spec wrote for the old depth: with the deeper body
			//     (160 units) the back face lands on ±9950 at centre ±9870.
			//  2. DEEPER: 1.0 -> 1.6 on the protrusion axis. A 100-unit bump on a
			//     20 000-unit wall carried no silhouette offset at overview
			//     distance; 160 units breaks the band's straight edge.
			//  3. DARKER: its own MID one banked value step (x0.72 — the step the
			//     floor and the CoverB hierarchy use) under the Wall_ band, so the
			//     buttress reads as a mass in front of the wall instead of a
			//     same-value smear on it. Hue and the measured 8.94 gain unchanged
			//     (luminance-only, 15.5).
			UMaterialInstanceDynamic* ButtressMid = MakeDressMid(GraphiteLit * 0.72f, GraphiteShade * 0.72f, TEXT("/Game/Art/Textures/T_Concrete042A_diff.T_Concrete042A_diff"), 8.94f, 400.0f, 0.6f);
			const struct { FVector Loc; FVector Scale; } Buttresses[] = {
				{ FVector(-9870.0f, -800.0f, 170.0f), FVector(1.6f, 1.4f, 3.4f) },
				{ FVector(-9870.0f, 950.0f, 170.0f), FVector(1.6f, 1.4f, 3.4f) },
				{ FVector(-9870.0f, -3200.0f, 170.0f), FVector(1.6f, 1.4f, 3.4f) },
				{ FVector(-9870.0f, 5200.0f, 170.0f), FVector(1.6f, 1.4f, 3.4f) },
				{ FVector(-3000.0f, 9870.0f, 170.0f), FVector(1.4f, 1.6f, 3.4f) },
				{ FVector(1000.0f, 9870.0f, 170.0f), FVector(1.4f, 1.6f, 3.4f) },
			};
			for (const auto& Buttress : Buttresses)
			{
				SpawnDress(CubeMesh, ButtressMid, Buttress.Loc, FRotator::ZeroRotator, Buttress.Scale, TEXT("Deco_Bunker"), true);
			}
			// Checkpoint bunker: body + roof cap off ONE anchor (the cap sits on
			// the 110-unit body, hence +66 to its own centre), plus a grounding
			// blob sized off the cap — the widest part is what casts.
			const FVector BunkerLoc(-9350.0f, -750.0f, 55.0f);
			const FVector BunkerScale(2.2f, 1.6f, 1.1f);
			const FVector BunkerCapScale(2.6f, 2.0f, 0.22f);
			constexpr float BunkerYaw = 5.0f;
			SpawnDress(CubeMesh, BunkerMid, BunkerLoc, FRotator(0.0f, BunkerYaw, 0.0f), BunkerScale, TEXT("Deco_Bunker"), true);
			SpawnDress(CubeMesh, BunkerMid, BunkerLoc + FVector(0.0f, 0.0f, 66.0f), FRotator(0.0f, BunkerYaw, 0.0f), BunkerCapScale, TEXT("Deco_Bunker"), true);
			SpawnGroundDecal(BlobMid, FVector2D(BunkerLoc.X, BunkerLoc.Y),
				FVector2D(BunkerCapScale.X * 115.0f, BunkerCapScale.Y * 115.0f), BunkerYaw, BlobDecalZ, TEXT("Deco_Blob"), BlobSortPriority);

			UE_LOG(LogEclipse, Display, TEXT("Graybox: 15.8 dressing placed — SciFi10 slots 1/5/6/7/9 (+10 on the gantry), machine banks, containers, wall variant; rubble/dock only when their meshes loaded (warnings above otherwise)."));
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
		// Tag-scoped so authored ambient audio from a later art map survives; a
		// mid-play rebuild must not stack a second never-silent bed (review fix).
		for (TActorIterator<AAmbientSound> It(&World); It; ++It)
		{
			if (It->ActorHasTag(TEXT("Audio_AmbientBed"))) { Stale.Add(*It); }
		}
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
			// WELKE ZON DE ZON IS, met zoveel woorden. Het district heeft twee
			// directionele lampen (deze en de fill hieronder), en dan moet de
			// forward-shading weten welke de leidende is. Zonder die keuze pakt
			// de engine er zelf een "op algehele helderheid" en zet daar een
			// GELE WAARSCHUWING OVER HET SCHERM — gevonden op het eerste beeld
			// dat de UI meenam, en dus iets wat de owner tijdens het spelen
			// gewoon over zijn scherm zou zien lopen.
			//
			// De fallback koos hier waarschijnlijk goed, maar "waarschijnlijk
			// goed omdat hij toevallig helderder is" is geen keuze. Zodra iemand
			// de fill-intensiteit aanpast kantelt het beeld zonder dat iemand
			// iets aan de belichting deed.
			SunComponent->ForwardShadingPriority = 10;
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
			// Nadrukkelijk lager dan de zon: dit is tegenlicht, geen tweede zon.
			FillComponent->ForwardShadingPriority = 0;
		}
	}

	// Kessara smog: warm sodium haze with volumetrics so the sun shafts read.
	if (AExponentialHeightFog* Fog = World.SpawnActor<AExponentialHeightFog>(FVector(0, 0, -50), FRotator::ZeroRotator, Params))
	{
		if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
		{
			// Haze, not soup: the district must read across its full 200 m at
			// command distance (15.5); the smog hugs the ground via the falloff.
			// Dressing-iteratie 3 note — density 0.02 was TRIED here and REVERTED
			// (2026-07-25). It was aimed at a "ground brightens toward the horizon"
			// defect that turned out to be a measurement error: a probe rect at the
			// horizon band contained clipped non-floor pixels. A clean depth scan on
			// cam 4 (Tools/measure_frame_values.py, five rects from near to far)
			// reads 0.0489 / 0.0495 / 0.0553 / 0.0536 / 0.0523 linear — flat within
			// ~7%, not the 2x I had derived. Raising the density changed the
			// near-to-far ratio by 0.02x, i.e. nothing. The floor's aerial
			// perspective is therefore NOT fog-limited at this density, and the
			// value stays where it was measured to belong.
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

	// Layer-1 ambient bed (GDD 16.7/16.12): the Kessara industrial loop as the
	// district's never-silent sound floor, spawned with the graybox like the
	// lights and smog above. ONE flat 2D bed rather than attenuated emitters at
	// the industrial edges: the bed's contract is "audible everywhere, always",
	// and a 2D source makes that structural — no falloff radii to tune or
	// coverage holes to audit at this graybox tier, while positioned edge
	// emitters arrive later as 16.7 Layer-2 spot sources ON TOP of this floor.
	// The attenuation override below is load-bearing for that choice: the
	// imported cue carries ATT_Ambient_Bed baked in, and without the override
	// the bed would fall off around one point — silent corners, 16.7 broken.
	// Missing cue = one log line and a silent district, never a crash (14.3.5).
	if (USoundBase* AmbientBed = LoadObject<USoundBase>(nullptr, AmbientBedCuePath))
	{
		if (AAmbientSound* Bed = World.SpawnActor<AAmbientSound>(FVector(0, 0, 200), FRotator::ZeroRotator, Params))
		{
			if (UAudioComponent* BedComponent = Bed->GetAudioComponent())
			{
				BedComponent->SetSound(AmbientBed);
				BedComponent->SetVolumeMultiplier(AmbientBedVolume);
				BedComponent->bAllowSpatialization = false;
				BedComponent->bOverrideAttenuation = true;
				BedComponent->AttenuationOverrides.bAttenuate = false;
				BedComponent->AttenuationOverrides.bSpatialize = false;
				if (World.HasBegunPlay() && !BedComponent->IsPlaying())
				{
					// InitGame builds run pre-BeginPlay and ride the component's
					// auto-activate; a mid-play rebuild must kick playback itself.
					BedComponent->Play();
				}
			}
			Bed->Tags.Add(TEXT("Audio_AmbientBed"));
		}
	}
	else
	{
		UE_LOG(LogEclipse, Warning, TEXT("Graybox: ambient bed cue %s missing — district runs without its sound floor (GDD 14.3.5; run Tools/generate_audio_assets.py + import_generated_audio.py)."), AmbientBedCuePath);
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
		// Dressing-iteratie 3, step 1 — THE metering fix (art-review of shots
		// 00064-00069 measured it): the frames rendered palette albedo at x1.79
		// (floor mid authored 0.1198 -> 0.2145 linear on screen). With a free-
		// running histogram and a floor that fills 60-80% of every frame, that
		// floor's screen value is a FIXED POINT — the histogram drags the dominant
		// surface to mid-grey whatever we author. That is why iteration 2's dusk
		// retint changed nothing readable, and worse: it made the floor 16%
		// LIGHTER (Rec709 lum 0.1539 -> 0.1791) because it anchored to Wall_, the
		// brightest large surface, instead of to the sky. Until the key is pinned,
		// no colour decision on a dominant surface is art-directable.
		// -1.55 = -0.7 - log2(1.79): exactly the measured overshoot, not a guess.
		// Verify by re-shooting and measuring the floor back at 0.1198 +/-0.01
		// linear (Tools/measure_frame_values.py) — never by eye.
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = -1.55f;
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
