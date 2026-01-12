# RTPlanVR Plugin

## Overview
**RTPlanVR** implements the VR-specific interaction modes and player pawn.

## Key Functionality
*   **VR Pawn**: `ARTPlanVRPawn` handles motion controller input and maps it to the unified `FRTPointerEvent` system.
*   **Mode Manager**: `ARTPlanVRModeManager` handles switching between "Walk" mode (1:1 scale) and "Tabletop" mode (miniature scale for drafting).
*   **Interaction**: Supports widget interaction for VR UI.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`, `HeadMountedDisplay`, `XRBase`
*   **Plugins**: `RTPlanCore`, `RTPlanInput`, `RTPlanUI`
