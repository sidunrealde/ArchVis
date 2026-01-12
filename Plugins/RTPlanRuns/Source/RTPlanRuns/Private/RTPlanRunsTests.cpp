#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "RTPlanRunSolver.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanRunsSolverTest, "ArchVis.RTPlanRuns.Solver", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanRunsSolverTest::RunTest(const FString& Parameters)
{
	// Test 1: Exact Fit (120cm = 2x 60cm)
	{
		FRTPlanRunSolver::FRunInput Input;
		Input.TotalLength = 120.0f;
		auto Modules = FRTPlanRunSolver::Solve(Input);
		
		TestEqual("Exact Fit Count", Modules.Num(), 2);
		if (Modules.Num() == 2)
		{
			TestEqual("Mod 1 Width", Modules[0].Width, 60.0f);
			TestEqual("Mod 2 Width", Modules[1].Width, 60.0f);
		}
	}

	// Test 2: Mixed Fit (105cm = 60 + 45)
	{
		FRTPlanRunSolver::FRunInput Input;
		Input.TotalLength = 105.0f;
		auto Modules = FRTPlanRunSolver::Solve(Input);
		
		TestEqual("Mixed Fit Count", Modules.Num(), 2);
		if (Modules.Num() == 2)
		{
			TestEqual("Mod 1 Width", Modules[0].Width, 60.0f);
			TestEqual("Mod 2 Width", Modules[1].Width, 45.0f);
		}
	}

	// Test 3: Filler (70cm = 60 + 10 Filler)
	{
		FRTPlanRunSolver::FRunInput Input;
		Input.TotalLength = 70.0f;
		auto Modules = FRTPlanRunSolver::Solve(Input);
		
		TestEqual("Filler Count", Modules.Num(), 2);
		if (Modules.Num() == 2)
		{
			TestEqual("Mod 1 Width", Modules[0].Width, 60.0f);
			TestEqual("Mod 2 ID", Modules[1].ProductId, FName("Filler"));
			TestEqual("Mod 2 Width", Modules[1].Width, 10.0f);
		}
	}

	return true;
}
