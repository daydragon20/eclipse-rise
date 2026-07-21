#include "AbilitySystem/EclipseHealthAttributeSet.h"

#include "GameplayEffectExtension.h"

UEclipseHealthAttributeSet::UEclipseHealthAttributeSet()
{
	InitMaxHealth(100.0f);
	InitHealth(100.0f);
	InitIncomingDamage(0.0f);
}

void UEclipseHealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(1.0f, NewValue);
	}
}

void UEclipseHealthAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		// Meta-attribute pattern: damage never edits Health directly, so future
		// mitigation (armor plates, GDD 4.1.4) slots in here without touching
		// any weapon code.
		const float Damage = GetIncomingDamage();
		SetIncomingDamage(0.0f);
		if (Damage > 0.0f)
		{
			SetHealth(FMath::Clamp(GetHealth() - Damage, 0.0f, GetMaxHealth()));
		}
	}
}
