# Editor Changes: Input System Refactoring

This document details the required changes to the Enhanced Input assets in the Unreal Editor to support the new multi-mode drafting system. We will deconstruct the existing monolithic `IMC_2DBase` and create a modular, mode-specific context structure.

---

## 1. Input Actions (IA)

Create the following new Input Actions in the Content Browser.

### 1.1 Mode Switching Actions
**Location**: `Content/Input/Actions/ModeActions/` (Create folder)

| Name | Value Type | Description |
|------|------------|-------------|
| `IA_Mode_Wall` | Digital (Bool) | Switch to Wall Drafting Mode |
| `IA_Mode_Floor` | Digital (Bool) | Switch to Floor Drafting Mode |
| `IA_Mode_Ceiling` | Digital (Bool) | Switch to Ceiling Drafting Mode |

### 1.2 Floor Tool Actions
**Location**: `Content/Input/Actions/FloorActions/` (Create folder)

| Name | Value Type | Description |
|------|------------|-------------|
| `IA_Tool_DrawFloor` | Digital (Bool) | Activate Draw Floor Tool |
| `IA_Tool_NewFloorArea` | Digital (Bool) | Activate New Floor Area Tool |
| `IA_Tool_ExtrudeFloor` | Digital (Bool) | Activate Extrude Floor Tool |
| `IA_Tool_ExtendFloorArea` | Digital (Bool) | Activate Extend Floor Area Tool |
| `IA_Tool_EditExtrude` | Digital (Bool) | Activate Edit Extrude Tool |

### 1.3 Ceiling Tool Actions
**Location**: `Content/Input/Actions/CeilingActions/` (Create folder)

| Name | Value Type | Description |
|------|------------|-------------|
| `IA_Tool_DrawCeiling` | Digital (Bool) | Activate Draw Ceiling Tool |
| `IA_Tool_CreateFalseCeiling` | Digital (Bool) | Activate Create False Ceiling Tool |
| `IA_Tool_EditFalseCeiling` | Digital (Bool) | Activate Edit False Ceiling Tool |

---

## 2. Input Mapping Contexts (IMC)

We will restructure the IMCs to separate **Navigation** from **Tool Selection**.

### 2.1 Refactor `IMC_2DBase` -> `IMC_Drafting_Navigation`
**Action**: Rename `IMC_2DBase` to `IMC_Drafting_Navigation` (or create new and delete old).
**Goal**: This context should **ONLY** contain navigation and view controls.
**Changes**:
1.  **Keep**: `IA_Pan`, `IA_PanDelta`, `IA_Zoom`, `IA_PointerPosition`, `IA_ResetView`.
2.  **Remove**: All Tool shortcuts (`IA_ToolLine`, `IA_ToolPolyLine`, `IA_ToolArc`, `IA_ToolTrim`, `IA_ToolSelect`). These will move to mode-specific contexts.

### 2.2 Create `IMC_Global_Modes`
**Priority**: 0 (Always Active)
**Goal**: Global shortcuts to switch between drafting modes.

| Input Action | Key Binding |
|--------------|-------------|
| `IA_Mode_Wall` | `1` |
| `IA_Mode_Floor` | `2` |
| `IA_Mode_Ceiling` | `3` |

### 2.3 Create `IMC_Drafting_Wall`
**Priority**: 1 (Active only in Wall Mode)
**Goal**: Shortcuts for Wall tools.

| Input Action | Key Binding |
|--------------|-------------|
| `IA_ToolSelect` | `V` |
| `IA_ToolLine` | `L` |
| `IA_ToolPolyLine` | `P` |
| `IA_ToolArc` | `A` |
| `IA_ToolTrim` | `T` |

### 2.4 Create `IMC_Drafting_Floor`
**Priority**: 1 (Active only in Floor Mode)
**Goal**: Shortcuts for Floor tools.

| Input Action | Key Binding |
|--------------|-------------|
| `IA_ToolSelect` | `V` |
| `IA_Tool_DrawFloor` | `F` |
| `IA_Tool_NewFloorArea` | `N` |
| `IA_Tool_ExtrudeFloor` | `E` |
| `IA_Tool_ExtendFloorArea` | `X` |
| `IA_Tool_EditExtrude` | `M` |

### 2.5 Create `IMC_Drafting_Ceiling`
**Priority**: 1 (Active only in Ceiling Mode)
**Goal**: Shortcuts for Ceiling tools.

| Input Action | Key Binding |
|--------------|-------------|
| `IA_ToolSelect` | `V` |
| `IA_Tool_DrawCeiling` | `C` |
| `IA_Tool_CreateFalseCeiling` | `F` |
| `IA_Tool_EditFalseCeiling` | `E` |

---

## 3. Data Asset Updates

You must update the `DA_ArchVisInput` data asset to reference these new assets so the C++ code can load them.

1.  Open `DA_ArchVisInput`.
2.  Locate the **Input Mapping Contexts** section.
3.  Add/Update entries for:
    *   `IMC_Drafting_Navigation` (was `IMC_2DBase`)
    *   `IMC_Global_Modes`
    *   `IMC_Drafting_Wall`
    *   `IMC_Drafting_Floor`
    *   `IMC_Drafting_Ceiling`
4.  Locate the **Input Actions** section.
5.  Add entries for all the new Input Actions created in Section 1.

---

## 4. Summary of Context Activation Logic

This is how the C++ code will manage these contexts (for reference):

*   **Always Active**: `IMC_Global` (Undo/Redo), `IMC_Global_Modes` (1/2/3 Switching).
*   **Active in 2D View**: `IMC_Drafting_Navigation` (Pan/Zoom).
*   **Active in Wall Mode**: `IMC_Drafting_Wall`.
*   **Active in Floor Mode**: `IMC_Drafting_Floor`.
*   **Active in Ceiling Mode**: `IMC_Drafting_Ceiling`.
*   **Active when Tool Selected**: `IMC_Tool_Draw` (or specific tool IMCs like `IMC_2DLineTool`).

This structure allows `V` to trigger `Select Tool` in all modes, while `F` does something different in Floor Mode vs Ceiling Mode.


The following is my current IMC setup You can create a document to keep track

IMC_Base :
- IA_ModifierCtrl - LCtrl,Rctrl
- IA_ModifierAlt - LeftAlt
- IA_ModifierShift - LShift,RShift
- IA_Undo - Z + chordedaction IA_ModifierCtrl
- IA_Redo - Y + chordedaction IA_ModifierCtrl
- IA_Save - S + chordedaction IA_ModifierCtrl
- IA_Delete - Delete
- IA_Escape - escape
- IA_ToggleView - Tab
- IA_ToolSelect - V
- IA_ToolLine - L
- IA_ToolPolyline - P
- IA_ToolTrim - T
- IA_ToolArc - R

IMC_DraftingNavigation :
- IA_Pan - mmb
- IA_PanDelta - mouse xy + chorded action IA_Pan
- IA_Zoom - mouse wheel axis
- IA_ResetView - Home
- IA_FocusSelection - F
- IA_PointerPosition - Mouse xy
- IA_SnapToggle - s
- IA_GridToggle - g

IMC_3DNavigation
- IA_ViewTop - num7
- IA_ViewRight - Num3
- IA_ViewFront - Num1
- IA_ViewPerspective - Num5

IMC_3DBase
- IA_Pan - mmb
- IA_PanDelta - mouse xy + chorded action IA_Pan
- IA_Zoom - mouse wheel axis
- IA_Orbit - Lmb + Chorded action IA_ModifierAlt + down
- IA_OrbitDelta - mouse xy + chorded action IA_orbit
- IA_ResetView - Home
- IA_FocusSelection - F
- IA_PointerPosition - MouseXY
- IA_SnapToggle - S
- IA_GridToggle - G
- IA_Move - W + SwizzleinputAxisValues YXZ, S + SwizzleinputAxisValues YXZ + negate, A + Negate, D
- IA_MoveUp - E
- IA_MoveDown - Q
- IA_Look - MouseXY + chordedaction IA_FlyMode
- IA_FlyMode - Rmb
- IA_DollyZoom - MouseY + chorded action IA_FlyMode + chorded action IA_ModifierAlt
- IA_AdjustFlySpeed - MouseWheelAxis + chorded action IA_FlyMode

IMC_2DSelection
- IA_SelectAdd - Lmb + choeded action IA_ModifierShift
- IA_SelectRemove - Lmb + chroeded action IA_ModifierAlt
- IA_SelectToggle - Lmb + Chordedaction IA_ModifierCtrl
- IA_SelectAll - A + Chorded Action IA_ModifierCtrl
- IA_DeselectAll - D + chorded action IA_ModifierCtrl
- IA_BoxSelectStart - Lmb + hold 0.15
- IA_BoxSelectEnd - LMB + Released
- IA_BoxSelectDrag - MouseXY + chorded action IA_BoxSelectStart
- IA_Select - LMB
- IA_CycleSelction - SpaceBar
- IA_FocusSelection - F

IMC_3DSelection
- IA_Select - LMB
- IA_SelectAdd - Lmb + choeded action IA_ModifierShift
- IA_SelectRemove - Lmb + chroeded action IA_ModifierAlt
- IA_SelectToggle - Lmb + Chordedaction IA_ModifierCtrl
- IA_SelectAll - A + Chorded Action IA_ModifierCtrl
- IA_DeselectAll - D + chorded action IA_ModifierCtrl

IMC_2DLineTool
- IA_DrawPlacePoint - LMB + pressed
- IA_DrawCancel - Rmb + pressed
- IA_OrthoLock - LeftShift + down
- IA_AngleSnap - A + pressed

IMC_2DpolyLineTool
- IA_DrawPlacePoint - LMB + pressed
- IA_DrawConfirm - Enter
- IA_DrawCancel - Rmb + pressed
- IA_DrawClose - C + pressed
- IA_DrawRemoveLastPoint - BackSpace + pressed
- IA_OrthoLock - LeftShift + down
- IA_AngleSnap - A + pressed

IMC_2DArcTool
- IA_DrawPlacePoint - LMB + pressed
- IA_DrawCancel - Rmb + pressed
- IA_OrthoLock - LeftShift + down
- IA_AngleSnap - A + pressed

IMC_2DTrimTool
- IA_Select - LMB
- IA_BoxSelectStart - Lmb + hold 0.15
- IA_BoxSelectEnd - LMB + Released
- IA_BoxSelectDrag - MouseXY + chorded action IA_BoxSelectStart

IMC_NumericEntry
- IA_NumericDigit - 0-9/num0-9 + scalar 1-10 
- IA_NumericDecimal - period, num.
- IA_BackSpace - Backspace
- IA_NumericCommit - Enter
- IA_NumericCancel - escape
- IA_NumericSwitchField - Tab
- IA_NumericCycleUnits - U
- IA_NumericAdd - Num+
- IA_NumericSubtract - Num-
- IA_NumericMultiply - Num*
- IA_NumericDivide - Num/