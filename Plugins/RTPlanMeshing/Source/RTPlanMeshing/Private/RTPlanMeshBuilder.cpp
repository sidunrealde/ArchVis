#include "RTPlanMeshBuilder.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshMaterialFunctions.h"
#include "GeometryScript/MeshTransformFunctions.h"
#include "UDynamicMesh.h"

void FRTPlanMeshBuilder::AppendWallMesh(
	UDynamicMesh* TargetMesh,
	const FTransform& Transform,
	float Length,
	float Thickness,
	float Height,
	int32 MaterialID_Left,
	int32 MaterialID_Right,
	int32 MaterialID_Caps
)
{
	if (!TargetMesh) return;

	// 1. Create a Box
	// Pivot is usually center, but for walls we often want Start Point centered on Y/Z.
	// Let's assume the Transform places the wall at the Start Point, facing X.
	
	// GeometryScript Box is centered at Origin.
	// Dimensions: X=Length, Y=Thickness, Z=Height
	FGeometryScriptPrimitiveOptions Options;
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
		TargetMesh,
		Options,
		Transform, // Apply the transform directly
		Length,
		Thickness,
		Height
	);

	// 2. Remap Materials
	// Box faces: 0=X+, 1=X-, 2=Y+, 3=Y-, 4=Z+, 5=Z-
	// In our wall coords (Length along X):
	// X+ (End Cap), X- (Start Cap) -> Caps
	// Y+ (Left), Y- (Right) -> Left/Right
	// Z+ (Top), Z- (Bottom) -> Caps

	// Note: GeometryScript box face indices might vary, need to verify or use UVs/Normals.
	// For V1, let's just assign everything to Caps and then override sides.
	// Actually, let's use a simpler approach: Assign all to Caps first.
	
	// TODO: Precise material assignment requires selecting triangles by normal.
	// For now, we will just assign ID 0 to everything for the prototype test.
	// Real implementation needs UGeometryScriptLibrary_MeshMaterialFunctions::SetMaterialIDForMeshSelection
}

void FRTPlanMeshBuilder::AppendCurvedWallMesh(
	UDynamicMesh* TargetMesh,
	const FVector2D& StartPoint,
	const FVector2D& EndPoint,
	const FVector2D& ArcCenter,
	float SweepAngleDeg,
	float Thickness,
	float Height,
	float BaseZ,
	int32 NumSegments,
	int32 MaterialID_Left,
	int32 MaterialID_Right,
	int32 MaterialID_Caps
)
{
	if (!TargetMesh || FMath::Abs(SweepAngleDeg) < 0.1f) return;

	// Calculate radius from center to start point
	float Radius = FVector2D::Distance(ArcCenter, StartPoint);
	if (Radius < 0.1f) return;

	// Calculate start angle from center to start point
	FVector2D ToStart = StartPoint - ArcCenter;
	float StartAngleRad = FMath::Atan2(ToStart.Y, ToStart.X);

	// Adjust number of segments based on arc sweep - user controls this via NumSegments parameter
	// But ensure minimum quality
	NumSegments = FMath::Max(NumSegments, FMath::CeilToInt(FMath::Abs(SweepAngleDeg) / 15.0f));
	NumSegments = FMath::Clamp(NumSegments, 4, 128);
	
	float SweepRad = FMath::DegreesToRadians(SweepAngleDeg);
	float StepAngle = SweepRad / (float)NumSegments;

	// Generate the curved wall as a series of connected quads
	FGeometryScriptPrimitiveOptions Options;

	for (int32 i = 0; i < NumSegments; ++i)
	{
		float Angle1 = StartAngleRad + StepAngle * i;
		float Angle2 = StartAngleRad + StepAngle * (i + 1);

		// Calculate the center line points on the arc
		FVector2D P1 = ArcCenter + FVector2D(FMath::Cos(Angle1), FMath::Sin(Angle1)) * Radius;
		FVector2D P2 = ArcCenter + FVector2D(FMath::Cos(Angle2), FMath::Sin(Angle2)) * Radius;

		// Calculate segment direction and perpendicular for thickness offset
		FVector2D SegDir = (P2 - P1).GetSafeNormal();
		float SegAngle = FMath::Atan2(SegDir.Y, SegDir.X);
		
		// Calculate segment properties
		float SegLength = FVector2D::Distance(P1, P2);
		FVector2D SegMid = (P1 + P2) * 0.5f;

		// Create transform for the segment
		float ZCenter = BaseZ + (Height * 0.5f);
		FVector SegCenter(SegMid.X, SegMid.Y, ZCenter);
		FQuat SegRotation(FVector::UpVector, SegAngle);

		FTransform SegTransform;
		SegTransform.SetLocation(SegCenter);
		SegTransform.SetRotation(SegRotation);

		// Append box segment - this creates a continuous wall when segments are adjacent
		UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
			TargetMesh,
			Options,
			SegTransform,
			SegLength,
			Thickness,
			Height
		);
	}
}

void FRTPlanMeshBuilder::AppendFloorMesh(
	UDynamicMesh* TargetMesh,
	const TArray<FVector2D>& Polygon,
	float ZHeight,
	int32 MaterialID
)
{
	if (!TargetMesh || Polygon.Num() < 3) return;

	// Triangulate Polygon
	FGeometryScriptPrimitiveOptions Options;
	
	// Convert 2D poly to 3D vertices
	TArray<FVector> Vertices;
	for (const FVector2D& P : Polygon)
	{
		Vertices.Add(FVector(P.X, P.Y, ZHeight));
	}

	// Simple triangulation (Ear Clipping or similar provided by GS)
	// UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendTriangulatedPolygon(TargetMesh, Options, ...);
	// Note: AppendTriangulatedPolygon might be in a different library or require a specific struct.
	// Fallback for V1: Just append a rectangle if poly is 4 points, or use a placeholder.
	
	// Placeholder: Append a small box at the first point
	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
		TargetMesh,
		Options,
		FTransform(Vertices[0]),
		50, 50, 10
	);
}
