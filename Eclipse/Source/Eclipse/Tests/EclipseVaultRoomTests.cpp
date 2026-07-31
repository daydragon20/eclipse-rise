// DE TWEE FALSIFICATIES DIE DE VAULT-SHELLS AFSLUITEN (EXECUTION_PLAN §2d).
//
// §2d formuleert ze in twee helften, en die helften meten met opzet iets anders:
//
//   1. "Een ronde loopt van faciliteit naar faciliteit zonder door geometrie te
//      vallen."  -> Eclipse.Base.VaultWalkingRound
//   2. "De vier faciliteiten zijn in het frame te onderscheiden."
//      -> Eclipse.Base.VaultRoomsAreTellable
//
// De eerste is de makkelijke helft: een echt personage, echte invoer, echte
// botsing, en een assert op de Z per TICK. De tweede is de moeilijke, want
// "onderscheidbaar" is geen getal tenzij je opschrijft hoe je het vaststelt.
//
// HOE DE TWEEDE MEET, in één alinea, want als ik dat niet kan uitleggen is het
// geen meting: hij zet een camera in de deuropening van elke kamer, schiet door
// elke beeldcel een straal, en bewaart per cel de AFSTAND tot het eerste vlak
// dat hij raakt. Dat is een echte diepte-opname van 40x22 met correcte
// occlusie - geen plan, geen inventarislijst, maar wat een camera op die plek
// tegenkomt. Daarna neemt hij van elke kamer een TWEEDE opname vanuit een
// standpunt dat de eerste nooit gezien heeft, en vraagt: welke van de vier
// deuropening-opnames lijkt hier het meest op? Zijn de vier kamers echt
// verschillende plekken, dan wijst elke tweede opname zijn eigen kamer aan.
//
// Twee dingen die de meting expres NIET mag gebruiken:
//   - KLEUR. De diepte-vingerafdruk kent geen palet. Vier identieke kamers in
//     vier tinten moeten zakken, want dat is precies wat er stond.
//   - LINKS/RECHTS. Kamers wisselen per index van kant en de bouwer spiegelt ze;
//     de opname van een zuidkamer wordt daarom horizontaal teruggespiegeld. Zo
//     kan "hij ligt aan de andere kant van de gang" geen gratis onderscheid
//     opleveren. Dat maakt de test strenger, niet soepeler.
//
// EN DE CONTROLEPROEF STAAT VOOROP, niet achteraan (owner-regel 31-07: bewijs
// eerst dat je meting een NIET-onderscheidbare ruimte afkeurt). Er draaien twee
// negatieve controles vóór de echte meting, en allebei door dezelfde bouwer:
//   A. vier identieke kamers zonder annexen -> de meting MOET falen (<= 1 van 4)
//   B. de echte plattegrond, maar met de generieke blokkendoos in alle vier de
//      kamers - letterlijk wat deze bouwer tot vanavond opleverde -> de meting
//      mag NIET alle vier goed hebben.
// Pas als die twee rood zijn, betekent een groene uitslag op de echte kamers
// iets. Een meting die nooit rood wordt, meet niets.

#if WITH_DEV_AUTOMATION_TESTS

#include "Base/EclipseBaseLogic.h"
#include "Base/EclipseBaseTypes.h"
#include "Base/EclipseVaultBuilder.h"
#include "Characters/EclipseCharacter.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "Strategy/EclipseCampaignTypes.h"
#include "Tests/EclipseFeelHarness.h"

namespace EclipseVaultRoomTest
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext | EAutomationTestFlags::CommandletContext
		| EAutomationTestFlags::ProductFilter;

	/** De echte Act 1-plattegrond uit DA_BaseLayout_HollowPoint (setup_base_data.py). */
	TArray<FEclipseBaseSlotDef> MakeHollowPointSlots()
	{
		TArray<FEclipseBaseSlotDef> Slots;

		FEclipseBaseSlotDef& A = Slots.AddDefaulted_GetRef();
		A.SlotId = EclipseBaseDefaults::CommandSlotId;
		A.AllowedFacilityRows = { EclipseBaseDefaults::CommandCenterFacilityId };
		A.AdjacentSlotIds = { EclipseVault::SpineNodeId };

		FEclipseBaseSlotDef& B = Slots.AddDefaulted_GetRef();
		B.SlotId = TEXT("Slot_B");
		B.AllowedFacilityRows = { TEXT("Barracks") };
		B.AdjacentSlotIds = { EclipseVault::SpineNodeId, EclipseVault::MemorialNodeId };
		B.EmptyStateStreamingId = TEXT("HP_SlotB_BunkCamp");

		FEclipseBaseSlotDef& C = Slots.AddDefaulted_GetRef();
		C.SlotId = TEXT("Slot_C");
		C.AllowedFacilityRows = { TEXT("Workshop") };
		C.AdjacentSlotIds = { EclipseVault::SpineNodeId, EclipseVault::GeneratorNodeId };

		FEclipseBaseSlotDef& D = Slots.AddDefaulted_GetRef();
		D.SlotId = TEXT("Slot_D");
		D.AllowedFacilityRows = { TEXT("IntelligenceCenter") };
		D.AdjacentSlotIds = { EclipseVault::SpineNodeId };

		return Slots;
	}

	// ---------------------------------------------------------------------
	// De diepte-opname
	// ---------------------------------------------------------------------

	constexpr int32 GridW = 40;
	constexpr int32 GridH = 22;
	constexpr int32 GridCells = GridW * GridH;
	/** Diepte-emmers: 30 stappen van 60 cm (0-18 m) plus 31 = "niets geraakt". */
	constexpr uint8 MissBucket = 31;
	constexpr float BucketCm = 60.0f;
	constexpr float HorizontalFovDeg = 90.0f;

	struct FDepthFrame
	{
		uint8 Cells[GridCells] = {};
	};

	/**
	 * Straal tegen één geplaatste doos. De instances staan op de engine-kubus,
	 * die in mesh-ruimte van -50 tot +50 loopt; de schaal in de transform is
	 * Size/100. Positie én richting gaan door dezelfde inverse transform, dus de
	 * gevonden t is dezelfde parameter als in wereldruimte - dat is precies
	 * waarom de diepte in centimeters klopt zonder herschaling.
	 */
	bool RayHitsBox(const FTransform& Xf, const FVector& Origin, const FVector& Dir, float& OutT)
	{
		const FVector LocalOrigin = Xf.InverseTransformPosition(Origin);
		const FVector LocalDir = Xf.InverseTransformVector(Dir);
		double TMin = 0.0;
		double TMax = 1.0e9;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			const double O = LocalOrigin[Axis];
			const double D = LocalDir[Axis];
			if (FMath::Abs(D) < 1.0e-9)
			{
				if (O < -50.0 || O > 50.0)
				{
					return false;
				}
				continue;
			}
			double T0 = (-50.0 - O) / D;
			double T1 = (50.0 - O) / D;
			if (T0 > T1)
			{
				Swap(T0, T1);
			}
			TMin = FMath::Max(TMin, T0);
			TMax = FMath::Min(TMax, T1);
			if (TMin > TMax)
			{
				return false;
			}
		}
		OutT = static_cast<float>(TMin);
		return OutT > 1.0f;
	}

	/**
	 * Eén opname. bMirror spiegelt de kolommen, zodat een zuidkamer en een
	 * noordkamer met dezelfde inrichting dezelfde vingerafdruk opleveren en
	 * "welke kant van de gang" geen onderscheid kan zijn.
	 */
	FDepthFrame RenderDepth(const TArray<EclipseVault::FEclipseVaultPlacedBox>& Boxes,
		const FVector& Eye, const FVector& LookAt, bool bMirror)
	{
		FDepthFrame Frame;
		const FMatrix Basis = FRotationMatrix((LookAt - Eye).Rotation());
		const FVector Fwd = Basis.GetUnitAxis(EAxis::X);
		const FVector Right = Basis.GetUnitAxis(EAxis::Y);
		const FVector Up = Basis.GetUnitAxis(EAxis::Z);
		const float TanH = FMath::Tan(FMath::DegreesToRadians(HorizontalFovDeg * 0.5f));
		const float TanV = TanH * static_cast<float>(GridH) / static_cast<float>(GridW);

		for (int32 Row = 0; Row < GridH; ++Row)
		{
			for (int32 Col = 0; Col < GridW; ++Col)
			{
				const float ScreenX = ((Col + 0.5f) / GridW) * 2.0f - 1.0f;
				const float ScreenY = 1.0f - ((Row + 0.5f) / GridH) * 2.0f;
				const FVector Dir = (Fwd + Right * (ScreenX * TanH) + Up * (ScreenY * TanV)).GetSafeNormal();

				float Nearest = TNumericLimits<float>::Max();
				for (const EclipseVault::FEclipseVaultPlacedBox& Box : Boxes)
				{
					float T = 0.0f;
					if (RayHitsBox(Box.Transform, Eye, Dir, T) && T < Nearest)
					{
						Nearest = T;
					}
				}
				const uint8 Bucket = Nearest == TNumericLimits<float>::Max()
					? MissBucket
					: static_cast<uint8>(FMath::Clamp(FMath::FloorToInt(Nearest / BucketCm), 0, MissBucket - 1));
				const int32 WriteCol = bMirror ? (GridW - 1 - Col) : Col;
				Frame.Cells[Row * GridW + WriteCol] = Bucket;
			}
		}
		return Frame;
	}

	/** Gemiddeld absoluut emmerverschil, genormaliseerd op 0..1. 0 = identiek beeld. */
	float FrameDistance(const FDepthFrame& A, const FDepthFrame& B)
	{
		int32 Sum = 0;
		for (int32 Cell = 0; Cell < GridCells; ++Cell)
		{
			Sum += FMath::Abs(static_cast<int32>(A.Cells[Cell]) - static_cast<int32>(B.Cells[Cell]));
		}
		return static_cast<float>(Sum) / (GridCells * static_cast<float>(MissBucket));
	}

	/**
	 * DE TWEE OPNAMESTATIONS, en waarom het er twee zijn: één camera die toevallig
	 * goed uitkomt is geen meting. Station 0 staat in de gang, in de deuropening,
	 * en kijkt recht naar binnen - het beeld dat een speler krijgt als hij
	 * langsloopt. Station 1 staat één stap naar binnen, opzij van de hartlijn en
	 * lager, en kijkt naar het midden van de kamer. Allebei staan ze in de strook
	 * die de bouwer in ELKE kamer gegarandeerd vrijhoudt (het keep-clear-contract
	 * in EclipseVaultBuilder), zodat de lens nooit tegen een meubel aan komt te
	 * staan en per kamer iets anders meet. Het criterium hieronder moet op
	 * ALLEBEI de stations gehaald worden.
	 */
	void StationCamera(const EclipseVault::FEclipseVaultSlotPlan& Plan, int32 Station,
		FVector& OutEye, FVector& OutLook, float& OutSide)
	{
		const float Side = Plan.bNorthSide ? 1.0f : -1.0f;
		const float MouthY = Plan.ChamberCenter.Y - Side * EclipseVault::ChamberDepthCm * 0.5f;
		const float FloorZ = Plan.ChamberCenter.Z;
		OutSide = Side;
		if (Station == 0)
		{
			OutEye = FVector(Plan.ChamberCenter.X, MouthY - Side * 40.0f, FloorZ + 170.0f);
			OutLook = FVector(Plan.ChamberCenter.X, MouthY + Side * 600.0f, FloorZ + 140.0f);
		}
		else
		{
			OutEye = FVector(Plan.ChamberCenter.X + 150.0f, MouthY + Side * 120.0f, FloorZ + 150.0f);
			OutLook = FVector(Plan.ChamberCenter.X, MouthY + Side * 620.0f, FloorZ + 110.0f);
		}
	}

	/**
	 * Een handvol kleine camerabewegingen: een stap opzij, een stap naar voren,
	 * een hoofd dat hoger of lager staat. Dit is de RUIS van het instrument -
	 * hoeveel een opname van dezelfde kamer al verandert als de camera een beetje
	 * beweegt.
	 */
	struct FJitter { float dU; float dV; float dZ; float LookDU; float LookDZ; };
	const FJitter Jitters[] = {
		{  30.0f,   0.0f,   0.0f,   0.0f,   0.0f },
		{ -30.0f,   0.0f,   0.0f,   0.0f,   0.0f },
		{   0.0f,  40.0f,  15.0f,  70.0f,   0.0f },
		{   0.0f, -25.0f, -18.0f, -70.0f,  30.0f },
	};

	/**
	 * DE UITSLAG VAN ÉÉN STATION, en het criterium is drempelvrij met opzet: elk
	 * verzonnen getal ("de afstand moet boven 0,08 liggen") is een getal dat ik
	 * achteraf op mijn eigen uitslag kan passen.
	 *
	 * Wat §2d vraagt is letterlijk een HERKENNINGSvraag - "zijn de vier
	 * faciliteiten in het frame te onderscheiden" - dus dat is wat er gemeten
	 * wordt: van elke opname met de camera een stukje verzet wordt gevraagd
	 * welke van de vier kamers hij toont, en het antwoord moet zijn eigen kamer
	 * zijn. De MARGE zegt hoe ruim: hoeveel verder de dichtstbijzijnde VERKEERDE
	 * kamer ligt dan de eigen, in procenten. Marge <= 0 betekent dat de opname de
	 * verkeerde kamer aanwijst.
	 *
	 * Twee getallen gaan mee als diagnose maar zijn GEEN poort, en daarom staan
	 * ze hier met de reden erbij in plaats van als een cijfer dat niemand duidt:
	 *   TUSSEN  = kleinste afstand tussen twee verschillende kamers vanaf DEZELFDE
	 *             plek. Bij de controleproeven hoort dit exact 0,0000 te zijn -
	 *             dat is het bewijs dat de meting geen POSITIE lekt en echt alleen
	 *             de inrichting ziet.
	 *   BINNEN  = grootste afstand tussen één kamer en zichzelf met de camera
	 *             verzet. Dit is groter dan TUSSEN, en dat is geen defect maar
	 *             parallax: een meubel vlak bij de lens schuift bij een stap opzij
	 *             over veel meer beeldcellen dan een kast aan de achterwand. Wie
	 *             hierop zou poorten meet camerabeweging, niet inrichting.
	 */
	struct FStationResult
	{
		float Between = 0.0f;
		float Within = 0.0f;
		int32 CorrectlyClassified = 0;
		int32 Total = 0;
		float MinMargin = 0.0f;
		FString ClosestPair;
		FString TightestFrame;
	};

	FStationResult MeasureStation(FAutomationTestBase& Test, const TCHAR* Scenario, int32 Station,
		const TArray<EclipseVault::FEclipseVaultPlacedBox>& Boxes,
		const TArray<EclipseVault::FEclipseVaultSlotPlan>& Plans, bool bVerbose)
	{
		TArray<FDepthFrame> References;
		TArray<TArray<FDepthFrame>> Nudged;
		for (const EclipseVault::FEclipseVaultSlotPlan& Plan : Plans)
		{
			FVector Eye, Look;
			float Side = 1.0f;
			StationCamera(Plan, Station, Eye, Look, Side);
			const bool bMirror = !Plan.bNorthSide;
			References.Add(RenderDepth(Boxes, Eye, Look, bMirror));

			TArray<FDepthFrame>& Set = Nudged.AddDefaulted_GetRef();
			for (const FJitter& Jitter : Jitters)
			{
				const FVector JitterEye = Eye + FVector(Jitter.dU, Side * Jitter.dV, Jitter.dZ);
				const FVector JitterLook = Look + FVector(Jitter.LookDU, 0.0f, Jitter.LookDZ);
				Set.Add(RenderDepth(Boxes, JitterEye, JitterLook, bMirror));
			}
		}

		FStationResult Result;
		Result.Between = TNumericLimits<float>::Max();
		for (int32 A = 0; A < Plans.Num(); ++A)
		{
			for (int32 B = A + 1; B < Plans.Num(); ++B)
			{
				const float Distance = FrameDistance(References[A], References[B]);
				if (Distance < Result.Between)
				{
					Result.Between = Distance;
					Result.ClosestPair = FString::Printf(TEXT("%s<->%s"),
						*Plans[A].SlotId.ToString(), *Plans[B].SlotId.ToString());
				}
			}
		}
		Result.MinMargin = TNumericLimits<float>::Max();
		for (int32 Room = 0; Room < Plans.Num(); ++Room)
		{
			for (int32 Nudge = 0; Nudge < Nudged[Room].Num(); ++Nudge)
			{
				const FDepthFrame& Frame = Nudged[Room][Nudge];
				const float Own = FrameDistance(References[Room], Frame);
				Result.Within = FMath::Max(Result.Within, Own);

				float BestWrong = TNumericLimits<float>::Max();
				int32 BestWrongRoom = INDEX_NONE;
				for (int32 Ref = 0; Ref < Plans.Num(); ++Ref)
				{
					if (Ref == Room)
					{
						continue;
					}
					const float Distance = FrameDistance(Frame, References[Ref]);
					if (Distance < BestWrong)
					{
						BestWrong = Distance;
						BestWrongRoom = Ref;
					}
				}

				++Result.Total;
				Result.CorrectlyClassified += (BestWrong > Own) ? 1 : 0;
				const float Margin = Own > KINDA_SMALL_NUMBER ? (BestWrong - Own) / Own : 0.0f;
				if (Margin < Result.MinMargin)
				{
					Result.MinMargin = Margin;
					Result.TightestFrame = FString::Printf(TEXT("%s nudge %d, dichtstbij verkeerd: %s"),
						*Plans[Room].SlotId.ToString(), Nudge,
						BestWrongRoom != INDEX_NONE ? *Plans[BestWrongRoom].SlotId.ToString() : TEXT("-"));
				}
			}
		}

		if (bVerbose)
		{
			Test.AddInfo(FString::Printf(
				TEXT("GEMETEN %s · station %d: %d van %d opnames wijst zijn eigen kamer aan · krapste marge %+.1f%% (%s) · TUSSEN %.4f (%s) · BINNEN %.4f"),
				Scenario, Station, Result.CorrectlyClassified, Result.Total,
				Result.MinMargin * 100.0f, *Result.TightestFrame,
				Result.Between, *Result.ClosestPair, Result.Within));
		}
		return Result;
	}

	/** De strengste van de twee stations - één goed station maakt een kamer niet leesbaar. */
	FStationResult MeasureRooms(FAutomationTestBase& Test, const TCHAR* Scenario,
		const TArray<EclipseVault::FEclipseVaultPlacedBox>& Boxes,
		const TArray<EclipseVault::FEclipseVaultSlotPlan>& Plans)
	{
		const FStationResult Zero = MeasureStation(Test, Scenario, 0, Boxes, Plans, /*bVerbose*/ true);
		const FStationResult One = MeasureStation(Test, Scenario, 1, Boxes, Plans, /*bVerbose*/ true);
		return Zero.MinMargin <= One.MinMargin ? Zero : One;
	}

	/** Alles bezet met dezelfde onbekende faciliteit: de bouwer valt terug op de generieke blokkendoos. */
	FEclipseBaseState MakeGenericState(TConstArrayView<FEclipseBaseSlotDef> Slots)
	{
		FEclipseBaseState State;
		State.Facilities.Reset();
		for (const FEclipseBaseSlotDef& Slot : Slots)
		{
			FEclipseFacilityState& Facility = State.Facilities.AddDefaulted_GetRef();
			Facility.SlotId = Slot.SlotId;
			Facility.FacilityId = TEXT("GenericBlockout");
			Facility.Level = 1;
			Facility.DaysRemaining = 0;
		}
		return State;
	}
}

// ---------------------------------------------------------------------------
// FALSIFICATIE 2 — de vier faciliteiten zijn in het frame te onderscheiden
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVaultRoomsAreTellableTest,
	"Eclipse.Base.VaultRoomsAreTellable",
	EclipseVaultRoomTest::TestFlags)

bool FEclipseVaultRoomsAreTellableTest::RunTest(const FString& Parameters)
{
	using namespace EclipseVault;
	using namespace EclipseVaultRoomTest;

	UEclipseBaseLayoutAsset* Layout = NewObject<UEclipseBaseLayoutAsset>();
	Layout->Slots = MakeHollowPointSlots();

	// --- CONTROLEPROEF A -------------------------------------------------
	// Vier identieke kamers, geen annexen, dus de vier beelden kunnen alleen
	// nog van elkaar verschillen door iets dat er niet is. Dit is de proef die
	// bewijst dat het instrument kán zakken.
	{
		UEclipseBaseLayoutAsset* FlatLayout = NewObject<UEclipseBaseLayoutAsset>();
		FlatLayout->Slots = MakeHollowPointSlots();
		for (FEclipseBaseSlotDef& Slot : FlatLayout->Slots)
		{
			Slot.AdjacentSlotIds = { SpineNodeId }; // geen memorial-nis, geen generatorkamer
			Slot.EmptyStateStreamingId = NAME_None;
		}
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false);
		if (!TestNotNull(TEXT("controleproef A: wereld"), World))
		{
			return false;
		}
		const FEclipseBaseState Generic = MakeGenericState(FlatLayout->Slots);
		BuildVault(*World, FlatLayout, Generic);
		const FStationResult Flat = MeasureRooms(*this, TEXT("controle A (vier identieke kamers)"),
			ReadPlacedBoxes(*World), PlanSlots(FlatLayout->Slots, Generic));
		World->DestroyWorld(false);

		TestTrue(FString::Printf(
			TEXT("CONTROLEPROEF A: vier identieke kamers zijn NIET te herkennen (%d van %d opnames goed; alles onder %d is het instrument dat zakt zoals het hoort)"),
			Flat.CorrectlyClassified, Flat.Total, Flat.Total),
			Flat.CorrectlyClassified < Flat.Total);
		TestEqual(TEXT("CONTROLEPROEF A: identieke kamers geven vanaf dezelfde plek een IDENTIEK beeld — de meting lekt geen positie, alleen inrichting"),
			FMath::RoundToInt(Flat.Between * 10000.0f), 0);
	}

	// --- CONTROLEPROEF B -------------------------------------------------
	// De ECHTE plattegrond - inclusief de memorial-nis achter B en de
	// generatorkamer achter C - maar met de generieke blokkendoos in alle vier
	// de kamers. Dit is letterlijk wat deze bouwer tot vanavond opleverde: vier
	// keer dezelfde drie dozen, alleen anders getint. Als DIT al vier van de
	// vier haalt, bewijst de groene uitslag hieronder niets over mijn werk.
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false);
		if (!TestNotNull(TEXT("controleproef B: wereld"), World))
		{
			return false;
		}
		const FEclipseBaseState Generic = MakeGenericState(Layout->Slots);
		BuildVault(*World, Layout, Generic);
		const FStationResult Blockout = MeasureRooms(*this, TEXT("controle B (echte plattegrond, generieke blokkendoos)"),
			ReadPlacedBoxes(*World), PlanSlots(Layout->Slots, Generic));
		World->DestroyWorld(false);

		TestTrue(FString::Printf(
			TEXT("CONTROLEPROEF B: de oude generieke blokkendoos is NIET te herkennen (%d van %d goed, krapste marge %+.1f%%) — anders zou de shell het onderscheid maken en niet de inrichting"),
			Blockout.CorrectlyClassified, Blockout.Total, Blockout.MinMargin * 100.0f),
			Blockout.CorrectlyClassified < Blockout.Total);
	}

	// --- DE ECHTE METING -------------------------------------------------
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false);
		if (!TestNotNull(TEXT("de vault: wereld"), World))
		{
			return false;
		}
		const FEclipseBaseState Review = MakeReviewState(Layout->Slots);
		BuildVault(*World, Layout, Review);

		const TArray<FEclipseVaultPlacedBox> Boxes = ReadPlacedBoxes(*World);
		const TArray<FEclipseVaultSlotPlan> Plans = PlanSlots(Layout->Slots, Review);
		AddInfo(FString::Printf(TEXT("GEMETEN de vault staat: %d geplaatste dozen over %d kamers."), Boxes.Num(), Plans.Num()));

		// De vier moeten ook echt alle vier operationeel zijn, anders meet ik
		// lege kamers tegen elkaar en dat zou net zo goed lukken.
		for (const FEclipseVaultSlotPlan& Plan : Plans)
		{
			TestEqual(*FString::Printf(TEXT("%s staat operationeel in de reviewstaat"), *Plan.SlotId.ToString()),
				VisualToString(Plan.Visual), TEXT("Built"));
		}

		const FStationResult Rooms = MeasureRooms(*this, TEXT("de vier kamers"), Boxes, Plans);
		World->DestroyWorld(false);

		TestEqual(FString::Printf(
			TEXT("FALSIFICATIE 2: de vier faciliteiten zijn in het frame te onderscheiden — elke opname wijst zijn eigen kamer aan, ook met de camera verzet (%d van %d, krapste marge %+.1f%% bij %s)"),
			Rooms.CorrectlyClassified, Rooms.Total, Rooms.MinMargin * 100.0f, *Rooms.TightestFrame),
			Rooms.CorrectlyClassified, Rooms.Total);
		TestTrue(FString::Printf(
			TEXT("FALSIFICATIE 2: en met ruimte, niet op het randje (krapste marge %+.1f%%, moet ruim boven 0 liggen)"),
			Rooms.MinMargin * 100.0f), Rooms.MinMargin > 0.05f);
	}
	return true;
}

// ---------------------------------------------------------------------------
// DE VAULT WORDT OOK ECHT GETEKEND
// ---------------------------------------------------------------------------
//
// Vastgepind omdat het EEN AVOND LANG NIET ZO WAS en geen enkele test het zag.
// Het anker is een ATargetPoint, en die zet zichzelf in zijn constructor op
// hidden - hij is bedoeld als markering, niet als iets om te tekenen. Alle
// instanced meshes van de kluis hangen aan dat anker, dus de vault stond er
// compleet: kloppende tags, kloppende transforms, kloppende botsing, een
// beloopbare ronde zonder één val. En hij werd nooit getekend.
//
// De pariteits-Gauntlet was groen, want die leest tags. De loopronde was groen,
// want botsing werkt op verborgen actoren. Alleen een CAMERA vond het, en die
// stond er tot vanavond niet. Dit is dezelfde vorm als de HUD-vondst in
// EXECUTION_PLAN §1b: het instrument gaf het geruststellende antwoord op een
// andere vraag dan die gesteld werd.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVaultIsActuallyDrawnTest,
	"Eclipse.Base.VaultIsActuallyDrawn",
	EclipseVaultRoomTest::TestFlags)

bool FEclipseVaultIsActuallyDrawnTest::RunTest(const FString& Parameters)
{
	using namespace EclipseVault;
	using namespace EclipseVaultRoomTest;

	UEclipseBaseLayoutAsset* Layout = NewObject<UEclipseBaseLayoutAsset>();
	Layout->Slots = MakeHollowPointSlots();

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, /*bInformEngineOfWorld*/ false);
	if (!TestNotNull(TEXT("Headless test world"), World))
	{
		return false;
	}
	BuildVault(*World, Layout, MakeReviewState(Layout->Slots));

	int32 Anchors = 0;
	int32 HiddenAnchors = 0;
	int32 Components = 0;
	int32 Instances = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (!It->ActorHasTag(FName(TEXT("HP_VaultAnchor"))))
		{
			continue;
		}
		++Anchors;
		HiddenAnchors += It->IsHidden() ? 1 : 0;
		TArray<UInstancedStaticMeshComponent*> Isms;
		It->GetComponents<UInstancedStaticMeshComponent>(Isms);
		Components += Isms.Num();
		for (const UInstancedStaticMeshComponent* Ism : Isms)
		{
			Instances += Ism->GetInstanceCount();
			TestTrue(*FString::Printf(TEXT("%s is zichtbaar"), *Ism->GetName()), Ism->GetVisibleFlag());
			TestFalse(*FString::Printf(TEXT("%s is niet verborgen in game"), *Ism->GetName()), Ism->bHiddenInGame);
		}
	}
	World->DestroyWorld(false);

	AddInfo(FString::Printf(TEXT("GEMETEN de vault: %d anker(s), %d ISM-componenten, %d instances."), Anchors, Components, Instances));
	TestEqual(TEXT("Er staat precies één vault-anker"), Anchors, 1);
	TestEqual(TEXT("HET ANKER IS NIET VERBORGEN — een verborgen actor tekent ook zijn instanced meshes niet, en dan staat de hele kluis er onzichtbaar"),
		HiddenAnchors, 0);
	TestTrue(FString::Printf(TEXT("Er hangt echte geometrie aan (%d instances)"), Instances), Instances > 100);
	return true;
}

// ---------------------------------------------------------------------------
// FALSIFICATIE 1 — een ronde loopt van faciliteit naar faciliteit
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseVaultWalkingRoundTest,
	"Eclipse.Base.VaultWalkingRound",
	EclipseVaultRoomTest::TestFlags)

bool FEclipseVaultWalkingRoundTest::RunTest(const FString& Parameters)
{
	using namespace EclipseVault;
	using namespace EclipseVaultRoomTest;

	// Het feel-harnas zonder game mode: vlakke vloer op Z=0, echte speler, echte
	// invoer. De vault staat 150 m dieper, dus die vloer zit de ronde niet in de
	// weg - en als de speler er wél doorheen zakt, valt hij eeuwig, wat precies
	// het onderscheid is dat deze test moet kunnen maken.
	EclipseFeelHarness::FHarness Harness;
	EclipseFeelHarness::FHarness::FOptions Options;
	Options.StepSeconds = 1.0f / 60.0f; // deze ronde legt tientallen meters af
	if (!Harness.Start(*this, Options))
	{
		Harness.Shutdown();
		return false;
	}

	UEclipseBaseLayoutAsset* Layout = NewObject<UEclipseBaseLayoutAsset>();
	Layout->Slots = MakeHollowPointSlots();
	const FEclipseBaseState Review = MakeReviewState(Layout->Slots);

	// --- CONTROLEPROEF: kan de valmeter überhaupt rood worden? ---------------
	// Eerst het bewijs dat de meting werkt, dan pas de meting. Een vault zonder
	// plattegrond is per contract een anker zonder vloer (GDD 14.3.5), dus wie
	// daar staat MOET vallen - en als de meter dat niet meldt, meet hij niets en
	// is elke groene uitslag hieronder waardeloos. Dezelfde drempel (60 cm) als
	// de echte assert, want een controleproef met een soepelere lat bewijst niet
	// dat de LAT werkt.
	{
		BuildVault(*Harness.World, /*Layout*/ nullptr, Review);
		const FVector Nergens(0.0f, 1210.0f, -15000.0f + 120.0f);
		Harness.Body->SetActorLocation(Nergens, /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
		Harness.Body->GetCharacterMovement()->StopMovementImmediately();
		const double VoorZ = Harness.Location().Z;
		Harness.HoldFor(TEXT("Move"), FVector2D(0.0f, 1.0f), 1.5);
		const double ControleVal = VoorZ - Harness.Location().Z;
		EclipseFeelHarness::Report(*this, TEXT("controleproef: val zonder vloer onder je"), ControleVal, TEXT("cm"),
			TEXT("fors — dit is de toestand die de echte ronde moet kunnen afkeuren"));
		TestTrue(FString::Printf(
			TEXT("CONTROLEPROEF 1: de valmeter slaat aan op een vault zonder vloer (%.0f cm gevallen, drempel 60)"), ControleVal),
			ControleVal > 60.0);
	}

	// En nu de echte kluis: RebuildVault ruimt het lege anker eerst op.
	RebuildVault(*Harness.World, Layout, Review);

	FVector Entry;
	if (!TestTrue(TEXT("de vault heeft een ingang (Entry_Vault)"), FindVaultPoint(*Harness.World, TEXT("Entry_Vault"), Entry)))
	{
		Harness.Shutdown();
		return false;
	}

	// De route loopt op de INTERACTIEPUNTEN die de bouwer zelf plaatst, niet op
	// getallen die ik hier verzin: dat zijn de plekken waar het spel vindt dat
	// je hoort te staan, en een ronde die daar niet komt heeft de kamer niet
	// bezocht.
	struct FStop { const TCHAR* Naam; FName Tag; };
	const FStop Stops[] = {
		{ TEXT("Command Center — de kaarttafel"), TEXT("Site_MapTable") },
		{ TEXT("Barracks — het musterbord"),      TEXT("Site_MusterBoard") },
		{ TEXT("Workshop — de werkbank"),         TEXT("Site_Workbench") },
		{ TEXT("Intelligence Center — de console"), TEXT("Site_IntelConsole") },
	};

	Harness.Body->SetActorLocation(Entry + FVector(0.0f, 0.0f, 120.0f), /*bSweep*/ false, nullptr, ETeleportType::TeleportPhysics);
	Harness.Body->GetCharacterMovement()->StopMovementImmediately();
	Harness.HoldFor(TEXT("Move"), FVector2D::ZeroVector, 2.0, [&Harness]()
	{
		return Harness.Body->GetCharacterMovement()->IsMovingOnGround() && Harness.SpeedCm() < 1.0f;
	});

	const double StandZ = Harness.Location().Z;
	EclipseFeelHarness::Report(*this, TEXT("stahoogte in de vault"), StandZ, TEXT("cm"),
		TEXT("de vloer van de kluis ligt op -15000; hier staat hij bovenop"));
	if (!TestTrue(FString::Printf(TEXT("de speler STAAT in de vault na de teleport (%.1f cm, grond=%d)"),
		StandZ, Harness.Body->GetCharacterMovement()->IsMovingOnGround() ? 1 : 0),
		Harness.Body->GetCharacterMovement()->IsMovingOnGround()))
	{
		Harness.Shutdown();
		return false;
	}

	// De meting loopt MEE, niet erna: een val die tussen twee routepunten begint
	// en eindigt zou anders onzichtbaar blijven. Dat is de fout die dit hele
	// bestand moet kunnen vinden, dus wordt er per tick gekeken.
	double LaagsteZ = StandZ;
	int32 TicksLos = 0;
	int32 TicksTotaal = 0;
	FString WaarHetMisGing;

	auto LoopNaar = [&](const FVector& Doel, double MaxSeconden, float AankomstStraal) -> bool
	{
		const int32 MaxStappen = FMath::Max(1, FMath::RoundToInt(MaxSeconden / Harness.StepSeconds));
		const int32 Venster = FMath::Max(1, FMath::RoundToInt(0.5 / Harness.StepSeconds));
		float AfstandBijVensterStart = FVector::Dist2D(Harness.Location(), Doel);
		int32 StappenInVenster = 0;
		int32 ZijstapStappen = 0;
		float ZijstapRichting = 1.0f;

		for (int32 Stap = 0; Stap < MaxStappen; ++Stap)
		{
			const float Afstand = FVector::Dist2D(Harness.Location(), Doel);
			if (Afstand <= AankomstStraal)
			{
				return true;
			}
			if (++StappenInVenster >= Venster)
			{
				if (ZijstapStappen <= 0 && AfstandBijVensterStart - Afstand < 20.0f)
				{
					ZijstapStappen = FMath::Max(1, FMath::RoundToInt(0.7 / Harness.StepSeconds));
					ZijstapRichting = -ZijstapRichting;
				}
				AfstandBijVensterStart = Afstand;
				StappenInVenster = 0;
			}

			Harness.AimAt(FVector(Doel.X, Doel.Y, Harness.Location().Z + 60.0f));
			Harness.Inject(TEXT("Move"), ZijstapStappen > 0
				? FVector2D(ZijstapRichting, 0.35f)
				: FVector2D(0.0f, 1.0f));
			if (ZijstapStappen > 0)
			{
				--ZijstapStappen;
			}
			Harness.Step();

			++TicksTotaal;
			const double Z = Harness.Location().Z;
			LaagsteZ = FMath::Min(LaagsteZ, Z);
			if (!Harness.Body->GetCharacterMovement()->IsMovingOnGround())
			{
				++TicksLos;
			}
			if (Z < StandZ - 60.0 && WaarHetMisGing.IsEmpty())
			{
				WaarHetMisGing = FString::Printf(TEXT("%s (Z %.1f, %.0f cm onder stahoogte)"),
					*Harness.Location().ToString(), Z, StandZ - Z);
			}
		}
		return FVector::Dist2D(Harness.Location(), Doel) <= AankomstStraal;
	};

	// De hartlijn van de gang, afgeleid uit de kamers zelf: die liggen paarsgewijs
	// aan weerszijden op gelijke afstand, dus het midden van de uiterste twee IS
	// de gang. Geen 0 hardcoderen - dat zou een tweede plek zijn waar de
	// vault-oorsprong staat opgeschreven, en die twee gaan een keer uit elkaar
	// lopen (GDD 14.2).
	const TArray<FEclipseVaultSlotPlan> RoutePlans = PlanSlots(Layout->Slots, Review);
	double MinKamerY = TNumericLimits<double>::Max();
	double MaxKamerY = -TNumericLimits<double>::Max();
	for (const FEclipseVaultSlotPlan& Plan : RoutePlans)
	{
		MinKamerY = FMath::Min(MinKamerY, Plan.ChamberCenter.Y);
		MaxKamerY = FMath::Max(MaxKamerY, Plan.ChamberCenter.Y);
	}
	const double SpineY = (MinKamerY + MaxKamerY) * 0.5;

	// Naar binnen: eerst de sluis door, DAARNA pas de gang op. Die tweede stap
	// stond er eerst niet, en de eerste ronde liep daardoor vanuit de sluiskoker
	// schuin tegen de koker-wand aan de kamer aan de verre kant proberen te
	// bereiken. Dat was een gat in de ROUTE en niet in de vault - het onderscheid
	// is de moeite waard, want de meting wees eerst naar de kluis.
	bool bRondeCompleet = LoopNaar(FVector(Entry.X, Entry.Y - 700.0, StandZ), 12.0, 150.0f);
	bRondeCompleet = LoopNaar(FVector(Entry.X, SpineY, StandZ), 12.0, 150.0f) && bRondeCompleet;
	TestTrue(TEXT("de ronde komt door de sluis de gang in"), bRondeCompleet);

	int32 Bezocht = 0;
	for (const FStop& Stop : Stops)
	{
		FVector Site;
		if (!TestTrue(*FString::Printf(TEXT("%s heeft een interactiepunt"), Stop.Naam),
			FindVaultPoint(*Harness.World, Stop.Tag, Site)))
		{
			continue;
		}
		// Eerst het kruispunt in de gang op de hoogte van de kamer, dan naar
		// binnen. Zonder dat tussenpunt duwt een rechte lijn de speler tegen de
		// gangmuur - dat is geen defect in de vault maar in de route.
		// 90 cm en niet 150: op 150 stopt de ronde nog IN de deuropening, en dan
		// bewijst hij dat je de kamer kunt zien en niet dat je erin kunt staan.
		const bool bKruispunt = LoopNaar(FVector(Site.X, SpineY, StandZ), 16.0, 150.0f);
		const bool bBinnen = LoopNaar(FVector(Site.X, Site.Y, StandZ), 12.0, 90.0f);
		const double Afstand = FVector::Dist2D(Harness.Location(), Site);
		EclipseFeelHarness::Report(*this, Stop.Naam, Afstand, TEXT("cm van het interactiepunt"),
			TEXT("< 90 = hij STAAT in de kamer, niet in de deuropening"));
		if (bKruispunt && bBinnen)
		{
			++Bezocht;
		}
		else
		{
			AddError(FString::Printf(TEXT("de ronde bereikte %s niet (%.0f cm ervandaan, kruispunt=%d)"),
				Stop.Naam, Afstand, bKruispunt ? 1 : 0));
		}
		bRondeCompleet = bRondeCompleet && bKruispunt && bBinnen;
		// Terug de gang op voor de volgende kamer. Dat de ronde er ook weer UIT
		// komt is de helft van de belofte: een kamer die je alleen in kunt is een
		// val en geen ruimte.
		if (!LoopNaar(FVector(Site.X, SpineY, StandZ), 12.0, 150.0f))
		{
			AddError(FString::Printf(TEXT("de ronde kwam %s niet meer uit — de kamer laat je er wel in maar niet meer uit"), Stop.Naam));
			bRondeCompleet = false;
		}
	}

	const double GrootsteVal = StandZ - LaagsteZ;
	EclipseFeelHarness::Report(*this, TEXT("grootste val onder stahoogte"), GrootsteVal, TEXT("cm"),
		TEXT("~0 — hij loopt over een vloer, hij zakt er nergens in"));
	EclipseFeelHarness::Report(*this, TEXT("ticks zonder grond onder de voeten"), TicksLos, TEXT(""),
		TEXT("een handvol is een stap over een kabelgoot; honderden is een val"));
	EclipseFeelHarness::Report(*this, TEXT("ticks in de ronde"), TicksTotaal, TEXT(""), nullptr);

	TestEqual(TEXT("FALSIFICATIE 1a: de ronde bezoekt alle vier de faciliteiten"), Bezocht, 4);
	TestTrue(FString::Printf(
		TEXT("FALSIFICATIE 1b: de ronde valt nergens door de geometrie (%.1f cm onder stahoogte%s)"),
		GrootsteVal, WaarHetMisGing.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("; eerst hier: %s"), *WaarHetMisGing)),
		GrootsteVal < 60.0);
	TestTrue(FString::Printf(TEXT("FALSIFICATIE 1c: de speler staat het overgrote deel van de ronde op de grond (%d van %d ticks los)"),
		TicksLos, TicksTotaal),
		TicksTotaal > 0 && TicksLos < TicksTotaal / 10);

	Harness.Shutdown();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
