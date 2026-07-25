// Movement CAPABILITIES the game advertises to the player, as opposed to the
// tuning numbers those capabilities run on. The distinction is the whole point
// of this file: crouch had a tuned speed (DA_CharacterTuning -> CrouchSpeed,
// applied in ApplyTuning) while FNavAgentProperties::bCanCrouch was still at its
// engine default of false, so ACharacter::Crouch() returned without doing
// anything and Ctrl / pad-B was a dead key that both the F2 control table and
// the F3 test guide happily taught. A tuned-but-disabled capability reads as
// "implemented" in every place except the running game, so it gets a test.

#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/EclipseCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEclipseCrouchCapabilityTest,
	"Eclipse.Characters.Movement.CrouchIsActuallyEnabled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEclipseCrouchCapabilityTest::RunTest(const FString& Parameters)
{
	const AEclipseCharacter* Defaults = GetDefault<AEclipseCharacter>();
	if (!TestNotNull(TEXT("AEclipseCharacter CDO resolves"), Defaults))
	{
		return false;
	}

	const UCharacterMovementComponent* Movement = Defaults->GetCharacterMovement();
	if (!TestNotNull(TEXT("character movement component exists on the CDO"), Movement))
	{
		return false;
	}

	// The assert that would have caught the dead key: ACharacter::Crouch() checks
	// this flag first and returns early when it is false, so every crouch input,
	// tuned speed and animation hook downstream is unreachable without it.
	TestTrue(TEXT("bCanCrouch is on, so ACharacter::Crouch() is not a no-op (GDD 04: crouch = stealth default)"),
		Movement->GetNavAgentPropertiesRef().bCanCrouch);

	// Crouching must also be slower than standing, otherwise the stealth default
	// costs the player nothing and the control has no read. The absolute numbers
	// live in DA_CharacterTuning and are applied per character in ApplyTuning;
	// what is pinned here is only that the engine defaults do not already invert
	// the relationship before tuning ever lands.
	TestTrue(TEXT("crouched speed stays under standing speed"),
		Movement->MaxWalkSpeedCrouched < Movement->MaxWalkSpeed);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
