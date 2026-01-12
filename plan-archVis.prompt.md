# ArchVis Project Understanding Report (C: both)

This document summarizes what this Unreal project currently implements, mapped against `ArchPlan_Refined.md`. It includes:
1) a high-level “what’s done” overview, and
2) a module-by-module breakdown of implemented vs missing systems.

---

## 0) Big picture architecture (what exists today)

### Canonical data model (source of truth)
- **`URTPlanDocument`** (`Plugins/RTPlanCore`) is the core runtime “plan document”.
  - Stores canonical data in **`FRTPlanData`** (vertices, walls, openings, objects, runs).
  - Uses a **command stack** (Undo/Redo) for edits.
  - Supports **JSON serialization** (`ToJson` / `FromJson`).

### Interaction flow
- **Desktop input** is captured in **`AArchVisPlayerController`** (`Source/ArchVis`).
  - Uses **Enhanced Input**.
  - Maintains a **virtual cursor** (software cursor) and produces unified pointer events (`FRTPointerEvent`).
- Pointer events are routed into **`URTPlanToolManager`** (`Plugins/RTPlanTools`).
  - Manages the active tool.
  - Rebuilds the `FRTPlanSpatialIndex` when `URTPlanDocument` changes.
- The active tool (`URTPlanToolBase` subclasses) submits commands into `URTPlanDocument`.

### Derived outputs
- **3D shell mesh generation** is implemented via **`ARTPlanShellActor`** (`Plugins/RTPlanShell`).
  - Listens to `URTPlanDocument::OnPlanChanged`.
  - Rebuilds walls into a `UDynamicMeshComponent`.
  - Supports **openings** by splitting wall segments using `RTPlanOpenings` utilities.

### Networking
- **`ARTPlanNetDriver`** (`Plugins/RTPlanNet`) replicates the plan by replicating a **JSON string** (`ReplicatedPlanJson`).
  - Server updates `ReplicatedPlanJson` on plan changes (debounced).
  - Clients apply updates by calling `Document->FromJson(...)`.
  - Has server RPC stubs for `Server_SubmitAddWall` / `Server_SubmitDeleteWall`.

---

## 1) What’s implemented vs `ArchPlan_Refined.md`

### Phase 1: Viewport & Input (The “CAD Feel”)
**1) Camera Controller: 2D/3D switching logic**
- **DONE (baseline)**
  - `AArchVisCameraController` supports:
    - Top-down **Orthographic** mode (fixed rotation).
    - Perspective mode.
    - `ToggleViewMode()`.
    - Pan + Zoom.

**2) HUD: full-screen crosshair cursor**
- **DONE**
  - `AArchVisHUD::DrawHUD()` draws full-screen horizontal + vertical lines.
  - Uses snapped tool cursor if available; otherwise uses virtual cursor.

**3) Input handling: numeric keystrokes buffer**
- **MISSING**
  - There is `URTPlanToolBase::OnNumericInput(float)` and `URTPlanLineTool::OnNumericInput(float)`.
  - But there’s no implemented input buffer / text entry / Enter commit / Tab switch.


### Phase 2: Wall Tool 2.0 (Click-Move-Click)
**1) Refactor line tool: click-move-click state machine**
- **DONE**
  - `URTPlanLineTool` uses a state machine:
    - WaitingForStart → WaitingForEnd.
    - Commits on a second click (currently uses pointer `Down`).

**2) Ortho locking**
- **DONE (basic)**
  - `URTPlanLineTool::ApplyOrthoLock()` locks to X/Y.
  - **Missing:** modifier key to disable (Shift/Alt) isn’t wired.

**3) Numeric entry hook-up**
- **PARTIAL (tool-side implemented)**
  - `URTPlanLineTool::OnNumericInput(float)` exists and commits at a typed length.
  - **Missing:** the input system to feed values into `OnNumericInput`.


### Phase 3: 2D Visualization
**1) 2D renderer for plan view (lines, arcs for doors)**
- **PARTIAL**
  - Crosshair exists.
  - No full plan 2D drawing layer found yet (walls/doors/windows in 2D).

**2) Overlay: render wall dimensions (text)**
- **MISSING**


### Phase 4: Room Detection & Generation
**1) Graph logic + cycle detection**
- **MISSING**
  - `FRTPlanData` currently does not include `FRTRoom`.

**2) Floor/ceiling triangulation**
- **MISSING**
  - Shell currently generates wall meshes only.


### Phase 5: Properties & Editing
**1) Selection system (raycast to find walls/rooms)**
- **MISSING/PARTIAL**
  - `RTPlanSpatialIndex` is for snapping/alignment, not selection.

**2) Property UI binding**
- **PARTIAL/STUB**
  - `RTPlanUI` exists with widgets like `RTPlanToolbar`, `RTPlanProperties`, `RTPlanCatalogBrowser`.
  - No clear connection from ArchVis runtime to spawn/display these widgets yet.

**3) Commands for property changes (`URTCmdUpdateWall` etc.)**
- **MISSING**
  - Commands currently implemented: `URTCmdAddVertex`, `URTCmdAddWall`, `URTCmdDeleteWall`.
  - TODO notes mention Openings/Objects/Runs commands are not implemented yet.

---

## 2) Module-by-module breakdown

### `Source/ArchVis` (game module)
**Key classes:**
- `AArchVisGameMode`
  - **DONE:** Creates `URTPlanDocument`, `URTPlanToolManager`.
  - **DONE:** Spawns `ARTPlanShellActor` and links the document.
  - **DONE:** Spawns `ARTPlanNetDriver` (optional) and links the document.
- `AArchVisPlayerController`
  - **DONE:** Enhanced Input bindings for left/right click, mouse move, wheel, toggle view.
  - **DONE:** Virtual cursor that supports CAD-like cursor positioning.
  - **PARTIAL:** sends `Move`/`Down`/`Up` pointer events to ToolMgr (right click doesn’t yet send cancel/confirm events).
  - **MISSING:** numeric input buffer + feeding tools.
- `AArchVisHUD`
  - **DONE:** full-screen crosshair.
  - **PARTIAL:** relies on tool reporting a snapped cursor; doesn’t render plan geometry.
- `AArchVisCameraController`
  - **DONE:** 2D/3D toggle, ortho/persp, pan/zoom.


### `Plugins/RTPlanCore`
- `RTPlanSchema.h`
  - **DONE:** defines vertices, walls (with thickness/height/baseZ and left/right finishes), openings, interior instances, cabinet runs.
  - **MISSING:** rooms (`FRTRoom`) and per-room materials in schema.
- `URTPlanDocument`
  - **DONE:** command stack (undo/redo), OnPlanChanged event, JSON serialization.
- `RTPlanCommand`
  - **DONE:** add vertex, add wall, delete wall.
  - **MISSING:** update wall params, commands for openings/objects/runs.


### `Plugins/RTPlanSpatial`
- `FRTPlanSpatialIndex`
  - **DONE:** rebuilds snap point caches from document.
  - **DONE:** snap query priority: endpoints/midpoints first, then projection.
  - **DONE:** alignment guides via unique X/Y.
  - **MISSING:** explicit grid snapping (the code mentions grid in plan, but current implementation is snap points + projection + alignment).


### `Plugins/RTPlanInput`
- `FRTPointerEvent` + enums
  - **DONE:** unified pointer event data model for mouse/VR.
  - **MISSING:** keyboard/numeric input buffering and “Enter/Tab” semantics described in refined plan.


### `Plugins/RTPlanTools`
- `URTPlanToolManager`
  - **DONE:** active tool lifecycle + spatial index refresh.
- `URTPlanToolBase`
  - **DONE:** provides ground-plane intersection + snap helper.
- `URTPlanLineTool`
  - **DONE:** click-move-click wall drawing.
  - **DONE:** polyline mode toggle.
  - **DONE:** ortho lock.
  - **PARTIAL:** numeric input support exists but is not wired to actual keyboard entry.
- Tests
  - **DONE:** there are automation tests for tools (line tool input simulation), indicating some test scaffolding is present.


### `Plugins/RTPlanMeshing`
- Module and mesh builder headers exist.
- **PARTIAL:** seems used by `RTPlanShell` (`RTPlanMeshBuilder.h`), but room triangulation and floor/ceiling generation are not present in observed usage.


### `Plugins/RTPlanShell`
- `ARTPlanShellActor`
  - **DONE:** wall mesh building into dynamic mesh.
  - **DONE:** listens to document changes.
  - **PARTIAL:** walls only; no floors/ceilings.


### `Plugins/RTPlanOpenings`
- **PARTIAL:** utilities exist and are used by shell to split walls.
- **MISSING:** interactive opening placement tool + commands + 2D visualization aren’t implemented yet.


### `Plugins/RTPlanNet`
- `ARTPlanNetDriver`
  - **DONE (baseline):** replicates entire plan JSON and applies it on clients.
  - **PARTIAL:** RPCs exist for add/delete wall, but the editing flow currently calls `Document->SubmitCommand(...)` directly client-side via tools, not via net driver.
  - **MISSING:** permissions (Authoring vs Viewer), partial replication (FastArray), shared object move/lock.


### `Plugins/RTPlanUI`
- **PARTIAL/STUB:** widget base classes exist (toolbar/properties/catalog), but no confirmed runtime wiring in `ArchVis` to show them.


### `Plugins/RTPlanVR`
- **PARTIAL/STUB:** VR pawn exists with pointer-event generation.


### `Plugins/RTPlanObjects` / `RTPlanRuns` / `RTPlanCatalog` / `RTFeature_*`
- Present in the project tree.
- Not yet confirmed as actively used in `ArchVis` module code paths (based on the code reviewed so far).

---

## 3) Concrete “next steps” (highest leverage)

1) **Numeric input pipeline (Phase 1 + Phase 2)**
   - Implement a small input buffer in `AArchVisPlayerController` (or a new `RTPlanInput` component) that:
     - Collects digits and a decimal point.
     - On `Enter`, parses to float and calls `ToolMgr->GetActiveTool()->OnNumericInput(Value)`.
     - Supports backspace and escape to clear.
     - (Later) add `Tab` to switch length/angle fields.

2) **Tool cancel and right-click finish (Phase 2)**
   - On RightClick/Esc, send an `ERTPointerAction::Cancel` event to the tool.
   - Update `URTPlanLineTool` to finish polyline on cancel.

3) **2D plan drawing layer (Phase 3)**
   - Add a lightweight renderer (HUD draw or a custom component) to draw:
     - Wall segments in screen space.
     - Door arcs and window markers based on `FRTPlanData.Openings`.

4) **Room detection + floors/ceilings (Phase 4)**
   - Add `FRTRoom` to schema or compute derived rooms.
   - Triangulate polygon loops in `RTPlanMeshing`.
   - Extend `RTPlanShellActor` to generate floors/ceilings.

5) **Selection + property editing commands (Phase 5)**
   - Add commands: `URTCmdUpdateWall` (and later opening/object updates).
   - Implement selection hit-testing (initially based on distance-to-wall in XY using `RTPlanSpatialIndex` segments).

---

## 4) Requirements coverage (your ask)
- “Go through the whole project and understand what’s been done”: **Done** (based on ArchVis module + key RTPlan plugins used by runtime loop).
- “Output as (C) both”: **Done** (high-level + module-by-module + phase mapping).

