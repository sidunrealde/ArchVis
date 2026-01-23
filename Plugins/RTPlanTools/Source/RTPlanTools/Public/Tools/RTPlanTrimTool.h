#pragma once

#include "CoreMinimal.h"
#include "RTPlanToolBase.h"
#include "RTPlanTrimTool.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRTPlanTrimTool, Log, All);

/**
 * Trim Tool for cutting walls at intersections.
 * Supports:
 * - Click to trim a wall segment up to the nearest intersection
 * - Marquee drag to trim multiple segments (fence trim)
 */
UCLASS(BlueprintType)
class RTPLANTOOLS_API URTPlanTrimTool : public URTPlanToolBase
{
	GENERATED_BODY()

public:
	virtual void OnEnter() override;
	virtual void OnExit() override;
	virtual void OnPointerEvent(const FRTPointerEvent& Event) override;

	// --- Debug ---
	
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Debug")
	void SetDebugEnabled(bool bEnabled) { bDebugEnabled = bEnabled; }

	// --- Marquee State (for HUD visualization) ---

	// Is a marquee drag currently in progress?
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Trim")
	bool IsMarqueeDragging() const { return bIsMarqueeDragging; }

	// Get marquee start point (world space)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Trim")
	FVector2D GetMarqueeStart() const { return MarqueeStartWorld; }

	// Get marquee current end point (world space)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Trim")
	FVector2D GetMarqueeEnd() const { return MarqueeEndWorld; }

	// Draw the marquee visualization (call from Tick or HUD)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Trim")
	void DrawMarqueeVisualization(UWorld* World) const;

	// --- Settings ---

	// Distance threshold for considering a click vs a drag (in screen pixels)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTPlan|Trim")
	float DragThreshold = 5.0f;

	// Tolerance for hit testing (in world units / cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTPlan|Trim")
	float HitTestTolerance = 20.0f;

private:
	enum class EState
	{
		Idle,
		PotentialDrag,  // Mouse down, waiting to see if it's a click or drag
		MarqueeDragging
	};

	EState State = EState::Idle;

	// Debug flag
	bool bDebugEnabled = false;

	// Marquee state
	bool bIsMarqueeDragging = false;
	FVector2D MarqueeStartWorld;
	FVector2D MarqueeEndWorld;
	FVector2D MouseDownScreenPos;

	// Helper: Perform a single-click trim at the given world position
	void PerformClickTrim(const FVector2D& WorldPos);

	// Helper: Perform marquee trim (fence trim)
	void PerformMarqueeTrim();

	// Helper: Get world position from pointer event
	bool GetWorldPosition(const FRTPointerEvent& Event, FVector& OutWorldPos3D, FVector2D& OutWorldPos2D) const;
	
	// Helper: Perform 3D line trace for selection
	bool PerformLineTrace(const FRTPointerEvent& Event, FVector& OutHitLocation) const;

	// Helper: Find intersection points on a wall
	// Returns a sorted list of intersection points (T values 0..1) along the wall
	void FindIntersectionsOnWall(const FGuid& WallId, TArray<float>& OutIntersections) const;

	// Helper: Trim a wall at the clicked point
	void TrimWallAtPoint(const FGuid& WallId, const FVector2D& ClickPoint);
};
