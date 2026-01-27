#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RTPlanInputData.h"
#include "ArchVisHUD.generated.h"

/**
 * HUD for drafting. Draws CAD-style visualization:
 * - Full-screen crosshair cursor
 * - Solid wall preview line
 * - Dashed measurement line for length
 * - Dashed arc for angle visualization
 * - Length and angle labels with active/inactive states
 */
UCLASS()
class ARCHVIS_API AArchVisHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	// --- Crosshair Settings ---
	UPROPERTY(EditAnywhere, Category = "Drafting|Crosshair")
	FLinearColor CrosshairColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Drafting|Crosshair")
	float CrosshairThickness = 1.0f;

	// --- Wall Preview Line (Solid) ---
	UPROPERTY(EditAnywhere, Category = "Drafting|WallPreview")
	FLinearColor WallPreviewColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Drafting|WallPreview")
	float WallPreviewThickness = 2.0f;

	// --- Measurement Lines (Dashed) ---
	UPROPERTY(EditAnywhere, Category = "Drafting|Measurement")
	FLinearColor MeasurementLineColor = FLinearColor(0.6f, 0.6f, 0.6f, 1.0f); // Gray

	UPROPERTY(EditAnywhere, Category = "Drafting|Measurement")
	float MeasurementLineThickness = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Drafting|Measurement")
	float DashLength = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Drafting|Measurement")
	float GapLength = 4.0f;

	// --- Angle Arc Settings ---
	UPROPERTY(EditAnywhere, Category = "Drafting|AngleArc")
	float AngleArcRadius = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Drafting|AngleArc")
	int32 AngleArcSegments = 24;

	// --- Label Settings ---
	UPROPERTY(EditAnywhere, Category = "Drafting|Labels")
	FLinearColor ActiveLabelBackgroundColor = FLinearColor(0.0f, 0.5f, 1.0f, 0.9f); // Bright blue for active

	UPROPERTY(EditAnywhere, Category = "Drafting|Labels")
	FLinearColor InactiveLabelBackgroundColor = FLinearColor(0.25f, 0.25f, 0.25f, 0.85f); // Dark gray for inactive

	UPROPERTY(EditAnywhere, Category = "Drafting|Labels")
	FLinearColor LabelTextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Drafting|Labels")
	FVector2D LabelPadding = FVector2D(6.0f, 4.0f);

	UPROPERTY(EditAnywhere, Category = "Drafting|Labels")
	float LengthLabelOffsetFromLine = 15.0f;

	UPROPERTY(EditAnywhere, Category = "Drafting|Labels")
	float AngleLabelOffsetFromArc = 10.0f;

	// --- Numeric Input Display ---
	UPROPERTY(EditAnywhere, Category = "Drafting|NumericInput")
	FLinearColor InputBufferColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f); // Gold

	UPROPERTY(EditAnywhere, Category = "Drafting|NumericInput")
	FVector2D InputBufferOffset = FVector2D(30.0f, 50.0f);

	// --- Arc Preview Settings ---
	UPROPERTY(EditAnywhere, Category = "Drafting|ArcPreview")
	FLinearColor ArcPreviewColor = FLinearColor(0.0f, 0.8f, 1.0f, 1.0f); // Cyan for arc preview

	UPROPERTY(EditAnywhere, Category = "Drafting|ArcPreview")
	float ArcPreviewThickness = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Drafting|ArcPreview")
	FLinearColor ChordLineColor = FLinearColor(0.5f, 0.5f, 0.5f, 0.8f); // Gray for chord reference

	UPROPERTY(EditAnywhere, Category = "Drafting|ArcPreview")
	int32 ArcPreviewSegments = 32;

private:
	// Draw a dashed line between two screen points
	void DrawDashedLine(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color, float Thickness, float DashLen, float GapLen);

	// Draw a dashed arc for angle visualization
	void DrawDashedArc(const FVector2D& Center, float Radius, float StartAngleDeg, float EndAngleDeg, const FLinearColor& Color, float Thickness, float DashLen, float GapLen);

	// Draw a solid arc (for arc preview) - uses world-space angles with Y inversion
	void DrawSolidArc(const FVector2D& Center, float Radius, float StartAngleDeg, float EndAngleDeg, const FLinearColor& Color, float Thickness, int32 NumSegments);

	// Draw a solid arc using screen-space angles (no Y inversion needed)
	void DrawSolidArcScreenSpace(const FVector2D& Center, float Radius, float StartAngleDeg, float EndAngleDeg, const FLinearColor& Color, float Thickness, int32 NumSegments);

	// Draw a label with background
	void DrawLabelWithBackground(const FString& Text, const FVector2D& Position, const FLinearColor& TextColor, const FLinearColor& BackgroundColor);

	// Draw the drafting visualization (wall line, measurement lines, angle arc, labels)
	void DrawDraftingVisualization(const FRTDraftingState& DraftState, const FRTNumericInputBuffer& InputBuffer);

	// Draw arc-specific drafting visualization (arc preview, chord, radius/angle labels)
	void DrawArcDraftingVisualization(const FRTDraftingState& DraftState, const FRTNumericInputBuffer& InputBuffer);
};
