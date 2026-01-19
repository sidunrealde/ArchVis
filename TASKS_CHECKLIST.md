# ArchVis Task Checklist

This checklist tracks implementation progress against the drafted phases in `plan-archVis.prompt.md` / `ArchPlan_Refined.md`.

Legend: [x] done, [-] partial, [ ] pending

---

## Known Issues / Bugs (Updated 2026-01-19)

| Issue | Status | Description |
|-------|--------|-------------|
| Selection uses raw input | 🔴 Open | `OnSelectStarted` uses `IsInputKeyDown()` for Shift/Ctrl/Alt instead of Enhanced Input actions |
| Marquee selection broken | 🔴 Open | Box/marquee selection not working in 2D mode - world positions not updating correctly |
| SelectAll not working | 🔴 Open | `IA_SelectAll` handler exists but doesn't trigger `SelectTool->SelectAll()` |
| DeselectAll not working | 🔴 Open | `IA_DeselectAll` handler exists but doesn't trigger `SelectTool->ClearSelection()` |
| FocusSelection broken | 🔴 Open | `IA_FocusSelection` not working properly in both 2D and 3D mode |
| 3D selection issues | ⚠️ Partial | Line trace may fail in some cases, cursor can snap to horizon |

### Selection System Refactor Needed
The current selection implementation in `AArchVisPlayerController::OnSelectStarted()` uses raw keyboard input:
```cpp
bool bShiftHeld = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
bool bCtrlHeld = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
bool bAltHeld = IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt);
```

This should be refactored to use Enhanced Input entirely:
- `IA_SelectAdd` (Shift+LMB) should fire independently as a chorded action
- `IA_SelectToggle` (Ctrl+LMB) should fire independently as a chorded action
- `IA_SelectRemove` (Alt+LMB) should fire independently as a chorded action
- `IA_Select` (plain LMB) should only fire when no modifiers are held

---

## Phase 0: Baseline plumbing (game ↔ plugins)
- [x] `AArchVisGameMode` creates `URTPlanDocument`.
- [x] `AArchVisGameMode` creates `URTPlanToolManager` and initializes it with the document.
- [x] `AArchVisGameMode` spawns `ARTPlanShellActor` and links the document.
- [x] `AArchVisGameMode` spawns `ARTPlanNetDriver` and links the document.

---

## Phase 1: Viewport & Input (The "CAD Feel")
### 1.1 Camera/Pawn System: 2D/3D switching logic
- [x] Top-down orthographic mode (`AArchVisDraftingPawn`).
- [x] Perspective orbit mode (`AArchVisOrbitPawn`).
- [x] Toggle between 2D/3D pawns (`SwitchToPawnType()`).
- [x] Pan + zoom on 2D pawn (AutoCAD-style, 1:1 cursor-to-scene movement).
- [x] Pan + zoom on 3D pawn (via `UOrbitInputComponent`).
- [x] Orbit controls on 3D pawn (Alt+LMB via `UOrbitInputComponent`).
- [x] Fly mode on 3D pawn (RMB+WASD via `UOrbitInputComponent`).
- [x] WASD movement axis correctly mapped (W/S=forward/back, A/D=left/right).
- [x] Adjustable fly speed via RMB+Scroll (`IA_AdjustFlySpeed`).
- [x] First-person walkthrough pawn (`AArchVisFirstPersonPawn`).
- [x] Third-person walkthrough pawn (`AArchVisThirdPersonPawn`).
- [x] Base pawn class with common camera interface (`AArchVisPawnBase`).
- [x] Camera state transfer when switching pawns (`SetCameraTransform()`).
- [x] Undo/Redo wired to Document (`OnUndo`, `OnRedo`).

### 1.1b Modular Input Components (Refactored Jan 2026)
**Architecture:** Pawn-centric input with components instead of monolithic controller.

**Created Files:**
- [x] `Source/ArchVis/Public/Input/ArchVisInputComponent.h/.cpp` - Abstract base
- [x] `Source/ArchVis/Public/Input/DraftingInputComponent.h/.cpp` - 2D navigation
- [x] `Source/ArchVis/Public/Input/OrbitInputComponent.h/.cpp` - 3D navigation
- [x] `Source/ArchVis/Public/Input/ToolInputComponent.h/.cpp` - Tool actions

**Implementation Status:**
- [x] `UArchVisInputComponent` - Abstract base class for pawn input components.
- [x] `UDraftingInputComponent` - 2D pan/zoom input, attached to `AArchVisDraftingPawn`.
- [x] `UOrbitInputComponent` - 3D orbit/pan/dolly/fly input, attached to `AArchVisOrbitPawn`.
- [x] `UToolInputComponent` - Tool actions (draw/select), attached to `AArchVisPlayerController`.
- [ ] `UFirstPersonInputComponent` - FPS movement, for `AArchVisFirstPersonPawn`.
- [ ] `UThirdPersonInputComponent` - TPS camera/movement, for `AArchVisThirdPersonPawn`.

**Integration:**
- [x] Pawns create their own input components in constructor.
- [x] Pawns override `PossessedBy()`/`UnPossessed()` to notify input components.
- [x] Pawns call `InitializeInput()` with InputConfig from controller.
- [x] Controller initializes pawn input in `BeginPlay()` and `SwitchToPawnType()`.
- [x] Input components manage their own IMCs on possession/unpossession.
- [x] Mouse lock/unlock handled per-pawn in `UpdateMouseLockState()`.

**Cleanup (TODO):**
- [ ] Remove redundant navigation handlers from `AArchVisPlayerController` (OnPan, OnPanDelta, OnOrbit, etc.)
- [ ] Remove redundant Tick navigation logic from controller
- [ ] Migrate remaining tool handlers from controller to `UToolInputComponent`
- [ ] Remove `UpdateInputMappingContexts()` complexity from controller
- [x] Test 2D drafting with new `UDraftingInputComponent` (1:1 pan working)
- [x] Test 3D navigation with new `UOrbitInputComponent` (orbit/pan/fly/WASD working)

### 1.2 HUD: full-screen crosshair cursor
- [x] Crosshair lines (full screen).
- [x] Uses active tool's snapped cursor position when available.
- [x] Solid wall preview line during drafting.
- [x] Dashed measurement line parallel to wall with end ticks.
- [x] Dashed arc for angle visualization at start point.
- [x] Length label with units displayed near the measurement line.
- [x] Angle label with degree symbol near the arc.
- [x] Active/inactive input field color differentiation (blue=active, gray=inactive).
- [-] 2D plan visualization (existing walls/openings) is not yet drawn here.

### 1.3 Input handling: virtual cursor + sensitivity + numeric buffer
- [x] Virtual cursor implemented.
- [x] Virtual cursor Y movement matches mouse movement.
- [x] Mouse sensitivity scalar (`AArchVisPlayerController::MouseSensitivity`) added.
- [x] Mouse sensitivity applied to virtual cursor movement.
- [x] Numeric input buffer struct (`FRTNumericInputBuffer`) with parsing.
- [x] Unit enum (`ERTLengthUnit`) with cm/m/in/ft and conversion to cm.
- [x] Input action slots added to `UArchVisInputConfig` for numeric entry.
- [x] Numeric input handlers in `AArchVisPlayerController` (digit, decimal, backspace, commit, clear, switch field, cycle units).
- [x] Enter-to-commit numeric input into active tool (`OnNumericInput`).
- [x] Tab-to-switch numeric fields (length/angle).
- [x] Unit cycling via keyboard action.
- [x] Real-time preview update while typing numeric values (`UpdatePreviewFromLength`, `UpdatePreviewFromAngle`).
- [ ] Hook `MouseSensitivity` to a settings source (config / `UGameUserSettings` / settings subsystem).

### 1.4 Modular Input Mapping Context System
- [x] **Hierarchical IMC Architecture:**
  - IMC_Global (Priority 0 - Always active)
  - IMC_2D_Base / IMC_3D_Base (Priority 1 - Mode-specific)
  - IMC_2D_Selection / IMC_2D_LineTool / IMC_2D_PolylineTool (Priority 2 - Tool-specific)
  - IMC_3D_Selection / IMC_3D_Navigation (Priority 2 - 3D tool contexts)
  - IMC_NumericEntry (Priority 3 - Layered during numeric input)
- [x] `UArchVisInputConfig` data asset with all IMC and IA slots defined.
- [x] `AddMappingContext()` / `RemoveMappingContext()` helpers implemented.
- [x] `UpdateInputMappingContexts()` for dynamic context switching.
- [x] `SetInteractionMode()` for 2D/3D mode switching.
- [x] `OnToolChanged()` callback to update IMC when tool changes.
- [x] `SwitchTo2DToolMode()` for switching between 2D tools.
- [x] `ActivateNumericEntryContext()` / `DeactivateNumericEntryContext()` for numeric overlay.
- [x] Priority constants: `IMC_Priority_Global=0`, `IMC_Priority_ModeBase=1`, `IMC_Priority_Tool=2`, `IMC_Priority_NumericEntry=3`.
- [x] **Global input handlers implemented:** OnUndo, OnRedo, OnDelete, OnEscape, OnSave.
- [x] **Modifier key handlers implemented:** OnModifierCtrl/Shift/Alt Started/Completed.
- [x] **Camera ResetView() function added.**
- [x] **Create IMC assets in editor** - All IMC assets created in `Content/Input/`.
- [x] **Create Input Action assets in editor** - All IA assets created in `Content/Input/Actions/`.
- [ ] **Configure key bindings in each IMC asset** (see Editor Setup section below).
- [ ] **Update DA_ArchVisInput** data asset with all new actions and contexts.

---

## Phase 2: Wall Tool 2.0 (Click-Move-Click)
### 2.1 Line/Polyline tool state machine
- [x] Click-move-click workflow implemented (`URTPlanLineTool`).
- [x] Polyline mode toggle exists (`SetPolylineMode`).
- [x] Polyline close functionality (`ClosePolyline()` - connects to origin).
- [x] Remove last point in polyline (`RemoveLastPoint()` - deletes wall mesh).
- [x] Cancel drawing (`CancelDrawing()` - discards current segment).

### 2.2 Snapping + ortho
- [x] Hard snapping to endpoints/midpoints.
- [x] Projection snapping to wall segments.
- [x] Soft alignment snapping (align X/Y to existing geometry).
- [-] Explicit grid snapping (as a first-class mode) not implemented.
- [x] Soft ortho snap (snaps to 0/90/180/270 within threshold, allows free angle otherwise).
- [x] Ortho snap threshold configurable (`OrthoSnapThreshold` property).
- [x] Modifier key to toggle/hold snap on/off (`IA_SnapModifier`).
- [x] Snap modifier mode setting (Toggle / HoldToDisable / HoldToEnable).
- [x] Ortho lock (Shift held = force 90° angles only) via `ApplyOrthoLock()`.
- [x] Angle snap (toggle = snap to 45° increments) via `ApplyAngleSnap()`.
- [x] `bOrthoLockActive` and `bAngleSnapEnabled` passed via `FRTPointerEvent`.

### 2.3 Numeric entry hook-up
- [x] Tool supports numeric input commit (`URTPlanLineTool::OnNumericInput`).
- [x] Input system feeds numeric values to the tool via `CommitNumericInput()`.
- [x] Drafting state exposed (`FRTDraftingState`) for HUD visualization.
- [x] Current length/angle cached and exposed via getters.
- [x] Angle clamped to 0-180 degrees.
- [x] Length/angle display with 2 decimal precision.
- [x] Unit cycling preserves line length (converts display value).
- [x] Backspace repeat support (hold to delete multiple chars).
- [x] Backspace delegates to RemoveLastPoint when buffer empty in polyline mode.

### 2.4 Cancel / finish behaviors
- [x] Escape / cancel via `OnDrawCancel` → `CancelDrawing()`.
- [x] Enter to confirm/finish polyline via `OnDrawConfirm`.
- [x] Close polygon loop via `OnDrawClose` → `ClosePolyline()`.
- [x] Backspace to remove last point via `RemoveLastPoint()` (deletes wall from document).

---

## Phase 3: 2D Visualization
### 3.1 Plan view renderer
- [ ] Draw existing walls in 2D (screen space) in plan view.
- [ ] Draw openings (door swing arcs, window markers) in 2D.

### 3.2 Overlay dimensions
- [x] Render current line length as text (during drafting).
- [x] Render current line angle as text (during drafting).
- [ ] Render existing wall lengths/dimensions as text.

---

## Phase 4: Room Detection & Generation
### 4.1 Room detection
- [ ] Graph/cycle detection for rooms.
- [ ] Add `FRTRoom` to schema or implement derived room data.

### 4.2 Floors/ceilings generation
- [ ] Triangulate room polygons.
- [ ] Generate floors.
- [ ] Generate ceilings.

---

## Phase 5: Properties & Editing
### 5.1 Selection
- [x] Add modifier key fields to FRTPointerEvent (bShiftDown, bAltDown, bCtrlDown).
- [x] Add IA_SelectionAddModifier and IA_SelectionRemoveModifier to InputConfig.
- [x] Track modifier key states in PlayerController.
- [x] Pass modifier states in GetPointerEvent().
- [x] Add HitTestWall() to SpatialIndex.
- [x] Add HitTestWallsInRect() to SpatialIndex.
- [x] Add HitTestOpening() to SpatialIndex.
- [x] Add HitTestOpeningsInRect() to SpatialIndex.
- [x] Create RTPlanSelectTool header.
- [x] Create RTPlanSelectTool implementation.
- [x] Implement single-click selection.
- [x] Implement Shift+click add to selection.
- [x] Implement Alt+click remove from selection.
- [x] Implement marquee drag selection.
- [ ] Selection visualization (highlight selected walls in HUD).
- [ ] Draw marquee rectangle during drag in HUD.

### 5.2 Property UI
- [x] `RTPlanUI` widgets exist (toolbar/properties/catalog browser).
- [x] ERTPlanToolType enum added (None, Select, Line, Polyline).
- [x] ToolManager SelectToolByType() method added.
- [x] Toolbar SelectToolByType() and GetActiveToolType() added.
- [ ] Create WBP_Toolbar widget blueprint with Select/Line/Polyline buttons.
- [ ] Wire toolbar into runtime (spawn widget, bind to tool manager).
- [ ] Property panel for selected items.

### 5.3 Commands for edits
- [x] Commands: add vertex, add wall, delete wall.
- [ ] Command: update wall (thickness/height/finishes) for property edits.
- [ ] Commands for openings.
- [ ] Commands for objects.
- [ ] Commands for runs.

---

## Networking (cross-cutting)
- [x] Replicate plan as JSON string (`ReplicatedPlanJson`) baseline.
- [-] RPCs exist for add/delete wall, but tools currently submit commands locally (not routed via net driver).
- [ ] Permissions (Authoring vs Viewer).
- [ ] Partial replication (FastArray or delta updates).
- [ ] Shared interaction replication (object move/locks).

---

## 3D Shell (cross-cutting)
- [x] Walls generated into `UDynamicMeshComponent`.
- [x] Wall splitting for openings via `RTPlanOpenings` utilities.
- [ ] Floors/ceilings.
- [ ] Materials/finishes applied to left/right/caps.

---

## Editor Setup Required

After compiling, create the following Enhanced Input assets in the Unreal Editor.

### STEP 1: Create Input Actions (Content/Input/Actions/)

Create each Input Action as a new asset (Right-click → Input → Input Action).

#### Global Actions (Always Available)
| Asset Name | Value Type | Description |
|------------|------------|-------------|
| `IA_ModifierCtrl` | Digital (Bool) | Ctrl key held |
| `IA_ModifierShift` | Digital (Bool) | Shift key held |
| `IA_ModifierAlt` | Digital (Bool) | Alt key held |
| `IA_Undo` | Digital (Bool) | Undo command |
| `IA_Redo` | Digital (Bool) | Redo command |
| `IA_Delete` | Digital (Bool) | Delete selection |
| `IA_Escape` | Digital (Bool) | Cancel/Escape |
| `IA_ToggleView` | Digital (Bool) | Toggle 2D/3D view |
| `IA_Save` | Digital (Bool) | Save document |
| `IA_ToolSelect` | Digital (Bool) | Switch to Select tool |
| `IA_ToolLine` | Digital (Bool) | Switch to Line tool |
| `IA_ToolPolyline` | Digital (Bool) | Switch to Polyline tool |

#### View/Navigation Actions
| Asset Name | Value Type | Description |
|------------|------------|-------------|
| `IA_Pan` | Digital (Bool) | Pan mode toggle (MMB held) |
| `IA_PanDelta` | Axis2D (Vector2D) | Pan movement delta |
| `IA_Zoom` | Axis1D (Float) | Zoom in/out |
| `IA_Orbit` | Digital (Bool) | Orbit mode toggle (3D) |
| `IA_OrbitDelta` | Axis2D (Vector2D) | Orbit movement delta |
| `IA_ResetView` | Digital (Bool) | Reset view to default |
| `IA_FocusSelection` | Digital (Bool) | Focus on selection |
| `IA_PointerPosition` | Axis2D (Vector2D) | Mouse position tracking |
| `IA_SnapToggle` | Digital (Bool) | Toggle snap on/off |
| `IA_GridToggle` | Digital (Bool) | Toggle grid visibility |

#### Selection Actions
| Asset Name | Value Type | Description |
|------------|------------|-------------|
| `IA_Select` | Digital (Bool) | Select object |
| `IA_SelectAdd` | Digital (Bool) | Add to selection (Shift+Click) |
| `IA_SelectToggle` | Digital (Bool) | Toggle selection (Ctrl+Click) |
| `IA_SelectAll` | Digital (Bool) | Select all |
| `IA_DeselectAll` | Digital (Bool) | Deselect all |
| `IA_BoxSelectStart` | Digital (Bool) | Start box selection |
| `IA_BoxSelectDrag` | Axis2D (Vector2D) | Box selection drag |
| `IA_BoxSelectEnd` | Digital (Bool) | End box selection |
| `IA_CycleSelection` | Digital (Bool) | Cycle through overlapping |

#### Drawing Actions
| Asset Name | Value Type | Description |
|------------|------------|-------------|
| `IA_DrawPlacePoint` | Digital (Bool) | Place vertex |
| `IA_DrawConfirm` | Digital (Bool) | Finish polyline |
| `IA_DrawCancel` | Digital (Bool) | Cancel drawing |
| `IA_DrawClose` | Digital (Bool) | Close polygon |
| `IA_DrawRemoveLastPoint` | Digital (Bool) | Remove last vertex |
| `IA_OrthoLock` | Digital (Bool) | Lock to orthogonal |
| `IA_AngleSnap` | Digital (Bool) | Toggle 45° snap |

#### Numeric Entry Actions
| Asset Name | Value Type | Description |
|------------|------------|-------------|
| `IA_NumericDigit` | **Axis1D (Float)** | Single action for all digits 0-9. Use Scalar modifier on each key binding. |
| `IA_NumericDecimal` | Digital (Bool) | Decimal point |
| `IA_NumericBackspace` | Digital (Bool) | Backspace |
| `IA_NumericCommit` | Digital (Bool) | Commit value (Enter) |
| `IA_NumericCancel` | Digital (Bool) | Cancel entry |
| `IA_NumericSwitchField` | Digital (Bool) | Switch Length/Angle (Tab) |
| `IA_NumericCycleUnits` | Digital (Bool) | Cycle units |
| `IA_NumericAdd` | Digital (Bool) | Addition operator |
| `IA_NumericSubtract` | Digital (Bool) | Subtraction operator |
| `IA_NumericMultiply` | Digital (Bool) | Multiplication operator |
| `IA_NumericDivide` | Digital (Bool) | Division operator |

#### 3D Navigation Actions
| Asset Name | Value Type | Description |
|------------|------------|-------------|
| `IA_ViewTop` | Digital (Bool) | Top view |
| `IA_ViewFront` | Digital (Bool) | Front view |
| `IA_ViewRight` | Digital (Bool) | Right view |
| `IA_ViewPerspective` | Digital (Bool) | Toggle ortho/perspective |

---

### STEP 2: Create Input Mapping Contexts (Content/Input/)

Create each IMC as a new asset (Right-click → Input → Input Mapping Context).

#### IMC_Global (Priority 0 - Always Active)
This context is **always** active regardless of mode or tool.

| Action | Key Binding | Modifiers |
|--------|-------------|-----------|
| `IA_ModifierCtrl` | Left Ctrl | - |
| `IA_ModifierShift` | Left Shift | - |
| `IA_ModifierAlt` | Left Alt | - |
| `IA_Escape` | Escape | - |
| `IA_ToggleView` | Tab | - |
| `IA_Delete` | Delete | - |
| `IA_Undo` | Z | Chorded Action: `IA_ModifierCtrl` |
| `IA_Redo` | Y | Chorded Action: `IA_ModifierCtrl` |
| `IA_Save` | S | Chorded Action: `IA_ModifierCtrl` |
| `IA_ToolSelect` | V | - |
| `IA_ToolLine` | L | - |
| `IA_ToolPolyline` | P | - |

---

#### IMC_2D_Base (Priority 1 - Active in 2D Mode)
Common 2D drafting controls.

**Note:** Pan uses Chorded Action with `IA_Pan` (MMB held) as the chord. This means `IA_PanDelta` only triggers while MMB is held.

| Action | Key Binding | Modifiers |
|--------|-------------|-----------|
| `IA_Pan` | Middle Mouse Button | - |
| `IA_PanDelta` | Mouse XY 2D-Axis | Chorded Action: `IA_Pan` |
| `IA_Zoom` | Mouse Wheel Axis | - |
| `IA_PointerPosition` | Mouse XY 2D-Axis | - |
| `IA_ResetView` | Home | - |
| `IA_FocusSelection` | F | - |
| `IA_SnapToggle` | S | - |
| `IA_GridToggle` | G | - |

---

#### IMC_3D_Base (Priority 1 - Active in 3D Mode)
Common 3D navigation controls (Unreal Editor-style).

**Navigation Controls:**
- **MMB + Drag**: Pan in camera plane
- **Scroll Wheel**: Zoom in/out
- **Alt + LMB + Drag**: Orbit around target point
- **RMB + Drag**: Look around (FPS-style)
- **RMB + WASD**: Fly movement
- **RMB + Q/E**: Fly up/down
- **Alt + RMB + Drag**: Dolly zoom

| Action | Key Binding | Modifiers |
|--------|-------------|-----------|
| `IA_Pan` | Middle Mouse Button | - |
| `IA_PanDelta` | Mouse XY 2D-Axis | Chorded Action: `IA_Pan` |
| `IA_Zoom` | Mouse Wheel Axis | - |
| `IA_Orbit` | Left Mouse Button | Chorded Action: `IA_ModifierAlt` |
| `IA_OrbitDelta` | Mouse XY 2D-Axis | Chorded Action: `IA_Orbit` |
| `IA_FlyMode` | Right Mouse Button | - |
| `IA_Look` | Mouse XY 2D-Axis | Chorded Action: `IA_FlyMode` |
| `IA_Move` (W) | W | Chorded Action: `IA_FlyMode` |
| `IA_Move` (S) | S | Chorded Action: `IA_FlyMode`, Negate Y |
| `IA_Move` (A) | A | Chorded Action: `IA_FlyMode`, Swizzle YXZ, Negate |
| `IA_Move` (D) | D | Chorded Action: `IA_FlyMode`, Swizzle YXZ |
| `IA_MoveUp` | E | Chorded Action: `IA_FlyMode` |
| `IA_MoveDown` | Q | Chorded Action: `IA_FlyMode` |
| `IA_DollyZoom` | Mouse Y | Chorded Action: `IA_ModifierAlt`, Chorded Action: `IA_FlyMode` |
| `IA_ResetView` | Home | - |
| `IA_FocusSelection` | F | - |

---

#### IMC_2D_Selection (Priority 2 - Active when Select Tool in 2D)
Selection tool controls.

| Action | Key Binding | Modifiers |
|--------|-------------|-----------|
| `IA_Select` | Left Mouse Button | - |
| `IA_SelectAdd` | Left Mouse Button | Chorded Action: `IA_ModifierShift` |
| `IA_SelectToggle` | Left Mouse Button | Chorded Action: `IA_ModifierCtrl` |
| `IA_BoxSelectStart` | Left Mouse Button | Hold Time: 0.15s |
| `IA_BoxSelectDrag` | Mouse XY 2D-Axis | - |
| `IA_BoxSelectEnd` | Left Mouse Button (Released) | - |
| `IA_SelectAll` | A | Chorded Action: `IA_ModifierCtrl` |
| `IA_DeselectAll` | D | Chorded Action: `IA_ModifierCtrl` |
| `IA_CycleSelection` | Spacebar | - |

---

#### IMC_2D_LineTool (Priority 2 - Active when Line Tool in 2D)
Line drawing tool controls.

| Action | Key Binding | Modifiers |
|--------|-------------|-----------|
| `IA_DrawPlacePoint` | Left Mouse Button | - |
| `IA_DrawCancel` | Right Mouse Button | - |
| `IA_DrawCancel` | Escape | - |
| `IA_OrthoLock` | Left Shift | - |
| `IA_AngleSnap` | A | - |

---

#### IMC_2D_PolylineTool (Priority 2 - Active when Polyline Tool in 2D)
Polyline drawing tool controls.

| Action | Key Binding | Modifiers |
|--------|-------------|-----------|
| `IA_DrawPlacePoint` | Left Mouse Button | - |
| `IA_DrawConfirm` | Enter | - |
| `IA_DrawCancel` | Escape | - |
| `IA_DrawCancel` | Right Mouse Button | - |
| `IA_DrawClose` | C | - |
| `IA_DrawRemoveLastPoint` | Backspace | - |
| `IA_OrthoLock` | Left Shift | - |
| `IA_AngleSnap` | A | - |

---

#### IMC_NumericEntry (Priority 3 - Layered During Numeric Input)
Numeric input controls - layered on top when user starts typing numbers.

**Note:** `IA_NumericDigit` is a **1D Axis** action. Each key uses a **Scalar** modifier to output its digit value (1-9 for digits 1-9, value 10 for digit 0 which maps to 0 in code).

| Action | Key Binding | Modifiers |
|--------|-------------|-----------|
| `IA_NumericDigit` | One | Scalar: 1.0 |
| `IA_NumericDigit` | Numpad One | Scalar: 1.0 |
| `IA_NumericDigit` | Two | Scalar: 2.0 |
| `IA_NumericDigit` | Numpad Two | Scalar: 2.0 |
| `IA_NumericDigit` | Three | Scalar: 3.0 |
| `IA_NumericDigit` | Numpad Three | Scalar: 3.0 |
| `IA_NumericDigit` | Four | Scalar: 4.0 |
| `IA_NumericDigit` | Numpad Four | Scalar: 4.0 |
| `IA_NumericDigit` | Five | Scalar: 5.0 |
| `IA_NumericDigit` | Numpad Five | Scalar: 5.0 |
| `IA_NumericDigit` | Six | Scalar: 6.0 |
| `IA_NumericDigit` | Numpad Six | Scalar: 6.0 |
| `IA_NumericDigit` | Seven | Scalar: 7.0 |
| `IA_NumericDigit` | Numpad Seven | Scalar: 7.0 |
| `IA_NumericDigit` | Eight | Scalar: 8.0 |
| `IA_NumericDigit` | Numpad Eight | Scalar: 8.0 |
| `IA_NumericDigit` | Nine | Scalar: 9.0 |
| `IA_NumericDigit` | Numpad Nine | Scalar: 9.0 |
| `IA_NumericDigit` | Zero | Scalar: 10.0 (maps to digit 0) |
| `IA_NumericDigit` | Numpad Zero | Scalar: 10.0 (maps to digit 0) |
| `IA_NumericDecimal` | Period | - |
| `IA_NumericDecimal` | Numpad Decimal | - |
| `IA_NumericBackspace` | Backspace | - |
| `IA_NumericCommit` | Enter | - |
| `IA_NumericCommit` | Numpad Enter | - |
| `IA_NumericCancel` | Escape | - |
| `IA_NumericSwitchField` | Tab | - |
| `IA_NumericCycleUnits` | U | - |
| `IA_NumericAdd` | Add (Numpad +) | - |
| `IA_NumericSubtract` | Subtract (Numpad -) | - |
| `IA_NumericMultiply` | Multiply (Numpad *) | - |
| `IA_NumericDivide` | Divide (Numpad /) | - |

---

#### IMC_3D_Selection (Priority 2 - Active in 3D for Selection)
3D selection controls.

| Action | Key Binding | Modifiers |
|--------|-------------|-----------|
| `IA_Select` | Left Mouse Button | - |
| `IA_SelectAdd` | Left Mouse Button | Chorded Action: `IA_ModifierShift` |
| `IA_SelectToggle` | Left Mouse Button | Chorded Action: `IA_ModifierCtrl` |

---

#### IMC_3D_Navigation (Priority 2 - Active in 3D for View Control)
3D view navigation controls.

| Action | Key Binding | Modifiers |
|--------|-------------|-----------|
| `IA_ViewTop` | Numpad Seven | - |
| `IA_ViewFront` | Numpad One | - |
| `IA_ViewRight` | Numpad Three | - |
| `IA_ViewPerspective` | Numpad Five | - |

---

### STEP 3: Update Data Asset (DA_ArchVisInput)

Open the `DA_ArchVisInput` data asset (or create one based on `UArchVisInputConfig`) and assign:

1. **All Input Mapping Contexts** to their corresponding slots:
   - `IMC_Global`
   - `IMC_2D_Base`
   - `IMC_2D_Selection`
   - `IMC_2D_LineTool`
   - `IMC_2D_PolylineTool`
   - `IMC_NumericEntry`
   - `IMC_3D_Base`
   - `IMC_3D_Selection`
   - `IMC_3D_Navigation`

2. **All Input Actions** to their corresponding slots (organized by category in the data asset).

---

### STEP 4: Configure BP_ArchVisController

In the Blueprint derived from `AArchVisPlayerController`:

| Property | Description |
|----------|-------------|
| `InputConfig` | Assign `DA_ArchVisInput` |
| `MouseSensitivity` | Default: 2.0 (adjust for feel) |
| `DefaultUnit` | Default unit for numeric input (Centimeters) |
| `SnapModifierMode` | How snap modifier behaves (HoldToDisable recommended) |
| `bSnapEnabledByDefault` | Whether snap is on by default (true) |

---

### How the System Works

1. **Priority System**: Higher priority contexts are processed first. If a key is bound in a higher-priority context, it "consumes" the input.

2. **Context Layering**:
   - `IMC_Global` (0) - Always active
   - `IMC_2D_Base` or `IMC_3D_Base` (1) - Based on current view mode
   - Tool-specific IMC (2) - Based on active tool
   - `IMC_NumericEntry` (3) - Layered on top when typing numbers

3. **Chorded Actions**: For key combinations like Ctrl+Z, bind Z to `IA_Undo` with a "Chorded Action" modifier referencing `IA_ModifierCtrl`.

4. **Numeric Entry Activation**: When user starts typing digits during drawing, `IMC_NumericEntry` is automatically layered on top to capture all numeric input.

5. **Dynamic Switching**: The controller automatically adds/removes contexts when:
   - Toggling between 2D/3D mode
   - Switching tools (Select, Line, Polyline)
   - Starting/ending numeric input

---

### Snap Modifier Settings (on BP_ArchVisController)

| Property | Description |
|----------|-------------|
| `SnapModifierMode` | **Toggle**: Press to toggle snap on/off. **HoldToDisable**: Snap on by default, hold to disable. **HoldToEnable**: Snap off by default, hold to enable. |
| `bSnapEnabledByDefault` | Whether snap is on by default (used with Toggle and HoldToDisable modes) |

---

## Phase 6: Input Action Functionality Implementation

**Status**: 🔄 In Progress  
**Goal**: Wire all Input Actions to C++ handlers and implement full input functionality.

### 6.1 Global Actions - Wire to PlayerController
- [x] `IA_ModifierCtrl` → `OnModifierCtrl` (Started/Completed)
- [x] `IA_ModifierShift` → `OnModifierShift` (Started/Completed)
- [x] `IA_ModifierAlt` → `OnModifierAlt` (Started/Completed)
- [x] `IA_Escape` → `OnEscape`
- [x] `IA_Undo` → `OnUndo` (calls `Document->Undo()`)
- [x] `IA_Redo` → `OnRedo` (calls `Document->Redo()`)
- [x] `IA_Delete` → `OnDelete`
- [x] `IA_Save` → `OnSave`
- [ ] `IA_ToggleView` → `OnToggleView` (wire to `CameraController->ToggleViewMode()`)
- [ ] `IA_ToolSelect` → `OnToolSelect` (calls `ToolManager->SelectToolByType(Select)`)
- [ ] `IA_ToolLine` → `OnToolLine` (calls `ToolManager->SelectToolByType(Line)`)
- [ ] `IA_ToolPolyLine` → `OnToolPolyline` (calls `ToolManager->SelectToolByType(Polyline)`)

### 6.2 View/Navigation Actions - 2D Mode
- [x] `IA_Pan` → `OnPanStarted/OnPanCompleted` (set `bPanning` state)
- [x] `IA_PanDelta` → `OnPanDelta` (1:1 cursor-to-scene pan with interpolation)
- [x] `IA_Zoom` → `OnZoom` (adjust ortho width)
- [x] `IA_PointerPosition` → `OnPointerPosition` (update virtual cursor position)
- [x] `IA_ResetView` → `OnResetView` (reset camera to default)
- [ ] `IA_FocusSelection` → `OnFocusSelection` (**BROKEN**: not working properly in 2D and 3D mode)
- [ ] `IA_SnapToggle` → `OnSnapToggle` (toggle snap on/off)
- [ ] `IA_GridToggle` → `OnGridToggle` (toggle grid visibility)

### 6.3 View/Navigation Actions - 3D Mode
- [x] `IA_Orbit` → `OnOrbitStarted/OnOrbitCompleted` (set `bOrbiting` state)
- [x] `IA_OrbitDelta` → `OnOrbitDelta` (rotate camera around target)
- [x] `IA_Move` → `OnMove` (WASD movement with correct axis mapping: Y=forward/back, X=right/left)
- [x] `IA_AdjustFlySpeed` → `OnAdjustFlySpeed` (RMB+Scroll adjusts fly speed)
- [ ] `IA_ViewTop` → `OnViewTop` (set camera to top orthographic view)
- [ ] `IA_ViewFront` → `OnViewFront` (set camera to front view)
- [ ] `IA_ViewRight` → `OnViewRight` (set camera to right view)
- [ ] `IA_ViewPerspective` → `OnViewPerspective` (toggle ortho/perspective)

### 6.4 Selection Actions - Wire to SelectTool

> ⚠️ **KNOWN ISSUES (2026-01-19):**
> - Selection currently uses raw keyboard input (`IsInputKeyDown`) for modifier detection instead of Enhanced Input
> - Need to refactor to use `IA_SelectAdd`, `IA_SelectToggle`, `IA_SelectRemove` as proper chorded actions
> - Marquee selection (box select) is not working in 2D mode
> - SelectAll and DeselectAll handlers exist but are not working
> - FocusSelection is not working properly in both 2D and 3D mode
> - 3D selection uses line trace but may have issues with hit detection

- [x] `IA_Select` → `OnSelectStarted/OnSelectCompleted` (basic click selection works)
- [ ] `IA_SelectAdd` → `OnSelectAdd` (Shift+Click - **BROKEN**: using raw input instead of Enhanced Input)
- [ ] `IA_SelectToggle` → `OnSelectToggle` (Ctrl+Click - **BROKEN**: using raw input instead of Enhanced Input)
- [ ] `IA_SelectRemove` → `OnSelectRemove` (Alt+Click - **BROKEN**: using raw input instead of Enhanced Input)
- [ ] `IA_SelectAll` → `OnSelectAll` (**BROKEN**: handler exists but not working)
- [ ] `IA_DeselectAll` → `OnDeselectAll` (**BROKEN**: handler exists but not working)
- [ ] `IA_CycleSelection` → `OnCycleSelection` (cycle through overlapping objects)
- [ ] `IA_BoxSelectStart` → `OnBoxSelectStart` (**BROKEN**: marquee not working in 2D mode)
- [ ] `IA_BoxSelectDrag` → `OnBoxSelectDrag` (**BROKEN**: marquee not working in 2D mode)
- [ ] `IA_BoxSelectEnd` → `OnBoxSelectEnd` (**BROKEN**: marquee not working in 2D mode)

#### Selection TODO:
- [ ] Refactor `OnSelectStarted` to NOT use `IsInputKeyDown()` - use Enhanced Input entirely
- [ ] Make `IA_SelectAdd` (Shift+LMB) work as a proper chorded action
- [ ] Make `IA_SelectToggle` (Ctrl+LMB) work as a proper chorded action
- [ ] Make `IA_SelectRemove` (Alt+LMB) work as a proper chorded action
- [ ] Fix marquee selection in 2D mode - investigate why world positions aren't updating
- [ ] Fix SelectAll - ensure it calls `SelectTool->SelectAll()` properly
- [ ] Fix DeselectAll - ensure it calls `SelectTool->ClearSelection()` properly
- [ ] Fix FocusSelection in 2D mode - camera should center on selection bounds
- [ ] Fix FocusSelection in 3D mode - camera should orbit to frame selection

### 6.5 Drawing Actions - Wire to LineTool/PolylineTool
- [x] `IA_DrawPlacePoint` → `OnDrawPlacePoint` (route LMB to active drawing tool)
- [x] `IA_DrawConfirm` → `OnDrawConfirm` (finish polyline with Enter)
- [x] `IA_DrawCancel` → `OnDrawCancel` (cancel current segment - calls CancelDrawing())
- [x] `IA_DrawClose` → `OnDrawClose` (close polygon loop - calls ClosePolyline())
- [x] `IA_DrawRemoveLastPoint` → `OnDrawRemoveLastPoint` (remove last vertex + delete wall mesh)
- [x] `IA_OrthoLock` → `OnOrthoLock` (hold Shift for 90° angles only)
- [x] `IA_AngleSnap` → `OnAngleSnap` (toggle 45° angle snap increments)

### 6.6 Numeric Entry Actions
- [x] `IA_NumericDigit` → `OnNumericDigit` (parse scalar value to digit 0-9)
- [x] `IA_NumericDecimal` → `OnNumericDecimal` (add decimal point)
- [x] `IA_NumericBackspace` → `OnNumericBackspace` (remove char / RemoveLastPoint when empty)
- [x] `IA_NumericCommit` → `OnNumericCommit` (commit value to tool)
- [x] `IA_NumericCancel` → `OnNumericCancel` (cancel numeric entry, clear buffer)
- [x] `IA_NumericSwitchField` → `OnNumericSwitchField` (toggle length/angle field)
- [x] `IA_NumericCycleUnits` → `OnNumericCycleUnits` (cycle cm/m/in/ft - preserves line length)
- [ ] `IA_NumericAdd/Subtract/Multiply/Divide` → Future calculator functionality

### 6.7 Configure IMC Key Bindings in Editor
- [x] `IMC_Base` - Add global key bindings (modifiers, Esc, Undo/Redo, tool hotkeys)
- [x] `IMC_2DBase` - Add 2D navigation bindings (Pan, Zoom, Pointer)
- [x] `IMC_2DSelection` - Add selection bindings (click, box select, modifiers)
- [x] `IMC_2DLineTool` - Add line tool bindings (place point, cancel, ortho)
- [x] `IMC_2DPolylineTool` - Add polyline bindings (confirm, close, remove point)
- [x] `IMC_3DBase` - Add 3D navigation bindings (orbit, pan, zoom)
- [x] `IMC_3DSelection` - Add 3D selection bindings
- [x] `IMC_3DNavigation` - Add numpad view shortcuts
- [x] `IMC_NumericEntry` - Add all digit keys with Scalar modifiers

### 6.8 Update DA_ArchVisInput Data Asset
- [x] Assign all IMC references to data asset slots
- [x] Assign all IA references to data asset slots
- [x] Set default property values

### 6.9 Integration Testing
- [x] Test tool switching via hotkeys (V = Select, L = Line, P = Polyline)
- [x] Test 2D pan/zoom with MMB and scroll wheel (1:1 cursor tracking)
- [x] Test 3D orbit/pan/fly with mouse navigation
- [x] Test 3D WASD movement (W/S=forward/back, A/D=left/right)
- [x] Test fly speed adjustment via RMB+Scroll
- [ ] Test selection with LMB, Shift+LMB, Alt+LMB
- [x] Test line drawing with click-move-click workflow
- [x] Test polyline with multiple vertices and Enter to confirm
- [x] Test numeric input during line drawing
- [ ] Test Undo/Redo with Ctrl+Z/Ctrl+Y
- [x] Test 2D/3D mode toggle with Tab
- [x] Test IMC priority layering (numeric entry overrides tool IMC)
