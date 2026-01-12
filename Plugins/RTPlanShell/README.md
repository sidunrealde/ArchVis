# RTPlanShell Plugin

## Overview
**RTPlanShell** is responsible for rendering the architectural shell (Walls, Floors, Ceilings) in the game world. It listens to plan updates and rebuilds the 3D representation.

## Key Functionality
*   **Shell Actor**: `ARTPlanShellActor` is the main actor that renders the plan. It subscribes to `OnPlanChanged` events.
*   **Dynamic Updates**: Rebuilds the Dynamic Mesh Component whenever the plan data changes.
*   **Opening Integration**: Uses `RTPlanOpenings` to split walls into solid segments where doors and windows are placed.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`, `GeometryFramework`, `GeometryScriptingCore`
*   **Plugins**: `RTPlanCore`, `RTPlanMeshing`, `RTPlanMath`, `RTPlanOpenings`
