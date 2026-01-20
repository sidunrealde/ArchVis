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

	// --- Debug ---
	
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Debug")
	void SetDebugEnabled(bool bEnabled) { bDebugEnabled = bEnabled; }
	
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Debug")
	bool IsDebugEnabled() const { return bDebugEnabled; }

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

	// Select all walls and openings in the document
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	void SelectAll();

	// Get the center point of all selected items (for focus camera)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	FVector GetSelectionCenter() const;

	// Get the bounding box of all selected items
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	FBox GetSelectionBounds() const;

	// --- Marquee State (for HUD visualization) ---

	// Is a marquee drag currently in progress?
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	bool IsMarqueeDragging() const { return bIsMarqueeDragging; }

	// Get marquee start point (world space)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	FVector2D GetMarqueeStart() const { return MarqueeStartWorld; }

	// Get marquee current end point (world space)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	FVector2D GetMarqueeEnd() const { return MarqueeEndWorld; }

	// Draw the marquee visualization (call from Tick or HUD)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	void DrawMarqueeVisualization(UWorld* World) const;

	// --- Events ---

	// Broadcast when selection changes
	UPROPERTY(BlueprintAssignable, Category = "RTPlan|Selection")
	FOnSelectionChanged OnSelectionChanged;

	// --- Wall Property Access ---

	/** Log properties of all selected walls to the output log */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	void LogSelectedWallProperties() const;

	/** Get wall length in cm (distance between vertices) */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Selection")
	float GetWallLength(const FGuid& WallId) const;

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

	// Debug flag
	bool bDebugEnabled = false;

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
	bool bCtrlOnMouseDown = false;

	// Helper: Perform a single-click selection at the given world position
	void PerformClickSelection(const FVector2D& WorldPos, bool bAddToSelection, bool bRemoveFromSelection);

	// Helper: Perform marquee selection
	void PerformMarqueeSelection(bool bAddToSelection, bool bRemoveFromSelection);

	// Helper: Get world position from pointer event
	// Returns true if a valid position was found.
	// OutWorldPos3D contains the full 3D hit location (for crosshair).
	// OutWorldPos2D contains the 2D plan coordinates (for selection logic).
	bool GetWorldPosition(const FRTPointerEvent& Event, FVector& OutWorldPos3D, FVector2D& OutWorldPos2D) const;
	
	// Helper: Perform 3D line trace for selection
	bool PerformLineTrace(const FRTPointerEvent& Event, FVector& OutHitLocation) const;
};
