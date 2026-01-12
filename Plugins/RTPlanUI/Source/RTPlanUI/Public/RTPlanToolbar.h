#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "RTPlanToolBase.h"
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

protected:
	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|UI")
	TObjectPtr<URTPlanToolManager> ToolManager;
};
