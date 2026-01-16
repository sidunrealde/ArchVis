// ArchVisOrbitPawn.cpp
// 3D Orbit pawn - direct camera movement without spring arm

#include "ArchVisOrbitPawn.h"
#include "Input/OrbitInputComponent.h"
#include "ArchVisInputConfig.h"
#include "Camera/CameraComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogOrbitPawn, Log, All);

AArchVisOrbitPawn::AArchVisOrbitPawn()
{
	PawnType = EArchVisPawnType::Orbit3D;

	// Create camera directly attached to root (no spring arm)
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootScene);
	Camera->SetRelativeLocation(FVector::ZeroVector);
	Camera->SetRelativeRotation(FRotator::ZeroRotator);

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

	// Initialize camera position - start looking at origin from default distance/angle
	OrbitFocusPoint = FVector::ZeroVector;
	OrbitDistance = DefaultDistance;
	TargetCameraRotation = FRotator(DefaultPitch, DefaultYaw, 0.0f);

	// Calculate initial camera position based on orbit parameters
	FVector Offset = TargetCameraRotation.Vector() * -OrbitDistance;
	TargetCameraLocation = OrbitFocusPoint + Offset;

	// Set initial position immediately
	SetActorLocation(TargetCameraLocation);
	Camera->SetWorldRotation(TargetCameraRotation);
}

void AArchVisOrbitPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Process fly movement when in fly mode
	if (bFlyModeActive && (!CurrentMovementInput.IsNearlyZero() || !FMath::IsNearlyZero(CurrentVerticalInput)))
	{
		float Speed = bSpeedBoost ? FlySpeed * FastFlyMultiplier : FlySpeed;

		// Get camera's forward, right, up vectors
		FVector Forward = TargetCameraRotation.Vector();
		FVector Right = FRotationMatrix(TargetCameraRotation).GetUnitAxis(EAxis::Y);
		FVector Up = FVector::UpVector;

		// Calculate movement direction
		FVector MoveDirection = Forward * CurrentMovementInput.Y + Right * CurrentMovementInput.X + Up * CurrentVerticalInput;

		if (!MoveDirection.IsNearlyZero())
		{
			MoveDirection.Normalize();
			TargetCameraLocation += MoveDirection * Speed * DeltaTime;

			// Update orbit focus point to be in front of camera at current orbit distance
			OrbitFocusPoint = TargetCameraLocation + TargetCameraRotation.Vector() * OrbitDistance;
		}
	}

	InterpolateCameraState(DeltaTime);
}

void AArchVisOrbitPawn::InterpolateCameraState(float DeltaTime)
{
	// Smooth interpolation to target values
	FVector CurrentLoc = GetActorLocation();
	FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetCameraLocation, DeltaTime, InterpSpeed);
	SetActorLocation(NewLoc);

	if (Camera)
	{
		FRotator CurrentRot = Camera->GetComponentRotation();
		FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetCameraRotation, DeltaTime, InterpSpeed);
		Camera->SetWorldRotation(NewRot);
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

// --- Camera Interface ---

void AArchVisOrbitPawn::Zoom(float Amount)
{
	// Zoom moves camera forward/backward along view direction
	FVector Forward = TargetCameraRotation.Vector();
	TargetCameraLocation += Forward * Amount * ZoomSpeed;

	// Update orbit distance
	OrbitDistance = FVector::Dist(TargetCameraLocation, OrbitFocusPoint);
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("Zoom: Amount=%.2f NewDist=%.2f"), Amount, OrbitDistance);
	}
}

void AArchVisOrbitPawn::Pan(FVector2D Delta)
{
	if (Delta.IsNearlyZero())
	{
		return;
	}

	// Pan moves camera and focus point together in camera's local XY plane
	FVector Right = FRotationMatrix(TargetCameraRotation).GetUnitAxis(EAxis::Y);
	FVector Up = FRotationMatrix(TargetCameraRotation).GetUnitAxis(EAxis::Z);
	
	// Scale pan speed based on distance from focus point
	float DistanceScale = FMath::Max(OrbitDistance / 1000.0f, 0.5f);
	
	FVector MoveDelta = (Right * -Delta.X + Up * Delta.Y) * PanSpeed * DistanceScale;
	
	TargetCameraLocation += MoveDelta;
	OrbitFocusPoint += MoveDelta;
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("Pan: Delta=%s Move=%s"), *Delta.ToString(), *MoveDelta.ToString());
	}
}

void AArchVisOrbitPawn::Orbit(FVector2D Delta)
{
	if (Delta.IsNearlyZero())
	{
		return;
	}

	// Orbit rotates camera around the focus point
	FRotator NewRot = TargetCameraRotation;
	
	// Yaw - horizontal orbit
	NewRot.Yaw += Delta.X * OrbitSpeed;
	
	// Pitch - vertical orbit (clamped)
	NewRot.Pitch = FMath::Clamp(NewRot.Pitch - Delta.Y * OrbitSpeed, MinPitch, MaxPitch);
	NewRot.Roll = 0.0f;
	
	TargetCameraRotation = NewRot;

	// Recalculate camera position based on new rotation and orbit distance
	FVector Offset = TargetCameraRotation.Vector() * -OrbitDistance;
	TargetCameraLocation = OrbitFocusPoint + Offset;
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("Orbit: Delta=%s Rot=%s Dist=%.2f"), *Delta.ToString(), *TargetCameraRotation.ToString(), OrbitDistance);
	}
}

void AArchVisOrbitPawn::ResetView()
{
	OrbitFocusPoint = FVector::ZeroVector;
	OrbitDistance = DefaultDistance;
	TargetCameraRotation = FRotator(DefaultPitch, DefaultYaw, 0.0f);

	FVector Offset = TargetCameraRotation.Vector() * -OrbitDistance;
	TargetCameraLocation = OrbitFocusPoint + Offset;

	bFlyModeActive = false;
	bOrbitModeActive = false;
	bDollyModeActive = false;
	CurrentMovementInput = FVector2D::ZeroVector;
	CurrentVerticalInput = 0.0f;
}

void AArchVisOrbitPawn::FocusOnLocation(FVector WorldLocation)
{
	OrbitFocusPoint = WorldLocation;

	// Keep current rotation and distance, just move to focus on new point
	FVector Offset = TargetCameraRotation.Vector() * -OrbitDistance;
	TargetCameraLocation = OrbitFocusPoint + Offset;
}

void AArchVisOrbitPawn::SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel)
{
	TargetCameraLocation = Location;
	TargetCameraRotation = Rotation;
	TargetCameraRotation.Pitch = FMath::Clamp(TargetCameraRotation.Pitch, MinPitch, MaxPitch);
	TargetCameraRotation.Roll = 0.0f;
	
	OrbitDistance = ZoomLevel;
	OrbitFocusPoint = TargetCameraLocation + TargetCameraRotation.Vector() * OrbitDistance;
}

float AArchVisOrbitPawn::GetZoomLevel() const
{
	return OrbitDistance;
}

FVector AArchVisOrbitPawn::GetCameraLocation() const
{
	if (Camera)
	{
		return Camera->GetComponentLocation();
	}
	return GetActorLocation();
}

FRotator AArchVisOrbitPawn::GetCameraRotation() const
{
	if (Camera)
	{
		return Camera->GetComponentRotation();
	}
	return TargetCameraRotation;
}

// --- Movement ---

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
	if (Delta.IsNearlyZero())
	{
		return;
	}

	// Look rotates the camera in place (updates rotation, focus point moves)
	FRotator NewRot = TargetCameraRotation;
	NewRot.Yaw += Delta.X * LookSensitivity;
	NewRot.Pitch = FMath::Clamp(NewRot.Pitch - Delta.Y * LookSensitivity, MinPitch, MaxPitch);
	NewRot.Roll = 0.0f;

	TargetCameraRotation = NewRot;

	// Update focus point to be in front of camera
	OrbitFocusPoint = TargetCameraLocation + TargetCameraRotation.Vector() * OrbitDistance;
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("Look: Delta=%s Rot=%s"), *Delta.ToString(), *TargetCameraRotation.ToString());
	}
}

void AArchVisOrbitPawn::DollyZoom(float Amount)
{
	if (FMath::IsNearlyZero(Amount))
	{
		return;
	}

	// Dolly moves camera along view direction (changes distance to focus point)
	FVector Forward = TargetCameraRotation.Vector();
	float DollyAmount = Amount * ZoomSpeed * 0.1f;
	
	TargetCameraLocation += Forward * DollyAmount;
	OrbitDistance = FVector::Dist(TargetCameraLocation, OrbitFocusPoint);
	
	// Clamp minimum distance
	if (OrbitDistance < 100.0f)
	{
		OrbitDistance = 100.0f;
		TargetCameraLocation = OrbitFocusPoint - Forward * OrbitDistance;
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("DollyZoom: Amount=%.2f NewDist=%.2f"), Amount, OrbitDistance);
	}
}

// --- Mode Control ---

void AArchVisOrbitPawn::SetFlyModeActive(bool bActive)
{
	if (bFlyModeActive == bActive)
	{
		return;
	}
	
	bFlyModeActive = bActive;
	
	if (!bActive)
	{
		// Clear movement when exiting fly mode
		CurrentMovementInput = FVector2D::ZeroVector;
		CurrentVerticalInput = 0.0f;
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("SetFlyModeActive: %d"), bActive);
	}
}

void AArchVisOrbitPawn::SetOrbitModeActive(bool bActive)
{
	if (bOrbitModeActive == bActive)
	{
		return;
	}
	
	bOrbitModeActive = bActive;

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("SetOrbitModeActive: %d"), bActive);
	}
}

void AArchVisOrbitPawn::SetDollyModeActive(bool bActive)
{
	if (bDollyModeActive == bActive)
	{
		return;
	}
	
	bDollyModeActive = bActive;

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitPawn, Log, TEXT("SetDollyModeActive: %d"), bActive);
	}
}

void AArchVisOrbitPawn::ArchVisOrbitDebug()
{
	bDebugEnabled = !bDebugEnabled;
	UE_LOG(LogOrbitPawn, Log, TEXT("OrbitPawn debug: %s"), bDebugEnabled ? TEXT("ON") : TEXT("OFF"));
	UE_LOG(LogOrbitPawn, Log, TEXT("  CamLoc=%s CamRot=%s"), *TargetCameraLocation.ToString(), *TargetCameraRotation.ToString());
	UE_LOG(LogOrbitPawn, Log, TEXT("  FocusPoint=%s OrbitDist=%.2f"), *OrbitFocusPoint.ToString(), OrbitDistance);
	UE_LOG(LogOrbitPawn, Log, TEXT("  Fly=%d Orbit=%d Dolly=%d"), bFlyModeActive, bOrbitModeActive, bDollyModeActive);
}

