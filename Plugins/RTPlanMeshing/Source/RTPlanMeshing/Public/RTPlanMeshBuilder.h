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
		int32 MaterialID_Left,
		int32 MaterialID_Right,
		int32 MaterialID_Caps,
		int32 MaterialID_Skirting_Left,
		int32 MaterialID_Skirting_Right,
		int32 MaterialID_Skirting_Cap
	);

	// Generate a curved wall mesh (arc wall) with skirting
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
		int32 MaterialID_Left,
		int32 MaterialID_Right,
		int32 MaterialID_Caps,
		int32 MaterialID_Skirting_Left,
		int32 MaterialID_Skirting_Right,
		int32 MaterialID_Skirting_Cap
	);

	// Generate a floor mesh from a polygon loop
	static void AppendFloorMesh(
		UDynamicMesh* TargetMesh,
		const TArray<FVector2D>& Polygon,
		float ZHeight,
		int32 MaterialID
	);
};
