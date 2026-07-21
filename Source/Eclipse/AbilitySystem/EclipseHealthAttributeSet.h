#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "EclipseHealthAttributeSet.generated.h"

#define ECLIPSE_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Health attributes (SPEC-P1-05: "GAS-based health/damage attributes from day
 * one" — GDD 12.1 core architecture decision). Phase 1 carries Health only;
 * Stamina/Armor/Suppression/Morale join per GDD 12.3 as their systems land.
 */
UCLASS()
class ECLIPSE_API UEclipseHealthAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UEclipseHealthAttributeSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Attributes")
	FGameplayAttributeData Health;
	ECLIPSE_ATTRIBUTE_ACCESSORS(UEclipseHealthAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Attributes")
	FGameplayAttributeData MaxHealth;
	ECLIPSE_ATTRIBUTE_ACCESSORS(UEclipseHealthAttributeSet, MaxHealth)

	/** Meta attribute: incoming damage funnels through here and drains Health in PostGameplayEffectExecute. */
	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Attributes")
	FGameplayAttributeData IncomingDamage;
	ECLIPSE_ATTRIBUTE_ACCESSORS(UEclipseHealthAttributeSet, IncomingDamage)
};
