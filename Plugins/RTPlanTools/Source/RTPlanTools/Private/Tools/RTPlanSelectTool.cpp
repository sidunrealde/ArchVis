#include "Tools/RTPlanSelectTool.h"
#include "RTPlanSpatialIndex.h"

DEFINE_LOG_CATEGORY(LogRTPlanSelectTool);

void URTPlanSelectTool::OnEnter()
{
	UE_LOG(LogRTPlanSelectTool, Log, TEXT("SelectTool: Entered"));
	State = EState::Idle;
	bIsMarqueeDragging = false;
}

void URTPlanSelectTool::OnExit()
{
	UE_LOG(LogRTPlanSelectTool, Log, TEXT("SelectTool: Exited"));
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
		OnSelectionChanged.Broadcast();
		UE_LOG(LogRTPlanSelectTool, Log, TEXT("Selection cleared"));
	}
}

bool URTPlanSelectTool::GetWorldPosition(const FRTPointerEvent& Event, FVector2D& OutWorldPos) const
{
	// Intersect ray with ground plane (Z=0)
	if (FMath::IsNearlyZero(Event.WorldDirection.Z))
	{
		return false;
	}

	float T = -Event.WorldOrigin.Z / Event.WorldDirection.Z;
	if (T < 0)
	{
		return false;
	}

	FVector Intersection = Event.WorldOrigin + Event.WorldDirection * T;
	OutWorldPos = FVector2D(Intersection.X, Intersection.Y);
	return true;
}

void URTPlanSelectTool::OnPointerEvent(const FRTPointerEvent& Event)
{
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
			bShiftOnMouseDown = Event.bShiftDown;
			bAltOnMouseDown = Event.bAltDown;
			
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
				UE_LOG(LogRTPlanSelectTool, Verbose, TEXT("Started marquee drag"));
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
		}
		else if (Event.Action == ERTPointerAction::Up)
		{
			// Complete marquee selection
			PerformMarqueeSelection(bShiftOnMouseDown, bAltOnMouseDown);
			
			State = EState::Idle;
			bIsMarqueeDragging = false;
		}
		else if (Event.Action == ERTPointerAction::Cancel)
		{
			// Cancel marquee
			State = EState::Idle;
			bIsMarqueeDragging = false;
			UE_LOG(LogRTPlanSelectTool, Verbose, TEXT("Marquee cancelled"));
		}
		break;
	}
}

void URTPlanSelectTool::PerformClickSelection(const FVector2D& WorldPos, bool bAddToSelection, bool bRemoveFromSelection)
{
	if (!SpatialIndex)
	{
		UE_LOG(LogRTPlanSelectTool, Warning, TEXT("No spatial index available for hit testing"));
		return;
	}

	// Try to hit test openings first (they're on top of walls visually)
	FGuid HitOpeningId = SpatialIndex->HitTestOpening(WorldPos, HitTestTolerance);
	
	if (HitOpeningId.IsValid())
	{
		if (bRemoveFromSelection)
		{
			SelectedOpeningIds.Remove(HitOpeningId);
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("Removed opening %s from selection"), *HitOpeningId.ToString());
		}
		else if (bAddToSelection)
		{
			SelectedOpeningIds.Add(HitOpeningId);
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("Added opening %s to selection"), *HitOpeningId.ToString());
		}
		else
		{
			// Replace selection
			SelectedWallIds.Empty();
			SelectedOpeningIds.Empty();
			SelectedOpeningIds.Add(HitOpeningId);
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("Selected opening %s"), *HitOpeningId.ToString());
		}
		
		OnSelectionChanged.Broadcast();
		return;
	}

	// Try to hit test walls
	FGuid HitWallId = SpatialIndex->HitTestWall(WorldPos, HitTestTolerance);
	
	if (HitWallId.IsValid())
	{
		if (bRemoveFromSelection)
		{
			SelectedWallIds.Remove(HitWallId);
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("Removed wall %s from selection"), *HitWallId.ToString());
		}
		else if (bAddToSelection)
		{
			SelectedWallIds.Add(HitWallId);
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("Added wall %s to selection"), *HitWallId.ToString());
		}
		else
		{
			// Replace selection
			SelectedWallIds.Empty();
			SelectedOpeningIds.Empty();
			SelectedWallIds.Add(HitWallId);
			UE_LOG(LogRTPlanSelectTool, Log, TEXT("Selected wall %s"), *HitWallId.ToString());
		}
		
		OnSelectionChanged.Broadcast();
		return;
	}

	// Clicked on empty space
	if (!bAddToSelection && !bRemoveFromSelection)
	{
		// Clear selection when clicking on empty space (without modifiers)
		ClearSelection();
	}
}

void URTPlanSelectTool::PerformMarqueeSelection(bool bAddToSelection, bool bRemoveFromSelection)
{
	if (!SpatialIndex)
	{
		UE_LOG(LogRTPlanSelectTool, Warning, TEXT("No spatial index available for marquee selection"));
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

	// Get all items in the marquee rect
	TArray<FGuid> WallsInRect = SpatialIndex->HitTestWallsInRect(RectMin, RectMax);
	TArray<FGuid> OpeningsInRect = SpatialIndex->HitTestOpeningsInRect(RectMin, RectMax);

	UE_LOG(LogRTPlanSelectTool, Log, TEXT("Marquee selection: %d walls, %d openings"), WallsInRect.Num(), OpeningsInRect.Num());

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
