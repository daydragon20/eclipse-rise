#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EclipseCharacter.generated.h"

class AEclipseCharacter;
class UAbilitySystemComponent;
class UEclipseCharacterTuningAsset;
class UEclipseHealthAttributeSet;

DECLARE_MULTICAST_DELEGATE_TwoParams(FEclipseCharacterDownedDelegate, AEclipseCharacter* /*Character*/, FName /*Cause*/);

/**
 * The one character body (GDD 12.3: player, soldiers and enemies share
 * AEclipseCharacter — soldiers must be player-quality; behavior differs by
 * controller and components, never by a divergent class). GAS carries health
 * from day one (SPEC-P1-05).
 */
UCLASS()
class ECLIPSE_API AEclipseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEclipseCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystem; }
	virtual void PostInitializeComponents() override;

	/** Apply tuning (movement speeds, max health) from data — never hardcode (GDD 14.2). */
	void ApplyTuning(const UEclipseCharacterTuningAsset* Tuning);

	/** Direct health init for archetype-driven spawns (enemies from DT_EnemyArchetypes). */
	void InitializeHealth(float MaxHealth);

	/**
	 * Hitscan damage entry point (SPEC-P1-05 minimal combat): routes through the
	 * GAS meta attribute so future mitigation composes.
	 */
	void ApplyDamage(float Amount, AEclipseCharacter* Instigator, FName Cause);

	bool IsDowned() const { return bDowned; }
	float GetHealth() const;

	/** Campaign identity for roster soldiers (invalid for enemies/player in Phase 1). */
	FGuid GetSoldierId() const { return SoldierId; }
	void SetSoldierId(const FGuid& InSoldierId) { SoldierId = InSoldierId; }

	/** Fired once when health reaches zero; mission/squad wiring listens (SPEC-P1-06/07 pipeline). */
	FEclipseCharacterDownedDelegate OnDowned;

private:
	void HandleHealthChanged(const struct FOnAttributeChangeData& Data);

	UPROPERTY(VisibleAnywhere, Category = "Eclipse")
	TObjectPtr<UAbilitySystemComponent> AbilitySystem;

	UPROPERTY()
	TObjectPtr<UEclipseHealthAttributeSet> HealthAttributes;

	FGuid SoldierId;
	FName LastDamageCause;
	bool bDowned = false;
};
