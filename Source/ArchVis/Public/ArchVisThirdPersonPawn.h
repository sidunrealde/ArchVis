// ArchVisThirdPersonPawn.h
// Third-person walkthrough character

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArchVisPawnBase.h"
#include "ArchVisThirdPersonPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * Third-person character for walkthrough mode.
 * Uses standard TPS controls with camera boom.
 */
UCLASS()
class ARCHVIS_API AArchVisThirdPersonPawn : public ACharacter
{
	GENERATED_BODY()

public:
	AArchVisThirdPersonPawn();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Get the pawn type (for compatibility with pawn switching system)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	EArchVisPawnType GetPawnType() const { return EArchVisPawnType::ThirdPerson; }

	// Get the camera component
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	UCameraComponent* GetCameraComponent() const { return ThirdPersonCamera; }

	// Look/orbit input
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void Look(FVector2D Delta);

	// Move input
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void Move(FVector2D Direction);

	// Zoom input
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void Zoom(float Amount);

	// Set initial position and rotation
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Camera")
	void SetInitialTransform(FVector Location, FRotator Rotation);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> ThirdPersonCamera;

	// Look sensitivity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float LookSensitivity = 1.0f;

	// Walk speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 400.0f;

	// Camera boom settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float DefaultBoomLength = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MinBoomLength = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MaxBoomLength = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float ZoomSpeed = 50.0f;

	float TargetBoomLength;
};

