#include "RTPlanNetDriver.h"
#include "Net/UnrealNetwork.h"
#include "RTPlanCommand.h"

ARTPlanNetDriver::ARTPlanNetDriver()
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

void ARTPlanNetDriver::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ARTPlanNetDriver, ReplicatedPlanJson);
}

void ARTPlanNetDriver::BeginPlay()
{
	Super::BeginPlay();
}

void ARTPlanNetDriver::SetDocument(URTPlanDocument* InDoc)
{
	if (Document)
	{
		Document->OnPlanChanged.RemoveDynamic(this, &ARTPlanNetDriver::OnPlanChanged);
	}

	Document = InDoc;

	if (Document)
	{
		Document->OnPlanChanged.AddDynamic(this, &ARTPlanNetDriver::OnPlanChanged);
		
		// Initial Sync (Server -> Client)
		if (HasAuthority())
		{
			OnPlanChanged();
		}
	}
}

void ARTPlanNetDriver::OnPlanChanged()
{
	if (HasAuthority() && Document)
	{
		// Serialize and update replicated property
		// Debounce to avoid spamming network on drag operations
		GetWorld()->GetTimerManager().SetTimer(UpdateTimer, [this]()
		{
			if (Document)
			{
				ReplicatedPlanJson = Document->ToJson();
				// Force update if needed, though RepNotify handles clients
			}
		}, 0.1f, false);
	}
}

void ARTPlanNetDriver::OnRep_PlanJson()
{
	if (!HasAuthority() && Document)
	{
		// Client receives update
		Document->FromJson(ReplicatedPlanJson);
	}
}

// --- Server RPCs ---

void ARTPlanNetDriver::Server_SubmitAddWall_Implementation(FRTWall Wall)
{
	if (Document)
	{
		URTCmdAddWall* Cmd = NewObject<URTCmdAddWall>();
		Cmd->Wall = Wall;
		Document->SubmitCommand(Cmd);
	}
}

bool ARTPlanNetDriver::Server_SubmitAddWall_Validate(FRTWall Wall)
{
	return true; // Add permission checks here
}

void ARTPlanNetDriver::Server_SubmitDeleteWall_Implementation(FGuid WallId)
{
	if (Document)
	{
		URTCmdDeleteWall* Cmd = NewObject<URTCmdDeleteWall>();
		Cmd->WallId = WallId;
		Document->SubmitCommand(Cmd);
	}
}

bool ARTPlanNetDriver::Server_SubmitDeleteWall_Validate(FGuid WallId)
{
	return true;
}
