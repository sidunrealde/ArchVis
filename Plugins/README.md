# Runtime Interior Planner Plugin Suite

This directory contains the modular plugin suite for the Runtime Interior Planner application. All plugins are designed to be **Runtime-only** (no Editor dependencies) to support shipping builds on Windows clients and Dedicated Servers.

## 1. Foundation Plugins

### **RTPlanCore**
*   **Description**: Defines the canonical data model (`PlanDocument`) and the Command pattern infrastructure.
*   **Key Classes**:
    *   `URTPlanDocument`: Authoritative container for the plan data (Walls, Vertices, Objects). Handles Undo/Redo stacks and JSON serialization.
    *   `URTCommand`: Base class for all state-modifying actions (e.g., `URTCmdAddWall`).
    *   `FRTPlanData`: The raw data struct containing all map data.
*   **Dependencies**: None (Core).

### **RTPlanMath**
*   **Description**: Pure math and geometry utilities for 2D plan operations.
*   **Key Features**:
    *   `FRTPlanGeometryUtils`: Distance point-to-segment, projection, wall normal calculations (Left/Right), and intersection tests.
*   **Dependencies**: RTPlanCore.

### **RTPlanSpatial**
*   **Description**: Spatial indexing system for fast queries and snapping.
*   **Key Features**:
    *   `FRTPlanSpatialIndex`: Builds a cache of endpoints, midpoints, and edges from the `PlanDocument`.
    *   `QuerySnap`: Finds the closest snap point (Endpoint, Midpoint, Grid) for tools.
*   **Dependencies**: RTPlanCore, RTPlanMath.

### **RTPlanInput**
*   **Description**: Unified input handling for Desktop (Mouse) and VR (Motion Controllers).
*   **Key Features**:
    *   `FRTPointerEvent`: A unified struct representing a pointer ray and action state (Down, Up, Move).
    *   Abstraction for input sources (`Mouse`, `Touch`, `VRLeft`, `VRRight`).
*   **Dependencies**: RTPlanCore, EnhancedInput.

### **RTPlanTools**
*   **Description**: The interactive tool framework.
*   **Key Classes**:
    *   `URTPlanToolManager`: Manages the active tool and routes input events.
    *   `URTPlanToolBase`: Base class for tools. Handles raycasting against the ground plane and snapping.
    *   `URTPlanLineTool`: Example tool for drawing walls via Click-Drag-Release.
    *   `URTPlanPlaceTool`: Tool for placing furniture objects from the catalog.
*   **Dependencies**: RTPlanCore, RTPlanInput, RTPlanSpatial, RTPlanCatalog, RTPlanObjects.

### **RTPlanMeshing**
*   **Description**: Geometry generation recipes using Geometry Scripting.
*   **Key Features**:
    *   `FRTPlanMeshBuilder`: Static helpers to generate Dynamic Meshes for walls (Box) and floors (Triangulated Polygon).
*   **Dependencies**: RTPlanCore, GeometryScripting.

---

## 2. Building & Openings

### **RTPlanShell**
*   **Description**: Renders the 3D architectural shell (Walls, Floors).
*   **Key Classes**:
    *   `ARTPlanShellActor`: Listens to `PlanDocument` changes and rebuilds the Dynamic Mesh for walls. Handles wall transforms and dimensions.
*   **Dependencies**: RTPlanCore, RTPlanMeshing, RTPlanMath, RTPlanOpenings.

### **RTPlanOpenings**
*   **Description**: Logic for handling wall openings (Doors, Windows).
*   **Key Features**:
    *   `FRTPlanOpeningUtils`: Algorithms to compute "solid intervals" of a wall by subtracting opening widths. Used by `RTPlanShell` to cut holes.
*   **Dependencies**: RTPlanCore.

---

## 3. Interiors & Catalog

### **RTPlanCatalog**
*   **Description**: Product definitions and library management.
*   **Key Classes**:
    *   `URTProductCatalog`: Data Asset holding a list of `FRTProductDefinition`.
    *   `FRTProductDefinition`: Defines a product's ID, Display Name, Mesh, and placement constraints.
*   **Dependencies**: RTPlanCore.

### **RTPlanObjects**
*   **Description**: Manages the lifecycle of 3D interior objects (Furniture).
*   **Key Classes**:
    *   `ARTPlanObjectManager`: Spawns, updates, and destroys `AStaticMeshActor` instances to match the `PlanDocument` state.
*   **Dependencies**: RTPlanCore, RTPlanCatalog.

### **RTPlanRuns**
*   **Description**: Procedural generation for cabinet and wardrobe runs.
*   **Key Classes**:
    *   `FRTPlanRunSolver`: Greedy algorithm to fill a linear space with standard module widths (60cm, 45cm, etc.).
    *   `ARTPlanRunManager`: Generates virtual object instances in the document based on Run definitions.
*   **Dependencies**: RTPlanCore, RTPlanObjects.

---

## 4. UI, VR & Networking

### **RTPlanUI**
*   **Description**: User Interface widgets (UMG).
*   **Key Classes**:
    *   `URTPlanUIBase`: Root HUD widget.
    *   `URTPlanToolbar`: Tool selection UI.
    *   `URTPlanProperties`: Property inspector for selected items.
    *   `URTPlanCatalogBrowser`: Browser for the product catalog.
*   **Dependencies**: RTPlanCore, RTPlanTools, RTPlanCatalog, CommonUI.

### **RTPlanVR**
*   **Description**: VR specific interaction and modes.
*   **Key Classes**:
    *   `ARTPlanVRPawn`: VR Pawn with motion controller support and input mapping to `FRTPointerEvent`.
    *   `ARTPlanVRModeManager`: Handles switching between "Walk" (1:1 scale) and "Tabletop" (miniature) modes.
*   **Dependencies**: RTPlanCore, RTPlanInput, RTPlanUI, XRBase.

### **RTPlanNet**
*   **Description**: Networking and Replication.
*   **Key Classes**:
    *   `ARTPlanNetDriver`: Server-authoritative actor that replicates the `PlanDocument` (as JSON) to clients and handles RPCs for commands (`Server_SubmitAddWall`).
*   **Dependencies**: RTPlanCore.

---

## 5. Feature Bundles

These plugins group dependencies to easily enable/disable entire feature sets in the `.uproject` file.

*   **RTFeature_Drafting2D**: Enables Core, Tools, and UI for 2D drafting.
*   **RTFeature_Shell3D**: Enables Shell generation and Openings.
*   **RTFeature_Interiors**: Enables Catalog, Objects, and Procedural Runs.
*   **RTFeature_VR**: Enables VR Pawn and interaction.
*   **RTFeature_Multiplayer**: Enables Networking support.

## Usage

1.  **Enable Plugins**: Ensure the desired plugins (or Feature Bundles) are enabled in `ArchVis.uproject`.
2.  **Setup Scene**:
    *   Place `ARTPlanShellActor`, `ARTPlanObjectManager`, and `ARTPlanNetDriver` in your level.
    *   Initialize them with a `URTPlanDocument` instance (usually created by a GameState or GameMode).
3.  **Tools**:
    *   Create a `URTPlanToolManager` and initialize it with the document.
    *   Route input from your PlayerController or Pawn to `ToolManager->ProcessInput()`.
