# RTPlanTools Plugin

## Overview
**RTPlanTools** contains the interactive tool framework and specific drafting tools. It acts as the bridge between user input and the `PlanDocument`.

## Key Functionality
*   **Tool Manager**: `URTPlanToolManager` manages the active tool state and routes unified pointer events to it.
*   **Tool Base**: `URTPlanToolBase` provides common functionality like raycasting against the ground plane and querying the Spatial Index for snapping.
*   **Line Tool**: `URTPlanLineTool` implements the logic for drawing walls (Click-Drag-Release workflow).
*   **Place Tool**: `URTPlanPlaceTool` implements the logic for placing furniture objects from the catalog.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`
*   **Plugins**: `RTPlanCore`, `RTPlanInput`, `RTPlanSpatial`, `RTPlanCatalog`, `RTPlanObjects`
