#include "Tools/RTPlanLineTool.h"
#include "RTPlanCommand.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTPlanLineTool, Log, All);

void URTPlanLineTool::OnEnter()
{
	State = EState::WaitingForStart;
	bShowPreview = false;
	bIsOrthoSnapped = false;
	CachedLengthCm = 0.0f;
	CachedAngleDegrees = 0.0f;
	UE_LOG(LogRTPlanLineTool, Log, TEXT("Line Tool Activated"));
}

void URTPlanLineTool::OnExit()
{
	bShowPreview = false;
	bIsOrthoSnapped = false;
	UE_LOG(LogRTPlanLineTool, Log, TEXT("Line Tool Deactivated"));
}

void URTPlanLineTool::SetPolylineMode(bool bEnable)
{
	bIsPolyline = bEnable;
}

FRTDraftingState URTPlanLineTool::GetDraftingState() const
{
	FRTDraftingState DraftState;
	DraftState.bIsActive = (State == EState::WaitingForEnd) && bShowPreview;
	DraftState.StartPoint = StartPoint;
	DraftState.EndPoint = CurrentEndPoint;
	DraftState.LengthCm = CachedLengthCm;
	DraftState.AngleDegrees = CachedAngleDegrees;
	DraftState.bOrthoSnapped = bIsOrthoSnapped;
	return DraftState;
}

float URTPlanLineTool::GetCurrentLengthCm() const
{
	return CachedLengthCm;
}

float URTPlanLineTool::GetCurrentAngleDegrees() const
{
	return CachedAngleDegrees;
}

void URTPlanLineTool::UpdatePreviewFromLength(float LengthCm)
{
	if (State != EState::WaitingForEnd || LengthCm < 0.1f)
	{
		return;
	}
	
	// Save the current direction (above or below horizontal) when numeric input starts
	if (!bNumericInputActive)
	{
		FVector2D CurrentDir = (CurrentEndPoint - StartPoint).GetSafeNormal();
		// Y is positive going down in world coords, so positive Y means below horizontal
		bSavedDirectionAboveHorizontal = (CurrentDir.Y < 0.0f);
	}
	
	// Keep current direction, update length
	FVector2D Dir = (CurrentEndPoint - StartPoint).GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		Dir = FVector2D(1.0f, 0.0f); // Default to rightward
	}
	
	CurrentEndPoint = StartPoint + Dir * LengthCm;
	LastSnappedWorldPos = FVector(CurrentEndPoint.X, CurrentEndPoint.Y, 0.0f);
	bNumericInputActive = true; // Prevent mouse from overriding
	UpdateCachedValues();
	
	UE_LOG(LogRTPlanLineTool, Verbose, TEXT("UpdatePreviewFromLength: %.1f cm -> EndPoint: %s"), LengthCm, *CurrentEndPoint.ToString());
}

void URTPlanLineTool::UpdatePreviewFromAngle(float AngleDegrees)
{
	if (State != EState::WaitingForEnd)
	{
		return;
	}
	
	// Save the current direction (above or below horizontal) when numeric input starts
	if (!bNumericInputActive)
	{
		FVector2D CurrentDir = (CurrentEndPoint - StartPoint).GetSafeNormal();
		// Y is positive going down in world coords, so positive Y means below horizontal
		bSavedDirectionAboveHorizontal = (CurrentDir.Y < 0.0f);
	}
	
	// Clamp angle to 0-180 (angle from horizontal)
	AngleDegrees = FMath::Clamp(AngleDegrees, 0.0f, 180.0f);
	
	// Keep current length, update angle
	float CurrentLength = (CurrentEndPoint - StartPoint).Size();
	if (CurrentLength < 0.1f)
	{
		CurrentLength = 100.0f; // Default length if too short
	}
	
	// Apply angle based on saved direction
	// If originally above horizontal (negative Y), use positive angle (counter-clockwise)
	// If originally below horizontal (positive Y), use negative angle (clockwise)
	float AngleRad = FMath::DegreesToRadians(AngleDegrees);
	FVector2D NewDir;
	if (bSavedDirectionAboveHorizontal)
	{
		// Above horizontal: positive angle (Y negative in world coords)
		NewDir = FVector2D(FMath::Cos(AngleRad), -FMath::Sin(AngleRad));
	}
	else
	{
		// Below horizontal: negative angle (Y positive in world coords)
		NewDir = FVector2D(FMath::Cos(AngleRad), FMath::Sin(AngleRad));
	}
	
	CurrentEndPoint = StartPoint + NewDir * CurrentLength;
	LastSnappedWorldPos = FVector(CurrentEndPoint.X, CurrentEndPoint.Y, 0.0f);
	bNumericInputActive = true; // Prevent mouse from overriding
	UpdateCachedValues();
	
	UE_LOG(LogRTPlanLineTool, Verbose, TEXT("UpdatePreviewFromAngle: %.1f deg (above=%d) -> EndPoint: %s"), 
		AngleDegrees, bSavedDirectionAboveHorizontal ? 1 : 0, *CurrentEndPoint.ToString());
}

void URTPlanLineTool::UpdateCachedValues()
{
	FVector2D Delta = CurrentEndPoint - StartPoint;
	CachedLengthCm = Delta.Size();
	
	// Calculate angle: 0° = +X axis, counter-clockwise positive
	if (CachedLengthCm > 0.01f)
	{
		CachedAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		// Normalize to 0-360
		if (CachedAngleDegrees < 0.0f)
		{
			CachedAngleDegrees += 360.0f;
		}
	}
	else
	{
		CachedAngleDegrees = 0.0f;
	}
}

FVector2D URTPlanLineTool::ApplySoftOrthoSnap(const FVector2D& Start, const FVector2D& End)
{
	FVector2D Delta = End - Start;
	float Length = Delta.Size();
	
	if (Length < 0.1f)
	{
		bIsOrthoSnapped = false;
		return End;
	}
	
	// Calculate angle in degrees
	float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	
	// Normalize to 0-360
	if (AngleDeg < 0.0f)
	{
		AngleDeg += 360.0f;
	}
	
	// Check proximity to cardinal directions (0, 90, 180, 270)
	const float CardinalAngles[] = { 0.0f, 90.0f, 180.0f, 270.0f, 360.0f };
	
	for (float Cardinal : CardinalAngles)
	{
		float Diff = FMath::Abs(AngleDeg - Cardinal);
		if (Diff <= OrthoSnapThreshold || Diff >= (360.0f - OrthoSnapThreshold))
		{
			// Snap to this cardinal direction
			float SnapAngleRad = FMath::DegreesToRadians(Cardinal);
			FVector2D SnapDir(FMath::Cos(SnapAngleRad), FMath::Sin(SnapAngleRad));
			bIsOrthoSnapped = true;
			return Start + SnapDir * Length;
		}
	}
	
	// Not near any cardinal direction - allow free angle
	bIsOrthoSnapped = false;
	return End;
}

void URTPlanLineTool::OnPointerEvent(const FRTPointerEvent& Event)
{
	FVector GroundPoint;
	if (!GetGroundIntersection(Event, GroundPoint))
	{
		return;
	}

	FVector2D CursorPos(GroundPoint.X, GroundPoint.Y);
	FVector2D SnappedPos = CursorPos;

	// Only apply snapping if snap is enabled
	if (Event.bSnapEnabled)
	{
		// 1. Base Snap (Vertex/Grid)
		SnappedPos = GetSnappedPoint(GroundPoint);

		// 2. Soft Alignment Snap (if not snapped to vertex)
		if (SpatialIndex)
		{
			FRTSnapResult HardSnap = SpatialIndex->QuerySnap(CursorPos, 20.0f);
			if (!HardSnap.bValid)
			{
				FVector2D AlignedPos;
				if (SpatialIndex->QueryAlignment(CursorPos, 20.0f, AlignedPos))
				{
					SnappedPos = AlignedPos;
				}
			}
		}
	}

	if (State == EState::WaitingForStart)
	{
		CurrentEndPoint = SnappedPos;
		LastSnappedWorldPos = FVector(CurrentEndPoint.X, CurrentEndPoint.Y, 0);
		bIsOrthoSnapped = false;

		if (Event.Action == ERTPointerAction::Down)
		{
			StartPoint = SnappedPos;
			State = EState::WaitingForEnd;
			bShowPreview = true;
			UE_LOG(LogRTPlanLineTool, Log, TEXT("Start Point Set: %s"), *StartPoint.ToString());
		}
	}
	else if (State == EState::WaitingForEnd)
	{
		// If numeric input is active, mouse movement clears it so user can resume mouse control
		if (bNumericInputActive && Event.Action == ERTPointerAction::Move)
		{
			bNumericInputActive = false;
		}
		
		// Only update position from mouse if numeric input is not active
		if (!bNumericInputActive)
		{
			// 3. Apply Soft Ortho Snap only if snap is enabled
			FVector2D FinalEndPoint;
			if (Event.bSnapEnabled)
			{
				FinalEndPoint = ApplySoftOrthoSnap(StartPoint, SnappedPos);
			}
			else
			{
				// No snapping - use raw position
				FinalEndPoint = SnappedPos;
				bIsOrthoSnapped = false;
			}
			
			CurrentEndPoint = FinalEndPoint;
			LastSnappedWorldPos = FVector(CurrentEndPoint.X, CurrentEndPoint.Y, 0);
			
			// Update cached values for HUD
			UpdateCachedValues();
		}

		if (Event.Action == ERTPointerAction::Down)
		{
			// Clear numeric input flag on click
			bNumericInputActive = false;
			
			if (!StartPoint.Equals(CurrentEndPoint, 0.1f))
			{
				UE_LOG(LogRTPlanLineTool, Log, TEXT("End Point Set: %s (Length: %.1f, Angle: %.1f°). Committing Wall."), 
					*CurrentEndPoint.ToString(), CachedLengthCm, CachedAngleDegrees);

				FRTVertex V1; V1.Id = FGuid::NewGuid(); V1.Position = StartPoint;
				FRTVertex V2; V2.Id = FGuid::NewGuid(); V2.Position = CurrentEndPoint;
				FRTWall NewWall; NewWall.Id = FGuid::NewGuid(); NewWall.VertexAId = V1.Id; NewWall.VertexBId = V2.Id;
				NewWall.ThicknessCm = 20.0f; NewWall.HeightCm = 300.0f;

				URTCmdAddVertex* CmdV1 = NewObject<URTCmdAddVertex>(); CmdV1->Vertex = V1; Document->SubmitCommand(CmdV1);
				URTCmdAddVertex* CmdV2 = NewObject<URTCmdAddVertex>(); CmdV2->Vertex = V2; Document->SubmitCommand(CmdV2);
				URTCmdAddWall* CmdWall = NewObject<URTCmdAddWall>(); CmdWall->Wall = NewWall; Document->SubmitCommand(CmdWall);

				if (bIsPolyline)
				{
					StartPoint = CurrentEndPoint;
				}
				else
				{
					State = EState::WaitingForStart;
					bShowPreview = false;
				}
			}
		}
	}
}

void URTPlanLineTool::OnNumericInput(float Value)
{
	// Legacy method - treat as length input
	OnNumericInputWithField(Value, ERTNumericField::Length);
}

void URTPlanLineTool::OnNumericInputWithField(float Value, ERTNumericField Field)
{
	if (State != EState::WaitingForEnd)
	{
		return;
	}

	if (Field == ERTNumericField::Length)
	{
		// Length input - update end point keeping current direction
		if (Value < 0.1f)
		{
			return;
		}
		
		FVector2D Dir = (CurrentEndPoint - StartPoint).GetSafeNormal();
		if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);

		CurrentEndPoint = StartPoint + Dir * Value;
		
		UE_LOG(LogRTPlanLineTool, Log, TEXT("Numeric Length Input: %.1f cm. Committing at %s"), Value, *CurrentEndPoint.ToString());
	}
	else // ERTNumericField::Angle
	{
		// Angle input - update end point keeping current length
		// Clamp angle to 0-180
		float AngleDeg = FMath::Clamp(Value, 0.0f, 180.0f);
		
		float CurrentLength = (CurrentEndPoint - StartPoint).Size();
		if (CurrentLength < 0.1f)
		{
			CurrentLength = 100.0f; // Default length
		}
		
		// Use the saved direction (above or below horizontal)
		float AngleRad = FMath::DegreesToRadians(AngleDeg);
		FVector2D NewDir;
		if (bSavedDirectionAboveHorizontal)
		{
			// Above horizontal: positive angle (Y negative in world coords)
			NewDir = FVector2D(FMath::Cos(AngleRad), -FMath::Sin(AngleRad));
		}
		else
		{
			// Below horizontal: negative angle (Y positive in world coords)
			NewDir = FVector2D(FMath::Cos(AngleRad), FMath::Sin(AngleRad));
		}
		
		CurrentEndPoint = StartPoint + NewDir * CurrentLength;
		
		UE_LOG(LogRTPlanLineTool, Log, TEXT("Numeric Angle Input: %.1f deg (above=%d). Committing at %s"), 
			AngleDeg, bSavedDirectionAboveHorizontal ? 1 : 0, *CurrentEndPoint.ToString());
	}
	
	LastSnappedWorldPos = FVector(CurrentEndPoint.X, CurrentEndPoint.Y, 0);
	UpdateCachedValues();
	
	// Commit the wall
	FRTVertex V1; V1.Id = FGuid::NewGuid(); V1.Position = StartPoint;
	FRTVertex V2; V2.Id = FGuid::NewGuid(); V2.Position = CurrentEndPoint;
	FRTWall NewWall; NewWall.Id = FGuid::NewGuid(); NewWall.VertexAId = V1.Id; NewWall.VertexBId = V2.Id;
	NewWall.ThicknessCm = 20.0f; NewWall.HeightCm = 300.0f;

	URTCmdAddVertex* CmdV1 = NewObject<URTCmdAddVertex>(); CmdV1->Vertex = V1; Document->SubmitCommand(CmdV1);
	URTCmdAddVertex* CmdV2 = NewObject<URTCmdAddVertex>(); CmdV2->Vertex = V2; Document->SubmitCommand(CmdV2);
	URTCmdAddWall* CmdWall = NewObject<URTCmdAddWall>(); CmdWall->Wall = NewWall; Document->SubmitCommand(CmdWall);

	if (bIsPolyline)
	{
		StartPoint = CurrentEndPoint;
	}
	else
	{
		State = EState::WaitingForStart;
		bShowPreview = false;
	}
}

