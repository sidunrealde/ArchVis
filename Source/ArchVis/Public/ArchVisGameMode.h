#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RTPlanDocument.h"
#include "RTPlanToolManager.h"
#include "ArchVisGameMode.generated.h"

/**
 * Main GameMode. Initializes the Plan system.
 */
UCLASS()
class ARCHVIS_API AArchVisGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArchVisGameMode();

	virtual void StartPlay() override;

	UFUNCTION(BlueprintCallable, Category = "ArchVis")
	URTPlanDocument* GetDocument() const { return Document; }

	UFUNCTION(BlueprintCallable, Category = "ArchVis")
	URTPlanToolManager* GetToolManager() const { return ToolManager; }

protected:
	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;

	UPROPERTY(Transient)
	TObjectPtr<URTPlanToolManager> ToolManager;
};
