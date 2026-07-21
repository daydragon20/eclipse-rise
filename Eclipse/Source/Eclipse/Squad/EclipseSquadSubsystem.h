#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Squad/EclipseSquadTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "EclipseSquadSubsystem.generated.h"

class AEclipseCharacter;
class AEclipseSquadmateController;
class IConsoleObject;

/**
 * Order dispatch for the squad of 2 (SPEC-P1-06). Every issued order produces
 * exactly one Acknowledged or Refused event with a bark line — the zero-silence
 * contract (GDD 8.4) is enforced here by construction: the controller API
 * returns a decision synchronously, and this subsystem always broadcasts it.
 */
UCLASS()
class ECLIPSE_API UEclipseSquadSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Register a spawned squadmate (game mode wiring at mission start). */
	void RegisterSquadmate(AEclipseSquadmateController* Controller, const FGuid& SoldierId);
	void UnregisterAll();

	/** Issue to one squadmate by roster id; returns false only for unknown ids (unknown = caller bug, not refusal). */
	bool IssueOrder(const FGuid& SoldierId, EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor);

	/** Issue to everyone (the Phase 1 hotkey path). */
	void IssueOrderToAll(EEclipseSquadOrder Order, const FVector& TargetLocation, AActor* TargetActor);

	int32 GetSquadmateCount() const { return Squadmates.Num(); }

private:
	struct FSquadmateEntry
	{
		TWeakObjectPtr<AEclipseSquadmateController> Controller;
		FGuid SoldierId;
	};

	void BroadcastOrderEvent(const FGameplayTag& Tag, const FGuid& SoldierId, EEclipseSquadOrder Order, const FString& BarkLine, EEclipseOrderRefusalReason Reason);
	const UEclipseSquadTuningAsset* ResolveTuning() const;

	TArray<FSquadmateEntry> Squadmates;
	IConsoleObject* DumpCommand = nullptr;
};
