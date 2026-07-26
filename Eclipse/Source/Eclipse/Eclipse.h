#pragma once

#include "CoreMinimal.h"

// Project-wide log category. Subsystems add their own categories as they land (GDD 12.2).
ECLIPSE_API DECLARE_LOG_CATEGORY_EXTERN(LogEclipse, Log, All);

/**
 * STILLE DEGRADATIE TELLEN (owner-opdracht 26-07, 21:30).
 *
 * Aanleiding, letterlijk: "~24 uur werk, 152 groene tests, en toen ik de game
 * startte was er op het eerste gezicht niets veranderd." Drie lagen tussen de
 * code en het scherm waren stuk en geen enkele test werd rood — de skeletpoort
 * wees 2948 animaties af, de garbage collector ruimde de geluiden op, en het
 * personage verdween.
 *
 * De oorzaak is structureel: 14.3.5 zegt "degradeer LUID", en dat is jarenlang
 * gelezen als "log een waarschuwing". Een waarschuwing in een log dat niemand
 * leest is geen luid falen — het is stil falen met een alibi.
 *
 * Vanaf nu telt elke degradatie mee. Een test die een echte speelronde draait
 * eist dat de teller op NUL staat, dus een afgewezen animatie of een
 * niet-geladen geluid maakt de bar rood in plaats van een regel te produceren.
 *
 * Gebruik: roep EclipseDegradation::Note() aan op elk pad dat vandaag een
 * waarschuwing logt en daarna doorgaat met minder dan beloofd.
 */
namespace EclipseDegradation
{
	/** Meld dat er iets stils is misgegaan. Categorie is vrije tekst voor het rapport. */
	ECLIPSE_API void Note(const TCHAR* Category, const FString& Detail);

	/** Hoeveel er sinds de laatste Reset() gemeld is. */
	ECLIPSE_API int32 Count();

	/** Per categorie, voor een leesbaar rapport in de testuitvoer. */
	ECLIPSE_API TArray<FString> Report();

	/** Begin van een meting: zet de teller op nul. */
	ECLIPSE_API void Reset();
}
