#include "Squad/EclipseSquadSubsystem.h"

#include "AI/EclipseSquadmateController.h"
#include "Characters/EclipseCharacter.h"
#include "Core/EclipseEventBusSubsystem.h"
#include "Core/EclipseEventPayloads.h"
#include "Core/EclipseGameplayTags.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Quests/EclipseMissionSubsystem.h"
#include "Squad/EclipseSquadOrderLogic.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"
#include "StructUtils/InstancedStruct.h"

void UEclipseSquadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if !UE_BUILD_SHIPPING
	if (IConsoleManager::Get().FindConsoleObject(TEXT("Eclipse.Squad.DumpOrders")) == nullptr)
	{
		DumpCommand = IConsoleManager::Get().RegisterConsoleCommand(
			TEXT("Eclipse.Squad.DumpOrders"),
			TEXT("Log each squadmate's current order state (SPEC-P1-06 debug)."),
			FConsoleCommandDelegate::CreateWeakLambda(this, [this]()
			{
				UE_LOG(LogEclipse, Display, TEXT("Squad: %d registered squadmates."), Squadmates.Num());
				for (const FSquadmateEntry& Entry : Squadmates)
				{
					if (const AEclipseSquadmateController* Controller = Entry.Controller.Get())
					{
						UE_LOG(LogEclipse, Display, TEXT("  %s -> order %d"),
							*Entry.SoldierId.ToString(), static_cast<int32>(Controller->GetCurrentOrder()));
					}
				}
			}),
			ECVF_Default);
	}
#endif
}

void UEclipseSquadSubsystem::Deinitialize()
{
#if !UE_BUILD_SHIPPING
	if (DumpCommand != nullptr)
	{
		IConsoleManager::Get().UnregisterConsoleObject(DumpCommand);
		DumpCommand = nullptr;
	}
#endif
	Super::Deinitialize();
}

const UEclipseSquadTuningAsset* UEclipseSquadSubsystem::ResolveTuning() const
{
	const UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	const UEclipseCampaignSubsystem* Campaign = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseCampaignSubsystem>() : nullptr;
	const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
	return Setup != nullptr ? Setup->SquadTuning.LoadSynchronous() : nullptr;
}

void UEclipseSquadSubsystem::RegisterSquadmate(AEclipseSquadmateController* Controller, const FGuid& SoldierId)
{
	if (Controller == nullptr)
	{
		return;
	}

	FSquadmateEntry& Entry = Squadmates.AddDefaulted_GetRef();
	Entry.Controller = Controller;
	Entry.SoldierId = SoldierId;

	// The body reports downs; the mission runtime resolves them at debrief
	// (SPEC-P1-06/07 pipeline) — wired here so spawning stays one call.
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(Controller->GetPawn()))
	{
		Body->SetSoldierId(SoldierId);
		Body->OnDowned.AddWeakLambda(this, [this, Controller](AEclipseCharacter* Downed, FName Cause)
		{
			if (Controller != nullptr)
			{
				Controller->HandlePawnDowned();
			}

			UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
			if (GameInstance == nullptr)
			{
				return;
			}

			if (UEclipseEventBusSubsystem* Bus = GameInstance->GetSubsystem<UEclipseEventBusSubsystem>())
			{
				FEclipseSquadEventPayload Payload;
				Payload.SoldierId = Downed->GetSoldierId();
				Payload.Cause = Cause;
				Bus->Broadcast(EclipseTags::Event_Squad_SoldierDowned, FInstancedStruct::Make(Payload));
			}
			if (UEclipseMissionSubsystem* Mission = GameInstance->GetSubsystem<UEclipseMissionSubsystem>())
			{
				Mission->NotifySoldierDowned(Downed->GetSoldierId(), Cause);
			}
		});
	}
}

void UEclipseSquadSubsystem::UnregisterAll()
{
	Squadmates.Reset();
}

TArray<FString> UEclipseSquadSubsystem::GetOrderStateLines() const
{
	TArray<FString> Lines;
	for (const FSquadmateEntry& Entry : Squadmates)
	{
		const AEclipseSquadmateController* Controller = Entry.Controller.Get();
		const FString Order = Controller != nullptr
			? UEnum::GetValueAsString(Controller->GetCurrentOrder()).RightChop(FString(TEXT("EEclipseSquadOrder::")).Len())
			: TEXT("(lost)");
		Lines.Add(FString::Printf(TEXT("%s  ->  %s"), *Entry.SoldierId.ToString().Left(8), *Order));
	}
	return Lines;
}

bool UEclipseSquadSubsystem::IssueOrder(const FGuid& SoldierId, EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor)
{
	FSquadmateEntry* Entry = Squadmates.FindByPredicate(
		[&SoldierId](const FSquadmateEntry& Candidate) { return Candidate.SoldierId == SoldierId; });
	AEclipseSquadmateController* Controller = Entry != nullptr ? Entry->Controller.Get() : nullptr;
	if (Controller == nullptr)
	{
		UE_LOG(LogEclipse, Error, TEXT("IssueOrder: no squadmate with id %s"), *SoldierId.ToString());
		return false;
	}

	BroadcastOrderEvent(EclipseTags::Event_Squad_OrderIssued, SoldierId, Order, FString(), EEclipseOrderRefusalReason::None);

	const EclipseSquadOrderLogic::FEclipseOrderDecision Decision = Controller->ExecuteOrder(Order, TargetLocation, TargetActor);

	// Bark pools from data; the order id doubles as the row key.
	const UEclipseSquadTuningAsset* Tuning = ResolveTuning();
	const UDataTable* OrderDefs = Tuning != nullptr ? Tuning->OrderDefs.LoadSynchronous() : nullptr;
	const FName OrderRowName(*UEnum::GetValueAsString(Order).RightChop(FString(TEXT("EEclipseSquadOrder::")).Len()));
	const FEclipseSquadOrderDefRow* Row = OrderDefs != nullptr
		? OrderDefs->FindRow<FEclipseSquadOrderDefRow>(OrderRowName, TEXT("SquadOrder"), /*bWarnIfMissing*/ false)
		: nullptr;

	if (Decision.bAccepted)
	{
		const FString Bark = EclipseSquadOrderLogic::PickBarkLine(
			Row != nullptr ? Row->AcknowledgeLines : TArray<FString>(), SoldierId, static_cast<uint32>(Order));
		BroadcastOrderEvent(EclipseTags::Event_Squad_OrderAcknowledged, SoldierId, Order, Bark, EEclipseOrderRefusalReason::None);
	}
	else
	{
		const FString Bark = EclipseSquadOrderLogic::PickBarkLine(
			Row != nullptr ? Row->RefusalLines : TArray<FString>(), SoldierId, static_cast<uint32>(Order) + 100u);
		BroadcastOrderEvent(EclipseTags::Event_Squad_OrderRefused, SoldierId, Order, Bark, Decision.Reason);
	}
	return true;
}

void UEclipseSquadSubsystem::IssueOrderToAll(EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor)
{
	// Copy: entries never mutate mid-issue today, but order handlers may re-enter.
	const TArray<FSquadmateEntry> EntrySnapshot = Squadmates;
	for (const FSquadmateEntry& Entry : EntrySnapshot)
	{
		IssueOrder(Entry.SoldierId, Order, TargetLocation, TargetActor);
	}
}

void UEclipseSquadSubsystem::BroadcastOrderEvent(const FGameplayTag& Tag, const FGuid& SoldierId, EEclipseSquadOrder Order, const FString& BarkLine, EEclipseOrderRefusalReason Reason)
{
	UGameInstance* GameInstance = GetWorld() != nullptr ? GetWorld()->GetGameInstance() : nullptr;
	UEclipseEventBusSubsystem* Bus = GameInstance != nullptr ? GameInstance->GetSubsystem<UEclipseEventBusSubsystem>() : nullptr;
	if (Bus == nullptr)
	{
		return;
	}

	FEclipseSquadEventPayload Payload;
	Payload.SoldierId = SoldierId;
	Payload.Order = FName(*UEnum::GetValueAsString(Order));
	Payload.BarkLine = BarkLine;
	Payload.Reason = Reason == EEclipseOrderRefusalReason::None ? NAME_None : FName(*UEnum::GetValueAsString(Reason));
	Bus->Broadcast(Tag, FInstancedStruct::Make(Payload));

	if (!BarkLine.IsEmpty())
	{
		// Text-on-screen stub for VO (SPEC-P1-06 debug bark surface).
		UE_LOG(LogEclipse, Display, TEXT("[SQUAD %s] %s"), *Payload.Order.ToString(), *BarkLine);
	}
}
