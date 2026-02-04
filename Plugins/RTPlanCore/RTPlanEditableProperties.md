# RTPlan Editable Properties

This document outlines the editable properties for various architectural elements within the RTPlan system. These properties are generally defined in `RTPlanSchema.h`.

## Walls (Linear & Arc)

These properties are part of the `FRTWall` struct.

### General Properties

| Property Name | Description | Type | Default Value |
|---|---|---|---|
| `ThicknessCm` | The thickness of the wall in centimeters. | `float` | `20.0` |
| `HeightCm` | The height of the wall in centimeters. | `float` | `300.0` |
| `BaseZCm` | The starting height (Z-offset) of the wall from the ground plane in centimeters. | `float` | `0.0` |

### Arc-Specific Properties

These properties are only used if `bIsArc` is `true`.

| Property Name | Description | Type | Default Value |
|---|---|---|---|
| `bIsArc` | If true, the wall is generated as a curve instead of a straight line. | `bool` | `false` |
| `ArcCenter` | The 2D center point of the arc. | `FVector2D` | `(0,0)` |
| `ArcSweepAngle` | The angle of the arc in degrees. Positive values are counter-clockwise (CCW), negative values are clockwise (CW). | `float` | `0.0` |
| `ArcNumSegments` | The number of segments used to approximate the curve. If 0, it's calculated automatically. | `int32` | `0` |

### Skirting Properties

Skirting (or baseboards) are generated at the bottom of the wall. "Left" and "Right" are relative to the wall's direction (from Vertex A to Vertex B).

| Property Name | Description | Type | Default Value |
|---|---|---|---|
| `bHasLeftSkirting` | Enables skirting on the left side of the wall. | `bool` | `true` |
| `LeftSkirtingHeightCm` | The height of the left skirting in centimeters. | `float` | `10.0` |
| `LeftSkirtingThicknessCm` | How far the left skirting extends from the wall surface in centimeters. | `float` | `1.5` |
| `bHasRightSkirting` | Enables skirting on the right side of the wall. | `bool` | `true` |
| `RightSkirtingHeightCm` | The height of the right skirting in centimeters. | `float` | `10.0` |
| `RightSkirtingThicknessCm` | How far the right skirting extends from the wall surface in centimeters. | `float` | `1.5` |

### Material / Finish Properties

These properties reference material definitions from a catalog. Each face of the wall and skirting has its own designated material - there are no fallbacks or shared defaults.

#### Wall Face Materials

| Property Name | Description | Type | Default Value |
|---|---|---|---|
| `FinishLeftId` | The material ID for the left face of the wall. | `FName` | `None` |
| `FinishRightId` | The material ID for the right face of the wall. | `FName` | `None` |
| `FinishTopId` | The material ID for the top face of the wall. | `FName` | `None` |
| `FinishLeftCapId` | The material ID for the left/start cap face of the wall. | `FName` | `None` |
| `FinishRightCapId` | The material ID for the right/end cap face of the wall. | `FName` | `None` |

#### Left Skirting Materials

| Property Name | Description | Type | Default Value |
|---|---|---|---|
| `FinishLeftSkirtingId` | The material ID for the left skirting front face. | `FName` | `None` |
| `FinishLeftSkirtingTopId` | The material ID for the left skirting top face. | `FName` | `None` |
| `FinishLeftSkirtingCapId` | The material ID for the left skirting cap faces (start/end). | `FName` | `None` |

#### Right Skirting Materials

| Property Name | Description | Type | Default Value |
|---|---|---|---|
| `FinishRightSkirtingId` | The material ID for the right skirting front face. | `FName` | `None` |
| `FinishRightSkirtingTopId` | The material ID for the right skirting top face. | `FName` | `None` |
| `FinishRightSkirtingCapId` | The material ID for the right skirting cap faces (start/end). | `FName` | `None` |

---

## Openings (Doors, Windows, etc.)

These properties are part of the `FRTOpening` struct and are hosted on a wall.

| Property Name | Description | Type | Default Value |
|---|---|---|---|
| `Type` | The type of opening (Door, Window, or just a generic Opening). | `ERTOpeningType` | `Door` |
| `WallId` | The ID of the wall that hosts this opening. | `FGuid` | `None` |
| `OffsetCm` | The distance from the wall's start point (Vertex A) to the start of the opening. | `float` | `0.0` |
| `WidthCm` | The width of the opening in centimeters. | `float` | `90.0` |
| `HeightCm` | The height of the opening in centimeters. | `float` | `210.0` |
| `SillHeightCm` | The distance from the bottom of the wall to the bottom of the opening (e.g., for windows). | `float` | `0.0` |
| `ProductTypeId` | An asset reference ID from a catalog for the specific door/window model to be placed. | `FName` | `None` |
| `bFlip` | A boolean to flip the orientation of the opening (e.g., door swing direction). | `bool` | `false` |

---

## Floors (Future)

*Properties for floor elements will be documented here.*

---

## Ceilings (Future)

*Properties for ceiling elements will be documented here.*
