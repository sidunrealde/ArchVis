#pragma once

#include "CoreMinimal.h"
#include "RTPlanToolBase.h"
#include "RTPlanInputData.h"
#include "RTPlanArcTool.generated.h"

/**
 * Tool for drawing arcs using AutoCAD's default 3-Point Arc workflow:
 * 1. First click: Start Point of the arc
 * 2. Second click: A point ON the arc (second point)
 * 3. Third click: End Point of the arc
 * 
 * The arc passes through all three points in order: Start -> SecondPoint -> End
 * Arc preview updates in real-time as the cursor moves.
 */
UCLASS()
class RTPLANTOOLS_API URTPlanArcTool : public URTPlanToolBase
{
	GENERATED_BODY()

public:
	virtual void OnEnter() override;
	virtual void OnExit() override;
	virtual void OnPointerEvent(const FRTPointerEvent& Event) override;
	virtual void OnNumericInput(float Value) override;
	virtual void OnNumericInputWithField(float Value, ERTNumericField Field) override;

	// Get current drafting state for HUD visualization
	virtual FRTDraftingState GetDraftingState() const override;

	// Get current radius
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	float GetCurrentRadius() const;

	// Get current angle (sweep angle of the arc)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	float GetCurrentAngleDegrees() const;

	// Update preview from typed length value (doesn't commit, just updates visualization)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void UpdatePreviewFromLength(float LengthCm);

	// Update preview from typed angle value (doesn't commit, just updates visualization)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void UpdatePreviewFromAngle(float AngleDegrees);

	// Cancel the current drawing operation
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void CancelDrawing();

	// Angle increment for angle snap (degrees) - typically 45°
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTPlan|Tools")
	float AngleSnapIncrement = 45.0f;

	// Whether angle snap is currently engaged
	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|Tools")
	bool bIsAngleSnapped = false;

private:
	enum class EState
	{
		WaitingForStart,      // Waiting for first click (start point)
		WaitingForSecondPoint, // Waiting for second click (point on arc)
		WaitingForEnd          // Waiting for third click (end point)
	};

	EState State = EState::WaitingForStart;
	
	// The three points that define the arc
	FVector2D StartPoint;      // First click - start of arc
	FVector2D SecondPoint;     // Second click - point on arc
	FVector2D EndPoint;        // Third click - end of arc (or cursor during WaitingForEnd)
	
	FVector2D CurrentPoint;    // Current cursor position
	
	// Calculated arc properties
	FVector2D CenterPoint;
	float Radius = 0.0f;
	float SweepAngleDegrees = 0.0f;  // Positive = CCW, Negative = CW
	
	bool bShowPreview = false;
	bool bNumericInputActive = false;
	
	// Cached values for HUD
	float CachedRadius = 0.0f;
	float CachedAngleDegrees = 0.0f;

	// Calculate arc center and radius from 3 points (Start, SecondPoint, End/Cursor)
	void CalculateArcFrom3Points(const FVector2D& P1, const FVector2D& P2, const FVector2D& P3);

	// Update cached values for HUD
	void UpdateCachedValues();

	// Commit the arc to the document
	void CommitArc();
};
