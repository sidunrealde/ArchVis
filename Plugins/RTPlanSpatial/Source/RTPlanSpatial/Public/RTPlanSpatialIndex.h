#pragma once

#include "CoreMinimal.h"
#include "RTPlanSchema.h"
#include "RTPlanDocument.h"

/**
 * Result of a snap query.
 */
struct FRTSnapResult
{
	bool bValid = false;
	FVector2D Location = FVector2D::ZeroVector;
	float Distance = FLT_MAX;
	FString DebugType; // "Endpoint", "Midpoint", "Grid", "Alignment"
};

/**
 * Spatial Index for fast queries and snapping.
 * Rebuilt when PlanDocument changes.
 */
class RTPLANSPATIAL_API FRTPlanSpatialIndex
{
public:
	void Build(const URTPlanDocument* Document);

	// Find the best snap point near CursorPos within Radius.
	FRTSnapResult QuerySnap(const FVector2D& CursorPos, float Radius) const;

	// Find alignment guides (matching X or Y coordinates of existing vertices)
	// Returns a modified position that aligns with existing geometry if within Radius.
	bool QueryAlignment(const FVector2D& CursorPos, float Radius, FVector2D& OutAlignedPos) const;

private:
	// Cache of snap points (Endpoints, Midpoints)
	struct FSnapPoint
	{
		FVector2D Position;
		FString Type;
	};

	TArray<FSnapPoint> SnapPoints;

	// Cache of segments for projection snapping
	struct FSnapSegment
	{
		FVector2D A;
		FVector2D B;
	};

	TArray<FSnapSegment> SnapSegments;

	// Unique coordinates for alignment guides
	TArray<float> UniqueX;
	TArray<float> UniqueY;
};
