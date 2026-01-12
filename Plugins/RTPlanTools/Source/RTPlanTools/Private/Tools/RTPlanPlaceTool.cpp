#include "Tools/RTPlanPlaceTool.h"
#include "RTPlanCommand.h"
#include "RTPlanObjectManager.h" // Assuming we can access catalog via some global or context
#include "Engine/StaticMeshActor.h"

// Note: In a real implementation, the Tool needs access to the Catalog.
// We might pass it via Init or look it up from a subsystem.
// For this prototype, we'll assume the UI sets the product ID and we just emit the command.

void URTPlanPlaceTool::OnEnter()
{
	// Spawn Preview Actor if needed
}

void URTPlanPlaceTool::OnExit()
{
	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}
}

void URTPlanPlaceTool::SetProduct(FName ProductId)
{
	CurrentProductId = ProductId;
	// Update preview mesh...
}

void URTPlanPlaceTool::OnPointerEvent(const FRTPointerEvent& Event)
{
	FVector GroundPoint;
	if (!GetGroundIntersection(Event, GroundPoint))
	{
		return;
	}

	// Update Preview Transform
	if (PreviewActor)
	{
		PreviewActor->SetActorLocation(GroundPoint);
	}

	if (Event.Action == ERTPointerAction::Up && !CurrentProductId.IsNone())
	{
		// Place Object
		FRTInteriorInstance NewObj;
		NewObj.Id = FGuid::NewGuid();
		NewObj.ProductTypeId = CurrentProductId;
		NewObj.Transform.SetLocation(GroundPoint);
		NewObj.HostType = ERTHostType::Floor;

		// TODO: Add command for Adding Object
		// URTCmdAddObject* Cmd = NewObject<URTCmdAddObject>();
		// Cmd->Object = NewObj;
		// Document->SubmitCommand(Cmd);
	}
}
