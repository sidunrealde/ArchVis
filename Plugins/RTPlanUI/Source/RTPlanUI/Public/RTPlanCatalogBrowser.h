#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "RTPlanCatalogTypes.h"
#include "RTPlanCatalogBrowser.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProductSelected, FName, ProductId);

/**
 * Catalog Browser.
 * Displays list of products from a Catalog Data Asset.
 */
UCLASS(Abstract)
class RTPLANUI_API URTPlanCatalogBrowser : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RTPlan|UI")
	void SetCatalog(URTProductCatalog* InCatalog);

	UPROPERTY(BlueprintAssignable, Category = "RTPlan|UI")
	FOnProductSelected OnProductSelected;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|UI")
	TObjectPtr<URTProductCatalog> Catalog;
};
