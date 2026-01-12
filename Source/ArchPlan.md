# Runtime Interior Planner (Windows) — Complete Implementation Plan (Plugins + Authoring/Viewer + Multiplayer + VR)

> Audience: senior Unreal Engine developer(s).  
> Target: Windows client(s) + Windows dedicated server.  
> Output: Two client builds—**Authoring** (editing) and **Viewer** (visualization)—plus a **Dedicated Server**.  
> Key rule: **PlanDocument is the source of truth**; 3D meshes/actors are derived outputs rebuilt from PlanDocument state.  
> Networking rule: **Server authoritative** for PlanDocument and shared interactions (moving furniture). Unreal networking uses replication (properties/RepNotify) and RPCs to keep clients in sync; dedicated servers are a supported deployment model. [web:130][web:135]

---

## 0) Product overview

### 0.1 What the product does (V1)
- Draft an interior plan in 2D (runtime):
  - Line + Polyline for walls
  - Snapping (grid, endpoints, midpoints, projection)
  - Numeric length input while drawing (AutoCAD-like)
- Walls are parametric:
  - Per-wall **thickness** and **height**
  - Per-wall **left/right side finishes** (paint/material)
- Doors/windows:
  - Place on walls with width/height/sill (windows)
  - Wall geometry updates (V1: split-wall solids)
- Auto generate 3D interior:
  - Walls, floors, ceilings
  - Collision suitable for walk + VR
- Interior decoration:
  - Modular furniture and casework (beds, wardrobes, kitchen cabinets, appliances, decor)
  - Manual placement with snapping
  - Procedural cabinet runs (toggle + explode to manual)
  - Finishes/material overrides (wall paint, floor material, cabinet finishes)
- Multiplayer:
  - Multiple users connect to a dedicated server to see the same interior
  - **All clients can move furniture** (server-authoritative shared interaction)
  - Authoring build can edit the plan; Viewer cannot edit plan but can move furniture
- VR:
  - Walkthrough in VR (teleport + snap turn baseline) and desktop
  - Authoring build also supports VR tabletop drafting + VR UI

### 0.2 Non-goals (V1)
- Curved walls
- Editor-only tooling or baking workflows
- Arbitrary user file import (runtime import can be a future plugin)
- Perfect wall corner mitering (V1 butt join acceptable)

---

## 1) Delivery artifacts (Windows)

### 1.1 Builds
1. **Dedicated Server** (headless)
   - Holds authoritative PlanDocument and interaction state
2. **Authoring Client**
   - Full plan editing + VR tabletop drafting
   - Also participates in shared interaction (moving furniture)
3. **Viewer Client**
   - Walk/VR visualization
   - No plan editing tools
   - Can move furniture (shared)

### 1.2 Runtime-only requirement
- All features implemented in **Runtime plugins**; no Editor modules. Plugin structure and `.uplugin` descriptors are used to organize runtime modules. [web:101]

---

## 2) Architecture: canonical data + derived outputs

### 2.1 Canonical data (authoritative)
**PlanDocument** contains:
- Vertices (2D)
- Walls (A/B vertex IDs + thickness + height + baseZ + FinishLeft/FinishRight)
- Rooms (closed loops + floor/ceiling finishes)
- Openings (door/window params on walls)
- Interior objects (instances)
- Procedural runs (cabinet/wardrobe)
- Finish swatches/material definitions (or references)

### 2.2 Derived outputs (rebuilt from PlanDocument)
- Shell meshes:
  - walls, floors, ceilings
- Opening assets:
  - door/window meshes
- Interior object actors/instances:
  - furniture, cabinets, appliances
- Collision:
  - V1: primitive collision volumes (boxes) for stability and speed

---

## 3) Plugin suite (all Runtime)

> Everything below is Runtime-only and can be enabled per build. Plugins are layered so they can be used independently in other projects. [web:101]

### 3.1 Foundation plugins
- **RTPlanCore**
  - PlanDocument schema + stable IDs
  - Command stack (undo/redo)
  - Dirty tracking
  - Serialization (JSON) + versioning/migrations
- **RTPlanMath**
  - geometry utilities (segment distance, projection, polygon orientation, etc.)
  - numeric parsing helpers
  - deterministic wall left/right definitions
- **RTPlanSpatial**
  - snapping engine + 2D spatial index
  - hit testing queries
- **RTPlanInput**
  - unified pointer events for desktop and VR rays
  - action mapping (Confirm/Cancel/TextInput)
- **RTPlanTools**
  - tool framework + tools (command emitters)
  - LineTool, PolylineTool, SelectTool
  - DoorTool, WindowTool
  - PaintTool (left/right side)
  - PlaceObjectTool (hooks to catalog)
  - CabinetRunTool (hooks to runs)
- **RTPlanMeshing**
  - runtime mesh recipes / mesh target abstraction
  - isolates Geometry Script/Dynamic Mesh usage

### 3.2 Building + openings
- **RTPlanShell**
  - walls (per wall thickness/height) + left/right material sections
  - floors/ceilings from room loops
  - collision policy (V1: UBoxComponents per wall)
- **RTPlanOpenings**
  - opening placement constraints
  - V1 split-wall solids algorithm (deterministic)
  - door/window asset spawn via resolver interface

### 3.3 Interior catalog + objects + procedural runs
- **RTPlanCatalog**
  - product definitions (furniture/cabinets) + finish library
  - stable ID lookup
  - optional Asset Manager primary assets integration for scalable catalogs. [web:83]
- **RTPlanObjects**
  - spawn/update interior objects from PlanDocument
  - snapping constraints (floor clamp, wall align)
  - V1 actor-per-object, later HISM batching
- **RTPlanRuns**
  - procedural cabinet/wardrobe runs
  - toggle procedural/manual and “explode to manual”
  - optional countertop strip mesh generation hook

### 3.4 UI + VR
- **RTPlanUI**
  - desktop toolbar, property inspector, catalog browser
  - shared viewmodels for selection/tool state
- **RTPlanVR**
  - VR walk mode + VR tabletop drafting mode
  - VR radial menu + floating property panel
  - locomotion baseline: teleport + snap turn (pattern aligned with UE VR Template approach). [web:78]

### 3.5 Networking
- **RTPlanNet**
  - dedicated server authoritative replication of PlanDocument changes
  - role/permission gating (Authoring vs Viewer)
  - shared interaction replication for moving furniture (locks + transform streaming)

### 3.6 Feature bundles (runtime convenience plugins)
- **RTFeature_Drafting2D**
- **RTFeature_Shell3D**
- **RTFeature_Interiors**
- **RTFeature_VR**
- **RTFeature_Multiplayer**
These bundles ship example blueprints and “ready-to-use” actors; foundational plugins remain the core API surface.

---

## 4) Data model (required fields)

### 4.1 Wall definition (must support left/right finishes)
- `WallId`
- `VertexAId`, `VertexBId`
- `ThicknessCm`
- `HeightCm`
- `BaseZCm`
- `FinishLeftId`, `FinishRightId`, `FinishCapsId`

**Left/right rule**
- Wall direction = A→B.
- Left side = `perp(direction)` and Right side = `-perp(direction)`.
This ensures consistent painting UI and mesh material assignment.

### 4.2 Openings
- `OpeningId`
- `WallId`
- `OffsetCm` along A→B
- `WidthCm`, `HeightCm`
- `SillHeightCm` (windows)
- `Type` (Door/Window)
- `ProductTypeId` for asset selection
- `Flip` (hinge/swing)

### 4.3 Interior instance
- `ObjectId`
- `ProductTypeId`
- `Transform`
- `HostType` + optional `HostWallId`
- `GeneratedByRunId` (0 if manual)
- `Params` map (variants, finish overrides)

### 4.4 Cabinet runs
- `RunId`
- `HostWallId`
- `StartOffsetCm`, `EndOffsetCm`
- `DepthCm`, `HeightCm`
- `StyleSetId` and solver rules

---

## 5) Editing workflow (commands + undo/redo)

### 5.1 Command rules
- Every state change is a command:
  - Add/move vertex, add/edit wall params
  - Add/edit opening
  - Place/move/delete object
  - Add/edit run, explode run
  - Set finishes (left/right wall, floor, object)
- Commands compute a **DirtySet** indicating what builders must rebuild.

### 5.2 Dirty rebuild order
1. update snap spatial index
2. rebuild walls + collision
3. rebuild openings (split-wall) + door/window assets
4. rebuild floors/ceilings
5. rebuild objects
6. rebuild runs (and derived countertop meshes)

---

## 6) Drafting + snapping (runtime CAD feel)

### 6.1 Drafting modes
- Desktop: top-down ortho drafting camera
- VR: tabletop drafting (scaled plan on a table with controller ray)

### 6.2 Snapping (V1)
- grid
- endpoints
- midpoints
- projection to segment
- (recommended) ortho lock toggle

### 6.3 Numeric input (V1)
- typed length during Line/Polyline creation
- optional early enhancement:
  - angle lock increments
  - relative input formats

---

## 7) 2D → 3D shell generation

### 7.1 Walls
- For each wall, generate a prism using wall thickness and height.
- Assign left and right faces to different material sections:
  - left face uses `FinishLeftId`
  - right face uses `FinishRightId`

### 7.2 Floors/Ceilings
- Room loop triangulation → floor mesh
- Duplicate at ceiling height → ceiling mesh

### 7.3 Collision (V1)
- Use UBoxComponents per wall segment:
  - stable VR collision and cheap updates

---

## 8) Doors/windows (openings) — V1 deterministic method

### 8.1 Split-wall solids
- For each wall:
  1. collect openings
  2. convert to intervals along wall length
  3. sort + merge
  4. compute solid intervals = complement
  5. generate prisms only for solid intervals
- Spawn door/window meshes at opening locations.

Later upgrade:
- booleans (Geometry Script) if needed for complex cases.

---

## 9) Interior objects (manual) + catalog

### 9.1 Catalog requirements
- Each product:
  - stable ID
  - mesh reference
  - footprint bounds
  - default materials/finish slots
  - placement constraints (floor/wall/ceiling)
- Finish library:
  - paint swatches, wood, tile, metal, glass

### 9.2 Placement
- Floor-hosted objects:
  - clamp Z to floor, snap rotation increments
- Wall-hosted objects:
  - align back plane to wall normal, maintain offset

### 9.3 Rendering strategy
- V1: actor-per-object (simplest)
- V1.1+: HISM batching by product type

---

## 10) Procedural cabinet runs (toggle + explode)

### 10.1 Solver (V1)
- Greedy fill using allowed widths + filler piece policy
- Deterministic output

### 10.2 Output representation
- Generated items are stored as normal interior instances tagged with `GeneratedByRunId`.

### 10.3 Explode
- Removes linkage and converts everything to manual.

---

## 11) VR support (authoring + viewer)

### 11.1 Walk mode
- VR locomotion baseline: teleport + snap turn approach (VR Template style). [web:78]
- Interaction:
  - grab furniture
  - inspect materials/objects

### 11.2 Tabletop drafting (authoring)
- scaled plan root on a table
- controller ray becomes drafting pointer
- VR radial menu selects tools
- floating panel edits properties

---

## 12) Multiplayer (Dedicated server + multiple clients)

### 12.1 Roles
- **Authoring client**: can send PlanDocument edit commands to server.
- **Viewer client**: cannot edit plan but can move furniture.
- **Server**: authoritative for PlanDocument and interactions. [web:130][web:135]

### 12.2 Replication strategy (recommended)
- Replicate **PlanDocument deltas** (command stream) to clients.
- Clients rebuild shell/objects locally from the replicated PlanDocument.
- Do not replicate generated wall mesh actors (avoid enormous actor counts).

### 12.3 Furniture movement (shared interaction)
All clients can move furniture, but server is authoritative:

**Protocol**
- Grab start:
  - Client → Server RPC: `BeginGrab(ObjectId)`
  - Server grants lock if available.
- Grab update (stream):
  - Client → Server RPC: `UpdateGrab(ObjectId, TargetTransform)` (unreliable, throttled ~10–20Hz)
  - Server updates authoritative actor transform and replicates to other clients.
- Grab end:
  - Client → Server RPC: `EndGrab(ObjectId, FinalTransform)` (reliable)
  - Server commits final transform into PlanDocument as a TransformObject command and replicates it via command stream.

**Locking**
- Per-object lock: `LockedByPlayerId`, `ExpiryTime`.
- If lock denied, client shows “occupied”.

**Late join**
- Server sends snapshot + command tail so late join sees final furniture positions.

This aligns with UE’s server-authoritative replication model where clients request changes and server replicates state. [web:130]

---

## 13) Build configurations (what is enabled)

### 13.1 Authoring client enables
- All plugins:
  - Core, drafting tools, shell/openings, catalog/objects/runs, UI, VR, Net
- Full tool UI (desktop + VR)
- Permission: Editor

### 13.2 Viewer client enables
- Core, shell/openings, catalog/objects, UI (limited), VR (walk), Net
- Tool plugins may be disabled or compiled but **feature-gated** off
- Permission: Viewer + Mover (can move furniture only)

### 13.3 Dedicated server enables
- Core + Net (+ optional Catalog for validation)
- No UI/VR/meshing required
- Dedicated server setup is a standard UE deployment pattern. [web:135]

---

## 14) Validation and test plan (per milestone)

> Use both automation tests (fast deterministic) and functional tests (level-driven). UE provides an Automation Test Framework and Functional Testing framework suitable for this. [web:116][web:119]

### Milestone T0 — Plugin skeleton + packaging gate
- Test: package Shipping Windows with all runtime plugins enabled (no editor dependencies). [web:101]

### Milestone T1 — PlanCore correctness
- Automation:
  - Save/Load roundtrip equality
  - Undo/redo determinism
  - DocHash stable

### Milestone T2 — Snapping & drafting correctness
- Automation:
  - snap priority tests
  - typed length exactness tests
- Functional:
  - draw rectangle room → room loop exists and dimensions match

### Milestone T3 — Shell generation correctness
- Automation:
  - wall prism bounds match thickness/height
  - left/right material assignment correctness
  - room triangulation area approx match
- Functional:
  - pawn collision blocks walls

### Milestone T4 — Openings correctness
- Automation:
  - split-wall intervals correct (single + multiple openings)
  - sub-walls inherit wall params
- Functional:
  - door placed creates visible opening + collision works

### Milestone T5 — Catalog + object placement correctness
- Automation:
  - product ID resolves
  - wall alignment constraint correctness
- Functional:
  - place bed, save, reload: same pose

### Milestone T6 — Procedural runs correctness
- Automation:
  - solver deterministic
  - explode preserves objects and removes run
- Functional:
  - edit run length regenerates modules

### Milestone T7 — Multiplayer replication correctness
- Functional (server + 2 clients):
  - authoring edits replicate and DocHash matches on all clients
  - late join receives full state
- Furniture movement:
  - lock contention works
  - moving is visible to other clients in near real-time

### Milestone T8 — VR acceptance
- Manual VR acceptance:
  - connect to server
  - walk around with teleport + snap turn
  - move furniture
  - authoring VR tabletop drafting edits replicate to viewers

---

## 15) Implementation sequence (action checklist)

### Phase 0: Scaffolding
- Create all runtime plugins + compile gates
- Add demo project and test maps:
  - `L_DraftTest`, `L_WalkTest`, `L_VRTest`, `L_NetTest`

### Phase 1: Core
- RTPlanCore: PlanDocument + commands + JSON + DocHash

### Phase 2: Math/Spatial
- RTPlanMath + RTPlanSpatial: snapping and geometry queries

### Phase 3: Input/Tools (desktop first)
- RTPlanInput + RTPlanTools: Line/Polyline/Select + numeric length

### Phase 4: Meshing/Shell
- RTPlanMeshing + RTPlanShell: walls/floor/ceiling + left/right materials + collision

### Phase 5: Openings
- RTPlanOpenings: split-wall + door/window spawn

### Phase 6: Catalog/Objects
- RTPlanCatalog + RTPlanObjects: manual furniture placement + constraints

### Phase 7: Procedural runs
- RTPlanRuns: cabinet run generator + countertop + explode

### Phase 8: UI
- RTPlanUI: toolbar, property inspector, catalog, paint picker

### Phase 9: Networking
- RTPlanNet: server authoritative PlanDocument command replication
- Add viewer vs authoring permissions
- Add object move interaction (locks + streaming updates + final commit)

### Phase 10: VR
- RTPlanVR: walk + tabletop drafting + VR UI
- Integrate with RTPlanNet for shared sessions

---

## 16) Open decisions to confirm (developer must ask product owner)
1. Furniture constraints while moving:
   - Freeform vs snap-to-floor vs collision avoidance (recommended V1: snap-to-floor + allow overlaps; warn only).
2. Who can move what:
   - All objects movable, or only objects flagged movable in ProductDef.
3. Persistence policy:
   - Are viewer moves saved permanently (server PlanDocument), or is there a “reset to authored layout” feature?
4. Session model:
   - One active PlanDocument per server instance, or multiple rooms/sessions?

---

## 17) Acceptance criteria (V1)
- Authoring client can draft a room with typed lengths, paint both wall sides, add doors/windows, place furniture, generate kitchen run.
- Viewer clients can join the same server session and see the identical interior.
- All clients can grab/move furniture; movement is replicated and final positions persist (late join sees final positions).
- VR walk mode works (teleport + snap-turn) and supports moving furniture.
- Authoring VR tabletop drafting edits replicate to all clients.
- Windows Shipping builds package successfully (no editor dependencies).

---
