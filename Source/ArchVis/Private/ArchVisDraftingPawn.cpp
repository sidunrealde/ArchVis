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

	// Smooth interpolation to target values
	SpringArm->TargetArmLength = FMath::FInterpTo(
		SpringArm->TargetArmLength,
		TargetArmLength,
		DeltaTime,
		InterpSpeed
	);

	SetActorLocation(FMath::VInterpTo(
		GetActorLocation(),
		TargetLocation,
		DeltaTime,
		InterpSpeed
	));

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
	// AutoCAD-style pan: the point under the cursor should stay under the cursor
	// Scale pan by ortho width - larger ortho width = more zoomed out = faster pan
	float ZoomScale = 1.0f;
	if (Camera)
	{
		ZoomScale = Camera->OrthoWidth / 1000.0f;
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

