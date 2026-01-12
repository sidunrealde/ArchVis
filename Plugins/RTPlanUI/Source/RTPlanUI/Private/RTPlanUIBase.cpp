#include "RTPlanUIBase.h"

void URTPlanUIBase::InitializeContext(URTPlanDocument* InDoc, URTPlanToolManager* InToolMgr)
{
	Document = InDoc;
	ToolManager = InToolMgr;
	OnContextInitialized();
}
