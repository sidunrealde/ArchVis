# Technology Stack

**Analysis Date:** 2026-03-09

## Languages

**Primary:**
- C++ - Gameplay code, tools, geometry, UI bindings, replication, and tests live in `Source/ArchVis/` and `Plugins/*/Source/*/`.

**Secondary:**
- C# - Unreal Build Tool target and module rules live in `Source/*.Target.cs`, `Source/ArchVis/ArchVis.Build.cs`, and `Plugins/*/Source/*/*.Build.cs`.
- JSON descriptor files - Project and plugin manifests live in `ArchVis.uproject` and `Plugins/*/*.uplugin`.
- INI configuration - Engine, input, and game settings live in `Config/DefaultEngine.ini`, `Config/DefaultInput.ini`, `Config/DefaultEditor.ini`, and `Config/DefaultGame.ini`.

## Runtime

**Environment:**
- Unreal Engine 5.7 - Bound by `EngineAssociation: "5.7"` in `ArchVis.uproject` and `EngineIncludeOrderVersion.Unreal5_7` in `Source/ArchVis.Target.cs` and `Source/ArchVisEditor.Target.cs`.
- Desktop-first runtime with Windows DX12 configured in `Config/DefaultEngine.ini` under `[/Script/WindowsTargetPlatform.WindowsTargetSettings]`.

**Package Manager:**
- Unreal Build Tool / Visual Studio solution workflow.
- Lockfile: Not detected.
- Package manifests such as `package.json`, `pyproject.toml`, `go.mod`, or `Cargo.toml` are not detected at the repo root.

## Frameworks

**Core:**
- Unreal Engine gameplay framework - Root game module in `Source/ArchVis/` defines `AArchVisGameMode`, `AArchVisPlayerController`, HUD, and pawn classes.
- Plugin-oriented modular runtime - Domain and feature slices are split across `Plugins/RTPlanCore`, `Plugins/RTPlanTools`, `Plugins/RTPlanShell`, `Plugins/RTPlanUI`, `Plugins/RTPlanVR`, `Plugins/RTPlanNet`, and related plugins enabled in `ArchVis.uproject`.
- UMG / CommonUI - UI widgets and toolbar/property panels live in `Plugins/RTPlanUI/Source/RTPlanUI/` and depend on `UMG` plus `CommonUI` in `Plugins/RTPlanUI/Source/RTPlanUI/RTPlanUI.Build.cs`.

**Testing:**
- Unreal Automation Framework - Tests use `IMPLEMENT_SIMPLE_AUTOMATION_TEST` in files like `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanCoreTests.cpp` and `Plugins/RTPlanShell/Source/RTPlanShell/Private/RTPlanShellTests.cpp`.

**Build/Dev:**
- Unreal Build Tool - Module rules are defined in `Source/ArchVis/ArchVis.Build.cs` and every plugin `*.Build.cs` file.
- Unreal Header Tool - Reflected types use `UCLASS`, `USTRUCT`, `UFUNCTION`, and API macros throughout `Source/ArchVis/Public/` and `Plugins/*/Source/*/Public/`.
- Visual Studio solution - Workspace solution file is `ArchVis.sln`.

## Key Dependencies

**Critical engine modules:**
- `Core`, `CoreUObject`, `Engine` - Present in every game/plugin module build file such as `Source/ArchVis/ArchVis.Build.cs` and `Plugins/RTPlanCore/Source/RTPlanCore/RTPlanCore.Build.cs`.
- `EnhancedInput`, `InputCore` - Input mapping and controller bindings in `Source/ArchVis/Private/ArchVisPlayerController.cpp`, `Source/ArchVis/Public/ArchVisInputConfig.h`, and `Plugins/RTPlanInput/Source/RTPlanInput/RTPlanInput.Build.cs`.
- `UMG`, `CommonUI` - Runtime UI in `Plugins/RTPlanUI/Source/RTPlanUI/` and root module widget creation in `Source/ArchVis/ArchVis.Build.cs`.
- `Json`, `JsonUtilities` - Plan serialization in `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanDocument.cpp`.
- `NetCore` - Replication support in `Plugins/RTPlanNet/Source/RTPlanNet/RTPlanNet.Build.cs` and `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp`.
- `GeometryCore`, `GeometryFramework`, `GeometryScriptingCore`, `DynamicMesh`, `RHI`, `RenderCore` - Mesh generation and Nanite-aware shell rendering in `Plugins/RTPlanMeshing/Source/RTPlanMeshing/RTPlanMeshing.Build.cs` and `Plugins/RTPlanShell/Source/RTPlanShell/RTPlanShell.Build.cs`.
- `HeadMountedDisplay`, `XRBase` - VR support in `Plugins/RTPlanVR/Source/RTPlanVR/RTPlanVR.Build.cs`.

**Infrastructure modules (internal):**
- `RTPlanCore` - Canonical document, schema, command stack, and subsystem in `Plugins/RTPlanCore/Source/RTPlanCore/`.
- `RTPlanMath` and `RTPlanSpatial` - Geometry helpers and snapping/hit-testing in `Plugins/RTPlanMath/Source/RTPlanMath/` and `Plugins/RTPlanSpatial/Source/RTPlanSpatial/`.
- `RTPlanTools` - Interactive drafting/selection tools in `Plugins/RTPlanTools/Source/RTPlanTools/`.
- `RTPlanShell` and `RTPlanMeshing` - Shell mesh generation/rendering in `Plugins/RTPlanShell/Source/RTPlanShell/` and `Plugins/RTPlanMeshing/Source/RTPlanMeshing/`.
- `RTPlanCatalog`, `RTPlanObjects`, `RTPlanRuns`, `RTPlanOpenings` - Finish catalogs, object placement, run solving, and opening helpers in their matching plugin folders.
- `RTFeature_*` plugins - Thin feature aggregators that wire higher-level experiences such as `Plugins/RTFeature_Drafting2D`, `Plugins/RTFeature_Shell3D`, `Plugins/RTFeature_Interiors`, `Plugins/RTFeature_VR`, and `Plugins/RTFeature_Multiplayer`.

## Configuration

**Environment:**
- Environment-variable based configuration is not detected.
- Runtime/project configuration is checked into Unreal INI files under `Config/`.
- Map and rendering defaults are configured in `Config/DefaultEngine.ini`.
- Input capture and default Enhanced Input classes are configured in `Config/DefaultInput.ini`.

**Build:**
- Project descriptor: `ArchVis.uproject`
- Solution: `ArchVis.sln`
- Targets: `Source/ArchVis.Target.cs`, `Source/ArchVisEditor.Target.cs`, `Source/ArchVisServer.Target.cs`, `Source/ArchVisViewer.Target.cs`, `Source/ArchVisAuthoring.Target.cs`
- Root module rules: `Source/ArchVis/ArchVis.Build.cs`
- Plugin module rules: `Plugins/*/Source/*/*.Build.cs`
- Plugin descriptors: `Plugins/*/*.uplugin`

## Platform Requirements

**Development:**
- Unreal Engine 5.7 project workspace with C++ toolchain support.
- Visual Studio / MSBuild-style workflow is implied by `ArchVis.sln` and Windows target files in `Source/`.
- Windows-focused local development is evident from `Binaries/Win64/` and `Config/DefaultEngine.ini` DX12 settings.

**Production:**
- Windows desktop rendering is the most explicit target from `Config/DefaultEngine.ini`.
- Linux and Mac shader target sections exist in `Config/DefaultEngine.ini`, but no platform-specific packaging workflow is defined in-repo.
- VR capability is present through `Plugins/RTPlanVR` and `Plugins/RTFeature_VR`, but headset-specific deployment scripts are not detected.

## Plugin Inventory

**Core planning plugins:**
- `Plugins/RTPlanCore`
- `Plugins/RTPlanMath`
- `Plugins/RTPlanSpatial`
- `Plugins/RTPlanInput`
- `Plugins/RTPlanTools`
- `Plugins/RTPlanMeshing`
- `Plugins/RTPlanShell`
- `Plugins/RTPlanOpenings`
- `Plugins/RTPlanCatalog`
- `Plugins/RTPlanObjects`
- `Plugins/RTPlanRuns`
- `Plugins/RTPlanUI`
- `Plugins/RTPlanVR`
- `Plugins/RTPlanNet`

**Feature plugins:**
- `Plugins/RTFeature_Drafting2D`
- `Plugins/RTFeature_Shell3D`
- `Plugins/RTFeature_Interiors`
- `Plugins/RTFeature_VR`
- `Plugins/RTFeature_Multiplayer`

---

*Stack analysis: 2026-03-09*

