// Feel-harnas — de twee lagen waar de owner om vroeg (nachtopdracht 2026-07-25).
//
// LAAG 1  leest na het spawnen de DAADWERKELIJK toegepaste waarden van het
//         movement component en de camera en legt ze naast de tuningbron.
// LAAG 2  injecteert input via Enhanced Input — geen mens, geen hardware — en
//         MEET over tijd wat daaruit volgt.
//
// Waarom een echte wereld en niet pure logica: de vraag die dit harnas moet
// beantwoorden is nooit "klopt de formule" maar "komt het getal aan". De vorige
// ronde ging vier keer verloren aan plausibele verklaringen; een asset dat een
// C++-default overschrijft, een vlag die een handler stil maakt of een
// camera-term die met de snelheid meebeweegt zijn alleen zichtbaar op het
// draaiende ding. Dus: echte UWorld, echte AEclipseCharacter, echte
// AEclipsePlayerController, echte UInputAction-objecten.
//
// De injectie loopt via UEnhancedInputLocalPlayerSubsystem::InjectInputForAction
// en dus op DEZELFDE acties die de hardware van de speler aanstuurt. Een harnas
// dat zijn eigen acties zou bouwen bewijst niets over de verscheepte bindings.
// Enhanced Input verwerkt injecties los van de mapping contexts en "laat de toets
// los" zodra er een tick niet opnieuw geïnjecteerd wordt — dat is precies het
// hold/release-gedrag dat we nodig hebben.

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"

class AEclipseCharacter;
class AEclipsePlayerController;
class FAutomationTestBase;
class UEclipseCharacterTuningAsset;
class UEnhancedInputLocalPlayerSubsystem;
class UGameInstance;
class ULocalPlayer;
class UWorld;

namespace EclipseFeelHarness
{
	/** Vaste stap. 1/120 s: fijn genoeg om 100 ms-verschillen te scheiden, grof
	 *  genoeg om een meting van enkele seconden in tienden van seconden te doen. */
	constexpr float FixedStepSeconds = 1.0f / 120.0f;

	/** Bovengrens per meting, zodat een vastloper een gefaalde test wordt en geen
	 *  hangende suite (GDD 14.4: een test die niet eindigt, meet niets). */
	constexpr float DefaultTimeoutSeconds = 12.0f;

	/**
	 * Een draaiende wereld met vloer, speler en bediening. Bewust géén AGameMode:
	 * die bouwt bij InitGame het hele graybox-district, en een locomotie-meting
	 * wil een vlakke, bekende vloer zonder verkeer. De missie-speelronde
	 * (EclipseMissionPlaythroughTests) gebruikt juist wél het echte laadpad.
	 */
	struct FHarness
	{
		UGameInstance* GameInstance = nullptr;
		UWorld* World = nullptr;
		ULocalPlayer* LocalPlayer = nullptr;
		AEclipsePlayerController* Controller = nullptr;
		AEclipseCharacter* Body = nullptr;
		UEnhancedInputLocalPlayerSubsystem* Input = nullptr;
		/** De tuningbron waar laag 1 tegen vergelijkt. */
		const UEclipseCharacterTuningAsset* Tuning = nullptr;

		/** Opgetelde wereldtijd sinds Start(), in seconden. */
		double ElapsedSeconds = 0.0;

		bool IsValid() const { return World != nullptr && Controller != nullptr && Body != nullptr && Input != nullptr; }

		/**
		 * Wereld op, vloer erin, speler erin, tuning erop. Faalt luid via Test en
		 * geeft false terug; de aanroeper hoort dan af te breken.
		 */
		bool Start(FAutomationTestBase& Test);
		void Shutdown();

		/** Eén vaste stap wereldtijd. Injecties gelden per tick, dus wie een toets
		 *  vast wil houden, injecteert vóór elke stap opnieuw (zie HoldFor). */
		void Step(float DeltaSeconds = FixedStepSeconds);

		/** N stappen zonder injectie — "niets aanraken". */
		void Idle(float Seconds);

		// ---- injectie -------------------------------------------------------
		/** Injecteer één tick lang een waarde op een actie (naam per FindInputAction). */
		void Inject(FName ActionName, const FVector2D& Value);
		void Inject(FName ActionName, bool bPressed);

		/**
		 * Houd een actie Seconds lang vast, en tick mee. Predicate mag de lus
		 * vroegtijdig stoppen (bijv. "tot topsnelheid"); geeft de verstreken tijd.
		 */
		double HoldFor(FName ActionName, const FVector2D& Value, double Seconds,
			TFunctionRef<bool()> StopWhen);
		double HoldFor(FName ActionName, const FVector2D& Value, double Seconds);

		/** Eén klik: één tick ingedrukt, één tick los (dat laatste is wat Enhanced
		 *  Input een Completed laat vuren). */
		void Press(FName ActionName);

		/**
		 * Klikken TERWIJL de bewegingsstick vooruit blijft staan. Nodig omdat een
		 * losse Press twee ticks lang niets op Move injecteert, en Enhanced Input
		 * dat correct als "stick losgelaten" leest — waarmee de test een sprint zou
		 * meten die hij zelf net beëindigd had. Een speler die L3 drukt terwijl hij
		 * rent, laat de stick niet los; dit is die speler.
		 */
		void PressWhileMovingForward(FName ActionName);

		// ---- metingen -------------------------------------------------------
		float SpeedCm() const;
		FVector Location() const;

		/** Loop vooruit tot de snelheid niet meer noemenswaardig stijgt, zodat een
		 *  meting op "volle snelheid" ook echt daar begint. */
		bool RunUpToTopSpeed(FAutomationTestBase& Test, double MaxSeconds = 4.0);
	};

	/** Eén regel met naam, gemeten waarde en eenheid — het log dat de owner leest.
	 *  De opdracht was expliciet: log de GEMETEN waarden, niet alleen pass/fail. */
	void Report(FAutomationTestBase& Test, const TCHAR* Label, double Value, const TCHAR* Unit, const TCHAR* Expectation = nullptr);
}

#endif // WITH_DEV_AUTOMATION_TESTS
