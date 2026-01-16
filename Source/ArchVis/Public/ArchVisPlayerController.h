#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTPlanInputData.h"
#include "RTPlanToolManager.h"
#include "InputActionValue.h"
#include "ArchVisPawnBase.h"
#include "ArchVisPlayerController.generated.h"

class AArchVisCameraController;
class AArchVisPawnBase;
class AArchVisDraftingPawn;
class AArchVisOrbitPawn;
class AArchVisFlyPawn;
class AArchVisFirstPersonPawn;
class AArchVisThirdPersonPawn;
class UArchVisInputConfig;
class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;

/**
 * How the snap/constraint modifier key behaves.
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
 * Current view/interaction mode.
 */
UENUM(BlueprintType)
enum class EArchVisInteractionMode : uint8
{
	Drafting2D      UMETA(DisplayName = "2D Drafting"),
	Navigation3D    UMETA(DisplayName = "3D Navigation")
};

/**
 * Current 2D tool mode.
 */
UENUM(BlueprintType)
enum class EArchVis2DToolMode : uint8
{
	None,
	Selection,
	LineTool,
	PolylineTool
};

/**
 * Controller that captures mouse input for the drafting tools using Enhanced Input.
 * Manages Input Mapping Context switching based on tool and mode.
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

	// --- Input Context Management ---
	
	// Switch to a different interaction mode (2D/3D)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void SetInteractionMode(EArchVisInteractionMode NewMode);

	// Get current interaction mode
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	EArchVisInteractionMode GetInteractionMode() const { return CurrentInteractionMode; }

	// Notify the controller that the active tool has changed (updates IMC)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void OnToolChanged(ERTPlanToolType NewToolType);

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

	// --- Pawn Management ---

	// Switch to a different pawn type
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	void SwitchToPawnType(EArchVisPawnType NewPawnType);

	// Get current pawn type
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	EArchVisPawnType GetCurrentPawnType() const { return CurrentPawnType; }

	// Toggle between 2D and 3D pawns
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	void ToggleViewPawn();

	// Get the current ArchVis pawn (cast helper)
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	AArchVisPawnBase* GetArchVisPawn() const;

	/** Toggle verbose input/navigation debug logging at runtime. */
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Debug")
	void SetInputDebugEnabled(bool bEnabled) { bInputDebugEnabled = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Debug")
	bool IsInputDebugEnabled() const { return bInputDebugEnabled; }

	/** Toggle input debug logging at runtime. */
	UFUNCTION(Exec)
	void ArchVisToggleInputDebug();

	/** Set input debug logging explicitly (0/1). */
	UFUNCTION(Exec)
	void ArchVisSetInputDebug(int32 bEnabled);

protected:
	// --- Pawn Classes (Assign in BP or set defaults) ---
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pawn")
	TSubclassOf<AArchVisDraftingPawn> DraftingPawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pawn")
	TSubclassOf<AArchVisOrbitPawn> OrbitPawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pawn")
	TSubclassOf<AArchVisFlyPawn> FlyPawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pawn")
	TSubclassOf<AArchVisFirstPersonPawn> FirstPersonPawnClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pawn")
	TSubclassOf<AArchVisThirdPersonPawn> ThirdPersonPawnClass;

	// Current pawn type
	EArchVisPawnType CurrentPawnType = EArchVisPawnType::Drafting2D;

	// Spawn and possess a pawn of the given type
	APawn* SpawnArchVisPawn(EArchVisPawnType PawnType, FVector Location, FRotator Rotation);

protected:
	// Input Config Data Asset (Assign in BP)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UArchVisInputConfig> InputConfig;

	// Mouse sensitivity for virtual cursor movement.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "10.0"))
	float MouseSensitivity = 2.0f;

	// Default unit for length input
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	ERTLengthUnit DefaultUnit = ERTLengthUnit::Centimeters;

	// --- Snap Settings ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snap")
	ESnapModifierMode SnapModifierMode = ESnapModifierMode::HoldToDisable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snap")
	bool bSnapEnabledByDefault = true;

	// --- Input Context Priorities (Higher = Processed First) ---
	static constexpr int32 IMC_Priority_Global = 0;
	static constexpr int32 IMC_Priority_ModeBase = 1;
	static constexpr int32 IMC_Priority_Tool = 2;
	static constexpr int32 IMC_Priority_NumericEntry = 3;

	// --- Global Input Handlers ---
	void OnUndo(const FInputActionValue& Value);
	void OnRedo(const FInputActionValue& Value);
	void OnDelete(const FInputActionValue& Value);
	void OnEscape(const FInputActionValue& Value);
	void OnToggleView(const FInputActionValue& Value);
	void OnSave(const FInputActionValue& Value);

	// Modifier handlers
	void OnModifierCtrlStarted(const FInputActionValue& Value);
	void OnModifierCtrlCompleted(const FInputActionValue& Value);
	void OnModifierShiftStarted(const FInputActionValue& Value);
	void OnModifierShiftCompleted(const FInputActionValue& Value);
	void OnModifierAltStarted(const FInputActionValue& Value);
	void OnModifierAltCompleted(const FInputActionValue& Value);

	// --- View Input Handlers ---
	void OnPan(const FInputActionValue& Value);
	void OnPanDelta(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnOrbit(const FInputActionValue& Value);
	void OnOrbitDelta(const FInputActionValue& Value);
	void OnFlyMode(const FInputActionValue& Value);
	void OnDollyZoom(const FInputActionValue& Value);
	void OnResetView(const FInputActionValue& Value);
	void OnFocusSelection(const FInputActionValue& Value);
	void OnPointerPosition(const FInputActionValue& Value);
	void OnSnapToggle(const FInputActionValue& Value);
	void OnGridToggle(const FInputActionValue& Value);

	// --- Fly Pawn Movement Handlers ---
	void OnMove(const FInputActionValue& Value);
	void OnMoveUp(const FInputActionValue& Value);
	void OnMoveDown(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);

	// --- Selection Input Handlers ---
	void OnSelectStarted(const FInputActionValue& Value);
	void OnSelectCompleted(const FInputActionValue& Value);
	void OnSelectAdd(const FInputActionValue& Value);
	void OnSelectToggle(const FInputActionValue& Value);
	void OnSelectAll(const FInputActionValue& Value);
	void OnDeselectAll(const FInputActionValue& Value);
	void OnBoxSelectStart(const FInputActionValue& Value);
	void OnBoxSelectDrag(const FInputActionValue& Value);
	void OnBoxSelectEnd(const FInputActionValue& Value);
	void OnCycleSelection(const FInputActionValue& Value);

	// --- Drawing Input Handlers ---
	void OnDrawPlacePoint(const FInputActionValue& Value);
	void OnDrawConfirm(const FInputActionValue& Value);
	void OnDrawCancel(const FInputActionValue& Value);
	void OnDrawClose(const FInputActionValue& Value);
	void OnDrawRemoveLastPoint(const FInputActionValue& Value);
	void OnOrthoLock(const FInputActionValue& Value);
	void OnOrthoLockReleased(const FInputActionValue& Value);
	void OnAngleSnap(const FInputActionValue& Value);

	// --- Numeric Entry Handlers ---
	void OnNumericDigit(int32 Digit);
	void OnNumericDigitInput(const FInputActionValue& Value);  // Single handler for 1D Axis digit input
	void OnNumericDecimal(const FInputActionValue& Value);
	void OnNumericBackspaceStarted(const FInputActionValue& Value);
	void OnNumericBackspaceCompleted(const FInputActionValue& Value);
	void OnNumericCommit(const FInputActionValue& Value);
	void OnNumericCancel(const FInputActionValue& Value);
	void OnNumericSwitchField(const FInputActionValue& Value);
	void OnNumericCycleUnits(const FInputActionValue& Value);
	void OnNumericAdd(const FInputActionValue& Value);
	void OnNumericSubtract(const FInputActionValue& Value);
	void OnNumericMultiply(const FInputActionValue& Value);
	void OnNumericDivide(const FInputActionValue& Value);
	
	// Perform one backspace operation
	void PerformBackspace();

	// --- 3D Navigation Handlers ---
	void OnViewTop(const FInputActionValue& Value);
	void OnViewFront(const FInputActionValue& Value);
	void OnViewRight(const FInputActionValue& Value);
	void OnViewPerspective(const FInputActionValue& Value);

	// Tool switching handlers
	void OnToolSelectHotkey(const FInputActionValue& Value);
	void OnToolLineHotkey(const FInputActionValue& Value);
	void OnToolPolylineHotkey(const FInputActionValue& Value);

	// Helper to get the unified pointer event
	FRTPointerEvent GetPointerEvent(ERTPointerAction Action) const;

	// Commit the current numeric buffer to the active tool
	void CommitNumericInput();

	// Update the active tool's preview based on current buffer value
	void UpdateToolPreviewFromBuffer();

	// Update active Input Mapping Contexts based on mode and tool
	void UpdateInputMappingContexts();

	// Switch to a specific 2D tool mode
	void SwitchTo2DToolMode(EArchVis2DToolMode NewToolMode);

	// Activate numeric entry context (layered on top)
	void ActivateNumericEntryContext();

	// Deactivate numeric entry context
	void DeactivateNumericEntryContext();

	// Helper to add/remove mapping contexts
	void AddMappingContext(UInputMappingContext* Context, int32 Priority);
	void RemoveMappingContext(UInputMappingContext* Context);
	void ClearAllToolContexts();

	// State
	bool bPanActive = false;
	bool bOrbitActive = false;
	bool bBoxSelectActive = false;
	FVector2D BoxSelectStart;
	FVector2D VirtualCursorPos;
	FRTNumericInputBuffer NumericInputBuffer;

	// Modifier states
	bool bShiftDown = false;
	bool bAltDown = false;
	bool bCtrlDown = false;

	// Drafting constraint states (used by tools via FRTPointerEvent)
	bool bOrthoLockActive = false;
	bool bAngleSnapEnabled = false;

	/** Raw mouse button states (tracked via selection/pan/fly handlers). */
	bool bLMBDown = false;
	bool bRMBDown = false;
	bool bMMBDown = false;

	// Snap state
	bool bSnapToggledOn = true;
	bool bSnapModifierHeld = false;
	bool bGridVisible = true;

	// Mouse tracking for pan
	FVector2D LastMousePosition = FVector2D::ZeroVector;

	// WASD movement state (for 3D fly mode)
	FVector2D CurrentMoveInput = FVector2D::ZeroVector;
	float CurrentVerticalMoveInput = 0.0f;

	// Backspace repeat state
	bool bBackspaceHeld = false;
	float BackspaceHoldTime = 0.0f;
	float BackspaceRepeatDelay = 0.4f;  // Initial delay before repeat starts
	float BackspaceRepeatRate = 0.05f;  // Time between repeats
	float BackspaceNextRepeatTime = 0.0f;

	// Current states
	EArchVisInteractionMode CurrentInteractionMode = EArchVisInteractionMode::Drafting2D;
	EArchVis2DToolMode Current2DToolMode = EArchVis2DToolMode::Selection;
	ERTPlanToolType CurrentToolType = ERTPlanToolType::None;
	bool bNumericEntryContextActive = false;

	// Currently active tool-specific IMC (to track for removal)
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveToolIMC;

	// Currently active mode base IMC (to track for removal)
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveModeBaseIMC;

	UPROPERTY(Transient)
	TObjectPtr<AArchVisCameraController> CameraController;

	/** When enabled, logs a small amount of debug output for navigation input. */
	UPROPERTY(EditAnywhere, Category = "ArchVis|Debug")
	bool bInputDebugEnabled = false;

	/**
	 * Unified “Unreal Editor-style” navigation state for the 3D orbit pawn.
	 * This is derived from modifier + mouse buttons.
	 */
	struct FNav3DState
	{
		bool bPan = false;     // MMB
		bool bOrbit = false;   // Alt + LMB
		bool bFly = false;     // RMB
		bool bDolly = false;   // Alt + RMB
	};

	FNav3DState GetNav3DState() const;

	/** Emits a quick snapshot of current mode + button/modifier states. */
	void DebugLogInputSnapshot(const TCHAR* Context, const FVector2D& MouseDelta = FVector2D::ZeroVector) const;

	/** Emits debug logs for nav state changes when enabled. */
	void DebugLogNavState(const TCHAR* Context, const FVector2D& MouseDelta) const;
};
