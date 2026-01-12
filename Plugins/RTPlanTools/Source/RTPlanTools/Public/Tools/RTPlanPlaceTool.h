#pragma once

#include "CoreMinimal.h"
#include "RTPlanToolBase.h"
#include "RTPlanCatalogTypes.h"
#include "RTPlanPlaceTool.generated.h"

/**
 * Tool for placing furniture objects.
 */
UCLASS()
class RTPLANTOOLS_API URTPlanPlaceTool : public URTPlanToolBase
{
	GENERATED_BODY()

public:
	virtual void OnEnter() override;
	virtual void OnExit() override;
	virtual void OnPointerEvent(const FRTPointerEvent& Event) override;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Tools")
	void SetProduct(FName ProductId);

private:
	FName CurrentProductId;
	
	// Preview Actor (spawned locally, not in document)
	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviewActor;
};
