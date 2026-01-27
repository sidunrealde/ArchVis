#include "RTPlanSpatialIndex.h"
#include "RTPlanGeometryUtils.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

void FRTPlanSpatialIndex::Build(const URTPlanDocument* Document)
{
	SnapPoints.Empty();
	SnapSegments.Empty();
	UniqueX.Empty();
	UniqueY.Empty();
	CachedDocument = Document;

	if (!Document)
	{
		return;
	}

	const FRTPlanData& Data = Document->GetData();
	
	UE_LOG(LogTemp, Verbose, TEXT("SpatialIndex::Build: %d vertices, %d walls"), Data.Vertices.Num(), Data.Walls.Num());

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

			// Handle arc walls differently - add segments along the curve
			if (Wall.bIsArc && FMath::Abs(Wall.ArcSweepAngle) > 0.1f)
			{
				// Calculate arc parameters
				float CenterRadius = FVector2D::Distance(Wall.ArcCenter, A);
				FVector2D ToStart = A - Wall.ArcCenter;
				float StartAngleRad = FMath::Atan2(ToStart.Y, ToStart.X);
				float SweepRad = FMath::DegreesToRadians(Wall.ArcSweepAngle);
				
				// Use enough segments for accurate hit testing (at least 1 per 15 degrees)
				int32 NumSegments = FMath::Max(8, FMath::CeilToInt(FMath::Abs(Wall.ArcSweepAngle) / 15.0f));
				float StepAngle = SweepRad / (float)NumSegments;
				
				// Add midpoint of the arc
				float MidAngle = StartAngleRad + SweepRad * 0.5f;
				FVector2D ArcMid = Wall.ArcCenter + FVector2D(FMath::Cos(MidAngle), FMath::Sin(MidAngle)) * CenterRadius;
				SnapPoints.Add({ ArcMid, TEXT("Arc Midpoint") });
				
				// Add segments along the arc
				for (int32 i = 0; i < NumSegments; ++i)
				{
					float Angle0 = StartAngleRad + StepAngle * i;
					float Angle1 = StartAngleRad + StepAngle * (i + 1);
					
					FVector2D P0 = Wall.ArcCenter + FVector2D(FMath::Cos(Angle0), FMath::Sin(Angle0)) * CenterRadius;
					FVector2D P1 = Wall.ArcCenter + FVector2D(FMath::Cos(Angle1), FMath::Sin(Angle1)) * CenterRadius;
					
					FSnapSegment Seg;
					Seg.A = P0;
					Seg.B = P1;
					Seg.WallId = Wall.Id;
					SnapSegments.Add(Seg);
				}
				
				UE_LOG(LogTemp, Verbose, TEXT("  Arc Wall %s: Center=(%0.1f,%0.1f), Sweep=%0.1f°, %d segments"), 
					*Wall.Id.ToString().Left(8), Wall.ArcCenter.X, Wall.ArcCenter.Y, Wall.ArcSweepAngle, NumSegments);
			}
			else
			{
				// Straight wall - add single segment
				FVector2D Mid = (A + B) * 0.5f;
				SnapPoints.Add({ Mid, TEXT("Midpoint") });
				
				UniqueX.AddUnique(Mid.X);
				UniqueY.AddUnique(Mid.Y);

				FSnapSegment Seg;
				Seg.A = A;
				Seg.B = B;
				Seg.WallId = Wall.Id;
				SnapSegments.Add(Seg);
				
				UE_LOG(LogTemp, Verbose, TEXT("  Wall %s: (%0.1f,%0.1f)->(%0.1f,%0.1f)"), 
					*Wall.Id.ToString().Left(8), A.X, A.Y, B.X, B.Y);
			}
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("SpatialIndex: Built with %d segments"), SnapSegments.Num());
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

FGuid FRTPlanSpatialIndex::HitTestWall(const FVector2D& Point, float Tolerance) const
{
	FGuid BestWallId;
	float BestDistance = Tolerance;

	UE_LOG(LogTemp, Verbose, TEXT("HitTestWall: Point=(%0.1f, %0.1f), Tolerance=%0.1f, NumSegments=%d"),
		Point.X, Point.Y, Tolerance, SnapSegments.Num());

	for (const FSnapSegment& Seg : SnapSegments)
	{
		FVector2D Closest = FRTPlanGeometryUtils::ClosestPointOnSegment(Point, Seg.A, Seg.B);
		float Dist = FVector2D::Distance(Point, Closest);

		UE_LOG(LogTemp, Verbose, TEXT("  Segment %s: (%0.1f,%0.1f)->(%0.1f,%0.1f), Closest=(%0.1f,%0.1f), Dist=%0.1f"),
			*Seg.WallId.ToString().Left(8), Seg.A.X, Seg.A.Y, Seg.B.X, Seg.B.Y, Closest.X, Closest.Y, Dist);

		if (Dist < BestDistance)
		{
			BestDistance = Dist;
			BestWallId = Seg.WallId;
			UE_LOG(LogTemp, Verbose, TEXT("    -> New best!"));
		}
	}

	return BestWallId;
}

TArray<FGuid> FRTPlanSpatialIndex::HitTestWallsInRect(const FVector2D& RectMin, const FVector2D& RectMax) const
{
	TArray<FGuid> HitWalls;

	UE_LOG(LogTemp, Verbose, TEXT("HitTestWallsInRect: Min=(%0.1f, %0.1f), Max=(%0.1f, %0.1f), NumSegments=%d"),
		RectMin.X, RectMin.Y, RectMax.X, RectMax.Y, SnapSegments.Num());

	for (const FSnapSegment& Seg : SnapSegments)
	{
		// Check if either endpoint is inside the rect
		bool bAInside = (Seg.A.X >= RectMin.X && Seg.A.X <= RectMax.X && 
		                 Seg.A.Y >= RectMin.Y && Seg.A.Y <= RectMax.Y);
		bool bBInside = (Seg.B.X >= RectMin.X && Seg.B.X <= RectMax.X && 
		                 Seg.B.Y >= RectMin.Y && Seg.B.Y <= RectMax.Y);

		UE_LOG(LogTemp, Verbose, TEXT("  Segment (%0.1f,%0.1f)->(%0.1f,%0.1f): AInside=%d, BInside=%d"),
			Seg.A.X, Seg.A.Y, Seg.B.X, Seg.B.Y, bAInside, bBInside);

		if (bAInside || bBInside)
		{
			HitWalls.AddUnique(Seg.WallId);
			continue;
		}

		// Check if segment intersects any edge of the rectangle
		// For simplicity, check if the segment crosses any of the 4 rect edges
		FVector2D RectTL(RectMin.X, RectMax.Y);
		FVector2D RectBR(RectMax.X, RectMin.Y);

		// Top edge
		if (FRTPlanGeometryUtils::SegmentsIntersect(Seg.A, Seg.B, RectMin, RectTL) ||
		    // Left edge
		    FRTPlanGeometryUtils::SegmentsIntersect(Seg.A, Seg.B, RectTL, RectMax) ||
		    // Bottom edge
		    FRTPlanGeometryUtils::SegmentsIntersect(Seg.A, Seg.B, RectMax, RectBR) ||
		    // Right edge
		    FRTPlanGeometryUtils::SegmentsIntersect(Seg.A, Seg.B, RectBR, RectMin))
		{
			HitWalls.AddUnique(Seg.WallId);
		}
	}

	return HitWalls;
}

FGuid FRTPlanSpatialIndex::HitTestOpening(const FVector2D& Point, float Tolerance) const
{
	if (!CachedDocument) return FGuid();

	const FRTPlanData& Data = CachedDocument->GetData();
	FGuid BestOpeningId;
	float BestDistance = Tolerance;

	for (const auto& Pair : Data.Openings)
	{
		const FRTOpening& Opening = Pair.Value;
		
		// Get the wall this opening is on
		if (!Data.Walls.Contains(Opening.WallId)) continue;
		const FRTWall& Wall = Data.Walls[Opening.WallId];
		
		if (!Data.Vertices.Contains(Wall.VertexAId) || !Data.Vertices.Contains(Wall.VertexBId)) continue;
		
		FVector2D WallA = Data.Vertices[Wall.VertexAId].Position;
		FVector2D WallB = Data.Vertices[Wall.VertexBId].Position;
		FVector2D WallDir = (WallB - WallA).GetSafeNormal();
		
		// Opening center position along the wall (OffsetCm is distance from VertexA)
		FVector2D OpeningCenter = WallA + WallDir * Opening.OffsetCm;
		
		float Dist = FVector2D::Distance(Point, OpeningCenter);
		if (Dist < BestDistance)
		{
			BestDistance = Dist;
			BestOpeningId = Opening.Id;
		}
	}

	return BestOpeningId;
}

TArray<FGuid> FRTPlanSpatialIndex::HitTestOpeningsInRect(const FVector2D& RectMin, const FVector2D& RectMax) const
{
	TArray<FGuid> HitOpenings;
	
	if (!CachedDocument) return HitOpenings;

	const FRTPlanData& Data = CachedDocument->GetData();

	for (const auto& Pair : Data.Openings)
	{
		const FRTOpening& Opening = Pair.Value;
		
		// Get the wall this opening is on
		if (!Data.Walls.Contains(Opening.WallId)) continue;
		const FRTWall& Wall = Data.Walls[Opening.WallId];
		
		if (!Data.Vertices.Contains(Wall.VertexAId) || !Data.Vertices.Contains(Wall.VertexBId)) continue;
		
		FVector2D WallA = Data.Vertices[Wall.VertexAId].Position;
		FVector2D WallB = Data.Vertices[Wall.VertexBId].Position;
		FVector2D WallDir = (WallB - WallA).GetSafeNormal();
		
		// Opening center position along the wall (OffsetCm is distance from VertexA)
		FVector2D OpeningCenter = WallA + WallDir * Opening.OffsetCm;
		
		// Check if center is inside rect
		if (OpeningCenter.X >= RectMin.X && OpeningCenter.X <= RectMax.X &&
		    OpeningCenter.Y >= RectMin.Y && OpeningCenter.Y <= RectMax.Y)
		{
			HitOpenings.Add(Opening.Id);
		}
	}

	return HitOpenings;
}

void FRTPlanSpatialIndex::DrawDebugSegments(UWorld* World, float Duration) const
{
	if (!World) return;
	
	for (const FSnapSegment& Seg : SnapSegments)
	{
		FVector Start(Seg.A.X, Seg.A.Y, 5.0f);  // Slightly above ground
		FVector End(Seg.B.X, Seg.B.Y, 5.0f);
		
		DrawDebugLine(World, Start, End, FColor::Cyan, false, Duration, 0, 3.0f);
		DrawDebugSphere(World, Start, 10.0f, 6, FColor::Magenta, false, Duration);
		DrawDebugSphere(World, End, 10.0f, 6, FColor::Magenta, false, Duration);
	}
	
	UE_LOG(LogTemp, Log, TEXT("DrawDebugSegments: Drew %d segments"), SnapSegments.Num());
}
