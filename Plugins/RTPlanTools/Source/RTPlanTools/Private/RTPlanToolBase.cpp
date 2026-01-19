#include "RTPlanToolBase.h"

void URTPlanToolBase::Init(URTPlanDocument* InDoc, FRTPlanSpatialIndex* InSpatialIndex)
{
	Document = InDoc;
	SpatialIndex = InSpatialIndex;
}

void URTPlanToolBase::OnPointerEvent(const FRTPointerEvent& Event)
{
	// Base implementation updates LastSnappedWorldPos for simple tools
	FVector GroundPoint;
	if (GetGroundIntersection(Event, GroundPoint))
	{
		FVector2D Snapped = GetSnappedPoint(GroundPoint);
		LastSnappedWorldPos = FVector(Snapped.X, Snapped.Y, 0.0f);
	}
	else if (Event.Action == ERTPointerAction::Move)
	{
		// If we can't intersect ground (e.g. looking at sky), project far away
		// This prevents cursor from getting stuck at horizon
		LastSnappedWorldPos = Event.WorldOrigin + Event.WorldDirection * 100000.0f;
	}
}

bool URTPlanToolBase::GetGroundIntersection(const FRTPointerEvent& Event, FVector& OutPoint) const
{
	// Intersect Ray with Plane Z=0
	// Plane Normal = (0,0,1), Plane Point = (0,0,0)
	
	if (FMath::IsNearlyZero(Event.WorldDirection.Z))
	{
		return false; // Parallel to ground
	}

	float T = -Event.WorldOrigin.Z / Event.WorldDirection.Z;
	
	if (T < 0)
	{
		return false; // Behind the ray origin
	}

	OutPoint = Event.GetPointAtDistance(T);
	return true;
}

FVector2D URTPlanToolBase::GetSnappedPoint(const FVector& WorldPoint, float SnapRadius) const
{
	FVector2D CursorPos(WorldPoint.X, WorldPoint.Y);

	if (SpatialIndex)
	{
		FRTSnapResult Snap = SpatialIndex->QuerySnap(CursorPos, SnapRadius);
		if (Snap.bValid)
		{
			return Snap.Location;
		}
	}

	// Fallback: Grid Snap (e.g. 10cm)
	// TODO: Make grid size configurable
	float GridSize = 10.0f;
	return FVector2D(
		FMath::RoundToFloat(CursorPos.X / GridSize) * GridSize,
		FMath::RoundToFloat(CursorPos.Y / GridSize) * GridSize
	);
}
