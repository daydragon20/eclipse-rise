#include "Characters/EclipseCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/EclipseHealthAttributeSet.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Eclipse.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NavigationInvokerComponent.h"

namespace
{
	/** Best-guess base-color texture of a pack material (restyle detail source). */
	UTexture* FindBaseColorTexture(UMaterialInterface* Material)
	{
		if (Material == nullptr)
		{
			return nullptr;
		}
		TArray<UTexture*> Textures;
		Material->GetUsedTextures(Textures, EMaterialQualityLevel::High, true, ERHIFeatureLevel::SM6, true);
		UTexture* Fallback = nullptr;
		for (UTexture* Texture : Textures)
		{
			const FString TexName = Texture->GetName();
			if (TexName.Contains(TEXT("Diff")) || TexName.Contains(TEXT("Base")) || TexName.Contains(TEXT("Alb"))
				|| TexName.Contains(TEXT("_BC")) || TexName.Contains(TEXT("Color")) || TexName.EndsWith(TEXT("_D")))
			{
				return Texture;
			}
			if (Fallback == nullptr)
			{
				Fallback = Texture;
			}
		}
		return Fallback;
	}
}

AEclipseCharacter::AEclipseCharacter()
{
	// Event-driven by default (GDD 14.2): nothing here needs per-frame work.
	PrimaryActorTick.bCanEverTick = false;

	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	HealthAttributes = CreateDefaultSubobject<UEclipseHealthAttributeSet>(TEXT("HealthAttributes"));

	// Navmesh generates around characters (invoker model): the graybox level
	// needs no authored bounds volume, and big districts never pay for navmesh
	// where nobody walks (GDD 12.4 budget thinking).
	CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));

	// Third-person camera (owner playtest 2026-07-25). There was no camera layer at
	// all before this: no spring arm, no camera component, so the engine fell back
	// to the pawn's own view point and the player looked out of the body. For a
	// third-person action game (GDD 01 / 15.1) that is a hole, and it blocked the
	// R3 verdict outright — targeting readability and comfort cannot be judged
	// through a camera that does not exist.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetCapsuleComponent());
	CameraBoom->TargetArmLength = ThirdPersonArmLength;
	CameraBoom->SocketOffset = FVector(0.0f, 55.0f, 65.0f);
	// The boom follows where the player LOOKS; the camera then just rides its end.
	// Both true would double-apply control rotation and the view would swing twice
	// as fast as the stick.
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 12.0f;

	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	ViewCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ViewCamera->bUsePawnControlRotation = false;
	ViewCamera->SetFieldOfView(ThirdPersonFOV);

	// The body must NOT turn with the camera, or walking reads as skating: the
	// character faces where it moves, the camera looks where the player aims.
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	// Crouch is a designed control, not an engine extra (GDD 04_core_gameplay:
	// "Crouch = stealth default"), and ApplyTuning already feeds it a speed from
	// DA_CharacterTuning. But FNavAgentProperties::bCanCrouch defaults to FALSE,
	// so ACharacter::Crouch() silently did nothing and Ctrl / pad-B was a dead key
	// — the tuning knob was being applied to a capability that was switched off.
	// Found by the in-game test guide while writing the crouch step's expectation,
	// which is exactly what that guide exists for.
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
}

void AEclipseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	AbilitySystem->InitAbilityActorInfo(this, this);
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UEclipseHealthAttributeSet::GetHealthAttribute())
		.AddUObject(this, &AEclipseCharacter::HandleHealthChanged);
}

void AEclipseCharacter::ApplyTuning(const UEclipseCharacterTuningAsset* Tuning)
{
	if (Tuning == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("%s: no character tuning asset — engine defaults stay (GDD 14.3.5)."), *GetName());
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed = Tuning->RunSpeed;
	GetCharacterMovement()->MaxWalkSpeedCrouched = Tuning->CrouchSpeed;
	InitializeHealth(Tuning->MaxHealth);

	// Camera framing is data, like every other feel number (GDD 14.2). The
	// constructor's values are the same defaults, so an untuned pawn still frames
	// correctly rather than collapsing to a zero-length boom.
	ThirdPersonArmLength = Tuning->ThirdPersonArmLength;
	CommandModeArmLength = Tuning->CommandModeArmLength;
	CommandModeCameraRise = Tuning->CommandModeCameraRise;
	BaseSocketOffsetZ = Tuning->CameraSocketOffset.Z;
	FirstPersonFOV = Tuning->CameraFOV;
	ThirdPersonFOV = Tuning->ThirdPersonFOV;
	ViewToggleBlendTime = Tuning->ViewToggleBlendTime;
	if (CameraBoom != nullptr)
	{
		CameraBoom->SocketOffset = Tuning->CameraSocketOffset;
		CameraBoom->CameraLagSpeed = Tuning->CameraLagSpeed;
		CameraBoom->bEnableCameraLag = Tuning->CameraLagSpeed > 0.0f;
		CameraBoom->ProbeSize = Tuning->CameraProbeSize;
	}
	RefreshCameraTargets();
	// Land on the target immediately here: tuning arrives at spawn, and blending
	// in from the constructor defaults would show as a lurch on the first frame.
	if (CameraBoom != nullptr)
	{
		CameraBoom->TargetArmLength = TargetArmLength;
	}
	if (ViewCamera != nullptr)
	{
		ViewCamera->SetFieldOfView(TargetFOV);
	}
}

void AEclipseCharacter::SetFirstPerson(bool bNewFirstPerson)
{
	if (bFirstPerson == bNewFirstPerson)
	{
		return;
	}
	bFirstPerson = bNewFirstPerson;
	// Hide the own body in first person, or the view sits inside the skull. The
	// owner-only flag keeps the mesh visible to everyone else and in shadows.
	if (USkeletalMeshComponent* Body = GetMesh())
	{
		Body->SetOwnerNoSee(bFirstPerson);
	}
	RefreshCameraTargets();
}

void AEclipseCharacter::SetCommandModeCamera(bool bActive)
{
	if (bCommandModeCamera == bActive)
	{
		return;
	}
	bCommandModeCamera = bActive;
	RefreshCameraTargets();
}

void AEclipseCharacter::SetAiming(bool bNewAiming)
{
	if (bAiming == bNewAiming)
	{
		return;
	}
	bAiming = bNewAiming;
	RefreshCameraTargets();
}

void AEclipseCharacter::RefreshCameraTargets()
{
	// Command Mode wins over the view toggle: pulling back to read the field is
	// the whole point of the hold, and doing that from inside the head is not a
	// reading position.
	if (bCommandModeCamera)
	{
		TargetArmLength = CommandModeArmLength;
		TargetFOV = ThirdPersonFOV;
	}
	else
	{
		TargetArmLength = bFirstPerson ? 0.0f : ThirdPersonArmLength;
		TargetFOV = bFirstPerson ? FirstPersonFOV : ThirdPersonFOV;
		if (bAiming)
		{
			// Aiming pulls in and narrows: 0.55 of the boom and 0.8 of the FOV.
			// Relative to whichever view you are in, so ADS works the same in
			// first and third person instead of needing two more constants.
			TargetArmLength *= 0.55f;
			TargetFOV *= 0.80f;
		}
	}

	const bool bNeedsBlend =
		(CameraBoom != nullptr && !FMath::IsNearlyEqual(CameraBoom->TargetArmLength, TargetArmLength, 0.5f))
		|| (ViewCamera != nullptr && !FMath::IsNearlyEqual(ViewCamera->FieldOfView, TargetFOV, 0.1f));
	// Tick exists only for the duration of a blend (GDD 12.4). A camera that
	// costs a tick every frame forever, to move twice a minute, is not free at
	// four squadmates plus enemies sharing this class.
	SetActorTickEnabled(bNeedsBlend);
}

void AEclipseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCameraBlend(DeltaSeconds);
}

void AEclipseCharacter::UpdateCameraBlend(float DeltaSeconds)
{
	if (CameraBoom == nullptr || ViewCamera == nullptr)
	{
		SetActorTickEnabled(false);
		return;
	}

	// Constant-rate blend over ViewToggleBlendTime rather than an exponential
	// ease: the toggle must always take the same time, so the player learns it.
	const float ArmRate = ViewToggleBlendTime > KINDA_SMALL_NUMBER
		? FMath::Max(ThirdPersonArmLength, CommandModeArmLength) / ViewToggleBlendTime
		: BIG_NUMBER;
	const float FovRate = ViewToggleBlendTime > KINDA_SMALL_NUMBER
		? FMath::Abs(FirstPersonFOV - ThirdPersonFOV) / ViewToggleBlendTime
		: BIG_NUMBER;

	CameraBoom->TargetArmLength = FMath::FInterpConstantTo(
		CameraBoom->TargetArmLength, TargetArmLength, DeltaSeconds, ArmRate);
	ViewCamera->SetFieldOfView(FMath::FInterpConstantTo(
		ViewCamera->FieldOfView, TargetFOV, DeltaSeconds, FMath::Max(FovRate, 1.0f)));

	// Command Mode also raises the eye line. Height, not pitch: the boom runs on
	// pawn control rotation, so pitch is the player's stick — a camera that tilts
	// itself mid-aim fights the person holding the controller.
	const float TargetRise = bCommandModeCamera ? CommandModeCameraRise : 0.0f;
	const float RiseRate = ViewToggleBlendTime > KINDA_SMALL_NUMBER
		? FMath::Max(CommandModeCameraRise, 1.0f) / ViewToggleBlendTime : BIG_NUMBER;
	FVector Offset = CameraBoom->SocketOffset;
	Offset.Z = FMath::FInterpConstantTo(Offset.Z, BaseSocketOffsetZ + TargetRise, DeltaSeconds, RiseRate);
	CameraBoom->SocketOffset = Offset;

	if (FMath::IsNearlyEqual(CameraBoom->TargetArmLength, TargetArmLength, 0.5f)
		&& FMath::IsNearlyEqual(ViewCamera->FieldOfView, TargetFOV, 0.1f)
		&& FMath::IsNearlyEqual(Offset.Z, BaseSocketOffsetZ + TargetRise, 0.5f))
	{
		SetActorTickEnabled(false);
	}
}

void AEclipseCharacter::ApplyBodyDef(const FEclipseBodyDefRow& BodyDef)
{
	USkeletalMesh* BodyMesh = BodyDef.Mesh.LoadSynchronous();
	if (BodyMesh == nullptr)
	{
		UE_LOG(LogEclipse, Warning, TEXT("%s: body mesh %s missing — capsule body stands in (GDD 14.3.5)."),
			*GetName(), *BodyDef.Mesh.ToString());
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	MeshComponent->SetSkeletalMesh(BodyMesh);
	MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, BodyDef.MeshZOffset));
	MeshComponent->SetRelativeRotation(FRotator(0.0f, BodyDef.MeshYaw, 0.0f));
	MeshComponent->SetRelativeScale3D(FVector(BodyDef.MeshScale));

	if (UAnimSequence* Idle = BodyDef.IdleAnim.LoadSynchronous())
	{
		MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MeshComponent->PlayAnimation(Idle, /*bLooping*/ true);
	}

	// Toon restyle (15.5 asset policy): every slot re-dressed with the cel
	// master; the pack's own base texture stays as luminance detail, the
	// faction palette supplies the hue. This also puts bodies on the same
	// exposure tier as the unlit district — lit PBR underexposes to a
	// silhouette against the emissive world (step-2 QC forensics).
	if (BodyDef.bToonRestyle)
	{
		UMaterialInterface* ToonMaster = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/M_EclipseToon.M_EclipseToon"));
		if (ToonMaster == nullptr)
		{
			UE_LOG(LogEclipse, Warning, TEXT("%s: M_EclipseToon missing — pack materials stay (GDD 14.3.5)."), *GetName());
			return;
		}
		// Keep in sync with EclipseGrayboxBuilder's SunRotation (-25, 55): the
		// cel bands must tell the same sun story as the district around them.
		const FVector SunTravel = FRotator(-25.0f, 55.0f, 0.0f).Vector();
		for (int32 SlotIndex = 0; SlotIndex < MeshComponent->GetNumMaterials(); ++SlotIndex)
		{
			UTexture* BaseTexture = FindBaseColorTexture(MeshComponent->GetMaterial(SlotIndex));
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(ToonMaster, this);
			Mid->SetVectorParameterValue(TEXT("LitColor"), BodyDef.TintLit);
			Mid->SetVectorParameterValue(TEXT("ShadeColor"), BodyDef.TintShade);
			Mid->SetVectorParameterValue(TEXT("LightDir"), FLinearColor(FVector4(SunTravel, 0.0f)));
			Mid->SetScalarParameterValue(TEXT("EmissiveScale"), 10.0f);
			Mid->SetScalarParameterValue(TEXT("UVMode"), 1.0f);
			if (BaseTexture != nullptr)
			{
				Mid->SetTextureParameterValue(TEXT("AlbedoTex"), BaseTexture);
				// Generic gain: character textures hover ~0.3 linear; bodies are
				// small on screen, so the unmeasured normalization is acceptable
				// at this tier (the measured discipline applies to big surfaces).
				Mid->SetScalarParameterValue(TEXT("AlbedoGain"), 3.2f);
				Mid->SetScalarParameterValue(TEXT("AlbedoMix"), 0.9f);
			}
			MeshComponent->SetMaterial(SlotIndex, Mid);
		}
	}
}

void AEclipseCharacter::InitializeHealth(float MaxHealth)
{
	HealthAttributes->SetMaxHealth(MaxHealth);
	HealthAttributes->SetHealth(MaxHealth);
	bDowned = false;
}

void AEclipseCharacter::ReviveForMission()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	InitializeHealth(HealthAttributes->GetMaxHealth());
}

void AEclipseCharacter::ApplyDamage(float Amount, AEclipseCharacter* DamageInstigator, FName Cause)
{
	if (Amount <= 0.0f || bDowned)
	{
		return;
	}

	LastDamageCause = Cause;
	// PLACEHOLDER(GDD 12.1): direct attribute write until damage GameplayEffects
	// land with the ability pass — the IncomingDamage meta attribute is already
	// in place as their landing zone (PostGameplayEffectExecute).
	const float NewHealth = FMath::Max(0.0f, HealthAttributes->GetHealth() - Amount);
	AbilitySystem->SetNumericAttributeBase(UEclipseHealthAttributeSet::GetHealthAttribute(), NewHealth);
}

float AEclipseCharacter::GetHealth() const
{
	return HealthAttributes->GetHealth();
}

void AEclipseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	if (bDowned || Data.NewValue > 0.0f)
	{
		return;
	}

	bDowned = true;
	GetCharacterMovement()->DisableMovement();

	// One fact, broadcast once: squad/mission listeners resolve dead-vs-wounded
	// at debrief (SPEC-P1-07); the body itself only reports.
	OnDowned.Broadcast(this, LastDamageCause.IsNone() ? FName(TEXT("Unknown")) : LastDamageCause);
}
