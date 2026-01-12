# RTPlanOpenings Plugin

## Overview
**RTPlanOpenings** handles the logic for wall openings such as doors and windows. It provides algorithms to determine how walls should be cut.

## Key Functionality
*   **Interval Calculation**: `FRTPlanOpeningUtils::ComputeSolidIntervals` calculates the solid parts of a wall by subtracting the intervals occupied by openings.
*   **Overlap Handling**: Correctly merges overlapping or adjacent openings to ensure valid geometry generation.

## Dependencies
*   **Unreal Modules**: `Core`, `CoreUObject`, `Engine`
*   **Plugins**: `RTPlanCore`
