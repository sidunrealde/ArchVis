// ArchVisOrbitPawn.cpp

#include "ArchVisOrbitPawn.h"
#include "Input/OrbitInputComponent.h"
#include "ArchVisInputConfig.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogOrbitPawn, Log, All);

AArchVisOrbitPawn::AArchVisOrbitPawn()
{
	PawnType = EArchVisPawnType::Orbit3D;
	
	// Set default zoom range for 3D orbit
	MinZoom = 100.0f;
	MaxZoom = 50000.0f;
	TargetArmLength = 5000.0f;
	
	// 3D orbit settings
	OrbitSpeed = 0.5f;
	PanSpeed = 2.0f;  // Higher default for responsive 3D panning

	// Create input component
	OrbitInput = CreateDefaultSubobject<UOrbitInputComponent>(TEXT("OrbitInput"));
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

void AArchVisOrbitPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (OrbitInput)
	{
		OrbitInput->OnPawnPossessed(NewController);
	}
}

void AArchVisOrbitPawn::UnPossessed()
{
	if (OrbitInput)
	{
		OrbitInput->OnPawnUnpossessed(GetController());
	}

	Super::UnPossessed();
}

void AArchVisOrbitPawn::InitializeInput(UArchVisInputConfig* InputConfig)
{
	if (OrbitInput && InputConfig)
	{
		OrbitInput->Initialize(InputConfig);
	}
}

void AArchVisOrbitPawn::Tick(float DeltaTime)
{
	// Handle fly movement when fly mode is active
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
		
		if (bDebugEnabled)
		{
			UE_LOG(LogOrbitPawn, Log, TEXT("Tick Move: Input=%s Vert=%.2f CamRot=%s MoveDir=%s"), 
				*CurrentMovementInput.ToString(), CurrentVerticalInput, *CamRot.ToString(), *MoveDirection.ToString());
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
	float OldLength = TargetArmLength;
	TargetArmLength = FMath::Clamp(TargetArmLength - (Amount * ZoomSpeed), MinZoom, MaxZoom);
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("Zoom: Amount=%.2f Old=%.2f New=%.2f"), Amount, OldLength, TargetArmLength);
	}
}

void AArchVisOrbitPawn::Pan(FVector2D Delta)
{
	// Skip if delta is zero
	if (Delta.IsNearlyZero())
	{
		return;
	}
	
	// Unreal Editor-style pan: move in camera's local XY plane
	FRotator CamRot = TargetRotation;
	
	// Get camera's right and up vectors
	// Note: Spring arm rotation points FROM camera TO target, so Right is correct but we negate for screen mapping
	FVector Right = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
	FVector Up = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Z);
	
	// Scale pan speed based on distance (feels more natural)
	// Use a more generous scaling that ensures movement is always visible
	float DistanceScale = FMath::Max(TargetArmLength, SavedArmLength) / 1000.0f;
	// Ensure a minimum scale so pan always feels responsive
	DistanceScale = FMath::Max(DistanceScale, 1.0f);
	
	// Screen drag: right (Delta.X+) should move scene right (camera target moves right)
	// Screen drag: up (Delta.Y+) should move scene up (but Delta.Y is typically inverted)
	FVector MoveDelta = (Right * Delta.X + Up * -Delta.Y) * PanSpeed * DistanceScale;
	FVector OldLoc = TargetLocation;
	TargetLocation += MoveDelta;
	
	// Debug log when enabled
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("Pan: Delta=%s CamRot=%s DistScale=%.2f PanSpeed=%.2f MoveDelta=%s OldLoc=%s NewLoc=%s"), 
			*Delta.ToString(), *CamRot.ToString(), DistanceScale, PanSpeed, *MoveDelta.ToString(), *OldLoc.ToString(), *TargetLocation.ToString());
	}
}

void AArchVisOrbitPawn::Orbit(FVector2D Delta)
{
	FRotator OldRot = TargetRotation;
	FRotator NewRot = TargetRotation;
	
	// Yaw (horizontal orbit) - mouse right should orbit right
	NewRot.Yaw += Delta.X * OrbitSpeed;
	
	// Pitch (vertical orbit) - mouse up should orbit up (decrease pitch in Unreal)
	NewRot.Pitch = FMath::Clamp(NewRot.Pitch - (Delta.Y * OrbitSpeed), MinPitch, MaxPitch);
	
	TargetRotation = NewRot;
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("Orbit: Delta=%s OldRot=%s NewRot=%s OrbitSpeed=%.2f"), 
			*Delta.ToString(), *OldRot.ToString(), *NewRot.ToString(), OrbitSpeed);
	}
}

void AArchVisOrbitPawn::ResetView()
{
	TargetLocation = FVector::ZeroVector;
	TargetArmLength = 5000.0f;
	TargetRotation = FRotator(DefaultPitch, DefaultYaw, 0.0f);
	bFlyModeActive = false;
	bOrbitModeActive = false;
	bDollyModeActive = false;
	CurrentMovementInput = FVector2D::ZeroVector;
	CurrentVerticalInput = 0.0f;
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("ResetView called"));
	}
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
	
	if (bDebugEnabled && !Input.IsNearlyZero())
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("SetMovementInput: %s FlyMode=%d"), *Input.ToString(), bFlyModeActive);
	}
}

void AArchVisOrbitPawn::SetVerticalInput(float Input)
{
	CurrentVerticalInput = Input;
	
	if (bDebugEnabled && !FMath::IsNearlyZero(Input))
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("SetVerticalInput: %.2f FlyMode=%d"), Input, bFlyModeActive);
	}
}

void AArchVisOrbitPawn::Look(FVector2D Delta)
{
	if (Delta.IsNearlyZero())
	{
		return;
	}
	
	// When in fly mode, looking rotates the camera direction
	// Delta.X = mouse right (positive) should yaw right (increase yaw)
	// Delta.Y = mouse up (positive in screen coords) should pitch up (decrease pitch in Unreal)
	FRotator OldRot = TargetRotation;
	FRotator NewRot = TargetRotation;
	NewRot.Yaw += Delta.X * LookSensitivity;
	NewRot.Pitch = FMath::Clamp(NewRot.Pitch - (Delta.Y * LookSensitivity), MinPitch, MaxPitch);  // Negate Y for correct inversion
	TargetRotation = NewRot;
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("Look: Delta=%s OldRot=%s NewRot=%s LookSens=%.2f FlyMode=%d"), 
			*Delta.ToString(), *OldRot.ToString(), *NewRot.ToString(), LookSensitivity, bFlyModeActive);
	}
}

void AArchVisOrbitPawn::DollyZoom(float Amount)
{
	if (FMath::IsNearlyZero(Amount))
	{
		return;
	}
	
	// Dolly zoom: move camera forward/backward along view direction
	// This changes the arm length
	float OldLength = TargetArmLength;
	float ZoomAmount = Amount * ZoomSpeed * 0.5f;
	TargetArmLength = FMath::Clamp(TargetArmLength - ZoomAmount, MinZoom, MaxZoom);
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("DollyZoom: Amount=%.2f ZoomAmt=%.2f Old=%.2f New=%.2f DollyMode=%d"), 
			Amount, ZoomAmount, OldLength, TargetArmLength, bDollyModeActive);
	}
}

void AArchVisOrbitPawn::FocusOnLocation(FVector WorldLocation)
{
	// Move the orbit target to this location
	TargetLocation = WorldLocation;
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("FocusOnLocation: %s"), *WorldLocation.ToString());
	}
}

void AArchVisOrbitPawn::SetFlyModeActive(bool bActive)
{
	// Guard against redundant activations
	if (bFlyModeActive == bActive)
	{
		return;
	}
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("SetFlyModeActive: %d -> %d"), bFlyModeActive, bActive);
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
			
			if (bDebugEnabled)
			{
				UE_LOG(LogOrbitPawn, Log, TEXT("  Fly mode entered: CamPos=%s SavedArm=%.2f"), *CameraWorldPos.ToString(), SavedArmLength);
			}
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
		
		if (bDebugEnabled)
		{
			UE_LOG(LogOrbitPawn, Log, TEXT("  Fly mode exited: RestoredArm=%.2f"), TargetArmLength);
		}
	}
}

void AArchVisOrbitPawn::SetOrbitModeActive(bool bActive)
{
	if (bOrbitModeActive == bActive)
	{
		return;
	}
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("SetOrbitModeActive: %d -> %d"), bOrbitModeActive, bActive);
	}
	
	bOrbitModeActive = bActive;
}

void AArchVisOrbitPawn::SetDollyModeActive(bool bActive)
{
	if (bDollyModeActive == bActive)
	{
		return;
	}
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("SetDollyModeActive: %d -> %d"), bDollyModeActive, bActive);
	}
	
	bDollyModeActive = bActive;
}

void AArchVisOrbitPawn::ArchVisOrbitDebug()
{
	bDebugEnabled = !bDebugEnabled;
	UE_LOG(LogOrbitPawn, Log, TEXT("OrbitPawn debug: %s"), bDebugEnabled ? TEXT("ON") : TEXT("OFF"));
	UE_LOG(LogOrbitPawn, Log, TEXT("  Current state: Fly=%d Orbit=%d Dolly=%d"), bFlyModeActive, bOrbitModeActive, bDollyModeActive);
	UE_LOG(LogOrbitPawn, Log, TEXT("  TargetLoc=%s TargetRot=%s ArmLen=%.2f"), *TargetLocation.ToString(), *TargetRotation.ToString(), TargetArmLength);
}
