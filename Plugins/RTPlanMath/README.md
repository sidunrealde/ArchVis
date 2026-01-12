# RTPlanMath Plugin

## Overview
**RTPlanMath** provides pure mathematical and geometric utility functions tailored for 2D architectural planning. It abstracts complex vector math into reusable helper functions.

## Key Functionality
*   **Geometry Utils**: `FRTPlanGeometryUtils` class providing:
    *   `DistancePointToSegment`: Calculates the shortest distance from a point to a line segment.
    *   `ProjectPointToSegment`: Projects a point onto a line segment.
    *   `GetWallNormals`: Computes the Left and Right normal vectors for a wall segment (used for thickness and offset calculations).
    *   `SegmentIntersection`: Checks if two line segments intersect.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`
*   **Plugins**: `RTPlanCore`
