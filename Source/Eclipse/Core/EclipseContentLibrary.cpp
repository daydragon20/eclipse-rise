#include "Core/EclipseContentLibrary.h"

#include "Eclipse.h"

FGameplayTag UEclipseContentLibrary::RequestTag(FName TagName)
{
	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(TagName, /*ErrorIfNotFound*/ false);
	if (!Tag.IsValid())
	{
		UE_LOG(LogEclipse, Error, TEXT("RequestTag: '%s' is not a registered gameplay tag."), *TagName.ToString());
	}
	return Tag;
}
