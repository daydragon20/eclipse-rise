#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EclipseObjectiveTrigger.generated.h"

class UBoxComponent;

/**
 * Data-driven objective site (SPEC-P1-05: objective primitives as components —
 * one trigger class serves ReachLocation/CollectItem/ExtractSquad; DestroyTarget
 * binds to a destructible target's OnDowned instead). The level tags sites by
 * actor label; missions bind objectives by TargetId (GDD 11.3: generation
 * composes authored spaces).
 */
UCLASS()
class ECLIPSE_API AEclipseObjectiveTrigger : public AActor
{
	GENERATED_BODY()

public:
	AEclipseObjectiveTrigger();

	/** Site id missions reference via FEclipseObjectiveDef::TargetId. */
	UPROPERTY(EditAnywhere, Category = "Eclipse|Mission")
	FName SiteId;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Eclipse|Mission")
	TObjectPtr<UBoxComponent> TriggerBox;
};
