#include "Tools/RTPlanLineTool.h"
#include "RTPlanCommand.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTPlanLineTool, Log, All);

void URTPlanLineTool::OnEnter()
{
	State = EState::WaitingForStart;
	bShowPreview = false;
	UE_LOG(LogRTPlanLineTool, Log, TEXT("Line Tool Activated"));
}

void URTPlanLineTool::OnExit()
{
	bShowPreview = false;
	UE_LOG(LogRTPlanLineTool, Log, TEXT("Line Tool Deactivated"));
}

void URTPlanLineTool::SetPolylineMode(bool bEnable)
{
	bIsPolyline = bEnable;
}

void URTPlanLineTool::OnPointerEvent(const FRTPointerEvent& Event)
{
	FVector GroundPoint;
	if (!GetGroundIntersection(Event, GroundPoint))
	{
		return;
	}

	// 1. Base Snap (Vertex/Grid)
	FVector2D SnappedPos = GetSnappedPoint(GroundPoint);

	// 2. Soft Alignment Snap (if not snapped to vertex)
	if (SpatialIndex)
	{
		FRTSnapResult HardSnap = SpatialIndex->QuerySnap(FVector2D(GroundPoint.X, GroundPoint.Y), 20.0f);
		if (!HardSnap.bValid)
		{
			FVector2D AlignedPos;
			if (SpatialIndex->QueryAlignment(FVector2D(GroundPoint.X, GroundPoint.Y), 20.0f, AlignedPos))
			{
				SnappedPos = AlignedPos;
			}
		}
	}

	if (State == EState::WaitingForStart)
	{
		CurrentEndPoint = SnappedPos;
		LastSnappedWorldPos = FVector(CurrentEndPoint.X, CurrentEndPoint.Y, 0); // Update for HUD

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
		// 3. Apply Ortho Lock
		FVector2D OrthoPos = ApplyOrthoLock(StartPoint, SnappedPos);
		CurrentEndPoint = OrthoPos;
		LastSnappedWorldPos = FVector(CurrentEndPoint.X, CurrentEndPoint.Y, 0); // Update for HUD

		if (Event.Action == ERTPointerAction::Down)
		{
			if (!StartPoint.Equals(CurrentEndPoint, 0.1f))
			{
				UE_LOG(LogRTPlanLineTool, Log, TEXT("End Point Set: %s. Committing Wall."), *CurrentEndPoint.ToString());

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
	if (State == EState::WaitingForEnd && Value > 0.1f)
	{
		FVector2D Dir = (CurrentEndPoint - StartPoint).GetSafeNormal();
		if (Dir.IsNearlyZero()) Dir = FVector2D(1, 0);

		CurrentEndPoint = StartPoint + Dir * Value;
		LastSnappedWorldPos = FVector(CurrentEndPoint.X, CurrentEndPoint.Y, 0); // Update for HUD
		
		UE_LOG(LogRTPlanLineTool, Log, TEXT("Numeric Input: %f. Committing at %s"), Value, *CurrentEndPoint.ToString());
		
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

FVector2D URTPlanLineTool::ApplyOrthoLock(const FVector2D& Start, const FVector2D& End) const
{
	FVector2D Diff = End - Start;
	if (FMath::Abs(Diff.X) > FMath::Abs(Diff.Y))
	{
		return FVector2D(End.X, Start.Y);
	}
	else
	{
		return FVector2D(Start.X, End.Y);
	}
}
