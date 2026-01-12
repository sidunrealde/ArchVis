#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ArchVisHUD.generated.h"

/**
 * HUD for drafting. Draws the crosshair cursor.
 */
UCLASS()
class ARCHVIS_API AArchVisHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	// Crosshair settings
	UPROPERTY(EditAnywhere, Category = "Drafting")
	FLinearColor CrosshairColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Drafting")
	float CrosshairThickness = 1.0f;
};
