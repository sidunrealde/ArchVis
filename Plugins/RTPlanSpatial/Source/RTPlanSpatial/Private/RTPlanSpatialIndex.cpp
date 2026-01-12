#include "RTPlanSpatialIndex.h"
#include "RTPlanGeometryUtils.h"

void FRTPlanSpatialIndex::Build(const URTPlanDocument* Document)
{
	SnapPoints.Empty();
	SnapSegments.Empty();
	UniqueX.Empty();
	UniqueY.Empty();

	if (!Document) return;

	const FRTPlanData& Data = Document->GetData();

	// Collect Vertices (Endpoints)
	for (const auto& Pair : Data.Vertices)
	{
		FVector2D Pos = Pair.Value.Position;
		SnapPoints.Add({ Pos, TEXT("Endpoint") });
		
		UniqueX.AddUnique(Pos.X);
		UniqueY.AddUnique(Pos.Y);
	}

	// Collect Wall Midpoints and Segments
	for (const auto& Pair : Data.Walls)
	{
		const FRTWall& Wall = Pair.Value;
		if (Data.Vertices.Contains(Wall.VertexAId) && Data.Vertices.Contains(Wall.VertexBId))
		{
			FVector2D A = Data.Vertices[Wall.VertexAId].Position;
			FVector2D B = Data.Vertices[Wall.VertexBId].Position;

			// Midpoint
			FVector2D Mid = (A + B) * 0.5f;
			SnapPoints.Add({ Mid, TEXT("Midpoint") });
			
			UniqueX.AddUnique(Mid.X);
			UniqueY.AddUnique(Mid.Y);

			// Segment
			SnapSegments.Add({ A, B });
		}
	}
}

FRTSnapResult FRTPlanSpatialIndex::QuerySnap(const FVector2D& CursorPos, float Radius) const
{
	FRTSnapResult BestResult;
	BestResult.Distance = Radius; // Initial threshold

	// 1. Check Points (Endpoints, Midpoints) - High Priority
	for (const FSnapPoint& Pt : SnapPoints)
	{
		float Dist = FVector2D::Distance(CursorPos, Pt.Position);
		if (Dist < BestResult.Distance)
		{
			BestResult.bValid = true;
			BestResult.Distance = Dist;
			BestResult.Location = Pt.Position;
			BestResult.DebugType = Pt.Type;
		}
	}

	// If we found a point snap, return it (priority over edges)
	if (BestResult.bValid)
	{
		return BestResult;
	}

	// 2. Check Segments (Projection) - Lower Priority
	for (const FSnapSegment& Seg : SnapSegments)
	{
		FVector2D Projected = FRTPlanGeometryUtils::ClosestPointOnSegment(CursorPos, Seg.A, Seg.B);
		float Dist = FVector2D::Distance(CursorPos, Projected);

		if (Dist < BestResult.Distance)
		{
			BestResult.bValid = true;
			BestResult.Distance = Dist;
			BestResult.Location = Projected;
			BestResult.DebugType = TEXT("Projection");
		}
	}

	return BestResult;
}

bool FRTPlanSpatialIndex::QueryAlignment(const FVector2D& CursorPos, float Radius, FVector2D& OutAlignedPos) const
{
	OutAlignedPos = CursorPos;
	bool bAligned = false;

	// Check X Alignment (Vertical Guide)
	float BestDistX = Radius;
	for (float X : UniqueX)
	{
		float Dist = FMath::Abs(CursorPos.X - X);
		if (Dist < BestDistX)
		{
			OutAlignedPos.X = X;
			BestDistX = Dist;
			bAligned = true;
		}
	}

	// Check Y Alignment (Horizontal Guide)
	float BestDistY = Radius;
	for (float Y : UniqueY)
	{
		float Dist = FMath::Abs(CursorPos.Y - Y);
		if (Dist < BestDistY)
		{
			OutAlignedPos.Y = Y;
			BestDistY = Dist;
			bAligned = true;
		}
	}

	return bAligned;
}
