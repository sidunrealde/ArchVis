// ArchVisDraftingPawn.h
// 2D Top-down drafting pawn with orthographic camera

#pragma once

#include "CoreMinimal.h"
#include "ArchVisPawnBase.h"
#include "ArchVisDraftingPawn.generated.h"

/**
 * 2D Drafting Pawn with top-down orthographic view.
 * Primary pawn for CAD-style wall editing.
 */
UCLASS()
class ARCHVIS_API AArchVisDraftingPawn : public AArchVisPawnBase
{
	GENERATED_BODY()

public:
	AArchVisDraftingPawn();

	virtual void Zoom(float Amount) override;
	virtual void Pan(FVector2D Delta) override;
	virtual void ResetView() override;
	virtual void SetCameraTransform(FVector Location, FRotator Rotation, float ZoomLevel) override;
	virtual float GetZoomLevel() const override;

protected:
	virtual void BeginPlay() override;

	// Update ortho width based on zoom level
	void UpdateOrthoWidth();

	// Ortho width multiplier (ortho width = arm length * this value)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|2D")
	float OrthoWidthMultiplier = 2.0f;
};

