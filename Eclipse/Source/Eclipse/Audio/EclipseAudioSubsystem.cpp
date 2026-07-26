#include "Audio/EclipseAudioSubsystem.h"

#include "Audio/EclipseCharacterVoiceData.h"
#include "Audio/EclipseDialogueVoiceSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	/**
	 * Sting mix level (GDD 16.12): sits clearly above the Layer-1 ambient bed
	 * (0.35 in the graybox builder) with headroom left for dialogue. Debug-tier
	 * named constant by choice — the real mix (with 16.7 ducking) belongs in the
	 * coming audio-layer DataAsset; a one-value DA for a single sting now would
	 * be ceremony without tuning value (14.2 governs balance data, and a
	 * placeholder mix level is not balance yet).
	 */
	constexpr float MissionCompleteStingVolume = 0.9f;

	const TCHAR* MissionCompleteStingPath = TEXT("/Game/Audio/Music/MUS_Sting_MissionComplete_Resistance_01.MUS_Sting_MissionComplete_Resistance_01");

	/**
	 * Wapengeluid (gevechts-audit 26-07, punt 12). De cue ligt sinds de audio-import
	 * in de repo en werd door niemand afgespeeld — betaalde, geïmporteerde audio
	 * die nooit klonk, dezelfde klasse als de acht squad-zinnen.
	 *
	 * 0,7 en niet 0,9 zoals de sting: een schot klinkt vaak (6,67 keer per seconde
	 * bij automatisch vuur) en de sting één keer per missie. Wat één keer indruk
	 * moet maken mag luider dan wat je honderd keer hoort.
	 */
	constexpr float WeaponShotVolume = 0.7f;

	const TCHAR* WeaponShotCuePath = TEXT("/Game/Audio/SFX/Cue_SFX_Weapon_RebelRifle_Shot_01.Cue_SFX_Weapon_RebelRifle_Shot_01");

	/**
	 * Inslag (gevechts-audit punt 4). Lag net als de schotcue ongebruikt in de
	 * repo — er was geen feit dat "raak" betekende, want ShotFired vuurt ook bij
	 * een misser. Sinds HitLanded bestaat is dat er wel.
	 *
	 * Luider dan het schot (0,85 tegen 0,7) en dat is de hele pointe: dit is het
	 * enige signaal dat je RAAKT. In elke shooter is die bevestiging belangrijker
	 * dan het schot zelf, want zonder haar weet je niet of je mist of dat de
	 * vijand veel leven heeft.
	 */
	constexpr float ImpactVolume = 0.85f;

	const TCHAR* ImpactCuePath = TEXT("/Game/Audio/SFX/Cue_SFX_Impact_BulletMetal_01.Cue_SFX_Impact_BulletMetal_01");

	/**
	 * WAPENGELUID PER FAMILIE (owner-levering 26-07 avond: FreeWeaponSounds).
	 *
	 * Waarom drie varianten en niet één: bij 6,67 schoten per seconde is één
	 * sample geen geluid meer maar een loop, en dat hoor je binnen een halve
	 * seconde. Elke shooter met automatisch vuur wisselt daarom af — Borderlands,
	 * Destiny en Battlefield doen het alle drie, meestal met drie tot vijf takes.
	 *
	 * De mapnaam is de familienaam uit DT_Weapons; het prefix is hoe het pack zijn
	 * bestanden noemt. Beide hier, op één plek, zodat een familie toevoegen één
	 * regel is.
	 */
	struct FWeaponSoundFamilyDef
	{
		const TCHAR* Family;
		const TCHAR* Folder;
		const TCHAR* Prefix;
	};

	// ALLEEN DE FAMILIES DIE EEN WAPEN ECHT GEBRUIKT (26-07 avond, na de
	// dode-assetsweep). Het pack levert er vier; DT_Weapons gebruikt er twee.
	// Shotgun en GrenadeLauncher laden was geen fout maar wel verspilling: cues
	// inladen die niets kan selecteren.
	//
	// Een familie erbij is één regel, op de dag dat er een hagelwapen of een
	// granaatwerper in de tabel staat — en dan valt hij meteen op zijn plek, want
	// de validator eist al dat SoundFamily een bestaande naam is.
	const FWeaponSoundFamilyDef WeaponSoundFamilies[] = {
		{ TEXT("AssaultRifle"),    TEXT("AssaultRifle"),    TEXT("assault_rifle") },
		{ TEXT("Handgun"),         TEXT("Handgun"),         TEXT("handgun") },
	};

	/**
	 * De nagalm — de staart die een schot RUIMTE geeft in plaats van een klik.
	 *
	 * Niet bij elk schot, en dat is een keuze met een reden. Een nagalm duurt
	 * ongeveer twee seconden; bij automatisch vuur zouden er dertien over elkaar
	 * heen liggen en dat wordt modder. Insurgency en Squad lossen het op met een
	 * limiet op gelijktijdige staarten. Deze rem doet hetzelfde met één getal: de
	 * nagalm klinkt bij het OPENENDE schot van een serie, dat is het schot dat in
	 * stilte valt en de ruimte laat horen.
	 */
	constexpr float TailCooldownSeconds = 0.6f;
	constexpr float TailVolume = 0.55f;

	/**
	 * VOETSTAPPEN PER OPPERVLAK (owner-levering 26-07: Footsteps_Volume_02).
	 *
	 * De mapnamen zijn zoals het pack ze levert, tikfout en al: "Foostep_Grass"
	 * mist een t. Die overschrijven zou het pack veranderen; hier staan betekent
	 * dat het klopt met wat er op schijf ligt, en dat is wat telt.
	 *
	 * Het getal is het oppervlaktetype uit DefaultEngine.ini. Als BYTE en niet als
	 * EPhysicalSurface, zodat deze tabel niets hoeft te weten van de physics-enum
	 * en een zevende oppervlak één regel is.
	 */
	struct FFootstepBankDef
	{
		uint8 SurfaceType;
		const TCHAR* Prefix;
		int32 Count;
	};

	const FFootstepBankDef FootstepBankDefs[] = {
		{ 1, TEXT("Footstep_Metal"),         6 },   // SurfaceType1 — loopvlak, rooster, dekplaat
		{ 2, TEXT("Footstep_Shoe_On_Street"), 7 },  // SurfaceType2 — plein en straat
		{ 3, TEXT("Footstep_Mud"),           7 },   // SurfaceType3
		{ 4, TEXT("Foostep_Grass"),          7 },   // SurfaceType4 — pack-tikfout, met opzet
		{ 5, TEXT("Footstep_Sand"),          7 },   // SurfaceType5
		{ 6, TEXT("Footstep_Wood"),          7 },   // SurfaceType6
	};

	/** Waar het op valt als er geen physical material onder je voeten ligt. */
	constexpr uint8 DefaultFootstepSurface = 2;

	/**
	 * DE HERLAADKETEN (owner-levering 26-07 avond).
	 *
	 * "Deze vier cues horen op de fasen van die animatie, niet als een enkel geluid
	 * aan het begin." De VOLGORDE komt uit het pack zelf: die nummert zijn takes
	 * (01 drop, 02 insert, 03 bolt), en dat is de handeling zoals hij echt gaat.
	 * Het magazijn pakken zit ertussen, want je grijpt het nieuwe terwijl het oude
	 * valt.
	 *
	 * De BREUKEN zijn van mij, en dat hoort erbij te staan: het pack levert geen
	 * timing. Ze zijn zo gelegd dat de grendel vlak voor het einde valt — dat is
	 * het moment waarop een speler weet dat hij weer kan schieten, en dat moment
	 * hoort hoorbaar te zijn vóór het waar wordt, niet erna.
	 */
	struct FReloadFoleyDef
	{
		const TCHAR* Family;
		float Fraction;
		const TCHAR* Path;
	};

	const FReloadFoleyDef ReloadFoleyDefs[] = {
		{ TEXT("AssaultRifle"), 0.10f, TEXT("/Game/FreeWeaponSounds/Cue/AssaultRifle/Foley/01_assault_rifle_reload_1_drop_the_mag_Cue.01_assault_rifle_reload_1_drop_the_mag_Cue") },
		{ TEXT("AssaultRifle"), 0.32f, TEXT("/Game/FreeWeaponSounds/Cue/AssaultRifle/Foley/assault_rifle_mag_draw_Cue.assault_rifle_mag_draw_Cue") },
		{ TEXT("AssaultRifle"), 0.58f, TEXT("/Game/FreeWeaponSounds/Cue/AssaultRifle/Foley/02_assault_rifle_reload_1_insert_the_mag_Cue.02_assault_rifle_reload_1_insert_the_mag_Cue") },
		{ TEXT("AssaultRifle"), 0.85f, TEXT("/Game/FreeWeaponSounds/Cue/AssaultRifle/Foley/03_assault_rifle_reload_1_bolt_Cue.03_assault_rifle_reload_1_bolt_Cue") },

		{ TEXT("Handgun"), 0.05f, TEXT("/Game/FreeWeaponSounds/Cue/Handgun/Foley/01_slide_lock_handgun_reload_1_Cue.01_slide_lock_handgun_reload_1_Cue") },
		{ TEXT("Handgun"), 0.28f, TEXT("/Game/FreeWeaponSounds/Cue/Handgun/Foley/02_drop_the_mag_handgun_reload_1_Cue.02_drop_the_mag_handgun_reload_1_Cue") },
		{ TEXT("Handgun"), 0.58f, TEXT("/Game/FreeWeaponSounds/Cue/Handgun/Foley/03_insert_the_mag_handgun_reload_1_Cue.03_insert_the_mag_handgun_reload_1_Cue") },
		{ TEXT("Handgun"), 0.85f, TEXT("/Game/FreeWeaponSounds/Cue/Handgun/Foley/04_slide_release_handgun_reload_1_Cue.04_slide_release_handgun_reload_1_Cue") },

	};

	constexpr float ReloadFoleyVolume = 0.8f;

	/** Het optillen van een wapen — het pack levert er één per familie. */
	const FReloadFoleyDef WeaponEquipDefs[] = {
		{ TEXT("AssaultRifle"), 0.0f, TEXT("/Game/FreeWeaponSounds/Cue/AssaultRifle/Foley/assault_rifle_weapon_equip_Cue.assault_rifle_weapon_equip_Cue") },
		{ TEXT("Handgun"),      0.0f, TEXT("/Game/FreeWeaponSounds/Cue/Handgun/Foley/handgun_weapon_equip_Cue.handgun_weapon_equip_Cue") },
	};
}

void UEclipseAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// The bus must outlive-and-precede every consumer (same discipline as the
	// gameplay subsystems: Base/Economy/Mission all InitializeDependency it).
	if (UEclipseEventBusSubsystem* Bus = Collection.InitializeDependency<UEclipseEventBusSubsystem>())
	{
		BindToBus(*Bus);
	}
}

void UEclipseAudioSubsystem::Deinitialize()
{
	UnbindFromBus();
	Super::Deinitialize();
}

void UEclipseAudioSubsystem::BindToBus(UEclipseEventBusSubsystem& Bus)
{
	UnbindFromBus();
	BoundBus = &Bus;
	// Concrete tag, not the Event.Mission family: today's only reaction is the
	// completion sting, and the typed subscription lets the bus type-check the
	// payload at the seam (producer/consumer drift fails loud, GDD 14.3.5).
	MissionCompletedHandle = Bus.Subscribe(
		EclipseTags::Event_Mission_Completed,
		FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseAudioSubsystem::OnMissionCompleted),
		FEclipseMissionEventPayload::StaticStruct());

	// De squad-barks (owner-beslissing 26-07). De opnames en het stemsysteem
	// bestonden allebei al; er was alleen niets dat ze afspeelde — acht
	// ingesproken zinnen die nooit klonken. Hier en niet in de squad-subsystem,
	// omdat dit de plek is die al naar de bus luistert en niets van orders weet:
	// audio reageert op FEITEN, het besluit blijft bij de orderlaag (14.2).
	OrderAckHandle = Bus.Subscribe(
		EclipseTags::Event_Squad_OrderAcknowledged,
		FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseAudioSubsystem::OnOrderAnswered),
		FEclipseSquadEventPayload::StaticStruct());
	OrderRefusedHandle = Bus.Subscribe(
		EclipseTags::Event_Squad_OrderRefused,
		FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseAudioSubsystem::OnOrderAnswered),
		FEclipseSquadEventPayload::StaticStruct());

	// De twee gevechtscues NU laden en niet bij het eerste schot (26-07).
	//
	// LoadObject is synchroon. Met het lazy patroon viel die laadtijd precies op de
	// eerste kogel van je eerste vuurgevecht — het slechtste moment dat er is. Het
	// harnas mat de slechtste tick op 14,5 ms tegen een budget van 16,7 (12.4), en
	// dat is te weinig marge om er drie synchrone laadacties in te laten vallen.
	//
	// Hier kost het niets zichtbaars: binden gebeurt bij het opstarten van het
	// subsysteem, waar een hitch niemand opvalt. De sting hieronder houdt zijn
	// lazy pad — die klinkt bij de DEBRIEF, en daar valt een hitch weg tussen het
	// afronden en het laden van het hub-scherm.
	if (!bTriedLoadWeaponShot)
	{
		bTriedLoadWeaponShot = true;
		WeaponShotCue = LoadObject<USoundBase>(nullptr, WeaponShotCuePath);
		if (WeaponShotCue == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Audio: wapencue %s ontbreekt — schieten blijft stil (14.3.5)."), WeaponShotCuePath);
		}

		// De familiesets en de voetstapbanken om exact dezelfde reden hier en niet
		// bij het eerste schot of de eerste stap.
		LoadWeaponSoundSets();
		LoadFootstepBanks();
		LoadReloadFoley();
	}
	if (!bTriedLoadImpact)
	{
		bTriedLoadImpact = true;
		ImpactCue = LoadObject<USoundBase>(nullptr, ImpactCuePath);
		if (ImpactCue == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Audio: inslagcue %s ontbreekt — treffers blijven stil (14.3.5)."), ImpactCuePath);
		}
	}

	// Wapengeluid (gevechts-audit punt 12). Het schot-event bestaat sinds vanochtend
	// en vuurt bij elk schot dat de cadans passeert, raak of mis — precies het feit
	// waar een schotgeluid aan hoort te hangen.
	ShotFiredHandle = Bus.Subscribe(
		EclipseTags::Event_Combat_ShotFired,
		FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseAudioSubsystem::OnShotFired),
		FEclipseCombatEventPayload::StaticStruct());
	HitLandedHandle = Bus.Subscribe(
		EclipseTags::Event_Combat_HitLanded,
		FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseAudioSubsystem::OnHitLanded),
		FEclipseCombatEventPayload::StaticStruct());
	ReloadStartedHandle = Bus.Subscribe(
		EclipseTags::Event_Combat_ReloadStarted,
		FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseAudioSubsystem::OnReloadStarted),
		FEclipseCombatEventPayload::StaticStruct());
	WeaponSwappedHandle = Bus.Subscribe(
		EclipseTags::Event_Combat_WeaponSwapped,
		FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseAudioSubsystem::OnWeaponSwapped),
		FEclipseCombatEventPayload::StaticStruct());
}

void UEclipseAudioSubsystem::UnbindFromBus()
{
	if (UEclipseEventBusSubsystem* Bus = BoundBus.Get())
	{
		Bus->Unsubscribe(MissionCompletedHandle);
		Bus->Unsubscribe(OrderAckHandle);
		Bus->Unsubscribe(OrderRefusedHandle);
		Bus->Unsubscribe(ShotFiredHandle);
		Bus->Unsubscribe(HitLandedHandle);
		Bus->Unsubscribe(ReloadStartedHandle);
		Bus->Unsubscribe(WeaponSwappedHandle);
	}
	MissionCompletedHandle = FEclipseEventSubscriptionHandle();
	OrderAckHandle = FEclipseEventSubscriptionHandle();
	OrderRefusedHandle = FEclipseEventSubscriptionHandle();
	ShotFiredHandle = FEclipseEventSubscriptionHandle();
	HitLandedHandle = FEclipseEventSubscriptionHandle();
	ReloadStartedHandle = FEclipseEventSubscriptionHandle();
	WeaponSwappedHandle = FEclipseEventSubscriptionHandle();
	LastBarkSeconds.Reset();
	BoundBus = nullptr;
}

void UEclipseAudioSubsystem::LoadWeaponSoundSets()
{
	// BIJ INITIALIZE en niet bij het eerste schot. Dat is de les van vanmiddag:
	// cues die synchroon laadden op het moment van vuren kostten 14,48 ms in de
	// zwaarste tick — een hapering precies bij de eerste knal van een gevecht.
	for (const FWeaponSoundFamilyDef& Def : WeaponSoundFamilies)
	{
		FEclipseSoundVariantSet Set;

		auto LoadInto = [](TArray<TObjectPtr<USoundBase>>& Into, const FString& Path)
		{
			if (USoundBase* Cue = LoadObject<USoundBase>(nullptr, *Path))
			{
				Into.Add(Cue);
			}
		};

		for (int32 Index = 1; Index <= 3; ++Index)
		{
			const FString Shot = FString::Printf(
				TEXT("/Game/FreeWeaponSounds/Cue/%s/Gunshots/%s_gunshot_%02d_Cue.%s_gunshot_%02d_Cue"),
				Def.Folder, Def.Prefix, Index, Def.Prefix, Index);
			LoadInto(Set.Shots, Shot);

			const FString Silenced = FString::Printf(
				TEXT("/Game/FreeWeaponSounds/Cue/%s/Gunshots/%s_sil_gunshot_%02d_Cue.%s_sil_gunshot_%02d_Cue"),
				Def.Folder, Def.Prefix, Index, Def.Prefix, Index);
			LoadInto(Set.Suppressed, Silenced);
		}
		for (int32 Index = 1; Index <= 2; ++Index)
		{
			const FString Tail = FString::Printf(
				TEXT("/Game/FreeWeaponSounds/Cue/%s/Gunshots/%s_tail_%02d_Cue.%s_tail_%02d_Cue"),
				Def.Folder, Def.Prefix, Index, Def.Prefix, Index);
			LoadInto(Set.Tails, Tail);
		}

		if (Set.Shots.Num() == 0)
		{
			// 14.3.5: luid degraderen. Een familie zonder schoten betekent dat het
			// pack er niet is, en dan hoort dat één keer hardop gezegd te worden in
			// plaats van dat vuren stilletjes stil blijft.
			EclipseDegradation::Note(TEXT("geluid ontbreekt"), TEXT("zie de logregel hieronder"));
			UE_LOG(LogEclipse, Warning,
				TEXT("Audio: wapenfamilie %s heeft geen schotcues — die wapens vallen terug op de losse cue."),
				Def.Family);
			continue;
		}

		WeaponSoundSets.Add(FName(Def.Family), MoveTemp(Set));
	}
}

int32 UEclipseAudioSubsystem::PickVariantIndex(int32 Count, int32 LastIndex)
{
	if (Count <= 0)
	{
		return INDEX_NONE;
	}
	if (Count == 1)
	{
		return 0;
	}
	if (LastIndex < 0 || LastIndex >= Count)
	{
		return FMath::RandRange(0, Count - 1);
	}

	// Trek uit de REST, niet uit alles: bij een gewone loting valt een op de drie
	// keer dezelfde variant, en twee identieke knallen achter elkaar is precies
	// wat afwisselen moest voorkomen.
	int32 Index = FMath::RandRange(0, Count - 2);
	if (Index >= LastIndex)
	{
		++Index;
	}
	return Index;
}

namespace
{
	USoundBase* PickVariant(const TArray<TObjectPtr<USoundBase>>& Options, int32& LastIndex)
	{
		const int32 Index = UEclipseAudioSubsystem::PickVariantIndex(Options.Num(), LastIndex);
		if (Index == INDEX_NONE)
		{
			return nullptr;
		}
		LastIndex = Index;
		return Options[Index];
	}
}

int32 UEclipseAudioSubsystem::GetWeaponSoundVariantCount(FName Family) const
{
	const FEclipseSoundVariantSet* Set = WeaponSoundSets.Find(Family);
	return Set != nullptr ? Set->Shots.Num() : 0;
}

void UEclipseAudioSubsystem::LoadFootstepBanks()
{
	for (const FFootstepBankDef& Def : FootstepBankDefs)
	{
		FEclipseSoundVariantSet Bank;
		for (int32 Index = 1; Index <= Def.Count; ++Index)
		{
			const FString Path = FString::Printf(
				TEXT("/Game/Footsteps_Volume_02/Cues/%s_%02d_Cue.%s_%02d_Cue"),
				Def.Prefix, Index, Def.Prefix, Index);
			if (USoundBase* Cue = LoadObject<USoundBase>(nullptr, *Path))
			{
				Bank.Shots.Add(Cue);
			}
		}

		if (Bank.Shots.Num() == 0)
		{
			EclipseDegradation::Note(TEXT("geluid ontbreekt"), TEXT("zie de logregel hieronder"));
			UE_LOG(LogEclipse, Warning,
				TEXT("Audio: voetstapbank %s is leeg — lopen op dat oppervlak blijft stil (14.3.5)."),
				Def.Prefix);
			continue;
		}
		FootstepBanks.Add(Def.SurfaceType, MoveTemp(Bank));
	}
}

void UEclipseAudioSubsystem::PlayFootstep(const FVector& Location, uint8 SurfaceType, float Volume)
{
	if (GetWorld() == nullptr)
	{
		return;
	}

	FEclipseSoundVariantSet* Bank = FootstepBanks.Find(SurfaceType);
	if (Bank == nullptr)
	{
		// Onbekend oppervlak: op de straat-bank. Stil vallen zou erger zijn, want
		// dan verdwijnt de speler zodra iemand een vloer vergeet te taggen.
		Bank = FootstepBanks.Find(DefaultFootstepSurface);
	}
	if (Bank == nullptr)
	{
		return;
	}

	// Zeven varianten per oppervlak, en dezelfde regel als bij de schoten: nooit
	// twee dezelfde achter elkaar. Bij drie stappen per seconde hoor je een
	// herhaling meteen.
	if (USoundBase* Variant = PickVariant(Bank->Shots, Bank->LastShotIndex))
	{
		++FootstepSoundCount;
		UGameplayStatics::PlaySoundAtLocation(this, Variant, Location, Volume);
	}
}

int32 UEclipseAudioSubsystem::CountLiveCues() const
{
	int32 Live = 0;
	auto CountSet = [&Live](const FEclipseSoundVariantSet& Set)
	{
		for (const TObjectPtr<USoundBase>& Cue : Set.Shots)      { Live += IsValid(Cue) ? 1 : 0; }
		for (const TObjectPtr<USoundBase>& Cue : Set.Suppressed) { Live += IsValid(Cue) ? 1 : 0; }
		for (const TObjectPtr<USoundBase>& Cue : Set.Tails)      { Live += IsValid(Cue) ? 1 : 0; }
	};
	for (const TPair<FName, FEclipseSoundVariantSet>& Pair : WeaponSoundSets) { CountSet(Pair.Value); }
	for (const TPair<uint8, FEclipseSoundVariantSet>& Pair : FootstepBanks)   { CountSet(Pair.Value); }
	for (const TPair<FName, FEclipseFoleyChain>& Pair : ReloadFoley)
	{
		for (const FEclipseFoleyStep& Step : Pair.Value.Steps) { Live += IsValid(Step.Cue) ? 1 : 0; }
	}
	for (const TPair<FName, TObjectPtr<USoundBase>>& Pair : WeaponEquipCues) { Live += IsValid(Pair.Value) ? 1 : 0; }
	return Live;
}

int32 UEclipseAudioSubsystem::GetFootstepVariantCount(uint8 SurfaceType) const
{
	const FEclipseSoundVariantSet* Bank = FootstepBanks.Find(SurfaceType);
	return Bank != nullptr ? Bank->Shots.Num() : 0;
}

void UEclipseAudioSubsystem::LoadReloadFoley()
{
	for (const FReloadFoleyDef& Def : ReloadFoleyDefs)
	{
		USoundBase* Cue = LoadObject<USoundBase>(nullptr, Def.Path);
		if (Cue == nullptr)
		{
			EclipseDegradation::Note(TEXT("geluid ontbreekt"), TEXT("zie de logregel hieronder"));
			UE_LOG(LogEclipse, Warning,
				TEXT("Audio: foley %s ontbreekt — die fase van het herladen blijft stil (14.3.5)."), Def.Path);
			continue;
		}
		ReloadFoley.FindOrAdd(FName(Def.Family)).Steps.Add({ Def.Fraction, Cue });
	}

	for (const FReloadFoleyDef& Def : WeaponEquipDefs)
	{
		if (USoundBase* Cue = LoadObject<USoundBase>(nullptr, Def.Path))
		{
			WeaponEquipCues.Add(FName(Def.Family), Cue);
		}
	}
}

void UEclipseAudioSubsystem::OnWeaponSwapped(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseCombatEventPayload* Swap = Payload.GetPtr<FEclipseCombatEventPayload>();
	if (Swap == nullptr || GetWorld() == nullptr)
	{
		return;
	}
	TObjectPtr<USoundBase>* Cue = WeaponEquipCues.Find(Swap->WeaponSoundFamily);
	if (Cue == nullptr || *Cue == nullptr)
	{
		return;
	}
	++EquipSoundCount;
	// Bij de schutter, niet 2D: ook een vijand die naar zijn pistool grijpt hoort
	// hoorbaar te zijn — dat is dezelfde tell als het herladen.
	if (AActor* Live = Swap->Shooter.Get())
	{
		UGameplayStatics::SpawnSoundAttached(*Cue, Live->GetRootComponent(), NAME_None,
			FVector::ZeroVector, EAttachLocation::SnapToTarget, true, ReloadFoleyVolume);
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(this, *Cue, Swap->Origin, ReloadFoleyVolume);
}

int32 UEclipseAudioSubsystem::GetReloadFoleyStepCount(FName Family) const
{
	const FEclipseFoleyChain* Chain = ReloadFoley.Find(Family);
	return Chain != nullptr ? Chain->Steps.Num() : 0;
}

void UEclipseAudioSubsystem::OnReloadStarted(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseCombatEventPayload* Reload = Payload.GetPtr<FEclipseCombatEventPayload>();
	UWorld* World = GetWorld();
	if (Reload == nullptr || World == nullptr)
	{
		return;
	}

	const FEclipseFoleyChain* Chain = ReloadFoley.Find(Reload->WeaponSoundFamily);
	if (Chain == nullptr)
	{
		return;
	}

	// AAN DE ACTOR en niet aan een plek: een herlaadbeurt duurt seconden en de
	// schutter loopt door. Foley die achterblijft waar hij begon klinkt als iemand
	// anders die daar staat te rommelen. Dat geldt ook voor een VIJAND die
	// herlaadt — dat is een tell die je hoort verplaatsen, en het is de reden dat
	// dit voor iedereen speelt en niet alleen voor de speler.
	AActor* Shooter = Reload->Shooter.Get();
	const FVector Fallback = Reload->Origin;
	const float Duration = FMath::Max(Reload->DurationSeconds, 0.05f);

	for (const FEclipseFoleyStep& Step : Chain->Steps)
	{
		USoundBase* Cue = Step.Cue;
		if (Cue == nullptr)
		{
			continue;
		}
		++ReloadFoleyCount;

		const float Delay = Step.Fraction * Duration;
		TWeakObjectPtr<AActor> WeakShooter(Shooter);
		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateWeakLambda(this, [this, Cue, WeakShooter, Fallback]()
			{
				if (AActor* Live = WeakShooter.Get())
				{
					UGameplayStatics::SpawnSoundAttached(Cue, Live->GetRootComponent(), NAME_None,
						FVector::ZeroVector, EAttachLocation::SnapToTarget, /*bStopWhenAttachedToDestroyed=*/true,
						ReloadFoleyVolume);
					return;
				}
				// Schutter weg tijdens het herladen (neergeschoten). Het geluid nog
				// afmaken waar hij stond: dat is wat je zou horen.
				UGameplayStatics::PlaySoundAtLocation(this, Cue, Fallback, ReloadFoleyVolume);
			}),
			FMath::Max(Delay, 0.001f), /*bLoop=*/false);
	}
}

void UEclipseAudioSubsystem::OnShotFired(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseCombatEventPayload* Shot = Payload.GetPtr<FEclipseCombatEventPayload>();
	if (Shot == nullptr)
	{
		return;
	}

	++ShotSoundCount;

	// Op de PLEK van het schot en niet 2D: je moet kunnen horen dat er naast je
	// geschoten wordt, en straks waar vandaan. Het feit draagt zijn oorsprong al,
	// precies waarvoor die in het event zit.
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FEclipseSoundVariantSet* Set = WeaponSoundSets.Find(Shot->WeaponSoundFamily);
	if (Set == nullptr)
	{
		// Terugval op de losse cue: een wapen zonder (of met een onbekende) familie
		// hoort te knallen, niet stil te zijn. Eén logregel, want een tikfout in de
		// tabel mag niet per schot een regel kosten.
		if (!bWarnedMissingWeaponFamily && !Shot->WeaponSoundFamily.IsNone())
		{
			bWarnedMissingWeaponFamily = true;
			UE_LOG(LogEclipse, Warning,
				TEXT("Audio: onbekende wapenfamilie '%s' — dat wapen valt terug op de losse schotcue (14.3.5)."),
				*Shot->WeaponSoundFamily.ToString());
		}
		if (WeaponShotCue != nullptr)
		{
			UGameplayStatics::PlaySoundAtLocation(this, WeaponShotCue, Shot->Origin, WeaponShotVolume);
		}
		return;
	}

	// Gedempt heeft een eigen set; ontbreekt die (het pack levert geen gedempte
	// hagel), dan klinkt het gewone schot. Stil vallen zou erger zijn dan het
	// verkeerde timbre.
	const TArray<TObjectPtr<USoundBase>>& Bank =
		(Shot->bSuppressed && Set->Suppressed.Num() > 0) ? Set->Suppressed : Set->Shots;

	if (USoundBase* Variant = PickVariant(Bank, Set->LastShotIndex))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Variant, Shot->Origin, WeaponShotVolume);
	}

	// De nagalm, met de rem uit TailCooldownSeconds. Per SCHUTTER, want twee man
	// die naast elkaar vuren horen allebei hun eigen ruimte te maken — net als de
	// rem op de squad-stemmen, en om dezelfde reden.
	if (Set->Tails.Num() > 0 && !Shot->bSuppressed)
	{
		const double Now = World->GetTimeSeconds();
		const TWeakObjectPtr<AActor> Key = Shot->Shooter;
		const double* Last = LastTailSeconds.Find(Key);
		if (Last == nullptr || Now - *Last >= TailCooldownSeconds)
		{
			LastTailSeconds.Add(Key, Now);
			if (USoundBase* Tail = PickVariant(Set->Tails, Set->LastTailIndex))
			{
				++TailSoundCount;
				UGameplayStatics::PlaySoundAtLocation(this, Tail, Shot->Origin, TailVolume);
			}
		}
	}
}

void UEclipseAudioSubsystem::OnHitLanded(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseCombatEventPayload* Landed = Payload.GetPtr<FEclipseCombatEventPayload>();
	if (Landed == nullptr)
	{
		return;
	}

	++HitSoundCount;

	// Een kopschot klinkt harder. Dat is de goedkoopste manier om punt 5 uit de
	// audit half op te lossen: je hóórt het verschil, ook zonder hitmarker. Een
	// eigen cue zou beter zijn, maar die ligt er niet en er een genereren is een
	// owner-beslissing (dat kost een API-aanroep).
	if (ImpactCue != nullptr && GetWorld() != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactCue, Landed->Origin,
			Landed->bHeadshot ? ImpactVolume * 1.35f : ImpactVolume);
	}
}

void UEclipseAudioSubsystem::OnOrderAnswered(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseSquadEventPayload* Squad = Payload.GetPtr<FEclipseSquadEventPayload>();
	if (Squad == nullptr)
	{
		return;
	}

	// De rem, per soldaat (owner: 2 s). Vóór alle laadwerk, want een bark die de
	// rem tegenhoudt hoort niets te kosten.
	const UWorld* World = GetWorld();
	const double Now = World != nullptr ? World->GetTimeSeconds() : FPlatformTime::Seconds();
	if (const double* Last = LastBarkSeconds.Find(Squad->SoldierId))
	{
		if (Now - *Last < BarkCooldownSeconds)
		{
			++BarkSuppressedCount;
			return;
		}
	}

	// Welke zin? De opnames zijn geschreven tegen een ANDERE woordenlijst dan de
	// code, gemeten in de nacht van 25→26 juli. Deze tabel is die vertaling, en
	// hij staat hier expliciet in plaats van als naamtruc, omdat de gaten er dan
	// leesbaar in staan:
	//
	//   MoveTo        -> Bark_Ack_Move        (beide stemmen)
	//   Hold          -> Bark_Ack_Hold        (alleen A)
	//   Regroup       -> Bark_Ack_Regroup     (alleen B)
	//   FocusTarget   -> Bark_Ack_Focus       (alleen A; geschreven 26-07)
	//   NoRoute       -> Bark_Refuse_NoRoute  (alleen A)
	//   NoLineOfSight -> Bark_Refuse_NoShot   (alleen B; andere naam, zelfde reden)
	//   InvalidTarget -> Bark_Refuse_Invalid  (alleen B; geschreven 26-07)
	//   Downed        -> Bark_Down            (alleen B)
	// De wees Bark_Refuse_Pinned_A is 26-07 uit de seed gehaald: "pinned down"
	// belooft suppressie, en die mechaniek bestaat niet. De opname blijft op
	// schijf staan — betaalde audio weggooien is een owner-beslissing.
	//
	// Ontbreekt de zin, dan zwijgt de soldaat op het scherm niet: de tekstbark
	// staat er los van. Alleen de stem blijft weg, en dat wordt één keer gemeld.
	// De payload draagt de enums als volledige string ("EEclipseSquadOrder::Hold"),
	// dus vergelijken op het laatste segment. Dat houdt de audiolaag ook los van
	// de squad-headers: audio hoeft niet te weten dat er een orderenum bestaat,
	// alleen hoe het feit heet dat er langskomt.
	auto ShortName = [](const FName& Full)
	{
		const FString Text = Full.ToString();
		int32 Colon = INDEX_NONE;
		return Text.FindLastChar(TEXT(':'), Colon) ? Text.RightChop(Colon + 1) : Text;
	};

	const bool bRefused = EventTag == EclipseTags::Event_Squad_OrderRefused;
	FString LineText;
	EEclipseVoiceEmotion Emotion = EEclipseVoiceEmotion::Calm;
	if (bRefused)
	{
		const FString Reason = ShortName(Squad->Reason);
		if (Reason == TEXT("NoRoute"))
		{
			LineText = TEXT("Negative, I can't find a route.");
			Emotion = EEclipseVoiceEmotion::Urgent;
		}
		else if (Reason == TEXT("NoLineOfSight"))
		{
			LineText = TEXT("No shot from here.");
			Emotion = EEclipseVoiceEmotion::Urgent;
		}
		else if (Reason == TEXT("Downed"))
		{
			LineText = TEXT("I'm hit. Someone patch me up.");
			Emotion = EEclipseVoiceEmotion::Sad;
		}
		else if (Reason == TEXT("InvalidTarget"))
		{
			LineText = TEXT("That's not a target.");
			Emotion = EEclipseVoiceEmotion::Urgent;
		}
	}
	else
	{
		const FString Order = ShortName(Squad->Order);
		if (Order == TEXT("MoveTo"))
		{
			LineText = TEXT("Copy. Moving up.");
			Emotion = EEclipseVoiceEmotion::Confident;
		}
		else if (Order == TEXT("Hold"))
		{
			LineText = TEXT("Holding position.");
			Emotion = EEclipseVoiceEmotion::Calm;
		}
		else if (Order == TEXT("Regroup"))
		{
			LineText = TEXT("Falling back to you.");
			Emotion = EEclipseVoiceEmotion::Calm;
		}
		else if (Order == TEXT("FocusTarget"))
		{
			LineText = TEXT("Eyes on. Engaging.");
			Emotion = EEclipseVoiceEmotion::Confident;
		}
	}

	// De rem gaat aan zodra vaststaat DAT deze soldaat antwoordt — niet pas als
	// het geluidsbestand gevonden is. Eerst stond dat andersom, en dat maakte het
	// gedrag afhankelijk van welke clips er toevallig op die machine gegenereerd
	// waren: op de ene remt hij, op de andere niet. De rem hoort bij de soldaat,
	// niet bij de audio-pijplijn.
	if (!LineText.IsEmpty())
	{
		LastBarkSeconds.Add(Squad->SoldierId, Now);
	}

	if (LineText.IsEmpty())
	{
		if (!bWarnedMissingBarkLine)
		{
			bWarnedMissingBarkLine = true;
			UE_LOG(LogEclipse, Warning,
				TEXT("Audio: geen ingesproken zin voor deze order/reden — de soldaat antwoordt alleen op het scherm (14.3.5)."));
		}
		return;
	}

	UEclipseDialogueVoiceSubsystem* Voices = GetGameInstance() != nullptr
		? GetGameInstance()->GetSubsystem<UEclipseDialogueVoiceSubsystem>() : nullptr;
	if (Voices == nullptr)
	{
		return;
	}

	// Twee stemmen, vast per soldaat: dezelfde man klinkt altijd hetzelfde. De
	// GUID beslist, dus het is stabiel over sessies en vraagt geen extra data.
	const bool bPrefersA = (GetTypeHash(Squad->SoldierId) & 1) == 0;
	const TCHAR* PathA = TEXT("/Game/Audio/DA_Voice_SquadA.DA_Voice_SquadA");
	const TCHAR* PathB = TEXT("/Game/Audio/DA_Voice_SquadB.DA_Voice_SquadB");
	const TCHAR* Order[2] = { bPrefersA ? PathA : PathB, bPrefersA ? PathB : PathA };

	// ALLEEN al gegenereerde zinnen afspelen — nooit laten genereren.
	//
	// PlayLine valt bij een onbekende zin terug op "genereer nu en speel bij
	// aankomst", en dat is een betaalde ElevenLabs-aanroep. Dat is geld van de
	// owner en dus geen beslissing die in een bark-handler thuishoort. De acht
	// zinnen zijn per stem ingesproken (A: Move, Hold, NoRoute, Pinned; B: Move,
	// Regroup, NoShot, Down), dus een zin die de eigen stem niet heeft pakt de
	// andere stem — liever een andere stem dan stilte of een rekening. Heeft geen
	// van beide hem, dan blijft het stil en wordt dat één keer gemeld.
	const UEclipseCharacterVoiceData* Chosen = nullptr;
	for (const TCHAR* Path : Order)
	{
		const UEclipseCharacterVoiceData* Candidate = LoadObject<UEclipseCharacterVoiceData>(nullptr, Path);
		if (Candidate != nullptr && Voices->IsLineCached(Candidate, Emotion, LineText))
		{
			Chosen = Candidate;
			break;
		}
	}

	if (Chosen == nullptr)
	{
		if (!bWarnedMissingVoiceAsset)
		{
			bWarnedMissingVoiceAsset = true;
			UE_LOG(LogEclipse, Warning,
				TEXT("Audio: '%s' is voor geen van beide squad-stemmen gegenereerd — deze bark blijft stil (14.3.5). ")
				TEXT("Bewust GEEN generatie vanuit gameplay: dat kost een API-aanroep. Draai Tools/generate_audio_assets.py."),
				*LineText);
		}
		return;
	}

	++BarkRequestCount;
	Voices->PlayLine(Chosen, Emotion, LineText, this);
}

void UEclipseAudioSubsystem::OnMissionCompleted(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	++StingRequestCount;

	if (!bTriedLoadSting)
	{
		bTriedLoadSting = true;
		MissionCompleteSting = LoadObject<USoundBase>(nullptr, MissionCompleteStingPath);
		if (MissionCompleteSting == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("Audio: mission-complete sting %s missing — debrief stays silent (GDD 14.3.5; run Tools/generate_audio_assets.py + import_generated_audio.py)."),
				MissionCompleteStingPath);
		}
	}
	if (MissionCompleteSting == nullptr)
	{
		return;
	}

	// 2D by design (16.12): the sting is non-diegetic debrief punctuation, not a
	// world sound. World-less contexts (unit tests, commandlets) consume the
	// fact but skip playback — degradation, never a crash (GDD 14.3.5).
	UWorld* World = GetGameInstance() != nullptr ? GetGameInstance()->GetWorld() : nullptr;
	if (World == nullptr)
	{
		return;
	}
	UGameplayStatics::PlaySound2D(World, MissionCompleteSting, MissionCompleteStingVolume);
}
