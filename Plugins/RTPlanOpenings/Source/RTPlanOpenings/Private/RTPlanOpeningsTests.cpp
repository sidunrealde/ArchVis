#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "RTPlanOpeningUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanOpeningsIntervalTest, "ArchVis.RTPlanOpenings.Intervals", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanOpeningsIntervalTest::RunTest(const FString& Parameters)
{
	// Test Case 1: No Openings
	{
		TArray<FRTOpening> Ops;
		auto Solids = FRTPlanOpeningUtils::ComputeSolidIntervals(100.0f, Ops);
		TestEqual("No Openings Count", Solids.Num(), 1);
		if (Solids.Num() > 0)
		{
			TestEqual("Full Wall Start", Solids[0].Start, 0.0f);
			TestEqual("Full Wall End", Solids[0].End, 100.0f);
		}
	}

	// Test Case 2: Single Opening in Middle
	{
		TArray<FRTOpening> Ops;
		FRTOpening Op; Op.OffsetCm = 40; Op.WidthCm = 20;
		Ops.Add(Op);

		auto Solids = FRTPlanOpeningUtils::ComputeSolidIntervals(100.0f, Ops);
		TestEqual("Single Opening Count", Solids.Num(), 2);
		if (Solids.Num() == 2)
		{
			TestEqual("First Seg End", Solids[0].End, 40.0f);
			TestEqual("Second Seg Start", Solids[1].Start, 60.0f);
		}
	}

	// Test Case 3: Overlapping Openings
	{
		TArray<FRTOpening> Ops;
		FRTOpening Op1; Op1.OffsetCm = 30; Op1.WidthCm = 20; // 30-50
		FRTOpening Op2; Op2.OffsetCm = 40; Op2.WidthCm = 20; // 40-60
		Ops.Add(Op1);
		Ops.Add(Op2);

		auto Solids = FRTPlanOpeningUtils::ComputeSolidIntervals(100.0f, Ops);
		// Merged Hole: 30-60. Solids: 0-30, 60-100
		TestEqual("Overlap Count", Solids.Num(), 2);
		if (Solids.Num() == 2)
		{
			TestEqual("First Seg End", Solids[0].End, 30.0f);
			TestEqual("Second Seg Start", Solids[1].Start, 60.0f);
		}
	}

	return true;
}
