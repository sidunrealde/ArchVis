# Runtime Interior Planner - Refined Implementation Plan

> **Goal**: A CAD-like drafting experience for runtime architectural planning in Unreal Engine, supporting seamless 2D/3D switching, precision input, and auto-generation of shell geometry.

---

## 1. Core Drafting Experience (2D Ortho)

### 1.1 Viewport & Camera
*   **2D Mode**: Top-down Orthographic camera. Fixed rotation (looking down Z). Pan/Zoom controls.
*   **3D Mode**: Perspective camera (Orbit or Fly).
*   **Toggle**: A UI button to switch between `AArchVisPlayerController` states or Camera components.

### 1.2 Input & Cursor
*   **Crosshair Cursor**: Custom software cursor (UMG or HUD draw) extending full screen width/height (AutoCAD style).
*   **Numeric Input**:
    *   While drawing, typing numbers (e.g., "300") immediately updates the length of the segment being drawn.
    *   Pressing `Enter` commits the segment at that length.
    *   Pressing `Tab` toggles between Length and Angle input fields.

### 1.3 Wall Drawing Workflow
*   **Interaction**: **Click-Move-Click** (not Drag).
    1.  Click to set Start Point.
    2.  Move mouse to preview End Point.
    3.  Click (or type length + Enter) to set End Point.
    4.  Tool automatically starts the next wall from the previous End Point (Polyline behavior).
    5.  Right-Click or Esc to finish the chain.
*   **Ortho Lock (Axis Snapping)**:
    *   By default, walls snap to X or Y axis (0, 90, 180, 270 degrees).
    *   Modifier Key (`Shift` or `Alt`) disables this snap for free-angle drawing.

### 1.4 Snapping System
*   **Priorities**:
    1.  **Endpoints** (Vertices of existing walls).
    2.  **Perpendicular/Parallel** alignment (smart guides).
    3.  **Grid**.

---

## 2. Architectural Elements

### 2.1 Walls & Variants
*   **Standard Wall**: Full height, blocks visibility.
*   **Half Wall**: Lower height (e.g., 100cm), distinct visual style in 2D.
*   **Balcony Railing**: Thin profile, specific material/mesh.
*   **Properties**:
    *   Selection allows editing `Thickness` and `Height` in the Property Inspector.
    *   Changes update the 3D mesh immediately.

### 2.2 Openings (Doors & Windows)
*   **2D Visualization**:
    *   Doors: Arc swing visualization.
    *   Windows: Simple rectangle/line representation on the wall.
*   **Placement**:
    *   Hovering a wall snaps the opening to it.
    *   Click to place.
    *   Properties: Width, Height, Sill Height (for windows).

### 2.3 Room Generation (Floors & Ceilings)
*   **Auto-Detection**:
    *   When a wall chain forms a closed loop, the system detects a "Room".
    *   Algorithm: Graph cycle detection or flood fill from a seed point.
*   **Generation**:
    *   **Floor**: Generated mesh matching the room polygon.
    *   **Ceiling**: Generated mesh at wall height matching the room polygon.
*   **Materials**:
    *   Each room is a distinct entity (`FRTRoom`).
    *   Users can select a room (click floor) to change its Floor Material or Ceiling Material independently.

---

## 3. UI & Tools

### 3.1 Main Toolbar
*   **Draw Tools**: Wall, Half Wall, Railing.
*   **Opening Tools**: Door, Window.
*   **Edit Tools**: Select, Move, Rotate.
*   **View Toggle**: 2D / 3D.

### 3.2 Property Inspector
*   Context-sensitive panel.
*   **Wall Selected**: Thickness, Height, Type.
*   **Opening Selected**: Width, Height, Type, Flip Swing.
*   **Room Selected**: Floor Material, Ceiling Material.

### 3.3 Undo/Redo
*   Global command stack supporting all operations (Draw, Edit, Delete, Property Change).

---

## 4. Implementation Roadmap (Refined)

### Phase 1: Viewport & Input (The "CAD Feel")
1.  **Camera Controller**: Implement 2D/3D switching logic.
2.  **HUD**: Draw the full-screen crosshair cursor.
3.  **Input Handling**: Update `RTPlanInput` to support numeric keystrokes and capture them into a buffer.

### Phase 2: Wall Tool 2.0 (Click-Move-Click)
1.  **Refactor Line Tool**: Change from Drag to State Machine (Waiting -> Start Set -> End Set).
2.  **Ortho Locking**: Implement vector math to snap `CurrentEndPoint` to X/Y axes relative to `StartPoint`.
3.  **Numeric Entry**: Hook up the input buffer to drive `CurrentEndPoint` distance.

### Phase 3: 2D Visualization
1.  **Canvas/SceneProxy**: Implement a lightweight 2D renderer for the plan view (lines, arcs for doors).
2.  **Overlay**: Render dimensions (text) near the wall being drawn.

### Phase 4: Room Detection & Generation
1.  **Graph Logic**: Implement a graph structure in `RTPlanCore` to track connected vertices.
2.  **Cycle Detection**: Algorithm to find closed loops.
3.  **Mesh Builder**: Update `RTPlanMeshing` to triangulate these loops for floors/ceilings.

### Phase 5: Properties & Editing
1.  **Selection System**: Raycast to find Walls/Rooms.
2.  **Property UI**: Bind UMG widgets to the selected object's data.
3.  **Command Updates**: Ensure property changes emit `URTCmdUpdateWall` etc.

---

## 5. Technical Notes

*   **Coordinate System**: Unreal Z-up. 2D Plan is XY plane.
*   **Units**: Centimeters (cm).
*   **Persistence**: JSON serialization handles the new Room and Property data fields automatically if added to the Schema.
