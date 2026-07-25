#include "Quests/EclipseObjectiveTrigger.h"

#include "Characters/EclipseCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"
#include "Quests/EclipseMissionSubsystem.h"

AEclipseObjectiveTrigger::AEclipseObjectiveTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	SetRootComponent(TriggerBox);
	TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 150.0f));
	TriggerBox->SetCollisionProfileName(TEXT("Trigger"));
	TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AEclipseObjectiveTrigger::HandleOverlap);
}

void AEclipseObjectiveTrigger::HandleOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	// Only the player's side advances objectives; enemies wandering through a
	// site must not complete the mission for us.
	const AEclipseCharacter* Character = Cast<AEclipseCharacter>(OtherActor);
	if (Character == nullptr || Character->IsDowned() || !Character->IsPlayerSide())
	{
		return;
	}

	if (UEclipseMissionSubsystem* Mission = GetGameInstance()->GetSubsystem<UEclipseMissionSubsystem>())
	{
		// NotifySiteEntered en niet CompleteObjectiveByTarget: de trigger weet dat
		// er iemand STAAT, niet dat er iets gedaan is. Het type bepaalt of dat
		// genoeg is, en dat weet de missie-runtime (zie NotifySiteEntered).
		Mission->NotifySiteEntered(SiteId);
	}
}
