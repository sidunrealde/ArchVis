#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "RTPlanGeometryUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTPlanMathGeometryTest, "ArchVis.RTPlanMath.Geometry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRTPlanMathGeometryTest::RunTest(const FString& Parameters)
{
	// Test 1: Distance Point to Segment
	FVector2D A(0, 0);
	FVector2D B(100, 0);
	FVector2D P(50, 50);

	float Dist = FRTPlanGeometryUtils::DistancePointToSegment(P, A, B);
	TestEqual("Distance Point to Segment", Dist, 50.0f);

	// Test 2: Closest Point
	FVector2D Closest = FRTPlanGeometryUtils::ClosestPointOnSegment(P, A, B);
	TestEqual("Closest Point X", Closest.X, 50.0);
	TestEqual("Closest Point Y", Closest.Y, 0.0);

	// Test 3: Wall Normals
	FVector2D Left, Right;
	FRTPlanGeometryUtils::GetWallNormals(A, B, Left, Right);
	
	// A->B is (1, 0). Left should be (0, 1). Right should be (0, -1).
	TestEqual("Left Normal X", Left.X, 0.0);
	TestEqual("Left Normal Y", Left.Y, 1.0);
	TestEqual("Right Normal X", Right.X, 0.0);
	TestEqual("Right Normal Y", Right.Y, -1.0);

	// Test 4: Intersection
	FVector2D A2(50, -50);
	FVector2D B2(50, 50);
	FVector2D Intersection;
	bool bIntersects = FRTPlanGeometryUtils::SegmentIntersection(A, B, A2, B2, Intersection);
	
	TestTrue("Segments Intersect", bIntersects);
	TestEqual("Intersection X", Intersection.X, 50.0);
	TestEqual("Intersection Y", Intersection.Y, 0.0);

	return true;
}
