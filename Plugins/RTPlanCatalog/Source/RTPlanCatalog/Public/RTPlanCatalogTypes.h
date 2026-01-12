#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTPlanSchema.h"
#include "RTPlanCatalogTypes.generated.h"

/**
 * Definition of a product (Furniture, Door, Window, etc.)
 */
USTRUCT(BlueprintType)
struct RTPLANCATALOG_API FRTProductDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UStaticMesh> Mesh;

	// Dimensions for placement logic
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector Dimensions = FVector(100, 100, 100);

	// Placement constraints
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ERTHostType AllowedHost = ERTHostType::Floor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bSnapToWall = false;
};

/**
 * Data Asset to hold a collection of products.
 * In a real app, this might be split into multiple assets or loaded from a database.
 */
UCLASS(BlueprintType)
class RTPLANCATALOG_API URTProductCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FRTProductDefinition> Products;

	// UFUNCTION cannot return a pointer to a USTRUCT.
	// We change this to return by value or use a different pattern.
	// For Blueprint usage, returning by value (copy) is safer.
	// Or we can return a bool and output parameter.
	
	UFUNCTION(BlueprintCallable)
	bool GetProduct(FName ProductId, FRTProductDefinition& OutProduct) const
	{
		const FRTProductDefinition* Found = Products.FindByPredicate([&](const FRTProductDefinition& Item) { return Item.Id == ProductId; });
		if (Found)
		{
			OutProduct = *Found;
			return true;
		}
		return false;
	}

	// C++ only helper
	const FRTProductDefinition* FindProduct(FName ProductId) const
	{
		return Products.FindByPredicate([&](const FRTProductDefinition& Item) { return Item.Id == ProductId; });
	}
};
