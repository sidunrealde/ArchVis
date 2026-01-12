# RTPlanMeshing Plugin

## Overview
**RTPlanMeshing** provides the recipes for generating 3D geometry from the 2D plan data. It leverages Unreal's Geometry Scripting plugin for runtime mesh generation.

## Key Functionality
*   **Mesh Builder**: `FRTPlanMeshBuilder` contains static functions to append geometry to a `UDynamicMesh`.
*   **Wall Generation**: `AppendWallMesh` creates a box mesh with correct dimensions and transforms for a wall segment.
*   **Floor Generation**: `AppendFloorMesh` (placeholder) for generating floor geometry from room loops.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`, `GeometryFramework`, `GeometryScriptingCore`
*   **Plugins**: `RTPlanCore`
