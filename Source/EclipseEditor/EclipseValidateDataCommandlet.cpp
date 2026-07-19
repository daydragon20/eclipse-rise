#include "EclipseValidateDataCommandlet.h"
#include "EclipseEditor.h"

int32 UEclipseValidateDataCommandlet::Main(const FString& Params)
{
	// PLACEHOLDER(GDD 12.2 rule 3): validator registry is empty until the first Phase 1
	// DataAsset schema lands (SPEC-P1-02 campaign setup asset). The commandlet must fail
	// the build on any invalid asset once validators exist; returning success on an empty
	// registry is correct, returning success on skipped validation is not.
	UE_LOG(LogEclipseEditor, Display, TEXT("ValidateData: 0 validators registered, 0 assets checked (pre-Phase 1)."));
	return 0;
}
