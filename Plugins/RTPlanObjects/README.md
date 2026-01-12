# RTPlanObjects Plugin

## Overview
**RTPlanObjects** manages the lifecycle of 3D interior objects (furniture, appliances) in the scene.

## Key Functionality
*   **Object Manager**: `ARTPlanObjectManager` listens to the `PlanDocument` and ensures that for every object instance in the data, there is a corresponding `AStaticMeshActor` in the world.
*   **Synchronization**: Handles spawning new objects, updating transforms of moved objects, and destroying removed objects.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`
*   **Plugins**: `RTPlanCore`, `RTPlanCatalog`
