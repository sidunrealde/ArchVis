#pragma once

#include "CoreMinimal.h"
#include "RTPlanToolBase.h"
#include "RTPlanInputData.h"
#include "RTPlanLineTool.generated.h"

/**
 * Tool for drawing single walls (Line) or chains (Polyline).
 * Supports Soft Ortho Snap and Alignment Snapping.
 */
UCLASS()
class RTPLANTOOLS_API URTPlanLineTool : public URTPlanToolBase
{
	GENERATED_BODY()

public:
	virtual void OnEnter() override;
	virtual void OnExit() override;
	virtual void OnPointerEvent(const FRTPointerEvent& Event) override;
	virtual void OnNumericInput(float Value) override;
	virtual void OnNumericInputWithField(float Value, ERTNumericField Field) override;

	// Toggle between Line (Single) and Polyline (Chain) mode
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void SetPolylineMode(bool bEnable);

	// Get current drafting state for HUD visualization
	// UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	virtual FRTDraftingState GetDraftingState() const override;

	// Get current line length in cm
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	float GetCurrentLengthCm() const;

	// Get current line angle in degrees
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	float GetCurrentAngleDegrees() const;

	// Update preview from typed length value (doesn't commit, just updates visualization)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void UpdatePreviewFromLength(float LengthCm);

	// Update preview from typed angle value (doesn't commit, just updates visualization)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void UpdatePreviewFromAngle(float AngleDegrees);

	// --- Polyline Control Methods ---
	
	// Cancel the current drawing operation (discard all uncommitted points)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void CancelDrawing();

	// Close the polyline (connect last point to first point)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void ClosePolyline();

	// Remove the last placed point in polyline mode
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void RemoveLastPoint();

	// Check if we're currently in an active drawing state
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	bool IsDrawingActive() const { return State == EState::WaitingForEnd; }

	// Check if we're in polyline mode
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	bool IsPolylineMode() const { return bIsPolyline; }

	// Angle threshold for soft ortho snap (degrees)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTPlan|Tools")
	float OrthoSnapThreshold = 5.0f;

	// Angle increment for angle snap (degrees) - typically 45°
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTPlan|Tools")
	float AngleSnapIncrement = 45.0f;

	// Whether ortho snap is currently engaged
	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|Tools")
	bool bIsOrthoSnapped = false;

	// Whether angle snap is currently engaged
	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|Tools")
	bool bIsAngleSnapped = false;

	// Clear the numeric input override (call when mouse moves to resume mouse-based cursor)
	void ClearNumericInputOverride() { bNumericInputActive = false; }
	
	// Check if numeric input is currently controlling the cursor position
	bool IsNumericInputActive() const { return bNumericInputActive; }

private:
	enum class EState
	{
		WaitingForStart,
		WaitingForEnd
	};

	EState State = EState::WaitingForStart;
	FVector2D StartPoint;
	FVector2D CurrentEndPoint;
	
	// For polyline: track the original start point to allow closing
	FVector2D PolylineOriginPoint;
	
	// Track all placed vertices in the current polyline session for undo
	TArray<FGuid> PlacedVertexIds;
	TArray<FGuid> PlacedWallIds;
	
	// Track vertex positions for RemoveLastPoint functionality
	TArray<FVector2D> PlacedVertexPositions;
	
	bool bShowPreview = false;
	bool bIsPolyline = false; // Default to Single Line
	
	// When true, numeric input is controlling the end point - don't update from mouse
	bool bNumericInputActive = false;
	
	// Saved direction when numeric input starts (true = above horizontal, false = below)
	bool bSavedDirectionAboveHorizontal = true;

	// Cached values for HUD
	float CachedLengthCm = 0.0f;
	float CachedAngleDegrees = 0.0f;

	// Apply soft ortho snap - snaps to 0/90/180/270 if within threshold, otherwise free angle
	FVector2D ApplySoftOrthoSnap(const FVector2D& Start, const FVector2D& End);

	// Apply forced ortho lock - only allows 0/90/180/270 angles
	FVector2D ApplyOrthoLock(const FVector2D& Start, const FVector2D& End);

	// Apply angle snap - snaps to AngleSnapIncrement increments (e.g., 45°)
	FVector2D ApplyAngleSnap(const FVector2D& Start, const FVector2D& End);

	// Update cached length and angle
	void UpdateCachedValues();

	// Commit a wall segment and optionally continue polyline
	void CommitWallSegment(bool bContinuePolyline);
};
