#include "Characters/EclipseAnimInstance.h"

#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimTypes.h"
#include "Animation/AnimationAsset.h"
#include "Animation/AnimationPoseData.h"
#include "Animation/AttributesRuntime.h"
#include "Animation/Skeleton.h"
#include "AnimationRuntime.h"
#include "BonePose.h"
#include "Characters/EclipseCharacter.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Eclipse.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Strategy/EclipseCampaignSetupAsset.h"
#include "Strategy/EclipseCampaignSubsystem.h"

namespace
{
	/** One clip's contribution to this frame's pose. */
	struct FEclipseLocomotionSample
	{
		UAnimSequence* Sequence = nullptr;
		float Weight = 0.0f;
		float Time = 0.0f;
		bool bLooping = true;
	};

	void SampleInto(const FEclipseLocomotionSample& Sample, FAnimationPoseData& OutPoseData)
	{
		const FAnimExtractContext Context(
			static_cast<double>(Sample.Time),
			/*bExtractRootMotion*/ false,
			FDeltaTimeRecord(),
			Sample.bLooping);
		Sample.Sequence->GetAnimationPose(OutPoseData, Context);
	}

	/** Wrap a looping clock without a while-loop: a hitch must cost one frame, not a spin. */
	float AdvanceLooping(float Time, float Delta, float Length)
	{
		if (Length <= KINDA_SMALL_NUMBER)
		{
			return 0.0f;
		}
		const float Advanced = FMath::Fmod(Time + Delta, Length);
		return Advanced < 0.0f ? Advanced + Length : Advanced;
	}
}

// ---------------------------------------------------------------------------
// The pure half. No world, no assets, no time — see EclipseLocomotionTypes.h.
// ---------------------------------------------------------------------------

EEclipseLocomotionTier EclipseLocomotion::ClassifyTier(bool bHasIdle, bool bHasWalk, bool bHasRun)
{
	// Idle is the floor, not a nicety: without a standing pose there is nothing
	// to cross-fade out of, and a body that only owns a walk cycle would jitter
	// on the spot the moment it stopped.
	if (!bHasIdle)
	{
		return EEclipseLocomotionTier::Unavailable;
	}
	if (bHasWalk && bHasRun)
	{
		return EEclipseLocomotionTier::TwoGaits;
	}
	if (bHasWalk || bHasRun)
	{
		return EEclipseLocomotionTier::OneGait;
	}
	return EEclipseLocomotionTier::IdleOnly;
}

bool EclipseLocomotion::IsUsableOn(const USkeleton* ClipSkeleton, const USkeleton* MeshSkeleton)
{
	return ClipSkeleton != nullptr && ClipSkeleton == MeshSkeleton;
}

namespace
{
	/**
	 * Anchors, defensively ordered. A tuning asset with RunSpeed <= WalkSpeed is a
	 * data mistake, and a data mistake must not be able to invert the ramp or
	 * divide by zero (GDD 14.3.5) — it degrades to a still-monotonic curve.
	 */
	void OrderAnchors(float InWalk, float InRun, float& OutWalk, float& OutRun)
	{
		OutWalk = FMath::Max(InWalk, 1.0f);
		OutRun = FMath::Max(InRun, OutWalk + 1.0f);
	}
}

FEclipseLocomotionBlend EclipseLocomotion::ComputeBlend(float GroundSpeed, float InWalkAnchor, float InRunAnchor, bool bHasWalk, bool bHasRun)
{
	FEclipseLocomotionBlend Out;

	float WalkAnchor = 0.0f;
	float RunAnchor = 0.0f;
	OrderAnchors(InWalkAnchor, InRunAnchor, WalkAnchor, RunAnchor);
	const float Speed = FMath::Max(GroundSpeed, 0.0f);

	if (Speed <= StandingSpeedThreshold)
	{
		return Out;
	}

	// One gait only: it has to carry the whole ramp on its own, anchored at the
	// speed it was authored for. Half a locomotion still reads as locomotion.
	if (bHasWalk != bHasRun)
	{
		const float Anchor = bHasWalk ? WalkAnchor : RunAnchor;
		const float Alpha = FMath::Clamp(Speed / Anchor, 0.0f, 1.0f);
		Out.IdleWeight = 1.0f - Alpha;
		Out.WalkWeight = bHasWalk ? Alpha : 0.0f;
		Out.RunWeight = bHasRun ? Alpha : 0.0f;
		return Out;
	}

	if (!bHasWalk)
	{
		// No gait at all: standing is the only pose this body owns.
		return Out;
	}

	if (Speed <= WalkAnchor)
	{
		const float Alpha = Speed / WalkAnchor;
		Out.IdleWeight = 1.0f - Alpha;
		Out.WalkWeight = Alpha;
		return Out;
	}

	// Above the run anchor (i.e. sprinting) this saturates at full run — the
	// extra speed goes into the stride rate instead, because there is no sprint
	// take in any of the packs DT_BodyDefs points at.
	const float Alpha = FMath::Clamp((Speed - WalkAnchor) / (RunAnchor - WalkAnchor), 0.0f, 1.0f);
	Out.IdleWeight = 0.0f;
	Out.WalkWeight = 1.0f - Alpha;
	Out.RunWeight = Alpha;
	return Out;
}

float EclipseLocomotion::ComputeStrideRate(float GroundSpeed, float InWalkAnchor, float InRunAnchor, float SprintAnchor, const FEclipseLocomotionBlend& Blend)
{
	const float GaitWeight = Blend.WalkWeight + Blend.RunWeight;
	if (GaitWeight <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	float WalkAnchor = 0.0f;
	float RunAnchor = 0.0f;
	OrderAnchors(InWalkAnchor, InRunAnchor, WalkAnchor, RunAnchor);

	// The reference speed is the one the CURRENT blend is authored for, so the
	// rate stays at 1 through the whole ramp instead of spiking at each
	// cross-fade boundary.
	const float Reference = (Blend.WalkWeight * WalkAnchor + Blend.RunWeight * RunAnchor) / GaitWeight;

	// The ceiling is SprintSpeed's job: none of the packs DT_BodyDefs points at
	// carries a sprint take, so sprint has to read as "the run cycle, driven at
	// the sprint/run ratio". That keeps the fastest gait a tuning number instead
	// of a constant, while MaxStrideRate stays as the absolute guard rail
	// against a data mistake asking for a cartoon.
	const float SprintCeiling = FMath::Clamp(FMath::Max(SprintAnchor, RunAnchor) / RunAnchor, 1.0f, MaxStrideRate);

	return FMath::Clamp(FMath::Max(GroundSpeed, 0.0f) / Reference, MinStrideRate, SprintCeiling);
}

FEclipseLocomotionBlend EclipseLocomotion::ComputeBlend(float GroundSpeed, const FEclipseLocomotionSet& Set)
{
	return ComputeBlend(GroundSpeed, Set.WalkSpeed, Set.RunSpeed, Set.Walk != nullptr, Set.Run != nullptr);
}

float EclipseLocomotion::ComputeStrideRate(float GroundSpeed, const FEclipseLocomotionSet& Set, const FEclipseLocomotionBlend& Blend)
{
	return ComputeStrideRate(GroundSpeed, Set.WalkSpeed, Set.RunSpeed, Set.SprintSpeed, Blend);
}

EEclipseLocomotionTier FEclipseLocomotionSet::GetTier() const
{
	return EclipseLocomotion::ClassifyTier(Idle != nullptr, Walk != nullptr, Run != nullptr);
}

// ---------------------------------------------------------------------------
// Proxy (worker thread)
// ---------------------------------------------------------------------------

void FEclipseLocomotionProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);

	IdleTime = 0.0f;
	GaitPhase = 0.0f;
	DeathTime = 0.0f;
	Blend = FEclipseLocomotionBlend();
	StrideRate = 1.0f;
}

void FEclipseLocomotionProxy::InitializeObjects(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::InitializeObjects(InAnimInstance);

	if (const UEclipseAnimInstance* Instance = Cast<UEclipseAnimInstance>(InAnimInstance))
	{
		const FEclipseLocomotionSet& Set = Instance->GetLocomotionSet();
		Idle = Set.Idle;
		Walk = Set.Walk;
		Run = Set.Run;
		Death = Set.Death;
		WalkBack = Set.WalkBack;
		WalkLeft = Set.WalkLeft;
		WalkRight = Set.WalkRight;
		RunBack = Set.RunBack;
		RunLeft = Set.RunLeft;
		RunRight = Set.RunRight;
	}
}

void FEclipseLocomotionProxy::ClearObjects()
{
	FAnimInstanceProxy::ClearObjects();

	Idle = nullptr;
	Walk = nullptr;
	Run = nullptr;
	Death = nullptr;
	WalkBack = nullptr;
	WalkLeft = nullptr;
	WalkRight = nullptr;
	RunBack = nullptr;
	RunLeft = nullptr;
	RunRight = nullptr;
}

void FEclipseLocomotionProxy::SetLocomotionState(const FEclipseLocomotionBlend& InBlend, float InStrideRate, bool bInIsInAir, bool bInIsDowned,
	float InMoveDirectionDegrees)
{
	Blend = InBlend;
	StrideRate = InStrideRate;
	bIsInAir = bInIsInAir;
	bIsDowned = bInIsDowned;
	MoveDirectionDegrees = InMoveDirectionDegrees;
}

float FEclipseLocomotionProxy::GaitCycleLength() const
{
	const float WalkWeight = Walk != nullptr ? Blend.WalkWeight : 0.0f;
	const float RunWeight = Run != nullptr ? Blend.RunWeight : 0.0f;
	const float Total = WalkWeight + RunWeight;
	if (Total <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}
	const float WalkLength = Walk != nullptr ? Walk->GetPlayLength() : 0.0f;
	const float RunLength = Run != nullptr ? Run->GetPlayLength() : 0.0f;
	return (WalkWeight * WalkLength + RunWeight * RunLength) / Total;
}

void FEclipseLocomotionProxy::Update(float DeltaSeconds)
{
	FAnimInstanceProxy::Update(DeltaSeconds);

	// A hitch (level stream, shader compile) must cost one frame of animation,
	// not teleport the cycle to a random contact frame.
	const float Delta = FMath::Clamp(DeltaSeconds, 0.0f, 0.25f);

	if (bIsDowned && Death != nullptr)
	{
		// One-shot, held on its last frame. Clamping rather than looping is the
		// whole point: a corpse that replays its own death is comedy.
		const float Length = Death->GetPlayLength();
		DeathTime = FMath::Min(DeathTime + Delta, FMath::Max(Length - KINDA_SMALL_NUMBER, 0.0f));
		return;
	}
	DeathTime = 0.0f;

	if (Idle != nullptr)
	{
		IdleTime = AdvanceLooping(IdleTime, Delta, Idle->GetPlayLength());
	}

	// Airborne: freeze the gait clock instead of cycling feet through empty air.
	// There is no jump take wired in DT_BodyDefs, so the honest debug-grade
	// answer is "hold the pose you had", not "invent one".
	if (!bIsInAir)
	{
		const float CycleLength = GaitCycleLength();
		if (CycleLength > KINDA_SMALL_NUMBER)
		{
			GaitPhase = AdvanceLooping(GaitPhase, (Delta * StrideRate) / CycleLength, 1.0f);
		}
	}
}

bool FEclipseLocomotionProxy::Evaluate(FPoseContext& Output)
{
	Output.ResetToRefPose();

	// Death wins outright: a downed body is not walking. This is also the first
	// place DT_BodyDefs' DeathAnim column has ever actually been played — the
	// single-node tier only ever played the idle.
	if (bIsDowned && Death != nullptr)
	{
		FAnimationPoseData DeathPoseData(Output);
		const FEclipseLocomotionSample DeathSample{ Death, 1.0f, DeathTime, /*bLooping*/ false };
		SampleInto(DeathSample, DeathPoseData);
		return true;
	}

	// ComputeBlend never returns more than two non-zero weights, so this is a
	// single blend in practice; the accumulator below stays general anyway so a
	// future third pose cannot silently be dropped on the floor.
	TArray<FEclipseLocomotionSample, TInlineAllocator<3>> Samples;
	if (Idle != nullptr && FAnimWeight::IsRelevant(Blend.IdleWeight))
	{
		Samples.Add({ Idle, Blend.IdleWeight, IdleTime, true });
	}
	// Welke van de vier richtingen? De klippen staan 90 graden uit elkaar, dus de
	// dichtstbijzijnde kiezen is precies wat je bij dat interval ziet. Ontbreekt
	// die richting, dan valt hij terug op vooruit — moonwalken, maar zichtbaar
	// beter dan de ref-pose (14.3.5).
	auto PickDirectional = [this](UAnimSequence* Fwd, UAnimSequence* Back, UAnimSequence* Left, UAnimSequence* Right) -> UAnimSequence*
	{
		const float Abs = FMath::Abs(MoveDirectionDegrees);
		UAnimSequence* Chosen = Fwd;
		if (Abs > 135.0f)
		{
			Chosen = Back;
		}
		else if (Abs > 45.0f)
		{
			Chosen = MoveDirectionDegrees > 0.0f ? Right : Left;
		}
		return Chosen != nullptr ? Chosen : Fwd;
	};

	UAnimSequence* const WalkClip = PickDirectional(Walk, WalkBack, WalkLeft, WalkRight);
	UAnimSequence* const RunClip = PickDirectional(Run, RunBack, RunLeft, RunRight);

	if (WalkClip != nullptr && FAnimWeight::IsRelevant(Blend.WalkWeight))
	{
		Samples.Add({ WalkClip, Blend.WalkWeight, GaitPhase * WalkClip->GetPlayLength(), true });
	}
	if (RunClip != nullptr && FAnimWeight::IsRelevant(Blend.RunWeight))
	{
		Samples.Add({ RunClip, Blend.RunWeight, GaitPhase * RunClip->GetPlayLength(), true });
	}

	if (Samples.Num() == 0)
	{
		// Ref pose. Not a failure path worth logging per frame — ApplyBodyDef
		// already said, once, which clip was missing and why (GDD 14.3.5).
		return true;
	}

	FAnimationPoseData OutputPoseData(Output);
	SampleInto(Samples[0], OutputPoseData);
	float AccumulatedWeight = Samples[0].Weight;

	for (int32 Index = 1; Index < Samples.Num(); ++Index)
	{
		FCompactPose ScratchPose;
		ScratchPose.SetBoneContainer(&Output.Pose.GetBoneContainer());
		FBlendedCurve ScratchCurve;
		ScratchCurve.InitFrom(Output.Curve);
		UE::Anim::FStackAttributeContainer ScratchAttributes;
		FAnimationPoseData ScratchPoseData = { ScratchPose, ScratchCurve, ScratchAttributes };

		SampleInto(Samples[Index], ScratchPoseData);

		const float NextWeight = AccumulatedWeight + Samples[Index].Weight;
		if (NextWeight <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		FAnimationRuntime::BlendTwoPosesTogetherInPlace(OutputPoseData, ScratchPoseData, AccumulatedWeight / NextWeight);
		AccumulatedWeight = NextWeight;
	}

	Output.Pose.NormalizeRotations();
	return true;
}

// ---------------------------------------------------------------------------
// Anim instance (game thread)
// ---------------------------------------------------------------------------

FAnimInstanceProxy* UEclipseAnimInstance::CreateAnimInstanceProxy()
{
	return new FEclipseLocomotionProxy(this);
}

void UEclipseAnimInstance::SetLocomotionSet(const FEclipseLocomotionSet& InSet)
{
	LocomotionSet = InSet;
	ApplyTuningAnchors();
}

void UEclipseAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwningBody = Cast<AEclipseCharacter>(TryGetPawnOwner());
	if (OwningBody != nullptr)
	{
		// Pull rather than rely on the push: the mesh re-initializes its anim
		// instance on every SetSkeletalMesh and on LOD changes, so a body that
		// was dressed once must still find its clips on the second instance.
		LocomotionSet = OwningBody->GetLocomotionSet();
	}
	ApplyTuningAnchors();
}

void UEclipseAnimInstance::ApplyTuningAnchors()
{
	// Same route the controller uses for its look/sprint numbers: the anchors are
	// DA_CharacterTuning's, not this file's (GDD 14.2). The struct defaults
	// already mirror the locked feel targets, so a campaign without a tuning
	// asset blends on the right numbers rather than on zero — say so at Log
	// rather than Warning, because this fires once per body and a full mission
	// would otherwise print it eleven times.
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	const UEclipseCampaignSubsystem* Campaign = GameInstance != nullptr
		? GameInstance->GetSubsystem<UEclipseCampaignSubsystem>() : nullptr;
	const UEclipseCampaignSetupAsset* Setup = Campaign != nullptr ? Campaign->GetActiveSetup() : nullptr;
	const UEclipseCharacterTuningAsset* Tuning = Setup != nullptr ? Setup->CharacterTuning.LoadSynchronous() : nullptr;
	if (Tuning == nullptr)
	{
		UE_LOG(LogEclipse, Log, TEXT("%s: no DA_CharacterTuning on the active setup — locomotion blends on the locked feel targets (GDD 14.3.5)."),
			*GetName());
		return;
	}

	LocomotionSet.WalkSpeed = Tuning->WalkSpeed;
	LocomotionSet.RunSpeed = Tuning->RunSpeed;
	LocomotionSet.SprintSpeed = Tuning->SprintSpeed;
}

void UEclipseAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// Everything below is a handful of reads and a dozen multiplies. That budget
	// is the point (GDD 12.4): this runs for the player, four squadmates and
	// every enemy on screen, every frame.
	if (const AEclipseCharacter* Body = OwningBody.Get())
	{
		FVector Velocity = Body->GetVelocity();
		// Falling is not walking: a body dropping off a ledge at 900 cm/s must not
		// break into a sprint cycle on the way down.
		Velocity.Z = 0.0f;
		GroundSpeed = Velocity.Size();

		const UCharacterMovementComponent* Movement = Body->GetCharacterMovement();
		bIsInAir = Movement != nullptr && Movement->IsFalling();
		bIsDowned = Body->IsDowned();

		// De hoek tussen waar hij LOOPT en waar hij KIJKT (26-07, punt 8). Vóór het
		// camera-relatieve model was die per definitie 0 — het lichaam draaide naar
		// zijn eigen snelheid — en daarom bestond deze regel niet. Nu blijft hij
		// naar de camera kijken, dus achteruit duwen geeft ±180 en dat moet een
		// andere cyclus opleveren dan vooruit.
		//
		// Onder de sta-drempel bevriezen we de hoek op 0: bij bijna-nul snelheid
		// is de richting ruis, en die zou de cyclus laten flikkeren tussen links
		// en rechts op het moment dat je stopt.
		MoveDirectionDegrees = GroundSpeed > EclipseLocomotion::StandingSpeedThreshold
			? static_cast<float>(FMath::UnwindDegrees(Velocity.Rotation().Yaw - Body->GetActorRotation().Yaw))
			: 0.0f;
	}
	else
	{
		GroundSpeed = 0.0f;
		bIsInAir = false;
		bIsDowned = false;
		MoveDirectionDegrees = 0.0f;
	}

	const FEclipseLocomotionBlend NewBlend = EclipseLocomotion::ComputeBlend(GroundSpeed, LocomotionSet);
	IdleWeight = NewBlend.IdleWeight;
	WalkWeight = NewBlend.WalkWeight;
	RunWeight = NewBlend.RunWeight;
	StrideRate = EclipseLocomotion::ComputeStrideRate(GroundSpeed, LocomotionSet, NewBlend);

	// Push on the game thread, before the parallel update reads it. PreUpdate
	// runs BEFORE this function, so copying state there would evaluate every
	// frame against the previous frame's speed.
	GetProxyOnGameThread<FEclipseLocomotionProxy>().SetLocomotionState(NewBlend, StrideRate, bIsInAir, bIsDowned, MoveDirectionDegrees);
}
