# ArchVis Master Plan

> **Goal**: A CAD-like drafting experience for runtime architectural planning in Unreal Engine, supporting seamless 2D/3D switching, precision input, and auto-generation of shell geometry.

**Last Updated**: January 19, 2026  
**Current Focus**: 2D/3D Navigation Polish & Virtual Cursor Fixes

---

## Project Architecture Overview

### Plugin Suite Structure
| Plugin | Purpose | Status |
|--------|---------|--------|
| **RTPlanCore** | Data model (`URTPlanDocument`), Command pattern, Undo/Redo, JSON serialization | ✅ Done |
| **RTPlanMath** | 2D geometry utilities, distance calculations, intersections | ✅ Done |
| **RTPlanSpatial** | Spatial indexing, snap queries, hit testing | ✅ Done |
| **RTPlanInput** | Unified pointer events (`FRTPointerEvent`), modifier key states | ✅ Done |
| **RTPlanTools** | Tool framework, LineTool, SelectTool, ToolManager | ✅ Done |
| **RTPlanMeshing** | Dynamic mesh generation for walls | ✅ Done |
| **RTPlanShell** | 3D wall mesh rendering (`ARTPlanShellActor`) | ✅ Partial |
| **RTPlanOpenings** | Door/Window wall splitting utilities | ✅ Partial |
| **RTPlanUI** | UMG widgets (Toolbar, Properties, Catalog) | ⏳ Stub |
| **RTPlanCatalog** | Product catalog data assets | ⏳ Stub |
| **RTPlanObjects** | Furniture object lifecycle | ⏳ Stub |
| **RTPlanRuns** | Cabinet run generation | ⏳ Stub |
| **RTPlanNet** | Multiplayer replication | ⏳ Partial |
| **RTPlanVR** | VR pawn and interaction | ⏳ Stub |

### Game Module (`Source/ArchVis`)
| Component | Status |
|-----------|--------|
| `AArchVisGameMode` | ✅ Creates Document, ToolManager, ShellActor, NetDriver |
| `AArchVisPlayerController` | ✅ Enhanced Input, Virtual Cursor, Pawn Switching, Undo/Redo |
| `AArchVisPawnBase` | ✅ Base pawn with camera controls |
| `AArchVisDraftingPawn` | ✅ 2D Top-down orthographic drafting (uses UDraftingInputComponent) |
| `AArchVisOrbitPawn` | ✅ 3D Perspective orbit camera (uses UOrbitInputComponent) |
| `AArchVisFirstPersonPawn` | ✅ First-person walkthrough character |
| `AArchVisThirdPersonPawn` | ✅ Third-person walkthrough character |
| `AArchVisHUD` | ✅ Crosshair, Wall Preview, Measurements, Length/Angle Labels |

### Modular Input Components (`Source/ArchVis/Public/Input/`)
| Component | Status | Description |
|-----------|--------|-------------|
| `UArchVisInputComponent` | ✅ Base | Abstract base for pawn-specific input handling |
| `UDraftingInputComponent` | ✅ Done | 2D pan/zoom (1:1 cursor-to-scene movement with interpolation) - attached to DraftingPawn |
| `UOrbitInputComponent` | ✅ Done | 3D orbit/pan/dolly/fly/WASD movement with adjustable speed - attached to OrbitPawn |
| `UToolInputComponent` | ✅ Done | Tool actions (draw/select) - attached to Controller |
| `UFirstPersonInputComponent` | ⏳ TODO | FPS movement - for FirstPersonPawn |
| `UThirdPersonInputComponent` | ⏳ TODO | TPS camera/movement - for ThirdPersonPawn |

### Modular Input Architecture (Refactored Jan 2026)

The input system was refactored from a **monolithic PlayerController** to a **pawn-centric modular architecture** using Unreal's Enhanced Input system.

**Key Principles:**
1. **Pawn owns its navigation input** - Each pawn type has its own input component that handles navigation (pan/zoom/orbit/fly)
2. **Controller handles tool input** - Tool interactions (drawing, selection) stay on the controller since they're pawn-agnostic
3. **IMCs managed by components** - Each input component adds/removes its own IMCs on possession/unpossession
4. **No shared input handlers** - Each pawn type has dedicated handlers, eliminating "if mode == 2D else 3D" branching

**Architecture:**
```
AArchVisPlayerController
├── UToolInputComponent (tool actions, selection)
├── Global IMC (undo/redo/escape/modifiers)
└── Numeric input buffer

AArchVisDraftingPawn
├── UDraftingInputComponent
│   ├── IMC_2DBase
│   └── Handlers: OnPan, OnZoom, OnPointerMove
└── Camera controls (orthographic)

AArchVisOrbitPawn
├── UOrbitInputComponent
│   ├── IMC_3DBase, IMC_3DNavigation
│   └── Handlers: OnLMB, OnRMB, OnMMB, OnAlt, OnZoom, OnMove
│   └── ProcessMouseNavigation() in Tick
└── Camera controls (perspective, orbit, fly)
```

**Benefits:**
- Clean separation of concerns
- Easy to add new pawn types (just create new input component)
- Input handlers live close to the pawn they control
- Mouse lock/unlock managed per-pawn
- Eliminates complex mode-switching logic in controller

---

## Phase Status Summary

| Phase | Description | Status |
|-------|-------------|--------|
| **Phase 0** | Baseline plumbing (game ↔ plugins) | ✅ Complete |
| **Phase 1** | Viewport & Input (CAD Feel) | ✅ Complete (input assets + pawn system + navigation polished) |
| **Phase 2** | Wall Tool 2.0 (Click-Move-Click) | ✅ Complete (all drawing features done) |
| **Phase 3** | 2D Visualization | ⏳ 30% (crosshair done, plan drawing pending) |
| **Phase 4** | Room Detection & Generation | ❌ Not Started |
| **Phase 5** | Properties & Editing | ⏳ 60% (SelectTool done, UI pending) |

---

## Current Input System Status

### Input Mapping Contexts Created ✅
All IMC assets have been created in `Content/Input/`:
- `IMC_Base` (Priority 0 - Always Active)
- `IMC_2DBase` (Priority 1 - 2D Mode)
- `IMC_2DSelection` (Priority 2 - Select Tool)
- `IMC_2DLineTool` (Priority 2 - Line Tool)
- `IMC_2DPolylineTool` (Priority 2 - Polyline Tool)
- `IMC_3DBase` (Priority 1 - 3D Mode)
- `IMC_3DSelection` (Priority 2 - 3D Selection)
- `IMC_3DNavigation` (Priority 2 - 3D Nav Views)
- `IMC_NumericEntry` (Priority 3 - Numeric Overlay)
- `IMC_ArchVis` (Legacy/backup)
- `DA_ArchVisInput` (Data Asset)

### Input Actions Created ✅
All Input Actions have been created in `Content/Input/Actions/`:

**Global Actions (`GlobalActions/`):**
- `IA_ModifierCtrl`, `IA_ModifierShift`, `IA_ModifierAlt`
- `IA_Undo`, `IA_Redo`, `IA_Delete`, `IA_Save`, `IA_Escape`
- `IA_ToggleView`
- `IA_ToolSelect`, `IA_ToolLine`, `IA_ToolPolyLine`

**View Actions (`ViewActions/`):**
- `IA_Pan`, `IA_PanDelta`, `IA_Zoom`
- `IA_Orbit`, `IA_OrbitDelta`
- `IA_PointerPosition`, `IA_ResetView`, `IA_FocusSelection`
- `IA_SnapToggle`, `IA_GridToggle`
- `IA_Move`, `IA_MoveUp`, `IA_MoveDown`, `IA_AdjustFlySpeed` (3D fly mode)

**Selection Actions (`SelectionAction/`):**
- `IA_Select`, `IA_SelectAdd`, `IA_SelectToggle`
- `IA_SelectAll`, `IA_DeselectAll`, `IA_CycleSelection`
- `IA_BoxSelectStart`, `IA_BoxSelectDrag`, `IA_BoxSelectEnd`

**Drawing Actions (`DrawingActions/`):**
- `IA_DrawPlacePoint`, `IA_DrawConfirm`, `IA_DrawCancel`, `IA_DrawClose`
- `IA_DrawRemoveLastPoint`, `IA_OrthoLock`, `IA_AngleSnap`

**Numeric Entry Actions (`NumericEntry/`):**
- `IA_NumericDigit` (1D Axis with Scalar modifiers)
- `IA_NumericDecimal`, `IA_NumericBackspace`
- `IA_NumericCommit`, `IA_NumericCancel`, `IA_NumericSwitchField`
- `IA_NumericCycleUnits`
- `IA_NumericAdd`, `IA_NumericSubtract`, `IA_NumericMultiply`, `IA_NumericDivide`

**3D Navigation Actions (`3DNavigation/`):**
- `IA_ViewTop`, `IA_ViewFront`, `IA_ViewRight`, `IA_ViewPerspective`

**Root Level:**
- `IA_PrimaryAction`, `IA_SecondaryAction`, `IA_Move3D`

---

## TODO: Input Action Functionality Implementation

This section tracks the implementation of C++ handlers for each Input Action.

### Priority 1: Core Interaction (Implement First)

#### Global Actions
| Action | Handler | Status | Notes |
|--------|---------|--------|-------|
| `IA_ModifierCtrl` | `OnModifierCtrl` | ✅ Done | Tracks `bCtrlDown` state |
| `IA_ModifierShift` | `OnModifierShift` | ✅ Done | Tracks `bShiftDown` state |
| `IA_ModifierAlt` | `OnModifierAlt` | ✅ Done | Tracks `bAltDown` state |
| `IA_Escape` | `OnEscape` | ✅ Done | Context-sensitive cancel |
| `IA_Undo` | `OnUndo` | ✅ Done | `Document->Undo()` |
| `IA_Redo` | `OnRedo` | ✅ Done | `Document->Redo()` |
| `IA_Delete` | `OnDelete` | ✅ Done | Delete selection |
| `IA_Save` | `OnSave` | ✅ Done | Save document |
| `IA_ToggleView` | `OnToggleView` | ⏳ Pending | Wire to CameraController |
| `IA_ToolSelect` | `OnToolSelect` | ⏳ Pending | `ToolManager->SelectToolByType(Select)` |
| `IA_ToolLine` | `OnToolLine` | ⏳ Pending | `ToolManager->SelectToolByType(Line)` |
| `IA_ToolPolyLine` | `OnToolPolyline` | ⏳ Pending | `ToolManager->SelectToolByType(Polyline)` |

#### View/Navigation Actions - 2D Mode
| Action | Handler | Status | Notes |
|--------|---------|--------|-------|
| `IA_Pan` | `OnPanStarted/Completed` | ✅ Done | Set `bPanning` state |
| `IA_PanDelta` | `OnPanDelta` | ✅ Done | 1:1 cursor-to-scene pan with interpolation |
| `IA_Zoom` | `OnZoom` | ✅ Done | Adjust ortho width |
| `IA_PointerPosition` | `OnPointerPosition` | ✅ Done | Update virtual cursor |
| `IA_ResetView` | `OnResetView` | ✅ Done | Reset camera to default |
| `IA_FocusSelection` | `OnFocusSelection` | 🔴 Broken | Not working properly in 2D and 3D mode |
| `IA_SnapToggle` | `OnSnapToggle` | ⏳ Pending | Toggle snap on/off |
| `IA_GridToggle` | `OnGridToggle` | ⏳ Pending | Toggle grid visibility |

#### View/Navigation Actions - 3D Mode
| Action | Handler | Status | Notes |
|--------|---------|--------|-------|
| `IA_Orbit` | `OnOrbitStarted/Completed` | ✅ Done | Set `bOrbiting` state |
| `IA_OrbitDelta` | `OnOrbitDelta` | ✅ Done | Rotate camera around target |
| `IA_Move` | `OnMove` | ✅ Done | WASD fly movement (X=right/left, Y=forward/back) |
| `IA_AdjustFlySpeed` | `OnAdjustFlySpeed` | ✅ Done | RMB+Scroll adjusts fly speed |

### Priority 2: Selection Tool

> ⚠️ **KNOWN ISSUES:**
> - Selection currently uses raw keyboard input (`IsInputKeyDown`) instead of Enhanced Input for modifiers
> - Marquee selection is not working in 2D mode
> - SelectAll and DeselectAll are not working
> - Focus selection is not working properly in both 2D and 3D mode
> - Need to migrate to use `IA_SelectAdd`, `IA_SelectToggle`, `IA_SelectRemove` properly

| Action | Handler | Status | Notes |
|--------|---------|--------|-------|
| `IA_Select` | `OnSelectStarted/Completed` | ⚠️ Partial | Basic click selection works, uses raw input for modifiers |
| `IA_SelectAdd` | `OnSelectAdd` | 🔴 Broken | Shift+Click - handler exists but not using Enhanced Input properly |
| `IA_SelectToggle` | `OnSelectToggle` | 🔴 Broken | Ctrl+Click - handler exists but not using Enhanced Input properly |
| `IA_SelectRemove` | `OnSelectRemove` | 🔴 Broken | Alt+Click - handler exists but not using Enhanced Input properly |
| `IA_SelectAll` | `OnSelectAll` | 🔴 Broken | Handler exists but not working |
| `IA_DeselectAll` | `OnDeselectAll` | 🔴 Broken | Handler exists but not working |
| `IA_CycleSelection` | `OnCycleSelection` | ⏳ Pending | Cycle overlapping objects |
| `IA_BoxSelectStart` | `OnBoxSelectStart` | 🔴 Broken | Marquee not working in 2D mode |
| `IA_BoxSelectDrag` | `OnBoxSelectDrag` | 🔴 Broken | Marquee not working in 2D mode |
| `IA_BoxSelectEnd` | `OnBoxSelectEnd` | 🔴 Broken | Marquee not working in 2D mode |

### Priority 3: Drawing Tools

| Action | Handler | Status | Notes |
|--------|---------|--------|-------|
| `IA_DrawPlacePoint` | `OnDrawPlacePoint` | ✅ Done | Place vertex (route to LineTool) |
| `IA_DrawConfirm` | `OnDrawConfirm` | ✅ Done | Confirm/finish polyline |
| `IA_DrawCancel` | `OnDrawCancel` | ✅ Done | Cancel current drawing |
| `IA_DrawClose` | `OnDrawClose` | ✅ Done | Close polygon loop |
| `IA_DrawRemoveLastPoint` | `OnDrawRemoveLastPoint` | ✅ Done | Undo last vertex (deletes wall mesh) |
| `IA_OrthoLock` | `OnOrthoLock` | ✅ Done | Hold for 90° angles only |
| `IA_AngleSnap` | `OnAngleSnap` | ✅ Done | Toggle 45° angle snap |

### Priority 4: Numeric Entry

| Action | Handler | Status | Notes |
|--------|---------|--------|-------|
| `IA_NumericDigit` | `OnNumericDigit` | ✅ Done | Parse scalar value to digit |
| `IA_NumericDecimal` | `OnNumericDecimal` | ✅ Done | Add decimal point |
| `IA_NumericBackspace` | `OnNumericBackspace` | ✅ Done | Remove last char / RemoveLastPoint when empty |
| `IA_NumericCommit` | `OnNumericCommit` | ✅ Done | Commit value to tool |
| `IA_NumericCancel` | `OnNumericCancel` | ✅ Done | Cancel numeric entry |
| `IA_NumericSwitchField` | `OnNumericSwitchField` | ✅ Done | Toggle length/angle |
| `IA_NumericCycleUnits` | `OnNumericCycleUnits` | ✅ Done | Cycle cm/m/in/ft (preserves length) |
| `IA_NumericAdd` | `OnNumericOperator` | ⏳ Pending | Future: Calculator |
| `IA_NumericSubtract` | `OnNumericOperator` | ⏳ Pending | Future: Calculator |
| `IA_NumericMultiply` | `OnNumericOperator` | ⏳ Pending | Future: Calculator |
| `IA_NumericDivide` | `OnNumericOperator` | ⏳ Pending | Future: Calculator |

### Priority 5: 3D Navigation Views

| Action | Handler | Status | Notes |
|--------|---------|--------|-------|
| `IA_ViewTop` | `OnViewTop` | ⏳ Pending | Set camera to top view |
| `IA_ViewFront` | `OnViewFront` | ⏳ Pending | Set camera to front view |
| `IA_ViewRight` | `OnViewRight` | ⏳ Pending | Set camera to right view |
| `IA_ViewPerspective` | `OnViewPerspective` | ⏳ Pending | Toggle ortho/perspective |

---

## Implementation Checklist

### Step 1: Wire Input Actions to Handlers in PlayerController

```
[x] Add BindAction calls for all new Input Actions in SetupInputComponent()
[x] Create handler functions for each action (Started/Completed/Triggered as needed)
[x] Test each binding individually
```

### Step 2: Implement Tool Switching
```
[x] IA_ToolSelect → ToolManager->SelectToolByType(ERTPlanToolType::Select)
[x] IA_ToolLine → ToolManager->SelectToolByType(ERTPlanToolType::Line)
[x] IA_ToolPolyLine → ToolManager->SelectToolByType(ERTPlanToolType::Polyline)
[x] Update active IMC when tool changes
```

### Step 3: Implement View Controls
```
[x] IA_ToggleView → CameraController->ToggleViewMode()
[x] IA_Pan/IA_PanDelta → CameraController pan logic (1:1 cursor tracking with interpolation)
[x] IA_Zoom → CameraController zoom logic
[x] IA_Orbit/IA_OrbitDelta → CameraController orbit logic (3D mode)
[x] IA_Move → WASD fly movement (axis corrected: Y=forward/back, X=right/left)
[x] IA_AdjustFlySpeed → RMB+Scroll adjusts fly speed
[x] IA_ResetView → CameraController->ResetView()
```

### Step 4: Implement Selection Actions
```
[ ] Route IA_Select to SelectTool
[ ] Implement marquee box selection flow
[ ] Implement SelectAll/DeselectAll
[ ] Test modifier key combinations (Shift+Click, Alt+Click)
```

### Step 5: Implement Drawing Actions
```
[x] Route IA_DrawPlacePoint to active drawing tool
[x] Implement IA_DrawCancel (RMB/Escape) - calls LineTool->CancelDrawing()
[x] Implement IA_DrawConfirm (Enter to finish polyline)
[x] Implement IA_DrawClose (close polygon) - calls LineTool->ClosePolyline()
[x] Implement IA_DrawRemoveLastPoint (Backspace in polyline) - deletes wall mesh
[x] Test OrthoLock (Shift held = 90° only) and AngleSnap (toggle = 45° increments)
```

### Step 6: Configure IMC Key Bindings in Editor
```
[x] Open each IMC asset and add key bindings per TASKS_CHECKLIST.md tables
[x] Configure Chorded Actions for modifier combos (Ctrl+Z, etc.)
[x] Configure Scalar modifiers for IA_NumericDigit (0-9)
[x] Test IMC priority layering
```

### Step 7: Update DA_ArchVisInput Data Asset
```
[x] Assign all IMC references
[x] Assign all IA references
[x] Set default values (MouseSensitivity, SnapModifierMode, etc.)
```

---

## Next Major Milestones

### Milestone 1: Full Input Functionality (Current)
- All input actions wired and functional
- Tool switching via hotkeys (V, L, P)
- Complete 2D drafting workflow
- Numeric input fully integrated

### Milestone 2: 2D Visualization
- Draw existing walls in 2D plan view
- Draw openings (door arcs, windows)
- Selection highlight visualization
- Marquee rectangle during drag

### Milestone 3: Room Detection
- Graph structure for wall connectivity
- Cycle detection algorithm
- Auto-detect closed rooms
- Floor/ceiling mesh generation

### Milestone 4: Property Editing UI
- Spawn toolbar widget
- Property panel for selected items
- Wall thickness/height editing
- Material assignment

### Milestone 5: Networking Polish
- Route tool commands through NetDriver
- Server authority for edits
- Permission system (Authoring vs Viewer)

---

## File Reference

### Key Source Files
| File | Purpose |
|------|---------|
| `Source/ArchVis/Private/ArchVisPlayerController.cpp` | Input handling, IMC management, Pawn switching |
| `Source/ArchVis/Public/ArchVisInputConfig.h` | Input config data asset class |
| `Source/ArchVis/Private/ArchVisHUD.cpp` | HUD drawing, crosshair, measurements |
| `Source/ArchVis/Private/ArchVisPawnBase.cpp` | Base pawn with camera controls |
| `Source/ArchVis/Private/ArchVisDraftingPawn.cpp` | 2D orthographic drafting pawn |
| `Source/ArchVis/Private/ArchVisOrbitPawn.cpp` | 3D perspective orbit pawn |
| `Source/ArchVis/Private/ArchVisFirstPersonPawn.cpp` | First-person walkthrough character |
| `Source/ArchVis/Private/ArchVisThirdPersonPawn.cpp` | Third-person walkthrough character |
| `Source/ArchVis/Private/ArchVisCameraController.cpp` | Legacy camera modes (deprecated) |
| `Plugins/RTPlanTools/Private/Tools/RTPlanLineTool.cpp` | Line/Polyline tool |
| `Plugins/RTPlanTools/Private/Tools/RTPlanSelectTool.cpp` | Selection tool |
| `Plugins/RTPlanTools/Private/RTPlanToolManager.cpp` | Tool lifecycle management |

### Pawn Architecture
| Pawn Class | Type | Features |
|------------|------|----------|
| `AArchVisPawnBase` | Abstract base | Spring arm, camera, pan/zoom/orbit interface |
| `AArchVisDraftingPawn` | 2D Drafting | Top-down orthographic, fixed rotation |
| `AArchVisOrbitPawn` | 3D Orbit | Perspective, orbit around target point |
| `AArchVisFirstPersonPawn` | First Person | Character-based, WASD + mouselook |
| `AArchVisThirdPersonPawn` | Third Person | Character-based, camera boom, zoom |
| `ARTPlanVRPawn` | VR (plugin) | Motion controllers, teleport locomotion |

### Key Content Assets
| Asset | Purpose |
|-------|---------|
| `Content/Input/DA_ArchVisInput` | Input configuration data asset |
| `Content/Input/IMC_*` | Input Mapping Contexts |
| `Content/Input/Actions/*` | Input Actions |
| `Content/Core/BP_ArchVisController` | PlayerController blueprint |
| `Content/Core/BP_ArchVisGameMode` | GameMode blueprint |

---

## Archived Plans

The following documents have been consolidated into this master plan:
- `ArchPlan_Refined.md` - Original refined implementation plan
- `plan-archVis.prompt.md` - Project understanding report
- `plan-uiAndSelectionTool.prompt.md` - UI and Selection Tool plan

The detailed task checklist remains in `TASKS_CHECKLIST.md` for granular tracking.

