#include "ArchVisCameraController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

AArchVisCameraController::AArchVisCameraController()
{
	PrimaryActorTick.bCanEverTick = true;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootScene);
	SpringArm->bDoCollisionTest = false; // Don't collide with walls in drafting mode
	SpringArm->TargetArmLength = 1000.0f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);
}

void AArchVisCameraController::BeginPlay()
{
	Super::BeginPlay();

	// Initialize targets
	TargetArmLength = SpringArm->TargetArmLength;
	TargetRotation = SpringArm->GetRelativeRotation();
	TargetLocation = GetActorLocation();

	// Set initial mode
	SetViewMode(CurrentViewMode);

	// Possess this camera
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->SetViewTarget(this);
	}
}

void AArchVisCameraController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smooth interpolation
	float InterpSpeed = 10.0f;
	
	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetArmLength, DeltaTime, InterpSpeed);
	SpringArm->SetRelativeRotation(FMath::RInterpTo(SpringArm->GetRelativeRotation(), TargetRotation, DeltaTime, InterpSpeed));
	SetActorLocation(FMath::VInterpTo(GetActorLocation(), TargetLocation, DeltaTime, InterpSpeed));
}

void AArchVisCameraController::SetViewMode(EArchVisViewMode NewMode)
{
	CurrentViewMode = NewMode;

	if (CurrentViewMode == EArchVisViewMode::TopDown2D)
	{
		// Ortho Setup
		Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
		Camera->OrthoWidth = TargetArmLength * 2.0f; // Approximate zoom mapping
		
		// Look down Z
		TargetRotation = FRotator(-90.0f, -90.0f, 0.0f);
	}
	else
	{
		// Perspective Setup
		Camera->ProjectionMode = ECameraProjectionMode::Perspective;
		
		// Default 3D angle
		TargetRotation = FRotator(-45.0f, 0.0f, 0.0f);
	}
}

void AArchVisCameraController::ToggleViewMode()
{
	if (CurrentViewMode == EArchVisViewMode::TopDown2D)
	{
		SetViewMode(EArchVisViewMode::Perspective3D);
	}
	else
	{
		SetViewMode(EArchVisViewMode::TopDown2D);
	}
}

void AArchVisCameraController::Zoom(float Amount)
{
	TargetArmLength = FMath::Clamp(TargetArmLength - (Amount * ZoomSpeed), MinZoom, MaxZoom);
	
	if (CurrentViewMode == EArchVisViewMode::TopDown2D)
	{
		Camera->OrthoWidth = TargetArmLength * 2.0f;
	}
}

void AArchVisCameraController::Pan(FVector2D Delta)
{
	// Pan moves the actor in XY plane
	FVector MoveDelta(0.f);
	
	if (CurrentViewMode == EArchVisViewMode::TopDown2D)
	{
		// In top-down, we move opposite to the mouse drag to simulate pulling the world.
		// Mouse X moves world left/right. Mouse Y moves world up/down.
		MoveDelta = FVector(-Delta.Y, Delta.X, 0.0f) * PanSpeed;
	}
	else
	{
		// In 3D, we move relative to the camera's yaw.
		FRotator YawRot(0, SpringArm->GetRelativeRotation().Yaw, 0);
		FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		
		// Mouse X moves left/right, Mouse Y moves forward/backward
		MoveDelta = (Forward * -Delta.Y + Right * Delta.X) * PanSpeed;
	}

	TargetLocation -= MoveDelta; // Subtract to move the world WITH the mouse
}

void AArchVisCameraController::Orbit(FVector2D Delta)
{
	if (CurrentViewMode == EArchVisViewMode::Perspective3D)
	{
		FRotator NewRot = TargetRotation;
		NewRot.Yaw += Delta.X * OrbitSpeed;
		NewRot.Pitch = FMath::Clamp(NewRot.Pitch + (Delta.Y * OrbitSpeed), -85.0f, -10.0f);
		TargetRotation = NewRot;
	}
}
