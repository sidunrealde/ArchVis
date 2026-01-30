// RTPlanFinishCatalog.h
// Material/Finish Catalog for Wall Surfaces

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "RTPlanFinishCatalog.generated.h"

/**
 * Definition of a finish/material that can be applied to wall surfaces.
 */
USTRUCT(BlueprintType)
struct RTPLANCATALOG_API FRTFinishDefinition
{
	GENERATED_BODY()

	/** Unique identifier for this finish */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	FName Id;

	/** Display name shown in UI */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	FText DisplayName;

	/** The material to apply to geometry */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	TSoftObjectPtr<UMaterialInterface> Material;

	/** Preview thumbnail texture (for UI dropdowns) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	TSoftObjectPtr<UTexture2D> PreviewTexture;

	/** Optional category for grouping (e.g., "Paint", "Wood", "Tile", "Stone") */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	FName Category;

	/** Optional tiling scale (UVs per meter) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Finish")
	float TileScalePerMeter = 1.0f;
};

/**
 * Data Asset containing a collection of finish definitions.
 * Used by the Shell Actor to look up materials by ID.
 */
UCLASS(BlueprintType)
class RTPLANCATALOG_API URTFinishCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Array of all available finishes */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
	TArray<FRTFinishDefinition> Finishes;

	/** Default finish to use when no finish is specified */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalog")
	FName DefaultFinishId;

	/**
	 * Find a finish by ID.
	 * @param FinishId The ID to look up
	 * @param OutFinish The finish definition if found
	 * @return True if found
	 */
	UFUNCTION(BlueprintCallable, Category = "Finish Catalog")
	bool GetFinish(FName FinishId, FRTFinishDefinition& OutFinish) const
	{
		const FRTFinishDefinition* Found = Finishes.FindByPredicate(
			[&](const FRTFinishDefinition& Item) { return Item.Id == FinishId; }
		);
		if (Found)
		{
			OutFinish = *Found;
			return true;
		}
		return false;
	}

	/**
	 * Find a finish by ID (C++ only, returns pointer)
	 */
	const FRTFinishDefinition* FindFinish(FName FinishId) const
	{
		return Finishes.FindByPredicate(
			[&](const FRTFinishDefinition& Item) { return Item.Id == FinishId; }
		);
	}

	/**
	 * Get the default finish definition
	 */
	UFUNCTION(BlueprintCallable, Category = "Finish Catalog")
	bool GetDefaultFinish(FRTFinishDefinition& OutFinish) const
	{
		return GetFinish(DefaultFinishId, OutFinish);
	}

	/**
	 * Get all finishes in a specific category
	 */
	UFUNCTION(BlueprintCallable, Category = "Finish Catalog")
	TArray<FRTFinishDefinition> GetFinishesByCategory(FName Category) const
	{
		TArray<FRTFinishDefinition> Result;
		for (const FRTFinishDefinition& Finish : Finishes)
		{
			if (Finish.Category == Category)
			{
				Result.Add(Finish);
			}
		}
		return Result;
	}

	/**
	 * Get all unique categories in this catalog
	 */
	UFUNCTION(BlueprintCallable, Category = "Finish Catalog")
	TArray<FName> GetCategories() const
	{
		TSet<FName> CategorySet;
		for (const FRTFinishDefinition& Finish : Finishes)
		{
			if (!Finish.Category.IsNone())
			{
				CategorySet.Add(Finish.Category);
			}
		}
		return CategorySet.Array();
	}

	/**
	 * Load and get the material for a finish ID.
	 * Returns nullptr if not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Finish Catalog")
	UMaterialInterface* GetMaterialForFinish(FName FinishId) const
	{
		const FRTFinishDefinition* Finish = FindFinish(FinishId);
		if (Finish && !Finish->Material.IsNull())
		{
			return Finish->Material.LoadSynchronous();
		}
		return nullptr;
	}
};
