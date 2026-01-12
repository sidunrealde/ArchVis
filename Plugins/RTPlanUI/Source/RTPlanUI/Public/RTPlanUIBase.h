#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "RTPlanDocument.h"
#include "RTPlanToolManager.h"
#include "RTPlanUIBase.generated.h"

/**
 * Base class for main UI HUD.
 * Provides access to Document and ToolManager for child widgets.
 */
UCLASS(Abstract)
class RTPLANUI_API URTPlanUIBase : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	void InitializeContext(URTPlanDocument* InDoc, URTPlanToolManager* InToolMgr);

	UFUNCTION(BlueprintImplementableEvent, Category = "RTPlan|UI")
	void OnContextInitialized();

protected:
	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|UI")
	TObjectPtr<URTPlanDocument> Document;

	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|UI")
	TObjectPtr<URTPlanToolManager> ToolManager;
};
