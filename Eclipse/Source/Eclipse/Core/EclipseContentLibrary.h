#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EclipseContentLibrary.generated.h"

/**
 * Helpers for content tooling (Python bootstrap scripts, editor utilities).
 * FGameplayTag is intentionally opaque to scripting; this is the sanctioned
 * construction path for data pipelines.
 */
UCLASS()
class ECLIPSE_API UEclipseContentLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Resolve a registered tag by name; invalid names log and return an empty tag (GDD 14.3.5: loud, not fatal). */
	UFUNCTION(BlueprintPure, Category = "Eclipse|Content")
	static FGameplayTag RequestTag(FName TagName);
};
