#include "RTPlanMeshBuilder.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshMaterialFunctions.h"
#include "GeometryScript/MeshTransformFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "UDynamicMesh.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"

// Helper to add a quad with proper UVs, Normals, and MaterialID
void AddQuad(
	FDynamicMesh3& Mesh,
	const FVector3d& P0, const FVector3d& P1, const FVector3d& P2, const FVector3d& P3,
	const FVector2f& UV0, const FVector2f& UV1, const FVector2f& UV2, const FVector2f& UV3,
	const FVector3f& Normal,
	int32 MaterialID)
{
	UE::Geometry::FDynamicMeshUVOverlay* UVs = Mesh.Attributes()->PrimaryUV();
	UE::Geometry::FDynamicMeshNormalOverlay* Normals = Mesh.Attributes()->PrimaryNormals();

	int32 V0 = Mesh.AppendVertex(P0);
	int32 V1 = Mesh.AppendVertex(P1);
	int32 V2 = Mesh.AppendVertex(P2);
	int32 V3 = Mesh.AppendVertex(P3);

	int32 T0 = Mesh.AppendTriangle(V0, V1, V3, MaterialID);
	int32 T1 = Mesh.AppendTriangle(V1, V2, V3, MaterialID);

	if (T0 >= 0 && T1 >= 0)
	{
		if (UVs)
		{
			int32 UV_ID0 = UVs->AppendElement(UV0);
			int32 UV_ID1 = UVs->AppendElement(UV1);
			int32 UV_ID2 = UVs->AppendElement(UV2);
			int32 UV_ID3 = UVs->AppendElement(UV3);
			UVs->SetTriangle(T0, UE::Geometry::FIndex3i(UV_ID0, UV_ID1, UV_ID3));
			UVs->SetTriangle(T1, UE::Geometry::FIndex3i(UV_ID1, UV_ID2, UV_ID3));
		}
		if (Normals)
		{
			int32 N_ID = Normals->AppendElement(Normal);
			Normals->SetTriangle(T0, UE::Geometry::FIndex3i(N_ID, N_ID, N_ID));
			Normals->SetTriangle(T1, UE::Geometry::FIndex3i(N_ID, N_ID, N_ID));
		}
	}
}


void FRTPlanMeshBuilder::AppendWallMesh(
	UDynamicMesh* TargetMesh,
	const FTransform& Transform,
	float Length,
	float Thickness,
	float Height,
	float BaseZ,
	float SkirtingHeight_Left,
	float SkirtingThickness_Left,
	float SkirtingHeight_Right,
	float SkirtingThickness_Right,
	float SkirtingHeight_Cap,
	float SkirtingThickness_Cap,
	int32 MaterialID_Left,
	int32 MaterialID_Right,
	int32 MaterialID_Caps,
	int32 MaterialID_Skirting_Left,
	int32 MaterialID_Skirting_Right,
	int32 MaterialID_Skirting_Cap
)
{
	if (!TargetMesh || Length <= 0 || Thickness <= 0 || Height <= 0) return;

	TargetMesh->EditMesh([&](FDynamicMesh3& Mesh)
	{
		if (!Mesh.HasAttributes())
		{
			Mesh.EnableAttributes();
		}

		float HalfThickness = Thickness * 0.5f;
		float UVScale = 0.01f;

		// Define corner points of the wall cross-section profile
		// Using Y+ as Left, Y- as Right
		FVector3d P_RB(0, -HalfThickness, BaseZ); // Right-Bottom
		FVector3d P_RT(0, -HalfThickness, BaseZ + Height); // Right-Top
		FVector3d P_LT(0, HalfThickness, BaseZ + Height); // Left-Top
		FVector3d P_LB(0, HalfThickness, BaseZ); // Left-Bottom

		// Skirting points
		FVector3d P_RS_Out(0, -HalfThickness - SkirtingThickness_Right, BaseZ);
		FVector3d P_RS_Top(0, -HalfThickness - SkirtingThickness_Right, BaseZ + SkirtingHeight_Right);
		FVector3d P_LS_Out(0, HalfThickness + SkirtingThickness_Left, BaseZ);
		FVector3d P_LS_Top(0, HalfThickness + SkirtingThickness_Left, BaseZ + SkirtingHeight_Left);

		// Points at the other end of the wall
		FVector3d P_RB_End = P_RB + FVector3d(Length, 0, 0);
		FVector3d P_RT_End = P_RT + FVector3d(Length, 0, 0);
		FVector3d P_LT_End = P_LT + FVector3d(Length, 0, 0);
		FVector3d P_LB_End = P_LB + FVector3d(Length, 0, 0);
		FVector3d P_RS_Out_End = P_RS_Out + FVector3d(Length, 0, 0);
		FVector3d P_RS_Top_End = P_RS_Top + FVector3d(Length, 0, 0);
		FVector3d P_LS_Out_End = P_LS_Out + FVector3d(Length, 0, 0);
		FVector3d P_LS_Top_End = P_LS_Top + FVector3d(Length, 0, 0);

		// --- Main Wall Faces ---
		// Right Wall Face (above skirting)
		// Reversed winding for CCW
		AddQuad(Mesh, P_RB, P_RT, P_RT_End, P_RB_End,
			FVector2f(0, 0), FVector2f(0, Height * UVScale),
			FVector2f(Length * UVScale, Height * UVScale), FVector2f(Length * UVScale, 0),
			FVector3f(0, -1, 0), MaterialID_Right);

		// Left Wall Face (above skirting)
		// Reversed winding for CCW
		AddQuad(Mesh, P_LB_End, P_LT_End, P_LT, P_LB,
			FVector2f(Length * UVScale, 0), FVector2f(Length * UVScale, Height * UVScale),
			FVector2f(0, Height * UVScale), FVector2f(0, 0),
			FVector3f(0, 1, 0), MaterialID_Left);

		// --- Skirting Faces ---
		if (SkirtingHeight_Right > 0 && SkirtingThickness_Right > 0)
		{
			// Right Skirting Face
			// Reversed winding
			AddQuad(Mesh, P_RS_Out, P_RS_Top, P_RS_Top_End, P_RS_Out_End,
				FVector2f(0, 0), FVector2f(0, SkirtingHeight_Right * UVScale),
				FVector2f(Length * UVScale, SkirtingHeight_Right * UVScale), FVector2f(Length * UVScale, 0),
				FVector3f(0, -1, 0), MaterialID_Skirting_Right);
			
			// Right Skirting Top
			// Connects Skirting Top Outer Edge to Wall Surface at Skirting Height
			FVector3d P_RW_SkirtTop_Start(0, -HalfThickness, BaseZ + SkirtingHeight_Right);
			FVector3d P_RW_SkirtTop_End(Length, -HalfThickness, BaseZ + SkirtingHeight_Right);
			
			// Normal Up (0,0,1)
			// Winding: Outer Start -> Wall Start -> Wall End -> Outer End
			// P_RS_Top -> P_RW_SkirtTop_Start -> P_RW_SkirtTop_End -> P_RS_Top_End
			// UVs: U -> Length, V -> SkirtThickness
			AddQuad(Mesh, P_RS_Top, P_RW_SkirtTop_Start, P_RW_SkirtTop_End, P_RS_Top_End,
				FVector2f(0, 0), FVector2f(0, SkirtingThickness_Right * UVScale),
				FVector2f(Length * UVScale, SkirtingThickness_Right * UVScale), FVector2f(Length * UVScale, 0),
				FVector3f(0, 0, 1), MaterialID_Skirting_Right);
		}
		if (SkirtingHeight_Left > 0 && SkirtingThickness_Left > 0)
		{
			// Left Skirting Face
			// Reversed winding
			AddQuad(Mesh, P_LS_Out_End, P_LS_Top_End, P_LS_Top, P_LS_Out,
				FVector2f(Length * UVScale, 0), FVector2f(Length * UVScale, SkirtingHeight_Left * UVScale),
				FVector2f(0, SkirtingHeight_Left * UVScale), FVector2f(0, 0),
				FVector3f(0, 1, 0), MaterialID_Skirting_Left);
			
			// Left Skirting Top
			// Connects Skirting Top Outer Edge to Wall Surface at Skirting Height
			FVector3d P_LW_SkirtTop_Start(0, HalfThickness, BaseZ + SkirtingHeight_Left);
			FVector3d P_LW_SkirtTop_End(Length, HalfThickness, BaseZ + SkirtingHeight_Left);
			
			// Normal Up (0,0,1)
			// Winding: Outer Start -> Outer End -> Wall End -> Wall Start
			// P_LS_Top -> P_LS_Top_End -> P_LW_SkirtTop_End -> P_LW_SkirtTop_Start
			// UVs: U -> Length, V -> SkirtThickness
			AddQuad(Mesh, P_LS_Top, P_LS_Top_End, P_LW_SkirtTop_End, P_LW_SkirtTop_Start,
				FVector2f(0, 0), FVector2f(Length * UVScale, 0),
				FVector2f(Length * UVScale, SkirtingThickness_Left * UVScale), FVector2f(0, SkirtingThickness_Left * UVScale),
				FVector3f(0, 0, 1), MaterialID_Skirting_Left);
		}

		// --- Caps and Bottom/Top Faces ---
		// Top Cap
		// Reversed winding
		// UVs: U -> Length, V -> Thickness
		AddQuad(Mesh, P_RT, P_LT, P_LT_End, P_RT_End,
			FVector2f(0, 0), FVector2f(0, Thickness * UVScale),
			FVector2f(Length * UVScale, Thickness * UVScale), FVector2f(Length * UVScale, 0),
			FVector3f(0, 0, 1), MaterialID_Caps);
		
		// Bottom Cap (might be covered by floor, but good to have)
		// This is now more complex due to skirting. We build it from 3 parts.
		// Keeping these as is for now, assuming bottom is not visible or winding is less critical
		AddQuad(Mesh, P_RS_Out_End, P_RB_End, P_RB, P_RS_Out, FVector2f(), FVector2f(), FVector2f(), FVector2f(), FVector3f(0, 0, -1), MaterialID_Caps);
		AddQuad(Mesh, P_LB_End, P_LS_Out_End, P_LS_Out, P_LB, FVector2f(), FVector2f(), FVector2f(), FVector2f(), FVector3f(0, 0, -1), MaterialID_Caps);
		AddQuad(Mesh, P_RB_End, P_LB_End, P_LB, P_RB, FVector2f(), FVector2f(), FVector2f(), FVector2f(), FVector3f(0, 0, -1), MaterialID_Caps);


		// Start Cap
		// Reversed winding
		// UVs: U -> Thickness, V -> Height
		AddQuad(Mesh, P_RB, P_LB, P_LT, P_RT, 
			FVector2f(0, 0), FVector2f(Thickness * UVScale, 0), 
			FVector2f(Thickness * UVScale, Height * UVScale), FVector2f(0, Height * UVScale), 
			FVector3f(-1, 0, 0), MaterialID_Caps);
		
		if (SkirtingHeight_Right > 0) AddQuad(Mesh, P_RS_Out, P_RB, P_RS_Top, P_RS_Out, FVector2f(), FVector2f(), FVector2f(), FVector2f(), FVector3f(-1, 0, 0), MaterialID_Caps); // Triangulated quad? No, P_RS_Out twice?
		
		// End Cap
		// Reversed winding
		// UVs: U -> Thickness, V -> Height
		AddQuad(Mesh, P_RT_End, P_LT_End, P_LB_End, P_RB_End, 
			FVector2f(0, Height * UVScale), FVector2f(Thickness * UVScale, Height * UVScale), 
			FVector2f(Thickness * UVScale, 0), FVector2f(0, 0), 
			FVector3f(1, 0, 0), MaterialID_Caps);
		
		// --- Cap Skirting ---
		if (SkirtingHeight_Cap > 0 && SkirtingThickness_Cap > 0)
		{
			// Start Cap Skirting
			// Extends from Start Cap in -X direction
			FVector3d P_SC_Out_RB(-SkirtingThickness_Cap, -HalfThickness, BaseZ);
			FVector3d P_SC_Out_RT(-SkirtingThickness_Cap, -HalfThickness, BaseZ + SkirtingHeight_Cap);
			FVector3d P_SC_Out_LT(-SkirtingThickness_Cap, HalfThickness, BaseZ + SkirtingHeight_Cap);
			FVector3d P_SC_Out_LB(-SkirtingThickness_Cap, HalfThickness, BaseZ);

			// Face
			// Reversed winding (LB, LT, RT, RB)
			AddQuad(Mesh, P_SC_Out_LB, P_SC_Out_LT, P_SC_Out_RT, P_SC_Out_RB,
				FVector2f(0, 0), FVector2f(0, SkirtingHeight_Cap * UVScale),
				FVector2f(Thickness * UVScale, SkirtingHeight_Cap * UVScale), FVector2f(Thickness * UVScale, 0),
				FVector3f(-1, 0, 0), MaterialID_Skirting_Cap);
			
			// Top
			// Connects Skirting Top Outer Edge to Wall Cap Surface at Skirting Height
			FVector3d P_SC_Wall_RT(0, -HalfThickness, BaseZ + SkirtingHeight_Cap);
			FVector3d P_SC_Wall_LT(0, HalfThickness, BaseZ + SkirtingHeight_Cap);
			
			// Normal Up (0,0,1)
			// Winding: Outer RT -> Outer LT -> Wall LT -> Wall RT
			// P_SC_Out_RT -> P_SC_Out_LT -> P_SC_Wall_LT -> P_SC_Wall_RT
			// UVs: U -> Thickness, V -> SkirtThickness
			AddQuad(Mesh, P_SC_Out_RT, P_SC_Out_LT, P_SC_Wall_LT, P_SC_Wall_RT,
				FVector2f(0, SkirtingThickness_Cap * UVScale), FVector2f(Thickness * UVScale, SkirtingThickness_Cap * UVScale),
				FVector2f(Thickness * UVScale, 0), FVector2f(0, 0),
				FVector3f(0, 0, 1), MaterialID_Skirting_Cap);

			// Sides (connecting to Left/Right skirting if present, or wall)
			// Left Side of Start Cap Skirting
			if (SkirtingHeight_Left > 0)
			{
				// Connect to Left Skirting
				// P_SC_Out_LB -> P_LS_Out (at start)
				// Side face at Y=HalfThickness
				AddQuad(Mesh, P_SC_Out_LT, P_SC_Out_LB, P_LB, P_LB, // Degenerate? No, P_LB is at X=0.
					FVector2f(), FVector2f(), FVector2f(), FVector2f(),
					FVector3f(0, 1, 0), MaterialID_Skirting_Cap);
			}
			else
			{
				// P_LB is (0, HalfThickness, BaseZ)
				// P_SC_Out_LB is (-SkirtTCap, HalfThickness, BaseZ)
				// P_SC_Out_LT is (-SkirtTCap, HalfThickness, BaseZ+H)
				// We need point at (0, HalfThickness, BaseZ+H) which is P_LT (if H matches)
				// Let's use P_LT_Cap equivalent
				FVector3d P_LT_Cap(0, HalfThickness, BaseZ + SkirtingHeight_Cap);
				AddQuad(Mesh, P_SC_Out_LT, P_SC_Out_LB, P_LB, P_LT_Cap,
					FVector2f(), FVector2f(), FVector2f(), FVector2f(),
					FVector3f(0, 1, 0), MaterialID_Skirting_Cap);
			}
			
			// Right Side of Start Cap Skirting
			{
				FVector3d P_RT_Cap(0, -HalfThickness, BaseZ + SkirtingHeight_Cap);
				AddQuad(Mesh, P_SC_Out_RB, P_SC_Out_RT, P_RT_Cap, P_RB,
					FVector2f(), FVector2f(), FVector2f(), FVector2f(),
					FVector3f(0, -1, 0), MaterialID_Skirting_Cap);
			}


			// End Cap Skirting
			// Extends from End Cap in +X direction
			FVector3d P_EC_Out_RB(Length + SkirtingThickness_Cap, -HalfThickness, BaseZ);
			FVector3d P_EC_Out_RT(Length + SkirtingThickness_Cap, -HalfThickness, BaseZ + SkirtingHeight_Cap);
			FVector3d P_EC_Out_LT(Length + SkirtingThickness_Cap, HalfThickness, BaseZ + SkirtingHeight_Cap);
			FVector3d P_EC_Out_LB(Length + SkirtingThickness_Cap, HalfThickness, BaseZ);

			// Face
			// Reversed winding (RB, RT, LT, LB)
			AddQuad(Mesh, P_EC_Out_RB, P_EC_Out_RT, P_EC_Out_LT, P_EC_Out_LB,
				FVector2f(0, 0), FVector2f(0, SkirtingHeight_Cap * UVScale),
				FVector2f(Thickness * UVScale, SkirtingHeight_Cap * UVScale), FVector2f(Thickness * UVScale, 0),
				FVector3f(1, 0, 0), MaterialID_Skirting_Cap);

			// Top
			// Connects Skirting Top Outer Edge to Wall Cap Surface at Skirting Height
			FVector3d P_EC_Wall_RT(Length, -HalfThickness, BaseZ + SkirtingHeight_Cap);
			FVector3d P_EC_Wall_LT(Length, HalfThickness, BaseZ + SkirtingHeight_Cap);
			
			// Normal Up (0,0,1)
			// Winding: Outer LT -> Outer RT -> Wall RT -> Wall LT
			// P_EC_Out_LT -> P_EC_Out_RT -> P_EC_Wall_RT -> P_EC_Wall_LT
			// UVs: U -> Thickness, V -> SkirtThickness
			AddQuad(Mesh, P_EC_Out_LT, P_EC_Out_RT, P_EC_Wall_RT, P_EC_Wall_LT,
				FVector2f(Thickness * UVScale, SkirtingThickness_Cap * UVScale), FVector2f(0, SkirtingThickness_Cap * UVScale),
				FVector2f(0, 0), FVector2f(Thickness * UVScale, 0),
				FVector3f(0, 0, 1), MaterialID_Skirting_Cap);

			// Sides
			// Left Side of End Cap Skirting
			{
				FVector3d P_LT_End_Cap(Length, HalfThickness, BaseZ + SkirtingHeight_Cap);
				AddQuad(Mesh, P_EC_Out_LB, P_EC_Out_LT, P_LT_End_Cap, P_LB_End,
					FVector2f(), FVector2f(), FVector2f(), FVector2f(),
					FVector3f(0, 1, 0), MaterialID_Skirting_Cap);
			}
			// Right Side of End Cap Skirting
			{
				FVector3d P_RT_End_Cap(Length, -HalfThickness, BaseZ + SkirtingHeight_Cap);
				AddQuad(Mesh, P_EC_Out_RT, P_EC_Out_RB, P_RB_End, P_RT_End_Cap,
					FVector2f(), FVector2f(), FVector2f(), FVector2f(),
					FVector3f(0, -1, 0), MaterialID_Skirting_Cap);
			}
		}

		// Transform the entire mesh
		UGeometryScriptLibrary_MeshTransformFunctions::TransformMesh(TargetMesh, Transform);

	}, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);
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
	float SkirtingHeight_Left,
	float SkirtingThickness_Left,
	float SkirtingHeight_Right,
	float SkirtingThickness_Right,
	float SkirtingHeight_Cap,
	float SkirtingThickness_Cap,
	int32 MaterialID_Left,
	int32 MaterialID_Right,
	int32 MaterialID_Caps,
	int32 MaterialID_Skirting_Left,
	int32 MaterialID_Skirting_Right,
	int32 MaterialID_Skirting_Cap
)
{
	if (!TargetMesh || FMath::Abs(SweepAngleDeg) < 0.1f) return;

	float CenterRadius = FVector2D::Distance(ArcCenter, StartPoint);
	if (CenterRadius < 0.1f) return;

	float HalfThickness = Thickness * 0.5f;
	float InnerRadius = CenterRadius - HalfThickness;
	float OuterRadius = CenterRadius + HalfThickness;

	if (InnerRadius < 0.1f) InnerRadius = 0.1f;

	FVector2D ToStart = StartPoint - ArcCenter;
	float StartAngleRad = FMath::Atan2(ToStart.Y, ToStart.X);

	NumSegments = FMath::Max(NumSegments, FMath::CeilToInt(FMath::Abs(SweepAngleDeg) / 15.0f));
	NumSegments = FMath::Clamp(NumSegments, 4, 128);
	
	float SweepRad = FMath::DegreesToRadians(SweepAngleDeg);
	float StepAngle = SweepRad / (float)NumSegments;

	float TopZ = BaseZ + Height;
	float UVScale = 0.01f;

	TargetMesh->EditMesh([&](FDynamicMesh3& Mesh)
	{
		if (!Mesh.HasAttributes()) Mesh.EnableAttributes();

		for (int32 i = 0; i < NumSegments; ++i)
		{
			float Angle0 = StartAngleRad + StepAngle * i;
			float Angle1 = StartAngleRad + StepAngle * (i + 1);
			float CosA0 = FMath::Cos(Angle0), SinA0 = FMath::Sin(Angle0);
			float CosA1 = FMath::Cos(Angle1), SinA1 = FMath::Sin(Angle1);

			// Determine which side is Left/Right based on sweep direction
			// If Sweep > 0 (CCW): Inner is Left, Outer is Right.
			// If Sweep < 0 (CW): Inner is Right, Outer is Left.
			int32 InnerMat = SweepAngleDeg > 0 ? MaterialID_Left : MaterialID_Right;
			int32 OuterMat = SweepAngleDeg > 0 ? MaterialID_Right : MaterialID_Left;
			int32 SkirtInnerMat = SweepAngleDeg > 0 ? MaterialID_Skirting_Left : MaterialID_Skirting_Right;
			int32 SkirtOuterMat = SweepAngleDeg > 0 ? MaterialID_Skirting_Right : MaterialID_Skirting_Left;
			
			float SkirtHInner = SweepAngleDeg > 0 ? SkirtingHeight_Left : SkirtingHeight_Right;
			float SkirtHOuter = SweepAngleDeg > 0 ? SkirtingHeight_Right : SkirtingHeight_Left;
			float SkirtTInner = SweepAngleDeg > 0 ? SkirtingThickness_Left : SkirtingThickness_Right;
			float SkirtTOuter = SweepAngleDeg > 0 ? SkirtingThickness_Right : SkirtingThickness_Left;

			// Define profile points for segment start (0) and end (1)
			FVector2D P_Inner0 = ArcCenter + FVector2D(CosA0, SinA0) * InnerRadius;
			FVector2D P_Outer0 = ArcCenter + FVector2D(CosA0, SinA0) * OuterRadius;
			FVector2D P_Inner1 = ArcCenter + FVector2D(CosA1, SinA1) * InnerRadius;
			FVector2D P_Outer1 = ArcCenter + FVector2D(CosA1, SinA1) * OuterRadius;

			// Skirting profile points
			FVector2D P_Skirting_Inner0 = ArcCenter + FVector2D(CosA0, SinA0) * (InnerRadius - SkirtTInner);
			FVector2D P_Skirting_Outer0 = ArcCenter + FVector2D(CosA0, SinA0) * (OuterRadius + SkirtTOuter);
			FVector2D P_Skirting_Inner1 = ArcCenter + FVector2D(CosA1, SinA1) * (InnerRadius - SkirtTInner);
			FVector2D P_Skirting_Outer1 = ArcCenter + FVector2D(CosA1, SinA1) * (OuterRadius + SkirtTOuter);

			// UVs
			float U0 = CenterRadius * StepAngle * i * UVScale;
			float U1 = CenterRadius * StepAngle * (i + 1) * UVScale;

			// Main Wall Faces
			FVector3d P_IRB0(P_Inner0, BaseZ), P_IRT0(P_Inner0, TopZ);
			FVector3d P_IRB1(P_Inner1, BaseZ), P_IRT1(P_Inner1, TopZ);
			
			// Inner Face
			if (SweepAngleDeg > 0)
			{
				// CCW: 1->0 (Flipped from previous 0->1)
				AddQuad(Mesh, P_IRB1, P_IRT1, P_IRT0, P_IRB0, 
					FVector2f(U1, 0), FVector2f(U1, Height * UVScale), FVector2f(U0, Height * UVScale), FVector2f(U0, 0), 
					FVector3f(-CosA0, -SinA0, 0), InnerMat);
			}
			else
			{
				// CW: 0->1 (Flipped from previous 1->0)
				AddQuad(Mesh, P_IRB0, P_IRT0, P_IRT1, P_IRB1, 
					FVector2f(U0, 0), FVector2f(U0, Height * UVScale), FVector2f(U1, Height * UVScale), FVector2f(U1, 0), 
					FVector3f(-CosA0, -SinA0, 0), InnerMat);
			}

			FVector3d P_OLB0(P_Outer0, BaseZ), P_OLT0(P_Outer0, TopZ);
			FVector3d P_OLB1(P_Outer1, BaseZ), P_OLT1(P_Outer1, TopZ);
			
			// Outer Face
			if (SweepAngleDeg > 0)
			{
				// CCW: 0->1 (Flipped from previous 1->0)
				AddQuad(Mesh, P_OLB0, P_OLT0, P_OLT1, P_OLB1, 
					FVector2f(U0, 0), FVector2f(U0, Height * UVScale), FVector2f(U1, Height * UVScale), FVector2f(U1, 0), 
					FVector3f(CosA0, SinA0, 0), OuterMat);
			}
			else
			{
				// CW: 1->0 (Flipped from previous 0->1)
				AddQuad(Mesh, P_OLB1, P_OLT1, P_OLT0, P_OLB0, 
					FVector2f(U1, 0), FVector2f(U1, Height * UVScale), FVector2f(U0, Height * UVScale), FVector2f(U0, 0), 
					FVector3f(CosA0, SinA0, 0), OuterMat);
			}

			// Skirting
			if (SkirtHInner > 0)
			{
				FVector3d P_SIB0(P_Skirting_Inner0, BaseZ), P_SIT0(P_Skirting_Inner0, BaseZ + SkirtHInner);
				FVector3d P_SIB1(P_Skirting_Inner1, BaseZ), P_SIT1(P_Skirting_Inner1, BaseZ + SkirtHInner);
				
				// Inner Skirting Face
				if (SweepAngleDeg > 0)
				{
					// CCW: 1->0 (Flipped)
					AddQuad(Mesh, P_SIB1, P_SIT1, P_SIT0, P_SIB0, 
						FVector2f(U1, 0), FVector2f(U1, SkirtHInner * UVScale), FVector2f(U0, SkirtHInner * UVScale), FVector2f(U0, 0), 
						FVector3f(-CosA0, -SinA0, 0), SkirtInnerMat);
				}
				else
				{
					// CW: 0->1 (Flipped)
					AddQuad(Mesh, P_SIB0, P_SIT0, P_SIT1, P_SIB1, 
						FVector2f(U0, 0), FVector2f(U0, SkirtHInner * UVScale), FVector2f(U1, SkirtHInner * UVScale), FVector2f(U1, 0), 
						FVector3f(-CosA0, -SinA0, 0), SkirtInnerMat);
				}
					
				// Inner Skirting Top
				// Connect Skirting Top to Wall Inner Surface at Skirting Height
				FVector3d P_Wall_IB0(P_Inner0, BaseZ + SkirtHInner);
				FVector3d P_Wall_IB1(P_Inner1, BaseZ + SkirtHInner);
				
				// Normal Up (0,0,1)
				// Winding depends on sweep direction
				if (SweepAngleDeg > 0)
				{
					// CCW: S0, S1, W1, W0
					// UVs: U -> Arc Length, V -> SkirtThickness
					AddQuad(Mesh, P_SIT0, P_SIT1, P_Wall_IB1, P_Wall_IB0, 
						FVector2f(U0, 0), FVector2f(U1, 0), 
						FVector2f(U1, SkirtTInner * UVScale), FVector2f(U0, SkirtTInner * UVScale), 
						FVector3f(0,0,1), SkirtInnerMat);
				}
				else
				{
					// CW: S1, S0, W0, W1
					AddQuad(Mesh, P_SIT1, P_SIT0, P_Wall_IB0, P_Wall_IB1, 
						FVector2f(U1, 0), FVector2f(U0, 0), 
						FVector2f(U0, SkirtTInner * UVScale), FVector2f(U1, SkirtTInner * UVScale), 
						FVector3f(0,0,1), SkirtInnerMat);
				}
			}
			
			if (SkirtHOuter > 0)
			{
				FVector3d P_SOB0(P_Skirting_Outer0, BaseZ), P_SOT0(P_Skirting_Outer0, BaseZ + SkirtHOuter);
				FVector3d P_SOB1(P_Skirting_Outer1, BaseZ), P_SOT1(P_Skirting_Outer1, BaseZ + SkirtHOuter);
				
				// Outer Skirting Face
				if (SweepAngleDeg > 0)
				{
					// CCW: 0->1 (Flipped)
					AddQuad(Mesh, P_SOB0, P_SOT0, P_SOT1, P_SOB1, 
						FVector2f(U0, 0), FVector2f(U0, SkirtHOuter * UVScale), FVector2f(U1, SkirtHOuter * UVScale), FVector2f(U1, 0), 
						FVector3f(CosA0, SinA0, 0), SkirtOuterMat);
				}
				else
				{
					// CW: 1->0 (Flipped)
					AddQuad(Mesh, P_SOB1, P_SOT1, P_SOT0, P_SOB0, 
						FVector2f(U1, 0), FVector2f(U1, SkirtHOuter * UVScale), FVector2f(U0, SkirtHOuter * UVScale), FVector2f(U0, 0), 
						FVector3f(CosA0, SinA0, 0), SkirtOuterMat);
				}
					
				// Outer Skirting Top
				// Connect Skirting Top to Wall Outer Surface at Skirting Height
				FVector3d P_Wall_OB0(P_Outer0, BaseZ + SkirtHOuter);
				FVector3d P_Wall_OB1(P_Outer1, BaseZ + SkirtHOuter);
				
				// Normal Up (0,0,1)
				// Winding depends on sweep direction
				if (SweepAngleDeg > 0)
				{
					// CCW: S0, W0, W1, S1 (Flipped from S0, S1, W1, W0)
					// UVs: U -> Arc Length, V -> SkirtThickness
					AddQuad(Mesh, P_SOT0, P_Wall_OB0, P_Wall_OB1, P_SOT1, 
						FVector2f(U0, 0), FVector2f(U0, SkirtTOuter * UVScale), 
						FVector2f(U1, SkirtTOuter * UVScale), FVector2f(U1, 0), 
						FVector3f(0,0,1), SkirtOuterMat);
				}
				else
				{
					// CW: S0, S1, W1, W0 (Flipped from S1, S0, W0, W1)
					// Wait, previous was S1, S0, W0, W1.
					// Flipped is S0, S1, W1, W0.
					// S0 (Left Bottom). S1 (Right Bottom). W1 (Right Top). W0 (Left Top).
					// S0 -> S1 -> W1 -> W0.
					// LB -> RB -> RT -> LT.
					// CCW.
					// So S0, S1, W1, W0 is correct for CW.
					AddQuad(Mesh, P_SOT0, P_SOT1, P_Wall_OB1, P_Wall_OB0, 
						FVector2f(U0, 0), FVector2f(U1, 0), 
						FVector2f(U1, SkirtTOuter * UVScale), FVector2f(U0, SkirtTOuter * UVScale), 
						FVector3f(0,0,1), SkirtOuterMat);
				}
			}

			// Top Cap
			if (SweepAngleDeg > 0)
			{
				// CCW: Inner->Inner (0->1)
				// P_IRT0, P_IRT1, P_OLT1, P_OLT0
				AddQuad(Mesh, P_IRT0, P_IRT1, P_OLT1, P_OLT0, 
					FVector2f(U0, 0), FVector2f(U1, 0), 
					FVector2f(U1, Thickness * UVScale), FVector2f(U0, Thickness * UVScale), 
					FVector3f(0,0,1), MaterialID_Caps);
			}
			else
			{
				// CW: Inner->Outer (0->1)
				// P_IRT0, P_OLT0, P_OLT1, P_IRT1
				AddQuad(Mesh, P_IRT0, P_OLT0, P_OLT1, P_IRT1, 
					FVector2f(U0, 0), FVector2f(U0, Thickness * UVScale), 
					FVector2f(U1, Thickness * UVScale), FVector2f(U1, 0), 
					FVector3f(0,0,1), MaterialID_Caps);
			}
			
			// Start Cap (at i=0)
			if (i == 0)
			{
				// Wall Start Cap
				if (SweepAngleDeg > 0)
				{
					// CCW
					AddQuad(Mesh, P_IRB0, P_IRT0, P_OLT0, P_OLB0, 
						FVector2f(0, 0), FVector2f(0, Height * UVScale), 
						FVector2f(Thickness * UVScale, Height * UVScale), FVector2f(Thickness * UVScale, 0), 
						FVector3f(FMath::Sin(Angle0), -FMath::Cos(Angle0), 0), MaterialID_Caps);
						
					// Inner Skirting Start Cap
					if (SkirtHInner > 0)
					{
						FVector3d P_SIT0(P_Skirting_Inner0, BaseZ + SkirtHInner);
						FVector3d P_SIB0(P_Skirting_Inner0, BaseZ);
						FVector3d P_W_IB_H(P_Inner0, BaseZ + SkirtHInner);
						// P_SIT0, P_W_IB_H, P_IRB0, P_SIB0
						AddQuad(Mesh, P_SIT0, P_W_IB_H, P_IRB0, P_SIB0, 
							FVector2f(SkirtTInner * UVScale, SkirtHInner * UVScale), FVector2f(0, SkirtHInner * UVScale), 
							FVector2f(0, 0), FVector2f(SkirtTInner * UVScale, 0), 
							FVector3f(FMath::Sin(Angle0), -FMath::Cos(Angle0), 0), SkirtInnerMat);
					}
					
					// Outer Skirting Start Cap
					if (SkirtHOuter > 0)
					{
						FVector3d P_SOT0(P_Skirting_Outer0, BaseZ + SkirtHOuter);
						FVector3d P_SOB0(P_Skirting_Outer0, BaseZ);
						FVector3d P_W_OB_H(P_Outer0, BaseZ + SkirtHOuter);
						// P_SOT0, P_SOB0, P_OLB0, P_W_OB_H
						AddQuad(Mesh, P_SOT0, P_SOB0, P_OLB0, P_W_OB_H, 
							FVector2f(SkirtTOuter * UVScale, SkirtHOuter * UVScale), FVector2f(SkirtTOuter * UVScale, 0), 
							FVector2f(0, 0), FVector2f(0, SkirtHOuter * UVScale), 
							FVector3f(FMath::Sin(Angle0), -FMath::Cos(Angle0), 0), SkirtOuterMat);
					}
				}
				else
				{
					// CW
					AddQuad(Mesh, P_OLB0, P_OLT0, P_IRT0, P_IRB0, 
						FVector2f(0, 0), FVector2f(0, Height * UVScale), 
						FVector2f(Thickness * UVScale, Height * UVScale), FVector2f(Thickness * UVScale, 0), 
						FVector3f(-FMath::Sin(Angle0), FMath::Cos(Angle0), 0), MaterialID_Caps);
						
					// Inner Skirting Start Cap (CW)
					if (SkirtHInner > 0)
					{
						FVector3d P_SIT0(P_Skirting_Inner0, BaseZ + SkirtHInner);
						FVector3d P_SIB0(P_Skirting_Inner0, BaseZ);
						FVector3d P_W_IB_H(P_Inner0, BaseZ + SkirtHInner);
						// Reverse of CCW: P_SIB0, P_IRB0, P_W_IB_H, P_SIT0
						AddQuad(Mesh, P_SIB0, P_IRB0, P_W_IB_H, P_SIT0, 
							FVector2f(SkirtTInner * UVScale, 0), FVector2f(0, 0), 
							FVector2f(0, SkirtHInner * UVScale), FVector2f(SkirtTInner * UVScale, SkirtHInner * UVScale), 
							FVector3f(-FMath::Sin(Angle0), FMath::Cos(Angle0), 0), SkirtInnerMat);
					}
					
					// Outer Skirting Start Cap (CW)
					if (SkirtHOuter > 0)
					{
						FVector3d P_SOT0(P_Skirting_Outer0, BaseZ + SkirtHOuter);
						FVector3d P_SOB0(P_Skirting_Outer0, BaseZ);
						FVector3d P_W_OB_H(P_Outer0, BaseZ + SkirtHOuter);
						// Reverse of CCW: P_W_OB_H, P_OLB0, P_SOB0, P_SOT0
						AddQuad(Mesh, P_W_OB_H, P_OLB0, P_SOB0, P_SOT0, 
							FVector2f(0, SkirtHOuter * UVScale), FVector2f(0, 0), 
							FVector2f(SkirtTOuter * UVScale, 0), FVector2f(SkirtTOuter * UVScale, SkirtHOuter * UVScale), 
							FVector3f(-FMath::Sin(Angle0), FMath::Cos(Angle0), 0), SkirtOuterMat);
					}
				}
			}
			
			// End Cap (at i=NumSegments-1)
			if (i == NumSegments - 1)
			{
				// Wall End Cap
				if (SweepAngleDeg > 0)
				{
					// CCW
					AddQuad(Mesh, P_OLB1, P_OLT1, P_IRT1, P_IRB1, 
						FVector2f(0, 0), FVector2f(0, Height * UVScale), 
						FVector2f(Thickness * UVScale, Height * UVScale), FVector2f(Thickness * UVScale, 0), 
						FVector3f(-FMath::Sin(Angle1), FMath::Cos(Angle1), 0), MaterialID_Caps);
						
					// Inner Skirting End Cap
					if (SkirtHInner > 0)
					{
						FVector3d P_SIT1(P_Skirting_Inner1, BaseZ + SkirtHInner);
						FVector3d P_SIB1(P_Skirting_Inner1, BaseZ);
						FVector3d P_W_IB_H(P_Inner1, BaseZ + SkirtHInner);
						// P_SIT1, P_SIB1, P_IRB1, P_W_IB_H
						AddQuad(Mesh, P_SIT1, P_SIB1, P_IRB1, P_W_IB_H, 
							FVector2f(SkirtTInner * UVScale, SkirtHInner * UVScale), FVector2f(SkirtTInner * UVScale, 0), 
							FVector2f(0, 0), FVector2f(0, SkirtHInner * UVScale), 
							FVector3f(-FMath::Sin(Angle1), FMath::Cos(Angle1), 0), SkirtInnerMat);
					}
					
					// Outer Skirting End Cap
					if (SkirtHOuter > 0)
					{
						FVector3d P_SOT1(P_Skirting_Outer1, BaseZ + SkirtHOuter);
						FVector3d P_SOB1(P_Skirting_Outer1, BaseZ);
						FVector3d P_W_OB_H(P_Outer1, BaseZ + SkirtHOuter);
						// P_SOT1, P_W_OB_H, P_OLB1, P_SOB1
						AddQuad(Mesh, P_SOT1, P_W_OB_H, P_OLB1, P_SOB1, 
							FVector2f(SkirtTOuter * UVScale, SkirtHOuter * UVScale), FVector2f(0, SkirtHOuter * UVScale), 
							FVector2f(0, 0), FVector2f(SkirtTOuter * UVScale, 0), 
							FVector3f(-FMath::Sin(Angle1), FMath::Cos(Angle1), 0), SkirtOuterMat);
					}
				}
				else
				{
					// CW
					AddQuad(Mesh, P_IRB1, P_IRT1, P_OLT1, P_OLB1, 
						FVector2f(0, 0), FVector2f(0, Height * UVScale), 
						FVector2f(Thickness * UVScale, Height * UVScale), FVector2f(Thickness * UVScale, 0), 
						FVector3f(FMath::Sin(Angle1), -FMath::Cos(Angle1), 0), MaterialID_Caps);
						
					// Inner Skirting End Cap (CW)
					if (SkirtHInner > 0)
					{
						FVector3d P_SIT1(P_Skirting_Inner1, BaseZ + SkirtHInner);
						FVector3d P_SIB1(P_Skirting_Inner1, BaseZ);
						FVector3d P_W_IB_H(P_Inner1, BaseZ + SkirtHInner);
						// Reverse of CCW: P_W_IB_H, P_IRB1, P_SIB1, P_SIT1
						AddQuad(Mesh, P_W_IB_H, P_IRB1, P_SIB1, P_SIT1, 
							FVector2f(0, SkirtHInner * UVScale), FVector2f(0, 0), 
							FVector2f(SkirtTInner * UVScale, 0), FVector2f(SkirtTInner * UVScale, SkirtHInner * UVScale), 
							FVector3f(FMath::Sin(Angle1), -FMath::Cos(Angle1), 0), SkirtInnerMat);
					}
					
					// Outer Skirting End Cap (CW)
					if (SkirtHOuter > 0)
					{
						FVector3d P_SOT1(P_Skirting_Outer1, BaseZ + SkirtHOuter);
						FVector3d P_SOB1(P_Skirting_Outer1, BaseZ);
						FVector3d P_W_OB_H(P_Outer1, BaseZ + SkirtHOuter);
						// Reverse of CCW: P_SOB1, P_OLB1, P_W_OB_H, P_SOT1
						AddQuad(Mesh, P_SOB1, P_OLB1, P_W_OB_H, P_SOT1, 
							FVector2f(SkirtTOuter * UVScale, 0), FVector2f(0, 0), 
							FVector2f(0, SkirtHOuter * UVScale), FVector2f(SkirtTOuter * UVScale, SkirtHOuter * UVScale), 
							FVector3f(FMath::Sin(Angle1), -FMath::Cos(Angle1), 0), SkirtOuterMat);
					}
				}
			}
		}

		// Caps (simplified for now)
		// TODO: Add proper caps with skirting profile

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
