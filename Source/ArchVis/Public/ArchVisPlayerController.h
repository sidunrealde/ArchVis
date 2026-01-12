#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTPlanInputData.h"
#include "InputActionValue.h"
#include "ArchVisPlayerController.generated.h"

class AArchVisCameraController;
class UArchVisInputConfig;
class UEnhancedInputLocalPlayerSubsystem;

/**
 * Controller that captures mouse input for the drafting tools using Enhanced Input.
 * Manages a Virtual Cursor for CAD-style interaction.
 */
UCLASS()
class ARCHVIS_API AArchVisPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AArchVisPlayerController();

	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	// Get the current screen position of the virtual cursor
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	FVector2D GetVirtualCursorPos() const { return VirtualCursorPos; }

protected:
	// Input Config Data Asset (Assign in BP)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UArchVisInputConfig> InputConfig;

	// Input Handlers
	void OnLeftClick(const FInputActionValue& Value);
	void OnRightClick(const FInputActionValue& Value);
	void OnMouseMove(const FInputActionValue& Value);
	void OnMouseWheel(const FInputActionValue& Value);
	void OnToggleView(const FInputActionValue& Value);

	// Helper to get the unified pointer event
	FRTPointerEvent GetPointerEvent(ERTPointerAction Action) const;

	bool bLeftMouseDown = false;
	bool bRightMouseDown = false;

	// Virtual Cursor State
	FVector2D VirtualCursorPos;

	UPROPERTY(Transient)
	TObjectPtr<AArchVisCameraController> CameraController;
};
