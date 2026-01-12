#include "RTPlanVRModeManager.h"
#include "Kismet/GameplayStatics.h"
#include "RTPlanVRPawn.h"

ARTPlanVRModeManager::ARTPlanVRModeManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARTPlanVRModeManager::SetTabletopTransform(const FTransform& Transform)
{
	TabletopTransform = Transform;
	if (CurrentMode == ERTVRMode::Tabletop)
	{
		SetMode(ERTVRMode::Tabletop); // Refresh
	}
}

void ARTPlanVRModeManager::SetMode(ERTVRMode NewMode)
{
	CurrentMode = NewMode;

	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn) return;

	// In Walk mode, WorldToMeters is standard (100).
	// In Tabletop mode, we scale the world down (or scale the player up).
	// Scaling the player (VROrigin) is usually safer for physics/nav.
	
	// Actually, for Tabletop, we want the plan to appear small.
	// So we can scale the VROrigin UP (making the world look small).
	// Or we can have a separate "Tabletop" actor that renders the plan in miniature.
	// The requirement says "VR tabletop drafting mode".
	
	// Approach A: Scale VROrigin
	// If Scale = 20, then 1 unit of motion = 20 units in world.
	// World looks 1/20th size.
	
	if (NewMode == ERTVRMode::Walk)
	{
		Pawn->SetActorScale3D(FVector(1.0f));
		// Reset location to last walk position?
	}
	else if (NewMode == ERTVRMode::Tabletop)
	{
		float InverseScale = 1.0f / TabletopScale; // e.g. 20
		Pawn->SetActorScale3D(FVector(InverseScale));
		
		// Position Pawn so that the plan center is at the table transform
		// This math depends on where the plan is (usually 0,0,0).
		// If Plan is at 0,0,0, and we want it to appear at TabletopTransform relative to HMD...
		// This is complex. 
		// Simpler V1: Just scale the pawn and let user teleport/fly.
	}
}
