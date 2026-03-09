# Codebase Structure

**Analysis Date:** 2026-03-09

## Directory Layout

```text
ArchVis/
├── .claude/                 # GSD workflows, templates, and helpers
├── .github/                 # Prompt files and instruction files
├── Config/                  # Unreal project configuration (.ini)
├── Content/                 # Maps, materials, data assets, textures, UI assets
├── Plugins/                 # Modular RTPlan and RTFeature runtime plugins
├── Source/                  # Root ArchVis game module and target definitions
├── Binaries/                # Built editor/runtime artifacts
├── DerivedDataCache/        # Local Unreal cache
├── Intermediate/            # Generated/build intermediate files
├── Saved/                   # Logs, autosaves, screenshots, automation output
├── plans/                   # Project planning notes
├── ArchVis.uproject         # Unreal project descriptor
└── ArchVis.sln              # Visual Studio solution
```

## Directory Purposes

**`Source/`:**
- Purpose: Root game module and target definitions.
- Contains: `ArchVis.cpp`, `ArchVis.Build.cs`, target files, public headers under `Source/ArchVis/Public/`, and implementation files under `Source/ArchVis/Private/`.
- Key files: `Source/ArchVis/Private/ArchVisGameMode.cpp`, `Source/ArchVis/Private/ArchVisPlayerController.cpp`, `Source/ArchVis/Public/ArchVisGameMode.h`, `Source/ArchVis/Public/ArchVisPlayerController.h`.

**`Source/ArchVis/Public/` and `Source/ArchVis/Private/`:**
- Purpose: Unreal-style public/private split for the root module.
- Contains: Pawn classes, controller/HUD classes, input components, and module bootstrap code.
- Key files: `Source/ArchVis/Public/Input/ToolInputComponent.h`, `Source/ArchVis/Private/Input/ToolInputComponent.cpp`.

**`Plugins/`:**
- Purpose: Separate domain modules and feature modules.
- Contains: One plugin per bounded concern, each with its own `.uplugin`, `Source/<Module>/Public`, `Source/<Module>/Private`, and optional content.
- Key files: `Plugins/RTPlanCore/RTPlanCore.uplugin`, `Plugins/RTPlanTools/Source/RTPlanTools/RTPlanTools.Build.cs`, `Plugins/RTPlanShell/Source/RTPlanShell/Public/RTPlanShellActor.h`.

**`Plugins/RTPlan*/`:**
- Purpose: Reusable planning/domain infrastructure.
- Contains: Core model (`RTPlanCore`), math (`RTPlanMath`), spatial queries (`RTPlanSpatial`), input (`RTPlanInput`), tools (`RTPlanTools`), meshing (`RTPlanMeshing`), shell rendering (`RTPlanShell`), openings (`RTPlanOpenings`), catalogs (`RTPlanCatalog`), objects (`RTPlanObjects`), runs (`RTPlanRuns`), UI (`RTPlanUI`), VR (`RTPlanVR`), and net replication (`RTPlanNet`).
- Key files: `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanDocument.h`, `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Public/RTPlanSpatialIndex.h`, `Plugins/RTPlanUI/Source/RTPlanUI/Public/RTPlanToolbar.h`.

**`Plugins/RTFeature_*/`:**
- Purpose: Higher-level packaged feature modules.
- Contains: Lightweight bootstrap/runtime modules for drafting, shell, interiors, VR, and multiplayer.
- Key files: `Plugins/RTFeature_Drafting2D/Source/RTFeature_Drafting2D/RTFeature_Drafting2D.Build.cs`, `Plugins/RTFeature_Multiplayer/Source/RTFeature_Multiplayer/RTFeature_Multiplayer.Build.cs`.

**`Config/`:**
- Purpose: Project settings under source control.
- Contains: `DefaultEngine.ini`, `DefaultInput.ini`, `DefaultEditor.ini`, `DefaultGame.ini`.
- Key files: `Config/DefaultEngine.ini`, `Config/DefaultInput.ini`.

**`Content/`:**
- Purpose: Unreal assets and maps.
- Contains: `TestMap.umap`, data collections, textures, UI assets, materials, and testing assets.
- Key files: `Content/TestMap.umap`, `Content/UI/`, `Content/Data/`, `Content/Testing/`.

**`Saved/`, `Intermediate/`, `DerivedDataCache/`, `Binaries/`:**
- Purpose: Generated, cached, or local-run outputs.
- Contains: Logs, autosaves, generated project files, cached assets, and built binaries.
- Key files: `Saved/Logs/`, `Intermediate/ProjectFiles/`, `Binaries/Win64/`.

## Key File Locations

**Entry Points:**
- `ArchVis.uproject`: Project descriptor and plugin enablement.
- `Source/ArchVis.Target.cs`: Game target.
- `Source/ArchVisEditor.Target.cs`: Editor target.
- `Source/ArchVisServer.Target.cs`: Server target.
- `Source/ArchVisViewer.Target.cs`: Viewer target.
- `Source/ArchVisAuthoring.Target.cs`: Authoring target.
- `Source/ArchVis/Private/ArchVisGameMode.cpp`: Runtime composition entry point.
- `Source/ArchVis/Private/ArchVisPlayerController.cpp`: Input/UI interaction entry point.

**Configuration:**
- `Config/DefaultEngine.ini`: Maps, renderer, RHI, hardware targeting.
- `Config/DefaultInput.ini`: Default input classes, mouse capture, action mappings.
- `Source/ArchVis/ArchVis.Build.cs`: Root module dependencies.
- `Plugins/*/Source/*/*.Build.cs`: Per-module dependency boundaries.
- `Plugins/*/*.uplugin`: Plugin metadata and plugin-to-plugin enablement.

**Core Logic:**
- `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanDocument.h`: Authoritative document model.
- `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanSubsystem.h`: Global subsystem access.
- `Plugins/RTPlanTools/Source/RTPlanTools/Public/RTPlanToolManager.h`: Tool orchestration.
- `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Public/RTPlanSpatialIndex.h`: Snapping/hit testing.
- `Plugins/RTPlanShell/Source/RTPlanShell/Public/RTPlanShellActor.h`: Shell rendering actor.
- `Plugins/RTPlanNet/Source/RTPlanNet/Public/RTPlanNetDriver.h`: Network replication bridge.

**Testing:**
- `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanCoreTests.cpp`
- `Plugins/RTPlanMath/Source/RTPlanMath/Private/RTPlanMathTests.cpp`
- `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Private/RTPlanSpatialTests.cpp`
- `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolsTests.cpp`
- `Plugins/RTPlanShell/Source/RTPlanShell/Private/RTPlanShellTests.cpp`
- `Plugins/RTPlanOpenings/Source/RTPlanOpenings/Private/RTPlanOpeningsTests.cpp`
- `Plugins/RTPlanObjects/Source/RTPlanObjects/Private/RTPlanObjectsTests.cpp`
- `Plugins/RTPlanRuns/Source/RTPlanRuns/Private/RTPlanRunsTests.cpp`
- `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetTests.cpp`

## Naming Conventions

**Files:**
- Unreal classes use PascalCase filenames with engine prefixes, for example `Source/ArchVis/Public/ArchVisGameMode.h`, `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanDocument.h`, and `Plugins/RTPlanTools/Source/RTPlanTools/Public/Tools/RTPlanLineTool.h`.
- Module bootstrap files follow `<ModuleName>Module.h/.cpp`, for example `Plugins/RTPlanUI/Source/RTPlanUI/Public/RTPlanUIModule.h`.
- Build/config files follow Unreal conventions: `*.Build.cs`, `*.Target.cs`, `.uplugin`, `.uproject`, and `Default*.ini`.

**Directories:**
- Module source layout uses `Source/<Module>/Public` and `Source/<Module>/Private`.
- Tool specializations are grouped under `Plugins/RTPlanTools/Source/RTPlanTools/Public/Tools/` and `Private/Tools/`.
- Input-related root-module code lives under `Source/ArchVis/Public/Input/` and `Source/ArchVis/Private/Input/`.

## Where to Add New Code

**New gameplay feature in root app shell:**
- Primary code: `Source/ArchVis/Public/` and `Source/ArchVis/Private/`
- Tests: Prefer a focused plugin automation test if the logic is reusable; otherwise add a new automation test file adjacent to the owning module.

**New domain module or reusable planning service:**
- Implementation: Create a new plugin under `Plugins/<NewPlugin>/` with `.uplugin`, `Source/<Module>/Public`, `Source/<Module>/Private`, and `<Module>.Build.cs`.
- Place core model/data behaviors beside `Plugins/RTPlanCore/` patterns, math beside `Plugins/RTPlanMath/`, and interaction behaviors beside `Plugins/RTPlanTools/`.

**New tool:**
- Implementation: `Plugins/RTPlanTools/Source/RTPlanTools/Public/Tools/` and `Plugins/RTPlanTools/Source/RTPlanTools/Private/Tools/`
- Registration/wiring: `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp`
- UI affordance: `Plugins/RTPlanUI/Source/RTPlanUI/`

**New UI widget/module:**
- Implementation: `Plugins/RTPlanUI/Source/RTPlanUI/Public/` and `Plugins/RTPlanUI/Source/RTPlanUI/Private/`
- Asset/widget blueprints: `Content/UI/` or plugin content if the plugin descriptor allows content.

**New geometry/rendering behavior:**
- Mesh generation: `Plugins/RTPlanMeshing/Source/RTPlanMeshing/`
- Scene actor/render bridge: `Plugins/RTPlanShell/Source/RTPlanShell/`

**Utilities:**
- Shared math/helpers: `Plugins/RTPlanMath/Source/RTPlanMath/`
- Shared plan schema/commands: `Plugins/RTPlanCore/Source/RTPlanCore/`

## Special Directories

**`.claude/`:**
- Purpose: Local GSD workflow/runtime metadata.
- Generated: No.
- Committed: Yes.

**`.planning/`:**
- Purpose: Planning artifacts and generated codebase map documents.
- Generated: Mixed; this mapping created `.planning/codebase/`.
- Committed: Expected by GSD workflow.

**`Binaries/`:**
- Purpose: Built Unreal editor/runtime binaries.
- Generated: Yes.
- Committed: Present currently, but generated by builds.

**`DerivedDataCache/`:**
- Purpose: Unreal cache.
- Generated: Yes.
- Committed: Present currently, but cache data is environment-specific.

**`Intermediate/`:**
- Purpose: Generated project/build intermediates.
- Generated: Yes.
- Committed: Present currently, but not authoring source.

**`Saved/`:**
- Purpose: Logs, screenshots, autosaves, automation outputs.
- Generated: Yes.
- Committed: Present currently, but runtime-generated.

---

*Structure analysis: 2026-03-09*

