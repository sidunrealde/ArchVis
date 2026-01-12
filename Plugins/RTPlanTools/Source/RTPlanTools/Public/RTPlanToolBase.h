#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RTPlanInputData.h"
#include "RTPlanDocument.h"
#include "RTPlanSpatialIndex.h"
#include "RTPlanToolBase.generated.h"

/**
 * Base class for all interactive tools (Line, Polyline, Select, etc.).
 * Handles input events and emits commands to the document.
 */
UCLASS(Abstract, BlueprintType)
class RTPLANTOOLS_API URTPlanToolBase : public UObject
{
	GENERATED_BODY()

public:
	// Initialize the tool with context
	virtual void Init(URTPlanDocument* InDoc, FRTPlanSpatialIndex* InSpatialIndex);

	// Called when the tool becomes active
	virtual void OnEnter() {}

	// Called when the tool is deactivated
	virtual void OnExit() {}

	// Handle unified pointer input
	virtual void OnPointerEvent(const FRTPointerEvent& Event);

	// Handle numeric input (e.g. typing "150" for length) - legacy, assumes length
	virtual void OnNumericInput(float Value) {}

	// Handle numeric input with field type specification
	virtual void OnNumericInputWithField(float Value, ERTNumericField Field) { OnNumericInput(Value); }

	// Render tool visualization (gizmos, preview lines)
	// Usually called from the HUD or a SceneProxy
	virtual void DrawVisualization(class FPrimitiveDrawInterface* PDI, const FSceneView* View) {}

	// Get the current snapped cursor position in World Space (for HUD/Crosshair)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	virtual FVector GetSnappedCursorPos() const { return LastSnappedWorldPos; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;

	// Raw pointer to spatial index (owned by the ToolManager/System)
	FRTPlanSpatialIndex* SpatialIndex = nullptr;

	// Last calculated snapped position
	FVector LastSnappedWorldPos;

	// Helper: Raycast against the ground plane (Z=0)
	bool GetGroundIntersection(const FRTPointerEvent& Event, FVector& OutPoint) const;

	// Helper: Snap a world point to the grid/geometry using the Spatial Index
	FVector2D GetSnappedPoint(const FVector& WorldPoint, float SnapRadius = 20.0f) const;
};
