#include "Base/EclipseVaultBuilder.h"

#include "Base/EclipseBaseLogic.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Eclipse.h"
#include "Engine/ExponentialHeightFog.h"
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
	constexpr float SpineHalfW = 350.0f;
	constexpr float SlotSpacing = 1400.0f;
	// The three that the fit-outs and the room-reading probe BOTH depend on live
	// in the header now; these are the local names the shell code already used.
	constexpr float CeilH = EclipseVault::CeilingHeightCm;
	constexpr float ChamberW = EclipseVault::ChamberWidthCm;
	constexpr float ChamberD = EclipseVault::ChamberDepthCm;
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

	/**
	 * Same unlit-emissive luminance calibration as the district (see
	 * EclipseGrayboxBuilder). Deliberately NOT called ToonEmissiveScale: the
	 * district file has a constant by that exact name in ITS anonymous namespace,
	 * and UBT's unity build concatenates translation units — the two namespaces
	 * then merge and the differing storage class (const vs constexpr) is a hard
	 * redefinition error. It only surfaced when the adaptive-unity working set
	 * happened to pair these two files, which is why it landed on a green bar and
	 * would have broken the first clean/CI build instead.
	 */
	constexpr float VaultToonEmissiveScale = 10.0f;

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

		// --- fit-out families (the four rooms, GDD 5.3 "each level visibly
		// different: new machines, LIGHT, staff density") --------------------
		// The four rooms each get their OWN light colour, and that is the part
		// of 5.3 that is cheapest to skip and most expensive to miss: a room
		// you can name from the doorway before you read a label is a place, a
		// room you can only name from its tint is a menu entry with a floor.
		{ TEXT("VaultPipe"),   FLinearColor(0.140f, 0.160f, 0.190f), FLinearColor(0.046f, 0.056f, 0.092f), true },  // conduit / racking / masts - cold steel, one step under the shell
		{ TEXT("Bedding"),     FLinearColor(0.430f, 0.390f, 0.300f), FLinearColor(0.165f, 0.148f, 0.135f), false }, // canvas: mattresses and the laundry line (soft goods never block)
		{ TEXT("HoloGlow"),    FLinearColor(2.600f, 0.360f, 0.180f), FLinearColor(2.600f, 0.360f, 0.180f), false }, // Command: the stolen table's plate, Dominion oxide made light
		{ TEXT("EmberGlow"),   FLinearColor(2.300f, 0.780f, 0.150f), FLinearColor(2.300f, 0.780f, 0.150f), false }, // Barracks stove + the memorial votives + the geothermal grate
		{ TEXT("WeldGlow"),    FLinearColor(2.400f, 2.200f, 1.900f), FLinearColor(2.400f, 2.200f, 1.900f), false }, // Workshop: the near-white arc, the brightest thing in the vault
		{ TEXT("ScreenGlow"),  FLinearColor(0.220f, 1.400f, 2.400f), FLinearColor(0.220f, 1.400f, 2.400f), false }, // Intel: console readouts and the plot boards
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
	// The palette column is gone on purpose: a room is no longer ONE colour (see
	// the fit-outs below), so a single label per facility could only have been a
	// tint - and a tint is what made the four rooms interchangeable.
	struct FFacilityPresentationDef { FName FacilityId; const TCHAR* SiteTag; };
	const FFacilityPresentationDef* FindFacilityPresentation(FName FacilityId)
	{
		static const FFacilityPresentationDef Defs[] = {
			{ EclipseBaseDefaults::CommandCenterFacilityId, TEXT("Site_MapTable") },
			{ FName(TEXT("Barracks")),                      TEXT("Site_MusterBoard") },
			{ FName(TEXT("Workshop")),                      TEXT("Site_Workbench") },
			{ FName(TEXT("IntelligenceCenter")),            TEXT("Site_IntelConsole") },
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

	/**
	 * The room fit-out sink. Fit-outs are authored in ROOM-LOCAL coordinates so
	 * one number reads the same in a north chamber and a south one:
	 *   U    along the Spine; 0 = the chamber's centre line, +/-450 = the side walls
	 *   V    INTO the room from the door mouth; 0 = threshold, 900 = back wall
	 *   Z    world up; the deck is 0 and the ceiling is CeilH
	 *   Yaw  about Z in room space (the caller mirrors it for south chambers)
	 * The caller owns the mirroring, so no fit-out below ever mentions a side.
	 */
	using FRoomBoxFn = TFunctionRef<void(const TCHAR* /*Label*/, float /*U*/, float /*V*/, float /*Z*/,
		float /*SizeX*/, float /*SizeY*/, float /*SizeZ*/, float /*YawDeg*/)>;

	/**
	 * Two keep-clear rules every fit-out obeys, and they are load-bearing rather
	 * than tidy:
	 *  - THRESHOLD: the first 180 cm inside the mouth stays walkable across the
	 *    door width, so entering a room can never be a collision problem. The
	 *    walking round (Eclipse.Base.VaultWalkingRound) asserts on exactly this.
	 *  - ANNEX DOOR: a chamber with an annex behind it keeps |U| < 190 clear for
	 *    V > 700, or the fit-out walls in the memorial alcove / generator room.
	 * Nothing here is placed to fill a corner (GDD 20.2). Every box is a thing
	 * that is used: a surface worked at, a place slept in, stock, power, or the
	 * light by which one of those happens.
	 */
	constexpr float RoomThresholdV = 180.0f;
	constexpr float AnnexDoorHalfU = 190.0f;
	constexpr float AnnexDoorV = 700.0f;

	/**
	 * COMMAND CENTER - 5.2's stolen Dominion map table.
	 * The read from the doorway is ONE MASS IN THE MIDDLE that everything else
	 * faces: the only room in the vault whose centre is occupied and whose walls
	 * are comparatively bare. Light: a low red plate (Dominion oxide made light)
	 * plus two lamps hung directly over it, so the brightness is at TABLE height.
	 */
	void FitOutCommandCenter(int32 Level, FRoomBoxFn Room)
	{
		// The table: low, wide, standing height - rebels stand at it, there are
		// no chairs in Act 1 and that absence is the characterisation.
		Room(TEXT("FacCommand"), 0.0f, 470.0f, 45.0f, 380.0f, 260.0f, 90.0f, 0.0f);
		Room(TEXT("FacCommand"), 0.0f, 470.0f, 97.0f, 404.0f, 284.0f, 14.0f, 0.0f);   // overhanging top
		Room(TEXT("HoloGlow"),   0.0f, 470.0f, 108.0f, 300.0f, 190.0f, 8.0f, 0.0f);   // the plate that is switched on

		// Its two task lamps, directly over the table and nowhere else.
		Room(TEXT("HoloGlow"), -140.0f, 470.0f, CeilH - 55.0f, 70.0f, 70.0f, 16.0f, 0.0f);
		Room(TEXT("HoloGlow"),  140.0f, 470.0f, CeilH - 55.0f, 70.0f, 70.0f, 16.0f, 0.0f);

		// The chart boards the table argues with, flat on both side walls.
		for (const float WallSign : { -1.0f, 1.0f })
		{
			Room(TEXT("SurveyPost"), WallSign * 435.0f, 520.0f, 210.0f, 24.0f, 250.0f, 200.0f, 0.0f);
		}

		// The radio set: a map table without a way to send the order is a prop.
		Room(TEXT("FacCommand"), -330.0f, 250.0f, 70.0f, 130.0f, 130.0f, 140.0f, 0.0f);
		Room(TEXT("VaultPipe"),  -330.0f, 250.0f, 160.0f, 90.0f, 60.0f, 40.0f, 0.0f);
		// and the cable spool it draws from, with the run to the wall.
		Room(TEXT("VaultPipe"), 330.0f, 260.0f, 35.0f, 110.0f, 110.0f, 70.0f, 0.0f);
		Room(TEXT("VaultPipe"), 330.0f, 440.0f, 5.0f, 40.0f, 300.0f, 10.0f, 0.0f);

		if (Level >= 2)
		{
			// L2 = the map reaches further (5.3.1 gates map range on the CC), so
			// the second plotting station and a second lit plate, not a bigger box.
			Room(TEXT("FacCommand"), 300.0f, 640.0f, 45.0f, 220.0f, 180.0f, 90.0f, 20.0f);
			Room(TEXT("HoloGlow"),   300.0f, 640.0f, 96.0f, 170.0f, 130.0f, 8.0f, 20.0f);
			Room(TEXT("SurveyPost"), 0.0f, 880.0f, 230.0f, 420.0f, 20.0f, 240.0f, 0.0f);
		}
	}

	/**
	 * BARRACKS - where the eleven cages of 5.2 end up once they have a room.
	 * The read is the exact inverse of Command: the middle is EMPTY (it is an
	 * aisle) and both walls carry stacked horizontal mass at two heights. Light:
	 * one warm ember point at the back, low - people sleep here.
	 */
	void FitOutBarracks(int32 Level, bool bAnnexBehind, FRoomBoxFn Room)
	{
		// Eleven bunks, counted (5.2 says eleven, so this places eleven): three
		// two-tier stacks on one wall, two on the other, and one single.
		auto Bunk = [&Room](float U, float V, float Z, bool bUpper)
		{
			Room(TEXT("FacBarracks"), U, V, Z, 92.0f, 200.0f, 12.0f, 0.0f);          // the frame
			Room(TEXT("Bedding"),     U, V, Z + 19.0f, 82.0f, 186.0f, 24.0f, 0.0f);  // and what is on it
			if (!bUpper)
			{
				return;
			}
			// Only the stack's uprights, and only once per stack (they are drawn
			// with the upper tier so a single bunk does not grow a ladder).
			for (const float PostV : { -92.0f, 92.0f })
			{
				Room(TEXT("FacBarracks"), U, V + PostV, 115.0f, 14.0f, 14.0f, 230.0f, 0.0f);
			}
		};

		const float BunkU = 388.0f;
		for (const float StackV : { 260.0f, 480.0f, 700.0f })
		{
			Bunk(-BunkU, StackV, 45.0f, /*bUpper*/ false);
			Bunk(-BunkU, StackV, 175.0f, /*bUpper*/ true);
		}
		for (const float StackV : { 260.0f, 480.0f })
		{
			Bunk(BunkU, StackV, 45.0f, false);
			Bunk(BunkU, StackV, 175.0f, true);
		}
		Bunk(BunkU, 700.0f, 45.0f, false); // the eleventh, single - the room ran out of frames

		// A footlocker at the foot of every stack. Kit has to live somewhere and
		// this is the only storage a soldier owns.
		for (const float LockerV : { 260.0f, 480.0f, 700.0f })
		{
			Room(TEXT("VaultRubble"), -292.0f, LockerV, 24.0f, 76.0f, 76.0f, 48.0f, 0.0f);
		}
		for (const float LockerV : { 260.0f, 480.0f })
		{
			Room(TEXT("VaultRubble"), 292.0f, LockerV, 24.0f, 76.0f, 76.0f, 48.0f, 0.0f);
		}

		// The stove: heat, a kettle, and the only warm light in the room. Parked
		// off-centre so the annex door behind stays walkable.
		const float StoveU = bAnnexBehind ? -320.0f : -120.0f;
		Room(TEXT("VaultBlast"), StoveU, 790.0f, 60.0f, 104.0f, 104.0f, 120.0f, 0.0f);
		Room(TEXT("EmberGlow"),  StoveU, 790.0f, 124.0f, 64.0f, 64.0f, 14.0f, 0.0f);
		Room(TEXT("VaultPipe"),  StoveU, 790.0f, 275.0f, 34.0f, 34.0f, 290.0f, 0.0f);   // the flue
		Room(TEXT("VaultPipe"),  StoveU + 78.0f, 790.0f, 138.0f, 52.0f, 52.0f, 36.0f, 0.0f); // the kettle on it

		// The laundry line across the aisle, well over head height. Eleven people
		// living underground generate washing; this is where it goes.
		Room(TEXT("Bedding"), 0.0f, 380.0f, 300.0f, 640.0f, 6.0f, 6.0f, 0.0f);
		for (const float ClothU : { -160.0f, 0.0f, 150.0f })
		{
			Room(TEXT("Bedding"), ClothU, 380.0f, 258.0f, 72.0f, 8.0f, 80.0f, 0.0f);
		}

		if (Level >= 2)
		{
			// L2 = the room sleeps more, which is two more bunks and a second
			// stove - staff density is the point of the upgrade (5.3).
			Bunk(-BunkU, 700.0f, 305.0f, true);
			Bunk(BunkU, 700.0f, 175.0f, true);
			Room(TEXT("VaultBlast"), 300.0f, 790.0f, 60.0f, 104.0f, 104.0f, 120.0f, 0.0f);
			Room(TEXT("EmberGlow"),  300.0f, 790.0f, 124.0f, 64.0f, 64.0f, 14.0f, 0.0f);
		}
	}

	/**
	 * WORKSHOP - benches, stock, and a way to lift what you cannot carry.
	 * The read is the only room with MASS NEAR THE CEILING: a gantry beam and the
	 * hoist block hanging off it sit in the upper third, where every other room
	 * has nothing but air. Light: the near-white welding arc, the brightest and
	 * coldest point in the vault.
	 */
	void FitOutWorkshop(int32 Level, bool bAnnexBehind, FRoomBoxFn Room)
	{
		// Bench runs against the back wall, split around the generator-room door.
		// The split halves sit at U 325 and not 300: at 300 a 264-wide bench
		// reaches to within 168 cm of the centre line, and the keep-clear rule
		// wants 190. The builder's own guard caught that on the first run - which
		// is the entire reason the guard logs instead of trusting the author.
		for (const float BenchSign : { -1.0f, 1.0f })
		{
			Room(TEXT("FacWorkshop"), BenchSign * 325.0f, 800.0f, 47.0f, 230.0f, 94.0f, 94.0f, 0.0f);
			Room(TEXT("VaultPipe"),   BenchSign * 325.0f, 842.0f, 212.0f, 230.0f, 16.0f, 130.0f, 0.0f); // the tool wall over it
		}
		if (!bAnnexBehind)
		{
			// No door behind: the bench run is continuous, which is what a
			// workshop wants and what the annex costs slot C.
			Room(TEXT("FacWorkshop"), 0.0f, 800.0f, 47.0f, 264.0f, 94.0f, 94.0f, 0.0f);
			Room(TEXT("VaultPipe"),   0.0f, 842.0f, 212.0f, 264.0f, 16.0f, 130.0f, 0.0f);
		}

		// The parts rack: three loaded shelves and their uprights.
		for (int32 Shelf = 0; Shelf < 3; ++Shelf)
		{
			Room(TEXT("FacWorkshop"), -392.0f, 470.0f, 62.0f + Shelf * 92.0f, 104.0f, 420.0f, 16.0f, 0.0f);
		}
		for (const float UprightV : { 265.0f, 675.0f })
		{
			Room(TEXT("VaultPipe"), -392.0f, UprightV, 150.0f, 104.0f, 14.0f, 300.0f, 0.0f);
		}

		// The gantry. This is the room's signature and it is functional: the
		// hoist runs the length of the bay so an engine block reaches the bench.
		Room(TEXT("VaultPipe"),   140.0f, 440.0f, CeilH - 62.0f, 62.0f, 480.0f, 58.0f, 0.0f);
		Room(TEXT("VaultPipe"),   140.0f, 430.0f, 330.0f, 18.0f, 18.0f, 100.0f, 0.0f);   // the chain
		Room(TEXT("FacWorkshop"), 140.0f, 430.0f, 245.0f, 94.0f, 94.0f, 118.0f, 0.0f);   // and the block on it

		// The welding bay: bench, shield, and the arc.
		Room(TEXT("FacWorkshop"), 300.0f, 260.0f, 45.0f, 184.0f, 184.0f, 90.0f, 0.0f);
		Room(TEXT("WeldGlow"),    300.0f, 260.0f, 97.0f, 74.0f, 54.0f, 12.0f, 0.0f);
		Room(TEXT("VaultPipe"),   300.0f, 352.0f, 175.0f, 194.0f, 14.0f, 170.0f, 0.0f);  // the spark shield

		// What a bench produces: a swarf bin and stock ends leaning on the wall.
		Room(TEXT("VaultRubble"), -150.0f, 260.0f, 36.0f, 92.0f, 92.0f, 72.0f, 0.0f);
		Room(TEXT("VaultPipe"),    404.0f, 620.0f, 92.0f, 44.0f, 44.0f, 184.0f, 14.0f);
		Room(TEXT("VaultPipe"),    386.0f, 640.0f, 86.0f, 40.0f, 40.0f, 172.0f, -11.0f);

		if (Level >= 2)
		{
			// L2 = the fabricator (5.3.1's Workshop tier), a lit rack, and a
			// second arc. New MACHINES, not a bigger version of the same one.
			Room(TEXT("FacWorkshop"), -160.0f, 560.0f, 110.0f, 190.0f, 190.0f, 220.0f, 0.0f);
			Room(TEXT("WeldGlow"),    -160.0f, 465.0f, 150.0f, 130.0f, 12.0f, 70.0f, 0.0f);
			Room(TEXT("VaultPipe"),   -160.0f, 560.0f, 300.0f, 60.0f, 60.0f, 160.0f, 0.0f);  // its extraction duct
			Room(TEXT("WeldGlow"),     300.0f, 690.0f, 236.0f, 150.0f, 30.0f, 12.0f, 0.0f);  // the bench light that came with it
		}
	}

	/**
	 * INTELLIGENCE CENTER - listening, which is a posture before it is a machine.
	 * The read is the only CURVED arrangement in a vault of right angles: five
	 * stations bowed toward one wall of plot boards, with a mast in the corner
	 * and cable runs poured across the floor. Light: cyan, and all of it on the
	 * far wall, so the room reads dark at the door and bright at the back.
	 */
	void FitOutIntelligenceCenter(int32 Level, FRoomBoxFn Room)
	{
		// The console bow. Centre of curvature sits at the door side, so the
		// stations splay back toward you - a cockpit, not a row of desks.
		const int32 Stations = Level >= 2 ? 7 : 5;
		const float Spread = Level >= 2 ? 22.0f : 26.0f;
		for (int32 Station = 0; Station < Stations; ++Station)
		{
			const float Angle = (Station - (Stations - 1) * 0.5f) * Spread;
			const float Radians = FMath::DegreesToRadians(Angle);
			const float U = FMath::Sin(Radians) * 300.0f;
			const float V = 380.0f + FMath::Cos(Radians) * 300.0f;
			Room(TEXT("FacIntel"),    U, V, 48.0f, 150.0f, 96.0f, 96.0f, Angle);
			Room(TEXT("ScreenGlow"),  U, V + 4.0f, 119.0f, 122.0f, 16.0f, 46.0f, Angle);  // its readout
			Room(TEXT("FacIntel"),    U * 0.78f, V - 96.0f, 22.0f, 48.0f, 48.0f, 44.0f, Angle); // the stool at it
		}

		// The wall they all face. Four plot boards, each with its lit band.
		for (int32 Board = 0; Board < 4; ++Board)
		{
			const float U = -270.0f + Board * 180.0f;
			Room(TEXT("VaultPipe"),  U, 882.0f, 250.0f, 164.0f, 16.0f, 124.0f, 0.0f);
			Room(TEXT("ScreenGlow"), U, 866.0f, 250.0f, 142.0f, 10.0f, 104.0f, 0.0f);
		}

		// The mast the dish feed comes down, and the runs that carry it forward.
		// Cables on the floor are the room's only floor decoration and they point
		// at where the signal goes, which is the whole point of the place.
		Room(TEXT("VaultPipe"), -392.0f, 810.0f, CeilH * 0.5f, 72.0f, 72.0f, CeilH, 0.0f);
		Room(TEXT("FacIntel"),  -392.0f, 810.0f, 332.0f, 232.0f, 42.0f, 228.0f, 34.0f);   // the dish on it
		for (int32 Run = 0; Run < 4; ++Run)
		{
			Room(TEXT("VaultPipe"), -330.0f + Run * 155.0f, 600.0f, 4.0f, 26.0f, 330.0f, 8.0f, 0.0f);
		}

		if (Level >= 2)
		{
			// L2 = a second feed (5.3.1 widens Intel's reach), so a second dish
			// on the opposite corner and the run that ties it in.
			Room(TEXT("VaultPipe"), 392.0f, 810.0f, CeilH * 0.5f, 72.0f, 72.0f, CeilH, 0.0f);
			Room(TEXT("FacIntel"),  392.0f, 810.0f, 332.0f, 232.0f, 42.0f, 228.0f, -34.0f);
			Room(TEXT("VaultPipe"), 392.0f, 560.0f, 4.0f, 26.0f, 420.0f, 8.0f, 0.0f);
		}
	}

	/**
	 * Dispatch. An unknown facility id (a Phase 3 Medbay in an old save) gets the
	 * neutral operational blockout it always had - graceful, never a crash and
	 * never a room that pretends to be something it has no fit-out for
	 * (GDD 14.3.5).
	 */
	void EmitFacilityFitOut(FName FacilityId, int32 Level, bool bAnnexBehind, FRoomBoxFn Room)
	{
		if (FacilityId == EclipseBaseDefaults::CommandCenterFacilityId)
		{
			FitOutCommandCenter(Level, Room);
		}
		else if (FacilityId == FName(TEXT("Barracks")))
		{
			FitOutBarracks(Level, bAnnexBehind, Room);
		}
		else if (FacilityId == FName(TEXT("Workshop")))
		{
			FitOutWorkshop(Level, bAnnexBehind, Room);
		}
		else if (FacilityId == FName(TEXT("IntelligenceCenter")))
		{
			FitOutIntelligenceCenter(Level, Room);
		}
		else
		{
			// The pre-fit-out blockout, kept for exactly this case.
			Room(TEXT("VaultDefault"), 0.0f, 450.0f, 55.0f, 300.0f, 200.0f, 110.0f, 0.0f);
			Room(TEXT("VaultDefault"), 0.0f, 450.0f, 130.0f, 260.0f, 160.0f, 20.0f, 0.0f);
			Room(TEXT("GlowStrip"), 0.0f, 450.0f, CeilH - 60.0f, 200.0f, 30.0f, 20.0f, 0.0f);
		}
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

	// ZICHTBAAR MAKEN, en dit is geen detail: ATargetPoint zet zichzelf in zijn
	// constructor op hidden (het is een markeringsactor, niet iets om te tekenen),
	// en een verborgen actor tekent zijn CHILD-componenten ook niet. Alle
	// instanced meshes van de kluis hangen aan dit anker, dus de hele vault stond
	// er wel - beloopbaar, met kloppende botsing en kloppende tags - en werd nooit
	// één keer getekend.
	//
	// Waarom niemand het zag, en dat is precies de les van EXECUTION_PLAN §1b nog
	// een keer: de pariteits-Gauntlet leest TAGS en TRANSFORMS, en die klopten
	// allemaal. Het instrument beantwoordde "staat het er?" terwijl de vraag "zie
	// je het?" was. Gevonden door er een camera in te zetten en te merken dat er
	// lucht en zwart uit kwam - niet door een test.
	Anchor->SetActorHiddenInGame(false);

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
				Mid->SetScalarParameterValue(TEXT("EmissiveScale"), VaultToonEmissiveScale);
				// GEEN AlbedoMix hier, en dat is een gemeten beslissing en geen
				// vergeten regel. De eerste zichtbare frames lieten een grof
				// dambordpatroon zien en dat leek een niet-gezette albedotextuur.
				// Nagemeten in author_toon_material.py: AlbedoMix is standaard 0
				// (`col *= lerp(1.0, varL, saturate(AlbedoMix))` doet dan niets),
				// dus de textuur kan het niet zijn. Het patroon is de HATCH van de
				// master zelf - `step(0.75, frac((x+y)/HatchScale))` met HatchScale
				// 120 en HatchStrength 0.22 - de pennenstreken in de schaduwband.
				// Het district zet die evenmin: dit IS de huisstijl, op precies
				// dezelfde waarden. Er valt hier dus niets te repareren, en een
				// expliciete 0 zetten zou een reparatie suggereren die er geen is.
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

	// Box instance: local center (vault space) + full size in cm + yaw about Z.
	// Yaw exists for exactly one reason: the Intelligence Center's console bow is
	// the only curved arrangement in the vault, and a curve faked out of
	// axis-aligned boxes reads as a staircase.
	auto AddBoxYaw = [&](const TCHAR* Label, const FVector& LocalCenter, const FVector& Size, float YawDeg)
	{
		if (UInstancedStaticMeshComponent* Ism = IsmForLabel(Label))
		{
			Ism->AddInstance(FTransform(FRotator(0.0f, YawDeg, 0.0f).Quaternion(), VaultOrigin + LocalCenter, Size / 100.0f), /*bWorldSpace*/ true);
		}
	};
	auto AddBox = [&](const TCHAR* Label, const FVector& LocalCenter, const FVector& Size)
	{
		AddBoxYaw(Label, LocalCenter, Size, 0.0f);
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
				// A bench to sit on and three votives. That is the entire fit-out
				// and it stays that way: this is the one room in Hollow Point
				// that must never look like it was BOUGHT (5.3.2 - grief is never
				// a purchase), so it gets a place to sit and a light, not a rack.
				AddBox(TEXT("VaultRubble"), FVector(Local.X, AnnexMidY - S * 120.0f, 22.0f), FVector(AnnexW * 0.55f, 70.0f, 44.0f));
				for (const float VotiveU : { -110.0f, 0.0f, 110.0f })
				{
					AddBox(TEXT("EmberGlow"), FVector(Local.X + VotiveU, AnnexBackY - S * (WallThk * 0.5f + 70.0f), 24.0f), FVector(34.0f, 34.0f, 48.0f));
				}
				AddMarker(FVector(Local.X, AnnexMidY, 0), { FName(TEXT("Site_Memorial")), Plan.SlotId });
			}
			if (Plan.bHasGeneratorAnnex)
			{
				// The geothermal generator that coughs (5.2). No Energy meter in
				// Phase 2 (locked decision 5) - PLACEHOLDER(GDD 6.2 Energy) - so
				// everything here is ambient story, and it earns that by reading
				// as plumbing under load rather than as a box with a light on it:
				// a drum, the manifold you would actually turn, the risers that
				// take the heat out, and the grate it draws from.
				AddBox(TEXT("VaultBlast"), FVector(Local.X, AnnexMidY, 110.0f), FVector(320.0f, 260.0f, 220.0f));
				AddBox(TEXT("VaultBlast"), FVector(Local.X, AnnexMidY, 245.0f), FVector(232.0f, 190.0f, 60.0f));
				AddBox(TEXT("GlowStrip"), FVector(Local.X, AnnexMidY, 288.0f), FVector(120.0f, 60.0f, 18.0f));
				AddBox(TEXT("VaultPipe"), FVector(Local.X, AnnexMidY - S * 148.0f, 150.0f), FVector(300.0f, 42.0f, 42.0f));
				for (const float WheelU : { -100.0f, 0.0f, 100.0f })
				{
					AddBox(TEXT("VaultPipe"), FVector(Local.X + WheelU, AnnexMidY - S * 178.0f, 150.0f), FVector(52.0f, 22.0f, 52.0f));
				}
				for (const float RiserU : { -192.0f, 192.0f })
				{
					AddBox(TEXT("VaultPipe"), FVector(Local.X + RiserU, AnnexMidY, (CeilH + 220.0f) * 0.5f), FVector(46.0f, 46.0f, CeilH - 220.0f));
				}
				AddBox(TEXT("EmberGlow"), FVector(Local.X, AnnexMidY + S * 195.0f, 6.0f), FVector(260.0f, 110.0f, 12.0f));
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

		// Presentation now only supplies the interaction-point tag: the room's
		// colours moved into the fit-outs, where each family can pick more than
		// one (a Workshop is amber benches AND cold steel racking AND a white
		// arc, and collapsing that to a single tint is how the four rooms became
		// indistinguishable in the first place).
		const FFacilityPresentationDef* Presentation = Plan.FacilityId.IsNone() ? nullptr : FindFacilityPresentation(Plan.FacilityId);

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
			// The room fit-out. This is where a slot stops being a coloured box
			// and becomes a place: the four Act 1 facilities each get their own
			// furniture, their own silhouette and their own light colour, so the
			// room can be named from the doorway before any label is read
			// (GDD 5.3; the falsification is Eclipse.Base.VaultRoomsAreTellable).
			//
			// The mouth is the room's origin: V measures INTO the room, and the
			// mirroring for a south chamber happens here and nowhere else, so no
			// fit-out has to know which side of the Spine it woke up on.
			const float MouthY = S * (SpineHalfW + WallThk);
			auto RoomBox = [&](const TCHAR* Label, float U, float V, float Z, float SizeX, float SizeY, float SizeZ, float YawDeg)
			{
				// The two keep-clear rules, enforced instead of remembered. A
				// fit-out that walls in its own doorway is a bug that only shows
				// up as "the player got stuck", which is the most expensive way
				// to find it (GDD 14.3.5: loud, never silent).
				const bool bBlocksThreshold = V - SizeY * 0.5f < RoomThresholdV
					&& FMath::Abs(U) - SizeX * 0.5f < DoorW * 0.5f
					&& Z - SizeZ * 0.5f < 200.0f;
				const bool bBlocksAnnexDoor = bAnnex
					&& V + SizeY * 0.5f > AnnexDoorV
					&& FMath::Abs(U) - SizeX * 0.5f < AnnexDoorHalfU
					&& Z - SizeZ * 0.5f < 200.0f;
				if (bBlocksThreshold || bBlocksAnnexDoor)
				{
					UE_LOG(LogEclipse, Error, TEXT("Vault fit-out %s in %s blocks the %s doorway (U %.0f V %.0f Z %.0f) — placed anyway so the frame shows it, but this is a defect."),
						Label, *Plan.SlotId.ToString(), bBlocksThreshold ? TEXT("chamber") : TEXT("annex"), U, V, Z);
				}
				AddBoxYaw(Label, FVector(Local.X + U, MouthY + S * V, Z), FVector(SizeX, SizeY, SizeZ), S > 0.0f ? YawDeg : -YawDeg);
			};
			EmitFacilityFitOut(Plan.FacilityId, Plan.Level, bAnnex, RoomBox);

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

TArray<FEclipseVaultPlacedBox> ReadPlacedBoxes(UWorld& World)
{
	TArray<FEclipseVaultPlacedBox> Boxes;
	for (TActorIterator<ATargetPoint> It(&World); It; ++It)
	{
		if (!It->ActorHasTag(VaultAnchorTag))
		{
			continue;
		}
		TArray<UInstancedStaticMeshComponent*> Isms;
		It->GetComponents<UInstancedStaticMeshComponent>(Isms);
		for (const UInstancedStaticMeshComponent* Ism : Isms)
		{
			// The component name carries the palette family (VaultIsm_<Label>);
			// that is the same string the material was keyed on, so the read-back
			// cannot drift from what was placed.
			FString Palette = Ism->GetName();
			Palette.RemoveFromStart(TEXT("VaultIsm_"));
			for (int32 Index = 0; Index < Ism->GetInstanceCount(); ++Index)
			{
				FEclipseVaultPlacedBox& Box = Boxes.AddDefaulted_GetRef();
				Box.Palette = FName(*Palette);
				Ism->GetInstanceTransform(Index, Box.Transform, /*bWorldSpace*/ true);
			}
		}
	}
	// Sorted on content, never on spawn order: two builds of the same state must
	// hand back the same array (the determinism contract, one layer down).
	Boxes.Sort([](const FEclipseVaultPlacedBox& A, const FEclipseVaultPlacedBox& B)
	{
		if (A.Palette != B.Palette)
		{
			return A.Palette.LexicalLess(B.Palette);
		}
		const FVector LA = A.Transform.GetLocation();
		const FVector LB = B.Transform.GetLocation();
		if (LA.X != LB.X) { return LA.X < LB.X; }
		if (LA.Y != LB.Y) { return LA.Y < LB.Y; }
		return LA.Z < LB.Z;
	});
	return Boxes;
}

int32 SetSurfaceFogSuppressed(UWorld& World, bool bSuppressed, float& InOutPreviousDensity)
{
	int32 Touched = 0;
	for (TActorIterator<AExponentialHeightFog> It(&World); It; ++It)
	{
		UExponentialHeightFogComponent* Fog = It->GetComponent();
		if (Fog == nullptr)
		{
			continue;
		}
		if (bSuppressed)
		{
			// Remember the FIRST one's density; the district spawns exactly one
			// and the graybox builder sweeps strays, so one value is the truth.
			if (Touched == 0)
			{
				InOutPreviousDensity = Fog->FogDensity;
			}
			Fog->SetFogDensity(0.0f);
		}
		else
		{
			Fog->SetFogDensity(InOutPreviousDensity);
		}
		++Touched;
	}
	UE_LOG(LogEclipse, Display, TEXT("Vault: surface fog %s on %d actor(s) (density %.4f) — the vault is 150 m under a surface-authored height fog; STOPGAP until it is its own level."),
		bSuppressed ? TEXT("suppressed") : TEXT("restored"), Touched, InOutPreviousDensity);
	return Touched;
}

bool FindVaultPoint(UWorld& World, FName Tag, FVector& OutLocation)
{
	for (TActorIterator<ATargetPoint> It(&World); It; ++It)
	{
		if (It->ActorHasTag(VaultTag) && It->ActorHasTag(Tag))
		{
			OutLocation = It->GetActorLocation();
			return true;
		}
	}
	return false;
}

FEclipseBaseState MakeReviewState(TConstArrayView<FEclipseBaseSlotDef> Slots)
{
	// Built through the SAME pure core the campaign uses (StartConstruction +
	// ApplyRush), never by writing the struct fields by hand. A review state
	// that could not have been reached by playing would show rooms the player
	// can never stand in - which is the exact failure a review frame is for.
	FEclipseBaseState State; // seeded with the pre-built Command Center at Slot_A
	for (const FEclipseBaseSlotDef& Slot : Slots)
	{
		if (Slot.AllowedFacilityRows.Num() == 0)
		{
			continue;
		}
		const FName FacilityId = Slot.AllowedFacilityRows[0];
		const FEclipseFacilityState* Existing = State.FindBySlot(Slot.SlotId);
		if (Existing != nullptr && Existing->Level >= 1)
		{
			continue; // Slot_A is already operational
		}
		EclipseBaseLogic::StartConstruction(State, Slot.SlotId, FacilityId, /*BuildDays*/ 1);
		if (FEclipseFacilityState* Started = State.FindBySlot(Slot.SlotId))
		{
			EclipseBaseLogic::ApplyRush(*Started);
		}
		// The Workshop is the only slice facility with an L2 (locked decision:
		// no L2/L3 for anything else), so it is the only room that has to prove
		// "each level visibly different" - and it does that here.
		if (FacilityId == FName(TEXT("Workshop")))
		{
			EclipseBaseLogic::StartConstruction(State, Slot.SlotId, FacilityId, /*BuildDays*/ 1);
			if (FEclipseFacilityState* Upgrading = State.FindBySlot(Slot.SlotId))
			{
				EclipseBaseLogic::ApplyRush(*Upgrading);
			}
		}
	}
	return State;
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
