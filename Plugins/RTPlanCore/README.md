# RTPlanCore Plugin

## Overview
**RTPlanCore** is the foundational plugin for the Runtime Interior Planner. It defines the canonical data model (`PlanDocument`) and the Command pattern infrastructure used to modify that data. It is designed to be the single source of truth for the application state.

## Key Functionality
*   **Plan Schema**: Defines `FRTPlanData`, `FRTVertex`, `FRTWall`, `FRTOpening`, `FRTInteriorInstance`, and `FRTCabinetRun`.
*   **Stable IDs**: Uses `FGuid` to ensure objects can be reliably referenced across network sessions and save/load cycles.
*   **Plan Document**: `URTPlanDocument` acts as the container for the data, managing serialization (JSON) and the dirty state.
*   **Command System**: `URTCommand` base class and concrete implementations (`URTCmdAddWall`, `URTCmdAddVertex`, etc.) support a robust Undo/Redo history.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`, `Json`, `JsonUtilities`
*   **Plugins**: None
