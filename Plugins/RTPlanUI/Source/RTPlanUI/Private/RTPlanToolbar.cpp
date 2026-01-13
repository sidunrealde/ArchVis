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

