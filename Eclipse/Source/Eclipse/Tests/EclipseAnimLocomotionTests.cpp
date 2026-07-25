// The speed -> pose contract, and the ladder it falls down when an asset is
// missing. Both are pinned here rather than in the running game because the
// defect that produced this layer was invisible in code review and obvious in
// two seconds of play: ApplyBodyDef put every mesh in AnimationSingleNode and
// looped the idle, so player, squad and enemies all slid around the district in
// a standing pose. Nothing asserted otherwise, because nothing asserted at all.
//
// What is pinned is deliberately the PURE half (EclipseLocomotionTypes.h):
// weights, anchors, tiers, skeleton matching. Those are the parts where a wrong
// answer is silent. The pose evaluation on top of them is not testable headless
// — it needs a bone container, a skeleton and real clips — and is honestly
// reviewed by looking at the game, which is exactly what this tier is for
// (GDD 14.5).

#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/Skeleton.h"
#include "Characters/EclipseLocomotionTypes.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	/** The locked feel targets (phase0/graybox_feel_targets.md §2, GDD 4.1.1). */
	constexpr float LockedWalk = 180.0f;
	constexpr float LockedRun = 420.0f;
	constexpr float LockedSprint = 650.0f;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLocomotionBlendTest,
	"Eclipse.Characters.Locomotion.SpeedMapsToBlend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseLocomotionBlendTest::RunTest(const FString& Parameters)
{
	auto BlendAt = [](float Speed)
	{
		return EclipseLocomotion::ComputeBlend(Speed, LockedWalk, LockedRun, /*bHasWalk*/ true, /*bHasRun*/ true);
	};

	// Standing, and standing-with-a-nudge. The deadzone is the difference between
	// a body that stands still and one that twitches every time a squadmate
	// brushes past it.
	FEclipseLocomotionBlend Blend = BlendAt(0.0f);
	TestEqual(TEXT("stopped is pure idle"), Blend.IdleWeight, 1.0f);
	Blend = BlendAt(EclipseLocomotion::StandingSpeedThreshold - 0.5f);
	TestEqual(TEXT("sub-threshold drift is still pure idle"), Blend.IdleWeight, 1.0f);

	// The anchors are the cross-over points, not merely "somewhere on the ramp".
	Blend = BlendAt(LockedWalk);
	TestEqual(TEXT("walk speed is exactly full walk"), Blend.WalkWeight, 1.0f);
	TestEqual(TEXT("walk speed has no idle left"), Blend.IdleWeight, 0.0f);
	TestEqual(TEXT("walk speed has no run yet"), Blend.RunWeight, 0.0f);

	Blend = BlendAt(LockedRun);
	TestEqual(TEXT("run speed is exactly full run"), Blend.RunWeight, 1.0f);
	TestEqual(TEXT("run speed has no walk left"), Blend.WalkWeight, 0.0f);

	// Halfway between the anchors, both halves.
	Blend = BlendAt(LockedWalk * 0.5f);
	TestEqual(TEXT("half walk speed is half idle"), Blend.IdleWeight, 0.5f, 0.001f);
	TestEqual(TEXT("half walk speed is half walk"), Blend.WalkWeight, 0.5f, 0.001f);

	Blend = BlendAt((LockedWalk + LockedRun) * 0.5f);
	TestEqual(TEXT("between the gait anchors is half walk"), Blend.WalkWeight, 0.5f, 0.001f);
	TestEqual(TEXT("between the gait anchors is half run"), Blend.RunWeight, 0.5f, 0.001f);

	// Sprint has no take in any pack DT_BodyDefs points at, so it must saturate
	// at run rather than fall off a cliff into idle or into nothing.
	Blend = BlendAt(LockedSprint);
	TestEqual(TEXT("sprint saturates at full run"), Blend.RunWeight, 1.0f);
	Blend = BlendAt(100000.0f);
	TestEqual(TEXT("an absurd speed still saturates at full run"), Blend.RunWeight, 1.0f);

	// The two invariants the evaluate path depends on: weights are a partition of
	// one (or the pose loses/gains energy), and never more than two are live (or
	// the accumulating blend in FEclipseLocomotionProxy::Evaluate is doing work
	// nobody reviewed).
	float PreviousRunWeight = 0.0f;
	for (float Speed = -50.0f; Speed <= 900.0f; Speed += 7.0f)
	{
		Blend = BlendAt(Speed);
		const float Sum = Blend.IdleWeight + Blend.WalkWeight + Blend.RunWeight;
		if (!TestEqual(*FString::Printf(TEXT("weights sum to 1 at %.0f cm/s"), Speed), Sum, 1.0f, 0.001f))
		{
			return false;
		}

		int32 LiveWeights = 0;
		LiveWeights += Blend.IdleWeight > 0.001f ? 1 : 0;
		LiveWeights += Blend.WalkWeight > 0.001f ? 1 : 0;
		LiveWeights += Blend.RunWeight > 0.001f ? 1 : 0;
		if (!TestTrue(*FString::Printf(TEXT("at most two poses are live at %.0f cm/s"), Speed), LiveWeights <= 2))
		{
			return false;
		}

		if (!TestTrue(*FString::Printf(TEXT("run weight never falls as speed rises (%.0f cm/s)"), Speed),
			Blend.RunWeight >= PreviousRunWeight - 0.001f))
		{
			return false;
		}
		PreviousRunWeight = Blend.RunWeight;
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLocomotionAnchorsAreDataTest,
	"Eclipse.Characters.Locomotion.AnchorsComeFromTuning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseLocomotionAnchorsAreDataTest::RunTest(const FString& Parameters)
{
	// GDD 14.2: feel numbers live in DA_CharacterTuning. A blend that happens to
	// agree with the tuning asset's DEFAULTS looks identical in play to one that
	// hardcodes 180/420 — the only way to tell them apart is to move the numbers
	// and check that the cross-over moves with them.
	FEclipseLocomotionBlend Blend = EclipseLocomotion::ComputeBlend(100.0f, 100.0f, 300.0f, true, true);
	TestEqual(TEXT("the retuned walk anchor is full walk"), Blend.WalkWeight, 1.0f);

	Blend = EclipseLocomotion::ComputeBlend(200.0f, 100.0f, 300.0f, true, true);
	TestEqual(TEXT("the retuned midpoint is half walk"), Blend.WalkWeight, 0.5f, 0.001f);
	TestEqual(TEXT("the retuned midpoint is half run"), Blend.RunWeight, 0.5f, 0.001f);

	Blend = EclipseLocomotion::ComputeBlend(300.0f, 100.0f, 300.0f, true, true);
	TestEqual(TEXT("the retuned run anchor is full run"), Blend.RunWeight, 1.0f);

	// And the same speed against the locked defaults must NOT be full walk, or
	// the assertions above would pass on a hardcoded ramp too.
	Blend = EclipseLocomotion::ComputeBlend(100.0f, LockedWalk, LockedRun, true, true);
	TestTrue(TEXT("100 cm/s is only part-way up the locked ramp"), Blend.WalkWeight < 0.99f);

	// The wrapper must read the anchors off the set and not off a constant, or
	// none of the above reaches the running game.
	FEclipseLocomotionSet Set;
	Set.WalkSpeed = 100.0f;
	Set.RunSpeed = 300.0f;
	// Walk/Run stay null here on purpose: the wrapper's job is to forward the
	// anchors AND the has-clip flags, and a no-gait set must stay on idle.
	Blend = EclipseLocomotion::ComputeBlend(100.0f, Set);
	TestEqual(TEXT("a set with no gait clips stays on idle whatever the anchors"), Blend.IdleWeight, 1.0f);

	// Inverted anchors are a data mistake and must degrade, not divide by zero
	// or invert the ramp (GDD 14.3.5).
	for (float Speed = 0.0f; Speed <= 900.0f; Speed += 25.0f)
	{
		Blend = EclipseLocomotion::ComputeBlend(Speed, /*Walk*/ 500.0f, /*Run*/ 100.0f, true, true);
		const float Sum = Blend.IdleWeight + Blend.WalkWeight + Blend.RunWeight;
		if (!TestEqual(TEXT("inverted anchors still produce a valid partition"), Sum, 1.0f, 0.001f))
		{
			return false;
		}
	}
	// Zero anchors are the other half of the same mistake.
	Blend = EclipseLocomotion::ComputeBlend(300.0f, 0.0f, 0.0f, true, true);
	TestEqual(TEXT("zeroed anchors still produce a valid partition"),
		Blend.IdleWeight + Blend.WalkWeight + Blend.RunWeight, 1.0f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLocomotionDegradationTest,
	"Eclipse.Characters.Locomotion.DegradesInsteadOfCrashing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseLocomotionDegradationTest::RunTest(const FString& Parameters)
{
	// The nine packs behind DT_BodyDefs do not carry the same takes: Belica has
	// Jog_* and no walk, SciFiSoldier ships zero animations, the ThirdPerson demo
	// packs have exactly idle/walk/run. Every one of those shapes has to produce
	// a playable body (GDD 14.3.5).
	TestEqual(TEXT("no idle is the bottom rung"),
		static_cast<int32>(EclipseLocomotion::ClassifyTier(false, true, true)),
		static_cast<int32>(EEclipseLocomotionTier::Unavailable));
	TestEqual(TEXT("idle alone is the old single-node tier"),
		static_cast<int32>(EclipseLocomotion::ClassifyTier(true, false, false)),
		static_cast<int32>(EEclipseLocomotionTier::IdleOnly));
	TestEqual(TEXT("idle + walk is one gait"),
		static_cast<int32>(EclipseLocomotion::ClassifyTier(true, true, false)),
		static_cast<int32>(EEclipseLocomotionTier::OneGait));
	TestEqual(TEXT("idle + run is also one gait"),
		static_cast<int32>(EclipseLocomotion::ClassifyTier(true, false, true)),
		static_cast<int32>(EEclipseLocomotionTier::OneGait));
	TestEqual(TEXT("idle + walk + run is the full tier"),
		static_cast<int32>(EclipseLocomotion::ClassifyTier(true, true, true)),
		static_cast<int32>(EEclipseLocomotionTier::TwoGaits));

	// ApplyBodyDef only builds an anim instance from OneGait upward; below that
	// it keeps the single-node idle. That ordering is what the character's
	// `Tier >= OneGait` test relies on, so pin it here too.
	TestTrue(TEXT("the ladder is ordered, so >= OneGait means 'has a gait'"),
		EEclipseLocomotionTier::Unavailable < EEclipseLocomotionTier::IdleOnly
		&& EEclipseLocomotionTier::IdleOnly < EEclipseLocomotionTier::OneGait
		&& EEclipseLocomotionTier::OneGait < EEclipseLocomotionTier::TwoGaits);

	// Walk-only: the walk has to carry the whole ramp, including sprint, or a
	// running body drops back to standing at exactly the moment it speeds up.
	FEclipseLocomotionBlend Blend = EclipseLocomotion::ComputeBlend(
		LockedSprint, LockedWalk, LockedRun, /*bHasWalk*/ true, /*bHasRun*/ false);
	TestEqual(TEXT("walk-only saturates at full walk when sprinting"), Blend.WalkWeight, 1.0f);
	TestEqual(TEXT("walk-only never invents a run pose"), Blend.RunWeight, 0.0f);

	// Run-only (Belica's shape): the run is anchored at RunSpeed, so half of it
	// is half the ramp.
	Blend = EclipseLocomotion::ComputeBlend(
		LockedRun * 0.5f, LockedWalk, LockedRun, /*bHasWalk*/ false, /*bHasRun*/ true);
	TestEqual(TEXT("run-only blends against the run anchor"), Blend.RunWeight, 0.5f, 0.001f);
	TestEqual(TEXT("run-only never invents a walk pose"), Blend.WalkWeight, 0.0f);

	// No gait at all: the pre-2026-07-25 behavior, which is still a body standing
	// up rather than a crash or a ref pose.
	Blend = EclipseLocomotion::ComputeBlend(
		LockedSprint, LockedWalk, LockedRun, /*bHasWalk*/ false, /*bHasRun*/ false);
	TestEqual(TEXT("idle-only stays on the idle at any speed"), Blend.IdleWeight, 1.0f);

	// The default-constructed set is what a body gets when nothing resolved.
	const FEclipseLocomotionSet Empty;
	TestEqual(TEXT("the empty set is the bottom rung"), static_cast<int32>(Empty.GetTier()),
		static_cast<int32>(EEclipseLocomotionTier::Unavailable));
	TestEqual(TEXT("the empty set keeps the locked walk anchor"), Empty.WalkSpeed, LockedWalk);
	TestEqual(TEXT("the empty set keeps the locked run anchor"), Empty.RunSpeed, LockedRun);
	TestEqual(TEXT("the empty set keeps the locked sprint anchor"), Empty.SprintSpeed, LockedSprint);
	Blend = EclipseLocomotion::ComputeBlend(LockedRun, Empty);
	TestEqual(TEXT("the empty set still returns a valid partition"),
		Blend.IdleWeight + Blend.WalkWeight + Blend.RunWeight, 1.0f, 0.001f);
	TestEqual(TEXT("the empty set has a stride rate of 1"),
		EclipseLocomotion::ComputeStrideRate(LockedRun, Empty, Blend), 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLocomotionSkeletonGateTest,
	"Eclipse.Characters.Locomotion.SkeletonMismatchIsRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseLocomotionSkeletonGateTest::RunTest(const FString& Parameters)
{
	// This gate is the reason the layer resolves clips per body instead of once.
	// The packs share the UE4-Mannequin skeleton FAMILY, so a walk from
	// SciFiSoldier02 looks assignable to BlueSoldier_Male in every tool that
	// shows you a name — and evaluates to a mangled pose, silently, at runtime.
	const TStrongObjectPtr<USkeleton> BodySkeleton(NewObject<USkeleton>(GetTransientPackage()));
	const TStrongObjectPtr<USkeleton> ForeignSkeleton(NewObject<USkeleton>(GetTransientPackage()));

	TestTrue(TEXT("a clip plays on its own skeleton"),
		EclipseLocomotion::IsUsableOn(BodySkeleton.Get(), BodySkeleton.Get()));
	TestFalse(TEXT("a clip from a sibling pack is rejected"),
		EclipseLocomotion::IsUsableOn(ForeignSkeleton.Get(), BodySkeleton.Get()));
	TestFalse(TEXT("a clip with no skeleton is rejected"),
		EclipseLocomotion::IsUsableOn(nullptr, BodySkeleton.Get()));
	TestFalse(TEXT("no mesh skeleton rejects everything"),
		EclipseLocomotion::IsUsableOn(BodySkeleton.Get(), nullptr));
	// The one that would slip through a naive `A == B`: two missing skeletons are
	// equal, and must still not count as a match.
	TestFalse(TEXT("two absent skeletons are not a match"),
		EclipseLocomotion::IsUsableOn(nullptr, nullptr));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseLocomotionStrideRateTest,
	"Eclipse.Characters.Locomotion.StrideRateTracksSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseLocomotionStrideRateTest::RunTest(const FString& Parameters)
{
	auto RateAt = [](float Speed, float Sprint = LockedSprint)
	{
		const FEclipseLocomotionBlend Blend =
			EclipseLocomotion::ComputeBlend(Speed, LockedWalk, LockedRun, true, true);
		return EclipseLocomotion::ComputeStrideRate(Speed, LockedWalk, LockedRun, Sprint, Blend);
	};

	TestEqual(TEXT("standing needs no stride"), RateAt(0.0f), 1.0f);

	// Between the anchors the clips are already authored for the speed the body
	// is doing, so the clock must stay at 1 — any deviation there is the layer
	// fighting the animation instead of driving it.
	TestEqual(TEXT("walk anchor runs the clip at authored speed"), RateAt(LockedWalk), 1.0f, 0.001f);
	TestEqual(TEXT("mid-ramp runs the clip at authored speed"), RateAt((LockedWalk + LockedRun) * 0.5f), 1.0f, 0.001f);
	TestEqual(TEXT("run anchor runs the clip at authored speed"), RateAt(LockedRun), 1.0f, 0.001f);

	// Sprint is the run cycle driven at the sprint/run ratio, because no pack
	// ships a sprint take. That ratio is DATA — it moves with SprintSpeed.
	TestEqual(TEXT("sprint drives the run cycle at the tuned ratio"),
		RateAt(LockedSprint), LockedSprint / LockedRun, 0.001f);
	TestEqual(TEXT("a sprint tuned down to run speed stops stretching the cycle"),
		RateAt(LockedSprint, /*Sprint*/ LockedRun), 1.0f, 0.001f);

	// Guard rails hold on both sides, whatever the speed.
	for (float Speed = 0.0f; Speed <= 5000.0f; Speed += 37.0f)
	{
		const float Rate = RateAt(Speed);
		if (!TestTrue(*FString::Printf(TEXT("stride rate stays bounded at %.0f cm/s"), Speed),
			Rate >= EclipseLocomotion::MinStrideRate - 0.001f && Rate <= EclipseLocomotion::MaxStrideRate + 0.001f))
		{
			return false;
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
