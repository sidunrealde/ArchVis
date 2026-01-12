#include "ArchVisHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "ArchVisGameMode.h"
#include "RTPlanToolManager.h"
#include "Tools/RTPlanLineTool.h"
#include "ArchVisPlayerController.h"
#include "Kismet/GameplayStatics.h"

void AArchVisHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	FVector2D CrosshairPos;
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

				// Get drafting state if this is a LineTool
				if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(Tool))
				{
					DraftState = LineTool->GetDraftingState();
				}
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
			DrawDraftingVisualization(DraftState, InputBuffer);
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
		// Show typed value with units
		LengthText = InputBuffer.Buffer + TEXT(" ") + InputBuffer.GetUnitSuffix();
	}
	else
	{
		// Show calculated length with units
		LengthText = FString::Printf(TEXT("%.0f %s"), DisplayLength, *InputBuffer.GetUnitSuffix());
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
		// Show typed angle value
		AngleText = InputBuffer.Buffer + TEXT("\u00B0");
	}
	else
	{
		// Show calculated angle (0-180 from horizontal)
		AngleText = FString::Printf(TEXT("%.0f\u00B0"), DisplayAngle);
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
	// Calculate arc length for dash pattern
	float AngleSpanDeg = FMath::Abs(EndAngleDeg - StartAngleDeg);
	if (AngleSpanDeg < 0.1f)
	{
		return;
	}

	float ArcLength = (AngleSpanDeg / 360.0f) * 2.0f * PI * Radius;
	float PatternLength = DashLen + GapLen;
	int32 NumPatterns = FMath::Max(1, FMath::FloorToInt(ArcLength / PatternLength));
	
	float AnglePerPattern = AngleSpanDeg / (float)NumPatterns;
	float DashAngle = AnglePerPattern * (DashLen / PatternLength);
	
	float CurrentAngle = StartAngleDeg;
	bool bDrawing = true;
	
	for (int32 i = 0; i < NumPatterns * 2 && CurrentAngle < EndAngleDeg; ++i)
	{
		if (bDrawing)
		{
			float SegmentEndAngle = FMath::Min(CurrentAngle + DashAngle, EndAngleDeg);
			
			// Draw arc segment
			int32 SegmentSteps = FMath::Max(2, AngleArcSegments / NumPatterns);
			float StepAngle = (SegmentEndAngle - CurrentAngle) / (float)SegmentSteps;
			
			for (int32 j = 0; j < SegmentSteps; ++j)
			{
				float A1 = FMath::DegreesToRadians(CurrentAngle + StepAngle * j);
				float A2 = FMath::DegreesToRadians(CurrentAngle + StepAngle * (j + 1));
				
				// Screen Y is inverted, so negate sin
				FVector2D P1 = Center + FVector2D(FMath::Cos(A1) * Radius, -FMath::Sin(A1) * Radius);
				FVector2D P2 = Center + FVector2D(FMath::Cos(A2) * Radius, -FMath::Sin(A2) * Radius);
				
				DrawLine(P1.X, P1.Y, P2.X, P2.Y, Color, Thickness);
			}
			
			CurrentAngle = SegmentEndAngle;
		}
		else
		{
			CurrentAngle += AnglePerPattern - DashAngle;
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
