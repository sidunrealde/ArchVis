#include "Tools/RTPlanSelectTool.h"
#include "RTPlanSpatialIndex.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"

DEFINE_LOG_CATEGORY(LogRTPlanSelectTool);

void URTPlanSelectTool::OnEnter()
{
	State = EState::Idle;
	bIsMarqueeDragging = false;
}

void URTPlanSelectTool::OnExit()
{
	// Don't clear selection on exit - preserve it for property editing
	State = EState::Idle;
	bIsMarqueeDragging = false;
}

void URTPlanSelectTool::ClearSelection()
{
	bool bHadSelection = HasSelection();
	SelectedWallIds.Empty();
	SelectedOpeningIds.Empty();
	
	if (bHadSelection)
	{
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("ClearSelection: Selection cleared"));
		OnSelectionChanged.Broadcast();
	}
}

void URTPlanSelectTool::SelectAll()
{
	if (!Document)
	{
		UE_LOG(LogRTPlanSelectTool, Warning, TEXT("SelectAll: No document"));
		return;
	}

	const FRTPlanData& Data = Document->GetData();
	
	SelectedWallIds.Empty();
	SelectedOpeningIds.Empty();
	
	// Select all walls
	for (const auto& Pair : Data.Walls)
	{
		SelectedWallIds.Add(Pair.Key);
	}
	
	// Select all openings
	for (const auto& Pair : Data.Openings)
	{
		SelectedOpeningIds.Add(Pair.Key);
	}
	
	UE_LOG(LogRTPlanSelectTool, Log, TEXT("SelectAll: Selected %d walls, %d openings"), 
		SelectedWallIds.Num(), SelectedOpeningIds.Num());
	
	OnSelectionChanged.Broadcast();
}

FVector URTPlanSelectTool::GetSelectionCenter() const
{
	if (!Document || !HasSelection())
	{
		return FVector::ZeroVector;
	}

	const FRTPlanData& Data = Document->GetData();
	FVector Sum = FVector::ZeroVector;
	int32 Count = 0;

	// Accumulate wall centers
	for (const FGuid& WallId : SelectedWallIds)
	{
		const FRTWall* Wall = Data.Walls.Find(WallId);
		if (Wall)
		{
			const FRTVertex* VertA = Data.Vertices.Find(Wall->VertexAId);
			const FRTVertex* VertB = Data.Vertices.Find(Wall->VertexBId);
			if (VertA && VertB)
			{
				FVector2D Center = (VertA->Position + VertB->Position) * 0.5f;
				Sum += FVector(Center.X, Center.Y, Wall->HeightCm * 0.5f);
				Count++;
			}
		}
	}

	// Accumulate opening centers
	for (const FGuid& OpeningId : SelectedOpeningIds)
	{
		const FRTOpening* Opening = Data.Openings.Find(OpeningId);
		if (Opening)
		{
			// Get the wall this opening belongs to
			const FRTWall* Wall = Data.Walls.Find(Opening->WallId);
			if (Wall)
			{
				const FRTVertex* VertA = Data.Vertices.Find(Wall->VertexAId);
				const FRTVertex* VertB = Data.Vertices.Find(Wall->VertexBId);
				if (VertA && VertB)
				{
					// Interpolate position along wall based on offset
					FVector2D WallDir = (VertB->Position - VertA->Position);
					FVector2D OpeningPos = VertA->Position + WallDir.GetSafeNormal() * Opening->OffsetCm;
					Sum += FVector(OpeningPos.X, OpeningPos.Y, Opening->SillHeightCm + Opening->HeightCm * 0.5f);
					Count++;
				}
			}
		}
	}

	if (Count > 0)
	{
		return Sum / static_cast<float>(Count);
	}

	return FVector::ZeroVector;
}

FBox URTPlanSelectTool::GetSelectionBounds() const
{
	FBox Bounds(ForceInit);
	
	if (!Document || !HasSelection())
	{
		return Bounds;
	}

	const FRTPlanData& Data = Document->GetData();

	// Accumulate wall bounds
	for (const FGuid& WallId : SelectedWallIds)
	{
		const FRTWall* Wall = Data.Walls.Find(WallId);
		if (Wall)
		{
			const FRTVertex* VertA = Data.Vertices.Find(Wall->VertexAId);
			const FRTVertex* VertB = Data.Vertices.Find(Wall->VertexBId);
			if (VertA && VertB)
			{
				Bounds += FVector(VertA->Position.X, VertA->Position.Y, 0);
				Bounds += FVector(VertB->Position.X, VertB->Position.Y, 0);
				Bounds += FVector(VertA->Position.X, VertA->Position.Y, Wall->HeightCm);
				Bounds += FVector(VertB->Position.X, VertB->Position.Y, Wall->HeightCm);
			}
		}
	}

	// Accumulate opening bounds
	for (const FGuid& OpeningId : SelectedOpeningIds)
	{
		const FRTOpening* Opening = Data.Openings.Find(OpeningId);
		if (Opening)
		{
			const FRTWall* Wall = Data.Walls.Find(Opening->WallId);
			if (Wall)
			{
				const FRTVertex* VertA = Data.Vertices.Find(Wall->VertexAId);
				const FRTVertex* VertB = Data.Vertices.Find(Wall->VertexBId);
				if (VertA && VertB)
				{
					// Calculate opening position along wall
					FVector2D WallDir = (VertB->Position - VertA->Position);
					FVector2D DirNormalized = WallDir.GetSafeNormal();
					FVector2D OpeningCenter = VertA->Position + DirNormalized * Opening->OffsetCm;
					
					// Calculate opening width extent
					float HalfWidth = Opening->WidthCm * 0.5f;
					FVector2D P1 = OpeningCenter - DirNormalized * HalfWidth;
					FVector2D P2 = OpeningCenter + DirNormalized * HalfWidth;
					
					// Add bounds (approximate as rectangular prism aligned with wall)
					float ZBottom = Opening->SillHeightCm;
					float ZTop = Opening->SillHeightCm + Opening->HeightCm;
					
					Bounds += FVector(P1.X, P1.Y, ZBottom);
					Bounds += FVector(P2.X, P2.Y, ZTop);
					Bounds += FVector(P1.X, P1.Y, ZTop);
					Bounds += FVector(P2.X, P2.Y, ZBottom);
				}
			}
		}
	}

	return Bounds;
}

bool URTPlanSelectTool::GetWorldPosition(const FRTPointerEvent& Event, FVector2D& OutWorldPos) const
{
	// For 2D mode (top-down ortho), the WorldDirection will be (0,0,-1)
	// For 3D mode (perspective), we need to handle rays that might not intersect Z=0
	
	if (bDebugEnabled)
	{
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("GetWorldPosition: Origin=(%0.1f, %0.1f, %0.1f), Dir=(%0.3f, %0.3f, %0.3f)"),
			Event.WorldOrigin.X, Event.WorldOrigin.Y, Event.WorldOrigin.Z,
			Event.WorldDirection.X, Event.WorldDirection.Y, Event.WorldDirection.Z);
	}
	
	// Check if this is 3D mode (perspective camera - direction is not straight down)
	bool bIs3DMode = FMath::Abs(Event.WorldDirection.Z) < 0.99f;
	
	if (bIs3DMode)
	{
		// In 3D mode, perform a line trace to find the actual hit point on geometry
		FVector HitLocation;
		if (PerformLineTrace(Event, HitLocation))
		{
			OutWorldPos = FVector2D(HitLocation.X, HitLocation.Y);
			return true;
		}
		
		// Line trace failed - fall through to ground plane intersection
		if (bDebugEnabled)
		{
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("  -> 3D LineTrace failed, falling back to ground plane"));
		}
	}
	
	// Try to intersect with the ground plane (Z=0)
	if (!FMath::IsNearlyZero(Event.WorldDirection.Z))
	{
		float T = -Event.WorldOrigin.Z / Event.WorldDirection.Z;
		
		if (T > 0)
		{
			FVector Intersection = Event.WorldOrigin + Event.WorldDirection * T;
			OutWorldPos = FVector2D(Intersection.X, Intersection.Y);
			
			if (bDebugEnabled && GWorld)
			{
				DrawDebugSphere(GWorld, Intersection, 20.0f, 8, FColor::Green, false, 0.1f);
			}
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
			OutWorldPos = FVector2D(Intersection.X, Intersection.Y);
			
			if (bDebugEnabled && GWorld)
			{
				DrawDebugSphere(GWorld, Intersection, 20.0f, 8, FColor::Blue, false, 0.1f);
			}
			return true;
		}
	}
	
	// Last resort: forward projection (looking horizontally)
	const float DefaultProjectDistance = 1000.0f;
	FVector ProjectedPoint = Event.WorldOrigin + Event.WorldDirection * DefaultProjectDistance;
	OutWorldPos = FVector2D(ProjectedPoint.X, ProjectedPoint.Y);
	
	if (bDebugEnabled && GWorld)
	{
		DrawDebugSphere(GWorld, ProjectedPoint, 20.0f, 8, FColor::Red, false, 0.1f);
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("  -> Forward projection at (%0.1f, %0.1f)"), OutWorldPos.X, OutWorldPos.Y);
	}
	return true;
}

bool URTPlanSelectTool::PerformLineTrace(const FRTPointerEvent& Event, FVector& OutHitLocation) const
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
	
	if (bDebugEnabled)
	{
		// Draw the trace line
		DrawDebugLine(GWorld, TraceStart, bHit ? HitResult.ImpactPoint : TraceEnd, 
			bHit ? FColor::Green : FColor::Red, false, 3.0f, 0, 2.0f);
		
		if (bHit)
		{
			// Draw hit point
			DrawDebugSphere(GWorld, HitResult.ImpactPoint, 15.0f, 12, FColor::Green, false, 3.0f);
			DrawDebugLine(GWorld, HitResult.ImpactPoint, HitResult.ImpactPoint + HitResult.ImpactNormal * 50.0f, 
				FColor::Blue, false, 3.0f, 0, 2.0f);
			
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("LineTrace: Hit %s at (%0.1f, %0.1f, %0.1f)"),
				*HitResult.GetActor()->GetName(),
				HitResult.ImpactPoint.X, HitResult.ImpactPoint.Y, HitResult.ImpactPoint.Z);
		}
		else
		{
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("LineTrace: No hit"));
		}
	}
	
	if (bHit)
	{
		OutHitLocation = HitResult.ImpactPoint;
		return true;
	}
	
	return false;
}

void URTPlanSelectTool::OnPointerEvent(const FRTPointerEvent& Event)
{
	// Log non-Move actions for debugging
	if (bDebugEnabled && Event.Action != ERTPointerAction::Move)
	{
		const TCHAR* ActionStr = TEXT("Unknown");
		switch (Event.Action)
		{
			case ERTPointerAction::Down: ActionStr = TEXT("Down"); break;
			case ERTPointerAction::Up: ActionStr = TEXT("Up"); break;
			case ERTPointerAction::Cancel: ActionStr = TEXT("Cancel"); break;
			case ERTPointerAction::Confirm: ActionStr = TEXT("Confirm"); break;
			default: break;
		}
		
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("OnPointerEvent: Action=%s, Shift=%d, Alt=%d, Ctrl=%d"),
			ActionStr, Event.bShiftDown, Event.bAltDown, Event.bCtrlDown);
	}

	FVector2D WorldPos;
	bool bHasWorldPos = GetWorldPosition(Event, WorldPos);

	switch (State)
	{
	case EState::Idle:
		if (Event.Action == ERTPointerAction::Down)
		{
			// Start potential drag
			State = EState::PotentialDrag;
			MouseDownScreenPos = Event.ScreenPosition;
			
			// Capture modifier flags from event
			bShiftOnMouseDown = Event.bShiftDown;
			bAltOnMouseDown = Event.bAltDown;
			bCtrlOnMouseDown = Event.bCtrlDown;
			
			if (bDebugEnabled)
			{
				UE_LOG(LogRTPlanSelectTool, Log, TEXT("  -> PotentialDrag started: Shift=%d, Alt=%d, Ctrl=%d"),
					bShiftOnMouseDown, bAltOnMouseDown, bCtrlOnMouseDown);
			}
			if (bHasWorldPos)
			{
				MarqueeStartWorld = WorldPos;
				MarqueeEndWorld = WorldPos;
			}
		}
		else if (Event.Action == ERTPointerAction::Move)
		{
			if (bHasWorldPos)
			{
				// Update snapped cursor position for crosshair
				LastSnappedWorldPos = FVector(WorldPos.X, WorldPos.Y, 0.0f);
			}
			else
			{
				// Project cursor into distance when looking at sky so it doesn't get stuck at horizon
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
				if (bDebugEnabled)
				{
					UE_LOG(LogRTPlanSelectTool, Log, TEXT("  -> State=MarqueeDragging (DragDistance=%0.1f > Threshold=%0.1f)"), 
						DragDistance, DragThreshold);
				}
			}
			
			if (bHasWorldPos)
			{
				MarqueeEndWorld = WorldPos;
				LastSnappedWorldPos = FVector(WorldPos.X, WorldPos.Y, 0.0f);
			}
			else
			{
				LastSnappedWorldPos = Event.WorldOrigin + Event.WorldDirection * 100000.0f;
			}
		}
		else if (Event.Action == ERTPointerAction::Up)
		{
			// This was a click, not a drag
			if (bHasWorldPos)
			{
				PerformClickSelection(WorldPos, bShiftOnMouseDown, bAltOnMouseDown);
			}
			State = EState::Idle;
		}
		break;

	case EState::MarqueeDragging:
		if (Event.Action == ERTPointerAction::Move)
		{
			if (bHasWorldPos)
			{
				MarqueeEndWorld = WorldPos;
				LastSnappedWorldPos = FVector(WorldPos.X, WorldPos.Y, 0.0f);
			}
			else
			{
				LastSnappedWorldPos = Event.WorldOrigin + Event.WorldDirection * 100000.0f;
			}
			
			// Always draw marquee visualization during drag
			if (GWorld)
			{
				DrawMarqueeVisualization(GWorld);
			}
		}
		else if (Event.Action == ERTPointerAction::Up)
		{
			// Complete marquee selection
			if (bDebugEnabled)
			{
				UE_LOG(LogRTPlanSelectTool, Log, TEXT("Marquee complete: Start=(%0.1f, %0.1f), End=(%0.1f, %0.1f)"),
					MarqueeStartWorld.X, MarqueeStartWorld.Y, MarqueeEndWorld.X, MarqueeEndWorld.Y);
			}
			PerformMarqueeSelection(bShiftOnMouseDown, bAltOnMouseDown);
			
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

void URTPlanSelectTool::DrawMarqueeVisualization(UWorld* World) const
{
	if (!World || !bIsMarqueeDragging) return;
	
	FVector MinCorner(FMath::Min(MarqueeStartWorld.X, MarqueeEndWorld.X), 
					  FMath::Min(MarqueeStartWorld.Y, MarqueeEndWorld.Y), 5.0f);
	FVector MaxCorner(FMath::Max(MarqueeStartWorld.X, MarqueeEndWorld.X), 
					  FMath::Max(MarqueeStartWorld.Y, MarqueeEndWorld.Y), 5.0f);
	
	// Draw rectangle outline in white
	FColor MarqueeColor = FColor::White;
	float LineThickness = 2.0f;
	float Duration = 0.0f; // Single frame
	
	// Bottom edge
	DrawDebugLine(World, FVector(MinCorner.X, MinCorner.Y, 5.0f), FVector(MaxCorner.X, MinCorner.Y, 5.0f), MarqueeColor, false, Duration, 0, LineThickness);
	// Right edge
	DrawDebugLine(World, FVector(MaxCorner.X, MinCorner.Y, 5.0f), FVector(MaxCorner.X, MaxCorner.Y, 5.0f), MarqueeColor, false, Duration, 0, LineThickness);
	// Top edge
	DrawDebugLine(World, FVector(MaxCorner.X, MaxCorner.Y, 5.0f), FVector(MinCorner.X, MaxCorner.Y, 5.0f), MarqueeColor, false, Duration, 0, LineThickness);
	// Left edge
	DrawDebugLine(World, FVector(MinCorner.X, MaxCorner.Y, 5.0f), FVector(MinCorner.X, MinCorner.Y, 5.0f), MarqueeColor, false, Duration, 0, LineThickness);
	
	// Draw corner markers
	float CornerSize = 10.0f;
	DrawDebugSphere(World, FVector(MinCorner.X, MinCorner.Y, 5.0f), CornerSize, 4, FColor::Yellow, false, Duration);
	DrawDebugSphere(World, FVector(MaxCorner.X, MinCorner.Y, 5.0f), CornerSize, 4, FColor::Yellow, false, Duration);
	DrawDebugSphere(World, FVector(MaxCorner.X, MaxCorner.Y, 5.0f), CornerSize, 4, FColor::Yellow, false, Duration);
	DrawDebugSphere(World, FVector(MinCorner.X, MaxCorner.Y, 5.0f), CornerSize, 4, FColor::Yellow, false, Duration);
}

void URTPlanSelectTool::PerformClickSelection(const FVector2D& WorldPos, bool bAddToSelection, bool bRemoveFromSelection)
{
	if (!SpatialIndex)
	{
		UE_LOG(LogRTPlanSelectTool, Warning, TEXT("PerformClickSelection: No SpatialIndex"));
		return;
	}

	// Check if this is a toggle operation (Ctrl+Click)
	bool bIsToggle = bCtrlOnMouseDown;
	
	if (bDebugEnabled)
	{
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("PerformClickSelection: Pos=(%0.1f, %0.1f), Add=%d, Remove=%d, Toggle=%d, Tolerance=%0.1f"),
			WorldPos.X, WorldPos.Y, bAddToSelection, bRemoveFromSelection, bIsToggle, HitTestTolerance);
		
		// Draw debug visualization of spatial index
		if (GWorld)
		{
			SpatialIndex->DrawDebugSegments(GWorld, 5.0f);
			
			// Draw the click point
			DrawDebugSphere(GWorld, FVector(WorldPos.X, WorldPos.Y, 10.0f), 15.0f, 12, FColor::Orange, false, 5.0f);
			
			// Draw the hit tolerance radius
			DrawDebugCircle(GWorld, FVector(WorldPos.X, WorldPos.Y, 10.0f), HitTestTolerance, 32, FColor::Yellow, false, 5.0f, 0, 2.0f, FVector(1, 0, 0), FVector(0, 1, 0), false);
		}
	}

	// Try to hit test openings first (they're on top of walls visually)
	FGuid HitOpeningId = SpatialIndex->HitTestOpening(WorldPos, HitTestTolerance);
	
	if (HitOpeningId.IsValid())
	{
		if (bIsToggle)
		{
			// Toggle: if selected, remove; if not selected, add
			if (SelectedOpeningIds.Contains(HitOpeningId))
			{
				SelectedOpeningIds.Remove(HitOpeningId);
			}
			else
			{
				SelectedOpeningIds.Add(HitOpeningId);
			}
		}
		else if (bRemoveFromSelection)
		{
			SelectedOpeningIds.Remove(HitOpeningId);
		}
		else if (bAddToSelection)
		{
			SelectedOpeningIds.Add(HitOpeningId);
		}
		else
		{
			// Replace selection
			SelectedWallIds.Empty();
			SelectedOpeningIds.Empty();
			SelectedOpeningIds.Add(HitOpeningId);
		}
		
		OnSelectionChanged.Broadcast();
		return;
	}

	// Try to hit test walls
	FGuid HitWallId = SpatialIndex->HitTestWall(WorldPos, HitTestTolerance);
	
	if (HitWallId.IsValid())
	{
		if (bDebugEnabled)
		{
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("  Hit wall: %s"), *HitWallId.ToString());
		}
		
		if (bIsToggle)
		{
			// Toggle: if selected, remove; if not selected, add
			if (SelectedWallIds.Contains(HitWallId))
			{
				SelectedWallIds.Remove(HitWallId);
				if (bDebugEnabled) UE_LOG(LogRTPlanSelectTool, Log, TEXT("  -> Toggled OFF"));
			}
			else
			{
				SelectedWallIds.Add(HitWallId);
				if (bDebugEnabled) UE_LOG(LogRTPlanSelectTool, Log, TEXT("  -> Toggled ON"));
			}
		}
		else if (bRemoveFromSelection)
		{
			SelectedWallIds.Remove(HitWallId);
			if (bDebugEnabled) UE_LOG(LogRTPlanSelectTool, Log, TEXT("  -> Removed from selection"));
		}
		else if (bAddToSelection)
		{
			SelectedWallIds.Add(HitWallId);
			if (bDebugEnabled) UE_LOG(LogRTPlanSelectTool, Log, TEXT("  -> Added to selection"));
		}
		else
		{
			// Replace selection
			SelectedWallIds.Empty();
			SelectedOpeningIds.Empty();
			SelectedWallIds.Add(HitWallId);
			if (bDebugEnabled) UE_LOG(LogRTPlanSelectTool, Log, TEXT("  -> Replaced selection"));
		}
		
		if (bDebugEnabled) UE_LOG(LogRTPlanSelectTool, Log, TEXT("  Selection now: %d walls"), SelectedWallIds.Num());
		OnSelectionChanged.Broadcast();
		return;
	}
	else
	{
		if (bDebugEnabled) UE_LOG(LogRTPlanSelectTool, Log, TEXT("  No wall hit at (%0.1f, %0.1f)"), WorldPos.X, WorldPos.Y);
	}

	// Clicked on empty space
	if (!bAddToSelection && !bRemoveFromSelection && !bIsToggle)
	{
		// Clear selection when clicking on empty space (without modifiers)
		if (bDebugEnabled) UE_LOG(LogRTPlanSelectTool, Log, TEXT("  -> Clearing selection (empty space click)"));
		ClearSelection();
	}
}

void URTPlanSelectTool::PerformMarqueeSelection(bool bAddToSelection, bool bRemoveFromSelection)
{
	if (!SpatialIndex)
	{
		UE_LOG(LogRTPlanSelectTool, Warning, TEXT("PerformMarqueeSelection: No SpatialIndex"));
		return;
	}

	// Calculate rect bounds (ensure Min < Max)
	FVector2D RectMin(
		FMath::Min(MarqueeStartWorld.X, MarqueeEndWorld.X),
		FMath::Min(MarqueeStartWorld.Y, MarqueeEndWorld.Y)
	);
	FVector2D RectMax(
		FMath::Max(MarqueeStartWorld.X, MarqueeEndWorld.X),
		FMath::Max(MarqueeStartWorld.Y, MarqueeEndWorld.Y)
	);
	
	if (bDebugEnabled)
	{
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("PerformMarqueeSelection: Min=(%0.1f, %0.1f), Max=(%0.1f, %0.1f), Add=%d, Remove=%d"),
			RectMin.X, RectMin.Y, RectMax.X, RectMax.Y, bAddToSelection, bRemoveFromSelection);
		
		// Draw the final marquee rect
		if (GWorld)
		{
			DrawDebugLine(GWorld, FVector(RectMin.X, RectMin.Y, 10.0f), FVector(RectMax.X, RectMin.Y, 10.0f), FColor::Cyan, false, 5.0f, 0, 3.0f);
			DrawDebugLine(GWorld, FVector(RectMax.X, RectMin.Y, 10.0f), FVector(RectMax.X, RectMax.Y, 10.0f), FColor::Cyan, false, 5.0f, 0, 3.0f);
			DrawDebugLine(GWorld, FVector(RectMax.X, RectMax.Y, 10.0f), FVector(RectMin.X, RectMax.Y, 10.0f), FColor::Cyan, false, 5.0f, 0, 3.0f);
			DrawDebugLine(GWorld, FVector(RectMin.X, RectMax.Y, 10.0f), FVector(RectMin.X, RectMin.Y, 10.0f), FColor::Cyan, false, 5.0f, 0, 3.0f);
			
			// Draw debug segments to compare
			SpatialIndex->DrawDebugSegments(GWorld, 5.0f);
		}
	}

	// Get all items in the marquee rect
	TArray<FGuid> WallsInRect = SpatialIndex->HitTestWallsInRect(RectMin, RectMax);
	TArray<FGuid> OpeningsInRect = SpatialIndex->HitTestOpeningsInRect(RectMin, RectMax);

	if (bDebugEnabled)
	{
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("  Found %d walls, %d openings in rect"), WallsInRect.Num(), OpeningsInRect.Num());
	}


	if (bRemoveFromSelection)
	{
		// Remove items from selection
		for (const FGuid& WallId : WallsInRect)
		{
			SelectedWallIds.Remove(WallId);
		}
		for (const FGuid& OpeningId : OpeningsInRect)
		{
			SelectedOpeningIds.Remove(OpeningId);
		}
	}
	else if (bAddToSelection)
	{
		// Add items to selection
		for (const FGuid& WallId : WallsInRect)
		{
			SelectedWallIds.Add(WallId);
		}
		for (const FGuid& OpeningId : OpeningsInRect)
		{
			SelectedOpeningIds.Add(OpeningId);
		}
	}
	else
	{
		// Replace selection
		SelectedWallIds.Empty();
		SelectedOpeningIds.Empty();
		
		for (const FGuid& WallId : WallsInRect)
		{
			SelectedWallIds.Add(WallId);
		}
		for (const FGuid& OpeningId : OpeningsInRect)
		{
			SelectedOpeningIds.Add(OpeningId);
		}
	}

	OnSelectionChanged.Broadcast();
}

float URTPlanSelectTool::GetWallLength(const FGuid& WallId) const
{
	if (!Document) return 0.0f;

	const FRTPlanData& Data = Document->GetData();
	const FRTWall* Wall = Data.Walls.Find(WallId);
	if (!Wall) return 0.0f;

	const FRTVertex* VertA = Data.Vertices.Find(Wall->VertexAId);
	const FRTVertex* VertB = Data.Vertices.Find(Wall->VertexBId);
	if (!VertA || !VertB) return 0.0f;

	return FVector2D::Distance(VertA->Position, VertB->Position);
}

void URTPlanSelectTool::LogSelectedWallProperties() const
{
	if (!Document)
	{
		UE_LOG(LogRTPlanSelectTool, Warning, TEXT("No document available"));
		return;
	}

	const FRTPlanData& Data = Document->GetData();
	
	if (SelectedWallIds.Num() == 0)
	{
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("No walls selected"));
		return;
	}

	UE_LOG(LogRTPlanSelectTool, Log, TEXT("=== Selected Wall Properties (%d walls) ==="), SelectedWallIds.Num());

	for (const FGuid& WallId : SelectedWallIds)
	{
		const FRTWall* Wall = Data.Walls.Find(WallId);
		if (!Wall)
		{
			UE_LOG(LogRTPlanSelectTool, Warning, TEXT("  Wall %s: NOT FOUND"), *WallId.ToString());
			continue;
		}

		// Get vertices for length calculation
		float Length = GetWallLength(WallId);

		UE_LOG(LogRTPlanSelectTool, Log, TEXT("  Wall ID: %s"), *WallId.ToString());
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("    Length: %.2f cm (%.2f m)"), Length, Length / 100.0f);
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("    Thickness: %.2f cm"), Wall->ThicknessCm);
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("    Height: %.2f cm (%.2f m)"), Wall->HeightCm, Wall->HeightCm / 100.0f);
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("    Base Z: %.2f cm"), Wall->BaseZCm);
		
		// Skirting info
		if (Wall->bHasLeftSkirting)
		{
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("    Left Skirting: Height=%.2f cm, Thickness=%.2f cm"), 
				Wall->LeftSkirtingHeightCm, Wall->LeftSkirtingThicknessCm);
		}
		else
		{
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("    Left Skirting: None"));
		}

		if (Wall->bHasRightSkirting)
		{
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("    Right Skirting: Height=%.2f cm, Thickness=%.2f cm"), 
				Wall->RightSkirtingHeightCm, Wall->RightSkirtingThicknessCm);
		}
		else
		{
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("    Right Skirting: None"));
		}
	}

	UE_LOG(LogRTPlanSelectTool, Log, TEXT("========================================"));
}
