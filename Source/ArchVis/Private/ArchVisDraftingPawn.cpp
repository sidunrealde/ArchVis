// ArchVisDraftingPawn.cpp

#include "ArchVisDraftingPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AArchVisDraftingPawn::AArchVisDraftingPawn()
{
	PawnType = EArchVisPawnType::Drafting2D;
	
	// Set default zoom range for 2D drafting
	MinZoom = 1000.0f;
	MaxZoom = 20000.0f;
	TargetArmLength = 5000.0f;
}

void AArchVisDraftingPawn::BeginPlay()
{
	Super::BeginPlay();

	// Configure for orthographic top-down view
	if (Camera)
	{
		Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
		UpdateOrthoWidth();
	}

	if (SpringArm)
	{
		// Look straight down (-Z axis in world = looking at XY plane)
		// Pitch -90 = looking down, Yaw 0 = X is right, Y is forward on screen
		TargetRotation = FRotator(-90.0f, 0.0f, 0.0f);
		SpringArm->SetRelativeRotation(TargetRotation);
	}
}

void AArchVisDraftingPawn::Zoom(float Amount)
{
	float OldArmLength = TargetArmLength;
	TargetArmLength = FMath::Clamp(TargetArmLength - (Amount * ZoomSpeed), MinZoom, MaxZoom);
	UE_LOG(LogTemp, Log, TEXT("DraftingPawn::Zoom: Amount=%f, OldArm=%f, NewArm=%f, ZoomSpeed=%f"), 
		Amount, OldArmLength, TargetArmLength, ZoomSpeed);
	UpdateOrthoWidth();
}

void AArchVisDraftingPawn::Pan(FVector2D Delta)
{
	// AutoCAD-style pan: the point under the cursor should stay under the cursor
	// We need to convert screen delta to world delta based on zoom level
	
	// OrthoWidth represents the visible width in world units
	float ZoomScale = 1.0f;
	if (Camera)
	{
		// Scale pan by ortho width - larger ortho width = more zoomed out = faster pan
		ZoomScale = Camera->OrthoWidth / 1000.0f; // Normalize to reasonable speed
	}
	
	// With camera looking down (Pitch=-90, Yaw=0):
	// Screen X (right) -> World X (positive)
	// Screen Y (up) -> World Y (positive)
	FVector MoveDelta = FVector(Delta.X, Delta.Y, 0.0f) * ZoomScale * PanSpeed;
	TargetLocation -= MoveDelta;
}

void AArchVisDraftingPawn::ResetView()
{
	TargetLocation = FVector::ZeroVector;
	TargetArmLength = 5000.0f;
	TargetRotation = FRotator(-90.0f, 0.0f, 0.0f);
	UpdateOrthoWidth();
}

void AArchVisDraftingPawn::SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel)
{
	// For 2D, we only use location and zoom - rotation is fixed
	TargetLocation = FVector(Location.X, Location.Y, 0.0f);
	TargetArmLength = ZoomLevel;
	TargetRotation = FRotator(-90.0f, 0.0f, 0.0f);
	UpdateOrthoWidth();
}

float AArchVisDraftingPawn::GetZoomLevel() const
{
	// In 2D, zoom level is the ortho width
	if (Camera)
	{
		return Camera->OrthoWidth;
	}
	return TargetArmLength * OrthoWidthMultiplier;
}

void AArchVisDraftingPawn::UpdateOrthoWidth()
{
	if (Camera)
	{
		Camera->OrthoWidth = TargetArmLength * OrthoWidthMultiplier;
	}
}

