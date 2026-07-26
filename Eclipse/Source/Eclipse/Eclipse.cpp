#include "Eclipse.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogEclipse);

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, Eclipse, "Eclipse");

namespace
{
	/** Categorie -> aantal. Eén regel per soort, want honderd keer dezelfde melding
	 *  is één bevinding en geen honderd. */
	TMap<FString, int32> GDegradationCounts;

	/** Het eerste voorbeeld per categorie; genoeg om te weten waar je moet kijken. */
	TMap<FString, FString> GDegradationFirstDetail;
}

void EclipseDegradation::Note(const TCHAR* Category, const FString& Detail)
{
	const FString Key(Category);
	int32& Count = GDegradationCounts.FindOrAdd(Key);
	++Count;
	if (!GDegradationFirstDetail.Contains(Key))
	{
		GDegradationFirstDetail.Add(Key, Detail);
	}
}

int32 EclipseDegradation::Count()
{
	int32 Total = 0;
	for (const TPair<FString, int32>& Pair : GDegradationCounts)
	{
		Total += Pair.Value;
	}
	return Total;
}

TArray<FString> EclipseDegradation::Report()
{
	TArray<FString> Lines;
	for (const TPair<FString, int32>& Pair : GDegradationCounts)
	{
		const FString* First = GDegradationFirstDetail.Find(Pair.Key);
		Lines.Add(FString::Printf(TEXT("%s: %d x   eerste: %s"),
			*Pair.Key, Pair.Value, First != nullptr ? **First : TEXT("-")));
	}
	Lines.Sort();
	return Lines;
}

void EclipseDegradation::Reset()
{
	GDegradationCounts.Reset();
	GDegradationFirstDetail.Reset();
}
