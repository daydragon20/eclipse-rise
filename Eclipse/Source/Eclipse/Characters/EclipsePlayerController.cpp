#include "Characters/EclipsePlayerController.h"

#include "Characters/EclipseCharacter.h"
#include "Combat/EclipseHitscanWeaponComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Squad/EclipseSquadSubsystem.h"

void AEclipsePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		InputSubsystem->AddMappingContext(MappingContext, /*Priority*/ 0);
	}
}

void AEclipsePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	MappingContext = NewObject<UInputMappingContext>(this);

	auto MakeAction = [this](EInputActionValueType ValueType)
	{
		UInputAction* Action = NewObject<UInputAction>(this);
		Action->ValueType = ValueType;
		return Action;
	};

	MoveAction = MakeAction(EInputActionValueType::Axis2D);
	LookAction = MakeAction(EInputActionValueType::Axis2D);
	FireAction = MakeAction(EInputActionValueType::Boolean);
	SprintAction = MakeAction(EInputActionValueType::Boolean);
	CrouchAction = MakeAction(EInputActionValueType::Boolean);

	// WASD with swizzle/negate modifiers (standard EI idiom, in code for now).
	auto MapKey = [this](UInputAction* Action, FKey Key, bool bNegate = false, bool bSwizzle = false)
	{
		FEnhancedActionKeyMapping& Mapping = MappingContext->MapKey(Action, Key);
		if (bSwizzle)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		}
		if (bNegate)
		{
			Mapping.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		}
	};

	MapKey(MoveAction, EKeys::W, /*negate*/ false, /*swizzle*/ true);
	MapKey(MoveAction, EKeys::S, /*negate*/ true, /*swizzle*/ true);
	MapKey(MoveAction, EKeys::D);
	MapKey(MoveAction, EKeys::A, /*negate*/ true);
	MapKey(LookAction, EKeys::Mouse2D);
	MapKey(FireAction, EKeys::LeftMouseButton);
	MapKey(SprintAction, EKeys::LeftShift);
	MapKey(CrouchAction, EKeys::LeftControl);

	const FKey OrderKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
	OrderActions.SetNum(4);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		OrderActions[Index] = MakeAction(EInputActionValueType::Boolean);
		MapKey(OrderActions[Index], OrderKeys[Index]);
	}

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(InputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleMove);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleLook);
	Input->BindAction(FireAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleFire);
	Input->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AEclipsePlayerController::HandleSprint);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &AEclipsePlayerController::HandleSprint);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &AEclipsePlayerController::HandleCrouch);

	Input->BindActionValueLambda(OrderActions[0], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::MoveTo); });
	Input->BindActionValueLambda(OrderActions[1], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::FocusTarget); });
	Input->BindActionValueLambda(OrderActions[2], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::Hold); });
	Input->BindActionValueLambda(OrderActions[3], ETriggerEvent::Started, [this](const FInputActionValue&) { IssueSquadOrder(EEclipseSquadOrder::Regroup); });
}

void AEclipsePlayerController::HandleMove(const FInputActionValue& Value)
{
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn == nullptr)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0, GetControlRotation().Yaw, 0);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Axis.Y);
	ControlledPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Axis.X);
}

void AEclipsePlayerController::HandleLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddYawInput(Axis.X);
	AddPitchInput(-Axis.Y);
}

void AEclipsePlayerController::HandleFire()
{
	AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	UEclipseHitscanWeaponComponent* Weapon = Body != nullptr ? Body->FindComponentByClass<UEclipseHitscanWeaponComponent>() : nullptr;
	if (Weapon == nullptr)
	{
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);
	Weapon->Fire(ViewLocation, ViewRotation.Vector(), TEXT("PlayerFire"));
}

void AEclipsePlayerController::HandleSprint(const FInputActionValue& Value)
{
	// PLACEHOLDER(feel targets §2): sprint toggles run<->sprint speed; stamina
	// only bites above Medium armor and armor lands later.
	AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn());
	if (Body == nullptr)
	{
		return;
	}
	const bool bSprinting = Value.Get<bool>();
	Body->GetCharacterMovement()->MaxWalkSpeed = bSprinting ? 650.0f : 420.0f;
}

void AEclipsePlayerController::HandleCrouch()
{
	if (AEclipseCharacter* Body = Cast<AEclipseCharacter>(GetPawn()))
	{
		Body->bIsCrouched ? Body->UnCrouch() : Body->Crouch();
	}
}

bool AEclipsePlayerController::GetAimPoint(FVector& OutLocation, AActor*& OutActor) const
{
	FVector ViewLocation;
	FRotator ViewRotation;
	GetPlayerViewPoint(ViewLocation, ViewRotation);

	constexpr float AimReachCm = 10000.0f;
	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * AimReachCm;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(EclipseAim), false, GetPawn());

	// Geometry (Visibility) fixes the move-to point — walls and floor, which pawn
	// capsules deliberately don't block.
	FHitResult GeoHit;
	const bool bHitGeo = GetWorld()->LineTraceSingleByChannel(GeoHit, ViewLocation, TraceEnd, ECC_Visibility, Params);
	OutLocation = bHitGeo ? GeoHit.ImpactPoint : TraceEnd;

	// Actor acquisition MUST use a channel pawn capsules block (Pawn) — the same
	// channel the weapon fires on. Tracing actors on Visibility passes straight
	// through every character, so Focus-target could never lock a live enemy.
	FHitResult PawnHit;
	OutActor = nullptr;
	if (GetWorld()->LineTraceSingleByChannel(PawnHit, ViewLocation, TraceEnd, ECC_Pawn, Params))
	{
		OutActor = PawnHit.GetActor();
		if (!bHitGeo || PawnHit.Distance < GeoHit.Distance)
		{
			OutLocation = PawnHit.ImpactPoint; // a body in front of the wall is the aim point
		}
	}
	return OutActor != nullptr || bHitGeo;
}

void AEclipsePlayerController::IssueSquadOrder(EEclipseSquadOrder Order)
{
	UEclipseSquadSubsystem* Squad = GetWorld()->GetSubsystem<UEclipseSquadSubsystem>();
	if (Squad == nullptr)
	{
		return;
	}

	FVector AimLocation = GetPawn() != nullptr ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
	AActor* AimActor = nullptr;
	GetAimPoint(AimLocation, AimActor);

	const FVector Target = Order == EEclipseSquadOrder::Regroup && GetPawn() != nullptr
		? GetPawn()->GetActorLocation()
		: AimLocation;
	Squad->IssueOrderToAll(Order, Target, AimActor);
}
