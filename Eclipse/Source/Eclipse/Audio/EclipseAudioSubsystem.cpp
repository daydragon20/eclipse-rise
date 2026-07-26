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

	// Wapengeluid (gevechts-audit punt 12). Het schot-event bestaat sinds vanochtend
	// en vuurt bij elk schot dat de cadans passeert, raak of mis — precies het feit
	// waar een schotgeluid aan hoort te hangen.
	ShotFiredHandle = Bus.Subscribe(
		EclipseTags::Event_Combat_ShotFired,
		FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseAudioSubsystem::OnShotFired),
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
	}
	MissionCompletedHandle = FEclipseEventSubscriptionHandle();
	OrderAckHandle = FEclipseEventSubscriptionHandle();
	OrderRefusedHandle = FEclipseEventSubscriptionHandle();
	ShotFiredHandle = FEclipseEventSubscriptionHandle();
	LastBarkSeconds.Reset();
	BoundBus = nullptr;
}

void UEclipseAudioSubsystem::OnShotFired(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	const FEclipseCombatEventPayload* Shot = Payload.GetPtr<FEclipseCombatEventPayload>();
	if (Shot == nullptr)
	{
		return;
	}

	++ShotSoundCount;

	if (!bTriedLoadWeaponShot)
	{
		bTriedLoadWeaponShot = true;
		WeaponShotCue = LoadObject<USoundBase>(nullptr, WeaponShotCuePath);
		if (WeaponShotCue == nullptr)
		{
			UE_LOG(LogEclipse, Warning,
				TEXT("Audio: wapencue %s ontbreekt — schieten blijft stil (14.3.5)."), WeaponShotCuePath);
		}
	}

	// Op de PLEK van het schot en niet 2D: je moet kunnen horen dat er naast je
	// geschoten wordt, en straks waar vandaan. Het feit draagt zijn oorsprong al,
	// precies waarvoor die in het event zit.
	if (WeaponShotCue != nullptr && GetWorld() != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, WeaponShotCue, Shot->Origin, WeaponShotVolume);
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
