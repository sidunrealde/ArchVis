// ArchVisFirstPersonPawn.cpp

#include "ArchVisFirstPersonPawn.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AArchVisFirstPersonPawn::AArchVisFirstPersonPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Configure capsule
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// Create first-person camera
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, EyeHeightOffset));
	FirstPersonCamera->bUsePawnControlRotation = true;

	// Configure character movement
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = WalkSpeed;
		MovementComp->bOrientRotationToMovement = false;
		MovementComp->bUseControllerDesiredRotation = false;
	}

	// Don't rotate when the controller rotates
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
}

void AArchVisFirstPersonPawn::BeginPlay()
{
	Super::BeginPlay();

	// Update camera position based on eye height
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, EyeHeightOffset));
	}

	// Update walk speed
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = WalkSpeed;
	}
}

void AArchVisFirstPersonPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AArchVisFirstPersonPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Input bindings will be handled by the PlayerController
}

void AArchVisFirstPersonPawn::Look(FVector2D Delta)
{
	// Apply look input
	AddControllerYawInput(Delta.X * LookSensitivity);
	AddControllerPitchInput(-Delta.Y * LookSensitivity);
}

void AArchVisFirstPersonPawn::Move(FVector2D Direction)
{
	if (Controller)
	{
		// Get forward and right vectors based on controller yaw
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDir = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Move in the direction
		AddMovementInput(ForwardDir, Direction.Y);
		AddMovementInput(RightDir, Direction.X);
	}
}

void AArchVisFirstPersonPawn::SetInitialTransform(FVector Location, FRotator Rotation)
{
	SetActorLocation(Location);
	if (Controller)
	{
		Controller->SetControlRotation(Rotation);
	}
}

