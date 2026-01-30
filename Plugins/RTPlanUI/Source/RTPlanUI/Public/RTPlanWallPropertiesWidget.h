// RTPlanWallPropertiesWidget.h
// Wall Properties Floating Panel Widget
// Displays and allows editing of wall properties in real-time

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTPlanDocument.h"
#include "RTPlanSchema.h"
#include "RTPlanWallPropertiesWidget.generated.h"

class URTPlanSelectTool;
class URTFinishCatalog;

/**
 * Unit type for measurement display
 */
UENUM(BlueprintType)
enum class ERTPlanUnit : uint8
{
	Centimeters UMETA(DisplayName = "cm"),
	Meters UMETA(DisplayName = "m"),
	Inches UMETA(DisplayName = "in"),
	Feet UMETA(DisplayName = "ft")
};

/**
 * Material entry for the material dropdown
 */
USTRUCT(BlueprintType)
struct RTPLANUI_API FRTMaterialEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Material")
	FName MaterialId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Material")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Material")
	UMaterialInterface* PreviewMaterial = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Material")
	UTexture2D* PreviewTexture = nullptr;
};

/**
 * Wall Properties Widget
 * A floating window for editing wall properties in real-time.
 * 
 * Features:
 * - Wall dimension editing (thickness, height, base height)
 * - Unit conversion (cm, m, in, ft)
 * - Material/finish selection with preview
 * - Left/Right skirting configuration
 * - Real-time updates to the document
 */
UCLASS(BlueprintType, Blueprintable)
class RTPLANUI_API URTPlanWallPropertiesWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- Initialization ---

	/**
	 * Setup the widget with a document and select tool.
	 * Call this after creating the widget to connect it to the plan system.
	 * Note: Named SetupContext to avoid hiding UUserWidget::Initialize()
	 */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetupContext(URTPlanDocument* InDocument, URTPlanSelectTool* InSelectTool);

	/**
	 * Load materials from a finish catalog.
	 * This populates the available materials list from the catalog,
	 * loading preview textures for display in dropdowns.
	 */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void LoadMaterialsFromCatalog(URTFinishCatalog* Catalog);

	/**
	 * Set the available materials for dropdowns
	 * Call this to populate material options
	 */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetAvailableMaterials(const TArray<FRTMaterialEntry>& Materials);

	/**
	 * Show the widget for the selected wall(s)
	 */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void ShowForSelection();

	/**
	 * Hide the widget
	 */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void HidePanel();

	// --- Unit Conversion ---

	/** Get the display name for a unit */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTPlan|Units")
	static FText GetUnitDisplayName(ERTPlanUnit Unit);

	/** Convert from cm to the specified unit */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTPlan|Units")
	static float ConvertFromCm(float ValueCm, ERTPlanUnit ToUnit);

	/** Convert to cm from the specified unit */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTPlan|Units")
	static float ConvertToCm(float Value, ERTPlanUnit FromUnit);

	// --- Property Getters (for UI binding) ---

	/** Get wall thickness in current display unit */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	float GetWallThickness() const;

	/** Get wall height in current display unit */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	float GetWallHeight() const;

	/** Get base height in current display unit */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	float GetBaseHeight() const;

	/** Get left skirting height in current display unit */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	float GetLeftSkirtingHeight() const;

	/** Get left skirting thickness in current display unit */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	float GetLeftSkirtingThickness() const;

	/** Get right skirting height in current display unit */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	float GetRightSkirtingHeight() const;

	/** Get right skirting thickness in current display unit */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	float GetRightSkirtingThickness() const;

	/** Get whether the wall has left skirting */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	bool GetHasLeftSkirting() const;

	/** Get whether the wall has right skirting */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	bool GetHasRightSkirting() const;

	// --- Material Getters ---

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetLeftWallMaterial() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetRightWallMaterial() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetLeftCapMaterial() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetRightCapMaterial() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetTopMaterial() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetLeftSkirtingMaterial() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetLeftSkirtingTopMaterial() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetLeftSkirtingCapMaterial() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetRightSkirtingMaterial() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetRightSkirtingTopMaterial() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	FName GetRightSkirtingCapMaterial() const;

	// --- Property Setters (apply changes with undo support) ---

	/** Set wall thickness (value is in current display unit) */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetWallThickness(float Value);

	/** Set wall height (value is in current display unit) */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetWallHeight(float Value);

	/** Set base height (value is in current display unit) */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetBaseHeight(float Value);

	/** Set left skirting height (value is in current display unit) */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetLeftSkirtingHeight(float Value);

	/** Set left skirting thickness (value is in current display unit) */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetLeftSkirtingThickness(float Value);

	/** Set right skirting height (value is in current display unit) */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetRightSkirtingHeight(float Value);

	/** Set right skirting thickness (value is in current display unit) */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetRightSkirtingThickness(float Value);

	/** Set whether the wall has left skirting */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetHasLeftSkirting(bool bValue);

	/** Set whether the wall has right skirting */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetHasRightSkirting(bool bValue);

	// --- Material Setters ---

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetLeftWallMaterial(FName MaterialId);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetRightWallMaterial(FName MaterialId);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetLeftCapMaterial(FName MaterialId);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetRightCapMaterial(FName MaterialId);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetTopMaterial(FName MaterialId);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetLeftSkirtingMaterial(FName MaterialId);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetLeftSkirtingTopMaterial(FName MaterialId);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetLeftSkirtingCapMaterial(FName MaterialId);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetRightSkirtingMaterial(FName MaterialId);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetRightSkirtingTopMaterial(FName MaterialId);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetRightSkirtingCapMaterial(FName MaterialId);

	// --- Unit Selection ---

	/** Set the current display unit for dimensions */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetDimensionUnit(ERTPlanUnit NewUnit);

	/** Get the current display unit for dimensions */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	ERTPlanUnit GetDimensionUnit() const { return CurrentDimensionUnit; }

	/** Set the current display unit for skirting dimensions */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	void SetSkirtingUnit(ERTPlanUnit NewUnit);

	/** Get the current display unit for skirting dimensions */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	ERTPlanUnit GetSkirtingUnit() const { return CurrentSkirtingUnit; }

	// --- Blueprint Events ---

	/** Called when the selected wall data changes (refresh UI) */
	UFUNCTION(BlueprintImplementableEvent, Category = "RTPlan|WallProperties")
	void OnWallDataChanged();

	/** Called when selection changes */
	UFUNCTION(BlueprintImplementableEvent, Category = "RTPlan|WallProperties")
	void OnSelectionChanged();

	/** Called when unit changes (refresh displayed values) */
	UFUNCTION(BlueprintImplementableEvent, Category = "RTPlan|WallProperties")
	void OnUnitChanged();

	/** Called when materials are loaded from catalog (populate dropdowns) */
	UFUNCTION(BlueprintImplementableEvent, Category = "RTPlan|WallProperties")
	void OnMaterialsLoaded();

	// --- Access to material list ---

	/** Get the list of available materials */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	const TArray<FRTMaterialEntry>& GetAvailableMaterials() const { return AvailableMaterials; }

	/** Find material entry by ID */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	bool FindMaterialById(FName MaterialId, FRTMaterialEntry& OutEntry) const;

	/** Check if a wall is currently selected */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	bool HasSelectedWall() const;

	/** Get the number of selected walls */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|WallProperties")
	int32 GetSelectedWallCount() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Callbacks
	UFUNCTION()
	void HandleSelectionChanged();

	UFUNCTION()
	void HandlePlanChanged();

	// Helper to get the first selected wall (for display)
	const FRTWall* GetFirstSelectedWall() const;

	// Helper to apply changes to all selected walls
	void ApplyToSelectedWalls(TFunction<void(FRTWall&)> ModifyFunc, const FString& CommandDescription);

	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|WallProperties")
	TObjectPtr<URTPlanDocument> Document;

	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|WallProperties")
	TObjectPtr<URTPlanSelectTool> SelectTool;

	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|WallProperties")
	TArray<FRTMaterialEntry> AvailableMaterials;

	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|WallProperties")
	ERTPlanUnit CurrentDimensionUnit = ERTPlanUnit::Centimeters;

	UPROPERTY(BlueprintReadOnly, Category = "RTPlan|WallProperties")
	ERTPlanUnit CurrentSkirtingUnit = ERTPlanUnit::Centimeters;

private:
	// Flag to prevent recursive updates
	bool bIsUpdating = false;
};
