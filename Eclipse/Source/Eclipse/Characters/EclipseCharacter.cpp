#include "Characters/EclipseCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/EclipseHealthAttributeSet.h"
#include "Animation/AnimSequence.h"
#include "Characters/EclipseCharacterTypes.h"
#include "Components/SkeletalMeshComponent.h"
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
