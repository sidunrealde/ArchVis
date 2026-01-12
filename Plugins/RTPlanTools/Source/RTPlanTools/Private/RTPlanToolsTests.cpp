#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "RTPlanToolManager.h"
#include "Tools/RTPlanLineTool.h"
#include "RTPlanDocument.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanToolsLineTest, "ArchVis.RTPlanTools.LineTool", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanToolsLineTest::RunTest(const FString& Parameters)
{
	// Setup
	URTPlanDocument* Doc = NewObject<URTPlanDocument>();
	URTPlanToolManager* ToolMgr = NewObject<URTPlanToolManager>();
	ToolMgr->Initialize(Doc);

	// Select Line Tool
	ToolMgr->SelectTool(URTPlanLineTool::StaticClass());
	TestNotNull("Active Tool is LineTool", Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()));

	// Simulate Input: Draw a wall from (0,0) to (100,0)
	
	// 1. Mouse Down at (0,0)
	FRTPointerEvent EvtDown;
	EvtDown.Action = ERTPointerAction::Down;
	EvtDown.WorldOrigin = FVector(0, 0, 1000);
	EvtDown.WorldDirection = FVector(0, 0, -1); // Looking down
	ToolMgr->ProcessInput(EvtDown);

	// 2. Mouse Move to (100,0)
	FRTPointerEvent EvtMove;
	EvtMove.Action = ERTPointerAction::Move;
	EvtMove.WorldOrigin = FVector(100, 0, 1000);
	EvtMove.WorldDirection = FVector(0, 0, -1);
	ToolMgr->ProcessInput(EvtMove);

	// 3. Mouse Up at (100,0)
	FRTPointerEvent EvtUp;
	EvtUp.Action = ERTPointerAction::Up;
	EvtUp.WorldOrigin = FVector(100, 0, 1000);
	EvtUp.WorldDirection = FVector(0, 0, -1);
	ToolMgr->ProcessInput(EvtUp);

	// Verify Result
	const FRTPlanData& Data = Doc->GetData();
	
	TestEqual("Vertex Count", Data.Vertices.Num(), 2);
	TestEqual("Wall Count", Data.Walls.Num(), 1);

	// Check Wall Length (indirectly via vertices)
	if (Data.Walls.Num() == 1)
	{
		// Get the only wall
		TArray<FRTWall> Walls;
		Data.Walls.GenerateValueArray(Walls);
		FRTWall Wall = Walls[0];

		if (Data.Vertices.Contains(Wall.VertexAId) && Data.Vertices.Contains(Wall.VertexBId))
		{
			FVector2D A = Data.Vertices[Wall.VertexAId].Position;
			FVector2D B = Data.Vertices[Wall.VertexBId].Position;
			float Length = FVector2D::Distance(A, B);
			TestEqual("Wall Length", Length, 100.0f);
		}
		else
		{
			AddError("Wall references missing vertices");
		}
	}

	return true;
}
