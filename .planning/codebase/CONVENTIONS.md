# Coding Conventions

**Analysis Date:** 2026-03-09

## Naming Patterns

**Files:**
- Use Unreal-style PascalCase filenames matching the primary type, for example `Source/ArchVis/Public/ArchVisPlayerController.h`, `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanDocument.h`, and `Plugins/RTPlanTools/Source/RTPlanTools/Public/Tools/RTPlanLineTool.h`.
- Module bootstrap files use `<ModuleName>Module.h/.cpp`, for example `Plugins/RTPlanVR/Source/RTPlanVR/Public/RTPlanVRModule.h`.
- Build metadata uses standard Unreal names: `ArchVis.uproject`, `Plugins/RTPlanUI/RTPlanUI.uplugin`, `Source/ArchVis/ArchVis.Build.cs`, and `Source/ArchVisEditor.Target.cs`.

**Functions:**
- Use PascalCase method names following Unreal style, for example `BeginPlay`, `StartPlay`, `SetDocument`, `SelectToolByType`, `UpdateSpatialIndex`, and `OnGameModeReady`.
- Event/handler names use `On*` / `Handle*` prefixes, for example `OnPlanChanged` in `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp` and `HandleSelectionChanged` in `Source/ArchVis/Private/ArchVisPlayerController.cpp`.

**Variables:**
- Booleans use Unreal `b` prefixes, for example `bShowMouseCursor`, `bSnapEnabled`, `bGridEnabled`, `bReplicates`, `bAlwaysRelevant`, and `bNumericInputActive`.
- Input parameters commonly use `In*` prefixes, for example `SetDocument(URTPlanDocument* InDoc)` in `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp` and `Initialize(URTPlanDocument* InDoc)` in `Plugins/RTPlanTools/Source/RTPlanTools/Public/RTPlanToolManager.h`.
- Temporary variables are short but descriptive, for example `Doc`, `ToolMgr`, `World`, `Bounds`, `SelectedWalls`, and `CurrentEndPoint`.

**Types:**
- Unreal prefixes are followed consistently:
  - `A*` for actors/controllers/pawns like `AArchVisGameMode` and `ARTPlanShellActor`
  - `U*` for UObject types like `URTPlanDocument` and `URTPlanToolManager`
  - `F*` for structs/value types like `FRTPlanData`, `FRTWall`, and `FRTDraftingState`
  - `E*` for enums like `ERTPlanToolType` and `EArchVisPawnType`

## Code Style

**Formatting:**
- Tabs for indentation are common across `Source/ArchVis/*.cpp` and `Plugins/*/Source/*/*.cpp`.
- Opening braces typically appear on the next line for functions/classes, for example `Source/ArchVis/Private/ArchVisGameMode.cpp` and `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp`.
- Inline comments are used to explain intent at the statement level, especially in gameplay orchestration and geometry code.

**Linting:**
- No repo-local clang-format, `.editorconfig`, ESLint, Prettier, or Biome config is detected.
- Enforced style appears to rely on Unreal conventions and existing file patterns rather than a checked-in formatter config.

## Import Organization

**Order:**
1. Include the matching header first, for example `#include "ArchVisPlayerController.h"` in `Source/ArchVis/Private/ArchVisPlayerController.cpp` and `#include "RTPlanToolManager.h"` in `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp`.
2. Include sibling project/plugin headers next, for example `RTPlan` tool and domain headers.
3. Include engine/framework headers last, for example `Kismet/GameplayStatics.h`, `EnhancedInputComponent.h`, `Engine/World.h`, and `Misc/AutomationTest.h`.

**Path Aliases:**
- No custom include alias system is detected.
- Includes use Unreal relative module paths such as `#include "Tools/RTPlanLineTool.h"`, `#include "Input/ToolInputComponent.h"`, and engine include paths like `#include "Engine/World.h"`.

## Error Handling

**Patterns:**
- Prefer guard clauses and early returns over nested control flow. Examples:
  - `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanSubsystem.cpp`
  - `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp`
  - `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp`
- Return booleans for command success/failure, for example `URTPlanDocument::SubmitCommand()` and `URTPlanDocument::FromJson()` in `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanDocument.cpp`.
- Prefer `nullptr`/empty container returns when data is unavailable, for example `GetSelectTool()`, `GetSelectedWallIds()`, and subsystem `Get()` accessors.

## Logging

**Framework:** Unreal `UE_LOG` with log categories.

**Patterns:**
- Define local log categories with `DEFINE_LOG_CATEGORY_STATIC` in implementation files that have substantial runtime behavior, for example:
  - `Source/ArchVis/Private/ArchVisGameMode.cpp`
  - `Source/ArchVis/Private/ArchVisPlayerController.cpp`
  - `Plugins/RTPlanTools/Source/RTPlanTools/Private/Tools/RTPlanLineTool.cpp`
- Use logs for startup sequencing, tool activation/deactivation, and verbose geometric tracing.
- Use `Log`, `Warning`, `Error`, and `Verbose` severity levels rather than custom logging wrappers.

## Comments

**When to Comment:**
- Comment non-obvious engine behavior, workflow steps, and constraints, especially around startup order, spatial math, snapping, and dependency-cycle workarounds.
- Examples include the startup sequencing comments in `Source/ArchVis/Private/ArchVisPlayerController.cpp` and the arc/segment explanation in `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Private/RTPlanSpatialIndex.cpp`.

**JSDoc/TSDoc:**
- C++ API comments use Unreal-style block comments above reflected classes and methods rather than JS-style doc tooling.
- Good examples:
  - `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanSubsystem.h`
  - `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanDocument.h`
  - `Plugins/RTPlanShell/Source/RTPlanShell/Public/RTPlanShellActor.h`

## Function Design

**Size:**
- Small helper methods are common in plugin modules, but the root controller contains very large multi-responsibility methods and handlers. Keep new code smaller than `Source/ArchVis/Private/ArchVisPlayerController.cpp` and prefer plugin-local helpers when extending behavior.

**Parameters:**
- Pass Unreal objects as raw pointers and check them before use.
- Use structs/enums for tool and document interactions, for example `FRTPointerEvent`, `FRTWall`, `FGuid`, and `ERTPlanToolType`.

**Return Values:**
- Use direct primitive/struct returns for query methods, for example `GetDraftingState()`, `GetCurrentLengthCm()`, and `GetCurrentAngleDegrees()`.
- Use `bool` for operations that can fail without exceptions.

## Module Design

**Exports:**
- Public reflected/runtime types use Unreal API macros such as `RTPLANCORE_API`, `RTPLANTOOLS_API`, `RTPLANSHELL_API`, `RTPLANUI_API`, and `RTPLANNET_API` in public headers under each plugin.
- Modules generally expose a small public surface and keep implementation in `Private/`.

**Barrel Files:**
- Barrel/header aggregation files are not a dominant pattern.
- Modules are consumed through direct header includes rather than umbrella exports.

## Practical Rules to Follow

- Put reusable plan logic in the owning plugin, not in `Source/ArchVis/Private/ArchVisPlayerController.cpp`.
- Match Unreal naming/prefix conventions for every new type.
- Include the matching header first in each `.cpp` file.
- Use `UFUNCTION`, `UPROPERTY`, and API macros consistently when exposing runtime/editor behavior.
- Prefer guard clauses, `UE_LOG`, and boolean return values over exception-style control flow.
- Add new automation tests beside the owning module in that module’s `Private/` folder.

---

*Convention analysis: 2026-03-09*

