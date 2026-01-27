#include "Tools/RTPlanTrimTool.h"
#include "RTPlanSpatialIndex.h"
#include "RTPlanGeometryUtils.h"
#include "RTPlanCommand.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"

DEFINE_LOG_CATEGORY(LogRTPlanTrimTool);

void URTPlanTrimTool::OnEnter()
{
	State = EState::Idle;
	bIsMarqueeDragging = false;
	UE_LOG(LogRTPlanTrimTool, Log, TEXT("Trim Tool Entered"));
}

void URTPlanTrimTool::OnExit()
{
	State = EState::Idle;
	bIsMarqueeDragging = false;
	UE_LOG(LogRTPlanTrimTool, Log, TEXT("Trim Tool Exited"));
}

void URTPlanTrimTool::OnPointerEvent(const FRTPointerEvent& Event)
{
	FVector WorldPos3D;
	FVector2D WorldPos2D;
	bool bHasWorldPos = GetWorldPosition(Event, WorldPos3D, WorldPos2D);

	switch (State)
	{
	case EState::Idle:
		if (Event.Action == ERTPointerAction::Down)
		{
			// Start potential drag
			State = EState::PotentialDrag;
			MouseDownScreenPos = Event.ScreenPosition;
			
			if (bHasWorldPos)
			{
				MarqueeStartWorld = WorldPos2D;
				MarqueeEndWorld = WorldPos2D;
			}
		}
		else if (Event.Action == ERTPointerAction::Move)
		{
			if (bHasWorldPos)
			{
				LastSnappedWorldPos = WorldPos3D;
			}
			else
			{
				LastSnappedWorldPos = Event.WorldOrigin + Event.WorldDirection * 100000.0f;
			}
		}
		break;

	case EState::PotentialDrag:
		if (Event.Action == ERTPointerAction::Move)
		{
			// Check if we've moved enough to start a marquee drag
			float DragDistance = FVector2D::Distance(MouseDownScreenPos, Event.ScreenPosition);
			
			if (DragDistance > DragThreshold)
			{
				// Start marquee drag
				State = EState::MarqueeDragging;
				bIsMarqueeDragging = true;
			}
			
			if (bHasWorldPos)
			{
				MarqueeEndWorld = WorldPos2D;
				LastSnappedWorldPos = WorldPos3D;
			}
		}
		else if (Event.Action == ERTPointerAction::Up)
		{
			// This was a click, not a drag
			if (bHasWorldPos)
			{
				PerformClickTrim(WorldPos2D);
			}
			State = EState::Idle;
		}
		break;

	case EState::MarqueeDragging:
		if (Event.Action == ERTPointerAction::Move)
		{
			if (bHasWorldPos)
			{
				MarqueeEndWorld = WorldPos2D;
				LastSnappedWorldPos = WorldPos3D;
			}
			
			// Always draw marquee visualization during drag
			if (GWorld)
			{
				DrawMarqueeVisualization(GWorld);
			}
		}
		else if (Event.Action == ERTPointerAction::Up)
		{
			// Complete marquee trim
			PerformMarqueeTrim();
			
			State = EState::Idle;
			bIsMarqueeDragging = false;
		}
		else if (Event.Action == ERTPointerAction::Cancel)
		{
			// Cancel marquee
			State = EState::Idle;
			bIsMarqueeDragging = false;
		}
		break;
	}
}

void URTPlanTrimTool::DrawMarqueeVisualization(UWorld* World) const
{
	if (!World || !bIsMarqueeDragging) return;
	
	// Draw a line for fence trim
	FColor TrimColor = FColor::Red;
	float LineThickness = 2.0f;
	float Duration = 0.0f; // Single frame
	
	DrawDebugLine(World, FVector(MarqueeStartWorld.X, MarqueeStartWorld.Y, 10.0f), 
		FVector(MarqueeEndWorld.X, MarqueeEndWorld.Y, 10.0f), TrimColor, false, Duration, 0, LineThickness);
		
	// Draw endpoints
	DrawDebugSphere(World, FVector(MarqueeStartWorld.X, MarqueeStartWorld.Y, 10.0f), 5.0f, 4, TrimColor, false, Duration);
	DrawDebugSphere(World, FVector(MarqueeEndWorld.X, MarqueeEndWorld.Y, 10.0f), 5.0f, 4, TrimColor, false, Duration);
}

void URTPlanTrimTool::PerformClickTrim(const FVector2D& WorldPos)
{
	if (!SpatialIndex) return;

	// Hit test to find the wall to trim
	FGuid HitWallId = SpatialIndex->HitTestWall(WorldPos, HitTestTolerance);
	
	if (HitWallId.IsValid())
	{
		TrimWallAtPoint(HitWallId, WorldPos);
	}
}

void URTPlanTrimTool::PerformMarqueeTrim()
{
	if (!Document || !SpatialIndex) return;

	// Fence trim: Find all walls intersected by the fence line (MarqueeStartWorld -> MarqueeEndWorld)
	// For each intersected wall, trim the segment that was crossed.

	const FRTPlanData& Data = Document->GetData();
	FVector2D FenceStart = MarqueeStartWorld;
	FVector2D FenceEnd = MarqueeEndWorld;

	// Use spatial index to find candidate walls in the bounding box of the fence
	FVector2D Min(FMath::Min(FenceStart.X, FenceEnd.X), FMath::Min(FenceStart.Y, FenceEnd.Y));
	FVector2D Max(FMath::Max(FenceStart.X, FenceEnd.X), FMath::Max(FenceStart.Y, FenceEnd.Y));
	
	// Add some padding to bounding box
	float Padding = 10.0f;
	Min -= FVector2D(Padding, Padding);
	Max += FVector2D(Padding, Padding);

	TArray<FGuid> CandidateWalls = SpatialIndex->HitTestWallsInRect(Min, Max);
	
	bool bAnyTrim = false;

	for (const FGuid& WallId : CandidateWalls)
	{
		const FRTWall* Wall = Data.Walls.Find(WallId);
		if (!Wall) continue;

		const FRTVertex* V1 = Data.Vertices.Find(Wall->VertexAId);
		const FRTVertex* V2 = Data.Vertices.Find(Wall->VertexBId);
		if (!V1 || !V2) continue;

		FVector2D Intersection;
		if (FRTPlanGeometryUtils::SegmentIntersection(FenceStart, FenceEnd, V1->Position, V2->Position, Intersection))
		{
			TrimWallAtPoint(WallId, Intersection);
			bAnyTrim = true;
		}
	}

	if (bAnyTrim)
	{
		UE_LOG(LogRTPlanTrimTool, Log, TEXT("Fence trim completed"));
	}
}

void URTPlanTrimTool::FindIntersectionsOnWall(const FGuid& WallId, TArray<float>& OutIntersections) const
{
	if (!Document) return;
	const FRTPlanData& Data = Document->GetData();
	
	const FRTWall* TargetWall = Data.Walls.Find(WallId);
	if (!TargetWall) return;
	
	const FRTVertex* V1 = Data.Vertices.Find(TargetWall->VertexAId);
	const FRTVertex* V2 = Data.Vertices.Find(TargetWall->VertexBId);
	if (!V1 || !V2) return;
	
	FVector2D A = V1->Position;
	FVector2D B = V2->Position;
	
	// Add endpoints (0 and 1)
	OutIntersections.Add(0.0f);
	OutIntersections.Add(1.0f);
	
	// Check against all other walls
	for (const auto& Pair : Data.Walls)
	{
		if (Pair.Key == WallId) continue; // Skip self
		
		const FRTWall& OtherWall = Pair.Value;
		const FRTVertex* OV1 = Data.Vertices.Find(OtherWall.VertexAId);
		const FRTVertex* OV2 = Data.Vertices.Find(OtherWall.VertexBId);
		
		if (OV1 && OV2)
		{
			FVector2D Intersection;
			if (FRTPlanGeometryUtils::SegmentIntersection(A, B, OV1->Position, OV2->Position, Intersection))
			{
				// Calculate T along target wall
				FVector2D AB = B - A;
				float LengthSq = AB.SizeSquared();
				if (LengthSq > KINDA_SMALL_NUMBER)
				{
					float T = ((Intersection - A) | AB) / LengthSq;
					
					// Use a slightly larger epsilon to avoid treating endpoints as intersections
					// unless they are true T-junctions.
					// However, for T-junctions (where one wall ends exactly on another),
					// SegmentIntersection might return true.
					// We want to include these points if they are strictly inside the segment [0,1]
					// OR if they are endpoints that connect to other geometry.
					
					// For trim logic, we care about points that divide the wall into segments.
					// Endpoints (0 and 1) are already added.
					// So we only add strictly interior points.
					if (T > KINDA_SMALL_NUMBER && T < 1.0f - KINDA_SMALL_NUMBER)
					{
						OutIntersections.Add(T);
					}
				}
			}
		}
	}
	
	// Sort intersections
	OutIntersections.Sort();
}

void URTPlanTrimTool::TrimWallAtPoint(const FGuid& WallId, const FVector2D& ClickPoint)
{
	if (!Document) return;
	const FRTPlanData& Data = Document->GetData();
	
	const FRTWall* Wall = Data.Walls.Find(WallId);
	if (!Wall) return;
	
	const FRTVertex* V1 = Data.Vertices.Find(Wall->VertexAId);
	const FRTVertex* V2 = Data.Vertices.Find(Wall->VertexBId);
	if (!V1 || !V2) return;
	
	// Handle arc walls separately
	if (Wall->bIsArc && FMath::Abs(Wall->ArcSweepAngle) > 0.1f)
	{
		TrimArcWallAtPoint(WallId, ClickPoint);
		return;
	}
	
	// Calculate T of click point for straight walls
	FVector2D A = V1->Position;
	FVector2D B = V2->Position;
	FVector2D Closest = FRTPlanGeometryUtils::ClosestPointOnSegment(ClickPoint, A, B);
	
	FVector2D AB = B - A;
	float LengthSq = AB.SizeSquared();
	float ClickT = 0.5f;
	if (LengthSq > KINDA_SMALL_NUMBER)
	{
		ClickT = ((Closest - A) | AB) / LengthSq;
	}
	
	// Find all intersections
	TArray<float> Intersections;
	FindIntersectionsOnWall(WallId, Intersections);
	
	// Find the segment containing ClickT
	float StartT = 0.0f;
	float EndT = 1.0f;
	
	// Robustness fix: Handle cases where ClickT is very close to an intersection
	// If ClickT is essentially equal to an intersection, we need to decide which side to trim.
	// Usually, the user clicks slightly "inside" the segment they want to remove.
	// But if they click exactly on a T-junction, it's ambiguous.
	// For now, assume standard behavior.
	
	for (int32 i = 0; i < Intersections.Num() - 1; ++i)
	{
		// Use a small epsilon for comparison to handle precision issues
		if (ClickT >= Intersections[i] - KINDA_SMALL_NUMBER && ClickT <= Intersections[i+1] + KINDA_SMALL_NUMBER)
		{
			StartT = Intersections[i];
			EndT = Intersections[i+1];
			break;
		}
	}
	
	// Edge case fix: If StartT and EndT are effectively the same (due to multiple intersections at same point),
	// expand to find a valid segment.
	if (FMath::IsNearlyEqual(StartT, EndT))
	{
		UE_LOG(LogRTPlanTrimTool, Warning, TEXT("Trim segment collapsed (StartT ~ EndT). Aborting trim."));
		return;
	}

	UE_LOG(LogRTPlanTrimTool, Log, TEXT("Trim segment: T [%0.2f, %0.2f]"), StartT, EndT);
	
	// Create macro command
	URTCmdMacro* MacroCmd = NewObject<URTCmdMacro>();
	MacroCmd->Description = TEXT("Trim Wall");
	MacroCmd->Document = Document;
	
	// Case 1: Trim entire wall (0 to 1) -> Delete wall
	if (FMath::IsNearlyEqual(StartT, 0.0f) && FMath::IsNearlyEqual(EndT, 1.0f))
	{
		URTCmdDeleteWall* DelCmd = NewObject<URTCmdDeleteWall>();
		DelCmd->WallId = WallId;
		MacroCmd->AddCommand(DelCmd);
	}
	// Case 2: Trim start (0 to T) -> Update Vertex A
	else if (FMath::IsNearlyEqual(StartT, 0.0f))
	{
		// We need to move Vertex A to EndT
		FVector2D NewPos = A + AB * EndT;
		
		FGuid NewVertexId = FGuid::NewGuid();
		FRTVertex NewVertex;
		NewVertex.Id = NewVertexId;
		NewVertex.Position = NewPos;
		
		URTCmdAddVertex* AddVertCmd = NewObject<URTCmdAddVertex>();
		AddVertCmd->Vertex = NewVertex;
		MacroCmd->AddCommand(AddVertCmd);
		
		// Update wall to use new vertex A
		URTCmdAddWall* UpdateWallCmd = NewObject<URTCmdAddWall>();
		UpdateWallCmd->Wall = *Wall;
		UpdateWallCmd->Wall.VertexAId = NewVertexId;
		UpdateWallCmd->bIsNew = false;
		MacroCmd->AddCommand(UpdateWallCmd);
	}
	// Case 3: Trim end (T to 1) -> Update Vertex B
	else if (FMath::IsNearlyEqual(EndT, 1.0f))
	{
		// We need to move Vertex B to StartT
		FVector2D NewPos = A + AB * StartT;
		
		FGuid NewVertexId = FGuid::NewGuid();
		FRTVertex NewVertex;
		NewVertex.Id = NewVertexId;
		NewVertex.Position = NewPos;
		
		URTCmdAddVertex* AddVertCmd = NewObject<URTCmdAddVertex>();
		AddVertCmd->Vertex = NewVertex;
		MacroCmd->AddCommand(AddVertCmd);
		
		// Update wall to use new vertex B
		URTCmdAddWall* UpdateWallCmd = NewObject<URTCmdAddWall>();
		UpdateWallCmd->Wall = *Wall;
		UpdateWallCmd->Wall.VertexBId = NewVertexId;
		UpdateWallCmd->bIsNew = false;
		MacroCmd->AddCommand(UpdateWallCmd);
	}
	// Case 4: Trim middle (T1 to T2) -> Split into two walls
	else
	{
		// Original wall becomes Wall 1 (0 to StartT)
		// New wall becomes Wall 2 (EndT to 1)
		
		FVector2D Pos1 = A + AB * StartT;
		FVector2D Pos2 = A + AB * EndT;
		
		// Create two new vertices
		FGuid V1Id = FGuid::NewGuid();
		FGuid V2Id = FGuid::NewGuid();
		
		FRTVertex Vert1; Vert1.Id = V1Id; Vert1.Position = Pos1;
		FRTVertex Vert2; Vert2.Id = V2Id; Vert2.Position = Pos2;
		
		URTCmdAddVertex* AddV1 = NewObject<URTCmdAddVertex>(); AddV1->Vertex = Vert1; MacroCmd->AddCommand(AddV1);
		URTCmdAddVertex* AddV2 = NewObject<URTCmdAddVertex>(); AddV2->Vertex = Vert2; MacroCmd->AddCommand(AddV2);
		
		// Update original wall to end at V1
		URTCmdAddWall* UpdateOriginal = NewObject<URTCmdAddWall>();
		UpdateOriginal->Wall = *Wall;
		UpdateOriginal->Wall.VertexBId = V1Id;
		UpdateOriginal->bIsNew = false;
		MacroCmd->AddCommand(UpdateOriginal);
		
		// Create new wall starting at V2
		FRTWall NewWall = *Wall;
		NewWall.Id = FGuid::NewGuid();
		NewWall.VertexAId = V2Id;
		// VertexBId remains original B
		
		URTCmdAddWall* AddNewWall = NewObject<URTCmdAddWall>();
		AddNewWall->Wall = NewWall;
		AddNewWall->bIsNew = true;
		MacroCmd->AddCommand(AddNewWall);
	}
	
	Document->SubmitCommand(MacroCmd);
}

bool URTPlanTrimTool::GetWorldPosition(const FRTPointerEvent& Event, FVector& OutWorldPos3D, FVector2D& OutWorldPos2D) const
{
	// Check if this is 3D mode (perspective camera - direction is not straight down)
	bool bIs3DMode = FMath::Abs(Event.WorldDirection.Z) < 0.99f;
	
	if (bIs3DMode)
	{
		// In 3D mode, perform a line trace to find the actual hit point on geometry
		FVector HitLocation;
		if (PerformLineTrace(Event, HitLocation))
		{
			OutWorldPos3D = HitLocation;
			OutWorldPos2D = FVector2D(HitLocation.X, HitLocation.Y);
			return true;
		}
	}
	
	// Try to intersect with the ground plane (Z=0)
	if (!FMath::IsNearlyZero(Event.WorldDirection.Z))
	{
		float T = -Event.WorldOrigin.Z / Event.WorldDirection.Z;
		
		if (T > 0)
		{
			FVector Intersection = Event.WorldOrigin + Event.WorldDirection * T;
			OutWorldPos3D = Intersection;
			OutWorldPos2D = FVector2D(Intersection.X, Intersection.Y);
			return true;
		}
	}
	
	// If looking up (can't hit ground), project onto wall mid-height plane (Z=150)
	const float DefaultWallMidHeight = 150.0f;
	if (!FMath::IsNearlyZero(Event.WorldDirection.Z))
	{
		float T = (DefaultWallMidHeight - Event.WorldOrigin.Z) / Event.WorldDirection.Z;
		if (T > 0)
		{
			FVector Intersection = Event.WorldOrigin + Event.WorldDirection * T;
			OutWorldPos3D = Intersection;
			OutWorldPos2D = FVector2D(Intersection.X, Intersection.Y);
			return true;
		}
	}
	
	// Last resort: forward projection
	const float DefaultProjectDistance = 1000.0f;
	FVector ProjectedPoint = Event.WorldOrigin + Event.WorldDirection * DefaultProjectDistance;
	OutWorldPos3D = ProjectedPoint;
	OutWorldPos2D = FVector2D(ProjectedPoint.X, ProjectedPoint.Y);
	return true;
}

bool URTPlanTrimTool::PerformLineTrace(const FRTPointerEvent& Event, FVector& OutHitLocation) const
{
	if (!GWorld) return false;
	
	FVector TraceStart = Event.WorldOrigin;
	FVector TraceEnd = Event.WorldOrigin + Event.WorldDirection * 100000.0f;
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnFaceIndex = false;
	
	// Ignore the pawn
	if (GWorld->GetFirstPlayerController())
	{
		if (APawn* Pawn = GWorld->GetFirstPlayerController()->GetPawn())
		{
			QueryParams.AddIgnoredActor(Pawn);
		}
	}
	
	bool bHit = GWorld->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		QueryParams
	);
	
	if (bHit)
	{
		OutHitLocation = HitResult.ImpactPoint;
		return true;
	}
	
	return false;
}

void URTPlanTrimTool::FindIntersectionsOnArcWall(const FGuid& WallId, TArray<float>& OutIntersections) const
{
	if (!Document) return;
	const FRTPlanData& Data = Document->GetData();
	
	const FRTWall* TargetWall = Data.Walls.Find(WallId);
	if (!TargetWall || !TargetWall->bIsArc) return;
	
	const FRTVertex* V1 = Data.Vertices.Find(TargetWall->VertexAId);
	const FRTVertex* V2 = Data.Vertices.Find(TargetWall->VertexBId);
	if (!V1 || !V2) return;
	
	FVector2D A = V1->Position;
	FVector2D ArcCenter = TargetWall->ArcCenter;
	float Radius = FVector2D::Distance(ArcCenter, A);
	
	FVector2D ToStart = A - ArcCenter;
	float StartAngleRad = FMath::Atan2(ToStart.Y, ToStart.X);
	float SweepRad = FMath::DegreesToRadians(TargetWall->ArcSweepAngle);
	
	// Add endpoints (0 and 1)
	OutIntersections.Add(0.0f);
	OutIntersections.Add(1.0f);
	
	// Check against all other walls for intersections
	for (const auto& Pair : Data.Walls)
	{
		if (Pair.Key == WallId) continue; // Skip self
		
		const FRTWall& OtherWall = Pair.Value;
		const FRTVertex* OV1 = Data.Vertices.Find(OtherWall.VertexAId);
		const FRTVertex* OV2 = Data.Vertices.Find(OtherWall.VertexBId);
		
		if (!OV1 || !OV2) continue;
		
		FVector2D OtherA = OV1->Position;
		FVector2D OtherB = OV2->Position;
		
		// Find line-circle intersections
		// Line from OtherA to OtherB, circle at ArcCenter with Radius
		FVector2D D = OtherB - OtherA;
		FVector2D F = OtherA - ArcCenter;
		
		float a = D | D;
		float b = 2.0f * (F | D);
		float c = (F | F) - Radius * Radius;
		
		float Discriminant = b * b - 4.0f * a * c;
		
		if (Discriminant >= 0 && a > KINDA_SMALL_NUMBER)
		{
			float SqrtDisc = FMath::Sqrt(Discriminant);
			
			// Two potential intersection points
			float T1 = (-b - SqrtDisc) / (2.0f * a);
			float T2 = (-b + SqrtDisc) / (2.0f * a);
			
			for (float T : { T1, T2 })
			{
				// Check if intersection is within the line segment
				if (T >= -KINDA_SMALL_NUMBER && T <= 1.0f + KINDA_SMALL_NUMBER)
				{
					FVector2D IntersectionPoint = OtherA + D * T;
					
					// Check if intersection is within the arc's angular range
					FVector2D ToIntersect = IntersectionPoint - ArcCenter;
					float IntersectAngle = FMath::Atan2(ToIntersect.Y, ToIntersect.X);
					
					// Calculate angle difference from start
					float AngleDiff = IntersectAngle - StartAngleRad;
					
					// Normalize to the sweep direction
					if (SweepRad > 0)
					{
						while (AngleDiff < 0) AngleDiff += 2.0f * PI;
						while (AngleDiff > 2.0f * PI) AngleDiff -= 2.0f * PI;
					}
					else
					{
						while (AngleDiff > 0) AngleDiff -= 2.0f * PI;
						while (AngleDiff < -2.0f * PI) AngleDiff += 2.0f * PI;
					}
					
					// Calculate T along arc (0 to 1)
					float ArcT = AngleDiff / SweepRad;
					
					// Only include strictly interior points
					if (ArcT > KINDA_SMALL_NUMBER && ArcT < 1.0f - KINDA_SMALL_NUMBER)
					{
						OutIntersections.Add(ArcT);
					}
				}
			}
		}
	}
	
	// Sort intersections
	OutIntersections.Sort();
}

void URTPlanTrimTool::TrimArcWallAtPoint(const FGuid& WallId, const FVector2D& ClickPoint)
{
	if (!Document) return;
	const FRTPlanData& Data = Document->GetData();
	
	const FRTWall* Wall = Data.Walls.Find(WallId);
	if (!Wall || !Wall->bIsArc) return;
	
	const FRTVertex* V1 = Data.Vertices.Find(Wall->VertexAId);
	const FRTVertex* V2 = Data.Vertices.Find(Wall->VertexBId);
	if (!V1 || !V2) return;
	
	FVector2D A = V1->Position;
	FVector2D ArcCenter = Wall->ArcCenter;
	float Radius = FVector2D::Distance(ArcCenter, A);
	float SweepAngleDeg = Wall->ArcSweepAngle;
	float SweepRad = FMath::DegreesToRadians(SweepAngleDeg);
	
	FVector2D ToStart = A - ArcCenter;
	float StartAngleRad = FMath::Atan2(ToStart.Y, ToStart.X);
	
	// Calculate click position as T along the arc (0 to 1)
	// Project click point onto the arc by finding the closest point
	FVector2D ToClick = ClickPoint - ArcCenter;
	float ClickAngle = FMath::Atan2(ToClick.Y, ToClick.X);
	
	// Calculate angle difference from start
	float AngleDiff = ClickAngle - StartAngleRad;
	
	// Normalize to the sweep direction
	if (SweepRad > 0)
	{
		while (AngleDiff < 0) AngleDiff += 2.0f * PI;
		while (AngleDiff > 2.0f * PI) AngleDiff -= 2.0f * PI;
		// Handle case where angle wrapped around
		if (AngleDiff > SweepRad + 0.1f)
		{
			AngleDiff -= 2.0f * PI;
		}
	}
	else
	{
		while (AngleDiff > 0) AngleDiff -= 2.0f * PI;
		while (AngleDiff < -2.0f * PI) AngleDiff += 2.0f * PI;
		if (AngleDiff < SweepRad - 0.1f)
		{
			AngleDiff += 2.0f * PI;
		}
	}
	
	float ClickT = FMath::Clamp(AngleDiff / SweepRad, 0.0f, 1.0f);
	
	// Find all intersections on the arc
	TArray<float> Intersections;
	FindIntersectionsOnArcWall(WallId, Intersections);
	
	// Find the segment containing ClickT
	float StartT = 0.0f;
	float EndT = 1.0f;
	
	for (int32 i = 0; i < Intersections.Num() - 1; ++i)
	{
		if (ClickT >= Intersections[i] - KINDA_SMALL_NUMBER && ClickT <= Intersections[i+1] + KINDA_SMALL_NUMBER)
		{
			StartT = Intersections[i];
			EndT = Intersections[i+1];
			break;
		}
	}
	
	if (FMath::IsNearlyEqual(StartT, EndT))
	{
		UE_LOG(LogRTPlanTrimTool, Warning, TEXT("Arc trim segment collapsed. Aborting."));
		return;
	}
	
	UE_LOG(LogRTPlanTrimTool, Log, TEXT("Arc Trim: T [%0.2f, %0.2f], ClickT=%0.2f"), StartT, EndT, ClickT);
	
	// Create macro command
	URTCmdMacro* MacroCmd = NewObject<URTCmdMacro>();
	MacroCmd->Description = TEXT("Trim Arc Wall");
	MacroCmd->Document = Document;
	
	// Case 1: Trim entire arc (0 to 1) -> Delete wall
	if (FMath::IsNearlyEqual(StartT, 0.0f) && FMath::IsNearlyEqual(EndT, 1.0f))
	{
		URTCmdDeleteWall* DelCmd = NewObject<URTCmdDeleteWall>();
		DelCmd->WallId = WallId;
		MacroCmd->AddCommand(DelCmd);
	}
	// Case 2: Trim start of arc (0 to EndT) -> Move VertexA and adjust sweep angle
	else if (FMath::IsNearlyEqual(StartT, 0.0f))
	{
		// Calculate new start position at EndT along the arc
		float NewStartAngle = StartAngleRad + SweepRad * EndT;
		FVector2D NewPos = ArcCenter + FVector2D(FMath::Cos(NewStartAngle), FMath::Sin(NewStartAngle)) * Radius;
		
		FGuid NewVertexId = FGuid::NewGuid();
		FRTVertex NewVertex;
		NewVertex.Id = NewVertexId;
		NewVertex.Position = NewPos;
		
		URTCmdAddVertex* AddVertCmd = NewObject<URTCmdAddVertex>();
		AddVertCmd->Vertex = NewVertex;
		MacroCmd->AddCommand(AddVertCmd);
		
		// Update wall with new start vertex and adjusted sweep angle
		URTCmdAddWall* UpdateWallCmd = NewObject<URTCmdAddWall>();
		UpdateWallCmd->Wall = *Wall;
		UpdateWallCmd->Wall.VertexAId = NewVertexId;
		// New sweep angle covers the remaining portion (1 - EndT) of original sweep
		UpdateWallCmd->Wall.ArcSweepAngle = SweepAngleDeg * (1.0f - EndT);
		UpdateWallCmd->bIsNew = false;
		MacroCmd->AddCommand(UpdateWallCmd);
	}
	// Case 3: Trim end of arc (StartT to 1) -> Move VertexB and adjust sweep angle
	else if (FMath::IsNearlyEqual(EndT, 1.0f))
	{
		// Calculate new end position at StartT along the arc
		float NewEndAngle = StartAngleRad + SweepRad * StartT;
		FVector2D NewPos = ArcCenter + FVector2D(FMath::Cos(NewEndAngle), FMath::Sin(NewEndAngle)) * Radius;
		
		FGuid NewVertexId = FGuid::NewGuid();
		FRTVertex NewVertex;
		NewVertex.Id = NewVertexId;
		NewVertex.Position = NewPos;
		
		URTCmdAddVertex* AddVertCmd = NewObject<URTCmdAddVertex>();
		AddVertCmd->Vertex = NewVertex;
		MacroCmd->AddCommand(AddVertCmd);
		
		// Update wall with new end vertex and adjusted sweep angle
		URTCmdAddWall* UpdateWallCmd = NewObject<URTCmdAddWall>();
		UpdateWallCmd->Wall = *Wall;
		UpdateWallCmd->Wall.VertexBId = NewVertexId;
		// New sweep angle covers the first portion (StartT) of original sweep
		UpdateWallCmd->Wall.ArcSweepAngle = SweepAngleDeg * StartT;
		UpdateWallCmd->bIsNew = false;
		MacroCmd->AddCommand(UpdateWallCmd);
	}
	// Case 4: Trim middle of arc (StartT to EndT) -> Split into two arcs
	else
	{
		// Calculate positions at StartT and EndT
		float Angle1 = StartAngleRad + SweepRad * StartT;
		float Angle2 = StartAngleRad + SweepRad * EndT;
		
		FVector2D Pos1 = ArcCenter + FVector2D(FMath::Cos(Angle1), FMath::Sin(Angle1)) * Radius;
		FVector2D Pos2 = ArcCenter + FVector2D(FMath::Cos(Angle2), FMath::Sin(Angle2)) * Radius;
		
		// Create two new vertices
		FGuid V1Id = FGuid::NewGuid();
		FGuid V2Id = FGuid::NewGuid();
		
		FRTVertex Vert1; Vert1.Id = V1Id; Vert1.Position = Pos1;
		FRTVertex Vert2; Vert2.Id = V2Id; Vert2.Position = Pos2;
		
		URTCmdAddVertex* AddV1 = NewObject<URTCmdAddVertex>(); 
		AddV1->Vertex = Vert1; 
		MacroCmd->AddCommand(AddV1);
		
		URTCmdAddVertex* AddV2 = NewObject<URTCmdAddVertex>(); 
		AddV2->Vertex = Vert2; 
		MacroCmd->AddCommand(AddV2);
		
		// Update original arc wall to end at V1 (covers 0 to StartT)
		URTCmdAddWall* UpdateOriginal = NewObject<URTCmdAddWall>();
		UpdateOriginal->Wall = *Wall;
		UpdateOriginal->Wall.VertexBId = V1Id;
		UpdateOriginal->Wall.ArcSweepAngle = SweepAngleDeg * StartT;
		UpdateOriginal->bIsNew = false;
		MacroCmd->AddCommand(UpdateOriginal);
		
		// Create new arc wall from V2 to original B (covers EndT to 1)
		FRTWall NewWall = *Wall;
		NewWall.Id = FGuid::NewGuid();
		NewWall.VertexAId = V2Id;
		// VertexBId remains original B
		NewWall.ArcSweepAngle = SweepAngleDeg * (1.0f - EndT);
		
		URTCmdAddWall* AddNewWall = NewObject<URTCmdAddWall>();
		AddNewWall->Wall = NewWall;
		AddNewWall->bIsNew = true;
		MacroCmd->AddCommand(AddNewWall);
	}
	
	Document->SubmitCommand(MacroCmd);
}

