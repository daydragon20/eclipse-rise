#pragma once

#include "CoreMinimal.h"
#include "Core/EclipseEventPayloads.h"

/**
 * De datalaag ONDER de schermlaag voor het WAPEN — de tweede helft van wat
 * `Characters/EclipseVitalsFeed` voor het lichaam doet, met opzet in dezelfde vorm
 * (bouwvolgorde GDD 14.5: schema → pure logica + tests → bedrading + events → pas
 * daarna iets zichtbaars).
 *
 * Eén vraag, net als daar: *gegeven de vorige en de huidige stand van een wapen,
 * moet er iets over de bus, en met welke velden?* Geen actor, geen wereld, geen
 * component, geen tick — daarom is de test "een frame zonder verandering = niets"
 * hier te draaien zonder engine, en daarom is hij te vertrouwen.
 *
 * WAAROM DIT NIET GEWOON IN HET WAPENCOMPONENT KON. Munitie verandert het vaakst
 * van alles op het scherm: bij aanhoudend vuur zes tot zeven keer per seconde, en
 * de herlaadvoortgang is een KLOK die per frame een ander getal heeft. Wie dat
 * rechtstreeks doorgeeft, heeft het pollen van `UI/EclipseMissionHudWidget.cpp`
 * vervangen door spammen en de bus is er slechter aan toe dan met de poll. De
 * vergelijking hoort dus op één plek te staan, en op een plek waar hij te
 * falsificeren is.
 */
namespace EclipseWeaponStatusFeed
{
	/**
	 * De stap waarin herlaadvoortgang NIEUWS is: 5 % van de beurt.
	 *
	 * Dit is de enige drempel in dit bestand die niet over ruis gaat maar over
	 * RESOLUTIE, en het getal is afgeleid, niet gekozen. Een herlaadbeurt duurt in
	 * DT_Weapons 1,4 tot 2,8 s; op 60 fps is dat 84 tot 168 frames. Op 5 % komen er
	 * hooguit 20 feiten uit één beurt — grofweg negen per seconde — in plaats van
	 * honderdvijftig. Dat is de orde van grootte van het overige busverkeer en niet
	 * die van een tick.
	 *
	 * En 20 stappen is genoeg voor de balk waar dit voor bestaat: op een balk van
	 * 200 px is 5 % tien pixels, en tussen twee feiten door kan de schermlaag
	 * vloeiend interpoleren met `ReloadSecondsRemaining` uit hetzelfde feit. De
	 * afweging is dus niet "vloeiend of zuinig" — het feit draagt beide.
	 *
	 * De laatste stap (100 %) hoeft deze drempel niet te halen: het EINDE van de
	 * beurt is een toestandsomslag en gaat langs bReloadStateChanged de deur uit.
	 * Zonder die uitzondering zou een beurt op 96 % kunnen blijven hangen.
	 */
	constexpr float ReloadProgressEpsilon = 0.05f;

	/**
	 * Eén moment van één wapen, zoals de bedrading het uit het component schept.
	 *
	 * Rauwe waarneming, geen conclusies: `bEmpty` staat hier NIET, want of een leeg
	 * magazijn "leeg" heet is een BESLISSING (een wapen met MagazineSize 0 telt
	 * helemaal niet mee en is dus nooit leeg). Die beslissing staat in IsEmpty(),
	 * waar hij te lezen en te testen is — zelfde reden als ResolveStance bij de
	 * vitals.
	 */
	struct FEclipseWeaponSnapshot
	{
		int32 AmmoInMagazine = 0;
		int32 MagazineSize = 0;

		/** -1 = onbeperkt; zie FEclipseWeaponStatusPayload::SpareMagazines. */
		int32 SpareMagazines = -1;

		bool bReloading = false;
		float ReloadProgress = 0.0f;
		float ReloadSecondsRemaining = 0.0f;
		float ReloadSecondsTotal = 0.0f;

		/** De SLEUTEL van het wapen; hierop wordt "is het een ander wapen" beslist. */
		FName WeaponRowName;

		/** De leestekst. Bewust GEEN beslisveld — zie de toelichting bij Decide. */
		FText WeaponDisplayName;

		int32 ActiveSlot = 0;
		int32 SlotCount = 1;

		EEclipseWeaponFireMode FireMode = EEclipseWeaponFireMode::Unspecified;
	};

	/**
	 * Is dit wapen leeg? Op één plek, want elke widget die het zelf uitrekent,
	 * rekent het net iets anders uit.
	 *
	 * MagazineSize <= 0 betekent "dit wapen telt geen kogels" (de oneindige
	 * varianten, en de -1 die `ApplyWeaponRow` kort draagt voordat er een rij is).
	 * Dat is geen leeg magazijn maar de afwezigheid van een magazijn, en die twee
	 * als hetzelfde tonen zou de teller permanent op de scherpste stand zetten.
	 */
	ECLIPSE_API bool IsEmpty(const FEclipseWeaponSnapshot& Snapshot);

	/** Wat de bedrading met een monster moet doen. */
	struct FEclipseWeaponStatusDecision
	{
		bool bShouldBroadcast = false;

		/** Alleen betekenisvol als bShouldBroadcast; kant-en-klaar om de bus op te gaan. */
		FEclipseWeaponStatusPayload Payload;
	};

	/**
	 * De kern. Vergelijkt twee momenten en levert het feit dat ertussen zit.
	 *
	 * DE LEESTEKST BESLIST NIET MEE, en dat is bewust. `WeaponDisplayName` reist wél
	 * in elk feit mee (de schermlaag heeft hem nodig), maar een verschil erin zet
	 * geen uitzending in gang. Twee redenen: FText-gelijkheid is
	 * lokalisatie-afhankelijk — dezelfde rij kan bij een taalwissel een andere tekst
	 * opleveren zonder dat er aan het wapen iets veranderde — en de rijnaam is de
	 * SLEUTEL. Wie op de leestekst zou vergelijken, zou bovendien twee rijen met per
	 * ongeluk dezelfde DisplayName als hetzelfde wapen zien.
	 *
	 * @param bHasPrevious  Is Previous een echt eerder uitgezonden moment, of is dit
	 *   het eerste monster van dit wapen? Bij het eerste monster is er niets
	 *   veranderd (er was alleen nog niets bekend) en toch moet er iets naar het
	 *   scherm, anders staat de teller tot het eerste schot op nul. Het uitgaande
	 *   feit draagt dan bInitial en géén verander-vlaggen.
	 */
	ECLIPSE_API FEclipseWeaponStatusDecision Decide(const FEclipseWeaponSnapshot& Previous, const FEclipseWeaponSnapshot& Current, bool bHasPrevious);

	/**
	 * Decide plus het geheugen dat erbij hoort: onthoudt het LAATST UITGEZONDEN
	 * moment, niet het laatst waargenomen moment.
	 *
	 * Dat onderscheid is de hele rem, en bij de herlaadvoortgang is het het verschil
	 * tussen een balk die loopt en een balk die stilstaat: wie tegen het laatst
	 * GEZIENE monster vergelijkt, ziet per frame een sprongetje van 0,8 % en zendt
	 * dus nooit iets uit terwijl de beurt gewoon doorloopt. Nu stapelen die
	 * sprongetjes op tot ze de drempel halen en gaat er precies dan één feit.
	 */
	struct ECLIPSE_API FEclipseWeaponStatusTracker
	{
		FEclipseWeaponStatusDecision Submit(const FEclipseWeaponSnapshot& Current);

		/** Terug naar "nog nooit iets verstuurd"; het volgende monster is weer bInitial. */
		void Reset();

		bool HasBroadcast() const { return bHasBroadcast; }
		const FEclipseWeaponSnapshot& GetLastBroadcast() const { return LastBroadcast; }

		/** Hoeveel feiten dit wapen werkelijk de bus op stuurde (meetpunt voor de test/telemetrie). */
		int32 GetBroadcastCount() const { return BroadcastCount; }

	private:
		FEclipseWeaponSnapshot LastBroadcast;
		bool bHasBroadcast = false;
		int32 BroadcastCount = 0;
	};
}
