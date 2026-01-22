#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "RTPlanToolBase.h"
#include "RTPlanToolManager.h"
#include "RTPlanToolbar.generated.h"

class URTPlanToolManager;

/**
 * Toolbar widget for selecting tools.
 */
UCLASS(Abstract)
class RTPLANUI_API URTPlanToolbar : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	void SetToolManager(URTPlanToolManager* InMgr);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	void SelectTool(TSubclassOf<URTPlanToolBase> ToolClass);

	// Select tool by type enum (easier for Blueprint binding)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	void SelectToolByType(ERTPlanToolType ToolType);

	// Get current tool type (for highlighting active button)
	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	ERTPlanToolType GetActiveToolType() const;

	// Toggle snapping
	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	void ToggleSnap();

	// Toggle grid
	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	void ToggleGrid();

	// Is snapping enabled?
	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	bool IsSnapEnabled() const;

	// Is grid enabled?
	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	bool IsGridEnabled() const;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|UI")
	TObjectPtr<URTPlanToolManager> ToolManager;
};
