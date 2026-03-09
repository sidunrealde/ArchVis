# Revised IMC Structure

This document outlines a new, refactored structure for the project's Input Mapping Contexts (IMCs). The goal is to create a more modular, extensible, and context-sensitive input system that can accommodate the new drafting modes (Wall, Floor, Ceiling) and future expansion.

---

## 1. Analysis of the Current Structure

The current setup has several strengths, such as the use of dedicated IMCs for active tools (`IMC_2DLineTool`, etc.) and numeric entry. However, it has one major architectural issue:

*   **Monolithic `IMC_Base`**: This context mixes truly global actions (like Undo/Redo) with tool selection hotkeys (like `L` for Line, `P` for Polyline). This prevents us from using the same hotkey for different tools in different modes (e.g., using `F` for "Draw Floor" in Floor Mode and "False Ceiling" in Ceiling Mode).

## 2. The New Layered Architecture

The new structure is based on a clear separation of concerns, organized by priority. An action in a higher priority context will always take precedence.

```
Priority 4: [ IMC_NumericEntry ] (When typing numbers)
------------------------------------------------------------------
Priority 3: [ IMC_Tool_* ] (e.g., IMC_Tool_Select, IMC_Tool_Line) (When a tool is active)
------------------------------------------------------------------
Priority 2: [ IMC_Mode_*_Tools ] (e.g., IMC_Mode_Wall_Tools) (Based on Drafting Mode)
------------------------------------------------------------------
Priority 1: [ IMC_2D_Navigation ] or [ IMC_3D_Navigation ] (Based on Pawn)
------------------------------------------------------------------
Priority 0: [ IMC_Global_Base, IMC_Global_ModeSwitching ] (Always on)
```

### Activation Logic Summary

*   **Priority 0 (Global)**: Added once and are always active.
*   **Priority 1 (Navigation)**: Added/removed by the Pawn (`DraftingPawn` adds `IMC_2D_Navigation`, `OrbitPawn` adds `IMC_3D_Navigation`).
*   **Priority 2 (Mode Tools)**: The `PlayerController` swaps these out when the drafting mode changes (e.g., switching from Wall to Floor mode removes `IMC_Mode_Wall_Tools` and adds `IMC_Mode_Floor_Tools`).
*   **Priority 3 (Active Tool)**: The `PlayerController` swaps these out when a tool is selected (e.g., pressing `L` in Wall mode activates the Line Tool and adds `IMC_Tool_Line`).
*   **Priority 4 (Overlay)**: Added on top of everything else when the user starts typing a number.

---

## 3. Detailed IMC Definitions

Here is the detailed breakdown of the new IMCs. **This is the guide for restructuring the assets in the Unreal Editor.**

### Priority 0: Global Contexts

#### `IMC_Global_Base` (Replaces `IMC_Base`)
*   **Purpose**: Actions that should be available everywhere, regardless of mode or tool.
*   **Contents**:
    *   `IA_ModifierCtrl` (LCtrl, RCtrl)
    *   `IA_ModifierAlt` (LAlt)
    *   `IA_ModifierShift` (LShift, RShift)
    *   `IA_Undo` (Z + Chord: `IA_ModifierCtrl`)
    *   `IA_Redo` (Y + Chord: `IA_ModifierCtrl`)
    *   `IA_Save` (S + Chord: `IA_ModifierCtrl`)
    *   `IA_Delete` (Delete)
    *   `IA_Escape` (Escape)
    *   `IA_ToggleView` (Tab)

#### `IMC_Global_ModeSwitching` (New)
*   **Purpose**: Dedicated context for switching between the main drafting modes.
*   **Contents**:
    *   `IA_Mode_Wall` (`1`)
    *   `IA_Mode_Floor` (`2`)
    *   `IA_Mode_Ceiling` (`3`)

---

### Priority 1: Navigation Contexts

#### `IMC_2D_Navigation` (Renamed from `IMC_DraftingNavigation`)
*   **Purpose**: All navigation actions for 2D drafting pawns.
*   **Contents**:
    *   `IA_Pan` (MMB)
    *   `IA_PanDelta` (Mouse XY + Chord: `IA_Pan`)
    *   `IA_Zoom` (Mouse Wheel Axis)
    *   `IA_ResetView` (Home)
    *   `IA_FocusSelection` (F)
    *   `IA_PointerPosition` (Mouse XY)
    *   `IA_SnapToggle` (S)
    *   `IA_GridToggle` (G)

#### `IMC_3D_Navigation` (Combines `IMC_3DBase` and `IMC_3DNavigation`)
*   **Purpose**: All navigation and view-switching actions for 3D pawns.
*   **Contents**:
    *   All actions from `IMC_2D_Navigation` (Pan, Zoom, etc. are still useful).
    *   `IA_Orbit` (LMB + Chord: `IA_ModifierAlt`, Trigger: Down)
    *   `IA_OrbitDelta` (Mouse XY + Chord: `IA_Orbit`)
    *   `IA_Move` (W/S/A/D)
    *   `IA_MoveUp` (E)
    *   `IA_MoveDown` (Q)
    *   `IA_FlyMode` (RMB)
    *   `IA_Look` (Mouse XY + Chord: `IA_FlyMode`)
    *   `IA_DollyZoom` (Mouse Wheel Axis + Chord: `IA_FlyMode` + Chord: `IA_ModifierAlt`)
    *   `IA_AdjustFlySpeed` (Mouse Wheel Axis + Chord: `IA_FlyMode`)


---

### Priority 2: Mode-Specific Tool Hotkeys

#### `IMC_Mode_Wall_Tools` (New)
*   **Purpose**: Contains the hotkeys for selecting tools available in **Wall Mode**.
*   **Contents**:
    *   `IA_ToolSelect` (V)
    *   `IA_ToolLine` (L)
    *   `IA_ToolPolyline` (P)
    *   `IA_ToolTrim` (T)
    *   `IA_ToolArc` (R)

#### `IMC_Mode_Floor_Tools` (New)
*   **Purpose**: Contains the hotkeys for selecting tools available in **Floor Mode**.
*   **Contents**:
    *   `IA_ToolSelect` (V)
    *   `IA_Tool_DrawFloor` (F)
    *   `IA_Tool_NewFloorArea` (N)
    *   `IA_Tool_ExtrudeFloor` (E)
    *   `IA_Tool_ExtendFloorArea` (X)
    *   `IA_Tool_EditExtrude` (M)

#### `IMC_Mode_Ceiling_Tools` (New)
*   **Purpose**: Contains the hotkeys for selecting tools available in **Ceiling Mode**.
*   **Contents**:
    *   `IA_ToolSelect` (V)
    *   `IA_Tool_DrawCeiling` (C)
    *   `IA_Tool_CreateFalseCeiling` (F)
    *   `IA_Tool_EditFalseCeiling` (E)

---

### Priority 3: Active Tool Contexts

#### `IMC_Tool_Select` (Replaces `IMC_2DSelection` and `IMC_3DSelection`)
*   **Purpose**: Actions to perform when the Select Tool is active.
*   **Contents**:
    *   `IA_Select` (LMB)
    *   `IA_SelectAdd` (LMB + Chord: `IA_ModifierShift`)
    *   `IA_SelectRemove` (LMB + Chord: `IA_ModifierAlt`)
    *   `IA_SelectToggle` (LMB + Chord: `IA_ModifierCtrl`)
    *   `IA_SelectAll` (A + Chord: `IA_ModifierCtrl`)
    *   `IA_DeselectAll` (D + Chord: `IA_ModifierCtrl`)
    *   `IA_BoxSelectStart` (LMB, Trigger: Hold, Duration: 0.15s)
    *   `IA_BoxSelectEnd` (LMB, Trigger: Released)
    *   `IA_BoxSelectDrag` (Mouse XY + Chord: `IA_BoxSelectStart`)
    *   `IA_CycleSelection` (SpaceBar)

#### `IMC_Tool_Line` (Renamed from `IMC_2DLineTool`)
*   **Contents**:
    *   `IA_DrawPlacePoint` (LMB, Trigger: Pressed)
    *   `IA_DrawCancel` (RMB, Trigger: Pressed)
    *   `IA_OrthoLock` (LShift, Trigger: Down)
    *   `IA_AngleSnap` (A, Trigger: Pressed)

#### `IMC_Tool_Polyline` (Renamed from `IMC_2DPolylineTool`)
*   **Purpose**: A generic context for any "pen-style" tool. Can be reused for Draw Floor, Draw Ceiling, etc.
*   **Contents**:
    *   `IA_DrawPlacePoint` (LMB, Trigger: Pressed)
    *   `IA_DrawConfirm` (Enter)
    *   `IA_DrawCancel` (RMB, Trigger: Pressed)
    *   `IA_DrawClose` (C, Trigger: Pressed)
    *   `IA_DrawRemoveLastPoint` (Backspace, Trigger: Pressed)
    *   `IA_OrthoLock` (LShift, Trigger: Down)
    *   `IA_AngleSnap` (A, Trigger: Pressed)

#### `IMC_Tool_Arc` (Renamed from `IMC_2DArcTool`)
*   **Contents**:
    *   `IA_DrawPlacePoint` (LMB, Trigger: Pressed)
    *   `IA_DrawCancel` (RMB, Trigger: Pressed)
    *   `IA_OrthoLock` (LShift, Trigger: Down)
    *   `IA_AngleSnap` (A, Trigger: Pressed)

#### `IMC_Tool_Trim` (Renamed from `IMC_2DTrimTool`)
*   **Contents**:
    *   `IA_Select` (LMB)
    *   `IA_BoxSelectStart` (LMB, Trigger: Hold, Duration: 0.15s)
    *   `IA_BoxSelectEnd` (LMB, Trigger: Released)
    *   `IA_BoxSelectDrag` (Mouse XY + Chord: `IA_BoxSelectStart`)

---

### Priority 4: Overlay Context

#### `IMC_NumericEntry` (No Changes)
*   **Purpose**: Captures all numeric input when active. This context is well-designed and requires no changes.
*   **Contents**:
    *   `IA_NumericDigit` (0-9/Num 0-9 with Scalar modifiers)
    *   `IA_NumericDecimal` (Period, Numpad Decimal)
    *   `IA_NumericBackspace` (Backspace)
    *   `IA_NumericCommit` (Enter)
    *   `IA_NumericCancel` (Escape)
    *   `IA_NumericSwitchField` (Tab)
    *   `IA_NumericCycleUnits` (U)
    *   `IA_NumericAdd` (Num +)
    *   `IA_NumericSubtract` (Num -)
    *   `IA_NumericMultiply` (Num *)
    *   `IA_NumericDivide` (Num /)
