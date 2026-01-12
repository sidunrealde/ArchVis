#pragma once

#include "CoreMinimal.h"
#include "RTPlanToolBase.h"
#include "RTPlanLineTool.generated.h"

/**
 * Tool for drawing single walls (Line) or chains (Polyline).
 * Supports Ortho Locking and Soft Alignment Snapping.
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

	// Toggle between Line (Single) and Polyline (Chain) mode
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void SetPolylineMode(bool bEnable);

private:
	enum class EState
	{
		WaitingForStart,
		WaitingForEnd
	};

	EState State = EState::WaitingForStart;
	FVector2D StartPoint;
	FVector2D CurrentEndPoint;
	
	bool bShowPreview = false;
	bool bIsPolyline = false; // Default to Single Line

	// Helper to apply ortho locking
	FVector2D ApplyOrthoLock(const FVector2D& Start, const FVector2D& End) const;
};
