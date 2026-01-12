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
