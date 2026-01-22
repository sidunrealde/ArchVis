#include "RTPlanToolbar.h"
#include "RTPlanToolManager.h"

void URTPlanToolbar::SetToolManager(URTPlanToolManager* InMgr)
{
	ToolManager = InMgr;
}

void URTPlanToolbar::SelectTool(TSubclassOf<URTPlanToolBase> ToolClass)
{
	if (ToolManager)
	{
		ToolManager->SelectTool(ToolClass);
	}
}

void URTPlanToolbar::SelectToolByType(ERTPlanToolType ToolType)
{
	if (ToolManager)
	{
		ToolManager->SelectToolByType(ToolType);
	}
}

ERTPlanToolType URTPlanToolbar::GetActiveToolType() const
{
	if (ToolManager)
	{
		return ToolManager->GetActiveToolType();
	}
	return ERTPlanToolType::None;
}

void URTPlanToolbar::ToggleSnap()
{
	if (ToolManager)
	{
		ToolManager->ToggleSnap();
	}
}

void URTPlanToolbar::ToggleGrid()
{
	if (ToolManager)
	{
		ToolManager->ToggleGrid();
	}
}

bool URTPlanToolbar::IsSnapEnabled() const
{
	if (ToolManager)
	{
		return ToolManager->IsSnapEnabled();
	}
	return false;
}

bool URTPlanToolbar::IsGridEnabled() const
{
	if (ToolManager)
	{
		return ToolManager->IsGridEnabled();
	}
	return false;
}
