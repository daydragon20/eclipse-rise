#include "UI/EclipseMissionHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Quests/EclipseMissionTypes.h"
#include "Squad/EclipseSquadSubsystem.h"

void UEclipseMissionHudWidget::NativeConstruct()
{
	Super::NativeConstruct();

	Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionHudRoot"));
	WidgetTree->RootWidget = Root;

	if (UEclipseEventBusSubsystem* Bus = GetGameInstance() != nullptr ? GetGameInstance()->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		EventsHandle = Bus->Subscribe(
			FGameplayTag::RequestGameplayTag(TEXT("Event")),
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseMissionHudWidget::OnAnyFact));
	}

	Rebuild();
}

void UEclipseMissionHudWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
		{
			Bus->Unsubscribe(EventsHandle);
		}
	}
	Super::NativeDestruct();
}

void UEclipseMissionHudWidget::OnAnyFact(FGameplayTag /*EventTag*/, const FInstancedStruct& /*Payload*/)
{
	Rebuild();
}

void UEclipseMissionHudWidget::Rebuild()
{
	if (Root == nullptr)
	{
		return;
	}
	Root->ClearChildren();

	auto AddLine = [this](const FString& Text)
	{
		UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Row->SetText(FText::FromString(Text));
		Root->AddChildToVerticalBox(Row);
	};

	const UGameInstance* GameInstance = GetGameInstance();
	if (const UEclipseMissionSubsystem* Mission = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseMissionSubsystem>() : nullptr)
	{
		AddLine(FString::Printf(TEXT("== MISSION  [%s] =="),
			*UEnum::GetValueAsString(Mission->GetPhase()).RightChop(FString(TEXT("EEclipseMissionPhase::")).Len())));
		const TArray<FName>& Done = Mission->GetCompletedObjectiveIds();
		for (const FEclipseObjectiveDef& Obj : Mission->GetActiveObjectives())
		{
			const bool bDone = Done.Contains(Obj.ObjectiveId);
			AddLine(FString::Printf(TEXT("  [%s] %s"), bDone ? TEXT("X") : TEXT(" "), *Obj.Description.ToString()));
		}
	}

	if (const UWorld* World = GetWorld())
	{
		if (const UEclipseSquadSubsystem* Squad = World->GetSubsystem<UEclipseSquadSubsystem>())
		{
			AddLine(TEXT("-- squad orders --"));
			for (const FString& Line : Squad->GetOrderStateLines())
			{
				AddLine(FString::Printf(TEXT("  %s"), *Line));
			}
		}
	}

	AddLine(TEXT("[1] Move  [2] Focus  [3] Hold  [4] Regroup   LMB fire"));
}
