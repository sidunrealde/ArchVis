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
 * How the snap modifier key behaves.
 */
UENUM(BlueprintType)
enum class ESnapModifierMode : uint8
{
	// Press to toggle snap on/off
	Toggle,
	// Hold to temporarily disable snap (snap is on by default)
	HoldToDisable,
	// Hold to temporarily enable snap (snap is off by default)
	HoldToEnable
};

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

	// Get the numeric input buffer (for HUD display)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	const FRTNumericInputBuffer& GetNumericInputBuffer() const { return NumericInputBuffer; }

	// Get current length unit setting
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	ERTLengthUnit GetCurrentUnit() const { return NumericInputBuffer.CurrentUnit; }

	// Set current length unit
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void SetCurrentUnit(ERTLengthUnit NewUnit) { NumericInputBuffer.CurrentUnit = NewUnit; }

	// Get whether snap is currently enabled (accounts for modifier key state)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Snap")
	bool IsSnapEnabled() const;

protected:
	// Input Config Data Asset (Assign in BP)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UArchVisInputConfig> InputConfig;

	// Mouse sensitivity for virtual cursor movement.
	// 1.0 = default. Exposed so a settings system can drive it later.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "10.0"))
	float MouseSensitivity = 2.0f;

	// Default unit for length input (can be changed at runtime via settings)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	ERTLengthUnit DefaultUnit = ERTLengthUnit::Centimeters;

	// --- Snap Settings ---
	// How the snap modifier key behaves
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snap")
	ESnapModifierMode SnapModifierMode = ESnapModifierMode::HoldToDisable;

	// Whether snap is enabled by default (used with Toggle and HoldToDisable modes)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snap")
	bool bSnapEnabledByDefault = true;

	// --- Mouse/View Input Handlers ---
	void OnLeftClick(const FInputActionValue& Value);
	void OnRightClick(const FInputActionValue& Value);
	void OnMouseMove(const FInputActionValue& Value);
	void OnMouseWheel(const FInputActionValue& Value);
	void OnToggleView(const FInputActionValue& Value);

	// --- Numeric Entry Handlers ---
	void OnNumericDigit(const FInputActionValue& Value);
	void OnNumericDecimal(const FInputActionValue& Value);
	void OnNumericBackspace(const FInputActionValue& Value);
	void OnNumericCommit(const FInputActionValue& Value);
	void OnNumericClear(const FInputActionValue& Value);
	void OnNumericSwitchField(const FInputActionValue& Value);
	void OnCycleUnits(const FInputActionValue& Value);

	// --- Snap Modifier Handlers ---
	void OnSnapModifierStarted(const FInputActionValue& Value);
	void OnSnapModifierCompleted(const FInputActionValue& Value);

	// Helper to get the unified pointer event
	FRTPointerEvent GetPointerEvent(ERTPointerAction Action) const;

	// Commit the current numeric buffer to the active tool
	void CommitNumericInput();

	// Update the active tool's preview based on current buffer value (for real-time visualization)
	void UpdateToolPreviewFromBuffer();

	bool bLeftMouseDown = false;
	bool bRightMouseDown = false;

	// Virtual Cursor State
	FVector2D VirtualCursorPos;

	// Numeric Input Buffer for CAD-style entry
	FRTNumericInputBuffer NumericInputBuffer;

	// Snap state
	bool bSnapToggledOn = true; // For Toggle mode
	bool bSnapModifierHeld = false; // For Hold modes

	UPROPERTY(Transient)
	TObjectPtr<AArchVisCameraController> CameraController;
};
