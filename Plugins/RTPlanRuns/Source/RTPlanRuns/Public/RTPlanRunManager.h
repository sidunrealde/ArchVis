#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTPlanDocument.h"
#include "RTPlanRunManager.generated.h"

/**
 * Manages procedural runs.
 * Generates "virtual" objects in the document based on Run definitions.
 */
UCLASS()
class RTPLANRUNS_API ARTPlanRunManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ARTPlanRunManager();

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Runs")
	void SetDocument(URTPlanDocument* InDoc);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Runs")
	void RegenerateRuns();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPlanChanged();

	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;
};
