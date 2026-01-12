#include "RTPlanNetTests.h"
#include "Misc/AutomationTest.h"
#include "RTPlanNetDriver.h"
#include "RTPlanDocument.h"

// Note: Testing networking in Automation Tests is tricky without a full map/PIE session.
// We can test the serialization logic and RPC stubs locally.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanNetSerializationTest, "ArchVis.RTPlanNet.Serialization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanNetSerializationTest::RunTest(const FString& Parameters)
{
	// Setup
	URTPlanDocument* Doc = NewObject<URTPlanDocument>();
	FRTWall Wall;
	Wall.Id = FGuid::NewGuid();
	Doc->GetDataMutable().Walls.Add(Wall.Id, Wall);

	// Simulate Server Update
	FString Json = Doc->ToJson();
	
	// Simulate Client Receive
	URTPlanDocument* ClientDoc = NewObject<URTPlanDocument>();
	ClientDoc->FromJson(Json);

	TestTrue("Client received wall", ClientDoc->GetData().Walls.Contains(Wall.Id));

	return true;
}
