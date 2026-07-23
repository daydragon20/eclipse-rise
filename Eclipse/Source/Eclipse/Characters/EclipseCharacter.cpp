#include "Characters/EclipseCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/EclipseHealthAttributeSet.h"
#include "Animation/AnimSequence.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Eclipse.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationInvokerComponent.h"

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

	GetCharacterMovement()->bOrientRotationToMovement = true;
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
