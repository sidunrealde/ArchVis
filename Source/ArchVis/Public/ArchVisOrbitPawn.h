// ArchVisOrbitPawn.h
// 3D Orbit pawn with perspective camera and orbit controls

#pragma once

#include "CoreMinimal.h"
#include "ArchVisPawnBase.h"
#include "ArchVisOrbitPawn.generated.h"

/**
 * 3D Orbit Pawn with perspective view.
 * Supports Unreal Editor-style navigation:
 * - Alt + LMB: Orbit around target
 * - MMB: Pan
 * - Alt + RMB: Zoom (dolly)
 * - RMB + WASD/QE: Fly movement with mouselook
 * - Scroll: Zoom
 */
UCLASS()
class ARCHVIS_API AArchVisOrbitPawn : public AArchVisPawnBase
{
	GENERATED_BODY()

public:
	AArchVisOrbitPawn();

	virtual void Tick(float DeltaTime) override;

	virtual void Zoom(float Amount) override;
	virtual void Pan(FVector2D Delta) override;
	virtual void Orbit(FVector2D Delta) override;
	virtual void ResetView() override;
	virtual void SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel) override;

	// --- Fly-style Movement (RMB held) ---
	
	// Set WASD movement input
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Movement")
	void SetMovementInput(FVector2D Input);

	// Set vertical movement input (Q/E)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Movement")
	void SetVerticalInput(float Input);

	// Look around (when RMB held)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Movement")
	void Look(FVector2D Delta);

	// Alt+RMB zoom (dolly along view direction)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Movement")
	void DollyZoom(float Amount);

	// Focus on a world location
	virtual void FocusOnLocation(FVector WorldLocation) override;

	// Set fly mode active (RMB held)
	void SetFlyModeActive(bool bActive);
	bool IsFlyModeActive() const { return bFlyModeActive; }

	// Set speed boost (Shift held)
	void SetSpeedBoost(bool bBoost) { bSpeedBoost = bBoost; }

protected:
	virtual void BeginPlay() override;

	// Pitch limits for orbit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float MinPitch = -85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float MaxPitch = 85.0f;

	// Default orbit angle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float DefaultPitch = -45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|3D")
	float DefaultYaw = 0.0f;

	// --- Fly Movement Settings ---
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FlySpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FastFlyMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LookSensitivity = 0.5f;

	// --- State ---
	
	bool bFlyModeActive = false;
	bool bSpeedBoost = false;
	FVector2D CurrentMovementInput = FVector2D::ZeroVector;
	float CurrentVerticalInput = 0.0f;
	float SavedArmLength = 5000.0f;  // Saved arm length when entering fly mode
};

