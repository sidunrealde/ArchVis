#include "RTPlanToolManager.h"
#include "Tools/RTPlanSelectTool.h"
#include "Tools/RTPlanLineTool.h"
#include "Tools/RTPlanTrimTool.h"
#include "RTPlanCommand.h"

void URTPlanToolManager::Initialize(URTPlanDocument* InDoc)
{
	Document = InDoc;
	if (Document)
	{
		// Listen for changes to rebuild the index
		Document->OnPlanChanged.AddDynamic(this, &URTPlanToolManager::UpdateSpatialIndex);
	}
	
	UpdateSpatialIndex();
	
	// Create and cache the select tool for selection state queries
	CachedSelectTool = NewObject<URTPlanSelectTool>(this);
	CachedSelectTool->Init(Document, &SpatialIndex);
}

void URTPlanToolManager::SelectTool(TSubclassOf<URTPlanToolBase> ToolClass)
{
	if (ActiveTool)
	{
		ActiveTool->OnExit();
		ActiveTool = nullptr;
	}
	
	ActiveToolType = ERTPlanToolType::None;

	if (ToolClass)
	{
		ActiveTool = NewObject<URTPlanToolBase>(this, ToolClass);
		ActiveTool->Init(Document, &SpatialIndex);
		ActiveTool->OnEnter();
		
		// Determine tool type
		if (ToolClass->IsChildOf(URTPlanSelectTool::StaticClass()))
		{
			ActiveToolType = ERTPlanToolType::Select;
		}
		else if (ToolClass->IsChildOf(URTPlanLineTool::StaticClass()))
		{
			// Check if it's in polyline mode (we'd need to check after creation)
			ActiveToolType = ERTPlanToolType::Line;
		}
		else if (ToolClass->IsChildOf(URTPlanTrimTool::StaticClass()))
		{
			ActiveToolType = ERTPlanToolType::Trim;
		}
	}
}

void URTPlanToolManager::SelectToolByType(ERTPlanToolType ToolType)
{
	if (ActiveTool)
	{
		ActiveTool->OnExit();
		ActiveTool = nullptr;
	}
	
	ActiveToolType = ToolType;

	switch (ToolType)
	{
	case ERTPlanToolType::Select:
		// Use the cached select tool to preserve selection state
		ActiveTool = CachedSelectTool;
		if (ActiveTool)
		{
			ActiveTool->OnEnter();
		}
		else
		{
			CachedSelectTool = NewObject<URTPlanSelectTool>(this);
			CachedSelectTool->Init(Document, &SpatialIndex);
			ActiveTool = CachedSelectTool;
			ActiveTool->OnEnter();
		}
		break;
		
	case ERTPlanToolType::Line:
		ActiveTool = NewObject<URTPlanLineTool>(this);
		ActiveTool->Init(Document, &SpatialIndex);
		if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ActiveTool))
		{
			LineTool->SetPolylineMode(false);
		}
		ActiveTool->OnEnter();
		break;
		
	case ERTPlanToolType::Polyline:
		ActiveTool = NewObject<URTPlanLineTool>(this);
		ActiveTool->Init(Document, &SpatialIndex);
		if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ActiveTool))
		{
			LineTool->SetPolylineMode(true);
		}
		ActiveTool->OnEnter();
		break;

	case ERTPlanToolType::Trim:
		ActiveTool = NewObject<URTPlanTrimTool>(this);
		ActiveTool->Init(Document, &SpatialIndex);
		ActiveTool->OnEnter();
		break;
		
	case ERTPlanToolType::None:
	default:
		ActiveTool = nullptr;
		break;
	}
}

URTPlanSelectTool* URTPlanToolManager::GetSelectTool() const
{
	return CachedSelectTool;
}

TArray<FGuid> URTPlanToolManager::GetSelectedWallIds() const
{
	if (CachedSelectTool)
	{
		return CachedSelectTool->GetSelectedWallIds();
	}
	return TArray<FGuid>();
}

TArray<FGuid> URTPlanToolManager::GetSelectedOpeningIds() const
{
	if (CachedSelectTool)
	{
		return CachedSelectTool->GetSelectedOpeningIds();
	}
	return TArray<FGuid>();
}

void URTPlanToolManager::DeleteSelection()
{
	if (!Document || !CachedSelectTool) return;

	TArray<FGuid> SelectedWalls = CachedSelectTool->GetSelectedWallIds();
	TArray<FGuid> SelectedOpenings = CachedSelectTool->GetSelectedOpeningIds();

	if (SelectedWalls.Num() == 0 && SelectedOpenings.Num() == 0)
	{
		return;
	}

	// TODO: Group into a transaction/macro command if we want single undo step
	// For now, just submit individual commands

	for (const FGuid& WallId : SelectedWalls)
	{
		URTCmdDeleteWall* Cmd = NewObject<URTCmdDeleteWall>();
		Cmd->WallId = WallId;
		Document->SubmitCommand(Cmd);
	}

	// TODO: Implement URTCmdDeleteOpening and handle opening deletion
	/*
	for (const FGuid& OpeningId : SelectedOpenings)
	{
		URTCmdDeleteOpening* Cmd = NewObject<URTCmdDeleteOpening>();
		Cmd->OpeningId = OpeningId;
		Document->SubmitCommand(Cmd);
	}
	*/

	// Clear selection after deletion
	CachedSelectTool->ClearSelection();
}

void URTPlanToolManager::ProcessInput(const FRTPointerEvent& Event)
{
	if (ActiveTool)
	{
		ActiveTool->OnPointerEvent(Event);
	}
}

void URTPlanToolManager::UpdateSpatialIndex()
{
	if (Document)
	{
		SpatialIndex.Build(Document);
	}
}

void URTPlanToolManager::ToggleSnap()
{
	bSnapEnabled = !bSnapEnabled;
}

void URTPlanToolManager::SetSnapEnabled(bool bEnabled)
{
	bSnapEnabled = bEnabled;
}

void URTPlanToolManager::ToggleGrid()
{
	bGridEnabled = !bGridEnabled;
}

void URTPlanToolManager::SetGridEnabled(bool bEnabled)
{
	bGridEnabled = bEnabled;
}
