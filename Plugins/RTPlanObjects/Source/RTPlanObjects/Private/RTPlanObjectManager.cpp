#include "RTPlanObjectManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"

ARTPlanObjectManager::ARTPlanObjectManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARTPlanObjectManager::BeginPlay()
{
	Super::BeginPlay();
}

void ARTPlanObjectManager::SetDocument(URTPlanDocument* InDoc)
{
	if (Document)
	{
		Document->OnPlanChanged.RemoveDynamic(this, &ARTPlanObjectManager::OnPlanChanged);
	}

	Document = InDoc;

	if (Document)
	{
		Document->OnPlanChanged.AddDynamic(this, &ARTPlanObjectManager::OnPlanChanged);
		RebuildAll();
	}
}

void ARTPlanObjectManager::SetCatalog(URTProductCatalog* InCatalog)
{
	Catalog = InCatalog;
	RebuildAll();
}

void ARTPlanObjectManager::OnPlanChanged()
{
	RebuildAll();
}

void ARTPlanObjectManager::RebuildAll()
{
	if (!Document || !Catalog) return;

	const FRTPlanData& Data = Document->GetData();
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. Identify objects to remove
	TArray<FGuid> ToRemove;
	for (const auto& Pair : SpawnedObjects)
	{
		if (!Data.Objects.Contains(Pair.Key))
		{
			ToRemove.Add(Pair.Key);
		}
	}

	for (const FGuid& Id : ToRemove)
	{
		if (AActor* Actor = SpawnedObjects[Id])
		{
			Actor->Destroy();
		}
		SpawnedObjects.Remove(Id);
	}

	// 2. Create or Update objects
	for (const auto& Pair : Data.Objects)
	{
		const FRTInteriorInstance& Instance = Pair.Value;
		
		AActor* ExistingActor = SpawnedObjects.FindRef(Instance.Id);
		
		if (ExistingActor)
		{
			// Update Transform
			ExistingActor->SetActorTransform(Instance.Transform);
		}
		else
		{
			// Spawn New
			const FRTProductDefinition* Product = Catalog->FindProduct(Instance.ProductTypeId);
			if (Product)
			{
				// For V1, we just spawn a StaticMeshActor.
				// In a real app, we might use a custom BP class or HISM.
				AStaticMeshActor* NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Instance.Transform);
				
				if (NewActor)
				{
					NewActor->SetMobility(EComponentMobility::Movable);
					
					// Load Mesh (Synchronous for V1 prototype, Async recommended for prod)
					if (UStaticMesh* Mesh = Product->Mesh.LoadSynchronous())
					{
						NewActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
					}

					SpawnedObjects.Add(Instance.Id, NewActor);
				}
			}
		}
	}
}
