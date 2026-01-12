#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "RTPlanDocument.h"
#include "RTPlanProperties.generated.h"

/**
 * Property Inspector.
 * Listens to selection (future) or document changes to update UI.
 */
UCLASS(Abstract)
class RTPLANUI_API URTPlanProperties : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	void SetDocument(URTPlanDocument* InDoc);

	UFUNCTION(BlueprintImplementableEvent, Category = "RTPlan|UI")
	void Refresh();

protected:
	UFUNCTION()
	void OnPlanChanged();

	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|UI")
	TObjectPtr<URTPlanDocument> Document;
};
