# External Integrations

**Analysis Date:** 2026-03-09

## APIs & External Services

**External SaaS / third-party APIs:**
- Not detected. No Stripe, AWS SDK, Supabase, PlayFab, Firebase, REST client, or HTTP client integration is evident in `Source/ArchVis/` or `Plugins/*/Source/*/` from the inspected code.

**Engine-level platform services:**
- Unreal networking - `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp` replicates plan state using Unreal replication and server RPCs.
- VR runtime integration - `Plugins/RTPlanVR/Source/RTPlanVR/RTPlanVR.Build.cs` depends on `HeadMountedDisplay` and `XRBase` for headset integration through engine APIs.
- Modeling/editor support - `ArchVis.uproject` enables `ModelingToolsEditorMode` for editor targets.

## Data Storage

**Databases:**
- None detected.
- The authoritative in-memory model is `URTPlanDocument` in `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanDocument.h`.
- JSON serialization/deserialization is implemented in `Plugins/RTPlanCore/Source/RTPlanCore/Private/RTPlanDocument.cpp` using `JsonObjectConverter`.

**File Storage:**
- Local Unreal asset/content storage only.
  - Maps and assets live under `Content/`.
  - Plugin content is allowed where `CanContainContent` is true, for example `Plugins/RTPlanUI/RTPlanUI.uplugin`.
- A custom persisted save pipeline is not detected in the inspected runtime code.

**Caching:**
- Unreal derived-data cache exists locally in `DerivedDataCache/`.
- Runtime-specific application caching service is not detected.

## Authentication & Identity

**Auth Provider:**
- None detected.
- `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp` uses Unreal server RPC validation methods, but there is no user identity provider or permission service wired in the inspected code.

## Monitoring & Observability

**Error Tracking:**
- No external error tracking service is detected.

**Logs:**
- Use Unreal logging via `UE_LOG` and log categories such as:
  - `Source/ArchVis/Private/ArchVisGameMode.cpp`
  - `Source/ArchVis/Private/ArchVisPlayerController.cpp`
  - `Plugins/RTPlanTools/Source/RTPlanTools/Private/Tools/RTPlanLineTool.cpp`
  - `Plugins/RTPlanShell/Source/RTPlanShell/Public/RTPlanShellActor.h` declares `LogRTPlanShell`

## CI/CD & Deployment

**Hosting:**
- Not detected in-repo.
- The project is an Unreal application, not a serverless/web deployment target.

**CI Pipeline:**
- Not detected. No GitHub Actions workflow, TeamCity config, Azure pipeline, or similar delivery pipeline is present in the inspected files.

## Environment Configuration

**Required env vars:**
- Not detected.
- Configuration appears file-based via `Config/DefaultEngine.ini`, `Config/DefaultInput.ini`, `Config/DefaultEditor.ini`, and `Config/DefaultGame.ini`.

**Secrets location:**
- No secret-bearing config file was inspected.
- `.env` or secret credential files were not used as mapping sources.

## Webhooks & Callbacks

**Incoming:**
- None detected.

**Outgoing:**
- None detected.

## Internal Integration Boundaries

**Game module ↔ core planning model:**
- `Source/ArchVis/Private/ArchVisGameMode.cpp` creates `URTPlanDocument` and `URTPlanToolManager`, then wires them into the runtime.
- `Source/ArchVis/Private/ArchVisPlayerController.cpp` registers those objects with `URTPlanSubsystem` from `Plugins/RTPlanCore/Source/RTPlanCore/Public/RTPlanSubsystem.h`.

**Document ↔ rendering:**
- `Plugins/RTPlanShell/Source/RTPlanShell/Public/RTPlanShellActor.h` listens for plan changes and rebuilds shell meshes.

**Document ↔ networking:**
- `Plugins/RTPlanNet/Source/RTPlanNet/Private/RTPlanNetDriver.cpp` serializes document state to `ReplicatedPlanJson` and applies updates on clients through `OnRep_PlanJson`.

**Document ↔ tools and snapping:**
- `Plugins/RTPlanTools/Source/RTPlanTools/Private/RTPlanToolManager.cpp` subscribes to `URTPlanDocument::OnPlanChanged` and rebuilds the `FRTPlanSpatialIndex` from `Plugins/RTPlanSpatial/Source/RTPlanSpatial/Private/RTPlanSpatialIndex.cpp`.

**Catalog ↔ shell/material assignment:**
- `Plugins/RTPlanShell/Source/RTPlanShell/Public/RTPlanShellActor.h` exposes finish catalog integration through `SetFinishCatalog` / `GetFinishCatalog` and material lookup methods.

## Plugin Descriptor Dependencies

**Example descriptor-level plugin links:**
- `Plugins/RTPlanUI/RTPlanUI.uplugin` enables `RTPlanCore`, `RTPlanTools`, and `CommonUI`.
- `ArchVis.uproject` enables every `RTPlan*` and `RTFeature_*` plugin plus editor-only `ModelingToolsEditorMode`.

---

*Integration audit: 2026-03-09*

