// ArchVisFlyPawn.cpp

#include "ArchVisFlyPawn.h"
#include "Camera/CameraComponent.h"

AArchVisFlyPawn::AArchVisFlyPawn()
{
	PawnType = EArchVisPawnType::Fly;
	
	// Create camera directly attached to root (no spring arm)
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootScene);
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

	// Initialize from current actor transform
	TargetLocation = GetActorLocation();
	TargetRotation = GetActorRotation();
}

void AArchVisFlyPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Apply movement based on current input
	if (!CurrentMovementInput.IsNearlyZero() || !FMath::IsNearlyZero(CurrentVerticalInput))
	{
		float Speed = bSpeedBoost ? FlySpeed * FastFlyMultiplier : FlySpeed;
		
		// Get forward and right vectors based on current rotation
		FVector Forward = TargetRotation.Vector();
		FVector Right = FRotationMatrix(TargetRotation).GetUnitAxis(EAxis::Y);
		FVector Up = FVector::UpVector;

		// Calculate movement direction
		FVector MoveDirection = Forward * CurrentMovementInput.Y + Right * CurrentMovementInput.X + Up * CurrentVerticalInput;
		
		if (!MoveDirection.IsNearlyZero())
		{
			MoveDirection.Normalize();
			TargetLocation += MoveDirection * Speed * DeltaTime;
		}
	}

	// Smooth interpolation
	SetActorLocation(FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, InterpSpeed));
	
	if (Camera)
	{
		FRotator CurrentRot = Camera->GetComponentRotation();
		FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRotation, DeltaTime, InterpSpeed);
		Camera->SetWorldRotation(NewRot);
	}
}

void AArchVisFlyPawn::Look(FVector2D Delta)
{
	// Apply mouse look
	TargetRotation.Yaw += Delta.X * LookSensitivity;
	TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch - Delta.Y * LookSensitivity, -89.0f, 89.0f);
	TargetRotation.Roll = 0.0f;
}

void AArchVisFlyPawn::Move(FVector Direction)
{
	float Speed = bSpeedBoost ? FlySpeed * FastFlyMultiplier : FlySpeed;
	TargetLocation += Direction * Speed * GetWorld()->GetDeltaSeconds();
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
	FVector Right = FRotationMatrix(TargetRotation).GetUnitAxis(EAxis::Y);
	FVector Up = FRotationMatrix(TargetRotation).GetUnitAxis(EAxis::Z);

	FVector MoveDirection = Right * -Delta.X + Up * Delta.Y;
	TargetLocation += MoveDirection * PanSpeed;
}

void AArchVisFlyPawn::Orbit(FVector2D Delta)
{
	// For fly pawn, orbit just looks around
	Look(Delta);
}

void AArchVisFlyPawn::ResetView()
{
	TargetLocation = FVector::ZeroVector;
	TargetRotation = FRotator::ZeroRotator;
	if (Camera)
	{
		Camera->FieldOfView = 90.0f;
	}
}

void AArchVisFlyPawn::FocusOnLocation(FVector WorldLocation)
{
	// Look at the location
	FVector Direction = WorldLocation - TargetLocation;
	if (!Direction.IsNearlyZero())
	{
		TargetRotation = Direction.Rotation();
	}
}

void AArchVisFlyPawn::SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel)
{
	TargetLocation = Location;
	TargetRotation = Rotation;
	TargetRotation.Roll = 0.0f;
	
	if (Camera)
	{
		// ZoomLevel maps to FOV for fly pawn
		Camera->FieldOfView = FMath::Clamp(ZoomLevel, 30.0f, 120.0f);
	}
}

float AArchVisFlyPawn::GetZoomLevel() const
{
	if (Camera)
	{
		return Camera->FieldOfView;
	}
	return 90.0f;
}

FVector AArchVisFlyPawn::GetCameraLocation() const
{
	if (Camera)
	{
		return Camera->GetComponentLocation();
	}
	return GetActorLocation();
}

FRotator AArchVisFlyPawn::GetCameraRotation() const
{
	if (Camera)
	{
		return Camera->GetComponentRotation();
	}
	return TargetRotation;
}

