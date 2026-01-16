// ArchVisPawnBase.cpp

#include "ArchVisPawnBase.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

AArchVisPawnBase::AArchVisPawnBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root scene component
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	// Create spring arm
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootScene);
	SpringArm->bDoCollisionTest = false;
	SpringArm->TargetArmLength = TargetArmLength;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;

	// Create camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
}

void AArchVisPawnBase::BeginPlay()
{
	Super::BeginPlay();

	// Initialize targets from current state
	TargetArmLength = SpringArm->TargetArmLength;
	TargetRotation = SpringArm->GetRelativeRotation();
	TargetLocation = GetActorLocation();
}

void AArchVisPawnBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	InterpolateCameraState(DeltaTime);
}

void AArchVisPawnBase::InterpolateCameraState(float DeltaTime)
{
	// Smooth interpolation to target values
	SpringArm->TargetArmLength = FMath::FInterpTo(
		SpringArm->TargetArmLength, 
		TargetArmLength, 
		DeltaTime, 
		InterpSpeed
	);
	
	SpringArm->SetRelativeRotation(FMath::RInterpTo(
		SpringArm->GetRelativeRotation(), 
		TargetRotation, 
		DeltaTime, 
		InterpSpeed
	));
	
	SetActorLocation(FMath::VInterpTo(
		GetActorLocation(), 
		TargetLocation, 
		DeltaTime, 
		InterpSpeed
	));
}

void AArchVisPawnBase::Zoom(float Amount)
{
	// Base implementation - override in derived classes
	TargetArmLength = FMath::Clamp(TargetArmLength - (Amount * ZoomSpeed), MinZoom, MaxZoom);
}

void AArchVisPawnBase::Pan(FVector2D Delta)
{
	// Base implementation - move in XY plane
	FVector MoveDelta = FVector(-Delta.Y, Delta.X, 0.0f) * PanSpeed;
	TargetLocation -= MoveDelta;
}

void AArchVisPawnBase::Orbit(FVector2D Delta)
{
	// Base implementation - does nothing (override in 3D pawns)
}

void AArchVisPawnBase::ResetView()
{
	TargetLocation = FVector::ZeroVector;
	TargetArmLength = 5000.0f;
}

void AArchVisPawnBase::FocusOnLocation(FVector WorldLocation)
{
	TargetLocation = WorldLocation;
}

FVector AArchVisPawnBase::GetCameraLocation() const
{
	if (Camera)
	{
		return Camera->GetComponentLocation();
	}
	return GetActorLocation();
}

FRotator AArchVisPawnBase::GetCameraRotation() const
{
	if (Camera)
	{
		return Camera->GetComponentRotation();
	}
	return GetActorRotation();
}

void AArchVisPawnBase::SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel)
{
	TargetLocation = Location;
	TargetArmLength = ZoomLevel;
	// Rotation handling depends on pawn type - override in derived classes
}

float AArchVisPawnBase::GetZoomLevel() const
{
	return TargetArmLength;
}

