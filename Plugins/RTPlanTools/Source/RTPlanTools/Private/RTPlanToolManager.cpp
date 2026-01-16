#include "RTPlanToolManager.h"
#include "Tools/RTPlanSelectTool.h"
#include "Tools/RTPlanLineTool.h"

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
	}
}

void URTPlanToolManager::SelectToolByType(ERTPlanToolType ToolType)
{
	UE_LOG(LogTemp, Log, TEXT("SelectToolByType: %d, CachedSelectTool=%s"), 
		static_cast<int32>(ToolType), 
		CachedSelectTool ? TEXT("Valid") : TEXT("NULL"));
	
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
			UE_LOG(LogTemp, Log, TEXT("Select tool activated"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("CachedSelectTool is NULL! Creating new one."));
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
