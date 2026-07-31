#include "UI/EclipseStrategyMapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseStrategySubsystem.h"
#include "UI/EclipseStrategyMapLogic.h"

#define LOCTEXT_NAMESPACE "EclipseStrategyMapWidget"

namespace
{
	/**
	 * HET PALET, en het is er één (15.5, één-stijl-wet). Warme bot-inkt voor
	 * gewone tekst, de eigendom-trichotomie in de kleuren die de rest van het
	 * spel er al aan hangt, en per lane-status een eigen kleur — een gepoorte
	 * lane die er hetzelfde uitziet als een open lane leert je niets
	 * (REFERENTIE_BASE_MAP.md §1.4).
	 */
	const FLinearColor InkBone(0.93f, 0.90f, 0.83f);

	// GEMETEN op HUD_hub_kaart.png (1280x720, 01-08): op 0.62 grijs viel de
	// "borders:"-regel weg tegen de lichte muur die achter het bord door loopt.
	// Het bord heeft geen achtergrondplaat, dus elke regel staat op wat er
	// toevallig achter ligt — dan is "gedempt" een kleur die soms verdwijnt.
	const FLinearColor InkDim(0.80f, 0.78f, 0.72f);
	const FLinearColor OwnerPlayer(0.35f, 0.92f, 0.45f);
	const FLinearColor OwnerContested(0.97f, 0.76f, 0.22f);
	const FLinearColor OwnerDominion(0.92f, 0.31f, 0.26f);
	const FLinearColor LaneOpen(0.70f, 0.82f, 0.88f);
	const FLinearColor LaneGated(0.98f, 0.66f, 0.18f);
	const FLinearColor LaneSmuggler(0.74f, 0.53f, 0.95f);
	const FLinearColor Alarm(0.97f, 0.29f, 0.24f);

	FLinearColor ColorForOwner(EEclipseRegionOwner Owner)
	{
		switch (Owner)
		{
		case EEclipseRegionOwner::Player:    return OwnerPlayer;
		case EEclipseRegionOwner::Contested: return OwnerContested;
		default:                             return OwnerDominion;
		}
	}

	/** Status kleurt de lane; militair dicht kleurt hem alarmrood, want dat is het feit dat telt. */
	FLinearColor ColorForLane(const EclipseStrategyMap::FEclipseMapLaneView& Lane)
	{
		if (!Lane.bMilitaryPassable && !Lane.bSmugglerPassable)
		{
			return Alarm;
		}
		switch (Lane.Status)
		{
		case EEclipseLaneStatus::SpireGated:   return Lane.bMilitaryPassable ? LaneGated : Alarm;
		case EEclipseLaneStatus::SmugglerOnly: return LaneSmuggler;
		default:                               return LaneOpen;
		}
	}
}

void UEclipseStrategyOfferButton::Bind(FName InRegionId, TFunction<void(FName)> InOnPicked)
{
	RegionId = InRegionId;
	OnPicked = MoveTemp(InOnPicked);
	OnClicked.AddUniqueDynamic(this, &UEclipseStrategyOfferButton::HandleClicked);
}

void UEclipseStrategyOfferButton::HandleClicked()
{
	if (OnPicked)
	{
		OnPicked(RegionId);
	}
}

void UEclipseStrategyMapWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// DE WORTEL HOORT HIER. Rebuild() maakte hem lui aan, maar Rebuild() draait
	// vanuit NativeConstruct en dat is ná UUserWidget::RebuildWidget — die pakt dan
	// een lege WidgetTree->RootWidget en maakt er een SSpacer van. De kaart stond
	// dus nooit op het scherm, hoeveel rijen Rebuild() er ook in hing. Zelfde
	// oorzaak en reparatie als in UEclipseMissionHudWidget; dit is de hoogte "Map".
	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MapRoot"));
	WidgetTree->RootWidget = RootBox;
}

void UEclipseStrategyMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UEclipseEventBusSubsystem* Bus = GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>())
	{
		// Re-render on any strategy fact: consequence visibility is the whole
		// point of the board (SPEC-P1-04 "the map noticed").
		//
		// GEMETEN 01-08: dit dekt óók Event.Strategy.ResponseTierChanged, want
		// UEclipseEventBusSubsystem::Broadcast loopt de ouderketen van een tag af.
		// Een tweede abonnement op die tag zou de kaart dus TWEE keer laten
		// hertekenen per escalatie, niet één keer meer laten reageren.
		const FGameplayTag StrategyFamily = EclipseTags::Event_Strategy_RegionControlChanged.GetTag().RequestDirectParent();
		RegionChangedHandle = Bus->Subscribe(
			StrategyFamily,
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseStrategyMapWidget::OnBoardChanged));

		// A committed story beat retires/unlocks pins without any Strategy fact
		// (M1.1/M1.2/M1.4 flip no region, decision 6) — the board re-renders on
		// the beat or it shows the retired mission (catalog: BeatReached consumer).
		BeatReachedHandle = Bus->Subscribe(
			EclipseTags::Event_Story_BeatReached,
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseStrategyMapWidget::OnBoardChanged));
	}

	Rebuild();
}

void UEclipseStrategyMapWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
		{
			Bus->Unsubscribe(RegionChangedHandle);
			Bus->Unsubscribe(BeatReachedHandle);
		}
	}

	Super::NativeDestruct();
}

void UEclipseStrategyMapWidget::OnBoardChanged(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	Rebuild();
}

void UEclipseStrategyMapWidget::Rebuild()
{
	// De wortel komt uit NativeOnInitialized. Hem hier alsnog aanmaken zou de fout
	// terugzetten die daar beschreven staat: een wortel die ná RebuildWidget ontstaat
	// bereikt de Slate-boom nooit meer. Ontbreekt hij, dan is er iets fundamenteel
	// mis met de levenscyclus en moet dat luid zijn, niet stil gerepareerd (14.3.5).
	if (RootBox == nullptr)
	{
		UE_LOG(LogEclipse, Warning,
			TEXT("Strategiekaart: geen wortel bij Rebuild — NativeOnInitialized is niet gedraaid; het bord blijft leeg."));
		return;
	}
	RootBox->ClearChildren();

	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GetGameInstance()->GetSubsystem<UEclipseStrategySubsystem>();
	if (Campaign == nullptr || Strategy == nullptr)
	{
		return;
	}

	// DE KOPPELING (N-b). De graaf naast de toestand, in één aanroep: hier stond
	// een lus over Campaign->GetState().Regions, en die structuur heeft vier
	// velden en geen kanten. Wat er op het bord KOMT wordt hieronder niet meer
	// besloten — dat gebeurt in EclipseStrategyMapLogic, headless getoetst.
	static const TArray<FEclipseRegionDefinition> NoDefinitions;
	const UEclipseRegionGraphAsset* Graph = Strategy->GetRegionGraph();
	const FEclipseLaneTuning Tuning = Graph != nullptr ? Graph->LaneTuning : FEclipseLaneTuning();
	const EclipseStrategyMap::FEclipseMapView View = EclipseStrategyMap::ComposeMapView(
		Campaign->GetState(), Graph != nullptr ? Graph->Regions : NoDefinitions, Tuning);

	AddLine(View.HeaderText, InkBone, 18, /*bBold*/ true);

	// EEN KAPOTTE OF ONTBREKENDE GRAAF TEKENT NIETS. Niet de regio's, en ook
	// niet het missie-aanbod — dat aanbod komt uit dezelfde graaf, dus een
	// aanbod tonen boven een graaf die zichzelf tegenspreekt is precies de
	// leugen die de poort tegenhoudt. Het scherm zegt dan wat er mis is.
	if (!View.IsRenderable())
	{
		AddLine(View.StatusText, Alarm, 16, /*bBold*/ true, /*TopPadding*/ 8.0f);

		constexpr int32 MaxShownErrors = 6;
		const int32 ShownErrors = FMath::Min(View.GraphErrors.Num(), MaxShownErrors);
		for (int32 Index = 0; Index < ShownErrors; ++Index)
		{
			AddLine(FText::FromString(View.GraphErrors[Index]), InkDim, 12, /*bBold*/ false);
		}
		if (View.GraphErrors.Num() > ShownErrors)
		{
			AddLine(FText::Format(LOCTEXT("MoreErrors", "  ...and {0} more"),
				FText::AsNumber(View.GraphErrors.Num() - ShownErrors)), InkDim, 12, /*bBold*/ false);
		}

		UE_LOG(LogEclipse, Warning, TEXT("Strategiekaart: de graaf haalt het scherm niet — %s (%d fouten)"),
			*View.StatusText.ToString(), View.GraphErrors.Num());
		return;
	}

	for (const EclipseStrategyMap::FEclipseMapRegionView& Region : View.Regions)
	{
		AddLine(Region.HeaderText, ColorForOwner(Region.Owner), 15, /*bBold*/ true, /*TopPadding*/ 6.0f);

		// De letterlijke eis van N-b: de kaartlaag NOEMT per regio zijn buren.
		AddLine(Region.NeighborsText, InkDim, 12, /*bBold*/ false);

		// En daaronder wat elke buur KOST — status, dagen, risico. De lijst mag
		// blijven bestaan náást de graaf, nooit ervoor in de plaats
		// (REFERENTIE_BASE_MAP.md §1.5); dit is de lijst met topologie erin.
		for (const EclipseStrategyMap::FEclipseMapLaneView& Lane : Region.Lanes)
		{
			AddLine(Lane.Text, ColorForLane(Lane), 13, /*bBold*/ false);
		}
	}

	const TArray<FEclipseMissionOfferView> Offers = Strategy->GetAvailableOffers();
	if (!Offers.IsEmpty())
	{
		AddLine(LOCTEXT("OffersHeader", "AVAILABLE OPERATIONS"), InkBone, 15, /*bBold*/ true, /*TopPadding*/ 12.0f);
	}

	for (const FEclipseMissionOfferView& Offer : Offers)
	{
		UEclipseStrategyOfferButton* Button = WidgetTree->ConstructWidget<UEclipseStrategyOfferButton>(UEclipseStrategyOfferButton::StaticClass());
		Button->Bind(Offer.RegionId, [this](FName RegionId) { HandleOfferPicked(RegionId); });

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(FString::Printf(TEXT("STRIKE %s [%s] — %s"),
			*Offer.RegionId.ToString(), *Offer.TemplateId.ToString(), *Offer.ContextLine.ToString())));
		Button->AddChild(Label);
		RootBox->AddChildToVerticalBox(Button);
	}
}

UTextBlock* UEclipseStrategyMapWidget::AddLine(const FText& Text, const FLinearColor& Color, int32 FontSize, bool bBold, float TopPadding)
{
	UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Line->SetText(Text);
	Line->SetColorAndOpacity(FSlateColor(Color));

	FSlateFontInfo Font = Line->GetFont();
	Font.Size = FontSize;
	Font.TypefaceFontName = bBold ? TEXT("Bold") : TEXT("Regular");
	Line->SetFont(Font);

	// De inktrand (15.5). Een harde zwarte verschuiving van 1 px is wat een
	// gestileerde kaart van een spreadsheet onderscheidt, en het is hetzelfde
	// gebaar als de outline in de wereld — één stijl, niet twee.
	Line->SetShadowOffset(FVector2D(1.0f, 1.0f));
	Line->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.95f));

	// LineSlot en niet Slot: UWidget heeft zelf een `Slot`-lid, en die schaduw is
	// hier een harde fout (C4458 als error).
	if (UVerticalBoxSlot* LineSlot = RootBox->AddChildToVerticalBox(Line))
	{
		LineSlot->SetPadding(FMargin(0.0f, TopPadding, 0.0f, 0.0f));
	}
	return Line;
}

void UEclipseStrategyMapWidget::HandleOfferPicked(FName RegionId)
{
	UEclipseStrategySubsystem* Strategy = GetGameInstance()->GetSubsystem<UEclipseStrategySubsystem>();
	FString Error;
	if (Strategy != nullptr && !Strategy->SelectMission(RegionId, Error))
	{
		UE_LOG(LogEclipse, Warning, TEXT("Map: offer pick rejected: %s"), *Error);
	}
}

#undef LOCTEXT_NAMESPACE
