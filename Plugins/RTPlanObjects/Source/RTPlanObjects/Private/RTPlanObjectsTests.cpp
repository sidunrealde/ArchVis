#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "RTPlanObjectManager.h"
#include "RTPlanDocument.h"
#include "RTPlanCatalogTypes.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h" // Required for TActorIterator

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanObjectsSpawnTest, "ArchVis.RTPlanObjects.Spawning", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanObjectsSpawnTest::RunTest(const FString& Parameters)
{
	// Setup World
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);

	// Setup Catalog
	URTProductCatalog* Catalog = NewObject<URTProductCatalog>();
	FRTProductDefinition Def;
	Def.Id = FName("Chair");
	// Note: Mesh is null for test, but logic should still run
	Catalog->Products.Add(Def);

	// Setup Document
	URTPlanDocument* Doc = NewObject<URTPlanDocument>();
	FRTPlanData& Data = Doc->GetDataMutable();

	// Add Object
	FRTInteriorInstance Obj;
	Obj.Id = FGuid::NewGuid();
	Obj.ProductTypeId = FName("Chair");
	Obj.Transform.SetLocation(FVector(100, 100, 0));
	Data.Objects.Add(Obj.Id, Obj);

	// Spawn Manager
	ARTPlanObjectManager* Manager = World->SpawnActor<ARTPlanObjectManager>();
	Manager->SetCatalog(Catalog);
	Manager->SetDocument(Doc);

	// Verify Actor Spawned
	// Since we don't have public access to SpawnedObjects, we can check World actor count or similar.
	// Or we can make SpawnedObjects public/protected for test (friend class).
	// For now, let's just check if any StaticMeshActor exists at that location.
	
	bool bFound = false;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		if (It->GetActorLocation().Equals(FVector(100, 100, 0)))
		{
			bFound = true;
			break;
		}
	}

	TestTrue("Object Actor Spawned", bFound);

	// Cleanup
	World->DestroyWorld(false);

	return true;
}
