// ArchVisFirstPersonPawn.h
// First-person walkthrough character

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArchVisPawnBase.h"
#include "ArchVisFirstPersonPawn.generated.h"

class UCameraComponent;

/**
 * First-person character for walkthrough mode.
 * Uses standard FPS controls for navigation.
 */
UCLASS()
class ARCHVIS_API AArchVisFirstPersonPawn : public ACharacter
{
	GENERATED_BODY()

public:
	AArchVisFirstPersonPawn();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Get the pawn type (for compatibility with pawn switching system)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	EArchVisPawnType GetPawnType() const { return EArchVisPawnType::FirstPerson; }

	// Get the camera component
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	UCameraComponent* GetCameraComponent() const { return FirstPersonCamera; }

	// Look input
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void Look(FVector2D Delta);

	// Move input
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void Move(FVector2D Direction);

	// Set initial position and rotation
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	void SetInitialTransform(FVector Location, FRotator Rotation);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// Look sensitivity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float LookSensitivity = 1.0f;

	// Walk speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 400.0f;

	// Eye height offset from capsule center
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float EyeHeightOffset = 64.0f;
};

