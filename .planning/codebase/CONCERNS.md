# Codebase Concerns

**Analysis Date:** 2026-03-09

## Tech Debt

**Monolithic player controller:**
- Issue: `AArchVisPlayerController` owns startup sequencing, pawn switching, input binding, view-state management, selection, wall properties widget setup, numeric entry, and many tool handlers in one file.
- Files: `Source/ArchVis/Private/ArchVisPlayerController.cpp`
- Impact: High change risk, difficult testing, and a large blast radius for input or mode changes.
- Fix approach: Extract mode-specific handlers and tool/UI coordinators into plugin or component classes; keep the controller focused on delegation.

**Cycle-avoidance workaround at subsystem boundary:**
- Issue: `URTPlanSubsystem` stores the tool manager as `UObject*` instead of a typed dependency to avoid module cycles.
- Files: `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanSubsystem.h`, `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanSubsystem.cpp`
- Impact: Weak typing at a central service boundary and more runtime casting risk.
- Fix approach: Introduce an interface module or refactor ownership so `RTPlanCore` depends on abstractions rather than concrete tool types.

**Feature plugins are mostly dependency wrappers:**
- Issue: Several `RTFeature_*` plugins mainly declare dependencies and thin module bootstrap files.
- Files: `Plugins/RTFeature_Drafting2D/Source/RTFeature_Drafting2D/RTFeature_Drafting2D.Build.cs`, `Plugins/RTFeature_Shell3D/Source/RTFeature_Shell3D/RTFeature_Shell3D.Build.cs`, `Plugins/RTFeature_Interiors/Source/RTFeature_Interiors/RTFeature_Interiors.Build.cs`, `Plugins/RTFeature_VR/Source/RTFeature_VR/RTFeature_VR.Build.cs`, `Plugins/RTFeature_Multiplayer/Source/RTFeature_Multiplayer/RTFeature_Multiplayer.Build.cs`
- Impact: Plugin count is high relative to behavior, which adds maintenance overhead and can obscure where real logic belongs.
- Fix approach: Keep them only if packaging/ownership boundaries matter; otherwise consolidate or document their composition role more explicitly.

## Known Bugs

**Opening deletion is unfinished:**
- Symptoms: Deleting selected openings is stubbed out; only wall deletion is implemented.
- Files: `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp`
- Trigger: Call `DeleteSelection()` with openings selected.
- Workaround: Delete walls only; opening deletion command path is commented out.

**Selection and camera-view workflows are incomplete:**
- Symptoms: Box selection, overlap cycling, and view shortcuts are marked TODO and may not perform the expected action.
- Files: `Source/ArchVis/Private/ArchVisPlayerController.cpp`
- Trigger: Use the related commands around the TODO-marked sections near lines flagged by the grep inventory.
- Workaround: Use currently implemented direct tool interactions and existing camera modes only.

**Placement tool command path is incomplete:**
- Symptoms: Object placement tool logic contains a TODO rather than a finished add-object command submission.
- Files: `Plugins/RTPlanTools/Source/RTPlanTools/Private/Tools/RTPlanPlaceTool.cpp`
- Trigger: Attempt to use object placement flows that depend on the missing command.
- Workaround: Use implemented wall/line/trim/arc flows and object-management code paths that do not depend on `RTPlanPlaceTool` completion.

## Security Considerations

**Server RPC validation is permissive:**
- Risk: RPC validators currently return `true` without authorization or payload checks.
- Files: `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp`
- Current mitigation: Unreal authority checks gate some logic with `HasAuthority()`.
- Recommendations: Add document mutation permission checks, rate limiting, payload validation, and ownership/session rules before using this module for real multiplayer editing.

**Full document replication over JSON:**
- Risk: Entire plan state is serialized into `ReplicatedPlanJson`, increasing exposure to oversized payloads and potentially malformed client/server sync states.
- Files: `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp`, `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanDocument.cpp`
- Current mitigation: Replication is debounced by a timer.
- Recommendations: Move toward validated delta replication or authoritative command replication.

## Performance Bottlenecks

**Whole-document spatial rebuilds:**
- Problem: The spatial index is rebuilt on every plan change.
- Files: `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp`, `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Private/RTPlanSpatialIndex.cpp`
- Cause: `URTPlanToolManager::UpdateSpatialIndex()` calls `SpatialIndex.Build(Document)` after document change broadcasts.
- Improvement path: Track incremental dirty regions/entities or batch rebuilds during drag/continuous edits.

**Whole-document JSON replication:**
- Problem: Network sync sends serialized plan JSON instead of structural deltas.
- Files: `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp`
- Cause: `OnPlanChanged()` serializes `Document->ToJson()` into `ReplicatedPlanJson`.
- Improvement path: Replicate commands or entity deltas, and compress/bound payload size.

**Potentially heavy mesh rebuilds:**
- Problem: Shell rendering rebuilds dynamic geometry in response to plan changes.
- Files: `Plugins/RTPlanShell/Source/RTPlanShell/Public/RTPlanShellActor.h`, `Plugins/RTPlanMeshing/Source/RTPlanMeshing/Private/RTPlanMeshBuilder.cpp`
- Cause: Observer-style full rebuild behavior on document mutations.
- Improvement path: Add partial rebuilds, dirty wall tracking, and a clear edit-vs-baked rendering path.

## Fragile Areas

**Startup ordering between GameMode and PlayerController:**
- Files: `Source/ArchVis/Private/ArchVisGameMode.cpp`, `Source/ArchVis/Private/ArchVisPlayerController.cpp`
- Why fragile: Comments explicitly note that some setup is deferred because `GameMode::StartPlay` creates objects after `BeginPlay`.
- Safe modification: Preserve the `OnGameModeReady()` handoff or replace it with a clearer lifecycle contract before moving initialization code.
- Test coverage: No direct automation test covering this orchestration path was found.

**Geometry-heavy tool logic:**
- Files: `Plugins/RTPlanTools/Source/RTPlanTools/Private/Tools/RTPlanLineTool.cpp`, `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Private/RTPlanSpatialIndex.cpp`
- Why fragile: Complex snapping, angle constraints, and polyline state combine input, geometry, and command behavior.
- Safe modification: Add/extend automation tests before changing snapping or drafting behavior.
- Test coverage: Tool and spatial tests exist, but `RTPlanLineTool.cpp` is substantially larger than the sample tests.

**Shell/openings dependency boundary:**
- Files: `Plugins/RTPlanShell/Source/RTPlanShell/RTPlanShell.Build.cs`, `Plugins/RTPlanOpenings/Source/RTPlanOpenings/RTPlanOpenings.Build.cs`
- Why fragile: Build comments show active work to break circular dependencies while still sharing geometry/opening behavior.
- Safe modification: Keep module dependencies acyclic and move shared abstractions into neutral plugins if more cross-calls are added.
- Test coverage: `Plugins/RTPlanShell/Source/RTPlanShell/Private/RTPlanShellTests.cpp` and `Plugins/RTPlanOpenings/Source/RTPlanOpenings/Private/RTPlanOpeningsTests.cpp` cover slices, not the full boundary.

## Scaling Limits

**Authoritative single document model:**
- Current capacity: Suitable for a single in-memory `URTPlanDocument` and local editing workflows.
- Limit: Full rebuilds and full serialization become increasingly expensive as wall/object/opening counts grow.
- Scaling path: Introduce chunking, delta events, and subsystem-level partitioning for large plans.

**Controller-centric interaction surface:**
- Current capacity: Works while a single controller coordinates most UI/input behavior.
- Limit: More modes, platforms, or collaborative workflows will compound complexity in `Source/ArchVis/Private/ArchVisPlayerController.cpp`.
- Scaling path: Split editor-mode coordinators, UI presenters, and tool adapters into smaller units.

## Dependencies at Risk

**Geometry/render feature coupling:**
- Risk: `RTPlanShell` directly depends on multiple geometry/render modules plus `RTPlanOpenings` and `RTPlanCatalog`.
- Impact: Rendering changes can ripple through unrelated shell/opening/catalog concerns.
- Migration plan: Isolate pure mesh generation behind narrower interfaces and keep rendering/material concerns separate from plan topology concerns.

**CommonUI dependency for runtime widget plugin:**
- Risk: `Plugins/RTPlanUI/RTPlanUI.uplugin` and `Plugins/RTPlanUI/Source/RTPlanUI/RTPlanUI.Build.cs` require `CommonUI` in addition to `UMG`.
- Impact: UI portability depends on that plugin remaining enabled in all relevant targets.
- Migration plan: Keep CommonUI usage explicit in docs/assets, or reduce to plain UMG if advanced CommonUI features are not used.

## Missing Critical Features

**Transactional multi-delete / macro commands:**- Problem: Multi-item deletion is not grouped into a single undoable transaction.
- Blocks: Clean editing UX for bulk operations.

**Opening delete command implementation:**
- Problem: No implemented `URTCmdDeleteOpening` path is wired in `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp`.
- Blocks: Full parity between wall and opening selection deletion.

**Completed box selection and selection cycling:**
- Problem: TODOs remain in `Source/ArchVis/Private/ArchVisPlayerController.cpp`.
- Blocks: Rich desktop/CAD-style selection workflows.

**Finished camera/view shortcuts:**
- Problem: Top/front/right/ortho toggles are stubbed with TODO comments in `Source/ArchVis/Private/ArchVisPlayerController.cpp`.
- Blocks: Faster authoring navigation.

## Test Coverage Gaps

**Root module orchestration:**
- What's not tested: `AArchVisGameMode` and `AArchVisPlayerController` startup wiring, input context switching, and subsystem registration.
- Files: `Source/ArchVis/Private/ArchVisGameMode.cpp`, `Source/ArchVis/Private/ArchVisPlayerController.cpp`
- Risk: Regressions in startup ordering or input binding can slip through without automation coverage.
- Priority: High.

**Multiplayer authorization/security behavior:**
- What's not tested: RPC validation rules, abuse resistance, and malformed payload handling.
- Files: `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp`
- Risk: Insecure multiplayer behavior if the feature is expanded.
- Priority: High.

**Incomplete tools and feature plugins:**
- What's not tested: TODO-backed flows such as placement commands, advanced selection behavior, and feature plugin composition.
- Files: `Plugins/RTPlanTools/Source/RTPlanTools/Private/Tools/RTPlanPlaceTool.cpp`, `Source/ArchVis/Private/ArchVisPlayerController.cpp`, `Plugins/RTFeature_*/Source/*/Private/*Module.cpp`
- Risk: Stubs can look wired but fail at runtime when exercised.
- Priority: Medium.

---

*Concerns audit: 2026-03-09*

