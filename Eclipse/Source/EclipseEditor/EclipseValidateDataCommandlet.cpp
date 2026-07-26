#include "EclipseValidateDataCommandlet.h"
#include "EclipseDataValidators.h"
#include "EclipseEditor.h"

int32 UEclipseValidateDataCommandlet::Main(const FString& Params)
{
	// Validator registry (GDD 12.2 rule 3): one entry per data schema. Returning
	// success on an empty *asset set* is correct (pre-content), returning success
	// on skipped validation is not — every registered schema always runs.
	TArray<FString> Errors;
	int32 AssetsChecked = 0;
	int32 ValidatorCount = 0;

	int32 Checked = 0;
	EclipseDataValidators::ValidateRegionGraphAssets(Errors, Checked);
	AssetsChecked += Checked;
	++ValidatorCount;

	EclipseDataValidators::ValidateProductionItemTables(Errors, Checked);
	AssetsChecked += Checked;
	++ValidatorCount;

	EclipseDataValidators::ValidateClassDefTables(Errors, Checked);
	AssetsChecked += Checked;
	++ValidatorCount;

	EclipseDataValidators::ValidateLiberationTables(Errors, Checked);
	AssetsChecked += Checked;
	++ValidatorCount;

	EclipseDataValidators::ValidateWeaponTables(Errors, Checked);
	AssetsChecked += Checked;
	++ValidatorCount;

	EclipseDataValidators::ValidateLoadoutTables(Errors, Checked);
	AssetsChecked += Checked;
	++ValidatorCount;

	EclipseDataValidators::ValidateBodyDefTables(Errors, Checked);
	AssetsChecked += Checked;
	++ValidatorCount;

	for (const FString& Error : Errors)
	{
		UE_LOG(LogEclipseEditor, Error, TEXT("ValidateData: %s"), *Error);
	}

	UE_LOG(LogEclipseEditor, Display, TEXT("ValidateData: %d validators registered, %d assets checked, %d errors."),
		ValidatorCount, AssetsChecked, Errors.Num());

	return Errors.IsEmpty() ? 0 : 1;
}
