// ArchVisFlyPawn.h
// Free-flying pawn with no collision for exploring the scene

#pragma once

#include "CoreMinimal.h"
#include "ArchVisPawnBase.h"
#include "ArchVisFlyPawn.generated.h"

/**
 * Free-flying pawn for scene exploration.
 * Similar to Unreal's default pawn - WASD + mouse look, no collision.
 */
UCLASS()
class ARCHVIS_API AArchVisFlyPawn : public AArchVisPawnBase
{
	GENERATED_BODY()

public:
	AArchVisFlyPawn();

	virtual void Tick(float DeltaTime) override;

	// Override base pawn methods
	virtual void Zoom(float Amount) override;
	virtual void Pan(FVector2D Delta) override;
	virtual void Orbit(FVector2D Delta) override;
	virtual void ResetView() override;

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

protected:
	virtual void BeginPlay() override;

	// Fly speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FlySpeed = 1000.0f;

	// Fast fly speed multiplier (when holding shift)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float FastFlyMultiplier = 3.0f;

	// Look sensitivity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float LookSensitivity = 1.0f;

	// Speed boost state (shift held)
	bool bSpeedBoost = false;

	// Current movement input
	FVector2D CurrentMovementInput = FVector2D::ZeroVector;
	float CurrentVerticalInput = 0.0f;

	// Current look rotation
	FRotator CurrentRotation = FRotator::ZeroRotator;

public:
	// Set speed boost state
	void SetSpeedBoost(bool bBoost) { bSpeedBoost = bBoost; }
};

