# RTPlanInput Plugin

## Overview
**RTPlanInput** provides a unified input abstraction layer, allowing tools to handle events from both Desktop (Mouse) and VR (Motion Controllers) sources identically.

## Key Functionality
*   **Unified Pointer Event**: `FRTPointerEvent` struct encapsulates the ray origin, direction, and action state (Down, Up, Move).
*   **Input Sources**: Enum `ERTInputSource` distinguishes between Mouse, Touch, and VR controllers.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`, `InputCore`, `EnhancedInput`
*   **Plugins**: `RTPlanCore`
