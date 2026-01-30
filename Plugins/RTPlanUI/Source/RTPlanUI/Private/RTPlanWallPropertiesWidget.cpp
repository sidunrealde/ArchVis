// RTPlanWallPropertiesWidget.cpp
// Wall Properties Widget Implementation

#include "RTPlanWallPropertiesWidget.h"
#include "RTPlanCommand.h"
#include "RTPlanFinishCatalog.h"
#include "Tools/RTPlanSelectTool.h"

// Unit conversion constants
namespace RTPlanUnits
{
	constexpr float CmPerMeter = 100.0f;
	constexpr float CmPerInch = 2.54f;
	constexpr float CmPerFoot = 30.48f;
}

void URTPlanWallPropertiesWidget::SetupContext(URTPlanDocument* InDocument, URTPlanSelectTool* InSelectTool)
{
	Document = InDocument;
	SelectTool = InSelectTool;

	if (SelectTool)
	{
		SelectTool->OnSelectionChanged.AddDynamic(this, &URTPlanWallPropertiesWidget::HandleSelectionChanged);
	}

	if (Document)
	{
		Document->OnPlanChanged.AddDynamic(this, &URTPlanWallPropertiesWidget::HandlePlanChanged);
	}
}

void URTPlanWallPropertiesWidget::LoadMaterialsFromCatalog(URTFinishCatalog* Catalog)
{
	AvailableMaterials.Empty();

	if (!Catalog)
	{
		return;
	}

	for (const FRTFinishDefinition& Finish : Catalog->Finishes)
	{
		FRTMaterialEntry Entry;
		Entry.MaterialId = Finish.Id;
		Entry.DisplayName = Finish.DisplayName;
		
		// Load the material
		if (!Finish.Material.IsNull())
		{
			Entry.PreviewMaterial = Finish.Material.LoadSynchronous();
		}
		
		// Load the preview texture
		if (!Finish.PreviewTexture.IsNull())
		{
			Entry.PreviewTexture = Finish.PreviewTexture.LoadSynchronous();
		}
		
		AvailableMaterials.Add(Entry);
	}

	// Notify Blueprint that materials are available
	OnMaterialsLoaded();
}

void URTPlanWallPropertiesWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void URTPlanWallPropertiesWidget::NativeDestruct()
{
	if (SelectTool)
	{
		SelectTool->OnSelectionChanged.RemoveDynamic(this, &URTPlanWallPropertiesWidget::HandleSelectionChanged);
	}

	if (Document)
	{
		Document->OnPlanChanged.RemoveDynamic(this, &URTPlanWallPropertiesWidget::HandlePlanChanged);
	}

	Super::NativeDestruct();
}

void URTPlanWallPropertiesWidget::SetAvailableMaterials(const TArray<FRTMaterialEntry>& Materials)
{
	AvailableMaterials = Materials;
}

void URTPlanWallPropertiesWidget::ShowForSelection()
{
	SetVisibility(ESlateVisibility::Visible);
	OnWallDataChanged();
}

void URTPlanWallPropertiesWidget::HidePanel()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

// --- Unit Conversion ---

FText URTPlanWallPropertiesWidget::GetUnitDisplayName(ERTPlanUnit Unit)
{
	switch (Unit)
	{
	case ERTPlanUnit::Centimeters:
		return FText::FromString(TEXT("cm"));
	case ERTPlanUnit::Meters:
		return FText::FromString(TEXT("m"));
	case ERTPlanUnit::Inches:
		return FText::FromString(TEXT("in"));
	case ERTPlanUnit::Feet:
		return FText::FromString(TEXT("ft"));
	default:
		return FText::FromString(TEXT("cm"));
	}
}

float URTPlanWallPropertiesWidget::ConvertFromCm(float ValueCm, ERTPlanUnit ToUnit)
{
	switch (ToUnit)
	{
	case ERTPlanUnit::Centimeters:
		return ValueCm;
	case ERTPlanUnit::Meters:
		return ValueCm / RTPlanUnits::CmPerMeter;
	case ERTPlanUnit::Inches:
		return ValueCm / RTPlanUnits::CmPerInch;
	case ERTPlanUnit::Feet:
		return ValueCm / RTPlanUnits::CmPerFoot;
	default:
		return ValueCm;
	}
}

float URTPlanWallPropertiesWidget::ConvertToCm(float Value, ERTPlanUnit FromUnit)
{
	switch (FromUnit)
	{
	case ERTPlanUnit::Centimeters:
		return Value;
	case ERTPlanUnit::Meters:
		return Value * RTPlanUnits::CmPerMeter;
	case ERTPlanUnit::Inches:
		return Value * RTPlanUnits::CmPerInch;
	case ERTPlanUnit::Feet:
		return Value * RTPlanUnits::CmPerFoot;
	default:
		return Value;
	}
}

// --- Property Getters ---

const FRTWall* URTPlanWallPropertiesWidget::GetFirstSelectedWall() const
{
	if (!Document || !SelectTool)
	{
		return nullptr;
	}

	TArray<FGuid> SelectedWalls = SelectTool->GetSelectedWallIds();
	if (SelectedWalls.Num() == 0)
	{
		return nullptr;
	}

	return Document->GetData().Walls.Find(SelectedWalls[0]);
}

bool URTPlanWallPropertiesWidget::HasSelectedWall() const
{
	return GetFirstSelectedWall() != nullptr;
}

int32 URTPlanWallPropertiesWidget::GetSelectedWallCount() const
{
	if (!SelectTool)
	{
		return 0;
	}
	return SelectTool->GetSelectedWallIds().Num();
}

float URTPlanWallPropertiesWidget::GetWallThickness() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	if (!Wall)
	{
		return 0.0f;
	}
	return ConvertFromCm(Wall->ThicknessCm, CurrentDimensionUnit);
}

float URTPlanWallPropertiesWidget::GetWallHeight() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	if (!Wall)
	{
		return 0.0f;
	}
	return ConvertFromCm(Wall->HeightCm, CurrentDimensionUnit);
}

float URTPlanWallPropertiesWidget::GetBaseHeight() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	if (!Wall)
	{
		return 0.0f;
	}
	return ConvertFromCm(Wall->BaseZCm, CurrentDimensionUnit);
}

float URTPlanWallPropertiesWidget::GetLeftSkirtingHeight() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	if (!Wall)
	{
		return 0.0f;
	}
	return ConvertFromCm(Wall->LeftSkirtingHeightCm, CurrentSkirtingUnit);
}

float URTPlanWallPropertiesWidget::GetLeftSkirtingThickness() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	if (!Wall)
	{
		return 0.0f;
	}
	return ConvertFromCm(Wall->LeftSkirtingThicknessCm, CurrentSkirtingUnit);
}

float URTPlanWallPropertiesWidget::GetRightSkirtingHeight() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	if (!Wall)
	{
		return 0.0f;
	}
	return ConvertFromCm(Wall->RightSkirtingHeightCm, CurrentSkirtingUnit);
}

float URTPlanWallPropertiesWidget::GetRightSkirtingThickness() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	if (!Wall)
	{
		return 0.0f;
	}
	return ConvertFromCm(Wall->RightSkirtingThicknessCm, CurrentSkirtingUnit);
}

bool URTPlanWallPropertiesWidget::GetHasLeftSkirting() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	if (!Wall)
	{
		return false;
	}
	return Wall->bHasLeftSkirting;
}

bool URTPlanWallPropertiesWidget::GetHasRightSkirting() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	if (!Wall)
	{
		return false;
	}
	return Wall->bHasRightSkirting;
}

// --- Material Getters ---

FName URTPlanWallPropertiesWidget::GetLeftWallMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishLeftId : NAME_None;
}

FName URTPlanWallPropertiesWidget::GetRightWallMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishRightId : NAME_None;
}

FName URTPlanWallPropertiesWidget::GetLeftCapMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishLeftCapId : NAME_None;
}

FName URTPlanWallPropertiesWidget::GetRightCapMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishRightCapId : NAME_None;
}

FName URTPlanWallPropertiesWidget::GetTopMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishTopId : NAME_None;
}

FName URTPlanWallPropertiesWidget::GetLeftSkirtingMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishLeftSkirtingId : NAME_None;
}

FName URTPlanWallPropertiesWidget::GetLeftSkirtingTopMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishLeftSkirtingTopId : NAME_None;
}

FName URTPlanWallPropertiesWidget::GetLeftSkirtingCapMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishLeftSkirtingCapId : NAME_None;
}

FName URTPlanWallPropertiesWidget::GetRightSkirtingMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishRightSkirtingId : NAME_None;
}

FName URTPlanWallPropertiesWidget::GetRightSkirtingTopMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishRightSkirtingTopId : NAME_None;
}

FName URTPlanWallPropertiesWidget::GetRightSkirtingCapMaterial() const
{
	const FRTWall* Wall = GetFirstSelectedWall();
	return Wall ? Wall->FinishRightSkirtingCapId : NAME_None;
}

// --- Apply Changes Helper ---

void URTPlanWallPropertiesWidget::ApplyToSelectedWalls(TFunction<void(FRTWall&)> ModifyFunc, const FString& CommandDescription)
{
	if (!Document || !SelectTool || bIsUpdating)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyToSelectedWalls BLOCKED: Document=%s, SelectTool=%s, bIsUpdating=%d"),
			Document ? TEXT("OK") : TEXT("NULL"),
			SelectTool ? TEXT("OK") : TEXT("NULL"),
			bIsUpdating);
		return;
	}

	TArray<FGuid> SelectedWalls = SelectTool->GetSelectedWallIds();
	if (SelectedWalls.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyToSelectedWalls: No walls selected"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("ApplyToSelectedWalls: %s - %d wall(s) selected"), *CommandDescription, SelectedWalls.Num());

	bIsUpdating = true;

	// For multiple walls, use a macro command
	if (SelectedWalls.Num() > 1)
	{
		URTCmdMacro* MacroCmd = NewObject<URTCmdMacro>();
		MacroCmd->Document = Document;
		MacroCmd->Description = CommandDescription;

		for (const FGuid& WallId : SelectedWalls)
		{
			const FRTWall* OriginalWall = Document->GetData().Walls.Find(WallId);
			if (OriginalWall)
			{
				URTCmdAddWall* WallCmd = NewObject<URTCmdAddWall>();
				WallCmd->Wall = *OriginalWall;
				ModifyFunc(WallCmd->Wall);
				MacroCmd->AddCommand(WallCmd);
			}
		}

		Document->SubmitCommand(MacroCmd);
	}
	else
	{
		// Single wall
		const FRTWall* OriginalWall = Document->GetData().Walls.Find(SelectedWalls[0]);
		if (OriginalWall)
		{
			URTCmdAddWall* WallCmd = NewObject<URTCmdAddWall>();
			WallCmd->Document = Document;
			WallCmd->Wall = *OriginalWall;
			ModifyFunc(WallCmd->Wall);
			
			UE_LOG(LogTemp, Log, TEXT("  Submitting wall command for wall %s"), *SelectedWalls[0].ToString());
			Document->SubmitCommand(WallCmd);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("  Wall %s not found in document!"), *SelectedWalls[0].ToString());
		}
	}

	bIsUpdating = false;
}

// --- Property Setters ---

void URTPlanWallPropertiesWidget::SetWallThickness(float Value)
{
	float ValueCm = ConvertToCm(Value, CurrentDimensionUnit);
	ApplyToSelectedWalls([ValueCm](FRTWall& Wall) {
		Wall.ThicknessCm = ValueCm;
	}, TEXT("Change Wall Thickness"));
}

void URTPlanWallPropertiesWidget::SetWallHeight(float Value)
{
	float ValueCm = ConvertToCm(Value, CurrentDimensionUnit);
	ApplyToSelectedWalls([ValueCm](FRTWall& Wall) {
		Wall.HeightCm = ValueCm;
	}, TEXT("Change Wall Height"));
}

void URTPlanWallPropertiesWidget::SetBaseHeight(float Value)
{
	float ValueCm = ConvertToCm(Value, CurrentDimensionUnit);
	ApplyToSelectedWalls([ValueCm](FRTWall& Wall) {
		Wall.BaseZCm = ValueCm;
	}, TEXT("Change Wall Base Height"));
}

void URTPlanWallPropertiesWidget::SetLeftSkirtingHeight(float Value)
{
	float ValueCm = ConvertToCm(Value, CurrentSkirtingUnit);
	ApplyToSelectedWalls([ValueCm](FRTWall& Wall) {
		Wall.LeftSkirtingHeightCm = ValueCm;
	}, TEXT("Change Left Skirting Height"));
}

void URTPlanWallPropertiesWidget::SetLeftSkirtingThickness(float Value)
{
	float ValueCm = ConvertToCm(Value, CurrentSkirtingUnit);
	ApplyToSelectedWalls([ValueCm](FRTWall& Wall) {
		Wall.LeftSkirtingThicknessCm = ValueCm;
	}, TEXT("Change Left Skirting Thickness"));
}

void URTPlanWallPropertiesWidget::SetRightSkirtingHeight(float Value)
{
	float ValueCm = ConvertToCm(Value, CurrentSkirtingUnit);
	ApplyToSelectedWalls([ValueCm](FRTWall& Wall) {
		Wall.RightSkirtingHeightCm = ValueCm;
	}, TEXT("Change Right Skirting Height"));
}

void URTPlanWallPropertiesWidget::SetRightSkirtingThickness(float Value)
{
	float ValueCm = ConvertToCm(Value, CurrentSkirtingUnit);
	ApplyToSelectedWalls([ValueCm](FRTWall& Wall) {
		Wall.RightSkirtingThicknessCm = ValueCm;
	}, TEXT("Change Right Skirting Thickness"));
}

void URTPlanWallPropertiesWidget::SetHasLeftSkirting(bool bValue)
{
	ApplyToSelectedWalls([bValue](FRTWall& Wall) {
		Wall.bHasLeftSkirting = bValue;
	}, TEXT("Toggle Left Skirting"));
}

void URTPlanWallPropertiesWidget::SetHasRightSkirting(bool bValue)
{
	ApplyToSelectedWalls([bValue](FRTWall& Wall) {
		Wall.bHasRightSkirting = bValue;
	}, TEXT("Toggle Right Skirting"));
}

// --- Material Setters ---

void URTPlanWallPropertiesWidget::SetLeftWallMaterial(FName MaterialId)
{
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishLeftId = MaterialId;
	}, TEXT("Change Left Wall Material"));
}

void URTPlanWallPropertiesWidget::SetRightWallMaterial(FName MaterialId)
{
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishRightId = MaterialId;
	}, TEXT("Change Right Wall Material"));
}

void URTPlanWallPropertiesWidget::SetLeftCapMaterial(FName MaterialId)
{
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishLeftCapId = MaterialId;
	}, TEXT("Change Left Cap Material"));
}

void URTPlanWallPropertiesWidget::SetRightCapMaterial(FName MaterialId)
{
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishRightCapId = MaterialId;
	}, TEXT("Change Right Cap Material"));
}

void URTPlanWallPropertiesWidget::SetTopMaterial(FName MaterialId)
{
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishTopId = MaterialId;
	}, TEXT("Change Top Material"));
}

void URTPlanWallPropertiesWidget::SetLeftSkirtingMaterial(FName MaterialId)
{
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishLeftSkirtingId = MaterialId;
	}, TEXT("Change Left Skirting Material"));
}

void URTPlanWallPropertiesWidget::SetLeftSkirtingTopMaterial(FName MaterialId)
{
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishLeftSkirtingTopId = MaterialId;
	}, TEXT("Change Left Skirting Top Material"));
}

void URTPlanWallPropertiesWidget::SetLeftSkirtingCapMaterial(FName MaterialId)
{
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishLeftSkirtingCapId = MaterialId;
	}, TEXT("Change Left Skirting Cap Material"));
}

void URTPlanWallPropertiesWidget::SetRightSkirtingMaterial(FName MaterialId)
{
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishRightSkirtingId = MaterialId;
	}, TEXT("Change Right Skirting Material"));
}

void URTPlanWallPropertiesWidget::SetRightSkirtingTopMaterial(FName MaterialId)
{
	UE_LOG(LogTemp, Log, TEXT("SetRightSkirtingTopMaterial called with MaterialId: %s"), *MaterialId.ToString());
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishRightSkirtingTopId = MaterialId;
	}, TEXT("Change Right Skirting Top Material"));
}

void URTPlanWallPropertiesWidget::SetRightSkirtingCapMaterial(FName MaterialId)
{
	ApplyToSelectedWalls([MaterialId](FRTWall& Wall) {
		Wall.FinishRightSkirtingCapId = MaterialId;
	}, TEXT("Change Right Skirting Cap Material"));
}

// --- Unit Selection ---

void URTPlanWallPropertiesWidget::SetDimensionUnit(ERTPlanUnit NewUnit)
{
	if (CurrentDimensionUnit != NewUnit)
	{
		CurrentDimensionUnit = NewUnit;
		OnUnitChanged();
	}
}

void URTPlanWallPropertiesWidget::SetSkirtingUnit(ERTPlanUnit NewUnit)
{
	if (CurrentSkirtingUnit != NewUnit)
	{
		CurrentSkirtingUnit = NewUnit;
		OnUnitChanged();
	}
}

// --- Callbacks ---

void URTPlanWallPropertiesWidget::HandleSelectionChanged()
{
	if (HasSelectedWall())
	{
		ShowForSelection();
	}
	else
	{
		HidePanel();
	}
	OnSelectionChanged();
}

void URTPlanWallPropertiesWidget::HandlePlanChanged()
{
	if (!bIsUpdating)
	{
		OnWallDataChanged();
	}
}

// --- Material Lookup ---

bool URTPlanWallPropertiesWidget::FindMaterialById(FName MaterialId, FRTMaterialEntry& OutEntry) const
{
	for (const FRTMaterialEntry& Entry : AvailableMaterials)
	{
		if (Entry.MaterialId == MaterialId)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}
