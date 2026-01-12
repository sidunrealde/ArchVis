#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTPlanDocument.h"
#include "RTPlanShellActor.generated.h"

class UDynamicMeshComponent;

/**
 * Actor responsible for rendering the 3D shell (Walls, Floors).
 * Listens to PlanDocument changes and rebuilds meshes.
 */
UCLASS()
class RTPLANSHELL_API ARTPlanShellActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ARTPlanShellActor();

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Shell")
	void SetDocument(URTPlanDocument* InDoc);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Shell")
	void RebuildAll();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPlanChanged();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDynamicMeshComponent> WallMeshComponent;

	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;
};
