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
