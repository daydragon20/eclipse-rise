#include "UI/EclipseMapGraphWidget.h"

#include "Brushes/SlateColorBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

#define LOCTEXT_NAMESPACE "EclipseMapGraph"

namespace
{
	/**
	 * HET PALET, en het is hetzelfde als dat van de tekstregels eronder. Twee
	 * paletten op één bord zouden twee verhalen vertellen over dezelfde data.
	 *
	 * DE `Graph`-VOORVOEGSELS ZIJN GEEN STIJL, ZE ZIJN DE ISOLATIE.
	 *
	 * GEMETEN 01-08: dit bestand en `EclipseStrategyMapWidget.cpp` droegen
	 * allebei een anonieme namespace met exact DEZELFDE namen — toen nog
	 * `OwnerPlayer`, `LaneOpen`, `ColorForOwner`, zonder voorvoegsel. Dat ging
	 * goed zolang UBT ze in verschillende unity-blokken stopte. Eén nieuw
	 * .cpp-bestand in deze map verschoof die indeling, beide bestanden landden
	 * in hetzelfde blok, en toen waren het herdefinities: 18 compilerfouten in
	 * bestanden waar niemand iets aan veranderd had. De toenmalige `Ink` deed
	 * hetzelfde tegen de lokale `UImage* Ink` in `EclipseScreenPlate.cpp`
	 * (C4459, hides global declaration).
	 *
	 * Een anonieme namespace geeft interne binding per TRANSLATION UNIT — en in
	 * een unity-build zijn deze bestanden samen één translation unit. De
	 * anonieme namespace beschermt hier dus niets; de naam doet dat.
	 *
	 * Waarom niet een BENOEMDE namespace met `using namespace` eronder: dan
	 * staan deze namen alsnog in de globale scope van het hele blok, en wordt
	 * elke onvoorgevoegde aanroep in `EclipseStrategyMapWidget.cpp` DUBBELZINNIG
	 * (zijn eigen anonieme `ColorForOwner` plus die van hier). Dat ruilt een
	 * herdefinitie in voor een ambiguïteit en lost niets op.
	 */
	const FLinearColor GraphOwnerPlayer(0.35f, 0.92f, 0.45f);
	const FLinearColor GraphOwnerContested(0.97f, 0.76f, 0.22f);
	const FLinearColor GraphOwnerDominion(0.92f, 0.31f, 0.26f);
	const FLinearColor GraphLaneOpen(0.70f, 0.82f, 0.88f);
	const FLinearColor GraphLaneGated(0.98f, 0.66f, 0.18f);
	const FLinearColor GraphLaneSmuggler(0.74f, 0.53f, 0.95f);
	const FLinearColor GraphAlarm(0.97f, 0.29f, 0.24f);
	const FLinearColor GraphInk(0.02f, 0.02f, 0.03f, 1.0f);
	const FLinearColor GraphBone(0.93f, 0.90f, 0.83f);

	/** De inktlijn is DIKKER dan de kleur; dat verschil ís de outline (15.5). */
	constexpr float LaneInkThickness = 7.0f;
	constexpr float LaneThickness = 3.0f;

	/** Halve maat van een knoop; de inktrand ligt er 3 px omheen. */
	constexpr float NodeHalf = 11.0f;
	constexpr float NodeInkHalf = 15.0f;

	/** Marge binnen het vlak, zodat labels niet buiten de plaat vallen. */
	const FMargin BoardInset(76.0f, 28.0f, 76.0f, 46.0f);

	/**
	 * De maat van het tekenvlak, in LAYOUT-eenheden en niet in schermpixels — en
	 * dat verschil is de reden dat de eerste twee versies mis waren.
	 *
	 * GEMETEN op het frame van 01-08: een blad dat 440x300 vroeg, kwam er als
	 * 311x213 uit. Dat is geen krimpende doos maar de DPI-schaal van UMG: op 720p
	 * staat de standaardcurve op ~0,71, en die geldt voor ALLES in deze boom,
	 * inclusief de korpsgroottes. Wie hier in schermpixels denkt, krijgt
	 * consequent een derde te weinig — en zoekt dat vervolgens in de verkeerde
	 * hoek (ik zocht het eerst in een over-vol horizontaal vak).
	 *
	 * Twee dingen volgen eruit: het vlak mag veel groter dan het frame suggereert
	 * (1280 px = ~1800 eenheden breed), en de labels moeten een paar punten groter
	 * dan je zou zetten voor een 1:1-scherm.
	 *
	 * 600x430 naast een lijst van ~1140 eenheden past binnen die 1800, en is de
	 * grootste maat die dat doet.
	 */
	constexpr float BoardWidth = 600.0f;
	constexpr float BoardHeight = 430.0f;

	/** Korpsgroottes in dezelfde layout-eenheden; op 720p rendert 14 als ~10 px. */
	constexpr int32 NodeNameFont = 14;
	constexpr int32 NodeNumbersFont = 11;
	constexpr int32 EdgeCostFont = 11;

	const FSlateColorBrush& WhiteBrush()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return Brush;
	}

	FLinearColor GraphColorForOwner(EEclipseRegionOwner Owner)
	{
		switch (Owner)
		{
		case EEclipseRegionOwner::Player:    return GraphOwnerPlayer;
		case EEclipseRegionOwner::Contested: return GraphOwnerContested;
		default:                             return GraphOwnerDominion;
		}
	}

	/** Een lane die voor NIEMAND open is, is alarmrood; dat is het feit dat telt. */
	FLinearColor ColorForEdge(const EclipseStrategyMap::FEclipseMapEdgeView& Edge)
	{
		if (!Edge.bMilitaryPassable && !Edge.bSmugglerPassable)
		{
			return GraphAlarm;
		}
		switch (Edge.Status)
		{
		case EEclipseLaneStatus::SpireGated:   return Edge.bMilitaryPassable ? GraphLaneGated : GraphAlarm;
		case EEclipseLaneStatus::SmugglerOnly: return GraphLaneSmuggler;
		default:                               return GraphLaneOpen;
		}
	}
}

/**
 * Het blad dat verft. Alles wat het weet komt uit `FEclipseMapView`; het stelt
 * geen enkele vraag aan de wereld.
 */
class SEclipseMapGraph : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SEclipseMapGraph) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SetCanTick(false);
	}

	void SetBoard(const EclipseStrategyMap::FEclipseMapView& InView)
	{
		View = InView;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	int32 GetDrawnEdgeCount() const { return DrawnEdges; }

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		// Een VASTE maat, en dat is geen luiheid. De inhoud is genormaliseerd
		// (0..1), dus hij heeft geen eigen maat om naar te groeien; zonder een
		// vaste wens zou het blad 0x0 vragen en nooit één pixel krijgen.
		return FVector2D(BoardWidth, BoardHeight);
	}

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override
	{
		DrawnEdges = 0;
		if (!View.bHasLayout)
		{
			// GEEN LAYOUT IS GEEN LEEG VLAK. Zonder deze regel zou een bord
			// zonder geauthorde posities een zwart gat van 760x300 zijn en zou
			// niemand kunnen zien of dat de data of de tekenlaag is.
			DrawInkedText(OutDrawElements, LayerId, AllottedGeometry,
				View.LayoutStatusText.IsEmpty()
					? LOCTEXT("NoLayout", "no map layout").ToString()
					: View.LayoutStatusText.ToString(),
				FVector2f(8.0f, 8.0f), GraphAlarm, 11);
			return LayerId + 1;
		}

		const FVector2f Size = AllottedGeometry.GetLocalSize();
		const FVector2f Origin(BoardInset.Left, BoardInset.Top);
		const FVector2f Span(
			FMath::Max(1.0f, Size.X - BoardInset.Left - BoardInset.Right),
			FMath::Max(1.0f, Size.Y - BoardInset.Top - BoardInset.Bottom));
		auto ToLocal = [Origin, Span](const FVector2D& Normalised)
		{
			return FVector2f(
				Origin.X + Span.X * static_cast<float>(Normalised.X),
				Origin.Y + Span.Y * static_cast<float>(Normalised.Y));
		};

		const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();

		// ---------------------------------------------------------- de lanes
		// Eerst alle inkt, dan alle kleur, en dan pas de knopen: zo kan de inkt
		// van de ene lane nooit over de KLEUR van een andere heen vallen, en
		// dekken de knopen altijd de lijnuiteinden af.
		int32 Layer = LayerId;
		for (const EclipseStrategyMap::FEclipseMapEdgeView& Edge : View.Edges)
		{
			const TArray<FVector2f> Points = { ToLocal(Edge.A), ToLocal(Edge.B) };
			FSlateDrawElement::MakeLines(OutDrawElements, Layer, PaintGeometry, Points,
				ESlateDrawEffect::None, GraphInk, /*bAntialias*/ true, LaneInkThickness);
		}
		++Layer;

		for (const EclipseStrategyMap::FEclipseMapEdgeView& Edge : View.Edges)
		{
			const FVector2f From = ToLocal(Edge.A);
			const FVector2f To = ToLocal(Edge.B);
			const FLinearColor Color = ColorForEdge(Edge);

			if (Edge.Status == EEclipseLaneStatus::SmugglerOnly)
			{
				// GESTREEPT, en dat is geen versiering: een kruipgang is geen weg.
				// Een status die alleen in de kleur zit, verdwijnt voor iedereen
				// die kleuren anders ziet (`15_visual_quality_charter.md`).
				DrawDashed(OutDrawElements, Layer, PaintGeometry, From, To, Color);
			}
			else
			{
				const TArray<FVector2f> Points = { From, To };
				FSlateDrawElement::MakeLines(OutDrawElements, Layer, PaintGeometry, Points,
					ESlateDrawEffect::None, Color, /*bAntialias*/ true, LaneThickness);
			}
			++DrawnEdges;

			const FVector2f Middle = (From + To) * 0.5f;
			const FVector2f Along = (To - From).GetSafeNormal();
			const FVector2f Across(-Along.Y, Along.X);

			// Dicht voor colonnes = een streep DWARS over de lane. De vorm zegt
			// het, niet alleen de kleur.
			if (!Edge.bMilitaryPassable)
			{
				const TArray<FVector2f> Bar = { Middle - Across * 9.0f, Middle + Across * 9.0f };
				FSlateDrawElement::MakeLines(OutDrawElements, Layer + 1, PaintGeometry, Bar,
					ESlateDrawEffect::None, GraphAlarm, /*bAntialias*/ true, 5.0f);
			}

			// Wat de oversteek kost, op de lijn (GDD 3.1 regel 4).
			DrawInkedText(OutDrawElements, Layer + 2, AllottedGeometry, Edge.CostText.ToString(),
				Middle + Across * 13.0f - FVector2f(HalfTextWidth(Edge.CostText.ToString(), EdgeCostFont), 7.0f),
				Color, EdgeCostFont);
		}
		Layer += 3;

		// --------------------------------------------------------- de knopen
		for (const EclipseStrategyMap::FEclipseMapRegionView& Region : View.Regions)
		{
			if (!Region.bHasBoardPosition)
			{
				continue; // kan hier niet meer voorkomen; stil overslaan is goedkoper dan tekenen wat je niet weet
			}

			const FVector2f Centre = ToLocal(Region.BoardPosition);
			const FLinearColor Owner = GraphColorForOwner(Region.Owner);

			FSlateDrawElement::MakeBox(OutDrawElements, Layer,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(NodeInkHalf * 2.0f, NodeInkHalf * 2.0f),
					FSlateLayoutTransform(Centre - FVector2f(NodeInkHalf, NodeInkHalf))),
				&WhiteBrush(), ESlateDrawEffect::None, GraphInk);

			FSlateDrawElement::MakeBox(OutDrawElements, Layer + 1,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(NodeHalf * 2.0f, NodeHalf * 2.0f),
					FSlateLayoutTransform(Centre - FVector2f(NodeHalf, NodeHalf))),
				&WhiteBrush(), ESlateDrawEffect::None, Owner);

			// AFGESNEDEN IS EEN VORM EN GEEN KLEUR. Een regio die niet bevoorraad
			// kan worden krijgt een zwarte kern: hij is er wel, er komt alleen
			// niets doorheen (GDD 3.1 regel 4).
			if (!Region.bSupplied)
			{
				FSlateDrawElement::MakeBox(OutDrawElements, Layer + 2,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(NodeHalf, NodeHalf),
						FSlateLayoutTransform(Centre - FVector2f(NodeHalf * 0.5f, NodeHalf * 0.5f))),
					&WhiteBrush(), ESlateDrawEffect::None, GraphInk);
			}

			// HET LABEL BOVEN DE KNOOP, en de getallen eronder. Beide onder de
			// knoop zetten (de eerste versie) botste met de prijs die op de lijn
			// eronder staat: op het frame van 01-08 liep "Supply Depot" dwars door
			// "2d · r16 (smug)". Boven en onder verdelen is de goedkoopste manier
			// om twee labels bij één punt te houden zonder ze te stapelen.
			const FString Name = Region.DisplayName.ToString();
			const FString Numbers = FString::Printf(TEXT("g%d · u%d"), Region.GarrisonStrength, Region.Unrest);
			DrawInkedText(OutDrawElements, Layer + 3, AllottedGeometry, Name,
				Centre + FVector2f(-HalfTextWidth(Name, NodeNameFont), -NodeInkHalf - 19.0f), Owner, NodeNameFont);
			DrawInkedText(OutDrawElements, Layer + 3, AllottedGeometry, Numbers,
				Centre + FVector2f(-HalfTextWidth(Numbers, NodeNumbersFont), NodeInkHalf + 3.0f), GraphBone, NodeNumbersFont);
		}

		return Layer + 4;
	}

private:
	static FSlateFontInfo FontOf(int32 Size)
	{
		return FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}

	static float HalfTextWidth(const FString& Text, int32 Size)
	{
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return static_cast<float>(Measure->Measure(Text, FontOf(Size)).X) * 0.5f;
	}

	/**
	 * Tekst met contour. Twee keer zetten (zwart, dan kleur) is de goedkoopste
	 * inktrand die Slate kent, en het is hetzelfde gebaar als de schaduw onder
	 * elke tekstregel van dit bord — één stijl, niet twee.
	 */
	static void DrawInkedText(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FGeometry& Geometry,
		const FString& Text,
		const FVector2f& Position,
		const FLinearColor& Color,
		int32 Size)
	{
		const FSlateFontInfo Font = FontOf(Size);
		FSlateDrawElement::MakeText(OutDrawElements, Layer,
			Geometry.ToPaintGeometry(FVector2f(1.0f, 1.0f), FSlateLayoutTransform(Position + FVector2f(1.0f, 1.0f))),
			Text, Font, ESlateDrawEffect::None, FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
		FSlateDrawElement::MakeText(OutDrawElements, Layer,
			Geometry.ToPaintGeometry(FVector2f(1.0f, 1.0f), FSlateLayoutTransform(Position)),
			Text, Font, ESlateDrawEffect::None, Color);
	}

	static void DrawDashed(
		FSlateWindowElementList& OutDrawElements,
		int32 Layer,
		const FPaintGeometry& PaintGeometry,
		const FVector2f& From,
		const FVector2f& To,
		const FLinearColor& Color)
	{
		constexpr float Dash = 9.0f;
		constexpr float Gap = 6.0f;
		const float Length = (To - From).Size();
		const FVector2f Direction = (To - From).GetSafeNormal();
		for (float Walked = 0.0f; Walked < Length; Walked += Dash + Gap)
		{
			const float End = FMath::Min(Walked + Dash, Length);
			const TArray<FVector2f> Points = { From + Direction * Walked, From + Direction * End };
			FSlateDrawElement::MakeLines(OutDrawElements, Layer, PaintGeometry, Points,
				ESlateDrawEffect::None, Color, /*bAntialias*/ true, LaneThickness);
		}
	}

	EclipseStrategyMap::FEclipseMapView View;

	/** OnPaint is const; deze teller is het bewijs dat er verf op het scherm kwam. */
	mutable int32 DrawnEdges = 0;
};

UEclipseMapGraphWidget::UEclipseMapGraphWidget()
{
	// Geen muisdoel: het bord vangt klikken via de aanbodknoppen, niet via de
	// tekening. Een tekenlaag die klikken opslokt is een bug die niemand ziet.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UEclipseMapGraphWidget::SetBoard(const EclipseStrategyMap::FEclipseMapView& InView)
{
	PendingView = InView;
	if (Graph.IsValid())
	{
		Graph->SetBoard(InView);
	}
}

int32 UEclipseMapGraphWidget::GetDrawnEdgeCount() const
{
	return Graph.IsValid() ? Graph->GetDrawnEdgeCount() : 0;
}

TSharedRef<SWidget> UEclipseMapGraphWidget::RebuildWidget()
{
	Graph = SNew(SEclipseMapGraph);
	// De view die al gezet was VOOR de Slate-kant bestond. Dit overslaan is de
	// klassieke fout van deze widgetlaag: iets vastleggen op een moment waarop de
	// andere kant er nog niet is, en dan een leeg scherm niet kunnen verklaren.
	Graph->SetBoard(PendingView);
	return Graph.ToSharedRef();
}

void UEclipseMapGraphWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Graph.Reset();
}

#undef LOCTEXT_NAMESPACE
