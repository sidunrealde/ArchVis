#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTPlanDocument.h"
#include "RTPlanShellActor.generated.h"

class UDynamicMeshComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogRTPlanShell, Log, All);

/**
 * Actor responsible for rendering the 3D shell (Walls, Floors).
 * Listens to PlanDocument changes and rebuilds meshes.
 * Supports selection highlighting via custom stencil values.
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

	// --- Selection Highlighting ---

	/** Set which walls are currently selected (applies stencil value 1) */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Shell")
	void SetSelectedWalls(const TArray<FGuid>& WallIds);

	/** Clear all wall selection highlighting */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Shell")
	void ClearSelection();

	/** Get the stencil value used for selected walls */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "RTPlan|Shell")
	int32 SelectionStencilValue = 1;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPlanChanged();

	/** Update stencil values on wall mesh components based on selection */
	void UpdateSelectionHighlight();

	// The main combined mesh (for non-selected walls or legacy mode)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDynamicMeshComponent> WallMeshComponent;

	// Per-wall mesh components for individual selection highlighting
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<UDynamicMeshComponent>> WallMeshComponents;

	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;

	// Currently selected wall IDs
	UPROPERTY(Transient)
	TSet<FGuid> SelectedWallIds;
};
