// ArchVisPawnBase.h
// Base pawn class for all ArchVis pawn types

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ArchVisPawnBase.generated.h"

class UCameraComponent;

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
 * Provides minimal camera interface - derived classes implement specific navigation.
 */
UCLASS(Abstract)
class ARCHVIS_API AArchVisPawnBase : public APawn
{
	GENERATED_BODY()

public:
	AArchVisPawnBase();

	virtual void Tick(float DeltaTime) override;

	// --- Camera Control Interface (pure virtual - must be implemented by derived classes) ---
	
	// Zoom in/out
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void Zoom(float Amount) PURE_VIRTUAL(AArchVisPawnBase::Zoom, );

	// Pan the camera
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void Pan(FVector2D Delta) PURE_VIRTUAL(AArchVisPawnBase::Pan, );

	// Orbit around a point (3D modes only)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void Orbit(FVector2D Delta) {}

	// Reset camera to default position
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void ResetView() PURE_VIRTUAL(AArchVisPawnBase::ResetView, );

	// Focus on a world location
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void FocusOnLocation(FVector WorldLocation) PURE_VIRTUAL(AArchVisPawnBase::FocusOnLocation, );

	// Get the pawn type
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	EArchVisPawnType GetPawnType() const { return PawnType; }

	// Get the camera component
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual UCameraComponent* GetCameraComponent() const PURE_VIRTUAL(AArchVisPawnBase::GetCameraComponent, return nullptr;);

	// Get current camera world location
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual FVector GetCameraLocation() const PURE_VIRTUAL(AArchVisPawnBase::GetCameraLocation, return FVector::ZeroVector;);

	// Get current camera world rotation
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual FRotator GetCameraRotation() const PURE_VIRTUAL(AArchVisPawnBase::GetCameraRotation, return FRotator::ZeroRotator;);

	// Set camera transform (used when switching pawns to preserve view)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual void SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel) PURE_VIRTUAL(AArchVisPawnBase::SetCameraTransform, );

	// Get current zoom level
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	virtual float GetZoomLevel() const PURE_VIRTUAL(AArchVisPawnBase::GetZoomLevel, return 0.0f;);

protected:
	virtual void BeginPlay() override;

	// The type of this pawn
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ArchVis|Pawn")
	EArchVisPawnType PawnType = EArchVisPawnType::Drafting2D;

	// Root scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USceneComponent> RootScene;

	// --- Common Camera Settings ---
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float InterpSpeed = 10.0f;
};

