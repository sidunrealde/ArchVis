#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTPlanDocument.h"
#include "RTPlanShellActor.generated.h"

class UDynamicMeshComponent;
class URTFinishCatalog;

DECLARE_LOG_CATEGORY_EXTERN(LogRTPlanShell, Log, All);

/**
 * Actor responsible for rendering the 3D shell (Walls, Floors).
 * Listens to PlanDocument changes and rebuilds meshes.
 * Supports selection highlighting via custom stencil values.
 * Applies materials from finish catalog based on wall finish IDs.
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

	// --- Finish Catalog ---

	/** Set the finish catalog for material lookups */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Shell")
	void SetFinishCatalog(URTFinishCatalog* InCatalog);

	/** Get the current finish catalog */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Shell")
	URTFinishCatalog* GetFinishCatalog() const { return FinishCatalog; }

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

	/** Apply materials to a wall mesh component based on finish IDs */
	void ApplyWallMaterials(UDynamicMeshComponent* MeshComp, const struct FRTWall& Wall);

	/** Get material for a finish ID, returns nullptr if not found (no fallback) */
	UMaterialInterface* GetMaterialForFinish(FName FinishId) const;

	// The main combined mesh (for non-selected walls or legacy mode)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDynamicMeshComponent> WallMeshComponent;

	// Per-wall mesh components for individual selection highlighting
	UPROPERTY(Transient)
	TMap<FGuid, TObjectPtr<UDynamicMeshComponent>> WallMeshComponents;

	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;

	// Finish catalog for material lookups
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTPlan|Shell")
	TObjectPtr<URTFinishCatalog> FinishCatalog;

	// Currently selected wall IDs
	UPROPERTY(Transient)
	TSet<FGuid> SelectedWallIds;
};
