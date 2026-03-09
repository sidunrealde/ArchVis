#pragma once

#include "CoreMinimal.h"
#include "GeometryScript/GeometryScriptTypes.h"
#include "UDynamicMesh.h" 
#include "RTPlanSchema.h"

class UDynamicMesh;

/**
 * Helper class to generate Dynamic Meshes from Plan Data.
 * Uses Geometry Scripting Core functions.
 */
class RTPLANMESHING_API FRTPlanMeshBuilder
{
public:
	// Generate a wall mesh with skirting and different materials
	// Material slots:
	//   0 = Left wall face
	//   1 = Right wall face
	//   2 = Top face
	//   3 = Left cap (start/end)
	//   4 = Right cap (start/end)
	//   5 = Left skirting face
	//   6 = Left skirting top
	//   7 = Left skirting cap
	//   8 = Right skirting face
	//   9 = Right skirting top
	//  10 = Right skirting cap
	static void AppendWallMesh(
		UDynamicMesh* TargetMesh,
		const FTransform& Transform,
		float Length,
		float Thickness,
		float Height,
		float BaseZ,
		float SkirtingHeight_Left,
		float SkirtingThickness_Left,
		float SkirtingHeight_Right,
		float SkirtingThickness_Right,
		float SkirtingHeight_Cap,
		float SkirtingThickness_Cap,
		int32 MaterialID_Left,           // 0
		int32 MaterialID_Right,          // 1
		int32 MaterialID_Top,            // 2
		int32 MaterialID_LeftCap,        // 3
		int32 MaterialID_RightCap,       // 4
		int32 MaterialID_SkirtingLeft,   // 5
		int32 MaterialID_SkirtingLeftTop,// 6
		int32 MaterialID_SkirtingLeftCap,// 7
		int32 MaterialID_SkirtingRight,  // 8
		int32 MaterialID_SkirtingRightTop,// 9
		int32 MaterialID_SkirtingRightCap // 10
	);

	// Generate a curved wall mesh (arc wall) with skirting
	// Uses same material slot convention as AppendWallMesh
	static void AppendCurvedWallMesh(
		UDynamicMesh* TargetMesh,
		const FVector2D& StartPoint,
		const FVector2D& EndPoint,
		const FVector2D& ArcCenter,
		float SweepAngleDeg,
		float Thickness,
		float Height,
		float BaseZ,
		int32 NumSegments,
		float SkirtingHeight_Left,
		float SkirtingThickness_Left,
		float SkirtingHeight_Right,
		float SkirtingThickness_Right,
		float SkirtingHeight_Cap,
		float SkirtingThickness_Cap,
		int32 MaterialID_Left,           // 0
		int32 MaterialID_Right,          // 1
		int32 MaterialID_Top,            // 2
		int32 MaterialID_LeftCap,        // 3
		int32 MaterialID_RightCap,       // 4
		int32 MaterialID_SkirtingLeft,   // 5
		int32 MaterialID_SkirtingLeftTop,// 6
		int32 MaterialID_SkirtingLeftCap,// 7
		int32 MaterialID_SkirtingRight,  // 8
		int32 MaterialID_SkirtingRightTop,// 9
		int32 MaterialID_SkirtingRightCap // 10
	);

	// Generate a floor mesh from a polygon loop
	static void AppendFloorMesh(
		UDynamicMesh* TargetMesh,
		const TArray<FVector2D>& Polygon,
		float ZHeight,
		int32 MaterialID
	);
};
