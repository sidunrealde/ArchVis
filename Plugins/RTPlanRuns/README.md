# RTPlanRuns Plugin

## Overview
**RTPlanRuns** provides procedural generation capabilities for linear runs of cabinetry, wardrobes, or other modular furniture.

## Key Functionality
*   **Run Solver**: `FRTPlanRunSolver` implements a greedy algorithm to fill a specified length with a set of standard module widths (e.g., 60cm, 45cm, 30cm).
*   **Run Manager**: `ARTPlanRunManager` orchestrates the generation process, creating virtual object instances in the `PlanDocument` based on the solver's output.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`
*   **Plugins**: `RTPlanCore`, `RTPlanObjects`
