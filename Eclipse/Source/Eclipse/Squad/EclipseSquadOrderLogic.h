#pragma once

#include "CoreMinimal.h"
#include "Squad/EclipseSquadTypes.h"

/**
 * Order decisions as a pure table (GDD 14.3.2). Controllers gather the world
 * facts (path exists? target visible/valid? soldier standing?); this decides
 * accept-or-refuse-with-reason. The scenario suite (GDD 14.4 squad bar) tests
 * this table exhaustively headless — the "zero silent failures" contract lives
 * here, not in tick timing.
 *
 * Also home to the order round-trip meter: the answer bar of GDD 8.4 / the R3
 * gauntlet is a property of the order path, so it is measured and judged here
 * rather than in whatever UI happens to display it.
 */
namespace EclipseSquadOrderLogic
{
	struct FEclipseOrderWorldFacts
	{
		bool bSoldierConscious = true;
		bool bHasPathToTarget = true;
		bool bTargetValid = true;
		bool bTargetVisible = true;

		// --- SPEC-P2-02 Stage B: de feiten die de vijf nieuwe verbs nodig hebben.
		// Verzameld door de controller (navmesh, traces, perceptie), nooit hier
		// berekend — dit blijft een tabel die BESLIST (14.3.2).

		/** Suppress: heeft deze soldaat zicht op het aangewezen gebied? */
		bool bHasLineToArea = true;

		/** Breach: ligt er een AEclipseBreachPoint binnen BreachPointRangeCm van de plek? */
		bool bHasBreachPointInRange = true;

		/**
		 * SyncStrike: hoeveel markeringen staan er nog. Standaard NUL, en dat is
		 * met opzet het enige feit dat streng begint: een niet-ingevuld feitenblok
		 * hoort een sync strike te WEIGEREN, niet toe te staan. Fail-closed op het
		 * verb dat vier vijanden in één klap uitschakelt.
		 */
		int32 MarkedTargetCount = 0;

		/** SyncStrike: is deze soldaat (nog) ongezien? Gezien worden = stil toeslaan kan niet meer. */
		bool bAllAssignedConcealed = true;

		/** UseAbility: heeft het signatuurverb van deze soldaat hier iets om te doen? */
		bool bAbilityContextValid = true;
	};

	struct FEclipseOrderDecision
	{
		bool bAccepted = false;
		EEclipseOrderRefusalReason Reason = EEclipseOrderRefusalReason::None;
	};

	/** Every order gets exactly one answer; no input combination maps to silence. */
	ECLIPSE_API FEclipseOrderDecision DecideOrder(EEclipseSquadOrder Order, const FEclipseOrderWorldFacts& Facts);

	/**
	 * Welke rij in DT_SquadOrderDefs de weigerzinnen van deze REDEN draagt, of
	 * NAME_None als de reden de zinnen van het ordertype deelt.
	 *
	 * Waarom niet gewoon altijd de orderrij: dat leverde de bug van 26-07 op — een
	 * neergeschoten soldaat weigerde een MoveTo met "No route, boss." en wees de
	 * speler op een routeprobleem dat er niet was. Dezelfde val ligt klaar voor de
	 * drie nieuwe redenen: "geen breekpunt" is geen "kan er niet komen", en "ze
	 * zien me" is geen "er is niets gemarkeerd". Redenen die iets ANDERS betekenen
	 * dan hun order, krijgen hun eigen pool.
	 */
	ECLIPSE_API FName RefusalPoolRowName(EEclipseOrderRefusalReason Reason);

	/**
	 * The clock the round-trip meter runs on: FPlatformTime, i.e. the wall clock,
	 * NEVER the world clock. Command Mode dilates the world to 0.30 (SPEC-P2-02
	 * locked decision 2), so a game-time stamp would report a round trip 3.3x
	 * shorter than the tester actually waited — the one measurement error that
	 * could fabricate an R3 "true". One named seam so the test can assert the
	 * clock the subsystem really samples, with the world dilated.
	 */
	ECLIPSE_API double NowWallSeconds();

	/**
	 * Order round-trip tally (R3 criterion 1): per order the wall-clock seconds
	 * from issue to its ack/refusal, judged against the 1 s answer bar. Plain
	 * data driven by explicit stamps so the headless tier can sweep it — the
	 * subsystem feeds it, nobody else owns these numbers.
	 */
	struct ECLIPSE_API FEclipseOrderRoundTripStats
	{
		/** The R3 bar: every order answered within one second of real time. */
		static constexpr double BarSeconds = 1.0;

		int32 SampleCount = 0;

		/** Samples at or under BarSeconds; SampleCount - WithinBarCount = late answers. */
		int32 WithinBarCount = 0;

		double WorstSeconds = 0.0;
		double TotalSeconds = 0.0;

		/** Record one issue->answer round trip. Negative input is clamped to 0: a backwards clock is not a 0.0 s success story. */
		void NoteRoundTrip(double Seconds);

		void Reset();

		double GetAverageSeconds() const;
	};

	/** Deterministic bark pick (soldier id + order salt) so a given soldier has a stable voice in tests and replays. */
	ECLIPSE_API FString PickBarkLine(const TArray<FString>& Pool, const FGuid& SoldierId, uint32 Salt);

	/**
	 * Eén squadregel voor het scherm: DE SOLDAAT, niet zijn id.
	 *
	 * Hier stond `SoldierId.ToString().Left(8)`, en dat toonde voor élke soldaat
	 * hetzelfde: de eerste 32 bits van de GUID zijn per constructie de vaste kop
	 * `0x45434C53` ("ECLS", zie EclipseRosterLogic.cpp regel 15). Drie soldaten, drie
	 * keer `45434C53`. De id was niet stuk — er werd een stuk van getoond dat geen
	 * informatie draagt.
	 *
	 * De hele GUID tonen lost dat technisch op en inhoudelijk niet:
	 * `45434C53-50454F50-00000001-0000ABCD` is voor een mens even leeg. Wat daar
	 * hoort te staan is de NAAM — pijler 3 van dit spel is "People, Not Units", en
	 * het commentaar bij FEclipseSoldierRecord schrijft het zelf voor: "Names are
	 * data, never hardcoded."
	 *
	 * Puur en apart, zodat de falsificatie headless kan draaien: drie soldaten met
	 * verschillende namen horen drie regels met díe namen op te leveren, en de test
	 * hoort ROOD te worden zodra iemand teruggaat naar een id.
	 *
	 * Lege naam = de soldaat staat niet (meer) in de roster. Dat is een echt gat en
	 * krijgt daarom een regel die zich als gat aanmeldt in plaats van een getal dat
	 * op een naam lijkt (14.3.5).
	 */
	ECLIPSE_API FString ComposeOrderStateLine(const FString& SoldierName, const FGuid& SoldierId, const FString& OrderLabel);

	/**
	 * Per-class push modulation (SPEC-P2-01, GDD 9.5: "aggressive soldiers push
	 * further"): extends the ordered point PushDistanceCm past itself along the
	 * soldier->order direction. Zero push or a degenerate direction returns the
	 * ordered point unchanged — obeying the letter of the order stays default.
	 */
	ECLIPSE_API FVector ComputePushedOrderPoint(const FVector& SoldierLocation, const FVector& OrderedLocation, float PushDistanceCm);

	/**
	 * One ring sample's cover score (the SPEC-P1-06 scorer, factored pure so the
	 * class bias is testable headless). Blocked line-of-fire dominates; ties go
	 * to the sample nearest the order; LaneBias > 0 (Sniper) prefers covered
	 * samples with a longer clear lane to the threat.
	 */
	ECLIPSE_API float ScoreCoverSample(bool bBlocksThreatLine, float DistanceToOrderCm, float DistanceToThreatCm, float LaneBias,
		float CoverBlockBonus = 10.0f, float DistanceWeightPerCm = 0.001f);

	// ======================================================================
	// SPEC-P2-02 Stage B — de pure stukken onder de vijf nieuwe verbs.
	// Geen engine-actoren, geen wereld, geen tijdvertraging: plain functies over
	// plain structs, zodat de headless laag ze uitputtend kan afgaan (14.3.2).
	// ======================================================================

	/**
	 * De markeringenset van de sync strike (8.4: "up to 4 marked").
	 *
	 * Markeringen zijn ID's en geen actoren, precies zodat deze laag niets van de
	 * wereld hoeft te weten: de subsystem koppelt een id aan een lichaam en snoeit
	 * de set als dat lichaam neergaat of ons opmerkt.
	 */
	struct ECLIPSE_API FEclipseSyncStrikeMarkSet
	{
		TArray<FGuid> Marks;

		/**
		 * Markeer erbij. False = niets veranderd: ongeldige id, al gemarkeerd, of
		 * de cap uit data bereikt. MaxMarks < 1 wordt als 1 gelezen — een cap van
		 * nul zou het verb onbruikbaar maken zonder dat iemand dat bedoelde (14.3.5).
		 */
		bool AddMark(const FGuid& TargetId, int32 MaxMarks);

		/** Markering weg. False = die stond er niet. */
		bool RemoveMark(const FGuid& TargetId);

		/**
		 * Markeren is een SCHAKELAAR op het doel: nog eens aanwijzen haalt hem weg.
		 * bOutMarked zegt wat de nieuwe stand is; false teruggeven betekent "je
		 * wilde markeren maar de cap zat vol" — het enige geval waarin een druk op
		 * de knop niets doet, en dus het enige dat een zin verdient.
		 */
		bool ToggleMark(const FGuid& TargetId, int32 MaxMarks, bool& bOutMarked);

		/**
		 * Snoei alles wat niet meer in StillValidTargets staat (dood, verdwenen, of
		 * ons inmiddels dóór). Geeft terug hoeveel er wegvielen — de subsystem
		 * spreekt daar een regel bij, want een pip die uit zichzelf verdwijnt is
		 * precies de stille verandering die 9.5 verbiedt.
		 */
		int32 PruneMarks(const TArray<FGuid>& StillValidTargets);

		bool IsMarked(const FGuid& TargetId) const { return Marks.Contains(TargetId); }
		int32 Num() const { return Marks.Num(); }
		void Reset() { Marks.Reset(); }
	};

	/** Waar een flank-voorstel in zijn leven staat (SPEC-P2-02: Proposed → Approved / Expired / Cancelled). */
	enum class EEclipseFlankState : uint8
	{
		Idle,
		Proposed,
		Approved,
		Expired,
		Cancelled
	};

	/** Wat er met een flank-voorstel gebeurt. Tick = "de klok liep door", niet een speleractie. */
	enum class EEclipseFlankSignal : uint8
	{
		Propose,
		Approve,
		Cancel,
		Tick
	};

	struct ECLIPSE_API FEclipseFlankApproval
	{
		EEclipseFlankState State = EEclipseFlankState::Idle;

		/** Wandklok-stempel van het voorstel (locked decision 2: het venster loopt op echte seconden). */
		double ProposedWallSeconds = 0.0;
	};

	/**
	 * Eén stap van de goedkeuringsautomaat.
	 *
	 * De dragende regel staat in het midden: GOEDKEUREN NA DE TIJD KEURT NIET GOED.
	 * Een druk die één tel te laat komt hoort te eindigen op Expired en niet op
	 * Approved — anders is het venster een decoratie en vertrekt de soldaat alsnog
	 * naar een route die de speler allang losgelaten had.
	 *
	 * TimeoutSeconds <= 0 = geen verval. Dat is de degradatie voor ontbrekende
	 * data: een venster van nul zou elk voorstel dood laten geboren worden, en dan
	 * bestaat het verb niet meer terwijl de knop wel reageert (14.3.5).
	 */
	ECLIPSE_API FEclipseFlankApproval ApplyFlankSignal(const FEclipseFlankApproval& State, EEclipseFlankSignal Signal,
		double NowWallSeconds, double TimeoutSeconds);

	/** Staat er nu een voorstel dat nog goedgekeurd KAN worden? */
	ECLIPSE_API bool IsFlankWindowOpen(const FEclipseFlankApproval& State, double NowWallSeconds, double TimeoutSeconds);

	/** Wat een doctrine over ZELF vuren te zeggen heeft (de twee poorten van Stealth, de ene van Recon). */
	struct FEclipseFireDisciplineFacts
	{
		/** Er staat een order die vuren VRAAGT (FocusTarget, Suppress). */
		bool bOrderedToFire = false;

		/** Er is op deze soldaat geschoten. */
		bool bTakenFire = false;

		/** De vijand heeft onze kant opgemerkt (alarm / eerste contact). */
		bool bEnemyAware = false;
	};

	/**
	 * Mag deze soldaat op eigen initiatief het vuur openen?
	 *
	 * Dit is de hele stealth-stance, en met opzet als één functie: de doctrine is
	 * een KADER dat de basis inperkt (owner 26-07), dus er hoort precies één plek
	 * te zijn waar "de basis is autonoom vuren" wordt afgeknepen. Stond dit als
	 * drie ifs in UpdateEngagement, dan is de vierde doctrine die niemand toevoegt
	 * er een die stilletjes de basis terugkrijgt.
	 */
	ECLIPSE_API bool StanceAllowsAutonomousFire(EEclipseSquadStance Stance, const FEclipseFireDisciplineFacts& Facts);

	/**
	 * Wie slaat welke gemarkeerde doelen? Ronde verdeling, deterministisch, puur.
	 *
	 * De eigenschap die ertoe doet is een DEKKING: tel de uitkomsten over alle
	 * soldaten op en je hebt elke markering precies één keer. Meer soldaten dan
	 * markeringen betekent dus dat de laatsten niets krijgen (in plaats van dat er
	 * twee man op hetzelfde keeltje duiken); meer markeringen dan soldaten
	 * betekent dat de eersten er meer dan één pakken (in plaats van dat er stil
	 * eentje blijft staan — dat is precies het soort halve sync strike dat een
	 * stealth-run verpest zonder uit te leggen waarom).
	 */
	ECLIPSE_API TArray<int32> AssignSyncStrikeMarkIndices(int32 SoldierIndex, int32 SoldierCount, int32 MarkCount);
}
