#include "ArchVisHUD.h"
#include "Engine/Canvas.h"
#include "ArchVisGameMode.h"
#include "RTPlanToolManager.h"
#include "ArchVisPlayerController.h"
#include "Kismet/GameplayStatics.h"

void AArchVisHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	FVector2D CrosshairPos;
	bool bShouldDraw = false;

	// 1. Try to get Snapped Position from Active Tool
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanToolBase* Tool = ToolMgr->GetActiveTool())
			{
				FVector SnappedWorld = Tool->GetSnappedCursorPos();
				// Project World to Screen
				// AHUD::Project returns FVector (ScreenX, ScreenY, 0)
				FVector ScreenPos = Project(SnappedWorld);
				
				// Check if projection is valid (e.g. in front of camera)
				// AHUD::Project doesn't return bool, it just projects.
				// We assume it's valid if we are looking at the plan.
				CrosshairPos = FVector2D(ScreenPos.X, ScreenPos.Y);
				bShouldDraw = true;
			}
		}
	}

	// 2. Fallback to Virtual Cursor Position
	if (!bShouldDraw)
	{
		if (AArchVisPlayerController* PC = Cast<AArchVisPlayerController>(GetOwningPlayerController()))
		{
			CrosshairPos = PC->GetVirtualCursorPos();
			bShouldDraw = true;
		}
	}

	if (bShouldDraw)
	{
		// Draw Full Screen Crosshair
		float ScreenW = Canvas->ClipX;
		float ScreenH = Canvas->ClipY;

		// Horizontal Line
		DrawLine(0, CrosshairPos.Y, ScreenW, CrosshairPos.Y, CrosshairColor, CrosshairThickness);

		// Vertical Line
		DrawLine(CrosshairPos.X, 0, CrosshairPos.X, ScreenH, CrosshairColor, CrosshairThickness);
	}
}
