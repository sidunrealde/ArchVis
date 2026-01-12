#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchVisCameraController.generated.h"

class UCameraComponent;
class USpringArmComponent;

UENUM(BlueprintType)
enum class EArchVisViewMode : uint8
{
	TopDown2D,
	Perspective3D
};

/**
 * Manages the camera for the drafting experience.
 * Supports switching between 2D Ortho and 3D Perspective.
 */
UCLASS()
class ARCHVIS_API AArchVisCameraController : public AActor
{
	GENERATED_BODY()
	
public:	
	AArchVisCameraController();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	void SetViewMode(EArchVisViewMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	void ToggleViewMode();

	// Input handlers for Pan/Zoom/Orbit
	void Zoom(float Amount);
	void Pan(FVector2D Delta);
	void Orbit(FVector2D Delta);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	EArchVisViewMode CurrentViewMode = EArchVisViewMode::TopDown2D;

	// Settings
	float MinZoom = 500.0f;
	float MaxZoom = 5000.0f;
	float ZoomSpeed = 100.0f;
	float PanSpeed = 2.0f;
	float OrbitSpeed = 2.0f;

	// State
	float TargetArmLength;
	FRotator TargetRotation;
	FVector TargetLocation;
};
