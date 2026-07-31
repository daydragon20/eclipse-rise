#pragma once

#include "CoreMinimal.h"
#include "Quests/EclipseMissionLogic.h"
#include "Strategy/EclipseCampaignTypes.h"

/**
 * Roster generation and casualty policy as pure logic (GDD 14.3.2).
 * Deterministic by seed so campaign starts are reproducible in tests.
 */
namespace EclipseRosterLogic
{
	/** Resolved name/trait pools (the subsystem loads tables; the core sees plain data). */
	struct FEclipseNameGenerationParams
	{
		TArray<FString> FirstNames;
		TArray<FString> LastNames;
		TArray<FName> TraitIds;
	};

	/**
	 * Generate one soldier deterministically. Falls back to a numbered recruit
	 * when pools are empty (missing content = graceful default, GDD 14.3.5) —
	 * but a name is always produced: nameless soldiers violate Pillar 3.
	 *
	 * LET OP: dit is de losse trekking en die weet niets van wie er al bestaat.
	 * Twee opeenvolgende zaden kunnen dus dezelfde voornaam opleveren — GEMETEN in
	 * de verscheepte campagne op 31-07: zaad 1 en 2 gaven allebei "Sef" (Sef Voss en
	 * Sef Chen) in een squad van drie. Wie een ROSTER maakt, hoort GenerateRoster te
	 * gebruiken; deze functie blijft bestaan voor tests en voor losse aanwas.
	 */
	ECLIPSE_API FEclipseSoldierRecord GenerateSoldier(FName OriginId, const FEclipseNameGenerationParams& Params, int32 Seed);

	/**
	 * Een HELE ploeg, met onderscheidbare mensen erin (Pijler 3: "People, Not Units").
	 *
	 * WAAROM DIT BESTAAT. Op het scherm van 31-07 stonden "Sef Voss" en "Sef Chen"
	 * onder elkaar in een squad van drie. In een bark, in een order en in het
	 * casualty-bericht heet zo iemand alleen bij zijn voornaam, en dan is één op de
	 * drie soldaten niet uit een ander te houden. Dat is geen cosmetisch defect: de
	 * hele pijler leunt erop dat je je mensen herkent.
	 *
	 * Het is bovendien geen zeldzaam ongeluk maar het verjaardagsprobleem. Met 16
	 * voornamen in de pool is de kans op minstens één botsing al 17 % bij drie
	 * mensen en 65 % bij acht. Wie dit met een grotere pool "oplost", stelt hem
	 * alleen uit.
	 *
	 * DE REGEL: binnen één roster is elke VOORNAAM uniek zolang de pool dat toelaat,
	 * en is de VOLLEDIGE naam altijd uniek. Raakt de voornamenpool op (meer soldaten
	 * dan namen), dan mag een voornaam terugkomen — maar dan nooit met dezelfde
	 * achternaam, en de aanroeper krijgt het te horen via OutFirstNamesExhausted.
	 * Onbekende data degradeert luid, nooit stil (14.3.5).
	 *
	 * Deterministisch: dezelfde FirstSeed en dezelfde pools geven dezelfde ploeg,
	 * want dat is wat save-fixtures en soak-tests reproduceerbaar houdt.
	 */
	ECLIPSE_API TArray<FEclipseSoldierRecord> GenerateRoster(
		FName OriginId, const FEclipseNameGenerationParams& Params, int32 Count,
		int32 FirstSeed = 1, bool* OutFirstNamesExhausted = nullptr);

	/** De voornaam uit een volledige naam ("Sef Voss" -> "Sef"). Eén plek, zodat de test en de generator niet uiteen kunnen lopen. */
	ECLIPSE_API FString FirstNameOf(const FString& FullName);

	/**
	 * Hoeveel namen in deze lijst met minstens één andere BOTSEN op de voornaam.
	 *
	 * Bestaat als losse functie omdat de falsificatie hem nodig heeft: een test die
	 * "alle namen zijn uniek" zelf uitrekent met andere code dan de generator,
	 * bewijst niets over de generator. En 0 is hier pas een uitspraak als dezelfde
	 * functie op een geprepareerde botsing wél een getal > 0 geeft — die
	 * controleproef staat in EclipseRosterTests.
	 */
	ECLIPSE_API int32 CountFirstNameCollisions(const TArray<FEclipseSoldierRecord>& Roster);

	/**
	 * The death-resolution policy (SPEC-P1-07, extended by SPEC-P2-01):
	 *  - mission failed  -> every downed soldier is dead (nobody carried them out)
	 *  - mission won     -> downed soldiers come home Wounded (out WoundedDaysOut)
	 *  - stabilized      -> Wounded even on failure (the GDD 4.2.5 save: a
	 *    stabilize inside the window converts permadeath into an injury)
	 * The stabilized branch needs WoundedDaysOut > 0 to express a wound; without
	 * roster tuning the conservative all-dead reading stands (GDD 14.3.5).
	 */
	ECLIPSE_API TArray<FEclipseResolvedCasualty> ResolveCasualties(
		const TMap<FGuid, FName>& DownedSoldiers,
		const FEclipseCampaignState& State,
		bool bMissionSuccess,
		int32 WoundedDaysOut,
		const TSet<FGuid>& StabilizedSoldiers = TSet<FGuid>());

	/** Availability read (SPEC-P1-08 squad picking): wounded soldiers return when the clock passes their recovery day. */
	ECLIPSE_API bool IsSoldierAvailableOnDay(const FEclipseSoldierRecord& Soldier, int32 Day);
}
