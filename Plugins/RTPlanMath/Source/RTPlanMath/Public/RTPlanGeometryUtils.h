#pragma once

#include "CoreMinimal.h"

/**
 * Geometric utilities for 2D plan operations.
 */
class RTPLANMATH_API FRTPlanGeometryUtils
{
public:
	// Calculate distance from Point P to Segment AB
	static float DistancePointToSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B);

	// Project Point P onto Segment AB
	static FVector2D ProjectPointToSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B);

	// Get the closest point on Segment AB to Point P
	static FVector2D ClosestPointOnSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B);

	// Calculate Left and Right normal vectors for a wall from A to B.
	// Left is +90 degrees (CCW), Right is -90 degrees (CW).
	static void GetWallNormals(const FVector2D& A, const FVector2D& B, FVector2D& OutLeft, FVector2D& OutRight);

	// Check if two segments intersect (A1-B1 and A2-B2)
	static bool SegmentIntersection(const FVector2D& A1, const FVector2D& B1, const FVector2D& A2, const FVector2D& B2, FVector2D& OutIntersection);

	// Check if two segments intersect (simplified, no intersection point returned)
	static bool SegmentsIntersect(const FVector2D& A1, const FVector2D& B1, const FVector2D& A2, const FVector2D& B2);
};
