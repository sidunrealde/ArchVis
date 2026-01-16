// ArchVisPawnBase.h
// Base pawn class for all ArchVis pawn types

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ArchVisPawnBase.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * Enum for different pawn types in ArchVis
 */
UENUM(BlueprintType)
enum class EArchVisPawnType : uint8
{
	Drafting2D      UMETA(DisplayName = "2D Drafting"),
	Orbit3D         UMETA(DisplayName = "3D Orbit"),
	Fly             UMETA(DisplayName = "Fly"),
	FirstPerson     UMETA(DisplayName = "First Person"),
	ThirdPerson     UMETA(DisplayName = "Third Person"),
	VR              UMETA(DisplayName = "VR")
};

/**
 * Base pawn class for all ArchVis viewing modes.
 * Provides common camera functionality and interface for derived pawns.
 */
UCLASS(Abstract)
class ARCHVIS_API AArchVisPawnBase : public APawn
{
	GENERATED_BODY()

public:
	AArchVisPawnBase();

	virtual void Tick(float DeltaTime) override;

	// --- Camera Control Interface ---
	
	// Zoom in/out
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void Zoom(float Amount);

	// Pan the camera
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void Pan(FVector2D Delta);

	// Orbit around a point (3D modes only)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void Orbit(FVector2D Delta);

	// Reset camera to default position
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void ResetView();

	// Focus on a world location
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void FocusOnLocation(FVector WorldLocation);

	// Get the pawn type
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	EArchVisPawnType GetPawnType() const { return PawnType; }

	// Get the camera component
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	UCameraComponent* GetCameraComponent() const { return Camera; }

	// Get current camera world location
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	FVector GetCameraLocation() const;

	// Get current camera world rotation
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	FRotator GetCameraRotation() const;

	// Set camera transform (used when switching pawns to preserve view)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel);

	// Get current zoom level (arm length or ortho width depending on mode)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual float GetZoomLevel() const;

protected:
	virtual void BeginPlay() override;

	// The type of this pawn
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArchVis|Pawn")
	EArchVisPawnType PawnType = EArchVisPawnType::Drafting2D;

	// Root scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USceneComponent> RootScene;

	// Spring arm for camera positioning
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	// The camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// --- Camera Settings ---
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float MinZoom = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float MaxZoom = 50000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float ZoomSpeed = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float PanSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float OrbitSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float InterpSpeed = 10.0f;

	// --- Target State (for smooth interpolation) ---
	
	float TargetArmLength = 5000.0f;
	FRotator TargetRotation = FRotator::ZeroRotator;
	FVector TargetLocation = FVector::ZeroVector;

	// Apply smooth interpolation to camera
	virtual void InterpolateCameraState(float DeltaTime);
};

