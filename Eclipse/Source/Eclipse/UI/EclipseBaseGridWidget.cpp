#include "UI/EclipseBaseGridWidget.h"

#include "Brushes/SlateColorBrush.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UI/EclipseScreenPlate.h"
#include "Widgets/SLeafWidget.h"

#define LOCTEXT_NAMESPACE "EclipseBaseGrid"

namespace
{
	/**
	 * HET PALET. `Grid`-voorgevoegd, en dat is geen stijl maar de isolatie:
	 * `EclipseMapGraphWidget.cpp` en `EclipseStrategyMapWidget.cpp` droegen
	 * allebei een anonieme namespace met dezelfde namen, en toen dit bestand
	 * erbij kwam gooide UBT ze in één unity-blok — 18 herdefinitiefouten in
	 * bestanden waar niemand iets aan veranderd had. Een anonieme namespace
	 * isoleert per TRANSLATION UNIT, en een unity-blok is er één.
	 *
	 * De statuskleuren zijn dezelfde familie als het kaartbord al gebruikt
	 * (één stijl, 15.5). De TEAMkleur zit hier bewust NIET in: die ligt nergens
	 * vast en is stijlvraag O-8.
	 */
	const FLinearColor GridInk = EclipseScreenPlate::InkColor();
	const FLinearColor GridBone = EclipseScreenPlate::BoneColor();
	const FLinearColor GridDim(0.72f, 0.70f, 0.65f);
	const FLinearColor GridOnline(0.35f, 0.92f, 0.45f);
	const FLinearColor GridBuilding(0.97f, 0.76f, 0.22f);
	const FLinearColor GridDamaged(0.92f, 0.31f, 0.26f);
	const FLinearColor GridSealed(0.42f, 0.42f, 0.46f);
	const FLinearColor GridEmpty(0.62f, 0.72f, 0.80f);

	/** Tegelvulling: donker genoeg dat botkleurige tekst er altijd op leest. */
	const FLinearColor GridTileFill(0.055f, 0.060f, 0.075f, 0.96f);
	const FLinearColor GridTileFillSealed(0.030f, 0.032f, 0.038f, 0.96f);

	/**
	 * Maten in LAYOUT-eenheden (op 720p rendert dit als ~0,71x).
	 *
	 * GEMETEN op het eerste frame (`HUD_hub_faciliteiten.png`, 01-08): op 176x112
	 * liep "COMMAND CENTER L1" dwars over de tegel ernaast en stak "accepts
	 * Intelligence Center" er aan de rechterkant uit. Precies de bevinding die
	 * geen enkele test kan doen — de logica klopte, de MAAT niet. Breder en
	 * hoger, én elke regel gaat nu door `GridFitText` (zie daar): een tegel mag
	 * nooit in zijn buurman schrijven.
	 */
	constexpr float TileWidth = 250.0f;
	constexpr float TileHeight = 140.0f;
	constexpr float TileGap = 14.0f;

	/** Binnenmarge waarbinnen tekst moet blijven. */
	constexpr float TilePadX = 11.0f;

	/** De inktrand is DIKKER dan de vulling; dat verschil ís de outline (15.5). */
	constexpr float InkBorder = 4.5f;

	/** Hoe diep de schuine hoek de tegel in snijdt — bijna niets recht (Borderlands-taal). */
	constexpr float BevelDepth = 24.0f;

	constexpr int32 SlotLabelFont = 11;
	constexpr int32 FacilityFont = 14;
	constexpr int32 LevelFont = 26;
	constexpr int32 DetailFont = 11;

	const FSlateColorBrush& GridWhiteBrush()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return Brush;
	}

	FLinearColor GridColorForStatus(EclipseBaseView::EEclipseSlotStatus Status)
	{
		using EclipseBaseView::EEclipseSlotStatus;
		switch (Status)
		{
		case EEclipseSlotStatus::Locked:            return GridSealed;
		case EEclipseSlotStatus::Empty:             return GridEmpty;
		case EEclipseSlotStatus::UnderConstruction: return GridBuilding;
		case EEclipseSlotStatus::Damaged:           return GridDamaged;
		default:                                    return GridOnline;
		}
	}

	FSlateFontInfo GridFontOf(int32 Size, bool bBold = true)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	float GridTextWidth(const FString& Text, int32 Size, bool bBold = true)
	{
		const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return static_cast<float>(Measure->Measure(Text, GridFontOf(Size, bBold)).X);
	}

	/**
	 * Kort een regel af tot hij binnen `MaxWidth` past, met een beletselteken.
	 *
	 * WAAROM DIT ER MOET ZIJN EN NIET "we houden de teksten wel kort". De namen
	 * op deze tegels komen uit DT_Facilities en uit de layout — data die iemand
	 * anders authort, in een taal die vertaald wordt. "Intelligence Center" past
	 * net; het Duitse equivalent niet. Een tegel die overloopt schrijft in zijn
	 * buurman, en dan is niet één regel onleesbaar maar twee.
	 *
	 * Afkorten en niet verkleinen: een kleiner korps op een enkele tegel maakt
	 * het raster ongelijk, en ongelijke korpsgroottes lezen trager dan een
	 * afgekapte naam (`REFERENTIE_HUD_BORDERLANDS.md` r49 — binnen een halve
	 * seconde vindbaar).
	 */
	FString GridFitText(const FString& Text, int32 Size, bool bBold, float MaxWidth)
	{
		if (MaxWidth <= 0.0f || Text.IsEmpty() || GridTextWidth(Text, Size, bBold) <= MaxWidth)
		{
			return Text;
		}
		static const FString Ellipsis(TEXT("…"));
		const float EllipsisWidth = GridTextWidth(Ellipsis, Size, bBold);

		int32 Keep = Text.Len();
		while (Keep > 0 && GridTextWidth(Text.Left(Keep), Size, bBold) + EllipsisWidth > MaxWidth)
		{
			--Keep;
		}
		return Keep > 0 ? Text.Left(Keep).TrimEnd() + Ellipsis : Ellipsis;
	}

	/** Een gevuld vlak op een positie — de bouwsteen voor alles hieronder. */
	void GridFillRect(
		FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry,
		const FVector2f& TopLeft, const FVector2f& Size, const FLinearColor& Color)
	{
		if (Size.X <= 0.0f || Size.Y <= 0.0f)
		{
			return;
		}
		FSlateDrawElement::MakeBox(Out, Layer,
			Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(TopLeft)),
			&GridWhiteBrush(), ESlateDrawEffect::None, Color);
	}

	void GridLine(
		FSlateWindowElementList& Out, int32 Layer, const FPaintGeometry& Paint,
		const FVector2f& From, const FVector2f& To, const FLinearColor& Color, float Thickness)
	{
		const TArray<FVector2f> Points = { From, To };
		FSlateDrawElement::MakeLines(Out, Layer, Paint, Points, ESlateDrawEffect::None, Color, /*bAntialias*/ true, Thickness);
	}

	/**
	 * Tekst met contour: twee keer zetten, zwart en dan kleur. Hetzelfde gebaar
	 * als de outline op de 3D-geometrie en als het kaartbord — één visuele taal
	 * voor de hele schermlaag, niet een aparte voor dit raster.
	 */
	void GridInkedText(
		FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry,
		const FString& Text, const FVector2f& Position, const FLinearColor& Color, int32 Size, bool bBold = true)
	{
		if (Text.IsEmpty())
		{
			return;
		}
		const FSlateFontInfo Font = GridFontOf(Size, bBold);
		for (const FVector2f& Offset : { FVector2f(1.0f, 1.0f), FVector2f(-1.0f, 1.0f), FVector2f(1.0f, -1.0f), FVector2f(-1.0f, -1.0f) })
		{
			FSlateDrawElement::MakeText(Out, Layer,
				Geometry.ToPaintGeometry(FVector2f(1.0f, 1.0f), FSlateLayoutTransform(Position + Offset)),
				Text, Font, ESlateDrawEffect::None, GridInk);
		}
		FSlateDrawElement::MakeText(Out, Layer + 1,
			Geometry.ToPaintGeometry(FVector2f(1.0f, 1.0f), FSlateLayoutTransform(Position)),
			Text, Font, ESlateDrawEffect::None, Color);
	}

	/**
	 * Diagonale arcering binnen een vlak, geknipt op de tegelrand.
	 *
	 * Dit is het gereedschap waarmee VORM de status draagt: dicht voor SEALED,
	 * schuin en breed voor gevarenstrepen op het onafgebouwde deel. Het knippen
	 * gebeurt met de hand (de lijn wordt op de rechthoek begrensd), want Slate
	 * kent geen clip per teken-element zonder een eigen laag.
	 */
	void GridHatch(
		FSlateWindowElementList& Out, int32 Layer, const FPaintGeometry& Paint,
		const FVector2f& TopLeft, const FVector2f& Size,
		float Spacing, float Thickness, const FLinearColor& Color, bool bMirror = false)
	{
		const float X0 = TopLeft.X;
		const float Y0 = TopLeft.Y;
		const float X1 = TopLeft.X + Size.X;
		const float Y1 = TopLeft.Y + Size.Y;
		if (X1 <= X0 || Y1 <= Y0 || Spacing <= 0.0f)
		{
			return;
		}

		// EEN LIJN ONDER 45 GRADEN, ALGEBRAISCH GEKNIPT.
		//
		// Niet met de hand op de hoeken afknijpen (de eerste versie deed dat en
		// liet lijnen buiten de tegel doorlopen): een schuine lijn die je per as
		// clampt, verschuift van richting. In plaats daarvan het snijinterval
		// uitrekenen en alleen dat stuk tekenen.
		//
		//   !bMirror  x + y = c, dus y = c - x  -> x in [c-Y1, c-Y0]
		//    bMirror  x - y = c, dus y = x - c  -> x in [c+Y0, c+Y1]
		const float First = bMirror ? (X0 - Y1) : (X0 + Y0);
		const float Last = bMirror ? (X1 - Y0) : (X1 + Y1);

		for (float C = First; C <= Last; C += Spacing)
		{
			const float LowX = bMirror ? FMath::Max(X0, C + Y0) : FMath::Max(X0, C - Y1);
			const float HighX = bMirror ? FMath::Min(X1, C + Y1) : FMath::Min(X1, C - Y0);
			if (HighX - LowX < 1.0f)
			{
				continue;
			}
			const FVector2f From(LowX, bMirror ? LowX - C : C - LowX);
			const FVector2f To(HighX, bMirror ? HighX - C : C - HighX);
			GridLine(Out, Layer, Paint, From, To, Color, Thickness);
		}
	}

	/** Een onderbroken omtrek: het silhouet van een LEEG slot. */
	void GridDashedRect(
		FSlateWindowElementList& Out, int32 Layer, const FPaintGeometry& Paint,
		const FVector2f& TopLeft, const FVector2f& Size, const FLinearColor& Color, float Thickness)
	{
		constexpr float Dash = 11.0f;
		constexpr float Gap = 7.0f;
		const float Right = TopLeft.X + Size.X;
		const float Bottom = TopLeft.Y + Size.Y;

		for (float X = TopLeft.X; X < Right; X += Dash + Gap)
		{
			const float End = FMath::Min(X + Dash, Right);
			GridLine(Out, Layer, Paint, FVector2f(X, TopLeft.Y), FVector2f(End, TopLeft.Y), Color, Thickness);
			GridLine(Out, Layer, Paint, FVector2f(X, Bottom), FVector2f(End, Bottom), Color, Thickness);
		}
		for (float Y = TopLeft.Y; Y < Bottom; Y += Dash + Gap)
		{
			const float End = FMath::Min(Y + Dash, Bottom);
			GridLine(Out, Layer, Paint, FVector2f(TopLeft.X, Y), FVector2f(TopLeft.X, End), Color, Thickness);
			GridLine(Out, Layer, Paint, FVector2f(Right, Y), FVector2f(Right, End), Color, Thickness);
		}
	}
}

/**
 * Het blad dat verft. Alles wat het weet komt uit `FEclipseBaseView`; het stelt
 * geen enkele vraag aan de wereld.
 */
class SEclipseBaseGrid : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SEclipseBaseGrid) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SetCanTick(false);
	}

	void SetGrid(const EclipseBaseView::FEclipseBaseView& InView)
	{
		View = InView;
		Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
	}

	int32 GetDrawnTileCount() const { return DrawnTiles; }

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		// De maat volgt het RASTER en niet andersom. Een vaste maat zou bij acht
		// slots de onderste rij afsnijden, en dat is precies de klasse fout die
		// het kaartframe vond (de graaf duwde de aanbodknoppen van het scherm).
		if (View.Slots.IsEmpty())
		{
			return FVector2D::ZeroVector;
		}

		int32 Columns = 1;
		int32 Rows = 1;
		for (const EclipseBaseView::FEclipseBaseSlotView& Slot : View.Slots)
		{
			Columns = FMath::Max(Columns, Slot.Column + 1);
			Rows = FMath::Max(Rows, Slot.Row + 1);
		}
		return FVector2D(
			Columns * TileWidth + (Columns - 1) * TileGap,
			Rows * TileHeight + (Rows - 1) * TileGap);
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
		DrawnTiles = 0;
		if (View.Slots.IsEmpty())
		{
			// NIETS TEKENEN IS HIER HET JUISTE. `ComputeDesiredSize` vraagt dan 0x0
			// en het blad claimt geen enkele pixel — verborgen is niet afwezig, en
			// een leeg vlak van 176x112 zou een vierkant op het frame achterlaten
			// dat niemand kan verklaren (de HUD-plaat deed dat, 21x21 px).
			return LayerId;
		}

		const FPaintGeometry Paint = AllottedGeometry.ToPaintGeometry();
		int32 Layer = LayerId;

		for (const EclipseBaseView::FEclipseBaseSlotView& Slot : View.Slots)
		{
			const FVector2f Origin(
				Slot.Column * (TileWidth + TileGap),
				Slot.Row * (TileHeight + TileGap));
			DrawTile(OutDrawElements, Layer, AllottedGeometry, Paint, Origin, Slot);
			++DrawnTiles;
		}

		return Layer + 8;
	}

private:
	using EEclipseSlotStatus = EclipseBaseView::EEclipseSlotStatus;

	void DrawTile(
		FSlateWindowElementList& Out, int32 Layer, const FGeometry& Geometry, const FPaintGeometry& Paint,
		const FVector2f& Origin, const EclipseBaseView::FEclipseBaseSlotView& Slot) const
	{
		const FVector2f Size(TileWidth, TileHeight);
		const FLinearColor Accent = GridColorForStatus(Slot.Status);
		const bool bSealed = Slot.Status == EEclipseSlotStatus::Locked;

		// --- 1. DE INKTRAND. Eerst het grotere zwarte vlak, dan de vulling erop:
		// dat is de dikke inktrand van 15.5, met de hand omdat een post-effect een
		// widget per definitie nooit haalt.
		GridFillRect(Out, Layer, Geometry,
			Origin - FVector2f(InkBorder, InkBorder),
			Size + FVector2f(InkBorder * 2.0f, InkBorder * 2.0f), GridInk);

		// --- 2. DE VULLING, met TEXTUUR. Een plat vlak is precies wat de
		// Borderlands-taal niet is; een paar donkerder scanlijnen geven het
		// oppervlak een richting zonder de tekst erop te storen.
		GridFillRect(Out, Layer + 1, Geometry, Origin, Size, bSealed ? GridTileFillSealed : GridTileFill);
		for (float Y = 6.0f; Y < Size.Y; Y += 9.0f)
		{
			GridLine(Out, Layer + 1, Paint,
				Origin + FVector2f(1.0f, Y), Origin + FVector2f(Size.X - 1.0f, Y),
				FLinearColor(0.0f, 0.0f, 0.0f, 0.16f), 1.0f);
		}

		// --- 3. DE SCHUINE HOEKEN. Bijna niets recht: de linkerbovenhoek en de
		// rechteronderhoek worden weggesneden met inkt, zodat de tegel een
		// hoekige kaart is en geen rechthoekje.
		DrawBevel(Out, Layer + 2, Paint, Origin, Size);

		// --- 4. HET SILHOUET PER STATUS ------------------------------------
		switch (Slot.Status)
		{
		case EEclipseSlotStatus::Locked:
			// DICHTE KRUISARCERING: hier kan niets. Twee richtingen, want één
			// richting leest als "in aanbouw".
			GridHatch(Out, Layer + 3, Paint, Origin, Size, 13.0f, 2.0f, FLinearColor(GridSealed.R, GridSealed.G, GridSealed.B, 0.55f), false);
			GridHatch(Out, Layer + 3, Paint, Origin, Size, 13.0f, 2.0f, FLinearColor(GridSealed.R, GridSealed.G, GridSealed.B, 0.55f), true);

			// EN EEN PLAAT ONDER DE TEKST, binnen de tegel.
			//
			// GEMETEN 01-08 op `HUD_hub_slotvormen.png`: dit was de ENIGE tegel
			// die `measure_text_contrast.py` niet kon meten — "geen
			// achtergrondpixels over na halo-uitsluiting", omdat de arcering
			// overal doorheen loopt en er dus geen ondergrond meer over is om
			// tegen af te zetten. Dat is niet alleen een meetprobleem: het is
			// exact de bevinding die de schermplaat heeft afgedwongen, hier
			// terug op een halve vierkante centimeter.
			//
			// De arcering blijft onder de band zichtbaar, dus de VORM zegt nog
			// steeds "dichtgemetseld" — alleen niet meer dwars door de letters.
			GridFillRect(Out, Layer + 4, Geometry,
				Origin + FVector2f(3.0f, 3.0f),
				FVector2f(Size.X - 6.0f, 74.0f),
				FLinearColor(GridTileFillSealed.R, GridTileFillSealed.G, GridTileFillSealed.B, 0.97f));
			break;

		case EEclipseSlotStatus::Empty:
			// ONDERBROKEN OMTREK: een omlijnd gat. De tegel is er, de inhoud niet.
			GridDashedRect(Out, Layer + 3, Paint, Origin + FVector2f(5.0f, 5.0f), Size - FVector2f(10.0f, 10.0f), Accent, 2.0f);
			break;

		case EEclipseSlotStatus::UnderConstruction:
			DrawProgress(Out, Layer + 3, Paint, Origin, Size, Slot);
			break;

		case EEclipseSlotStatus::Damaged:
			// ZWARE STRIEMEN: kapot leest als kapot, ook zonder kleur.
			GridLine(Out, Layer + 3, Paint, Origin + FVector2f(6.0f, 6.0f), Origin + Size - FVector2f(6.0f, 6.0f), GridInk, 9.0f);
			GridLine(Out, Layer + 4, Paint, Origin + FVector2f(6.0f, 6.0f), Origin + Size - FVector2f(6.0f, 6.0f), GridDamaged, 5.0f);
			GridLine(Out, Layer + 3, Paint, Origin + FVector2f(Size.X - 6.0f, 6.0f), Origin + FVector2f(6.0f, Size.Y - 6.0f), GridInk, 9.0f);
			GridLine(Out, Layer + 4, Paint, Origin + FVector2f(Size.X - 6.0f, 6.0f), Origin + FVector2f(6.0f, Size.Y - 6.0f), GridDamaged, 5.0f);
			break;

		default:
			// ONLINE: een dikke accentbalk langs de linkerrand. Massief, want dit
			// is de enige toestand die daadwerkelijk levert.
			GridFillRect(Out, Layer + 3, Geometry, Origin + FVector2f(4.0f, 4.0f), FVector2f(8.0f, Size.Y - 8.0f), Accent);
			break;
		}

		// --- 5. DE TEKST ----------------------------------------------------
		const int32 TextLayer = Layer + 5;
		const float FullWidth = Size.X - TilePadX * 2.0f;

		// HET GROTE VETTE GETAL EERST, want het bepaalt hoeveel ruimte de naam
		// ernaast nog heeft. Bij een gebouwde faciliteit is dat het NIVEAU, bij
		// een bouw het percentage — in beide gevallen het getal dat je in één
		// oogopslag wilt hebben (`REFERENTIE_HUD_BORDERLANDS.md` r49).
		const FString BigNumber = BigNumberFor(Slot);
		const float BigWidth = BigNumber.IsEmpty() ? 0.0f : GridTextWidth(BigNumber, LevelFont) + 8.0f;
		if (!BigNumber.IsEmpty())
		{
			GridInkedText(Out, TextLayer, Geometry, BigNumber,
				Origin + FVector2f(Size.X - TilePadX - GridTextWidth(BigNumber, LevelFont), 22.0f),
				Accent, LevelFont);
		}

		// De slotnaam klein bovenaan: hij zegt WAAR je bent, niet WAT er staat.
		GridInkedText(Out, TextLayer, Geometry,
			GridFitText(Slot.SlotName.ToString().ToUpper(), SlotLabelFont, false, FullWidth),
			Origin + FVector2f(TilePadX, 7.0f), GridDim, SlotLabelFont, /*bBold*/ false);

		// De faciliteitsnaam is de kop van de tegel. ZONDER het niveau: dat staat
		// al als groot getal rechts, en op het eerste frame stond "COMMAND CENTER
		// L1" naast een tweede "L1" — dezelfde waarde twee keer, en samen te breed.
		GridInkedText(Out, TextLayer, Geometry,
			GridFitText(NameLineFor(Slot).ToUpper(), FacilityFont, true, FullWidth - BigWidth),
			Origin + FVector2f(TilePadX, 26.0f), Accent, FacilityFont);

		// De statusregel, en daaronder wat er te beslissen valt.
		GridInkedText(Out, TextLayer, Geometry,
			GridFitText(Slot.StatusText.ToString(), DetailFont, false, FullWidth),
			Origin + FVector2f(TilePadX, 62.0f), GridBone, DetailFont, /*bBold*/ false);

		if (!Slot.CrewText.IsEmpty())
		{
			GridInkedText(Out, TextLayer, Geometry,
				GridFitText(Slot.CrewText.ToString(), DetailFont, false, FullWidth),
				Origin + FVector2f(TilePadX, 80.0f), GridDim, DetailFont, /*bBold*/ false);
		}

		// DE RUSHPRIJS STAAT OP DE TEGEL, want dat is waar de beslissing valt —
		// niet in een paneel ernaast (§2.3 rij 3).
		if (Slot.bCanRush)
		{
			GridInkedText(Out, TextLayer, Geometry,
				GridFitText(Slot.RushText.ToString(), DetailFont, true, FullWidth),
				Origin + FVector2f(TilePadX, 98.0f),
				Slot.bRushAffordable ? GridBuilding : GridDamaged, DetailFont);
		}
	}

	/**
	 * De naamregel zonder het niveau — dat staat als groot getal rechts.
	 * `HeaderText` draagt het wél (die regel wordt ook buiten dit raster
	 * gebruikt), dus hier de kale faciliteitsnaam als die er is.
	 */
	static FString NameLineFor(const EclipseBaseView::FEclipseBaseSlotView& Slot)
	{
		return Slot.FacilityName.IsEmpty() ? Slot.HeaderText.ToString() : Slot.FacilityName.ToString();
	}

	/** Het getal dat van een afstand leesbaar moet zijn. */
	static FString BigNumberFor(const EclipseBaseView::FEclipseBaseSlotView& Slot)
	{
		if (Slot.Status == EEclipseSlotStatus::UnderConstruction && Slot.bHasProgress)
		{
			return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Slot.Progress01 * 100.0f));
		}
		if (Slot.Level > 0)
		{
			return FString::Printf(TEXT("L%d"), Slot.Level);
		}
		return FString();
	}

	/** De weggesneden hoeken, in inkt over de vulling heen. */
	static void DrawBevel(FSlateWindowElementList& Out, int32 Layer, const FPaintGeometry& Paint, const FVector2f& Origin, const FVector2f& Size)
	{
		// Een diagonale inktlijn over de hoek snijdt hem visueel weg. Dik genoeg
		// dat de hoek echt verdwijnt in plaats van een streepje te krijgen.
		for (int32 Step = 0; Step < 13; ++Step)
		{
			const float Depth = BevelDepth - Step * 1.9f;
			if (Depth <= 0.0f)
			{
				break;
			}
			GridLine(Out, Layer, Paint,
				Origin + FVector2f(0.0f, Depth), Origin + FVector2f(Depth, 0.0f), GridInk, 2.4f);
			GridLine(Out, Layer, Paint,
				Origin + FVector2f(Size.X - Depth, Size.Y), Origin + FVector2f(Size.X, Size.Y - Depth), GridInk, 2.4f);
		}
	}

	/**
	 * DE ETA ALS VOORTGANG (§2.3 rij 2): een balk die vult zegt iets anders dan
	 * "3 dagen". De gevarenstrepen zitten op het deel dat nog MOET — zo draagt
	 * ook hier de vorm de toestand en niet alleen de kleur.
	 */
	static void DrawProgress(
		FSlateWindowElementList& Out, int32 Layer, const FPaintGeometry& Paint,
		const FVector2f& Origin, const FVector2f& Size, const EclipseBaseView::FEclipseBaseSlotView& Slot)
	{
		const FVector2f BarOrigin = Origin + FVector2f(6.0f, Size.Y - 15.0f);
		const FVector2f BarSize(Size.X - 12.0f, 9.0f);

		// De baan zelf, altijd zichtbaar: zonder baan weet je niet hoe ver het
		// nog is, alleen hoe ver het al is.
		const TArray<FVector2f> Track = { BarOrigin + FVector2f(0.0f, BarSize.Y * 0.5f), BarOrigin + FVector2f(BarSize.X, BarSize.Y * 0.5f) };
		FSlateDrawElement::MakeLines(Out, Layer, Paint, Track, ESlateDrawEffect::None, GridInk, true, BarSize.Y + 4.0f);

		if (!Slot.bHasProgress)
		{
			// GEEN NOEMER, GEEN BALK. De dagen staan in de statusregel; een balk op
			// een geraden totaal zou een voortgang tonen die niemand heeft gemeten.
			GridHatch(Out, Layer + 1, Paint, BarOrigin, BarSize, 7.0f, 2.0f, GridDim, false);
			return;
		}

		const float Filled = BarSize.X * FMath::Clamp(Slot.Progress01, 0.0f, 1.0f);
		if (Filled > 1.0f)
		{
			const TArray<FVector2f> Done = { BarOrigin + FVector2f(0.0f, BarSize.Y * 0.5f), BarOrigin + FVector2f(Filled, BarSize.Y * 0.5f) };
			FSlateDrawElement::MakeLines(Out, Layer + 1, Paint, Done, ESlateDrawEffect::None, GridBuilding, true, BarSize.Y);
		}
		if (Filled < BarSize.X - 1.0f)
		{
			GridHatch(Out, Layer + 1, Paint,
				BarOrigin + FVector2f(Filled, 0.0f), FVector2f(BarSize.X - Filled, BarSize.Y),
				7.0f, 2.0f, FLinearColor(GridBuilding.R, GridBuilding.G, GridBuilding.B, 0.45f), false);
		}
	}

	EclipseBaseView::FEclipseBaseView View;
	mutable int32 DrawnTiles = 0;
};

UEclipseBaseGridWidget::UEclipseBaseGridWidget()
{
	bIsVariable = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UEclipseBaseGridWidget::RebuildWidget()
{
	Grid = SNew(SEclipseBaseGrid);
	Grid->SetGrid(PendingView);
	return Grid.ToSharedRef();
}

void UEclipseBaseGridWidget::SetGrid(const EclipseBaseView::FEclipseBaseView& InView)
{
	PendingView = InView;
	if (Grid.IsValid())
	{
		Grid->SetGrid(InView);
	}
}

int32 UEclipseBaseGridWidget::GetDrawnTileCount() const
{
	return Grid.IsValid() ? Grid->GetDrawnTileCount() : 0;
}

void UEclipseBaseGridWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Grid.Reset();
}

#undef LOCTEXT_NAMESPACE
