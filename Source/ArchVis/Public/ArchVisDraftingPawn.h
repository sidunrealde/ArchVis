// ArchVisDraftingPawn.h
// 2D Top-down drafting pawn with orthographic camera and spring arm

#pragma once

#include "CoreMinimal.h"
#include "ArchVisPawnBase.h"
#include "ArchVisDraftingPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UDraftingInputComponent;
class UArchVisInputConfig;

/**
 * 2D Drafting Pawn with top-down orthographic view.
 * Uses spring arm for zoom/position control.
 * Primary pawn for CAD-style wall editing.
 * 
 * Navigation:
 * - MMB: Pan
 * - Scroll: Zoom
 */
UCLASS()
class ARCHVIS_API AArchVisDraftingPawn : public AArchVisPawnBase
{
	GENERATED_BODY()

public:
	AArchVisDraftingPawn();

	virtual void Tick(float DeltaTime) override;

	// --- Camera Interface Implementation ---
	virtual void Zoom(float Amount) override;
	virtual void Pan(FVector2D Delta) override;
	virtual void ResetView() override;
	virtual void FocusOnLocation(FVector WorldLocation) override;
	virtual void SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel) override;
	virtual float GetZoomLevel() const override;
	virtual UCameraComponent* GetCameraComponent() const override { return Camera; }
	virtual FVector GetCameraLocation() const override;
	virtual FRotator GetCameraRotation() const override;

	// Initialize input with config
	void InitializeInput(UArchVisInputConfig* InputConfig);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	// Update ortho width based on zoom level
	void UpdateOrthoWidth();

	// Interpolate camera state smoothly
	void InterpolateCameraState(float DeltaTime);

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// --- Camera Settings ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|2D")
	float MinZoom = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|2D")
	float MaxZoom = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|2D")
	float ZoomSpeed = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|2D")
	float PanSpeed = 1.0f;

	// Ortho width multiplier (ortho width = arm length * this value)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|2D")
	float OrthoWidthMultiplier = 2.0f;

	// --- Target State (for smooth interpolation) ---

	float TargetArmLength = 5000.0f;
	FVector TargetLocation = FVector::ZeroVector;

	// Input component for 2D navigation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UDraftingInputComponent> DraftingInput;
};

