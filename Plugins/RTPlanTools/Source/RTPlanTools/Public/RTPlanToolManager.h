#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RTPlanToolBase.h"
#include "RTPlanDocument.h"
#include "RTPlanSpatialIndex.h"
#include "RTPlanToolManager.generated.h"

/**
 * Tool type enum for UI binding.
 */
UENUM(BlueprintType)
enum class ERTPlanToolType : uint8
{
	None,
	Select,
	Line,
	Polyline
};

class URTPlanSelectTool;

/**
 * Manages the active tool and routes input.
 */
UCLASS(BlueprintType)
class RTPLANTOOLS_API URTPlanToolManager : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void Initialize(URTPlanDocument* InDoc);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void SelectTool(TSubclassOf<URTPlanToolBase> ToolClass);

	// Select tool by type enum (easier for UI binding)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void SelectToolByType(ERTPlanToolType ToolType);

	// Get the current tool type
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	ERTPlanToolType GetActiveToolType() const { return ActiveToolType; }

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void ProcessInput(const FRTPointerEvent& Event);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	URTPlanToolBase* GetActiveTool() const { return ActiveTool; }

	// Get the select tool (for querying selection state)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	URTPlanSelectTool* GetSelectTool() const;

	// Get selected wall IDs (from SelectTool)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	TArray<FGuid> GetSelectedWallIds() const;

	// Get selected opening IDs (from SelectTool)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	TArray<FGuid> GetSelectedOpeningIds() const;

	// Delete currently selected items
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void DeleteSelection();

	// Call this every frame or when document changes to keep spatial index fresh
	UFUNCTION()
	void UpdateSpatialIndex();

private:
	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;

	UPROPERTY(Transient)
	TObjectPtr<URTPlanToolBase> ActiveTool;

	// Keep the select tool around for selection state queries
	UPROPERTY(Transient)
	TObjectPtr<URTPlanSelectTool> CachedSelectTool;

	// Current tool type
	ERTPlanToolType ActiveToolType = ERTPlanToolType::None;

	// The spatial index is owned here
	FRTPlanSpatialIndex SpatialIndex;
};
