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
	// Generate a simple box mesh for a wall
	static void AppendWallMesh(
		UDynamicMesh* TargetMesh,
		const FTransform& Transform,
		float Length,
		float Thickness,
		float Height,
		int32 MaterialID_Left,
		int32 MaterialID_Right,
		int32 MaterialID_Caps
	);

	// Generate a curved wall mesh (arc wall)
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
		int32 MaterialID_Left,
		int32 MaterialID_Right,
		int32 MaterialID_Caps
	);

	// Generate a floor mesh from a polygon loop
	static void AppendFloorMesh(
		UDynamicMesh* TargetMesh,
		const TArray<FVector2D>& Polygon,
		float ZHeight,
		int32 MaterialID
	);
};
