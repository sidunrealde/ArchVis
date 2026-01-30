#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTPlanInputData.h"
#include "RTPlanToolManager.h"
#include "InputActionValue.h"
#include "ArchVisPawnBase.h"
#include "ArchVisPlayerController.generated.h"

class AArchVisPawnBase;
class AArchVisDraftingPawn;
class AArchVisOrbitPawn;
class AArchVisFlyPawn;
class AArchVisFirstPersonPawn;
class AArchVisThirdPersonPawn;
class UArchVisInputConfig;
class UEnhancedInputLocalPlayerSubsystem;
class UInputMappingContext;
class UToolInputComponent;
class URTPlanWallPropertiesWidget;
class URTFinishCatalog;

/**
 * How the snap/constraint modifier key behaves.
 */
UENUM(BlueprintType)
enum class ESnapModifierMode : uint8
{
	Toggle,
	HoldToDisable,
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
 * Current 2D Drafting Mode (Wall, Floor, Ceiling).
 */
UENUM(BlueprintType)
enum class EDraftingMode : uint8
{
	Wall,
	Floor,
	Ceiling
};

/**
 * Current 2D tool mode.
 */
UENUM(BlueprintType)
enum class EArchVis2DToolMode : uint8
{
	None,
	Selection,
	// Wall Tools
	LineTool,
	PolylineTool,
	ArcTool,
	TrimTool,
	// Floor Tools
	DrawFloor,
	NewFloorArea,
	ExtrudeFloor,
	ExtendFloorArea,
	EditExtrude,
	// Ceiling Tools
	DrawCeiling,
	CreateFalseCeiling,
	EditFalseCeiling
};

/**
 * Controller for ArchVis drafting application.
 * 
 * Responsibilities:
 * - Global actions (Undo/Redo/Delete/Escape/Save)
 * - Tool switching and management
 * - Numeric input buffer
 * - Virtual cursor for 2D mode
 * - Pawn switching coordination
 * 
 * Navigation input is handled by pawn-specific input components:
 * - UDraftingInputComponent for 2D pan/zoom
 * - UOrbitInputComponent for 3D orbit/pan/dolly/fly
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

	/** Called by GameMode when it has finished initializing (ToolManager, ShellActor, etc.) */
	void OnGameModeReady();

	// --- Input Context Management ---
	
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void SetInteractionMode(EArchVisInteractionMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	EArchVisInteractionMode GetInteractionMode() const { return CurrentInteractionMode; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void SetDraftingMode(EDraftingMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	EDraftingMode GetDraftingMode() const { return CurrentDraftingMode; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void OnToolChanged(ERTPlanToolType NewToolType);

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	FVector2D GetVirtualCursorPos() const { return VirtualCursorPos; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	const FRTNumericInputBuffer& GetNumericInputBuffer() const { return NumericInputBuffer; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	ERTLengthUnit GetCurrentUnit() const { return NumericInputBuffer.CurrentUnit; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Input")
	void SetCurrentUnit(ERTLengthUnit NewUnit) { NumericInputBuffer.CurrentUnit = NewUnit; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Snap")
	bool IsSnapEnabled() const;

	// --- Pawn Management ---

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	void SwitchToPawnType(EArchVisPawnType NewPawnType);

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	EArchVisPawnType GetCurrentPawnType() const { return CurrentPawnType; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	void ToggleViewPawn();

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Pawn")
	AArchVisPawnBase* GetArchVisPawn() const;

	// --- Debug ---
	
	UFUNCTION(BlueprintCallable, Category = "ArchVis|Debug")
	void SetInputDebugEnabled(bool bEnabled) { bInputDebugEnabled = bEnabled; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Debug")
	bool IsInputDebugEnabled() const { return bInputDebugEnabled; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Debug")
	void SetSelectionDebugEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "ArchVis|Debug")
	bool IsSelectionDebugEnabled() const { return bSelectionDebugEnabled; }

	UFUNCTION(Exec)
	void ArchVisToggleInputDebug();

	UFUNCTION(Exec)
	void ArchVisToggleSelectionDebug();

protected:
	// --- Pawn Classes ---
	
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

	EArchVisPawnType CurrentPawnType = EArchVisPawnType::Drafting2D;

	APawn* SpawnArchVisPawn(EArchVisPawnType PawnType, FVector Location, FRotator Rotation);

	// --- Input Config ---
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UArchVisInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "10.0"))
	float MouseSensitivity = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	ERTLengthUnit DefaultUnit = ERTLengthUnit::Centimeters;

	// --- Wall Properties Widget ---
	
	/** Widget class for wall properties panel. Set in Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|WallProperties")
	TSubclassOf<URTPlanWallPropertiesWidget> WallPropertiesWidgetClass;

	/** Finish catalog asset containing wall materials. Set in Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|WallProperties")
	TSoftObjectPtr<URTFinishCatalog> FinishCatalogAsset;

	/** Get the wall properties widget instance */
	UFUNCTION(BlueprintCallable, Category = "UI|WallProperties")
	URTPlanWallPropertiesWidget* GetWallPropertiesWidget() const { return WallPropertiesWidget; }

	/** Get the loaded finish catalog */
	UFUNCTION(BlueprintCallable, Category = "UI|WallProperties")
	URTFinishCatalog* GetFinishCatalog() const { return FinishCatalog; }

	// --- Snap Settings ---
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snap")
	ESnapModifierMode SnapModifierMode = ESnapModifierMode::HoldToDisable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Snap")
	bool bSnapEnabledByDefault = true;

	// --- IMC Priorities ---
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

	// --- Mode Switching Handlers ---
	void OnModeWall(const FInputActionValue& Value);
	void OnModeFloor(const FInputActionValue& Value);
	void OnModeCeiling(const FInputActionValue& Value);

	// --- Selection Input Handlers ---
	void OnSelectStarted(const FInputActionValue& Value);
	void OnSelectCompleted(const FInputActionValue& Value);
	void OnSelectAdd(const FInputActionValue& Value);
	void OnSelectAddCompleted(const FInputActionValue& Value);
	void OnSelectToggle(const FInputActionValue& Value);
	void OnSelectToggleCompleted(const FInputActionValue& Value);
	void OnSelectRemove(const FInputActionValue& Value);
	void OnSelectRemoveCompleted(const FInputActionValue& Value);
	void OnSelectAll(const FInputActionValue& Value);
	void OnDeselectAll(const FInputActionValue& Value);
	void OnBoxSelectStart(const FInputActionValue& Value);
	void OnBoxSelectDrag(const FInputActionValue& Value);
	void OnBoxSelectEnd(const FInputActionValue& Value);
	void OnCycleSelection(const FInputActionValue& Value);

	// Selection change handler (bound to SelectTool's OnSelectionChanged delegate)
	UFUNCTION()
	void HandleSelectionChanged();

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
	void OnNumericDigitInput(const FInputActionValue& Value);
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
	void PerformBackspace();

	// --- View/Grid Handlers ---
	void OnSnapToggle(const FInputActionValue& Value);
	void OnGridToggle(const FInputActionValue& Value);
	void OnViewTop(const FInputActionValue& Value);
	void OnViewFront(const FInputActionValue& Value);
	void OnViewRight(const FInputActionValue& Value);
	void OnViewPerspective(const FInputActionValue& Value);
	void OnFocusSelection(const FInputActionValue& Value);

	// Tool switching handlers
	void OnToolSelectHotkey(const FInputActionValue& Value);
	void OnToolLineHotkey(const FInputActionValue& Value);
	void OnToolPolylineHotkey(const FInputActionValue& Value);
	void OnToolArcHotkey(const FInputActionValue& Value);
	void OnToolTrimHotkey(const FInputActionValue& Value);

	// Floor Tool Handlers
	void OnToolDrawFloor(const FInputActionValue& Value);
	void OnToolNewFloorArea(const FInputActionValue& Value);
	void OnToolExtrudeFloor(const FInputActionValue& Value);
	void OnToolExtendFloorArea(const FInputActionValue& Value);
	void OnToolEditExtrude(const FInputActionValue& Value);

	// Ceiling Tool Handlers
	void OnToolDrawCeiling(const FInputActionValue& Value);
	void OnToolCreateFalseCeiling(const FInputActionValue& Value);
	void OnToolEditFalseCeiling(const FInputActionValue& Value);

	// --- Console Commands ---
	UFUNCTION(Exec)
	void ArchVisSetInputDebug(int32 bEnabled);

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

	// Activate/deactivate numeric entry context
	void ActivateNumericEntryContext();
	void DeactivateNumericEntryContext();

	// Helper to add/remove mapping contexts
	void AddMappingContext(UInputMappingContext* Context, int32 Priority);
	void RemoveMappingContext(UInputMappingContext* Context);
	void ClearAllToolContexts();

	// Update mouse lock state for 3D navigation (locks mouse when orbit is active)
	void UpdateMouseLockState();

	// --- State ---
	
	bool bBoxSelectActive = false;
	FVector2D BoxSelectStart;
	FVector2D VirtualCursorPos;
	FVector2D LastMousePosition = FVector2D::ZeroVector;
	FRTNumericInputBuffer NumericInputBuffer;

	// Modifier states
	bool bShiftDown = false;
	bool bAltDown = false;
	bool bCtrlDown = false;

	// Selection mode (set by which action triggered the selection)
	enum class ESelectionMode { Replace, Add, Toggle, Remove };
	ESelectionMode CurrentSelectionMode = ESelectionMode::Replace;

	// Action states (for selection/orbit interaction)
	bool bSelectActionActive = false;  // IA_Select is active
	bool bOrbitActive = false;

	// Drafting constraint states
	bool bOrthoLockActive = false;
	bool bAngleSnapEnabled = false;

	// Snap state
	bool bSnapToggledOn = true;
	bool bSnapModifierHeld = false;
	bool bGridVisible = true;

	// Backspace repeat state
	bool bBackspaceHeld = false;
	float BackspaceHoldTime = 0.0f;
	float BackspaceRepeatDelay = 0.4f;
	float BackspaceRepeatRate = 0.05f;
	float BackspaceNextRepeatTime = 0.0f;

	// Current states
	EArchVisInteractionMode CurrentInteractionMode = EArchVisInteractionMode::Drafting2D;
	EDraftingMode CurrentDraftingMode = EDraftingMode::Wall;
	EArchVis2DToolMode Current2DToolMode = EArchVis2DToolMode::Selection;
	ERTPlanToolType CurrentToolType = ERTPlanToolType::None;
	bool bNumericEntryContextActive = false;

	// Mouse position when numeric input started (buffer became non-empty)
	FVector2D NumericInputStartMousePos;
	
	// Threshold in pixels for mouse movement to cancel numeric input
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Input")
	float NumericInputMoveThreshold = 10.0f;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveToolIMC;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveModeBaseIMC;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveDraftingModeIMC;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UToolInputComponent> ToolInput;

	UPROPERTY(EditAnywhere, Category = "ArchVis|Debug")
	bool bInputDebugEnabled = false;

	UPROPERTY(EditAnywhere, Category = "ArchVis|Debug")
	bool bSelectionDebugEnabled = false;

private:
	// --- Wall Properties Widget ---
	
	/** The wall properties widget instance */
	UPROPERTY(Transient)
	TObjectPtr<URTPlanWallPropertiesWidget> WallPropertiesWidget;

	/** The loaded finish catalog */
	UPROPERTY(Transient)
	TObjectPtr<URTFinishCatalog> FinishCatalog;

	/** Setup the wall properties widget (called from OnGameModeReady) */
	void SetupWallPropertiesWidget();
};
