# Implementation TODO: Drafting Modes & Advanced Tools

This document provides a detailed technical guide for implementing the new multi-mode drafting system. It is divided into phases, outlining the necessary steps, implementation details, and potential challenges.

---

## Phase 1: Core Data Model & System Architecture

**Goal**: Establish the foundational data structures and management systems for Floors and Ceilings.

### Step 1.1: Create New Plugins
- **Action**: Create two new plugins: `RTPlanFloors` and `RTPlanCeilings`.
- **Reasoning**: This maintains the project's modular architecture. `RTPlanFloors` will contain all logic for floor data, meshing, and tools. `RTPlanCeilings` will do the same for ceilings. This isolates dependencies and keeps the core clean.

### Step 1.2: Define Data Models
- **Action**: Create new `UObject` classes within the new plugins to store element data.

  **In `RTPlanFloors`:**
  ```cpp
  // Represents a single floor slab
  UCLASS()
  class RTPLANFLOORS_API URTPlanFloorItem : public UObject
  {
      GENERATED_BODY()
  public:
      // The 2D polygon defining the floor's boundary
      UPROPERTY()
      TArray<FVector> Boundary;

      // Sub-areas for different materials or properties
      UPROPERTY()
      TArray<URTPlanFloorArea*> MaterialAreas;

      // Extruded regions on this floor
      UPROPERTY()
      TArray<URTPlanFloorExtrusion*> Extrusions;
  };

  // Represents a sub-area for material assignment
  UCLASS()
  class RTPLANFLOORS_API URTPlanFloorArea : public UObject
  {
      GENERATED_BODY()
  public:
      UPROPERTY()
      TArray<FVector> Boundary;

      UPROPERTY()
      UMaterialInterface* AssignedMaterial;
  };

  // Represents an extruded volume from the floor
  UCLASS()
  class RTPLANFLOORS_API URTPlanFloorExtrusion : public UObject
  {
      GENERATED_BODY()
  public:
      UPROPERTY()
      TArray<FVector> Boundary;

      UPROPERTY()
      float ExtrusionHeight;
  };
  ```

  **In `RTPlanCeilings`:**
  - Create analogous classes: `URTPlanCeilingItem`, `URTPlanFalseCeilingSection`.

### Step 1.3: Update Core Document
- **Action**: Modify `URTPlanDocument` in the `RTPlanCore` plugin to store the new data types.
  ```cpp
  // In URTPlanDocument.h
  UPROPERTY()
  TArray<TObjectPtr<URTPlanFloorItem>> FloorItems;

  UPROPERTY()
  TArray<TObjectPtr<URTPlanCeilingItem>> CeilingItems;
  ```
- **Action**: Update the document's serialization and command (Undo/Redo) systems to handle adding, removing, and modifying these new item types.

### Step 1.4: Create Drafting Mode Manager
- **Action**: Create a new class, `UArchVisDraftingModeManager`, likely as a component on the `AArchVisPlayerController`.
- **Implementation**:
  - Define an enum: `enum class EDraftingMode { Walls, Floors, Ceilings };`
  - Implement `SetDraftingMode(EDraftingMode NewMode)` which broadcasts a delegate (`OnDraftingModeChanged`).
  - The manager will be responsible for swapping the tool-related Input Mapping Contexts (IMCs).

---

## Phase 2: Visibility, Rendering, and Camera

**Goal**: Ensure the viewport correctly displays elements based on the active drafting mode.

### Step 2.1: Implement Mode-Based Visibility & Rendering
- **Action**: Create new actors, `ARTPlanFloorActor` and `ARTPlanCeilingActor`, similar to `ARTPlanShellActor`.
- **Implementation**:
  - These actors will subscribe to `URTPlanDocument` changes.
  - They will each contain a `UProceduralMeshComponent` to render the geometry.
  - They will also subscribe to `UArchVisDraftingModeManager::OnDraftingModeChanged`.
  - Based on the new mode, they will change the visibility and/or materials of their rendered elements to one of three states:
    1.  **Editable**: Default, fully rendered.
    2.  **Reference**: Rendered with a "ghost" material (e.g., semi-transparent, single color) and disabled for hit-testing by tools (but not snapping).
    3.  **Hidden**: `SetVisibility(false)`.
- **Pitfall**: Managing multiple `UProceduralMeshComponent` sections for material areas can be complex. Ensure section indices are tracked correctly.

### Step 2.2: Implement Reflected Ceiling Plan (RCP) Camera
- **Action**: Modify `AArchVisDraftingPawn`.
- **Implementation**:
  - When switching to `EDraftingMode::Ceilings`, the pawn's camera should flip its orientation.
  - The simplest approach is to rotate the entire pawn or its camera boom by 180 degrees around the X or Y axis.
  - **Crucially**, pointer input coordinates must be transformed to match this new orientation. The projection from screen-space to world-space will need to be adjusted so that the user's input still feels natural. For example, moving the mouse "up" on the screen should still move the cursor "up" in the plan view, even if the world-space Z is inverted.

---

## Phase 3: Generic "Pen" Tool

**Goal**: Create a reusable base tool for drawing polygonal shapes to avoid code duplication.

### Step 3.1: `URTPlanPenTool` Base Class
- **How it works**: The user clicks to place vertices of a polygon. The tool provides real-time visual feedback of the lines. A final click on the first point, or pressing Enter, closes and confirms the shape. Backspace removes the last point. Escape cancels.
- **Implementation**:
  - Create `URTPlanPenTool` inheriting from `URTPlanTool`.
  - `OnPointerDown`: Adds a `FVector` to a `TArray<FVector> Points`.
  - `OnPointerMove`: Updates a "preview" point for the next segment.
  - `Render (HUD)`: In `AArchVisHUD`, draw lines connecting the `Points` array and a preview line to the current cursor position.
  - `OnConfirm`: A delegate `OnShapeConfirmed(const TArray<FVector>&)` is broadcast.
  - `OnBackspace`: `Points.Pop()`.
  - Integrate with `RTPlanSpatial` to provide snapping feedback.
- **Potential Pitfalls**:
  - Performance of real-time HUD drawing can degrade with very complex polygons.
  - Handling self-intersecting polygons. For now, we can ignore this and rely on the user, but a future enhancement might be to detect and disallow it.

---

## Phase 4: Floor Tools Implementation

**Goal**: Implement the full suite of floor creation and editing tools.

### Step 4.1: `URTPlanDrawFloorTool`
- **How it works**: Uses the Pen Tool to draw a boundary. On confirmation, it generates a triangulated mesh and creates a new `URTPlanFloorItem`.
- **Implementation**:
  - Inherit from `URTPlanPenTool`.
  - Subscribe to `OnShapeConfirmed`.
  - In the handler, take the `TArray<FVector>` boundary.
  - **Triangulation**: Use a robust algorithm like `FPolygonTriangulation::Triangulate` from the `Triangulation` module to convert the 2D polygon into a list of triangles (indices).
  - Create a `UAddFloorCommand` that, when executed, creates a `URTPlanFloorItem`, stores the boundary, and adds it to the `URTPlanDocument`.
  - The `ARTPlanFloorActor` will see the new item and use the boundary and triangulation data to create a new section on its `UProceduralMeshComponent`.
- **Potential Pitfalls**:
  - The `Triangulation` module requires vertices to be in a specific order (e.g., counter-clockwise). The tool must ensure the point order is correct.
  - Generating correct UV coordinates for the floor mesh is essential for materials. A simple planar projection based on the floor's bounding box is a good starting point.

### Step 4.2: `URTPlanNewFloorAreaTool`
- **How it works**: Draws a polygon *inside* an existing floor to define a region for a different material.
- **Implementation**:
  - The tool must first identify the target `URTPlanFloorItem` (e.g., via a click).
  - Use the `URTPlanPenTool` to draw the new area's boundary.
  - On confirmation, the new polygon is stored in a `URTPlanFloorArea` object and added to the parent `URTPlanFloorItem`.
  - The `ARTPlanFloorActor` must then update its rendering. It will need to perform boolean clipping operations (new area - existing areas) to create the correct mesh sections for each material. This is non-trivial. A simpler first-pass approach might be to just render the new area on top, but this is not ideal.
- **Potential Pitfalls**:
  - **Polygon Clipping**: This is the hardest part. You may need to integrate a robust geometry library (like Clipper2) to handle subtracting the new area from the base floor mesh section and creating a new section for the new material.

### Step 4.3: `URTPlanExtrudeFloorTool`
- **How it works**: Draws a 2D shape on the floor, then prompts for a height to create a 3D platform.
- **Implementation**:
  - Use `URTPlanPenTool` to define the 2D base polygon.
  - On confirmation, activate the numeric input overlay to get an extrusion height.
  - The tool will then generate a 3D mesh. This involves:
    1. The base polygon, triangulated.
    2. The base polygon, translated up by `Height`, with reversed winding order for the top cap.
    3. A series of quads connecting the corresponding vertices of the top and bottom polygons to form the side walls.
  - Store this data in a `URTPlanFloorExtrusion` object. The `ARTPlanFloorActor` will render it as a separate mesh.
- **Potential Pitfalls**:
  - Calculating correct normals for the side walls.
  - Generating UVs for the side walls ("wrapping" the texture).

### Step 4.4: Editing Tools (`ExtendFloorArea`, `EditExtrudeFloor`)
- **How it works**: These tools are for modifying existing geometry. They should be vertex/edge based.
- **Implementation**:
  - These will be closer to a `SelectTool`. When a floor area or extrusion is selected, the tool will display interactive handles (gizmos) at each vertex.
  - The user can click and drag a handle.
  - On drag, the tool fires a `UModifyFloorCommand` with the updated vertex data.
  - The `ARTPlanFloorActor` regenerates the mesh in real-time.
- **Potential Pitfalls**:
  - Performance. Full mesh regeneration on every mouse move during a drag can be slow. Consider updating the mesh only on drag end, with a simplified preview during the drag.
  - Undo/Redo for vertex drags needs to be managed carefully to avoid creating excessive numbers of commands.

---

## Phase 5: Ceiling Tools Implementation

**Goal**: Implement the ceiling-specific tools. This phase largely mirrors Phase 4.

- **`URTPlanDrawCeilingTool`**: Implement like `DrawFloorTool`, but for `URTPlanCeilingItem` and using the RCP camera.
- **`URTPlanCreateFalseCeilingTool`**: Implement like `NewFloorAreaTool`, but it will store an offset/height property. The `ARTPlanCeilingActor` will render this as a separate mesh suspended below the main ceiling.
- **`URTPlanEditFalseCeilingTool`**: Implement a vertex-editing tool similar to the floor editing tools.

---

## Phase 6: Input & UI Integration

**Goal**: Connect all new systems and tools to user input and the UI.

### Step 6.1: Create Mode-Specific IMCs
- **Action**: In the editor, create `IMC_FloorTools` and `IMC_CeilingTools`.
- **Implementation**:
  - These IMCs will contain `IA` mappings for the new tools (e.g., `IA_Tool_DrawFloor`, `IA_Tool_Extrude`).
  - The `UArchVisDraftingModeManager` will be responsible for adding and removing these IMCs from the `ULocalPlayer`'s `UEnhancedInputLocalPlayerSubsystem`.

### Step 6.2: Update UI
- **Action**: Modify the main toolbar UMG widget.
- **Implementation**:
  - Add buttons to switch between Wall, Floor, and Ceiling modes. These buttons will call `UArchVisDraftingModeManager::SetDraftingMode`.
  - The toolbar should listen to the `OnDraftingModeChanged` event.
  - The tool buttons themselves should be dynamically shown or hidden based on the active mode. For example, the "Draw Floor" button is only visible when `EDraftingMode` is `Floors`.
