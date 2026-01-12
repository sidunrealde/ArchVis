#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTPlanVRModeManager.generated.h"

UENUM(BlueprintType)
enum class ERTVRMode : uint8
{
	Walk,
	Tabletop
};

/**
 * Manages VR modes (Walk vs Tabletop).
 * Handles scaling the world or the player.
 */
UCLASS()
class RTPLANVR_API ARTPlanVRModeManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ARTPlanVRModeManager();

	UFUNCTION(BlueprintCallable, Category = "RTPlan|VR")
	void SetMode(ERTVRMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|VR")
	void SetTabletopTransform(const FTransform& Transform);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTPlan|VR")
	ERTVRMode CurrentMode = ERTVRMode::Walk;

	// Where the "Table" is located in the real world (relative to VROrigin)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTPlan|VR")
	FTransform TabletopTransform;

	// Scale factor for tabletop (e.g. 1:20)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RTPlan|VR")
	float TabletopScale = 0.05f;
};
