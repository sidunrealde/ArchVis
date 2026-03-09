# Architecture

**Analysis Date:** 2026-03-09

## Pattern Overview

**Overall:** Modular Unreal Engine application with a plugin-first domain architecture.

**Key Characteristics:**
- The root game module in `Source/ArchVis/` owns Unreal framework entry points and composes plugin services.
- Core domain logic is split into focused runtime plugins under `Plugins/RTPlan*`.
- Feature plugins under `Plugins/RTFeature_*` aggregate lower-level planning plugins without owning much deep logic.
- State changes are event-driven: document mutations broadcast delegates that trigger rebuilding in tools, shell rendering, and networking.

## Layers

**Application Shell:**
- Purpose: Own GameMode, PlayerController, pawns, HUD, and top-level input orchestration.
- Location: `Source/ArchVis/`
- Contains: `AArchVisGameMode`, `AArchVisPlayerController`, pawn variants, input components, HUD, target files, and root build rules.
- Depends on: `RTPlanCore`, `RTPlanTools`, `RTPlanShell`, `RTPlanInput`, `RTPlanNet`, `RTPlanUI`, and `RTPlanCatalog` from `Source/ArchVis/ArchVis.Build.cs`.
- Used by: Unreal target entry points in `Source/*.Target.cs` and the project descriptor `ArchVis.uproject`.

**Core Domain Layer:**
- Purpose: Hold canonical plan data, schema, command stack, and globally accessible subsystem state.
- Location: `Plugins/RTPlanCore/Source/RTPlanCore/`
- Contains: `URTPlanDocument`, `URTPlanSubsystem`, schema structs, command classes, and automation tests.
- Depends on: Unreal base modules plus `Json` and `JsonUtilities` in `Plugins/RTPlanCore/Source/RTPlanCore/RTPlanCore.Build.cs`.
- Used by: Nearly every other `RTPlan*` plugin and the root `ArchVis` module.

**Geometry & Spatial Services:**
- Purpose: Provide geometry math, snapping, alignment guides, hit testing, and derived spatial indices.
- Location: `Plugins/RTPlanMath/Source/RTPlanMath/` and `Plugins/RTPlanSpatial/Source/RTPlanSpatial/`
- Contains: `FRTPlanSpatialIndex`, geometry utilities, and automation tests.
- Depends on: `RTPlanCore`; `RTPlanSpatial` also depends on `RTPlanMath`.
- Used by: `Plugins/RTPlanTools/Source/RTPlanTools/` and any selection/snapping workflow.

**Interactive Tooling Layer:**
- Purpose: Convert pointer input into drafting, selection, trim, and arc commands.
- Location: `Plugins/RTPlanTools/Source/RTPlanTools/`
- Contains: `URTPlanToolManager`, `URTPlanToolBase`, and concrete tools in `Public/Tools/` and `Private/Tools/`.
- Depends on: `RTPlanInput`, `RTPlanSpatial`, `RTPlanMath`, `RTPlanCatalog`, and `RTPlanObjects` per `Plugins/RTPlanTools/Source/RTPlanTools/RTPlanTools.Build.cs`.
- Used by: `AArchVisPlayerController`, `RTPlanUI`, and feature plugins like `RTFeature_Drafting2D`.

**Rendering / Meshing Layer:**
- Purpose: Turn plan data into dynamic mesh geometry and visible shell actors.
- Location: `Plugins/RTPlanMeshing/Source/RTPlanMeshing/` and `Plugins/RTPlanShell/Source/RTPlanShell/`
- Contains: `FRTPlanMeshBuilder`, `ARTPlanShellActor`, shell tests, and Nanite-aware mesh conversion helpers.
- Depends on: Geometry engine modules, `RTPlanCore`, `RTPlanMath`, `RTPlanOpenings`, and `RTPlanCatalog`.
- Used by: `AArchVisGameMode` in `Source/ArchVis/Private/ArchVisGameMode.cpp` and shell-related feature plugins.

**UI Layer:**
- Purpose: Surface toolbars, property widgets, and catalog browser widgets.
- Location: `Plugins/RTPlanUI/Source/RTPlanUI/`
- Contains: `URTPlanToolbar`, `URTPlanProperties`, `URTPlanCatalogBrowser`, `URTPlanWallPropertiesWidget`, and module bootstrap.
- Depends on: `UMG`, `CommonUI`, `RTPlanCore`, `RTPlanTools`, and `RTPlanCatalog`.
- Used by: `Source/ArchVis/Private/ArchVisPlayerController.cpp` and VR/UI feature flows.

**Networking Layer:**
- Purpose: Replicate document state and accept server-side command submissions.
- Location: `Plugins/RTPlanNet/Source/RTPlanNet/`
- Contains: `ARTPlanNetDriver` and net tests.
- Depends on: `NetCore` and `RTPlanCore`.
- Used by: `AArchVisGameMode::StartPlay()` in `Source/ArchVis/Private/ArchVisGameMode.cpp` and the multiplayer feature plugin `Plugins/RTFeature_Multiplayer`.

**Feature Composition Layer:**
- Purpose: Package higher-level experiences over lower-level RTPlan modules.
- Location: `Plugins/RTFeature_Drafting2D`, `Plugins/RTFeature_Shell3D`, `Plugins/RTFeature_Interiors`, `Plugins/RTFeature_VR`, and `Plugins/RTFeature_Multiplayer`
- Contains: Thin runtime modules with dependency declarations and minimal bootstrap code.
- Depends on: Specific lower-level planning plugins per each feature module `*.Build.cs`.
- Used by: `ArchVis.uproject` plugin enablement.

## Data Flow

**Startup Composition Flow:**

1. Unreal launches the `ArchVis` module defined in `ArchVis.uproject` and target files under `Source/`.
2. `AArchVisGameMode::StartPlay()` in `Source/ArchVis/Private/ArchVisGameMode.cpp` creates a `URTPlanDocument`, initializes a `URTPlanToolManager`, spawns `ARTPlanShellActor`, and spawns `ARTPlanNetDriver`.
3. `AArchVisPlayerController::OnGameModeReady()` in `Source/ArchVis/Private/ArchVisPlayerController.cpp` registers the document/tool manager with `URTPlanSubsystem`, selects the default tool, and wires selection/UI hooks.

**Interactive Editing Flow:**

1. `AArchVisPlayerController` receives Enhanced Input actions in `Source/ArchVis/Private/ArchVisPlayerController.cpp`.
2. Tool input is routed to `URTPlanToolManager` in `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp`.
3. The active tool (for example `URTPlanLineTool` in `Plugins/RTPlanTools/Source/RTPlanTools/Private/Tools/RTPlanLineTool.cpp`) converts pointer events into command objects.
4. `URTPlanDocument::SubmitCommand()` in `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanDocument.cpp` mutates the canonical `FRTPlanData`, maintains undo/redo stacks, and broadcasts `OnPlanChanged`.

**Derived Systems Update Flow:**

1. `URTPlanDocument::OnPlanChanged` is subscribed to by `URTPlanToolManager` to rebuild the spatial index.
2. `ARTPlanShellActor` listens for plan changes and rebuilds visible geometry.
3. `ARTPlanNetDriver` serializes the document to replicated JSON and ships it to clients.

**State Management:**
- The canonical editable state is `FRTPlanData` inside `URTPlanDocument` in `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanDocument.h`.
- Global lookup is provided by `URTPlanSubsystem` in `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanSubsystem.h`.
- Tool-local transient state such as active tool, snap/grid toggles, and selection cache lives in `URTPlanToolManager`.
- Rendering state such as selected wall highlights lives in `ARTPlanShellActor`.

## Key Abstractions

**Document + Command Model:**
- Purpose: Represent the authoritative plan and all reversible mutations.
- Examples: `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanDocument.h`, `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanCommand.h`
- Pattern: Command pattern with undo/redo and event broadcasting.

**Subsystem Locator:**
- Purpose: Provide central access to document and tool manager from arbitrary world context.
- Examples: `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanSubsystem.h`
- Pattern: `UGameInstanceSubsystem` service locator.

**Tool Abstraction:**
- Purpose: Normalize interactive editing tools behind a shared lifecycle and pointer-event API.
- Examples: `Plugins/RTPlanTools/Source/RTPlanTools/Public/RTPlanToolBase.h`, `Plugins/RTPlanTools/Source/RTPlanTools/Public/Tools/RTPlanSelectTool.h`, `Plugins/RTPlanTools/Source/RTPlanTools/Public/Tools/RTPlanLineTool.h`
- Pattern: Polymorphic strategy objects managed by `URTPlanToolManager`.

**Spatial Index:**
- Purpose: Build fast snap/hit-test data derived from the current document.
- Examples: `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Public/RTPlanSpatialIndex.h`, `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Private/RTPlanSpatialIndex.cpp`
- Pattern: Rebuilt derived cache.

**Shell Renderer:**
- Purpose: Materialize 3D shell geometry from the plan document.
- Examples: `Plugins/RTPlanShell/Source/RTPlanShell/Public/RTPlanShellActor.h`, `Plugins/RTPlanMeshing/Source/RTPlanMeshing/Public/RTPlanMeshBuilder.h`
- Pattern: Observer over document changes plus mesh builder composition.

## Entry Points

**Game startup:**
- Location: `Source/ArchVis/Private/ArchVisGameMode.cpp`
- Triggers: Unreal game start / map load.
- Responsibilities: Construct document, tool manager, shell actor, network driver, and notify the player controller.

**Player/controller startup:**
- Location: `Source/ArchVis/Private/ArchVisPlayerController.cpp`
- Triggers: `BeginPlay()` and `OnGameModeReady()`.
- Responsibilities: Spawn or adopt the correct pawn, set up input mappings, bind tool/UI behavior, and register shared services in the subsystem.

**Plugin module startup:**
- Location: `Plugins/*/Source/*/Private/*Module.cpp`
- Triggers: Unreal module loading.
- Responsibilities: Minimal module bootstrap; most modules are intentionally thin.

## Error Handling

**Strategy:** Defensive null checks, boolean status returns, and Unreal logging.

**Patterns:**
- Guard clauses returning `nullptr`, `false`, or early return when world, document, or manager pointers are unavailable. Examples: `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanSubsystem.cpp` and `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp`.
- Diagnostic `UE_LOG` usage around startup, tool actions, and spatial rebuilds. Examples: `Source/ArchVis/Private/ArchVisGameMode.cpp`, `Source/ArchVis/Private/ArchVisPlayerController.cpp`, `Plugins/RTPlanTools/Source/RTPlanTools/Private/Tools/RTPlanLineTool.cpp`.
- Tests use assertions through Unreal Automation macros in `Plugins/*/Private/*Tests.cpp` instead of exception-style handling.

## Cross-Cutting Concerns

**Logging:** Use Unreal log categories and `UE_LOG` from runtime code.

**Validation:** Validation is mostly ad hoc through pointer checks and command return values; strict domain validation layers are not centralized.

**Authentication:** None detected.

**Dependency management:** Cycles are actively avoided, for example `URTPlanSubsystem` stores the tool manager as `UObject*` in `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanSubsystem.h`, and `Plugins/RTPlanOpenings/Source/RTPlanOpenings/RTPlanOpenings.Build.cs` notes a removed dependency to break a circular reference.

---

*Architecture analysis: 2026-03-09*

