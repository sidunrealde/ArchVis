// RTPlanSubsystem.cpp

#include "RTPlanSubsystem.h"
#include "RTPlanDocument.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void URTPlanSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("RTPlanSubsystem initialized"));
}

void URTPlanSubsystem::Deinitialize()
{
	Document = nullptr;
	ToolManager = nullptr;

	UE_LOG(LogTemp, Log, TEXT("RTPlanSubsystem deinitialized"));

	Super::Deinitialize();
}

URTPlanSubsystem* URTPlanSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<URTPlanSubsystem>();
}

URTPlanDocument* URTPlanSubsystem::GetDocument()
{
	if (!Document)
	{
		// Create a new document on demand
		Document = NewObject<URTPlanDocument>(this, NAME_None, RF_Transient);
		OnDocumentChanged.Broadcast(Document);
		UE_LOG(LogTemp, Log, TEXT("RTPlanSubsystem: Created new document"));
	}
	return Document;
}

void URTPlanSubsystem::SetDocument(URTPlanDocument* InDocument)
{
	if (Document != InDocument)
	{
		Document = InDocument;
		OnDocumentChanged.Broadcast(Document);
		UE_LOG(LogTemp, Log, TEXT("RTPlanSubsystem: Document set"));
	}
}

void URTPlanSubsystem::SetToolManager(UObject* InToolManager)
{
	if (ToolManager != InToolManager)
	{
		ToolManager = InToolManager;
		OnToolManagerChanged.Broadcast(ToolManager);
		UE_LOG(LogTemp, Log, TEXT("RTPlanSubsystem: Tool manager set"));
	}
}
