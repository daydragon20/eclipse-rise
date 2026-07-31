#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "EclipseLocomotionTypes.generated.h"

class UAnimSequence;
class USkeleton;

/**
 * How much locomotion a body actually got (GDD 14.3.5 degradation ladder). Every
 * rung is a playable body; only the bottom rung is "the engine draws a ref pose".
 * The ladder exists because DT_BodyDefs points at nine different packs and not
 * one of them carries the same take list — Belica has Jog_Fwd and no walk,
 * SciFiSoldier02 has ThirdPersonWalk/Run, SciFiSoldier has no animations at all.
 * A missing take must cost that body one rung, never the frame.
 */
enum class EEclipseLocomotionTier : uint8
{
	/** No idle on this skeleton: nothing to stand in, the mesh draws its ref pose. */
	Unavailable = 0,
	/** Idle only — the pre-2026-07-25 behavior: the body stands and slides when it moves. */
	IdleOnly = 1,
	/** Idle + one gait: speed cross-fades idle into that gait. */
	OneGait = 2,
	/** Idle + walk + run: the full minimal locomotion (SPEC-P2-01 debug tier). */
	TwoGaits = 3,
};

/**
 * The clips one body plays, already resolved AND already checked against that
 * body's skeleton, plus the speed anchors the blend runs on.
 *
 * "Already checked" is the load-bearing part: these packs share a skeleton
 * FAMILY, not a skeleton asset, so BlueSoldier_Male's Walk is a different
 * USkeleton from SciFiSoldier02's. Handing a clip to the wrong skeleton does not
 * throw — it silently evaluates to a mangled or empty pose, which is the exact
 * failure mode GDD 14.3.5 forbids. So resolution rejects a mismatch up front and
 * the body drops a rung with a log line instead.
 */
USTRUCT()
struct FEclipseLocomotionSet
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> Idle = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> Walk = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> Run = nullptr;

	/**
	 * Richtingsvarianten (owner-opdracht 26-07, punt 8). Null = die richting valt
	 * terug op de vooruit-cyclus; dat leest als moonwalken maar is beter dan de
	 * ref-pose (14.3.5). Vier richtingen en geen echte 2D-blendspace: de klippen
	 * staan 90 graden uit elkaar, en de dichtstbijzijnde kiezen is precies wat je
	 * bij dat interval ziet — een blend tussen twee zij-cycli levert bij 45 graden
	 * geen betere pose op, alleen meer rekentijd en meer manieren om het fout te
	 * doen. Als de packs ooit diagonalen krijgen, is dit de plek om dat uit te
	 * breiden.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> WalkBack = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> WalkLeft = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> WalkRight = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> RunBack = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> RunLeft = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> RunRight = nullptr;

	/** One-shot; holds its last frame. Not part of the locomotion ladder. */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> Death = nullptr;

	/** Eenmalige overlays (26-07, punten 2 en 3): vuren en geraakt worden. */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> Shoot = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> HitReact = nullptr;

	/** Herladen (26-07 avond). Lag in SciFiCharacter/Animations en werd nooit gespeeld. */
	UPROPERTY()
	TObjectPtr<UAnimSequence> Reload = nullptr;

	/**
	 * Draaien op de plek (26-07 avond, punt 7). ParagonLtBelica levert
	 * Idle_Turn_90_Left/Right; ik verklaarde ze 's ochtends ten onrechte
	 * onbestaand door in de verkeerde pack te kijken.
	 */
	UPROPERTY()
	TObjectPtr<UAnimSequence> TurnLeft = nullptr;

	UPROPERTY()
	TObjectPtr<UAnimSequence> TurnRight = nullptr;

	/** Hurken/opstaan (26-07, audit punt 13). */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CrouchTransition = nullptr;

	// Blend anchors, mirrored from DA_CharacterTuning (GDD 14.2 — feel numbers
	// are data). The defaults here are the LOCKED feel targets
	// (phase0/graybox_feel_targets.md §2) so a body that never sees a tuning
	// asset still blends on the right numbers instead of on zero.
	UPROPERTY(Transient)
	float WalkSpeed = 180.0f;

	UPROPERTY(Transient)
	float RunSpeed = 420.0f;

	UPROPERTY(Transient)
	float SprintSpeed = 650.0f;

	ECLIPSE_API EEclipseLocomotionTier GetTier() const;
};

/**
 * Where one frame's pose comes from: three weights that always sum to 1. Not a
 * USTRUCT on purpose — it is a pure computation result, never serialized and
 * never edited, and the automation tests build it by the hundred.
 */
struct FEclipseLocomotionBlend
{
	float IdleWeight = 1.0f;
	float WalkWeight = 0.0f;
	float RunWeight = 0.0f;
};

/**
 * The pure half of the locomotion layer. Everything here is a function of its
 * arguments — no world, no assets, no time — which is what lets the headless
 * tests pin the speed->pose contract without an editor, a mesh or a skeleton
 * (EclipseAnimLocomotionTests.cpp). Implementations live in EclipseAnimInstance.cpp.
 */
namespace EclipseLocomotion
{
	/** Under this ground speed the body is standing. Sub-centimetre drift (a
	 *  capsule settling, a push from a squadmate) must not flicker into a walk. */
	inline constexpr float StandingSpeedThreshold = 5.0f;

	/** How far the gait clock may be stretched to chase the ground speed. Bounded
	 *  because an unbounded rate is how a sprint reads as a cartoon and a nudged
	 *  body reads as a slideshow. */
	inline constexpr float MinStrideRate = 0.6f;
	inline constexpr float MaxStrideRate = 1.8f;

	/**
	 * Looprichting (graden, 0 = vooruit, + = rechts) -> aandeel per richtingsklip.
	 *
	 * DIT IS DE FORTNITE/BORDERLANDS-KANT VAN HET ONDERLICHAAM. Er stond een harde
	 * keuze: onder 45 graden vooruit, daarboven opzij. Daardoor KLAPTE de gangpose om
	 * op het moment dat je koers die grens passeerde. In beide referenties lopen
	 * naburige richtingen in elkaar over, en dat is wat deze verdeling doet.
	 *
	 * De vier klippen staan 90 graden uit elkaar, dus een driehoekige kern van 90
	 * graden breed geeft: op een as telt één klip volledig, ertussenin verdelen de
	 * twee buren zich, en de som is altijd 1. Vrij van assets — dezelfde vier takes.
	 */
	ECLIPSE_API void DirectionalWeights(float Degrees, float& OutFwd, float& OutRight,
		float& OutLeft, float& OutBack);

	/** Which rung of the ladder a set of resolved clips lands on. */
	ECLIPSE_API EEclipseLocomotionTier ClassifyTier(bool bHasIdle, bool bHasWalk, bool bHasRun);

	/**
	 * A clip may only play on the exact skeleton it was authored against. Same
	 * family is not the same asset, and "close enough" is what produces a body
	 * folded inside out (see FEclipseLocomotionSet's note).
	 */
	ECLIPSE_API bool IsUsableOn(const USkeleton* ClipSkeleton, const USkeleton* MeshSkeleton);

	/**
	 * Ground speed -> pose weights. Idle cross-fades into walk up to WalkAnchor,
	 * walk cross-fades into run up to RunAnchor, and everything above RunAnchor
	 * (sprint) is full run played faster — see ComputeStrideRate. At most two
	 * weights are ever non-zero, which is what keeps the evaluate path to one
	 * blend.
	 *
	 * Takes anchors and two flags rather than the set, so the whole contract can
	 * be pinned headlessly: constructing a UAnimSequence just to say "this body
	 * owns a walk" would drag an editor-only data model into a maths test.
	 */
	ECLIPSE_API FEclipseLocomotionBlend ComputeBlend(float GroundSpeed, float WalkAnchor, float RunAnchor, bool bHasWalk, bool bHasRun);

	/**
	 * Play-rate multiplier for the gait clock: the clips were authored at some
	 * speed, the body is moving at another, and the gap between the two is what
	 * the eye reads as skating. This does not remove foot sliding — that needs
	 * distance matching or IK — it just stops the legs from being obviously out
	 * of step at the tuned speeds. Returns 1 while standing.
	 */
	ECLIPSE_API float ComputeStrideRate(float GroundSpeed, float WalkAnchor, float RunAnchor, float SprintAnchor, const FEclipseLocomotionBlend& Blend);

	/** Set-shaped convenience wrappers — the runtime call sites use these. */
	ECLIPSE_API FEclipseLocomotionBlend ComputeBlend(float GroundSpeed, const FEclipseLocomotionSet& Set);
	ECLIPSE_API float ComputeStrideRate(float GroundSpeed, const FEclipseLocomotionSet& Set, const FEclipseLocomotionBlend& Blend);

	/**
	 * Het gewichtsverloop van ÉÉN eenmalige pose (schot, klap, herladen, draaien):
	 * verstreken tijd -> hoeveel die pose nu meetelt. Sin-envelope, dus 0 aan het
	 * begin, de piek halverwege, en 0 zodra de duur om is.
	 *
	 * WAAROM DIT HIER STAAT EN NIET TWEE KEER IN DE .CPP. Deze formule stond
	 * letterlijk twee keer: één keer in FEclipseLocomotionProxy::Evaluate (de pose
	 * die de speler ziet) en één keer in UEclipseAnimInstance::NativeUpdateAnimation
	 * (de spiegel die de testlaag uitleest, omdat de proxy op de werkthread leeft en
	 * daar uitlezen een datarace is). Twee kopieën van dezelfde curve betekent dat
	 * een meting aan de ene niets bewijst over de andere.
	 *
	 * En meten is precies waarvoor dit hier ligt: het dossier "trillen bij het
	 * schieten" vraagt om het gewicht PER FRAME, en de voorgeschreven route daarvoor
	 * (Rewind Debugger op de AnimBP, DEBUG_DISCIPLINE §4.2 oorzaak 1) bestaat voor de
	 * speler niet — er is geen AnimBP, er is deze proxy. Als functie van zijn
	 * argumenten is de curve wél headless te bemonsteren, zonder editor en zonder
	 * skelet (EclipseAnimOneShotWeightTests.cpp).
	 *
	 * Duur <= 0 of Alpha >= 1 geeft 0: een pose die niet loopt, weegt niets.
	 */
	ECLIPSE_API float OneShotEnvelope(float Elapsed, float Duration, float PeakWeight);
}
