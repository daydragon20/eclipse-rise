#include "UI/EclipseStrategyMapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "Strategy/EclipseStrategySubsystem.h"

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

void UEclipseStrategyMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UEclipseEventBusSubsystem* Bus = GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>())
	{
		// Re-render on any strategy fact: consequence visibility is the whole
		// point of the board (SPEC-P1-04 "the map noticed").
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
	if (RootBox == nullptr)
	{
		RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MapRoot"));
		WidgetTree->RootWidget = RootBox;
	}
	RootBox->ClearChildren();

	const UEclipseCampaignSubsystem* Campaign = GetGameInstance()->GetSubsystem<UEclipseCampaignSubsystem>();
	UEclipseStrategySubsystem* Strategy = GetGameInstance()->GetSubsystem<UEclipseStrategySubsystem>();
	if (Campaign == nullptr || Strategy == nullptr)
	{
		return;
	}

	UTextBlock* Header = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Header->SetText(FText::FromString(FString::Printf(TEXT("DISTRICT BOARD — Day %d"), Campaign->GetState().Day)));
	RootBox->AddChildToVerticalBox(Header);

	for (const FEclipseRegionState& Region : Campaign->GetState().Regions)
	{
		const TCHAR* OwnerLabel =
			Region.Owner == EEclipseRegionOwner::Player ? TEXT("PLAYER")
			: Region.Owner == EEclipseRegionOwner::Contested ? TEXT("CONTESTED") : TEXT("DOMINION");

		UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Row->SetText(FText::FromString(FString::Printf(TEXT("  %s — %s | garrison %d | unrest %d"),
			*Region.RegionId.ToString(), OwnerLabel, Region.GarrisonStrength, Region.Unrest)));

		// Owner readable by color, not only text (debug-grade, still readable).
		Row->SetColorAndOpacity(FSlateColor(
			Region.Owner == EEclipseRegionOwner::Player ? FLinearColor(0.3f, 0.9f, 0.4f)
			: Region.Owner == EEclipseRegionOwner::Contested ? FLinearColor(0.95f, 0.75f, 0.2f)
			: FLinearColor(0.9f, 0.3f, 0.25f)));
		RootBox->AddChildToVerticalBox(Row);
	}

	for (const FEclipseMissionOfferView& Offer : Strategy->GetAvailableOffers())
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

void UEclipseStrategyMapWidget::HandleOfferPicked(FName RegionId)
{
	UEclipseStrategySubsystem* Strategy = GetGameInstance()->GetSubsystem<UEclipseStrategySubsystem>();
	FString Error;
	if (Strategy != nullptr && !Strategy->SelectMission(RegionId, Error))
	{
		UE_LOG(LogEclipse, Warning, TEXT("Map: offer pick rejected: %s"), *Error);
	}
}
