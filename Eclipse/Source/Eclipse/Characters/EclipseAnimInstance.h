#pragma once

#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Characters/EclipseLocomotionTypes.h"
#include "CoreMinimal.h"
#include "EclipseAnimInstance.generated.h"

class AEclipseCharacter;
class UAnimSequence;

/**
 * Worker-thread half of the locomotion layer.
 *
 * WHY A PROXY AND NOT AN ANIMBLUEPRINT: a C++ UAnimInstance on its own has no
 * anim graph, so it outputs the ref pose no matter what its variables say — the
 * graph is the thing that produces a pose, and a graph is an editor-authored
 * asset. Overriding FAnimInstanceProxy::Evaluate is the engine's own answer to
 * that (UAnimSingleNodeInstance is built exactly this way), and it keeps this
 * whole tier inside the repository: no .uasset to author, no editor session in
 * the loop, no binary blob for a reviewer to take on faith. When the real
 * locomotion ABP lands it replaces this class wholesale; until then the owner
 * can actually see walking.
 *
 * Threading follows the engine contract: UObject pointers are copied in
 * InitializeObjects and dropped in ClearObjects, gameplay state is pushed from
 * the game thread in UEclipseAnimInstance::NativeUpdateAnimation, and Update /
 * Evaluate touch nothing but their own members.
 */
USTRUCT()
struct FEclipseLocomotionProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FEclipseLocomotionProxy()
	{
	}

	FEclipseLocomotionProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{
	}

	/** Pushed once per frame from the game thread, before the parallel update. */
	void SetLocomotionState(const FEclipseLocomotionBlend& InBlend, float InStrideRate, bool bInIsInAir, bool bInIsDowned,
		float InMoveDirectionDegrees);

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void InitializeObjects(UAnimInstance* InAnimInstance) override;
	virtual void ClearObjects() override;
	virtual void Update(float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	/** Weighted mean cycle length of the gait clips currently blending, or 0. */
	float GaitCycleLength() const;

	// Copied in InitializeObjects, dropped in ClearObjects — never dereferenced
	// outside the animation update/evaluate window, per the engine contract.
	UAnimSequence* Idle = nullptr;
	UAnimSequence* Walk = nullptr;
	UAnimSequence* Run = nullptr;
	UAnimSequence* Death = nullptr;

	/** Richtingsvarianten; null = terugval op de vooruit-cyclus (26-07, punt 8). */
	UAnimSequence* WalkBack = nullptr;
	UAnimSequence* WalkLeft = nullptr;
	UAnimSequence* WalkRight = nullptr;
	UAnimSequence* RunBack = nullptr;
	UAnimSequence* RunLeft = nullptr;
	UAnimSequence* RunRight = nullptr;

	/**
	 * Bewegingsrichting ten opzichte van waar het lichaam KIJKT, in graden:
	 * 0 = vooruit, +90 = naar rechts strafen, ±180 = achteruit, -90 = naar links.
	 * Vóór 26-07 bestond dit niet en hoefde het ook niet: het lichaam draaide naar
	 * zijn bewegingsrichting, dus die hoek was per definitie 0.
	 */
	float MoveDirectionDegrees = 0.0f;

	FEclipseLocomotionBlend Blend;
	float StrideRate = 1.0f;
	bool bIsInAir = false;
	bool bIsDowned = false;

	/** Idle runs on its own clock; it is not a gait and has no stride to match. */
	float IdleTime = 0.0f;

	/**
	 * Walk and run share ONE normalized phase in [0,1) rather than each running
	 * its own clock. Two free-running clocks cross-fading is what makes a blended
	 * locomotion look like two people arguing over one pair of legs: the contact
	 * frames drift apart and the result is a body with four half-visible feet.
	 * Normalized phase is the cheap version of what a sync group does.
	 */
	float GaitPhase = 0.0f;

	/** One-shot, clamped to the last frame — a death take must not loop. */
	float DeathTime = 0.0f;
};

/**
 * Minimal locomotion for the one character body (GDD 14.5 debug-grade,
 * SPEC-P2-01 tier). Reads ground speed and airborne state off the owning pawn
 * and cross-fades idle -> walk -> run on the DA_CharacterTuning speeds.
 *
 * Before this, ApplyBodyDef put the mesh in AnimationSingleNode and played the
 * idle on a loop, so every body in the game — player, squad, every enemy —
 * slid around the district in a standing pose. That is the defect this class
 * exists to close; it is not a locomotion system and does not pretend to be one
 * (no state machine, no starts/stops/pivots, no aim offset, no foot IK).
 */
UCLASS()
class ECLIPSE_API UEclipseAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	/** Install the clips + anchors for this body (game thread; ApplyBodyDef calls it). */
	void SetLocomotionSet(const FEclipseLocomotionSet& InSet);

	const FEclipseLocomotionSet& GetLocomotionSet() const { return LocomotionSet; }

	// ---- Read-only state, so a future ABP can sit on top of this without
	// re-deriving any of it (and so the animation debugger shows real numbers).

	/** Horizontal speed in cm/s — Z is dropped, falling is not walking. */
	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Locomotion")
	float GroundSpeed = 0.0f;

	/**
	 * Bewegingsrichting t.o.v. de kijkrichting (26-07, punt 8): 0 = vooruit,
	 * +90 = strafe rechts, ±180 = achteruit. Blueprint-leesbaar zodat een
	 * toekomstige animatiegraaf hem kan gebruiken zonder de proxy aan te raken.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Locomotion")
	float MoveDirectionDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Locomotion")
	bool bIsInAir = false;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Locomotion")
	bool bIsDowned = false;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Locomotion")
	float IdleWeight = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Locomotion")
	float WalkWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Locomotion")
	float RunWeight = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Eclipse|Locomotion")
	float StrideRate = 1.0f;

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

private:
	/** Fill the blend anchors from DA_CharacterTuning; keep the locked feel
	 *  targets when the campaign setup has none (GDD 14.3.5). */
	void ApplyTuningAnchors();

	UPROPERTY(Transient)
	FEclipseLocomotionSet LocomotionSet;

	UPROPERTY(Transient)
	TObjectPtr<AEclipseCharacter> OwningBody = nullptr;
};
