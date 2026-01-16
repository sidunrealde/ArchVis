// ArchVisOrbitPawn.cpp

#include "ArchVisOrbitPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AArchVisOrbitPawn::AArchVisOrbitPawn()
{
	PawnType = EArchVisPawnType::Orbit3D;
	
	// Set default zoom range for 3D orbit
	MinZoom = 100.0f;
	MaxZoom = 50000.0f;
	TargetArmLength = 5000.0f;
	
	// 3D orbit settings
	OrbitSpeed = 0.5f;
	PanSpeed = 1.0f;
}

void AArchVisOrbitPawn::BeginPlay()
{
	Super::BeginPlay();

	// Configure for perspective view
	if (Camera)
	{
		Camera->ProjectionMode = ECameraProjectionMode::Perspective;
		Camera->FieldOfView = 60.0f;
	}

	// Set default orbit angle
	TargetRotation = FRotator(DefaultPitch, DefaultYaw, 0.0f);
	if (SpringArm)
	{
		SpringArm->SetRelativeRotation(TargetRotation);
	}
}

void AArchVisOrbitPawn::Tick(float DeltaTime)
{
	// Handle fly movement when RMB is held
	if (bFlyModeActive && (!CurrentMovementInput.IsNearlyZero() || !FMath::IsNearlyZero(CurrentVerticalInput)))
	{
		float Speed = bSpeedBoost ? FlySpeed * FastFlyMultiplier : FlySpeed;
		
		// Get forward and right vectors based on current camera rotation
		// Note: TargetRotation is the spring arm rotation, which points FROM camera TO target
		// So we negate to get the actual look direction
		FRotator CamRot = TargetRotation;
		FVector Forward = -FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);  // Negate because spring arm points opposite to look
		FVector Right = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
		FVector Up = FVector::UpVector;

		// Calculate movement direction (WASD + QE)
		// Input.Y = forward/back (W=+1, S=-1)
		// Input.X = right/left (D=+1, A=-1)
		FVector MoveDirection = Forward * CurrentMovementInput.Y + Right * CurrentMovementInput.X + Up * CurrentVerticalInput;
		
		// Debug logging
		static int32 MoveLogCount = 0;
		if (MoveLogCount < 10)
		{
			UE_LOG(LogTemp, Log, TEXT("OrbitPawn Move: Input=%s, CamRot=%s"), 
				*CurrentMovementInput.ToString(), *CamRot.ToString());
			UE_LOG(LogTemp, Log, TEXT("  Forward=%s, Right=%s, MoveDir=%s"), 
				*Forward.ToString(), *Right.ToString(), *MoveDirection.ToString());
			MoveLogCount++;
		}
		
		if (!MoveDirection.IsNearlyZero())
		{
			MoveDirection.Normalize();
			TargetLocation += MoveDirection * Speed * DeltaTime;
		}
	}

	// Call base class for interpolation
	Super::Tick(DeltaTime);
}

void AArchVisOrbitPawn::Zoom(float Amount)
{
	TargetArmLength = FMath::Clamp(TargetArmLength - (Amount * ZoomSpeed), MinZoom, MaxZoom);
}

void AArchVisOrbitPawn::Pan(FVector2D Delta)
{
	// Unreal Editor-style pan: move in camera's local XY plane
	FRotator CamRot = TargetRotation;
	
	// Get camera's right and up vectors
	// Note: Spring arm rotation points FROM camera TO target, so Right is correct but we negate for screen mapping
	FVector Right = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
	FVector Up = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Z);
	
	// Scale pan speed based on distance (feels more natural)
	float DistanceScale = FMath::Max(TargetArmLength, SavedArmLength) / 5000.0f;
	
	// Screen drag: right (Delta.X+) should move scene right (camera target moves right)
	// Screen drag: up (Delta.Y+) should move scene up (but Delta.Y is typically inverted)
	FVector MoveDelta = (Right * Delta.X + Up * -Delta.Y) * PanSpeed * DistanceScale;
	TargetLocation += MoveDelta;
}

void AArchVisOrbitPawn::Orbit(FVector2D Delta)
{
	FRotator NewRot = TargetRotation;
	
	// Yaw (horizontal orbit) - mouse right should orbit right
	NewRot.Yaw += Delta.X * OrbitSpeed;
	
	// Pitch (vertical orbit) - mouse up should orbit up (decrease pitch in Unreal)
	NewRot.Pitch = FMath::Clamp(NewRot.Pitch - (Delta.Y * OrbitSpeed), MinPitch, MaxPitch);
	
	TargetRotation = NewRot;
}

void AArchVisOrbitPawn::ResetView()
{
	TargetLocation = FVector::ZeroVector;
	TargetArmLength = 5000.0f;
	TargetRotation = FRotator(DefaultPitch, DefaultYaw, 0.0f);
	bFlyModeActive = false;
	CurrentMovementInput = FVector2D::ZeroVector;
	CurrentVerticalInput = 0.0f;
}

void AArchVisOrbitPawn::SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel)
{
	TargetLocation = Location;
	TargetArmLength = ZoomLevel;
	
	// Apply rotation with pitch clamping
	TargetRotation.Yaw = Rotation.Yaw;
	TargetRotation.Pitch = FMath::Clamp(Rotation.Pitch, MinPitch, MaxPitch);
	TargetRotation.Roll = 0.0f;
}

void AArchVisOrbitPawn::SetMovementInput(FVector2D Input)
{
	CurrentMovementInput = Input;
}

void AArchVisOrbitPawn::SetVerticalInput(float Input)
{
	CurrentVerticalInput = Input;
}

void AArchVisOrbitPawn::Look(FVector2D Delta)
{
	// When in fly mode, looking rotates the camera direction
	// Delta.X = mouse right (positive) should yaw right (increase yaw)
	// Delta.Y = mouse up (positive in screen coords) should pitch up (decrease pitch in Unreal)
	FRotator NewRot = TargetRotation;
	NewRot.Yaw += Delta.X * LookSensitivity;
	NewRot.Pitch = FMath::Clamp(NewRot.Pitch - (Delta.Y * LookSensitivity), MinPitch, MaxPitch);  // Negate Y for correct inversion
	TargetRotation = NewRot;
}

void AArchVisOrbitPawn::DollyZoom(float Amount)
{
	// Alt+RMB zoom: move camera forward/backward along view direction
	// This changes the arm length
	float ZoomAmount = Amount * ZoomSpeed * 0.5f;
	TargetArmLength = FMath::Clamp(TargetArmLength - ZoomAmount, MinZoom, MaxZoom);
}

void AArchVisOrbitPawn::FocusOnLocation(FVector WorldLocation)
{
	// Move the orbit target to this location
	TargetLocation = WorldLocation;
	
	// Optionally adjust zoom based on distance
	// For now, keep current zoom level
}

void AArchVisOrbitPawn::SetFlyModeActive(bool bActive)
{
	// Guard against redundant activations
	if (bFlyModeActive == bActive)
	{
		return;
	}
	
	bFlyModeActive = bActive;
	
	if (bActive)
	{
		// When entering fly mode, move target to current camera position
		// so WASD movement starts from where the camera is
		if (SpringArm)
		{
			// Calculate current camera world position
			FVector CameraWorldPos = TargetLocation + SpringArm->GetRelativeRotation().Vector() * (-TargetArmLength);
			
			// Set target to camera position and zero out arm length
			SavedArmLength = TargetArmLength;
			TargetLocation = CameraWorldPos;
			TargetArmLength = 0.0f;
		}
	}
	else
	{
		// When exiting fly mode, restore arm length
		// The camera will smoothly transition back to orbit mode
		TargetArmLength = SavedArmLength;
		
		// Clear movement input
		CurrentMovementInput = FVector2D::ZeroVector;
		CurrentVerticalInput = 0.0f;
	}
}


