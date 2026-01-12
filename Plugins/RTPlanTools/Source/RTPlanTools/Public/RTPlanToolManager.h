#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RTPlanToolBase.h"
#include "RTPlanDocument.h"
#include "RTPlanSpatialIndex.h"
#include "RTPlanToolManager.generated.h"

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

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void ProcessInput(const FRTPointerEvent& Event);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	URTPlanToolBase* GetActiveTool() const { return ActiveTool; }

	// Call this every frame or when document changes to keep spatial index fresh
	UFUNCTION()
	void UpdateSpatialIndex();

private:
	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;

	UPROPERTY(Transient)
	TObjectPtr<URTPlanToolBase> ActiveTool;

	// The spatial index is owned here
	FRTPlanSpatialIndex SpatialIndex;
};
