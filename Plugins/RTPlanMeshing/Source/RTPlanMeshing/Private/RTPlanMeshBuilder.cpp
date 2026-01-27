#include "RTPlanMeshBuilder.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshMaterialFunctions.h"
#include "GeometryScript/MeshTransformFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "UDynamicMesh.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

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

	// Calculate radius from center to start point (this is the centerline radius)
	float CenterRadius = FVector2D::Distance(ArcCenter, StartPoint);
	if (CenterRadius < 0.1f) return;

	// Calculate inner and outer radii based on wall thickness
	float HalfThickness = Thickness * 0.5f;
	float InnerRadius = CenterRadius - HalfThickness;
	float OuterRadius = CenterRadius + HalfThickness;

	// Ensure inner radius is positive
	if (InnerRadius < 0.1f)
	{
		InnerRadius = 0.1f;
	}

	// Calculate start angle from center to start point
	FVector2D ToStart = StartPoint - ArcCenter;
	float StartAngleRad = FMath::Atan2(ToStart.Y, ToStart.X);

	// Adjust number of segments based on arc sweep
	NumSegments = FMath::Max(NumSegments, FMath::CeilToInt(FMath::Abs(SweepAngleDeg) / 15.0f));
	NumSegments = FMath::Clamp(NumSegments, 4, 128);
	
	float SweepRad = FMath::DegreesToRadians(SweepAngleDeg);
	float StepAngle = SweepRad / (float)NumSegments;

	float TopZ = BaseZ + Height;
	float BottomZ = BaseZ;

	// UV scale to match AppendBox (which uses 1 UV unit per 100 world units)
	float UVScale = 0.01f;

	// Pre-compute arc points and cumulative arc length for UV mapping
	TArray<FVector2D> InnerPoints, OuterPoints;
	TArray<float> ArcLengths;
	float TotalArcLength = 0.0f;

	InnerPoints.SetNum(NumSegments + 1);
	OuterPoints.SetNum(NumSegments + 1);
	ArcLengths.SetNum(NumSegments + 1);

	for (int32 i = 0; i <= NumSegments; ++i)
	{
		float Angle = StartAngleRad + StepAngle * i;
		float CosA = FMath::Cos(Angle);
		float SinA = FMath::Sin(Angle);
		
		InnerPoints[i] = ArcCenter + FVector2D(CosA, SinA) * InnerRadius;
		OuterPoints[i] = ArcCenter + FVector2D(CosA, SinA) * OuterRadius;
		
		if (i == 0)
		{
			ArcLengths[i] = 0.0f;
		}
		else
		{
			FVector2D CenterPrev = ArcCenter + FVector2D(FMath::Cos(StartAngleRad + StepAngle * (i - 1)), 
														  FMath::Sin(StartAngleRad + StepAngle * (i - 1))) * CenterRadius;
			FVector2D CenterCurr = ArcCenter + FVector2D(CosA, SinA) * CenterRadius;
			TotalArcLength += FVector2D::Distance(CenterPrev, CenterCurr);
			ArcLengths[i] = TotalArcLength;
		}
	}

	// Use EditMesh for proper change tracking
	TargetMesh->EditMesh([&](FDynamicMesh3& Mesh)
	{
		// Ensure attributes are enabled
		if (!Mesh.HasAttributes())
		{
			Mesh.EnableAttributes();
		}
		UE::Geometry::FDynamicMeshUVOverlay* UVs = Mesh.Attributes()->PrimaryUV();
		UE::Geometry::FDynamicMeshNormalOverlay* Normals = Mesh.Attributes()->PrimaryNormals();

		// Helper to add a triangle with UVs and normals
		auto AddTriangle = [&](
			const FVector3d& P0, const FVector3d& P1, const FVector3d& P2,
			const FVector2f& UV0, const FVector2f& UV1, const FVector2f& UV2,
			const FVector3f& Normal)
		{
			int32 V0 = Mesh.AppendVertex(P0);
			int32 V1 = Mesh.AppendVertex(P1);
			int32 V2 = Mesh.AppendVertex(P2);
			
			int32 TriID = Mesh.AppendTriangle(V0, V1, V2);
			if (TriID >= 0)
			{
				if (UVs)
				{
					int32 U0 = UVs->AppendElement(UV0);
					int32 U1 = UVs->AppendElement(UV1);
					int32 U2 = UVs->AppendElement(UV2);
					UVs->SetTriangle(TriID, UE::Geometry::FIndex3i(U0, U1, U2));
				}
				if (Normals)
				{
					int32 N0 = Normals->AppendElement(Normal);
					int32 N1 = Normals->AppendElement(Normal);
					int32 N2 = Normals->AppendElement(Normal);
					Normals->SetTriangle(TriID, UE::Geometry::FIndex3i(N0, N1, N2));
				}
			}
		};

		// Helper to add a quad (two triangles)
		auto AddQuad = [&](
			const FVector3d& P0, const FVector3d& P1, const FVector3d& P2, const FVector3d& P3,
			const FVector2f& UV0, const FVector2f& UV1, const FVector2f& UV2, const FVector2f& UV3,
			const FVector3f& Normal)
		{
			// P0---P1
			// |     |
			// P3---P2
			AddTriangle(P0, P1, P3, UV0, UV1, UV3, Normal);
			AddTriangle(P1, P2, P3, UV1, UV2, UV3, Normal);
		};

		// Generate wall faces for each segment
		for (int32 i = 0; i < NumSegments; ++i)
		{
			FVector2D Inner0 = InnerPoints[i];
			FVector2D Inner1 = InnerPoints[i + 1];
			FVector2D Outer0 = OuterPoints[i];
			FVector2D Outer1 = OuterPoints[i + 1];

			FVector3d IB0(Inner0.X, Inner0.Y, BottomZ);
			FVector3d IT0(Inner0.X, Inner0.Y, TopZ);
			FVector3d IB1(Inner1.X, Inner1.Y, BottomZ);
			FVector3d IT1(Inner1.X, Inner1.Y, TopZ);
			FVector3d OB0(Outer0.X, Outer0.Y, BottomZ);
			FVector3d OT0(Outer0.X, Outer0.Y, TopZ);
			FVector3d OB1(Outer1.X, Outer1.Y, BottomZ);
			FVector3d OT1(Outer1.X, Outer1.Y, TopZ);

			float U0 = ArcLengths[i] * UVScale;
			float U1 = ArcLengths[i + 1] * UVScale;
			float VBottom = 0.0f;
			float VTop = Height * UVScale;
			float VInner = 0.0f;
			float VOuter = Thickness * UVScale;

			float MidAngle = StartAngleRad + StepAngle * (i + 0.5f);
			FVector3f OutwardNormal(FMath::Cos(MidAngle), FMath::Sin(MidAngle), 0.0f);
			FVector3f InwardNormal = -OutwardNormal;
			FVector3f UpNormal(0.0f, 0.0f, 1.0f);
			FVector3f DownNormal(0.0f, 0.0f, -1.0f);

			if (SweepAngleDeg > 0)
			{
				AddQuad(OT0, OT1, OB1, OB0,
					FVector2f(U0, VTop), FVector2f(U1, VTop), FVector2f(U1, VBottom), FVector2f(U0, VBottom),
					OutwardNormal);

				AddQuad(IT1, IT0, IB0, IB1,
					FVector2f(U1, VTop), FVector2f(U0, VTop), FVector2f(U0, VBottom), FVector2f(U1, VBottom),
					InwardNormal);

				AddQuad(IT0, IT1, OT1, OT0,
					FVector2f(U0, VInner), FVector2f(U1, VInner), FVector2f(U1, VOuter), FVector2f(U0, VOuter),
					UpNormal);

				AddQuad(OB0, OB1, IB1, IB0,
					FVector2f(U0, VOuter), FVector2f(U1, VOuter), FVector2f(U1, VInner), FVector2f(U0, VInner),
					DownNormal);
			}
			else
			{
				AddQuad(OT1, OT0, OB0, OB1,
					FVector2f(U1, VTop), FVector2f(U0, VTop), FVector2f(U0, VBottom), FVector2f(U1, VBottom),
					OutwardNormal);

				AddQuad(IT0, IT1, IB1, IB0,
					FVector2f(U0, VTop), FVector2f(U1, VTop), FVector2f(U1, VBottom), FVector2f(U0, VBottom),
					InwardNormal);

				AddQuad(IT1, IT0, OT0, OT1,
					FVector2f(U1, VInner), FVector2f(U0, VInner), FVector2f(U0, VOuter), FVector2f(U1, VOuter),
					UpNormal);

				AddQuad(OB1, OB0, IB0, IB1,
					FVector2f(U1, VOuter), FVector2f(U0, VOuter), FVector2f(U0, VInner), FVector2f(U1, VInner),
					DownNormal);
			}
		}

		// START CAP
		{
			FVector2D Inner = InnerPoints[0];
			FVector2D Outer = OuterPoints[0];
			
			FVector3d IB(Inner.X, Inner.Y, BottomZ);
			FVector3d IT(Inner.X, Inner.Y, TopZ);
			FVector3d OB(Outer.X, Outer.Y, BottomZ);
			FVector3d OT(Outer.X, Outer.Y, TopZ);

			FVector3f CapNormal(-FMath::Sin(StartAngleRad), FMath::Cos(StartAngleRad), 0.0f);
			if (SweepAngleDeg > 0) CapNormal = -CapNormal;

			FVector2f UV_IB(0.0f, 0.0f);
			FVector2f UV_IT(0.0f, Height * UVScale);
			FVector2f UV_OB(Thickness * UVScale, 0.0f);
			FVector2f UV_OT(Thickness * UVScale, Height * UVScale);

			if (SweepAngleDeg > 0)
			{
				AddQuad(IT, OT, OB, IB, UV_IT, UV_OT, UV_OB, UV_IB, CapNormal);
			}
			else
			{
				AddQuad(OT, IT, IB, OB, UV_OT, UV_IT, UV_IB, UV_OB, CapNormal);
			}
		}

		// END CAP
		{
			FVector2D Inner = InnerPoints[NumSegments];
			FVector2D Outer = OuterPoints[NumSegments];
			
			FVector3d IB(Inner.X, Inner.Y, BottomZ);
			FVector3d IT(Inner.X, Inner.Y, TopZ);
			FVector3d OB(Outer.X, Outer.Y, BottomZ);
			FVector3d OT(Outer.X, Outer.Y, TopZ);

			float EndAngle = StartAngleRad + SweepRad;
			FVector3f CapNormal(-FMath::Sin(EndAngle), FMath::Cos(EndAngle), 0.0f);
			if (SweepAngleDeg < 0) CapNormal = -CapNormal;

			FVector2f UV_IB(0.0f, 0.0f);
			FVector2f UV_IT(0.0f, Height * UVScale);
			FVector2f UV_OB(Thickness * UVScale, 0.0f);
			FVector2f UV_OT(Thickness * UVScale, Height * UVScale);

			if (SweepAngleDeg > 0)
			{
				AddQuad(OT, IT, IB, OB, UV_OT, UV_IT, UV_IB, UV_OB, CapNormal);
			}
			else
			{
				AddQuad(IT, OT, OB, IB, UV_IT, UV_OT, UV_OB, UV_IB, CapNormal);
			}
		}
	}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);
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
