#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "RTPlanSpatialIndex.h"
#include "RTPlanDocument.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanSpatialSnapTest, "ArchVis.RTPlanSpatial.Snapping", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanSpatialSnapTest::RunTest(const FString& Parameters)
{
	// Setup Document
	URTPlanDocument* Doc = NewObject<URTPlanDocument>();
	FRTPlanData& Data = Doc->GetDataMutable();

	// Add Wall (0,0) to (100,0)
	FRTVertex V1; V1.Id = FGuid::NewGuid(); V1.Position = FVector2D(0, 0);
	FRTVertex V2; V2.Id = FGuid::NewGuid(); V2.Position = FVector2D(100, 0);
	
	FRTWall W1; W1.Id = FGuid::NewGuid(); W1.VertexAId = V1.Id; W1.VertexBId = V2.Id;

	Data.Vertices.Add(V1.Id, V1);
	Data.Vertices.Add(V2.Id, V2);
	Data.Walls.Add(W1.Id, W1);

	// Build Index
	FRTPlanSpatialIndex Index;
	Index.Build(Doc);

	// Test 1: Snap to Endpoint
	FRTSnapResult Res1 = Index.QuerySnap(FVector2D(5, 5), 10.0f);
	TestTrue("Snap to Endpoint Valid", Res1.bValid);
	TestEqual("Snap Location X", Res1.Location.X, 0.0);
	TestEqual("Snap Location Y", Res1.Location.Y, 0.0);
	TestEqual("Snap Type", Res1.DebugType, FString("Endpoint"));

	// Test 2: Snap to Midpoint
	FRTSnapResult Res2 = Index.QuerySnap(FVector2D(50, 5), 10.0f);
	TestTrue("Snap to Midpoint Valid", Res2.bValid);
	TestEqual("Snap Location X", Res2.Location.X, 50.0);
	TestEqual("Snap Location Y", Res2.Location.Y, 0.0);
	TestEqual("Snap Type", Res2.DebugType, FString("Midpoint"));

	// Test 3: Snap to Projection (Edge)
	FRTSnapResult Res3 = Index.QuerySnap(FVector2D(25, 5), 10.0f);
	TestTrue("Snap to Projection Valid", Res3.bValid);
	TestEqual("Snap Location X", Res3.Location.X, 25.0);
	TestEqual("Snap Location Y", Res3.Location.Y, 0.0);
	TestEqual("Snap Type", Res3.DebugType, FString("Projection"));

	// Test 4: No Snap (Too far)
	FRTSnapResult Res4 = Index.QuerySnap(FVector2D(50, 50), 10.0f);
	TestFalse("Snap Too Far", Res4.bValid);

	return true;
}
