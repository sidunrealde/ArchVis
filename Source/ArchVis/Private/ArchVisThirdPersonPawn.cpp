// ArchVisThirdPersonPawn.cpp

#include "ArchVisThirdPersonPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AArchVisThirdPersonPawn::AArchVisThirdPersonPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	// Configure capsule
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	// Create camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = DefaultBoomLength;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 12.0f;
	CameraBoom->ProbeChannel = ECollisionChannel::ECC_Camera;

	// Create third-person camera
	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;

	// Configure character movement
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = WalkSpeed;
		MovementComp->bOrientRotationToMovement = true;
		MovementComp->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}

	// Controller rotation settings
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	TargetBoomLength = DefaultBoomLength;
}

void AArchVisThirdPersonPawn::BeginPlay()
{
	Super::BeginPlay();

	// Initialize boom length
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = DefaultBoomLength;
		TargetBoomLength = DefaultBoomLength;
	}

	// Update walk speed
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = WalkSpeed;
	}
}

void AArchVisThirdPersonPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth zoom interpolation
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = FMath::FInterpTo(
			CameraBoom->TargetArmLength,
			TargetBoomLength,
			DeltaTime,
			10.0f
		);
	}
}

void AArchVisThirdPersonPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Input bindings will be handled by the PlayerController
}

void AArchVisThirdPersonPawn::Look(FVector2D Delta)
{
	// Apply look input to controller
	AddControllerYawInput(Delta.X * LookSensitivity);
	AddControllerPitchInput(-Delta.Y * LookSensitivity);
}

void AArchVisThirdPersonPawn::Move(FVector2D Direction)
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

void AArchVisThirdPersonPawn::Zoom(float Amount)
{
	TargetBoomLength = FMath::Clamp(TargetBoomLength - (Amount * ZoomSpeed), MinBoomLength, MaxBoomLength);
}

void AArchVisThirdPersonPawn::SetInitialTransform(FVector Location, FRotator Rotation)
{
	SetActorLocation(Location);
	if (Controller)
	{
		Controller->SetControlRotation(Rotation);
	}
}

