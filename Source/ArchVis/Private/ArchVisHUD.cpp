#include "ArchVisHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "ArchVisGameMode.h"
#include "RTPlanToolManager.h"
#include "RTPlanToolBase.h"
#include "ArchVisPlayerController.h"
#include "Kismet/GameplayStatics.h"

void AArchVisHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	FVector2D CrosshairPos = FVector2D::ZeroVector;
	bool bShouldDraw = false;
	FRTDraftingState DraftState;
	FRTNumericInputBuffer InputBuffer;

	AArchVisPlayerController* PC = Cast<AArchVisPlayerController>(GetOwningPlayerController());

	// 1. Try to get Snapped Position and Drafting State from Active Tool
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanToolBase* Tool = ToolMgr->GetActiveTool())
			{
				FVector SnappedWorld = Tool->GetSnappedCursorPos();
				FVector ScreenPos = Project(SnappedWorld);
				CrosshairPos = FVector2D(ScreenPos.X, ScreenPos.Y);
				bShouldDraw = true;

				// Get drafting state from any tool (base class virtual method)
				DraftState = Tool->GetDraftingState();
			}
		}
	}

	// Get numeric input buffer from player controller
	if (PC)
	{
		InputBuffer = PC->GetNumericInputBuffer();
	}

	// 2. Fallback to Virtual Cursor Position
	if (!bShouldDraw && PC)
	{
		CrosshairPos = PC->GetVirtualCursorPos();
		bShouldDraw = true;
	}

	if (bShouldDraw)
	{
		// Draw Full Screen Crosshair
		float ScreenW = Canvas->ClipX;
		float ScreenH = Canvas->ClipY;

		// Horizontal Line
		DrawLine(0, CrosshairPos.Y, ScreenW, CrosshairPos.Y, CrosshairColor, CrosshairThickness);

		// Vertical Line
		DrawLine(CrosshairPos.X, 0, CrosshairPos.X, ScreenH, CrosshairColor, CrosshairThickness);

		// Draw CAD-style visualization if drafting is active
		if (DraftState.bIsActive)
		{
			if (DraftState.bIsArc)
			{
				DrawArcDraftingVisualization(DraftState, InputBuffer);
			}
			else
			{
				DrawDraftingVisualization(DraftState, InputBuffer);
			}
		}
		else if (InputBuffer.bIsActive)
		{
			// Still show numeric buffer even if not in drafting mode
			FString BufferText = InputBuffer.Buffer + TEXT(" ") + InputBuffer.GetUnitSuffix();
			FVector2D InputPos = CrosshairPos + InputBufferOffset;
			DrawLabelWithBackground(BufferText, InputPos, InputBufferColor, ActiveLabelBackgroundColor);
		}
	}
}

void AArchVisHUD::DrawDraftingVisualization(const FRTDraftingState& DraftState, const FRTNumericInputBuffer& InputBuffer)
{
	// Project start and end points to screen space
	FVector StartWorld(DraftState.StartPoint.X, DraftState.StartPoint.Y, 0.0f);
	FVector EndWorld(DraftState.EndPoint.X, DraftState.EndPoint.Y, 0.0f);

	FVector StartScreen = Project(StartWorld);
	FVector EndScreen = Project(EndWorld);

	FVector2D Start2D(StartScreen.X, StartScreen.Y);
	FVector2D End2D(EndScreen.X, EndScreen.Y);

	// 1. Draw SOLID wall preview line
	DrawLine(Start2D.X, Start2D.Y, End2D.X, End2D.Y, WallPreviewColor, WallPreviewThickness);

	// Calculate the wall direction and angle first (needed for measurement line positioning)
	FVector2D LineDir = (End2D - Start2D).GetSafeNormal();
	FVector2D ToEnd = (End2D - Start2D).GetSafeNormal();
	float AngleToEnd = FMath::RadiansToDegrees(FMath::Atan2(-ToEnd.Y, ToEnd.X));
	
	// 2. Draw DASHED measurement line (parallel to wall, offset to opposite side of angle arc)
	// The angle arc is between horizontal (0°) and the wall direction
	// So we put the measurement line on the opposite side
	FVector2D Perpendicular(-LineDir.Y, LineDir.X);
	
	// Flip perpendicular direction based on wall angle
	// If angle is positive (wall going up in world space, which is negative Y in screen space),
	// the arc is above the wall, so put measurement line below (negative perpendicular)
	// If angle is negative or > 90, flip accordingly
	float MeasurementOffset = 20.0f;
	if (AngleToEnd > 0.0f && AngleToEnd < 180.0f)
	{
		// Wall is going upward (in world coords), arc is above - put measurement below
		MeasurementOffset = -MeasurementOffset;
	}
	// else: Wall is going downward or horizontal right, arc is below - measurement stays above
	
	FVector2D MeasureStart = Start2D + Perpendicular * MeasurementOffset;
	FVector2D MeasureEnd = End2D + Perpendicular * MeasurementOffset;
	
	// Draw the dashed measurement line
	DrawDashedLine(MeasureStart, MeasureEnd, MeasurementLineColor, MeasurementLineThickness, DashLength, GapLength);
	
	// Draw small perpendicular ticks at ends of measurement line
	float TickHalfLength = 5.0f;
	float TickSign = (MeasurementOffset > 0.0f) ? 1.0f : -1.0f;
	FVector2D TickStart1 = Start2D + Perpendicular * (MeasurementOffset - TickHalfLength * TickSign);
	FVector2D TickEnd1 = Start2D + Perpendicular * (MeasurementOffset + TickHalfLength * TickSign);
	DrawLine(TickStart1.X, TickStart1.Y, TickEnd1.X, TickEnd1.Y, MeasurementLineColor, MeasurementLineThickness);
	
	FVector2D TickStart2 = End2D + Perpendicular * (MeasurementOffset - TickHalfLength * TickSign);
	FVector2D TickEnd2 = End2D + Perpendicular * (MeasurementOffset + TickHalfLength * TickSign);
	DrawLine(TickStart2.X, TickStart2.Y, TickEnd2.X, TickEnd2.Y, MeasurementLineColor, MeasurementLineThickness);

	// 3. Draw DASHED angle visualization
	// Calculate arc radius first (same as wall length)
	float ArcRadius = (End2D - Start2D).Size();
	
	// HorizontalEnd is extended to match the arc radius
	FVector2D HorizontalEnd = Start2D + FVector2D(ArcRadius, 0.0f);
	
	// Draw horizontal reference line (dashed) from START point to HorizontalEnd
	DrawDashedLine(Start2D, HorizontalEnd, MeasurementLineColor, MeasurementLineThickness, DashLength, GapLength);
	
	// Calculate the angle from Start2D to HorizontalEnd (should be 0° for rightward)
	FVector2D ToHorizontalEnd = (HorizontalEnd - Start2D).GetSafeNormal();
	float AngleToHorizontal = FMath::RadiansToDegrees(FMath::Atan2(-ToHorizontalEnd.Y, ToHorizontalEnd.X));
	
	// AngleToEnd was already calculated earlier for measurement line positioning
	
	// Draw arc from horizontal direction to wall end direction, centered at Start2D
	float ArcStart = FMath::Min(AngleToHorizontal, AngleToEnd);
	float ArcEnd = FMath::Max(AngleToHorizontal, AngleToEnd);
	
	DrawDashedArc(Start2D, ArcRadius, ArcStart, ArcEnd, MeasurementLineColor, MeasurementLineThickness, DashLength, GapLength);

	// 4. Draw LENGTH label on the measurement line
	FVector2D MeasureMidPoint = (MeasureStart + MeasureEnd) * 0.5f;
	// Adjust label offset direction based on which side the measurement line is on
	float LabelOffset = (MeasurementOffset > 0.0f) ? LengthLabelOffsetFromLine : -LengthLabelOffsetFromLine;
	FVector2D LengthLabelPos = MeasureMidPoint + Perpendicular * LabelOffset;

	// Format length value WITH units
	float DisplayLength = InputBuffer.GetDisplayValue(DraftState.LengthCm);
	FString LengthText;
	bool bLengthIsActive = (InputBuffer.ActiveField == ERTNumericField::Length);
	
	if (InputBuffer.bIsActive && bLengthIsActive && !InputBuffer.Buffer.IsEmpty())
	{
		// Active field with user typing - show the buffer
		LengthText = InputBuffer.Buffer + TEXT(" ") + InputBuffer.GetUnitSuffix();
	}
	else if (!bLengthIsActive && InputBuffer.bHasSavedLength)
	{
		// Inactive field with saved value - show the saved value
		float SavedDisplayLength = InputBuffer.GetDisplayValue(InputBuffer.SavedLengthCm);
		LengthText = FString::Printf(TEXT("%.2f %s"), SavedDisplayLength, *InputBuffer.GetUnitSuffix());
	}
	else
	{
		// Show calculated length with units
		LengthText = FString::Printf(TEXT("%.2f %s"), DisplayLength, *InputBuffer.GetUnitSuffix());
	}

	// Use active color if length field is active, otherwise inactive
	FLinearColor LengthBgColor = bLengthIsActive ? ActiveLabelBackgroundColor : InactiveLabelBackgroundColor;
	DrawLabelWithBackground(LengthText, LengthLabelPos, LabelTextColor, LengthBgColor);

	// 5. Draw ANGLE label near the arc
	// The arc is centered at Start2D, so position label at the midpoint of the arc
	float LabelRadius = ArcRadius + AngleLabelOffsetFromArc + 15.0f;
	// Calculate midpoint angle of the arc
	float MidAngle = (ArcStart + ArcEnd) * 0.5f;
	float MidAngleRad = FMath::DegreesToRadians(MidAngle);
	FVector2D AngleLabelPos = Start2D + FVector2D(
		FMath::Cos(MidAngleRad) * LabelRadius,
		-FMath::Sin(MidAngleRad) * LabelRadius
	);
	
	bool bAngleIsActive = (InputBuffer.ActiveField == ERTNumericField::Angle);
	FString AngleText;
	
	// Display angle as 0-180 (angle from horizontal)
	float DisplayAngle = FMath::Abs(DraftState.AngleDegrees);
	if (DisplayAngle > 180.0f)
	{
		DisplayAngle = 360.0f - DisplayAngle;
	}
	
	if (InputBuffer.bIsActive && bAngleIsActive && !InputBuffer.Buffer.IsEmpty())
	{
		// Active field with user typing - show the buffer
		AngleText = InputBuffer.Buffer + TEXT("\u00B0");
	}
	else if (!bAngleIsActive && InputBuffer.bHasSavedAngle)
	{
		// Inactive field with saved value - show the saved value
		AngleText = FString::Printf(TEXT("%.2f\u00B0"), InputBuffer.SavedAngleDegrees);
	}
	else
	{
		// Show calculated angle (0-180 from horizontal)
		AngleText = FString::Printf(TEXT("%.2f\u00B0"), DisplayAngle);
	}

	// Use active color if angle field is active, otherwise inactive
	FLinearColor AngleBgColor = bAngleIsActive ? ActiveLabelBackgroundColor : InactiveLabelBackgroundColor;
	DrawLabelWithBackground(AngleText, AngleLabelPos, LabelTextColor, AngleBgColor);
}

void AArchVisHUD::DrawDashedLine(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color, float Thickness, float DashLen, float GapLen)
{
	FVector2D Direction = End - Start;
	float TotalLength = Direction.Size();

	if (TotalLength < 1.0f)
	{
		return;
	}

	Direction.Normalize();
	float CurrentPos = 0.0f;
	bool bDrawing = true;

	while (CurrentPos < TotalLength)
	{
		if (bDrawing)
		{
			float SegmentEnd = FMath::Min(CurrentPos + DashLen, TotalLength);
			FVector2D SegStart = Start + Direction * CurrentPos;
			FVector2D SegEnd = Start + Direction * SegmentEnd;
			DrawLine(SegStart.X, SegStart.Y, SegEnd.X, SegEnd.Y, Color, Thickness);
			CurrentPos = SegmentEnd;
		}
		else
		{
			CurrentPos += GapLen;
		}
		bDrawing = !bDrawing;
	}
}

void AArchVisHUD::DrawDashedArc(const FVector2D& Center, float Radius, float StartAngleDeg, float EndAngleDeg, const FLinearColor& Color, float Thickness, float DashLen, float GapLen)
{
	// Calculate arc sweep - handles both CW and CCW directions
	float SweepAngle = EndAngleDeg - StartAngleDeg;
	
	// Normalize sweep to handle wrap-around
	// If sweep is very large, it's likely meant to go the short way
	if (SweepAngle > 180.0f) SweepAngle -= 360.0f;
	if (SweepAngle < -180.0f) SweepAngle += 360.0f;
	
	float AngleSpanDeg = FMath::Abs(SweepAngle);
	if (AngleSpanDeg < 0.1f)
	{
		return;
	}

	float ArcLength = (AngleSpanDeg / 360.0f) * 2.0f * PI * Radius;
	float PatternLength = DashLen + GapLen;
	int32 NumPatterns = FMath::Max(1, FMath::FloorToInt(ArcLength / PatternLength));
	
	float AnglePerPattern = AngleSpanDeg / (float)NumPatterns;
	float DashAngle = AnglePerPattern * (DashLen / PatternLength);
	
	// Direction of sweep (+1 for CCW, -1 for CW)
	float Direction = (SweepAngle >= 0.0f) ? 1.0f : -1.0f;
	
	float CurrentAngle = StartAngleDeg;
	bool bDrawing = true;
	float AngleProgress = 0.0f;
	
	for (int32 i = 0; i < NumPatterns * 2 && AngleProgress < AngleSpanDeg; ++i)
	{
		if (bDrawing)
		{
			float RemainingAngle = AngleSpanDeg - AngleProgress;
			float SegmentAngle = FMath::Min(DashAngle, RemainingAngle);
			float SegmentEndAngle = CurrentAngle + SegmentAngle * Direction;
			
			// Draw arc segment
			int32 SegmentSteps = FMath::Max(2, AngleArcSegments / NumPatterns);
			float StepAngle = SegmentAngle / (float)SegmentSteps;
			
			for (int32 j = 0; j < SegmentSteps; ++j)
			{
				float A1 = FMath::DegreesToRadians(CurrentAngle + StepAngle * j * Direction);
				float A2 = FMath::DegreesToRadians(CurrentAngle + StepAngle * (j + 1) * Direction);
				
				// Screen Y is inverted, so negate sin
				FVector2D P1 = Center + FVector2D(FMath::Cos(A1) * Radius, -FMath::Sin(A1) * Radius);
				FVector2D P2 = Center + FVector2D(FMath::Cos(A2) * Radius, -FMath::Sin(A2) * Radius);
				
				DrawLine(P1.X, P1.Y, P2.X, P2.Y, Color, Thickness);
			}
			
			CurrentAngle = SegmentEndAngle;
			AngleProgress += SegmentAngle;
		}
		else
		{
			float GapAngle = AnglePerPattern - DashAngle;
			CurrentAngle += GapAngle * Direction;
			AngleProgress += GapAngle;
		}
		bDrawing = !bDrawing;
	}
}

void AArchVisHUD::DrawLabelWithBackground(const FString& Text, const FVector2D& Position, const FLinearColor& TextColor, const FLinearColor& BackgroundColor)
{
	if (Text.IsEmpty() || !GEngine || !GEngine->GetSmallFont())
	{
		return;
	}

	UFont* Font = GEngine->GetSmallFont();
	float TextWidth, TextHeight;
	GetTextSize(Text, TextWidth, TextHeight, Font);

	// Draw background rectangle
	float BoxX = Position.X - LabelPadding.X;
	float BoxY = Position.Y - LabelPadding.Y;
	float BoxW = TextWidth + LabelPadding.X * 2.0f;
	float BoxH = TextHeight + LabelPadding.Y * 2.0f;

	FCanvasTileItem BackgroundTile(
		FVector2D(BoxX, BoxY),
		FVector2D(BoxW, BoxH),
		BackgroundColor
	);
	BackgroundTile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BackgroundTile);

	// Draw text
	DrawText(Text, TextColor, Position.X, Position.Y, Font);
}

void AArchVisHUD::DrawArcDraftingVisualization(const FRTDraftingState& DraftState, const FRTNumericInputBuffer& InputBuffer)
{
	// Project all points to screen space
	FVector StartWorld(DraftState.StartPoint.X, DraftState.StartPoint.Y, 0.0f);
	FVector SecondWorld(DraftState.EndPoint.X, DraftState.EndPoint.Y, 0.0f);
	FVector EndWorld(DraftState.ArcThirdPoint.X, DraftState.ArcThirdPoint.Y, 0.0f);

	FVector StartScreen = Project(StartWorld);
	FVector SecondScreen = Project(SecondWorld);
	FVector EndScreen = Project(EndWorld);

	FVector2D Start2D(StartScreen.X, StartScreen.Y);
	FVector2D Second2D(SecondScreen.X, SecondScreen.Y);
	FVector2D End2D(EndScreen.X, EndScreen.Y);

	if (DraftState.ArcRadius > 0.1f)
	{
		FVector CenterWorld(DraftState.ArcCenter.X, DraftState.ArcCenter.Y, 0.0f);
		FVector CenterScreen = Project(CenterWorld);
		FVector2D Center2D(CenterScreen.X, CenterScreen.Y);

		float ScreenRadius = FVector2D::Distance(Center2D, Start2D);

		// Calculate screen-space start angle from projected center to projected start
		// Both points are already in screen space (Y inverted by Project)
		FVector2D ToStartScreen = Start2D - Center2D;
		float ScreenStartAngle = FMath::RadiansToDegrees(FMath::Atan2(ToStartScreen.Y, ToStartScreen.X));
		
		// The world-space sweep angle needs to be used directly
		// Since we calculated ScreenStartAngle from screen-space points, the sweep direction
		// is already accounted for by the Y inversion in both start angle calculation
		float WorldSweepAngle = DraftState.ArcEndAngle - DraftState.ArcStartAngle;
		float ScreenEndAngle = ScreenStartAngle + WorldSweepAngle;

		// Draw the arc using screen-space angles
		DrawSolidArcScreenSpace(Center2D, ScreenRadius, ScreenStartAngle, ScreenEndAngle, ArcPreviewColor, ArcPreviewThickness, ArcPreviewSegments);
	}

	// Line drafting from second point to cursor
	FVector2D LineStart = Second2D;
	FVector2D LineEnd = End2D;
	if (FVector2D::Distance(LineStart, LineEnd) < 5.0f)
	{
		return;
	}

	float LineLengthCm = DraftState.LengthCm;
	float LineAngleDeg = DraftState.AngleDegrees;

	DrawLine(LineStart.X, LineStart.Y, LineEnd.X, LineEnd.Y, WallPreviewColor, WallPreviewThickness);

	FVector2D LineDir = (LineEnd - LineStart).GetSafeNormal();
	float AngleToEnd = FMath::RadiansToDegrees(FMath::Atan2(-(LineEnd - LineStart).Y, (LineEnd - LineStart).X));

	FVector2D Perpendicular(-LineDir.Y, LineDir.X);
	float MeasurementOffset = (AngleToEnd > 0.0f && AngleToEnd < 180.0f) ? -20.0f : 20.0f;

	FVector2D MeasureStart = LineStart + Perpendicular * MeasurementOffset;
	FVector2D MeasureEnd = LineEnd + Perpendicular * MeasurementOffset;
	DrawDashedLine(MeasureStart, MeasureEnd, MeasurementLineColor, MeasurementLineThickness, DashLength, GapLength);

	float TickHalfLength = 5.0f;
	float TickSign = (MeasurementOffset > 0.0f) ? 1.0f : -1.0f;
	FVector2D TickStart1 = LineStart + Perpendicular * (MeasurementOffset - TickHalfLength * TickSign);
	FVector2D TickEnd1 = LineStart + Perpendicular * (MeasurementOffset + TickHalfLength * TickSign);
	DrawLine(TickStart1.X, TickStart1.Y, TickEnd1.X, TickEnd1.Y, MeasurementLineColor, MeasurementLineThickness);

	FVector2D TickStart2 = LineEnd + Perpendicular * (MeasurementOffset - TickHalfLength * TickSign);
	FVector2D TickEnd2 = LineEnd + Perpendicular * (MeasurementOffset + TickHalfLength * TickSign);
	DrawLine(TickStart2.X, TickStart2.Y, TickEnd2.X, TickEnd2.Y, MeasurementLineColor, MeasurementLineThickness);

	float AngleVisRadius = (LineEnd - LineStart).Size();
	if (AngleVisRadius > 10.0f)
	{
		FVector2D HorizontalEnd = LineStart + FVector2D(AngleVisRadius, 0.0f);
		DrawDashedLine(LineStart, HorizontalEnd, MeasurementLineColor, MeasurementLineThickness, DashLength, GapLength);

		float AngleArcStart = FMath::Min(0.0f, AngleToEnd);
		float AngleArcEnd = FMath::Max(0.0f, AngleToEnd);
		DrawDashedArc(LineStart, AngleArcRadius, AngleArcStart, AngleArcEnd, MeasurementLineColor, MeasurementLineThickness, DashLength, GapLength);
	}

	FVector2D MeasureMidPoint = (MeasureStart + MeasureEnd) * 0.5f;
	float LabelOffset = (MeasurementOffset > 0.0f) ? LengthLabelOffsetFromLine : -LengthLabelOffsetFromLine;
	FVector2D LengthLabelPos = MeasureMidPoint + Perpendicular * LabelOffset;

	float DisplayLength = InputBuffer.GetDisplayValue(LineLengthCm);
	FString LengthText;
	bool bLengthIsActive = (InputBuffer.ActiveField == ERTNumericField::Length);

	if (InputBuffer.bIsActive && bLengthIsActive && !InputBuffer.Buffer.IsEmpty())
	{
		// Active field with user typing - show the buffer
		LengthText = InputBuffer.Buffer + TEXT(" ") + InputBuffer.GetUnitSuffix();
	}
	else if (!bLengthIsActive && InputBuffer.bHasSavedLength)
	{
		// Inactive field with saved value - show the saved value
		float SavedDisplayLength = InputBuffer.GetDisplayValue(InputBuffer.SavedLengthCm);
		LengthText = FString::Printf(TEXT("%.2f %s"), SavedDisplayLength, *InputBuffer.GetUnitSuffix());
	}
	else
	{
		// Show calculated geometry value
		LengthText = FString::Printf(TEXT("%.2f %s"), DisplayLength, *InputBuffer.GetUnitSuffix());
	}

	FLinearColor LengthBgColor = bLengthIsActive ? ActiveLabelBackgroundColor : InactiveLabelBackgroundColor;
	DrawLabelWithBackground(LengthText, LengthLabelPos, LabelTextColor, LengthBgColor);

	if (AngleVisRadius > 10.0f)
	{
		float AngleArcStart = FMath::Min(0.0f, AngleToEnd);
		float AngleArcEnd = FMath::Max(0.0f, AngleToEnd);
		float LabelAngleRadius = AngleArcRadius + AngleLabelOffsetFromArc + 15.0f;
		float MidAngle = (AngleArcStart + AngleArcEnd) * 0.5f;
		float MidAngleRad = FMath::DegreesToRadians(MidAngle);
		FVector2D AngleLabelPos = LineStart + FVector2D(
			FMath::Cos(MidAngleRad) * LabelAngleRadius,
			-FMath::Sin(MidAngleRad) * LabelAngleRadius
		);

		bool bAngleIsActive = (InputBuffer.ActiveField == ERTNumericField::Angle);
		FString AngleText;

		float DisplayAngle = FMath::Abs(LineAngleDeg);
		if (DisplayAngle > 180.0f)
		{
			DisplayAngle = 360.0f - DisplayAngle;
		}

		if (InputBuffer.bIsActive && bAngleIsActive && !InputBuffer.Buffer.IsEmpty())
		{
			// Active field with user typing - show the buffer
			AngleText = InputBuffer.Buffer + TEXT("\u00B0");
		}
		else if (!bAngleIsActive && InputBuffer.bHasSavedAngle)
		{
			// Inactive field with saved value - show the saved value
			AngleText = FString::Printf(TEXT("%.2f\u00B0"), InputBuffer.SavedAngleDegrees);
		}
		else
		{
			// Show calculated geometry value
			AngleText = FString::Printf(TEXT("%.2f\u00B0"), DisplayAngle);
		}

		FLinearColor AngleBgColor = bAngleIsActive ? ActiveLabelBackgroundColor : InactiveLabelBackgroundColor;
		DrawLabelWithBackground(AngleText, AngleLabelPos, LabelTextColor, AngleBgColor);
	}
}

void AArchVisHUD::DrawSolidArc(const FVector2D& Center, float Radius, float StartAngleDeg, float EndAngleDeg, const FLinearColor& Color, float Thickness, int32 NumSegments)
{
	// Calculate sweep angle directly - the arc tool handles direction
	float SweepAngle = EndAngleDeg - StartAngleDeg;
	
	float AngleSpanDeg = FMath::Abs(SweepAngle);
	if (AngleSpanDeg < 0.1f) return;
	
	// Clamp to prevent >360 degree arcs (shouldn't happen but safety check)
	if (AngleSpanDeg > 360.0f)
	{
		SweepAngle = (SweepAngle > 0) ? 359.9f : -359.9f;
		AngleSpanDeg = 359.9f;
	}
	
	// Adjust number of segments based on arc length
	int32 ActualSegments = FMath::Clamp(FMath::RoundToInt(AngleSpanDeg / 5.0f), 8, NumSegments * 3);
	
	float StepAngle = SweepAngle / (float)ActualSegments;
	
	for (int32 i = 0; i < ActualSegments; ++i)
	{
		float A1 = FMath::DegreesToRadians(StartAngleDeg + StepAngle * i);
		float A2 = FMath::DegreesToRadians(StartAngleDeg + StepAngle * (i + 1));

		// World-space angles projected to screen: invert Y
		FVector2D P1 = Center + FVector2D(FMath::Cos(A1) * Radius, -FMath::Sin(A1) * Radius);
		FVector2D P2 = Center + FVector2D(FMath::Cos(A2) * Radius, -FMath::Sin(A2) * Radius);

		DrawLine(P1.X, P1.Y, P2.X, P2.Y, Color, Thickness);
	}
}

void AArchVisHUD::DrawSolidArcScreenSpace(const FVector2D& Center, float Radius, float StartAngleDeg, float EndAngleDeg, const FLinearColor& Color, float Thickness, int32 NumSegments)
{
	// This version uses screen-space angles directly (calculated from projected points)
	// No Y inversion needed since angles are already in screen-space convention
	float SweepAngle = EndAngleDeg - StartAngleDeg;
	
	float AngleSpanDeg = FMath::Abs(SweepAngle);
	if (AngleSpanDeg < 0.1f) return;
	
	// Clamp to prevent >360 degree arcs
	if (AngleSpanDeg > 360.0f)
	{
		SweepAngle = (SweepAngle > 0) ? 359.9f : -359.9f;
		AngleSpanDeg = 359.9f;
	}
	
	int32 ActualSegments = FMath::Clamp(FMath::RoundToInt(AngleSpanDeg / 5.0f), 8, NumSegments * 3);
	float StepAngle = SweepAngle / (float)ActualSegments;
	
	for (int32 i = 0; i < ActualSegments; ++i)
	{
		float A1 = FMath::DegreesToRadians(StartAngleDeg + StepAngle * i);
		float A2 = FMath::DegreesToRadians(StartAngleDeg + StepAngle * (i + 1));

		// Screen-space angles: no Y inversion, use Sin directly
		FVector2D P1 = Center + FVector2D(FMath::Cos(A1) * Radius, FMath::Sin(A1) * Radius);
		FVector2D P2 = Center + FVector2D(FMath::Cos(A2) * Radius, FMath::Sin(A2) * Radius);

		DrawLine(P1.X, P1.Y, P2.X, P2.Y, Color, Thickness);
	}
}

