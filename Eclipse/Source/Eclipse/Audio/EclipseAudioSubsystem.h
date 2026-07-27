#pragma once

#include "CoreMinimal.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EclipseAudioSubsystem.generated.h"

class USoundBase;

/**
 * Eén wapenfamilie of één oppervlak: de varianten die erbij horen.
 *
 * USTRUCT met UPROPERTY-arrays, en dat is geen ceremonie maar de reden dat dit
 * bestand op 26-07 om 21:01 crashte. Als gewone struct in een gewone TMap ziet de
 * garbage collector deze TObjectPtr-referenties NIET: hij ruimt de cues op terwijl
 * de maps er nog naar wijzen, en de eerste voetstap daarna speelt vrijgegeven
 * geheugen af (EXCEPTION_ACCESS_VIOLATION in PlayFootstep).
 *
 * De bug zat er vanaf het bouwen van de voetstappen; hij werd zichtbaar toen er
 * MINDER geluid geladen bleef (alleen de twee gebruikte wapenfamilies), want toen
 * viel de race de verkeerde kant op. Dat is precies waarom dit soort fout geen
 * enkele logica-test haalt: de code is correct, alleen het geheugen niet.
 */
USTRUCT()
struct FEclipseSoundVariantSet
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> Shots;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> Suppressed;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USoundBase>> Tails;

	/** Welke variant het laatst klonk — om hem niet meteen te herhalen. */
	int32 LastShotIndex = -1;
	int32 LastTailIndex = -1;
};

/** Eén fase van een herlaadbeurt: wanneer, en welk geluid. */
USTRUCT()
struct FEclipseFoleyStep
{
	GENERATED_BODY()

	float Fraction = 0.0f;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> Cue = nullptr;
};

/** De fasen van één familie; een eigen struct omdat een TMap geen TArray-waarde met UPROPERTY kan dragen. */
USTRUCT()
struct FEclipseFoleyChain
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<FEclipseFoleyStep> Steps;
};

/**
 * Runtime audio consumer (GDD 16.7/16.12): subscribes to gameplay facts on the
 * event bus and answers them with sound. Audio is a pure consumer by contract —
 * it emits no events and owns no gameplay state (EventCatalog: it appears only
 * in consumer columns). This subsystem is the seam the full 16.7 layer system
 * (bed/percussion/lead with ducking) grows behind; today it plays the
 * mission-complete resistance sting on Event.Mission.Completed.
 *
 * Placement note: the Layer-1 ambient bed is NOT here — it is district content
 * and spawns with the graybox (Core/EclipseGrayboxBuilder.cpp), the same way
 * lights and fog do. This subsystem covers world-agnostic event-driven audio.
 */
UCLASS()
class ECLIPSE_API UEclipseAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Bind/unbind the bus subscription. Split out of Initialize/Deinitialize so
	 * the headless unit layer can drive the subsystem against a bare bus without
	 * a full subsystem-collection init (same construction style as the bus
	 * tests). BindToBus is idempotent: re-binding never leaks a subscription.
	 */
	void BindToBus(UEclipseEventBusSubsystem& Bus);
	void UnbindFromBus();

	/** True while the Event.Mission.Completed subscription is live (test/diagnostic surface). */
	bool IsBoundToBus() const { return MissionCompletedHandle.IsValid(); }

	/**
	 * Diagnostic: facts consumed as sting requests. Counts even when playback
	 * degrades (missing asset, world-less context — GDD 14.3.5), which is what
	 * lets the unit layer prove delivery without an audio device.
	 */
	int32 GetStingRequestCount() const { return StingRequestCount; }

	/** Diagnostiek voor de testlaag: hoeveel schoten om geluid vroegen. */
	int32 GetShotSoundCount() const { return ShotSoundCount; }
	int32 GetHitSoundCount() const { return HitSoundCount; }

	/**
	 * Diagnostic: barks die daadwerkelijk een stem hebben aangevraagd, en barks
	 * die de rem tegenhield. Zelfde reden als StingRequestCount — zo kan de
	 * testlaag bewijzen dat de koppeling werkt zonder geluidskaart.
	 */
	int32 GetBarkRequestCount() const { return BarkRequestCount; }
	int32 GetBarkSuppressedCount() const { return BarkSuppressedCount; }
	int32 GetTailSoundCount() const { return TailSoundCount; }

	/**
	 * Speel de voetstap die bij dit oppervlak hoort. Het oppervlak komt van de
	 * beller (die traceert), want alleen hij weet waar de voeten staan.
	 */
	void PlayFootstep(const FVector& Location, uint8 SurfaceType, float Volume);
	int32 GetFootstepSoundCount() const { return FootstepSoundCount; }
	int32 GetFootstepVariantCount(uint8 SurfaceType) const;

	/**
	 * Hoeveel cues in de banken nog een GELDIG object zijn.
	 *
	 * Bestaat omdat een telling het niet bewijst: een TArray houdt zijn lengte ook
	 * als de garbage collector de objecten eronder heeft opgeruimd. Alleen dit
	 * getal zakt als de referenties niet beschermd zijn — en dat is precies wat de
	 * crash van 26-07 21:01 was.
	 */
	int32 CountLiveCues() const;

	/** Foley-stappen die echt gepland zijn — het bewijs dat de keten loopt. */
	int32 GetReloadFoleyCount() const { return ReloadFoleyCount; }
	int32 GetReloadFoleyStepCount(FName Family) const;
	int32 GetEquipSoundCount() const { return EquipSoundCount; }
	int32 GetWeaponSoundFamilyCount() const { return WeaponSoundSets.Num(); }
	int32 GetWeaponSoundVariantCount(FName Family) const;

	/**
	 * Kies een variant die niet dezelfde is als de vorige. Bij twee varianten is
	 * dat afwisselen, bij drie of meer een keuze uit de rest — precies wat je wilt
	 * horen: geen loop, maar ook geen twee identieke knallen achter elkaar.
	 *
	 * Publiek omdat het de enige regel is die je zonder geluidskaart kunt bewijzen:
	 * duizend trekkingen, nul herhalingen. Zelfde reden als de pure hitmarker-logica.
	 */
	static int32 PickVariantIndex(int32 Count, int32 LastIndex);

	/**
	 * De rem, in seconden per soldaat (owner-beslissing 26-07: 2 s). Vier orders
	 * achter elkaar mogen geen vier stemmen over elkaar heen worden; per SOLDAAT
	 * en niet globaal, want twee man die tegelijk antwoorden is juist het geluid
	 * van een squad die meedoet.
	 */
	static constexpr float BarkCooldownSeconds = 2.0f;

private:
	void OnMissionCompleted(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void OnOrderAnswered(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void OnShotFired(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void OnHitLanded(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void OnReloadStarted(FGameplayTag EventTag, const FInstancedStruct& Payload);
	void OnWeaponSwapped(FGameplayTag EventTag, const FInstancedStruct& Payload);

	/** Bus we subscribed on; weak so teardown order during shutdown cannot dangle. */
	TWeakObjectPtr<UEclipseEventBusSubsystem> BoundBus;
	FEclipseEventSubscriptionHandle MissionCompletedHandle;
	FEclipseEventSubscriptionHandle OrderAckHandle;
	FEclipseEventSubscriptionHandle OrderRefusedHandle;
	FEclipseEventSubscriptionHandle SelfActionHandle;
	FEclipseEventSubscriptionHandle ShotFiredHandle;
	FEclipseEventSubscriptionHandle HitLandedHandle;

	/** De MISSER, sinds 27-07 een eigen feit; deelt de handler met HitLanded. */
	FEclipseEventSubscriptionHandle WorldImpactHandle;
	FEclipseEventSubscriptionHandle ReloadStartedHandle;
	FEclipseEventSubscriptionHandle WeaponSwappedHandle;

	/** Laatste bark per soldaat (wereldseconden) — de rem van BarkCooldownSeconds. */
	TMap<FGuid, double> LastBarkSeconds;
	int32 BarkRequestCount = 0;
	int32 BarkSuppressedCount = 0;

	/** 14.3.5-missers houden zich tot een regel, net als de sting hierboven. */
	bool bWarnedMissingBarkLine = false;
	bool bWarnedMissingVoiceAsset = false;

	/** Lazily resolved sting; the tried-flag keeps a 14.3.5 miss to one log line. */
	UPROPERTY(Transient)
	TObjectPtr<USoundBase> MissionCompleteSting;
	bool bTriedLoadSting = false;

	int32 StingRequestCount = 0;

	/** Diagnostiek: schoten die om geluid vroegen (zelfde reden als StingRequestCount). */
	int32 ShotSoundCount = 0;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> WeaponShotCue;
	bool bTriedLoadWeaponShot = false;

	/**
	 * Eén wapenfamilie uit FreeWeaponSounds: drie schoten, drie gedempte, twee
	 * nagalmen. Als SET en niet als losse velden, want de keuze "welke variant nu"
	 * is precies wat een set moet kunnen en een veld niet.
	 */
	/** Familienaam (DT_Weapons.SoundFamily) → set. Gevuld bij Initialize. */
	UPROPERTY(Transient)
	TMap<FName, FEclipseSoundVariantSet> WeaponSoundSets;

	void LoadWeaponSoundSets();

	/**
	 * Voetstapbanken per oppervlak (Footsteps_Volume_02, owner-levering 26-07).
	 *
	 * HIER en niet in de anim-instance, en dat is de hele reden dat deze functie
	 * bestaat: elke soldaat heeft een eigen anim-instance, dus zes banken van zeven
	 * cues zouden per lichaam opnieuw geladen worden. Eén eigenaar, één keer laden.
	 */
	void LoadFootstepBanks();

	/**
	 * De herlaadketen per wapenfamilie: magazijn pakken, laten vallen, insteken,
	 * grendel. Als LIJST met tijdstippen en niet als één geluid bij de start —
	 * dat is precies wat de owner erover schreef, en het is ook waarom het pack
	 * ze genummerd levert.
	 */
	void LoadReloadFoley();

	UPROPERTY(Transient)
	TMap<FName, FEclipseFoleyChain> ReloadFoley;
	int32 ReloadFoleyCount = 0;

	/** Het geluid van een wapen dat je optilt, per familie. */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<USoundBase>> WeaponEquipCues;
	int32 EquipSoundCount = 0;

	UPROPERTY(Transient)
	TMap<uint8, FEclipseSoundVariantSet> FootstepBanks;
	int32 FootstepSoundCount = 0;


	/** Wanneer er voor het laatst een nagalm klonk, per schutter (wereldseconden). */
	TMap<TWeakObjectPtr<AActor>, double> LastTailSeconds;

	/** Diagnostiek voor de suite: hoeveel nagalmen er echt zijn gespeeld. */
	int32 TailSoundCount = 0;
	bool bWarnedMissingWeaponFamily = false;

	/** Treffers die om geluid vroegen. */
	int32 HitSoundCount = 0;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> ImpactCue;
	bool bTriedLoadImpact = false;
};
