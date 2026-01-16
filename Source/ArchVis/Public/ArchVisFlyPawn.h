// ArchVisFlyPawn.h
// Free-flying pawn with no collision for exploring the scene

#pragma once

#include "CoreMinimal.h"
#include "ArchVisPawnBase.h"
#include "ArchVisFlyPawn.generated.h"

class UCameraComponent;

/**
 * Free-flying pawn for scene exploration.
 * Similar to Unreal's default pawn - WASD + mouse look, no collision.
 * Has its own camera (no spring arm).
 */
UCLASS()
class ARCHVIS_API AArchVisFlyPawn : public AArchVisPawnBase
{
	GENERATED_BODY()

public:
	AArchVisFlyPawn();

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

	// Look around (mouse look)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void Look(FVector2D Delta);

	// Move in world space (WASD style)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void Move(FVector Direction);

	// Set movement input (for continuous movement from input)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void SetMovementInput(FVector2D Input);

	// Set vertical movement input (Q/E for up/down)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void SetVerticalInput(float Input);

	// Set speed boost state
	void SetSpeedBoost(bool bBoost) { bSpeedBoost = bBoost; }

protected:
	virtual void BeginPlay() override;

	// --- Components ---
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> Camera;

	// --- Movement Settings ---
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FlySpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FastFlyMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LookSensitivity = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float PanSpeed = 10.0f;

	// --- State ---
	
	bool bSpeedBoost = false;
	FVector2D CurrentMovementInput = FVector2D::ZeroVector;
	float CurrentVerticalInput = 0.0f;
	
	// Target camera state (for smooth interpolation)
	FVector TargetLocation = FVector::ZeroVector;
	FRotator TargetRotation = FRotator::ZeroRotator;
};

