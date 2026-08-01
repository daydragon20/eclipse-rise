#pragma once

#include "CoreMinimal.h"
#include "Core/EclipseEventPayloads.h"

/**
 * De datalaag ONDER de schermlaag (bouwvolgorde GDD 14.5: schema → pure logica +
 * tests → bedrading + events → pas daarna iets zichtbaars).
 *
 * Deze laag beantwoordt precies één vraag: *gegeven de vorige en de huidige
 * toestand van een lichaam, moet er iets over de bus, en met welke velden?* Geen
 * actor, geen wereld, geen subsysteem, geen tick — daarom is hij hoofdeloos te
 * testen en daarom kan de test "60 tikken zonder verandering = 0 uitzendingen"
 * bestaan zonder engine op te starten.
 *
 * Waarom deze laag er überhaupt is, en de bedrading niet gewoon zelf broadcast:
 * een gezondheidsattribuut kan per frame binnenkomen (GAS-delegates, herstel,
 * afronding) en een houding kan tijdens één sprong drie keer heen en weer
 * schakelen. Wie dat rechtstreeks doorgeeft, heeft pollen vervangen door spammen
 * en de bus is er slechter aan toe dan daarvoor. De vergelijking hoort dus op één
 * plek te staan, en op een plek waar hij te falsificeren is.
 */
namespace EclipseVitalsFeed
{
	/**
	 * Onder dit verschil is gezondheid geen NIEUWS voor het scherm.
	 *
	 * Nodig omdat gezondheid een float is die door GAS heen gaat: een herstel-tick
	 * of een mitigatiefactor levert zo 99.999998 op waar 100 stond, en zonder
	 * drempel is dat een uitzending per frame over een verschil dat niemand kan
	 * zien. 0,01 hp is ruim onder de kleinste eenheid die een balk of een getal
	 * ooit toont, en ruim boven de ruis van een float-vermenigvuldiging.
	 */
	constexpr float HealthEpsilon = 0.01f;

	/**
	 * Eén moment van één lichaam, zoals de bedrading het uit de pawn schept.
	 *
	 * Losse bools en niet meteen een EEclipseStance, omdat DIT de rauwe waarneming
	 * is: `bIsCrouched` van de capsule, de sprint-drive van de controller. De
	 * voorrang tussen die waarnemingen is een BESLISSING, en die hoort in
	 * ResolveStance te staan waar hij te lezen en te testen is.
	 */
	struct FEclipseVitalsSnapshot
	{
		float Health = 0.0f;
		float MaxHealth = 0.0f;
		bool bCrouched = false;
		bool bSprinting = false;

		/**
		 * Vandaag zet niets dit aan; zie de opmerking bij EEclipseStance. Het veld
		 * staat er zodat een echte dekkingstoestand later alleen dit monster hoeft
		 * te vullen — geen schema-, catalogus- of HUD-wijziging.
		 */
		bool bInCover = false;
		bool bDowned = false;
	};

	/**
	 * De voorrangsregel, op één plek.
	 *
	 * Dekking wint van hurken: wie gehurkt achter een muur zit, hoort "DEKKING" te
	 * lezen — dat is het tactische feit, de knieën zijn het gevolg. Hurken wint van
	 * sprint, en dat is geen smaak maar de waarheid van het bewegingscomponent:
	 * gehurkt geldt MaxWalkSpeedCrouched, dus een vastgezette sprint-latch die
	 * tijdens het hurken aan blijft staan is geen sprint — je kruipt. Dat als
	 * "SPRINT" tonen zou het scherm laten liegen over hoe snel je bent.
	 */
	ECLIPSE_API EEclipseStance ResolveStance(const FEclipseVitalsSnapshot& Snapshot);

	/** Wat de bedrading met een monster moet doen. */
	struct FEclipseVitalsDecision
	{
		bool bShouldBroadcast = false;

		/** Alleen betekenisvol als bShouldBroadcast; kant-en-klaar om de bus op te gaan. */
		FEclipsePlayerVitalsPayload Payload;
	};

	/**
	 * De kern. Vergelijkt twee momenten en levert het feit dat ertussen zit.
	 *
	 * @param bHasPrevious  Is Previous een echt eerder uitgezonden moment, of is dit
	 *   het eerste monster van dit lichaam? Die twee moeten uit elkaar te houden
	 *   zijn: bij het eerste monster is er niets veranderd (er was alleen nog niets
	 *   bekend) en toch moet er iets naar het scherm, anders staat de HUD bij spawn
	 *   op nul. Het uitgaande feit draagt dan bInitial en géén verander-vlaggen.
	 */
	ECLIPSE_API FEclipseVitalsDecision Decide(const FEclipseVitalsSnapshot& Previous, const FEclipseVitalsSnapshot& Current, bool bHasPrevious);

	/**
	 * Decide plus het geheugen dat erbij hoort: onthoudt het LAATST UITGEZONDEN
	 * moment, niet het laatst waargenomen moment.
	 *
	 * Dat onderscheid is de hele rem. Wie het laatst geziene monster onthoudt,
	 * vergelijkt tegen een waarde die hij nooit verstuurd heeft; een verandering
	 * van 0,004 hp per frame zou dan honderd frames lang onder de drempel blijven
	 * terwijl de speler ondertussen 0,4 hp kwijt is — en het scherm zou het nooit
	 * horen. Nu stapelt zo'n drup op tot hij de drempel haalt en dan één keer gaat.
	 */
	struct ECLIPSE_API FEclipseVitalsTracker
	{
		FEclipseVitalsDecision Submit(const FEclipseVitalsSnapshot& Current);

		/** Terug naar "nog nooit iets verstuurd"; het volgende monster is weer bInitial. */
		void Reset();

		/**
		 * Vergeet WAT er het laatst uit ging, maar niet DAT er iets uit ging.
		 *
		 * Bestaat voor de ene situatie die een bus zonder geheugen structureel niet
		 * dekt: een consument die LATER aankomt dan de producent. Het lichaam zendt
		 * zijn eerste vitals uit bij de bezetting (NotifyControllerChanged) en bij
		 * InitializeHealth; de missie-HUD wordt gemonteerd op Event.Mission.Started,
		 * ruim daarna. Zonder een manier om het opnieuw te vragen zou de hoek
		 * linksonder leeg blijven tot de speler zijn eerste klap oploopt — en een
		 * gezondheidsbalk die pas verschijnt als je al geraakt bent, is precies het
		 * gat waar deze laag voor bestaat.
		 *
		 * NIET Reset(), en dat verschil is het hele bestaansrecht van deze functie:
		 * Reset zet ook BroadcastCount op nul, en dat is een MEETINSTRUMENT (het
		 * scheidt "de poort wees het lichaam af" van "de feed vond geen verandering"
		 * van "de bus leverde niet af" — zie de vier-ketenmeting in
		 * EclipseVitalsFeedTests). Een HUD die bij zijn montage het bewijsmateriaal
		 * uitgumt, breekt precies het onderzoek dat je daarna zou willen doen.
		 *
		 * Identiek van vorm aan EclipseWeaponStatusFeed::ForgetLastBroadcast, en dat
		 * is geen toeval: het is dezelfde montagevolgorde en dus hetzelfde probleem.
		 */
		void ForgetLastBroadcast();

		bool HasBroadcast() const { return bHasBroadcast; }
		const FEclipseVitalsSnapshot& GetLastBroadcast() const { return LastBroadcast; }

		/** Hoeveel feiten dit lichaam werkelijk de bus op stuurde (meetpunt voor de test/telemetrie). */
		int32 GetBroadcastCount() const { return BroadcastCount; }

	private:
		FEclipseVitalsSnapshot LastBroadcast;
		bool bHasBroadcast = false;
		int32 BroadcastCount = 0;
	};
}
