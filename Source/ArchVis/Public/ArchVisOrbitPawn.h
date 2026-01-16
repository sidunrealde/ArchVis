// ArchVisOrbitPawn.h
// 3D Orbit pawn with perspective camera - direct camera movement (no spring arm)

#pragma once

#include "CoreMinimal.h"
#include "ArchVisPawnBase.h"
#include "ArchVisOrbitPawn.generated.h"

class UCameraComponent;
class UOrbitInputComponent;
class UArchVisInputConfig;

/**
 * 3D Orbit Pawn with perspective view.
 * Camera moves directly in 3D space (no spring arm).
 *
 * Supports Unreal Editor-style navigation:
 * - RMB + Mouse: Look around
 * - RMB + WASD/QE: Fly movement
 * - MMB + Mouse: Pan camera
 * - Alt + LMB + Mouse: Orbit around focus point
 * - Alt + RMB + Mouse Y: Dolly zoom
 * - Scroll: Zoom in/out
 * - Shift: Speed boost
 */
UCLASS()
class ARCHVIS_API AArchVisOrbitPawn : public AArchVisPawnBase
{
	GENERATED_BODY()

public:
	AArchVisOrbitPawn();

	virtual void Tick(float DeltaTime) override;

	// --- Camera Interface Implementation ---
	virtual void Zoom(float Amount) override;
	virtual void Pan(FVector2D Delta) override;
	virtual void Orbit(FVector2D Delta) override;
	virtual void ResetView() override;
	virtual void FocusOnLocation(FVector WorldLocation) override;
	virtual void SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel) override;
	virtual float GetZoomLevel() const override;
	virtual UCameraComponent* GetCameraComponent() const override { return Camera; }
	virtual FVector GetCameraLocation() const override;
	virtual FRotator GetCameraRotation() const override;

	// Initialize input with config
	void InitializeInput(UArchVisInputConfig* InputConfig);

	// --- Fly-style Movement ---
	
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Movement")
	void SetMovementInput(FVector2D Input);

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Movement")
	void SetVerticalInput(float Input);

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Movement")
	void Look(FVector2D Delta);

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Movement")
	void DollyZoom(float Amount);

	// Mode control
	void SetFlyModeActive(bool bActive);
	bool IsFlyModeActive() const { return bFlyModeActive; }

	void SetOrbitModeActive(bool bActive);
	bool IsOrbitModeActive() const { return bOrbitModeActive; }

	void SetDollyModeActive(bool bActive);
	bool IsDollyModeActive() const { return bDollyModeActive; }

	void SetSpeedBoost(bool bBoost) { bSpeedBoost = bBoost; }

	// Adjust fly speed (e.g., with scroll wheel while RMB held)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Movement")
	void AdjustFlySpeed(float Delta);

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Movement")
	float GetFlySpeed() const { return FlySpeed; }

	// Debug
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Debug")
	void SetDebugEnabled(bool bEnabled) { bDebugEnabled = bEnabled; }

	UFUNCTION(Exec)
	void ArchVisOrbitDebug();

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	// Interpolate camera state smoothly
	void InterpolateCameraState(float DeltaTime);

	// --- Components ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// --- Camera Settings ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float MinPitch = -85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float MaxPitch = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float DefaultPitch = -30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float DefaultYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float DefaultDistance = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float ZoomSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float PanSpeed = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float OrbitSpeed = 0.5f;

	// --- Fly Movement Settings ---
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FlySpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MinFlySpeed = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float MaxFlySpeed = 10000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FlySpeedAdjustMultiplier = 1.2f;  // Multiply/divide speed by this amount per scroll tick

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FastFlyMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LookSensitivity = 0.3f;

	// --- Debug ---
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDebugEnabled = false;

	// --- Input ---

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UOrbitInputComponent> OrbitInput;

	// --- State ---
	
	// Target camera state (interpolated to)
	FVector TargetCameraLocation = FVector::ZeroVector;
	FRotator TargetCameraRotation = FRotator::ZeroRotator;

	// Focus point for orbit mode (world location that camera orbits around)
	FVector OrbitFocusPoint = FVector::ZeroVector;
	float OrbitDistance = 5000.0f;

	// Mode flags
	bool bFlyModeActive = false;
	bool bOrbitModeActive = false;
	bool bDollyModeActive = false;
	bool bSpeedBoost = false;

	// Movement input
	FVector2D CurrentMovementInput = FVector2D::ZeroVector;
	float CurrentVerticalInput = 0.0f;
};

