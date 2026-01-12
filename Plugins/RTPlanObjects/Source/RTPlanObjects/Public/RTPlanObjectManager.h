#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTPlanDocument.h"
#include "RTPlanCatalogTypes.h"
#include "RTPlanObjectManager.generated.h"

/**
 * Manages the spawning and updating of interior object actors (Furniture).
 */
UCLASS()
class RTPLANOBJECTS_API ARTPlanObjectManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ARTPlanObjectManager();

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Objects")
	void SetDocument(URTPlanDocument* InDoc);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Objects")
	void SetCatalog(URTProductCatalog* InCatalog);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Objects")
	void RebuildAll();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPlanChanged();

	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<URTProductCatalog> Catalog;

	// Map from Object ID to Spawned Actor
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<AActor>> SpawnedObjects;
};
