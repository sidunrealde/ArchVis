// ArchVisFlyPawn.cpp

#include "ArchVisFlyPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AArchVisFlyPawn::AArchVisFlyPawn()
{
	PawnType = EArchVisPawnType::Fly;
	
	// Disable spring arm for fly pawn - camera is attached directly
	if (SpringArm)
	{
		SpringArm->TargetArmLength = 0.0f;
		SpringArm->bDoCollisionTest = false;
	}
}

void AArchVisFlyPawn::BeginPlay()
{
	Super::BeginPlay();

	// Configure for perspective view
	if (Camera)
	{
		Camera->ProjectionMode = ECameraProjectionMode::Perspective;
		Camera->FieldOfView = 90.0f;
	}

	// Set spring arm to zero length (camera at pawn location)
	if (SpringArm)
	{
		SpringArm->TargetArmLength = 0.0f;
		TargetArmLength = 0.0f;
	}

	// Initialize rotation from current actor rotation
	CurrentRotation = GetActorRotation();
	TargetRotation = CurrentRotation;
}

void AArchVisFlyPawn::Tick(float DeltaTime)
{
	// Don't call Super::Tick - we handle movement ourselves
	// Super interpolates spring arm which we don't want

	// Apply movement based on current input
	if (!CurrentMovementInput.IsNearlyZero() || !FMath::IsNearlyZero(CurrentVerticalInput))
	{
		float Speed = bSpeedBoost ? FlySpeed * FastFlyMultiplier : FlySpeed;
		
		// Get forward and right vectors based on current rotation
		FRotator YawRotation(0.0f, CurrentRotation.Yaw, 0.0f);
		FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		FVector Up = FVector::UpVector;

		// Calculate movement direction
		FVector MoveDirection = Forward * CurrentMovementInput.Y + Right * CurrentMovementInput.X + Up * CurrentVerticalInput;
		
		if (!MoveDirection.IsNearlyZero())
		{
			MoveDirection.Normalize();
			FVector NewLocation = GetActorLocation() + MoveDirection * Speed * DeltaTime;
			SetActorLocation(NewLocation);
		}
	}

	// Smooth rotation interpolation
	FRotator CurrentActorRotation = GetActorRotation();
	FRotator NewRotation = FMath::RInterpTo(CurrentActorRotation, CurrentRotation, DeltaTime, 15.0f);
	SetActorRotation(NewRotation);

	// Update spring arm rotation (for camera)
	if (SpringArm)
	{
		SpringArm->SetRelativeRotation(FRotator::ZeroRotator);
	}
}

void AArchVisFlyPawn::Look(FVector2D Delta)
{
	// Apply mouse look
	CurrentRotation.Yaw += Delta.X * LookSensitivity;
	CurrentRotation.Pitch = FMath::Clamp(CurrentRotation.Pitch + Delta.Y * LookSensitivity, -89.0f, 89.0f);
}

void AArchVisFlyPawn::Move(FVector Direction)
{
	float Speed = bSpeedBoost ? FlySpeed * FastFlyMultiplier : FlySpeed;
	FVector NewLocation = GetActorLocation() + Direction * Speed * GetWorld()->GetDeltaSeconds();
	SetActorLocation(NewLocation);
}

void AArchVisFlyPawn::SetMovementInput(FVector2D Input)
{
	CurrentMovementInput = Input;
}

void AArchVisFlyPawn::SetVerticalInput(float Input)
{
	CurrentVerticalInput = Input;
}

void AArchVisFlyPawn::Zoom(float Amount)
{
	// For fly pawn, zoom changes FOV
	if (Camera)
	{
		float NewFOV = Camera->FieldOfView - Amount * 2.0f;
		Camera->FieldOfView = FMath::Clamp(NewFOV, 30.0f, 120.0f);
	}
}

void AArchVisFlyPawn::Pan(FVector2D Delta)
{
	// Pan moves the pawn in the camera plane
	FRotator YawRotation(0.0f, CurrentRotation.Yaw, 0.0f);
	FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	FVector Up = FVector::UpVector;

	FVector MoveDirection = Right * Delta.X + Up * -Delta.Y;
	FVector NewLocation = GetActorLocation() + MoveDirection * PanSpeed;
	SetActorLocation(NewLocation);
}

void AArchVisFlyPawn::Orbit(FVector2D Delta)
{
	// For fly pawn, orbit just looks around
	Look(Delta);
}

void AArchVisFlyPawn::ResetView()
{
	SetActorLocation(FVector::ZeroVector);
	CurrentRotation = FRotator::ZeroRotator;
	if (Camera)
	{
		Camera->FieldOfView = 90.0f;
	}
}

