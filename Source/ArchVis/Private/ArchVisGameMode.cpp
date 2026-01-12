#include "ArchVisGameMode.h"
#include "RTPlanShellActor.h"
#include "RTPlanNetDriver.h"
#include "ArchVisHUD.h"
#include "ArchVisPlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogArchVisGM, Log, All);

AArchVisGameMode::AArchVisGameMode()
{
	// Set default pawn class to our custom controller/pawn if needed
	PlayerControllerClass = AArchVisPlayerController::StaticClass();
	HUDClass = AArchVisHUD::StaticClass();
}

void AArchVisGameMode::StartPlay()
{
	Super::StartPlay();

	UE_LOG(LogArchVisGM, Log, TEXT("AArchVisGameMode::StartPlay - Initializing System"));

	// 1. Create Document
	Document = NewObject<URTPlanDocument>(this);
	if (Document)
	{
		UE_LOG(LogArchVisGM, Log, TEXT("  - Document Created Successfully"));
	}
	else
	{
		UE_LOG(LogArchVisGM, Error, TEXT("  - Failed to create Document!"));
	}

	// 2. Create Tool Manager
	ToolManager = NewObject<URTPlanToolManager>(this);
	if (ToolManager)
	{
		ToolManager->Initialize(Document);
		UE_LOG(LogArchVisGM, Log, TEXT("  - ToolManager Created and Initialized"));
	}
	else
	{
		UE_LOG(LogArchVisGM, Error, TEXT("  - Failed to create ToolManager!"));
	}

	// 3. Spawn Shell Actor (Renderer)
	FVector Location(0.0f, 0.0f, 0.0f);
	FRotator Rotation(0.0f, 0.0f, 0.0f);
	ARTPlanShellActor* Shell = GetWorld()->SpawnActor<ARTPlanShellActor>(ARTPlanShellActor::StaticClass(), Location, Rotation);
	if (Shell)
	{
		Shell->SetDocument(Document);
		UE_LOG(LogArchVisGM, Log, TEXT("  - ShellActor Spawned and Linked"));
	}
	else
	{
		UE_LOG(LogArchVisGM, Error, TEXT("  - Failed to spawn ShellActor!"));
	}

	// 4. Spawn Net Driver (if needed for replication)
	ARTPlanNetDriver* NetDriver = GetWorld()->SpawnActor<ARTPlanNetDriver>(ARTPlanNetDriver::StaticClass(), Location, Rotation);
	if (NetDriver)
	{
		NetDriver->SetDocument(Document);
		UE_LOG(LogArchVisGM, Log, TEXT("  - NetDriver Spawned and Linked"));
	}
	else
	{
		UE_LOG(LogArchVisGM, Warning, TEXT("  - Failed to spawn NetDriver (Optional)"));
	}
}
