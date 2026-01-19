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

	// --- Hit Testing for Selection ---
	
	// Hit test a single point against walls, returns the closest wall ID within tolerance
	// Returns invalid GUID if no wall was hit
	FGuid HitTestWall(const FVector2D& Point, float Tolerance = 10.0f) const;

	// Hit test a rectangle against walls, returns all wall IDs that intersect
	TArray<FGuid> HitTestWallsInRect(const FVector2D& RectMin, const FVector2D& RectMax) const;

	// Hit test a single point against openings, returns the closest opening ID within tolerance
	FGuid HitTestOpening(const FVector2D& Point, float Tolerance = 10.0f) const;

	// Hit test a rectangle against openings, returns all opening IDs that intersect
	TArray<FGuid> HitTestOpeningsInRect(const FVector2D& RectMin, const FVector2D& RectMax) const;

	// Debug: Draw all segments in the spatial index
	void DrawDebugSegments(UWorld* World, float Duration = 5.0f) const;

	// Get the number of segments in the index
	int32 GetNumSegments() const { return SnapSegments.Num(); }

private:
	// Cache of snap points (Endpoints, Midpoints)
	struct FSnapPoint
	{
		FVector2D Position;
		FString Type;
	};

	TArray<FSnapPoint> SnapPoints;

	// Cache of segments for projection snapping and hit testing
	struct FSnapSegment
	{
		FVector2D A;
		FVector2D B;
		FGuid WallId;  // The wall this segment belongs to
	};

	TArray<FSnapSegment> SnapSegments;

	// Unique coordinates for alignment guides
	TArray<float> UniqueX;
	TArray<float> UniqueY;

	// Cached document reference for hit testing
	const URTPlanDocument* CachedDocument = nullptr;
};
