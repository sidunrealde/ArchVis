#pragma once

#include "CoreMinimal.h"
#include "RTPlanToolBase.h"
#include "RTPlanSelectTool.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogRTPlanSelectTool, Log, All);

/**
 * Delegate broadcast when selection changes.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSelectionChanged);

/**
 * Selection Tool for selecting and multi-selecting walls, openings, and other elements.
 * Supports:
 * - Click to select single item
 * - Shift+Click to add to selection
 * - Alt+Click to remove from selection
 * - Click+Drag for marquee (box) selection
 */
UCLASS(BlueprintType)
class RTPLANTOOLS_API URTPlanSelectTool : public URTPlanToolBase
{
	GENERATED_BODY()

public:
	virtual void OnEnter() override;
	virtual void OnExit() override;
	virtual void OnPointerEvent(const FRTPointerEvent& Event) override;

	// --- Selection State ---

	// Get all selected wall IDs
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	TArray<FGuid> GetSelectedWallIds() const { return SelectedWallIds.Array(); }

	// Get all selected opening IDs
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	TArray<FGuid> GetSelectedOpeningIds() const { return SelectedOpeningIds.Array(); }

	// Check if a specific wall is selected
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	bool IsWallSelected(const FGuid& WallId) const { return SelectedWallIds.Contains(WallId); }

	// Check if a specific opening is selected
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	bool IsOpeningSelected(const FGuid& OpeningId) const { return SelectedOpeningIds.Contains(OpeningId); }

	// Check if anything is selected
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	bool HasSelection() const { return SelectedWallIds.Num() > 0 || SelectedOpeningIds.Num() > 0; }

	// Clear all selection
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	void ClearSelection();

	// --- Marquee State (for HUD visualization) ---

	// Is a marquee drag currently in progress?
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	bool IsMarqueeDragging() const { return bIsMarqueeDragging; }

	// Get marquee start point (screen or world space based on implementation)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	FVector2D GetMarqueeStart() const { return MarqueeStartWorld; }

	// Get marquee current end point
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	FVector2D GetMarqueeEnd() const { return MarqueeEndWorld; }

	// --- Events ---

	// Broadcast when selection changes
	UPROPERTY(BlueprintAssignable, Category = "RTPlan|Selection")
	FOnSelectionChanged OnSelectionChanged;

	// --- Settings ---

	// Distance threshold for considering a click vs a drag (in screen pixels)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTPlan|Selection")
	float DragThreshold = 5.0f;

	// Tolerance for hit testing (in world units / cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTPlan|Selection")
	float HitTestTolerance = 20.0f;

private:
	enum class EState
	{
		Idle,
		PotentialDrag,  // Mouse down, waiting to see if it's a click or drag
		MarqueeDragging
	};

	EState State = EState::Idle;

	// Selection sets
	TSet<FGuid> SelectedWallIds;
	TSet<FGuid> SelectedOpeningIds;

	// Marquee state
	bool bIsMarqueeDragging = false;
	FVector2D MarqueeStartWorld;
	FVector2D MarqueeEndWorld;
	FVector2D MouseDownScreenPos;

	// Cached modifier states from the initial mouse down
	bool bShiftOnMouseDown = false;
	bool bAltOnMouseDown = false;

	// Helper: Perform a single-click selection at the given world position
	void PerformClickSelection(const FVector2D& WorldPos, bool bAddToSelection, bool bRemoveFromSelection);

	// Helper: Perform marquee selection
	void PerformMarqueeSelection(bool bAddToSelection, bool bRemoveFromSelection);

	// Helper: Get world position from pointer event
	bool GetWorldPosition(const FRTPointerEvent& Event, FVector2D& OutWorldPos) const;
};

