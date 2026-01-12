# ArchVis Task Checklist

This checklist tracks implementation progress against the drafted phases in `plan-archVis.prompt.md` / `ArchPlan_Refined.md`.

Legend: [x] done, [-] partial, [ ] pending

---

## Phase 0: Baseline plumbing (game ↔ plugins)
- [x] `AArchVisGameMode` creates `URTPlanDocument`.
- [x] `AArchVisGameMode` creates `URTPlanToolManager` and initializes it with the document.
- [x] `AArchVisGameMode` spawns `ARTPlanShellActor` and links the document.
- [x] `AArchVisGameMode` spawns `ARTPlanNetDriver` and links the document.

---

## Phase 1: Viewport & Input (The "CAD Feel")
### 1.1 Camera Controller: 2D/3D switching logic
- [x] Top-down orthographic mode.
- [x] Perspective mode.
- [x] Toggle between modes.
- [x] Pan + zoom.

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
- [ ] Create Enhanced Input Action assets in editor for numeric entry (IA_NumericDigit, etc.).

---

## Phase 2: Wall Tool 2.0 (Click-Move-Click)
### 2.1 Line/Polyline tool state machine
- [x] Click-move-click workflow implemented (`URTPlanLineTool`).
- [x] Polyline mode toggle exists (`SetPolylineMode`).

### 2.2 Snapping + ortho
- [x] Hard snapping to endpoints/midpoints.
- [x] Projection snapping to wall segments.
- [x] Soft alignment snapping (align X/Y to existing geometry).
- [-] Explicit grid snapping (as a first-class mode) not implemented.
- [x] Soft ortho snap (snaps to 0/90/180/270 within threshold, allows free angle otherwise).
- [x] Ortho snap threshold configurable (`OrthoSnapThreshold` property).
- [x] Modifier key to toggle/hold snap on/off (`IA_SnapModifier`).
- [x] Snap modifier mode setting (Toggle / HoldToDisable / HoldToEnable).
- [ ] Modifier key to disable ortho snap (Shift/Alt) wiring.

### 2.3 Numeric entry hook-up
- [x] Tool supports numeric input commit (`URTPlanLineTool::OnNumericInput`).
- [x] Input system feeds numeric values to the tool via `CommitNumericInput()`.
- [x] Drafting state exposed (`FRTDraftingState`) for HUD visualization.
- [x] Current length/angle cached and exposed via getters.

### 2.4 Cancel / finish behaviors
- [ ] Right click / Esc sends `ERTPointerAction::Cancel` to active tool.
- [ ] Finish polyline on cancel.

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
- [ ] Hit test/select walls/rooms (raycast or distance-to-segment).
- [ ] Selection state + highlighting.

### 5.2 Property UI
- [-] `RTPlanUI` widgets exist (toolbar/properties/catalog browser).
- [ ] Wire UI into runtime (spawn widgets, bind to tool manager/document).

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
After compiling, create the following Enhanced Input assets in the Unreal Editor:

### Input Actions to Create (in Content/Input/)
1. `IA_NumericDigit` - Value Type: **Axis1D (Float)**. No modifiers on the action itself.
2. `IA_NumericDecimal` - Value Type: Digital (Bool). Map to Period key.
3. `IA_NumericBackspace` - Value Type: Digital (Bool). Map to Backspace key.
4. `IA_NumericCommit` - Value Type: Digital (Bool). Map to Enter key.
5. `IA_NumericClear` - Value Type: Digital (Bool). Map to Escape key.
6. `IA_NumericSwitchField` - Value Type: Digital (Bool). Map to Tab key.
7. `IA_CycleUnits` - Value Type: Digital (Bool). Map to U key.
8. `IA_SnapModifier` - Value Type: Digital (Bool). Map to Left Shift key.

### Update Input Mapping Context (IMC_ArchVis)

#### For `IA_NumericDigit` - Add 10 mappings with Scalar modifiers:

| Key | Scalar X Value |
|-----|----------------|
| One | 1.0 |
| Two | 2.0 |
| Three | 3.0 |
| Four | 4.0 |
| Five | 5.0 |
| Six | 6.0 |
| Seven | 7.0 |
| Eight | 8.0 |
| Nine | 9.0 |
| Zero | **10.0** (maps to digit 0 in code) |

**Why 10.0 for Zero?** Enhanced Input's `Started` trigger requires values > 0 to fire. Using 0.0 for the Zero key causes it to be ignored. The code maps value 10 back to digit 0.

#### For other actions:
| Action | Key |
|--------|-----|
| `IA_NumericDecimal` | Period |
| `IA_NumericBackspace` | Backspace |
| `IA_NumericCommit` | Enter |
| `IA_NumericClear` | Escape |
| `IA_NumericSwitchField` | Tab |
| `IA_CycleUnits` | U |
| `IA_SnapModifier` | Left Shift |

### Update Data Asset (DA_ArchVisInput)
Assign all the new Input Actions to the corresponding slots in the data asset.

### Snap Modifier Settings (on BP_ArchVisController)
The snap modifier behavior can be configured via these properties:

| Property | Description |
|----------|-------------|
| `SnapModifierMode` | **Toggle**: Press to toggle snap on/off. **HoldToDisable**: Snap on by default, hold to disable. **HoldToEnable**: Snap off by default, hold to enable. |
| `bSnapEnabledByDefault` | Whether snap is on by default (used with Toggle and HoldToDisable modes) |

