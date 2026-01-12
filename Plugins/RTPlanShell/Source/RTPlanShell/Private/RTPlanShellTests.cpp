#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "RTPlanShellActor.h"
#include "RTPlanDocument.h"
#include "Components/DynamicMeshComponent.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "UDynamicMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanShellGenerationTest, "ArchVis.RTPlanShell.Generation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanShellGenerationTest::RunTest(const FString& Parameters)
{
	// Setup World (needed for Actor)
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	
	// Setup Document
	URTPlanDocument* Doc = NewObject<URTPlanDocument>();
	FRTPlanData& Data = Doc->GetDataMutable();

	// Add Wall 200cm long, 20cm thick, 300cm high
	FRTVertex V1; V1.Id = FGuid::NewGuid(); V1.Position = FVector2D(0, 0);
	FRTVertex V2; V2.Id = FGuid::NewGuid(); V2.Position = FVector2D(200, 0);
	FRTWall W1; W1.Id = FGuid::NewGuid(); W1.VertexAId = V1.Id; W1.VertexBId = V2.Id;
	W1.ThicknessCm = 20.0f;
	W1.HeightCm = 300.0f;

	Data.Vertices.Add(V1.Id, V1);
	Data.Vertices.Add(V2.Id, V2);
	Data.Walls.Add(W1.Id, W1);

	// Spawn Actor
	ARTPlanShellActor* ShellActor = World->SpawnActor<ARTPlanShellActor>();
	ShellActor->SetDocument(Doc);

	// Verify Mesh Generated
	UDynamicMeshComponent* MeshComp = ShellActor->FindComponentByClass<UDynamicMeshComponent>();
	TestNotNull("Mesh Component Exists", MeshComp);

	UDynamicMesh* Mesh = MeshComp->GetDynamicMesh();
	TestNotNull("Dynamic Mesh Exists", Mesh);

	// Check Triangle Count (Box = 12 triangles)
	int32 TriCount = UGeometryScriptLibrary_MeshQueryFunctions::GetNumTriangleIDs(Mesh);
	TestEqual("Triangle Count", TriCount, 12);

	// Check Bounds
	FBox Bounds = MeshComp->Bounds.GetBox();
	// X: 0 to 200 (Length)
	// Y: -10 to 10 (Thickness 20 centered)
	// Z: 0 to 300 (Height)
	
	// Note: Bounds might be slightly loose or transformed.
	// Let's check dimensions roughly.
	FVector Size = Bounds.GetSize();
	TestTrue("Bounds X approx 200", FMath::IsNearlyEqual(Size.X, 200.0f, 1.0f));
	TestTrue("Bounds Y approx 20", FMath::IsNearlyEqual(Size.Y, 20.0f, 1.0f));
	TestTrue("Bounds Z approx 300", FMath::IsNearlyEqual(Size.Z, 300.0f, 1.0f));

	// Cleanup
	World->DestroyWorld(false);

	return true;
}
