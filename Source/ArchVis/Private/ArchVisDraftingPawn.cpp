// ArchVisDraftingPawn.cpp

#include "ArchVisDraftingPawn.h"
#include "Input/DraftingInputComponent.h"
#include "ArchVisInputConfig.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AArchVisDraftingPawn::AArchVisDraftingPawn()
{
	PawnType = EArchVisPawnType::Drafting2D;
	
	// Create spring arm (attached to RootScene from base class)
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootScene);
	SpringArm->bDoCollisionTest = false;
	SpringArm->TargetArmLength = TargetArmLength;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;

	// Create camera attached to spring arm
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	// Create input component
	DraftingInput = CreateDefaultSubobject<UDraftingInputComponent>(TEXT("DraftingInput"));
}

void AArchVisDraftingPawn::BeginPlay()
{
	Super::BeginPlay();

	// Initialize target state
	TargetArmLength = SpringArm->TargetArmLength;
	TargetLocation = GetActorLocation();

	// Configure for orthographic top-down view
	if (Camera)
	{
		Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
		UpdateOrthoWidth();
	}

	if (SpringArm)
	{
		// Look straight down (-Z axis in world = looking at XY plane)
		SpringArm->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	}
}

void AArchVisDraftingPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	InterpolateCameraState(DeltaTime);
}

void AArchVisDraftingPawn::InterpolateCameraState(float DeltaTime)
{
	if (!SpringArm)
	{
		return;
	}

	// Smooth interpolation for zoom
	SpringArm->TargetArmLength = FMath::FInterpTo(
		SpringArm->TargetArmLength,
		TargetArmLength,
		DeltaTime,
		InterpSpeed
	);

	// Only interpolate position if not already at target (pan sets position instantly)
	const FVector CurrentLocation = GetActorLocation();
	if (!CurrentLocation.Equals(TargetLocation, 0.1f))
	{
		SetActorLocation(FMath::VInterpTo(
			CurrentLocation,
			TargetLocation,
			DeltaTime,
			InterpSpeed
		));
	}

	// Update ortho width during interpolation
	UpdateOrthoWidth();
}

void AArchVisDraftingPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (DraftingInput)
	{
		DraftingInput->OnPawnPossessed(NewController);
	}
}

void AArchVisDraftingPawn::UnPossessed()
{
	if (DraftingInput)
	{
		DraftingInput->OnPawnUnpossessed(GetController());
	}

	Super::UnPossessed();
}

void AArchVisDraftingPawn::InitializeInput(UArchVisInputConfig* InputConfig)
{
	if (DraftingInput && InputConfig)
	{
		DraftingInput->Initialize(InputConfig);
	}
}

void AArchVisDraftingPawn::Zoom(float Amount)
{
	TargetArmLength = FMath::Clamp(TargetArmLength - (Amount * ZoomSpeed), MinZoom, MaxZoom);
}

void AArchVisDraftingPawn::Pan(FVector2D Delta)
{
	// Make the point under the cursor follow the cursor exactly
	// Use deprojection for accurate screen-to-world conversion
	
	if (!Camera || !SpringArm)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// Get current mouse position
	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	// Calculate previous mouse position from delta
	const float PrevMouseX = MouseX - Delta.X;
	const float PrevMouseY = MouseY - Delta.Y;

	// Deproject current and previous mouse positions to world space
	FVector CurrWorldPos, CurrWorldDir;
	FVector PrevWorldPos, PrevWorldDir;
	
	PC->DeprojectScreenPositionToWorld(MouseX, MouseY, CurrWorldPos, CurrWorldDir);
	PC->DeprojectScreenPositionToWorld(PrevMouseX, PrevMouseY, PrevWorldPos, PrevWorldDir);

	// For orthographic camera looking down, we can use the X,Y directly
	// The world positions are on a plane at the camera's Z position
	// We just need the XY difference
	const FVector WorldDelta(
		CurrWorldPos.X - PrevWorldPos.X,
		CurrWorldPos.Y - PrevWorldPos.Y,
		0.0f
	);


	// Move camera opposite to the delta (dragging world = camera moves opposite)
	TargetLocation -= WorldDelta;

	// Move instantly for 1:1 cursor tracking
	SetActorLocation(TargetLocation);
}

void AArchVisDraftingPawn::ResetView()
{
	TargetLocation = FVector::ZeroVector;
	TargetArmLength = 5000.0f;
}

void AArchVisDraftingPawn::FocusOnLocation(FVector WorldLocation)
{
	TargetLocation = FVector(WorldLocation.X, WorldLocation.Y, 0.0f);
}

void AArchVisDraftingPawn::SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel)
{
	// For 2D, we only use location and zoom - rotation is fixed
	TargetLocation = FVector(Location.X, Location.Y, 0.0f);
	TargetArmLength = ZoomLevel;
}

float AArchVisDraftingPawn::GetZoomLevel() const
{
	if (Camera)
	{
		return Camera->OrthoWidth;
	}
	return TargetArmLength * OrthoWidthMultiplier;
}

FVector AArchVisDraftingPawn::GetCameraLocation() const
{
	if (Camera)
	{
		return Camera->GetComponentLocation();
	}
	return GetActorLocation();
}

FRotator AArchVisDraftingPawn::GetCameraRotation() const
{
	if (Camera)
	{
		return Camera->GetComponentRotation();
	}
	return FRotator(-90.0f, 0.0f, 0.0f);
}

void AArchVisDraftingPawn::UpdateOrthoWidth()
{
	if (Camera && SpringArm)
	{
		Camera->OrthoWidth = SpringArm->TargetArmLength * OrthoWidthMultiplier;
	}
}
