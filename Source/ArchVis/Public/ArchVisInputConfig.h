#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ArchVisInputConfig.generated.h"

/**
 * Data Asset to hold Input Actions and Mapping Contexts for the ArchVis project.
 * 
 * Context Hierarchy:
 * IMC_Global (Priority: 0 - Always Active)
 * │
 * ├── IMC_2D_Base (Priority: 1 - When in 2D mode)
 * │   ├── IMC_2D_Selection (Priority: 2)
 * │   ├── IMC_2D_LineTool (Priority: 2)
 * │   │   └── IMC_NumericEntry (Priority: 3 - Layered when typing)
 * │   ├── IMC_2D_PolylineTool (Priority: 2)
 * │   │   └── IMC_NumericEntry (Priority: 3 - Layered when typing)
 * │   └── IMC_2D_TrimTool (Priority: 2)
 * │
 * └── IMC_3D_Base (Priority: 1 - When in 3D mode)
 *     ├── IMC_3D_Selection (Priority: 2)
 *     └── IMC_3D_Navigation (Priority: 2)
 */
UCLASS()
class ARCHVIS_API UArchVisInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// ============================================
	// INPUT MAPPING CONTEXTS - Hierarchical Structure
	// ============================================
	
	// Global context - always active, lowest priority (0)
	// Contains: Undo/Redo, Delete, Escape, ToggleView, Save, Modifiers
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts|Global")
	TObjectPtr<UInputMappingContext> IMC_Global;

	// --- 2D Mode Contexts ---
	
	// 2D Base context - active in 2D mode (Priority 1)
	// Contains: Pan, Zoom, Pointer position, Grid/Snap toggles
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts|2D")
	TObjectPtr<UInputMappingContext> IMC_2D_Base;

	// 2D Selection tool context (Priority 2)
	// Contains: Select, BoxSelect, SelectAll, DeselectAll
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts|2D")
	TObjectPtr<UInputMappingContext> IMC_2D_Selection;

	// 2D Line tool context (Priority 2)
	// Contains: DrawPlacePoint, DrawCancel, OrthoLock
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts|2D")
	TObjectPtr<UInputMappingContext> IMC_2D_LineTool;

	// 2D Polyline tool context (Priority 2)
	// Contains: DrawPlacePoint, DrawConfirm, DrawCancel, DrawClose, DrawRemoveLastPoint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts|2D")
	TObjectPtr<UInputMappingContext> IMC_2D_PolylineTool;

	// 2D Trim tool context (Priority 2)
	// Contains: Select, BoxSelect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts|2D")
	TObjectPtr<UInputMappingContext> IMC_2D_TrimTool;

	// Numeric Entry context - layered on top during numeric input (Priority 3)
	// Contains: Digit keys, Decimal, Backspace, Arithmetic operators, Commit, Cancel
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts|Numeric")
	TObjectPtr<UInputMappingContext> IMC_NumericEntry;

	// --- 3D Mode Contexts ---

	// 3D Base context - active in 3D mode (Priority 1)
	// Contains: Orbit, Pan, Zoom, Pointer position
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts|3D")
	TObjectPtr<UInputMappingContext> IMC_3D_Base;

	// 3D Selection context (Priority 2)
	// Contains: Select, SelectAdd, SelectToggle
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts|3D")
	TObjectPtr<UInputMappingContext> IMC_3D_Selection;

	// 3D Navigation context (Priority 2)
	// Contains: ViewTop, ViewFront, ViewRight, ViewPerspective
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Contexts|3D")
	TObjectPtr<UInputMappingContext> IMC_3D_Navigation;

	// ============================================
	// GLOBAL INPUT ACTIONS (IMC_Global)
	// ============================================
	
	// Modifier keys - helper actions for chord bindings
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Global|Modifiers")
	TObjectPtr<UInputAction> IA_ModifierCtrl;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Global|Modifiers")
	TObjectPtr<UInputAction> IA_ModifierShift;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Global|Modifiers")
	TObjectPtr<UInputAction> IA_ModifierAlt;

	// Global commands
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Global")
	TObjectPtr<UInputAction> IA_Undo;  // Ctrl+Z

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Global")
	TObjectPtr<UInputAction> IA_Redo;  // Ctrl+Y

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Global")
	TObjectPtr<UInputAction> IA_Delete;  // Delete key

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Global")
	TObjectPtr<UInputAction> IA_Escape;  // Escape - cancel/deselect

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Global")
	TObjectPtr<UInputAction> IA_ToggleView;  // Tab - switch 2D/3D

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Global")
	TObjectPtr<UInputAction> IA_Save;  // Ctrl+S

	// ============================================
	// VIEW ACTIONS (IMC_2D_Base / IMC_3D_Base)
	// ============================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_Pan;  // MMB held

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_PanDelta;  // Mouse XY / Arrow keys

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_Zoom;  // Mouse wheel

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_Move;  // WASD movement (Fly pawn)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_MoveUp;  // E key (Fly pawn up)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_MoveDown;  // Q key (Fly pawn down)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_Look;  // Mouse XY for look (Fly pawn)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_Orbit;  // Orbit mode (Alt + Select in 3D)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_OrbitDelta;  // Mouse XY for orbit

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_DollyZoom;  // Dolly zoom along view direction

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_FlyMode;  // Fly mode - enables WASD movement and mouselook

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_AdjustFlySpeed;  // Adjust fly speed (RMB + Scroll)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_ResetView;  // Reset camera to default

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_FocusSelection;  // Focus on selected object

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_PointerPosition;  // Mouse XY tracking

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_SnapToggle;  // Toggle grid snap

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|View")
	TObjectPtr<UInputAction> IA_GridToggle;  // Toggle grid visibility

	// ============================================
	// SELECTION ACTIONS (IMC_2D_Selection / IMC_3D_Selection)
	// ============================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Selection")
	TObjectPtr<UInputAction> IA_Select;  // Primary select action

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Selection")
	TObjectPtr<UInputAction> IA_SelectAdd;  // Add to selection (Shift modifier)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Selection")
	TObjectPtr<UInputAction> IA_SelectToggle;  // Toggle selection (Ctrl modifier)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Selection")
	TObjectPtr<UInputAction> IA_SelectRemove;  // Remove from selection (Alt modifier)

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Selection")
	TObjectPtr<UInputAction> IA_SelectAll;  // Select all objects

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Selection")
	TObjectPtr<UInputAction> IA_DeselectAll;  // Deselect all objects

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Selection")
	TObjectPtr<UInputAction> IA_BoxSelectStart;  // Start box selection

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Selection")
	TObjectPtr<UInputAction> IA_BoxSelectDrag;  // Mouse XY during box select

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Selection")
	TObjectPtr<UInputAction> IA_BoxSelectEnd;  // End box selection

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Selection")
	TObjectPtr<UInputAction> IA_CycleSelection;  // Cycle through overlapping objects

	// ============================================
	// DRAWING ACTIONS (IMC_2D_LineTool / IMC_2D_PolylineTool)
	// ============================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Drawing")
	TObjectPtr<UInputAction> IA_DrawPlacePoint;  // Place vertex

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Drawing")
	TObjectPtr<UInputAction> IA_DrawConfirm;  // Finish drawing

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Drawing")
	TObjectPtr<UInputAction> IA_DrawCancel;  // Cancel drawing

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Drawing")
	TObjectPtr<UInputAction> IA_DrawClose;  // Close polygon

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Drawing")
	TObjectPtr<UInputAction> IA_DrawRemoveLastPoint;  // Remove last vertex

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Drawing")
	TObjectPtr<UInputAction> IA_OrthoLock;  // Constrain to axis

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Drawing")
	TObjectPtr<UInputAction> IA_AngleSnap;  // A - toggle 45° snap

	// ============================================
	// NUMERIC ENTRY ACTIONS (IMC_NumericEntry)
	// ============================================
	
	// Single digit action with 1D Axis - each key 0-9 uses Scalar modifier
	// Keys 1-9 output 1.0-9.0, Key 0 outputs 10.0 (special case, maps to 0)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric")
	TObjectPtr<UInputAction> IA_NumericDigit;  // 1D Axis - receives digit value via Scalar modifier

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric")
	TObjectPtr<UInputAction> IA_NumericDecimal;  // Period / Numpad Decimal

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric")
	TObjectPtr<UInputAction> IA_NumericBackspace;  // Backspace

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric")
	TObjectPtr<UInputAction> IA_NumericCommit;  // Enter / Numpad Enter

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric")
	TObjectPtr<UInputAction> IA_NumericCancel;  // Escape

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric")
	TObjectPtr<UInputAction> IA_NumericSwitchField;  // Tab - switch Length/Angle

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric")
	TObjectPtr<UInputAction> IA_NumericCycleUnits;  // U - cycle units

	// Arithmetic operators
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric|Arithmetic")
	TObjectPtr<UInputAction> IA_NumericAdd;  // Plus / Numpad Plus

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric|Arithmetic")
	TObjectPtr<UInputAction> IA_NumericSubtract;  // Minus / Numpad Minus

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric|Arithmetic")
	TObjectPtr<UInputAction> IA_NumericMultiply;  // Asterisk / Numpad Multiply

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Numeric|Arithmetic")
	TObjectPtr<UInputAction> IA_NumericDivide;  // Slash / Numpad Divide

	// ============================================
	// 3D NAVIGATION ACTIONS (IMC_3D_Navigation)
	// ============================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|3DNav")
	TObjectPtr<UInputAction> IA_ViewTop;  // Numpad 7

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|3DNav")
	TObjectPtr<UInputAction> IA_ViewFront;  // Numpad 1

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|3DNav")
	TObjectPtr<UInputAction> IA_ViewRight;  // Numpad 3

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|3DNav")
	TObjectPtr<UInputAction> IA_ViewPerspective;  // Numpad 5 - toggle ortho/perspective

	// ============================================
	// TOOL SWITCHING (IMC_Global)
	// ============================================
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Tools")
	TObjectPtr<UInputAction> IA_ToolSelect;  // V key

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Tools")
	TObjectPtr<UInputAction> IA_ToolLine;  // L key

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Tools")
	TObjectPtr<UInputAction> IA_ToolPolyline;  // P key

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Actions|Tools")
	TObjectPtr<UInputAction> IA_ToolTrim;  // T key
};
