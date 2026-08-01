#include "UI/EclipseScreenPlate.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"

namespace EclipseScreenPlate
{
namespace
{
	/** 3 px inkt rondom de vulling: de dikke lijn uit §15.5, hier in Slate. */
	constexpr float InkThickness = 3.0f;

	/** 3 px inkt + lucht, zodat letters de rand van hun eigen plaat niet raken. */
	const FMargin ContentPadding(11.0f, 9.0f, 11.0f, 9.0f);
}

FLinearColor InkColor()
{
	// FromSRGBColor omdat Slate een FLinearColor als LINEAIR leest: 0,055
	// lineair is 64 van de 255, niet 14.
	return FLinearColor::FromSRGBColor(FColor(5, 5, 7, 255));
}

FLinearColor FillColor()
{
	// Alfa 209 van 255: de wereld blijft er zwak doorheen staan. Volledig dicht
	// zou de plek weggummen, en de basis is een PLEK (05_base_building.md §5.2)
	// en geen menu op een zwart vlak.
	return FLinearColor::FromSRGBColor(FColor(18, 19, 24, 209));
}

FLinearColor BoneColor()
{
	return FLinearColor(0.93f, 0.90f, 0.83f);
}

UOverlay* Wrap(UWidgetTree& Tree, UWidget& Content, FName Name)
{
	UOverlay* Plate = Tree.ConstructWidget<UOverlay>(UOverlay::StaticClass(), Name);

	UImage* Ink = Tree.ConstructWidget<UImage>(UImage::StaticClass(), FName(*(Name.ToString() + TEXT("Ink"))));
	Ink->SetColorAndOpacity(InkColor());
	if (UOverlaySlot* InkSlot = Plate->AddChildToOverlay(Ink))
	{
		// EXPLICIET Fill. Een UOverlaySlot staat standaard op Left/Top, en dan
		// houdt de plaat zijn eigen borstelmaat van 32x32 aan in plaats van de
		// inhoud te dekken — precies het vierkantje dat op het HUD-reviewframe
		// stond toen die fout er nog in zat.
		InkSlot->SetHorizontalAlignment(HAlign_Fill);
		InkSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UImage* Fill = Tree.ConstructWidget<UImage>(UImage::StaticClass(), FName(*(Name.ToString() + TEXT("Fill"))));
	Fill->SetColorAndOpacity(FillColor());
	if (UOverlaySlot* FillSlot = Plate->AddChildToOverlay(Fill))
	{
		FillSlot->SetHorizontalAlignment(HAlign_Fill);
		FillSlot->SetVerticalAlignment(VAlign_Fill);
		// Positieve inspringing en geen negatieve uitloop, zodat er nergens op
		// een negatieve marge in een Fill-slot vertrouwd wordt.
		FillSlot->SetPadding(FMargin(InkThickness));
	}

	if (UOverlaySlot* ContentSlot = Plate->AddChildToOverlay(&Content))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Fill);
		ContentSlot->SetPadding(ContentPadding);
	}

	return Plate;
}

void ApplyVisibility(UOverlay* Plate, bool bHasContent)
{
	if (Plate == nullptr)
	{
		return;
	}
	// SelfHitTestInvisible en niet Visible: de plaat mag geen klikken opvangen
	// die voor de knoppen erop bedoeld zijn.
	const ESlateVisibility Desired = bHasContent ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
	if (Plate->GetVisibility() != Desired)
	{
		Plate->SetVisibility(Desired);
	}
}

void StyleButton(UButton& Button, UTextBlock& Label, int32 FontSize)
{
	// TINTEN en geen nieuwe borstel: de vorm (afgeronde hoek, randmarges) komt
	// uit de engine-stijl en die was niet het probleem — alleen de kleur.
	FButtonStyle Style = Button.GetStyle();
	Style.Normal.TintColor = FSlateColor(FillColor());
	Style.Hovered.TintColor = FSlateColor(FLinearColor::FromSRGBColor(FColor(38, 40, 50, 235)));
	Style.Pressed.TintColor = FSlateColor(InkColor());
	Style.Disabled.TintColor = FSlateColor(InkColor());
	Button.SetStyle(Style);

	StyleLine(Label, BoneColor(), FontSize, /*bBold*/ true);
}

void StyleLine(UTextBlock& Line, const FLinearColor& Color, int32 FontSize, bool bBold)
{
	Line.SetColorAndOpacity(FSlateColor(Color));

	FSlateFontInfo Font = Line.GetFont();
	Font.Size = FontSize;
	Font.TypefaceFontName = bBold ? TEXT("Bold") : TEXT("Regular");
	Line.SetFont(Font);

	Line.SetShadowOffset(FVector2D(1.0f, 1.0f));
	Line.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));
}
}
