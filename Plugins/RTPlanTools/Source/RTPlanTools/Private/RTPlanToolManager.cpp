#include "RTPlanToolManager.h"

void URTPlanToolManager::Initialize(URTPlanDocument* InDoc)
{
	Document = InDoc;
	if (Document)
	{
		// Listen for changes to rebuild the index
		Document->OnPlanChanged.AddDynamic(this, &URTPlanToolManager::UpdateSpatialIndex);
	}
	UpdateSpatialIndex();
}

void URTPlanToolManager::SelectTool(TSubclassOf<URTPlanToolBase> ToolClass)
{
	if (ActiveTool)
	{
		ActiveTool->OnExit();
		ActiveTool = nullptr;
	}

	if (ToolClass)
	{
		ActiveTool = NewObject<URTPlanToolBase>(this, ToolClass);
		ActiveTool->Init(Document, &SpatialIndex);
		ActiveTool->OnEnter();
	}
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
