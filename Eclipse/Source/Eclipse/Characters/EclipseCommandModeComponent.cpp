#include "Characters/EclipseCommandModeComponent.h"
#include "EngineUtils.h"
#include "AI/EclipseSquadmateController.h"

#include "Characters/EclipseCharacter.h"
#include "Characters/EclipsePlayerController.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Quests/EclipseMissionLogic.h"
#include "Squad/EclipseCommandModeTuning.h"
#include "Squad/EclipseSquadSubsystem.h"

namespace
{
	// Debug tooling, not balance data, so a CVar is the right home (same call as
	// Eclipse.Events.HistorySize; GDD 14.2 reserves DataAssets for balance).
	TAutoConsoleVariable<float> CVarEclipseGauntletBeatIdleGapSeconds(
		TEXT("Eclipse.Gauntlet.BeatIdleGapSeconds"),
		45.0f,
		TEXT("Wall-clock quiet time after which the next Command Mode entry counts as a new encounter beat (R3 criterion 5). 0 disables the automatic split."),
		ECVF_Default);
}

UEclipseCommandModeComponent::UEclipseCommandModeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // zero per-frame work outside the hold (12.4)

	// Default asset path; the DA materialises via the commandlet queue. Until
	// then LoadSynchronous fails and the code defaults below carry Stage A.
	Tuning = TSoftObjectPtr<UEclipseCommandModeTuningAsset>(FSoftObjectPath(TEXT("/Game/Data/DA_CommandModeTuning.DA_CommandModeTuning")));
}

void UEclipseCommandModeComponent::BeginPlay()
{
	Super::BeginPlay();

	UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	if (UEclipseEventBusSubsystem* Bus = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		MissionEventsHandle = Bus->Subscribe(
			FGameplayTag::RequestGameplayTag(TEXT("Event.Mission")),
			FEclipseEventNativeDelegate::CreateUObject(this, &UEclipseCommandModeComponent::OnMissionEvent));
	}

	DumpCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Eclipse.Command.Dump"),
		TEXT("Dump Command Mode state (held, dilation, selection, orders issued) — Gauntlet loops and bug reports (SPEC-P2-02)."),
		FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
		{
			for (const FString& Line : GetDebugLines())
			{
				UE_LOG(LogEclipse, Display, TEXT("[CMD] %s"), *Line);
			}
			for (const FString& Line : GetGauntletUsageLines())
			{
				UE_LOG(LogEclipse, Display, TEXT("[CMD] %s"), *Line);
			}
		}),
		ECVF_Default);
}

void UEclipseCommandModeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Level travel / teardown is a fail-safe exit: never leave the world dilated.
	ExitMode(EclipseCommandLogic::EEclipseCommandSignal::LevelTravel);

	if (DumpCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(DumpCommand);
		DumpCommand = nullptr;
	}
	UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	if (UEclipseEventBusSubsystem* Bus = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		Bus->Unsubscribe(MissionEventsHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void UEclipseCommandModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Only reachable while held: the pawn-death watch (locked decision 5's
	// fail-safe list). Death of the commanding pawn ends the mode.
	const AController* Controller = Cast<AController>(GetOwner());
	const AEclipseCharacter* Body = Controller != nullptr ? Cast<AEclipseCharacter>(Controller->GetPawn()) : nullptr;
	if (Body == nullptr || Body->IsDowned())
	{
		ExitMode(EclipseCommandLogic::EEclipseCommandSignal::PawnDowned);
	}
}

void UEclipseCommandModeComponent::OnHoldPressed()
{
	if (State.bHeld)
	{
		return;
	}
	const double NowWallSeconds = FPlatformTime::Seconds();
	State = EclipseCommandLogic::ApplySignal(State, EclipseCommandLogic::EEclipseCommandSignal::HoldPressed, NowWallSeconds);

	// R3 criterion 5 telemetry rides the existing entry — the component already
	// owns ModeEntered/Exited, so the counter belongs here and not in the HUD.
	BeatTally.NoteModeEntered(NowWallSeconds, CVarEclipseGauntletBeatIdleGapSeconds.GetValueOnGameThread());

	ApplyDesiredDilation();
	SetComponentTickEnabled(true);
	BroadcastMode(/*bEntered*/ true);
}

void UEclipseCommandModeComponent::BeginNextEncounterBeat(bool bCountEmptyBeat)
{
	BeatTally.BeginNextBeat(bCountEmptyBeat);
}

void UEclipseCommandModeComponent::OnHoldReleased()
{
	ExitMode(EclipseCommandLogic::EEclipseCommandSignal::HoldReleased);
}

void UEclipseCommandModeComponent::ExitMode(EclipseCommandLogic::EEclipseCommandSignal Signal)
{
	if (!State.bHeld)
	{
		return;
	}
	const EclipseCommandLogic::FEclipseCommandModeState Ended = State;
	State = EclipseCommandLogic::ApplySignal(State, Signal, FPlatformTime::Seconds());
	ApplyDesiredDilation(); // bHeld is now false => exactly 1.0, always
	SetComponentTickEnabled(false);

	// ModeExited carries the R3 telemetry measured on the wall clock.
	UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	if (UEclipseEventBusSubsystem* Bus = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		FEclipseCommandEventPayload Payload;
		Payload.DilationFactor = ResolveDilationFactor();
		Payload.HeldSeconds = static_cast<float>(FPlatformTime::Seconds() - Ended.EnteredWallSeconds);
		Payload.OrdersIssuedWhileHeld = Ended.OrdersIssuedWhileHeld;
		Bus->Broadcast(EclipseTags::Event_Command_ModeExited, FInstancedStruct::Make(Payload));
	}
}

void UEclipseCommandModeComponent::BroadcastMode(bool bEntered) const
{
	if (!bEntered)
	{
		return; // exits broadcast from ExitMode with telemetry
	}
	UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	if (UEclipseEventBusSubsystem* Bus = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr)
	{
		FEclipseCommandEventPayload Payload;
		Payload.DilationFactor = ResolveDilationFactor();
		Bus->Broadcast(EclipseTags::Event_Command_ModeEntered, FInstancedStruct::Make(Payload));
	}
}

void UEclipseCommandModeComponent::ApplyDesiredDilation()
{
	// NOTE: SetGlobalTimeDilation clamps below WorldSettings::MinGlobalTimeDilation
	// (0.0001) — the Tactician "factor 0 = pause" path can NOT ship through this
	// call; its Stage B wiring must use SetPause/world pause instead. Today only
	// DilationFactor (ClampMin 0.05) reaches the world, so the clamp is inert.
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGlobalTimeDilation(World, EclipseCommandLogic::DesiredDilation(State, ResolveDilationFactor()));
	}
}

float UEclipseCommandModeComponent::ResolveDilationFactor() const
{
	const UEclipseCommandModeTuningAsset* Asset = ResolveTuning();
	return Asset != nullptr ? Asset->DilationFactor : 0.30f; // fallback mirrors the locked 4.1.2 value (GDD 14.3.5)
}

const UEclipseCommandModeTuningAsset* UEclipseCommandModeComponent::ResolveTuning() const
{
	// One load attempt per component lifetime: a missing DA would otherwise
	// retry on every HUD rebuild (review minor). The asset materialises via the
	// commandlet queue between sessions, so a session-scoped miss stays a miss.
	if (CachedTuning.IsValid())
	{
		return CachedTuning.Get();
	}
	if (!bTuningLoadAttempted)
	{
		bTuningLoadAttempted = true;
		const UEclipseCommandModeTuningAsset* Asset = Tuning.LoadSynchronous();
		if (Asset != nullptr)
		{
			CachedTuning = Asset;
			return Asset;
		}
		UE_LOG(LogEclipse, Warning, TEXT("CommandMode: DA_CommandModeTuning missing — code defaults stand in (GDD 14.3.5)."));
	}
	return nullptr;
}

void UEclipseCommandModeComponent::OnMissionEvent(FGameplayTag EventTag, const FInstancedStruct& Payload)
{
	if (EventTag == EclipseTags::Event_Mission_Completed || EventTag == EclipseTags::Event_Mission_Failed)
	{
		ExitMode(EclipseCommandLogic::EEclipseCommandSignal::MissionEnded);
		return;
	}

	if (EventTag == EclipseTags::Event_Mission_Started)
	{
		BeatTally.Reset(); // usage pull is per run, like the round-trip tally
		return;
	}

	// The site going loud opens a new encounter beat (SPEC-P2-04: alarm travels as
	// a named sub-phase, never its own event). The approach before it may not have
	// been a fight at all, so an empty pre-alarm window is not recorded as a beat.
	if (EventTag == EclipseTags::Event_Mission_PhaseChanged)
	{
		const FEclipseMissionEventPayload* Mission = Payload.GetPtr<FEclipseMissionEventPayload>();
		if (Mission != nullptr && Mission->bAuthoredSubPhase && Mission->PhaseName == EclipseMissionLogic::AlarmSubPhaseName)
		{
			BeatTally.BeginNextBeat(/*bCountEmptyBeat*/ false);
		}
	}
}

void UEclipseCommandModeComponent::CycleSoldierSelection(int32 Direction)
{
	if (!State.bHeld)
	{
		return;
	}
	const UEclipseSquadSubsystem* Squad = GetWorld() != nullptr ? GetWorld()->GetSubsystem<UEclipseSquadSubsystem>() : nullptr;
	if (Squad == nullptr)
	{
		return;
	}
	State.SelectedSoldier = EclipseCommandLogic::CycleSelection(Squad->GetAliveSquadmateIds(), State.SelectedSoldier, Direction);
}

void UEclipseCommandModeComponent::PickSoldierUnderReticle()
{
	if (!State.bHeld)
	{
		return;
	}
	AEclipsePlayerController* Controller = Cast<AEclipsePlayerController>(GetOwner());
	const UEclipseSquadSubsystem* Squad = GetWorld() != nullptr ? GetWorld()->GetSubsystem<UEclipseSquadSubsystem>() : nullptr;
	if (Controller == nullptr || Squad == nullptr)
	{
		return;
	}

	FVector AimLocation;
	AActor* AimActor = nullptr;
	Controller->GetAimPoint(AimLocation, AimActor);
	const AEclipseCharacter* Body = Cast<AEclipseCharacter>(AimActor);
	const APawn* OwnPawn = Controller->GetPawn();

	// Pick range is data (review: a field that does nothing is worse than no
	// field — 14.2); the aim trace reaches further, the pick does not.
	const UEclipseCommandModeTuningAsset* Asset = ResolveTuning();
	const float MaxRangeCm = Asset != nullptr ? Asset->SoldierSelectMaxRangeCm : 10000.0f;

	if (Body != nullptr && OwnPawn != nullptr
		&& FVector::Dist(OwnPawn->GetActorLocation(), Body->GetActorLocation()) <= MaxRangeCm
		&& Squad->IsRegisteredSquadmate(Body->GetSoldierId()))
	{
		State.SelectedSoldier = Body->GetSoldierId();
	}
	// A miss keeps the current selection: a failed pick is visible on the HUD
	// line, and clearing on every stray trace would fight the cycle keys.
}

void UEclipseCommandModeComponent::ToggleHeldStance()
{
	if (!State.bHeld)
	{
		return;
	}
	// Door alle vier cyclen, niet twee heen en weer. De volgorde is die van
	// oplopende agressie — Recon, Ready, Overwatch, Aggressive — zodat één druk
	// altijd dezelfde kant op beweegt en je weet waar je uitkomt.
	HeldStance = static_cast<EEclipseSquadStance>(
		(static_cast<uint8>(HeldStance) + 1) % 4);
	PushDoctrineToSquad();
}

void UEclipseCommandModeComponent::PushDoctrineToSquad()
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	for (TActorIterator<AEclipseSquadmateController> It(GetWorld()); It; ++It)
	{
		if (AEclipseSquadmateController* Mate = *It)
		{
			Mate->SetDoctrine(HeldStance);
		}
	}
}

void UEclipseCommandModeComponent::NotifyOrderIssued()
{
	if (State.bHeld)
	{
		++State.OrdersIssuedWhileHeld;
	}
}

TArray<FString> UEclipseCommandModeComponent::GetDebugLines() const
{
	TArray<FString> Lines;
	if (!State.bHeld)
	{
		// DE DOCTRINE STAAT ER OOK BUITEN DE MODUS (26-07 avond). Sinds vanavond
		// verandert hij echt gedrag en geldt hij door tot je hem wisselt — dus als
		// je hem alleen ziet terwijl je LB vasthoudt, kun je niet weten waarom je
		// squad zich anders gedraagt dan je verwacht. Een kader dat je niet ziet is
		// geen kader maar een verrassing.
		Lines.Add(FString::Printf(TEXT("command: idle (hold Q / pad LB)  ·  doctrine: %s"),
			EclipseSquad::StanceLabel(HeldStance)));
		return Lines;
	}

	const UEclipseSquadSubsystem* Squad = GetWorld() != nullptr ? GetWorld()->GetSubsystem<UEclipseSquadSubsystem>() : nullptr;
	Lines.Add(FString::Printf(TEXT("COMMAND MODE  x%.2f  held %.1fs  orders %d"),
		ResolveDilationFactor(),
		FPlatformTime::Seconds() - State.EnteredWallSeconds,
		State.OrdersIssuedWhileHeld));
	Lines.Add(FString::Printf(TEXT("  target: %s  doctrine: %s"),
		State.SelectedSoldier.IsValid() && Squad != nullptr && Squad->IsRegisteredSquadmate(State.SelectedSoldier)
			? *State.SelectedSoldier.ToString().Left(8)
			: TEXT("ALL"),
		EclipseSquad::StanceLabel(HeldStance)));
	Lines.Add(TEXT("  Tab/RB next · scroll/LT prev · E/X pick · Y doctrine · 1-4/D-pad order"));
	return Lines;
}

TArray<FString> UEclipseCommandModeComponent::GetGauntletUsageLines() const
{
	TArray<FString> Lines;
	Lines.Add(FString::Printf(TEXT("usage pull: %d entries this beat, avg %.1f over %d closed beats"),
		BeatTally.EntriesThisBeat, BeatTally.GetAverageEntriesPerBeat(), BeatTally.ClosedBeatCount));
	return Lines;
}
