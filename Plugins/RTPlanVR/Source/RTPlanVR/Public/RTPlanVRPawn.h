#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "RTPlanInputData.h"
#include "RTPlanVRPawn.generated.h"

class UCameraComponent;
class UMotionControllerComponent;
class UWidgetInteractionComponent;

/**
 * VR Pawn supporting Walk and Tabletop modes.
 */
UCLASS()
class RTPLANVR_API ARTPlanVRPawn : public APawn
{
	GENERATED_BODY()

public:
	ARTPlanVRPawn();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Helper to get pointer event from a controller
	UFUNCTION(BlueprintCallable, Category = "RTPlan|VR")
	FRTPointerEvent GetPointerEvent(ERTInputSource Source) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	TObjectPtr<USceneComponent> VROrigin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	TObjectPtr<UMotionControllerComponent> LeftController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	TObjectPtr<UMotionControllerComponent> RightController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	TObjectPtr<UWidgetInteractionComponent> LeftInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR")
	TObjectPtr<UWidgetInteractionComponent> RightInteraction;

	// Input Actions (Enhanced Input would be better, using legacy binding for V1 simplicity or stub)
	void OnRightTriggerPressed();
	void OnRightTriggerReleased();

	bool bRightTriggerDown = false;
};
