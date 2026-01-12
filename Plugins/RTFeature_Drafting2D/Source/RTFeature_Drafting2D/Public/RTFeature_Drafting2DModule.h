#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FRTFeature_Drafting2DModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};