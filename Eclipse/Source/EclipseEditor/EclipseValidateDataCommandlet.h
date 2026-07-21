#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "EclipseValidateDataCommandlet.generated.h"

/**
 * CI entry point for content data validation (GDD 12.2 rule 3: DataAssets validated on load,
 * with a ValidateData commandlet in CI). Exists from day one so the CI contract never has to
 * be retrofitted; validators register per DataAsset schema as Phase 1 systems land.
 */
UCLASS()
class UEclipseValidateDataCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
