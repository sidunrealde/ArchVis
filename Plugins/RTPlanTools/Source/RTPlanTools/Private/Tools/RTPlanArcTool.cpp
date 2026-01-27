#include "Tools/RTPlanArcTool.h"
#include "RTPlanCommand.h"
#include "RTPlanGeometryUtils.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTPlanArcTool, Log, All);

void URTPlanArcTool::OnEnter()
{
	State = EState::WaitingForStart;
	bShowPreview = false;
	bIsAngleSnapped = false;
	CachedRadius = 0.0f;
	CachedAngleDegrees = 0.0f;
	Radius = 0.0f;
	SweepAngleDegrees = 0.0f;
	UE_LOG(LogRTPlanArcTool, Log, TEXT("Arc Tool Activated (3-Point Arc: Start -> Second -> End)"));
}

void URTPlanArcTool::OnExit()
{
	bShowPreview = false;
	bIsAngleSnapped = false;
	UE_LOG(LogRTPlanArcTool, Log, TEXT("Arc Tool Deactivated"));
}

FRTDraftingState URTPlanArcTool::GetDraftingState() const
{
	FRTDraftingState DraftState;
	DraftState.bIsActive = (State != EState::WaitingForStart);
	
	if (State == EState::WaitingForSecondPoint)
	{
		// Picking the second point (point on arc)
		// Show standard line drafting from Start to cursor
		DraftState.StartPoint = StartPoint;
		DraftState.EndPoint = CurrentPoint;
		DraftState.LengthCm = (CurrentPoint - StartPoint).Size();
		DraftState.AngleDegrees = FRTPlanGeometryUtils::GetAngleDegrees(StartPoint, CurrentPoint);
		// No arc preview yet - we need 3 points to define an arc
	}
	else if (State == EState::WaitingForEnd)
	{
		// Picking the end point - we have Start and SecondPoint, cursor is the end
		// Show arc preview through all three points
		DraftState.bIsArc = true;
		DraftState.ArcCenter = CenterPoint;
		DraftState.ArcRadius = Radius;
		
		// Pass the sweep angle - this is the SINGLE SOURCE OF TRUTH
		// StartAngle is calculated from center to StartPoint
		// EndAngle = StartAngle + SweepAngle
		FVector2D ToStart = StartPoint - CenterPoint;
		float StartAngle = FMath::RadiansToDegrees(FMath::Atan2(ToStart.Y, ToStart.X));
		DraftState.ArcStartAngle = StartAngle;
		DraftState.ArcEndAngle = StartAngle + SweepAngleDegrees;
		
		// Store points for visualization
		DraftState.StartPoint = StartPoint;
		DraftState.EndPoint = SecondPoint;  // Second point (clicked)
		DraftState.ArcThirdPoint = CurrentPoint;  // End point (cursor)
		
		// Length/Angle for the line from SecondPoint to cursor (for numeric input display)
		DraftState.LengthCm = (CurrentPoint - SecondPoint).Size();
		DraftState.AngleDegrees = FRTPlanGeometryUtils::GetAngleDegrees(SecondPoint, CurrentPoint);
	}
	
	DraftState.bOrthoSnapped = false;
	return DraftState;
}

float URTPlanArcTool::GetCurrentRadius() const
{
	return CachedRadius;
}

float URTPlanArcTool::GetCurrentAngleDegrees() const
{
	return CachedAngleDegrees;
}

void URTPlanArcTool::UpdatePreviewFromLength(float LengthCm)
{
	if (LengthCm < 0.1f) return;
	
	if (State == EState::WaitingForSecondPoint)
	{
		FVector2D Dir = (CurrentPoint - StartPoint).GetSafeNormal();
		if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);
		
		CurrentPoint = StartPoint + Dir * LengthCm;
		LastSnappedWorldPos = FVector(CurrentPoint.X, CurrentPoint.Y, 0);
		bNumericInputActive = true;
		UpdateCachedValues();
	}
	else if (State == EState::WaitingForEnd)
	{
		FVector2D Dir = (CurrentPoint - SecondPoint).GetSafeNormal();
		if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);
		
		CurrentPoint = SecondPoint + Dir * LengthCm;
		LastSnappedWorldPos = FVector(CurrentPoint.X, CurrentPoint.Y, 0);
		bNumericInputActive = true;
		
		CalculateArcFrom3Points(StartPoint, SecondPoint, CurrentPoint);
		UpdateCachedValues();
	}
}

void URTPlanArcTool::UpdatePreviewFromAngle(float AngleDegrees)
{
	float AngleDeg = FMath::Clamp(AngleDegrees, -180.0f, 180.0f);
	
	if (State == EState::WaitingForSecondPoint)
	{
		float CurrentLength = (CurrentPoint - StartPoint).Size();
		if (CurrentLength < 0.1f) CurrentLength = 100.0f;
		
		float AngleRad = FMath::DegreesToRadians(AngleDeg);
		FVector2D Dir(FMath::Cos(AngleRad), FMath::Sin(AngleRad));
		
		CurrentPoint = StartPoint + Dir * CurrentLength;
		LastSnappedWorldPos = FVector(CurrentPoint.X, CurrentPoint.Y, 0);
		bNumericInputActive = true;
		UpdateCachedValues();
	}
	else if (State == EState::WaitingForEnd)
	{
		float CurrentLength = (CurrentPoint - SecondPoint).Size();
		if (CurrentLength < 0.1f) CurrentLength = 100.0f;
		
		float AngleRad = FMath::DegreesToRadians(AngleDeg);
		FVector2D Dir(FMath::Cos(AngleRad), FMath::Sin(AngleRad));
		
		CurrentPoint = SecondPoint + Dir * CurrentLength;
		LastSnappedWorldPos = FVector(CurrentPoint.X, CurrentPoint.Y, 0);
		bNumericInputActive = true;
		
		CalculateArcFrom3Points(StartPoint, SecondPoint, CurrentPoint);
		UpdateCachedValues();
	}
}

void URTPlanArcTool::OnPointerEvent(const FRTPointerEvent& Event)
{
	if (Event.bAltDown || Event.bCtrlDown)
	{
		return;
	}

	FVector GroundPoint;
	if (!GetGroundIntersection(Event, GroundPoint))
	{
		if (Event.Action == ERTPointerAction::Move)
		{
			LastSnappedWorldPos = Event.WorldOrigin + Event.WorldDirection * 100000.0f;
		}
		return;
	}

	FVector2D CursorPos(GroundPoint.X, GroundPoint.Y);
	FVector2D SnappedPos = CursorPos;

	if (Event.bSnapEnabled)
	{
		SnappedPos = GetSnappedPoint(GroundPoint);
		
		if (SpatialIndex)
		{
			FRTSnapResult HardSnap = SpatialIndex->QuerySnap(CursorPos, 20.0f);
			if (HardSnap.bValid)
			{
				SnappedPos = HardSnap.Location;
			}
			else
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
		CurrentPoint = SnappedPos;
		LastSnappedWorldPos = FVector(CurrentPoint.X, CurrentPoint.Y, 0);
		
		if (Event.Action == ERTPointerAction::Down)
		{
			StartPoint = SnappedPos;
			State = EState::WaitingForSecondPoint;
			bShowPreview = true;
			UE_LOG(LogRTPlanArcTool, Log, TEXT("Start Point Set: %s"), *StartPoint.ToString());
		}
	}
	else if (State == EState::WaitingForSecondPoint)
	{
		CurrentPoint = SnappedPos;
		LastSnappedWorldPos = FVector(CurrentPoint.X, CurrentPoint.Y, 0);
		UpdateCachedValues();

		if (Event.Action == ERTPointerAction::Down)
		{
			if (!StartPoint.Equals(CurrentPoint, 0.1f))
			{
				SecondPoint = CurrentPoint;
				State = EState::WaitingForEnd;
				UE_LOG(LogRTPlanArcTool, Log, TEXT("Second Point (on arc) Set: %s"), *SecondPoint.ToString());
			}
		}
	}
	else if (State == EState::WaitingForEnd)
	{
		CurrentPoint = SnappedPos;
		LastSnappedWorldPos = FVector(CurrentPoint.X, CurrentPoint.Y, 0);
		
		CalculateArcFrom3Points(StartPoint, SecondPoint, CurrentPoint);
		UpdateCachedValues();

		if (Event.Action == ERTPointerAction::Down)
		{
			if (Radius > 0.1f && !SecondPoint.Equals(CurrentPoint, 0.1f))
			{
				EndPoint = CurrentPoint;
				CommitArc();
			}
		}
	}
}

void URTPlanArcTool::CalculateArcFrom3Points(const FVector2D& P1, const FVector2D& P2, const FVector2D& P3)
{
	// P1 = Start, P2 = Second Point (on arc), P3 = End Point
	// Find the circle center that passes through all three points
	// Then determine the sweep direction based on P2's position
	
	// Find perpendicular bisectors of P1-P2 and P2-P3
	FVector2D Mid12 = (P1 + P2) * 0.5f;
	FVector2D Mid23 = (P2 + P3) * 0.5f;
	
	FVector2D Dir12 = P2 - P1;
	FVector2D Dir23 = P3 - P2;
	
	// Perpendicular directions (rotate 90 degrees CCW)
	FVector2D Perp12(-Dir12.Y, Dir12.X);
	FVector2D Perp23(-Dir23.Y, Dir23.X);
	
	// Find intersection of perpendicular bisectors = circle center
	FVector2D Center;
	if (!FRTPlanGeometryUtils::SegmentIntersection(
		Mid12 - Perp12 * 100000.0f, Mid12 + Perp12 * 100000.0f,
		Mid23 - Perp23 * 100000.0f, Mid23 + Perp23 * 100000.0f,
		Center))
	{
		// Collinear points - no valid arc
		Radius = 0.0f;
		SweepAngleDegrees = 0.0f;
		return;
	}
	
	CenterPoint = Center;
	Radius = FVector2D::Distance(Center, P1);
	
	// Calculate angles from center to each point (in standard math coords: CCW from +X)
	FVector2D ToP1 = P1 - Center;
	FVector2D ToP2 = P2 - Center;
	FVector2D ToP3 = P3 - Center;
	
	float Angle1 = FMath::Atan2(ToP1.Y, ToP1.X);  // Radians
	float Angle2 = FMath::Atan2(ToP2.Y, ToP2.X);
	float Angle3 = FMath::Atan2(ToP3.Y, ToP3.X);
	
	// Normalize all angles to [0, 2π)
	auto NormalizeAngle = [](float A) {
		while (A < 0) A += 2.0f * PI;
		while (A >= 2.0f * PI) A -= 2.0f * PI;
		return A;
	};
	
	float A1 = NormalizeAngle(Angle1);
	float A2 = NormalizeAngle(Angle2);
	float A3 = NormalizeAngle(Angle3);
	
	// Calculate CCW sweep from A1 to A3
	float SweepCCW = A3 - A1;
	if (SweepCCW < 0) SweepCCW += 2.0f * PI;
	
	// Calculate CW sweep from A1 to A3 (negative direction)
	float SweepCW = SweepCCW - 2.0f * PI;  // This will be negative
	
	// Check where A2 falls relative to A1
	float A2FromA1 = A2 - A1;
	if (A2FromA1 < 0) A2FromA1 += 2.0f * PI;
	
	// If A2 is within the CCW sweep from A1 to A3, go CCW
	// Otherwise, go CW
	float FinalSweepRad;
	if (A2FromA1 <= SweepCCW + 0.001f)  // Small epsilon for floating point
	{
		// P2 is on the CCW path from P1 to P3
		FinalSweepRad = SweepCCW;
	}
	else
	{
		// P2 is on the CW path from P1 to P3
		FinalSweepRad = SweepCW;
	}
	
	SweepAngleDegrees = FMath::RadiansToDegrees(FinalSweepRad);
	
	// Clamp to prevent arcs > 360 degrees
	SweepAngleDegrees = FMath::Clamp(SweepAngleDegrees, -359.9f, 359.9f);
	
	UE_LOG(LogRTPlanArcTool, Verbose, TEXT("Arc Calc: Center=%s, Radius=%.1f, Sweep=%.1f deg"), 
		*CenterPoint.ToString(), Radius, SweepAngleDegrees);
}

void URTPlanArcTool::OnNumericInput(float Value)
{
	OnNumericInputWithField(Value, ERTNumericField::Length);
}

void URTPlanArcTool::OnNumericInputWithField(float Value, ERTNumericField Field)
{
	if (State == EState::WaitingForSecondPoint)
	{
		if (Field == ERTNumericField::Length)
		{
			if (Value < 0.1f) return;
			
			FVector2D Dir = (CurrentPoint - StartPoint).GetSafeNormal();
			if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);
			
			SecondPoint = StartPoint + Dir * Value;
			CurrentPoint = SecondPoint;
		}
		else // Angle
		{
			float CurrentLength = (CurrentPoint - StartPoint).Size();
			if (CurrentLength < 0.1f) CurrentLength = 100.0f;
			
			float AngleRad = FMath::DegreesToRadians(Value);
			FVector2D Dir(FMath::Cos(AngleRad), FMath::Sin(AngleRad));
			
			SecondPoint = StartPoint + Dir * CurrentLength;
			CurrentPoint = SecondPoint;
		}
		
		State = EState::WaitingForEnd;
		LastSnappedWorldPos = FVector(SecondPoint.X, SecondPoint.Y, 0);
		UE_LOG(LogRTPlanArcTool, Log, TEXT("Second Point Set via numeric input: %s"), *SecondPoint.ToString());
	}
	else if (State == EState::WaitingForEnd)
	{
		if (Field == ERTNumericField::Length)
		{
			if (Value < 0.1f) return;
			
			FVector2D Dir = (CurrentPoint - SecondPoint).GetSafeNormal();
			if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);
			
			EndPoint = SecondPoint + Dir * Value;
			CurrentPoint = EndPoint;
		}
		else // Angle
		{
			float CurrentLength = (CurrentPoint - SecondPoint).Size();
			if (CurrentLength < 0.1f) CurrentLength = 100.0f;
			
			float AngleRad = FMath::DegreesToRadians(Value);
			FVector2D Dir(FMath::Cos(AngleRad), FMath::Sin(AngleRad));
			
			EndPoint = SecondPoint + Dir * CurrentLength;
			CurrentPoint = EndPoint;
		}
		
		CalculateArcFrom3Points(StartPoint, SecondPoint, EndPoint);
		UpdateCachedValues();
		LastSnappedWorldPos = FVector(EndPoint.X, EndPoint.Y, 0);
		
		if (Radius > 0.1f)
		{
			CommitArc();
		}
	}
}

void URTPlanArcTool::UpdateCachedValues()
{
	if (State == EState::WaitingForSecondPoint)
	{
		CachedRadius = (CurrentPoint - StartPoint).Size();
		CachedAngleDegrees = FRTPlanGeometryUtils::GetAngleDegrees(StartPoint, CurrentPoint);
	}
	else if (State == EState::WaitingForEnd)
	{
		CachedRadius = Radius;
		CachedAngleDegrees = FMath::Abs(SweepAngleDegrees);
	}
}

void URTPlanArcTool::CommitArc()
{
	if (Radius < 0.1f) return;

	UE_LOG(LogRTPlanArcTool, Log, TEXT("Committing Arc: Start=%s, End=%s, Center=%s, Radius=%.1f, Sweep=%.1f"), 
		*StartPoint.ToString(), *EndPoint.ToString(), *CenterPoint.ToString(), Radius, SweepAngleDegrees);

	// Create start vertex
	FRTVertex V1;
	V1.Id = FGuid::NewGuid();
	V1.Position = StartPoint;
	URTCmdAddVertex* CmdV1 = NewObject<URTCmdAddVertex>();
	CmdV1->Vertex = V1;
	Document->SubmitCommand(CmdV1);

	// Create end vertex
	FRTVertex V2;
	V2.Id = FGuid::NewGuid();
	V2.Position = EndPoint;
	URTCmdAddVertex* CmdV2 = NewObject<URTCmdAddVertex>();
	CmdV2->Vertex = V2;
	Document->SubmitCommand(CmdV2);

	// Create a single curved wall
	FRTWall ArcWall;
	ArcWall.Id = FGuid::NewGuid();
	ArcWall.VertexAId = V1.Id;
	ArcWall.VertexBId = V2.Id;
	ArcWall.ThicknessCm = 20.0f;
	ArcWall.HeightCm = 300.0f;
	ArcWall.bIsArc = true;
	ArcWall.ArcCenter = CenterPoint;
	ArcWall.ArcSweepAngle = SweepAngleDegrees;  // Use the same sweep angle as visualization

	URTCmdAddWall* CmdWall = NewObject<URTCmdAddWall>();
	CmdWall->Wall = ArcWall;
	Document->SubmitCommand(CmdWall);

	// Reset tool
	State = EState::WaitingForStart;
	bShowPreview = false;
	bNumericInputActive = false;
}

void URTPlanArcTool::CancelDrawing()
{
	State = EState::WaitingForStart;
	bShowPreview = false;
	bNumericInputActive = false;
	UE_LOG(LogRTPlanArcTool, Log, TEXT("Arc drawing cancelled"));
}
