# RTPlanSpatial Plugin

## Overview
**RTPlanSpatial** implements a spatial indexing system to accelerate geometric queries, primarily for snapping during drafting.

## Key Functionality
*   **Spatial Index**: `FRTPlanSpatialIndex` builds a transient cache of geometric entities (Endpoints, Midpoints, Edges) from the `PlanDocument`.
*   **Snapping Engine**: `QuerySnap` function finds the best snap candidate for a given cursor position and radius, prioritizing points over edges.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`
*   **Plugins**: `RTPlanCore`, `RTPlanMath`
