#include "RTPlanCoreTests.h"
#include "RTPlanDocument.h"
#include "RTPlanCommand.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanCoreSerializationTest, "ArchVis.RTPlanCore.Serialization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanCoreSerializationTest::RunTest(const FString& Parameters)
{
	URTPlanDocument* Doc = NewObject<URTPlanDocument>();
	
	// Create some data
	FRTVertex V1;
	V1.Id = FGuid::NewGuid();
	V1.Position = FVector2D(100, 100);

	FRTWall W1;
	W1.Id = FGuid::NewGuid();
	W1.VertexAId = V1.Id;
	W1.ThicknessCm = 25.0f;

	// Manually inject data for test (bypassing commands for raw data test)
	Doc->GetDataMutable().Vertices.Add(V1.Id, V1);
	Doc->GetDataMutable().Walls.Add(W1.Id, W1);

	// Serialize
	FString Json = Doc->ToJson();
	
	// Deserialize into new doc
	URTPlanDocument* Doc2 = NewObject<URTPlanDocument>();
	bool bSuccess = Doc2->FromJson(Json);

	TestTrue("Deserialization returned true", bSuccess);
	TestTrue("Vertex count matches", Doc2->GetData().Vertices.Num() == 1);
	TestTrue("Wall count matches", Doc2->GetData().Walls.Num() == 1);
	
	if (Doc2->GetData().Vertices.Contains(V1.Id))
	{
		TestEqual("Vertex Position X", Doc2->GetData().Vertices[V1.Id].Position.X, 100.0);
	}
	else
	{
		AddError("Vertex ID not found in deserialized data");
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanCoreCommandTest, "ArchVis.RTPlanCore.Commands", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanCoreCommandTest::RunTest(const FString& Parameters)
{
	URTPlanDocument* Doc = NewObject<URTPlanDocument>();

	// 1. Add Vertex
	URTCmdAddVertex* Cmd1 = NewObject<URTCmdAddVertex>();
	Cmd1->Vertex.Id = FGuid::NewGuid();
	Cmd1->Vertex.Position = FVector2D(500, 500);
	
	bool bRes1 = Doc->SubmitCommand(Cmd1);
	TestTrue("Submit AddVertex", bRes1);
	TestTrue("Vertex exists", Doc->GetData().Vertices.Contains(Cmd1->Vertex.Id));

	// 2. Undo
	Doc->Undo();
	TestFalse("Vertex removed after undo", Doc->GetData().Vertices.Contains(Cmd1->Vertex.Id));

	// 3. Redo
	Doc->Redo();
	TestTrue("Vertex exists after redo", Doc->GetData().Vertices.Contains(Cmd1->Vertex.Id));

	return true;
}
