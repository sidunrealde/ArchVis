#include "RTPlanShellActor.h"
#include "Components/DynamicMeshComponent.h"
#include "RTPlanMeshBuilder.h"
#include "RTPlanGeometryUtils.h"
#include "RTPlanOpeningUtils.h"
#include "RTPlanFinishCatalog.h"
#include "UDynamicMesh.h"
#include "Materials/Material.h"

DEFINE_LOG_CATEGORY(LogRTPlanShell);

ARTPlanShellActor::ARTPlanShellActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create a root scene component
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Legacy combined mesh component (kept for backwards compatibility but not used in per-wall mode)
	WallMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("WallMesh"));
	WallMeshComponent->SetupAttachment(RootComponent);
	
	// Enable complex collision for the dynamic mesh
	WallMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	WallMeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
	WallMeshComponent->SetComplexAsSimpleCollisionEnabled(true, true);
	
	UE_LOG(LogRTPlanShell, Log, TEXT("ARTPlanShellActor created"));
}

void ARTPlanShellActor::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogRTPlanShell, Log, TEXT("ARTPlanShellActor::BeginPlay"));
}

void ARTPlanShellActor::SetDocument(URTPlanDocument* InDoc)
{
	UE_LOG(LogRTPlanShell, Log, TEXT("SetDocument: %s"), InDoc ? TEXT("Valid") : TEXT("NULL"));
	
	if (Document)
	{
		Document->OnPlanChanged.RemoveDynamic(this, &ARTPlanShellActor::OnPlanChanged);
	}

	Document = InDoc;

	if (Document)
	{
		Document->OnPlanChanged.AddDynamic(this, &ARTPlanShellActor::OnPlanChanged);
		RebuildAll();
	}
}

void ARTPlanShellActor::OnPlanChanged()
{
	UE_LOG(LogRTPlanShell, Log, TEXT("OnPlanChanged triggered"));
	RebuildAll();
}

void ARTPlanShellActor::SetFinishCatalog(URTFinishCatalog* InCatalog)
{
	FinishCatalog = InCatalog;
	// Rebuild to apply new materials
	if (Document)
	{
		RebuildAll();
	}
}

UMaterialInterface* ARTPlanShellActor::GetMaterialForFinish(FName FinishId, UMaterialInterface* DefaultMat) const
{
	if (FinishCatalog && !FinishId.IsNone())
	{
		UMaterialInterface* Mat = FinishCatalog->GetMaterialForFinish(FinishId);
		if (Mat)
		{
			UE_LOG(LogRTPlanShell, Verbose, TEXT("GetMaterialForFinish: %s -> %s"), *FinishId.ToString(), *Mat->GetName());
			return Mat;
		}
		else
		{
			UE_LOG(LogRTPlanShell, Warning, TEXT("GetMaterialForFinish: %s NOT FOUND in catalog, using default"), *FinishId.ToString());
		}
	}
	return DefaultMat;
}

void ARTPlanShellActor::ApplyWallMaterials(UDynamicMeshComponent* MeshComp, const FRTWall& Wall)
{
	if (!MeshComp) return;

	// Material slot indices correspond to the MaterialID values used in AppendWallMesh:
	//   0 = Left wall face
	//   1 = Right wall face
	//   2 = Top face
	//   3 = Left cap (start)
	//   4 = Right cap (end)
	//   5 = Left skirting face
	//   6 = Left skirting top
	//   7 = Left skirting cap
	//   8 = Right skirting face
	//   9 = Right skirting top
	//  10 = Right skirting cap

	UE_LOG(LogRTPlanShell, Log, TEXT("ApplyWallMaterials: Wall=%s, FinishCatalog=%s, DefaultWallMat=%s"),
		*Wall.Id.ToString(),
		FinishCatalog ? *FinishCatalog->GetName() : TEXT("NULL"),
		DefaultWallMaterial ? *DefaultWallMaterial->GetName() : TEXT("NULL"));

	// Get a fallback material if defaults aren't set
	UMaterialInterface* FallbackMaterial = DefaultWallMaterial.Get();
	if (!FallbackMaterial)
	{
		// Use engine's default material as ultimate fallback
		FallbackMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	}
	
	UMaterialInterface* FallbackSkirtingMaterial = DefaultSkirtingMaterial.Get();
	if (!FallbackSkirtingMaterial)
	{
		FallbackSkirtingMaterial = FallbackMaterial;
	}

	// --- Wall faces ---
	UMaterialInterface* MatLeft = GetMaterialForFinish(Wall.FinishLeftId, FallbackMaterial);
	UMaterialInterface* MatRight = GetMaterialForFinish(Wall.FinishRightId, FallbackMaterial);
	UMaterialInterface* MatTop = GetMaterialForFinish(Wall.FinishTopId, FallbackMaterial);
	UMaterialInterface* MatLeftCap = GetMaterialForFinish(Wall.FinishLeftCapId, FallbackMaterial);
	UMaterialInterface* MatRightCap = GetMaterialForFinish(Wall.FinishRightCapId, FallbackMaterial);
	
	// --- Left Skirting ---
	UMaterialInterface* MatSkirtingLeft = GetMaterialForFinish(Wall.FinishLeftSkirtingId, FallbackSkirtingMaterial);
	UMaterialInterface* MatSkirtingLeftTop = GetMaterialForFinish(Wall.FinishLeftSkirtingTopId, FallbackSkirtingMaterial);
	UMaterialInterface* MatSkirtingLeftCap = GetMaterialForFinish(Wall.FinishLeftSkirtingCapId, FallbackSkirtingMaterial);
	
	// --- Right Skirting ---
	UMaterialInterface* MatSkirtingRight = GetMaterialForFinish(Wall.FinishRightSkirtingId, FallbackSkirtingMaterial);
	UMaterialInterface* MatSkirtingRightTop = GetMaterialForFinish(Wall.FinishRightSkirtingTopId, FallbackSkirtingMaterial);
	UMaterialInterface* MatSkirtingRightCap = GetMaterialForFinish(Wall.FinishRightSkirtingCapId, FallbackSkirtingMaterial);

	UE_LOG(LogRTPlanShell, Log, TEXT("  Materials: Left=%s, Right=%s, Top=%s, LeftCap=%s, RightCap=%s"),
		MatLeft ? *MatLeft->GetName() : TEXT("NULL"),
		MatRight ? *MatRight->GetName() : TEXT("NULL"),
		MatTop ? *MatTop->GetName() : TEXT("NULL"),
		MatLeftCap ? *MatLeftCap->GetName() : TEXT("NULL"),
		MatRightCap ? *MatRightCap->GetName() : TEXT("NULL"));

	// Build array of materials for ConfigureMaterialSet
	// This tells the DynamicMeshComponent which materials map to which triangle group IDs
	TArray<UMaterialInterface*> MaterialSet;
	MaterialSet.SetNum(11);
	MaterialSet[0] = MatLeft;
	MaterialSet[1] = MatRight;
	MaterialSet[2] = MatTop;
	MaterialSet[3] = MatLeftCap;
	MaterialSet[4] = MatRightCap;
	MaterialSet[5] = MatSkirtingLeft;
	MaterialSet[6] = MatSkirtingLeftTop;
	MaterialSet[7] = MatSkirtingLeftCap;
	MaterialSet[8] = MatSkirtingRight;
	MaterialSet[9] = MatSkirtingRightTop;
	MaterialSet[10] = MatSkirtingRightCap;

	// Configure the material set on the component - this maps triangle groups to material slots
	MeshComp->ConfigureMaterialSet(MaterialSet);
}

void ARTPlanShellActor::SetSelectedWalls(const TArray<FGuid>& WallIds)
{
	SelectedWallIds.Empty();
	for (const FGuid& Id : WallIds)
	{
		SelectedWallIds.Add(Id);
	}
	UpdateSelectionHighlight();
}

void ARTPlanShellActor::ClearSelection()
{
	SelectedWallIds.Empty();
	UpdateSelectionHighlight();
}

void ARTPlanShellActor::UpdateSelectionHighlight()
{
	// Update stencil values on all per-wall mesh components
	for (auto& Pair : WallMeshComponents)
	{
		if (UDynamicMeshComponent* MeshComp = Pair.Value.Get())
		{
			bool bIsSelected = SelectedWallIds.Contains(Pair.Key);
			
			// Enable/disable custom stencil
			MeshComp->SetRenderCustomDepth(bIsSelected);
			MeshComp->SetCustomDepthStencilValue(bIsSelected ? SelectionStencilValue : 0);
		}
	}
}

void ARTPlanShellActor::RebuildAll()
{
	if (!Document) return;
	
	UE_LOG(LogRTPlanShell, Log, TEXT("RebuildAll started"));

	const FRTPlanData& Data = Document->GetData();

	// Track which walls still exist
	TSet<FGuid> CurrentWallIds;
	for (const auto& Pair : Data.Walls)
	{
		CurrentWallIds.Add(Pair.Key);
	}

	// Remove mesh components for walls that no longer exist
	TArray<FGuid> WallsToRemove;
	for (auto& Pair : WallMeshComponents)
	{
		if (!CurrentWallIds.Contains(Pair.Key))
		{
			if (UDynamicMeshComponent* MeshComp = Pair.Value.Get())
			{
				MeshComp->DestroyComponent();
			}
			WallsToRemove.Add(Pair.Key);
		}
	}
	for (const FGuid& Id : WallsToRemove)
	{
		WallMeshComponents.Remove(Id);
	}

	// Pre-process Openings: Map WallID -> List of Openings
	TMap<FGuid, TArray<FRTOpening>> WallOpenings;
	for (const auto& Pair : Data.Openings)
	{
		WallOpenings.FindOrAdd(Pair.Value.WallId).Add(Pair.Value);
	}

	// Iterate Walls and create/update per-wall mesh components
	for (const auto& Pair : Data.Walls)
	{
		const FRTWall& Wall = Pair.Value;
		
		if (!Data.Vertices.Contains(Wall.VertexAId) || !Data.Vertices.Contains(Wall.VertexBId))
		{
			continue;
		}

		FVector2D A = Data.Vertices[Wall.VertexAId].Position;
		FVector2D B = Data.Vertices[Wall.VertexBId].Position;

		float Length = FVector2D::Distance(A, B);
		if (Length < 1.0f) continue;

		// Get or create mesh component for this wall
		UDynamicMeshComponent* WallMeshComp = nullptr;
		if (TObjectPtr<UDynamicMeshComponent>* ExistingPtr = WallMeshComponents.Find(Wall.Id))
		{
			WallMeshComp = ExistingPtr->Get();
		}
		
		if (!WallMeshComp)
		{
			FName CompName = *FString::Printf(TEXT("WallMesh_%s"), *Wall.Id.ToString());
			WallMeshComp = NewObject<UDynamicMeshComponent>(this, CompName);
			WallMeshComp->SetupAttachment(RootComponent);
			WallMeshComp->RegisterComponent();
			
			// Configure collision
			WallMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WallMeshComp->SetCollisionProfileName(TEXT("BlockAll"));
			WallMeshComp->SetComplexAsSimpleCollisionEnabled(true, true);
			
			WallMeshComponents.Add(Wall.Id, WallMeshComp);
		}

		// Clear and rebuild this wall's mesh
		UDynamicMesh* Mesh = WallMeshComp->GetDynamicMesh();
		if (!Mesh) continue;
		Mesh->Reset();

		// Handle curved walls (arcs)
		if (Wall.bIsArc && FMath::Abs(Wall.ArcSweepAngle) > 0.1f)
		{
			// Use wall's segment count if specified, otherwise use default
			int32 NumSegments = (Wall.ArcNumSegments > 0) ? Wall.ArcNumSegments : 32;
			
			FRTPlanMeshBuilder::AppendCurvedWallMesh(
				Mesh,
				A,  // Start point
				B,  // End point
				Wall.ArcCenter,
				Wall.ArcSweepAngle,
				Wall.ThicknessCm,
				Wall.HeightCm,
				Wall.BaseZCm,
				NumSegments,
				Wall.bHasLeftSkirting ? Wall.LeftSkirtingHeightCm : 0.0f,
				Wall.bHasLeftSkirting ? Wall.LeftSkirtingThicknessCm : 0.0f,
				Wall.bHasRightSkirting ? Wall.RightSkirtingHeightCm : 0.0f,
				Wall.bHasRightSkirting ? Wall.RightSkirtingThicknessCm : 0.0f,
				Wall.bHasCapSkirting ? Wall.CapSkirtingHeightCm : 0.0f,
				Wall.bHasCapSkirting ? Wall.CapSkirtingThicknessCm : 0.0f,
				0,  // MaterialID_Left
				1,  // MaterialID_Right
				2,  // MaterialID_Top
				3,  // MaterialID_LeftCap
				4,  // MaterialID_RightCap
				5,  // MaterialID_SkirtingLeft
				6,  // MaterialID_SkirtingLeftTop
				7,  // MaterialID_SkirtingLeftCap
				8,  // MaterialID_SkirtingRight
				9,  // MaterialID_SkirtingRightTop
				10  // MaterialID_SkirtingRightCap
			);
			
			// Apply materials from finish catalog
			ApplyWallMaterials(WallMeshComp, Wall);
			
			// Apply selection highlight
			bool bIsSelected = SelectedWallIds.Contains(Wall.Id);
			WallMeshComp->SetRenderCustomDepth(bIsSelected);
			WallMeshComp->SetCustomDepthStencilValue(bIsSelected ? SelectionStencilValue : 0);
			continue;  // Skip straight wall processing
		}

		// Calculate Transform Base for straight wall
		FVector2D Dir = (B - A).GetSafeNormal();
		float Angle = FMath::Atan2(Dir.Y, Dir.X);
		FQuat WallRotation(FVector::UpVector, Angle);
		
		// Extend wall by half thickness at each end to eliminate corner gaps
		float HalfThickness = Wall.ThicknessCm * 0.5f;
		FVector2D ExtendedA = A - Dir * HalfThickness;
		FVector2D ExtendedB = B + Dir * HalfThickness;
		float ExtendedLength = Length + Wall.ThicknessCm;
		
		float ZCenter = Wall.BaseZCm + (Wall.HeightCm * 0.5f);

		// Get Openings for this wall
		TArray<FRTOpening>* Ops = WallOpenings.Find(Wall.Id);
		
		if (Ops && Ops->Num() > 0)
		{
			// Split Wall Logic - use original length for opening calculations
			TArray<FRTPlanOpeningUtils::FInterval> Solids = FRTPlanOpeningUtils::ComputeSolidIntervals(Length, *Ops);

			for (int32 i = 0; i < Solids.Num(); ++i)
			{
				const auto& Solid = Solids[i];
				float SegStart = Solid.Start;
				float SegEnd = Solid.End;
				
				// Extend first segment backward and last segment forward
				if (i == 0)
				{
					SegStart -= HalfThickness;
				}
				if (i == Solids.Num() - 1)
				{
					SegEnd += HalfThickness;
				}
				
				float SegLength = SegEnd - SegStart;
				if (SegLength < 0.1f) continue;

				float MidDist = (SegStart + SegEnd) * 0.5f;
				
				FVector2D SegStartPos2D = A + Dir * SegStart;
				FVector SegStartPos(SegStartPos2D.X, SegStartPos2D.Y, 0); // Z is handled by BaseZ param

				FTransform SegTransform;
				SegTransform.SetLocation(SegStartPos);
				SegTransform.SetRotation(WallRotation);

				FRTPlanMeshBuilder::AppendWallMesh(
					Mesh,
					SegTransform,
					SegLength,
					Wall.ThicknessCm,
					Wall.HeightCm,
					Wall.BaseZCm,
					Wall.bHasLeftSkirting ? Wall.LeftSkirtingHeightCm : 0.0f,
					Wall.bHasLeftSkirting ? Wall.LeftSkirtingThicknessCm : 0.0f,
					Wall.bHasRightSkirting ? Wall.RightSkirtingHeightCm : 0.0f,
					Wall.bHasRightSkirting ? Wall.RightSkirtingThicknessCm : 0.0f,
					Wall.bHasCapSkirting ? Wall.CapSkirtingHeightCm : 0.0f,
					Wall.bHasCapSkirting ? Wall.CapSkirtingThicknessCm : 0.0f,
					0,  // MaterialID_Left
					1,  // MaterialID_Right
					2,  // MaterialID_Top
					3,  // MaterialID_LeftCap
					4,  // MaterialID_RightCap
					5,  // MaterialID_SkirtingLeft
					6,  // MaterialID_SkirtingLeftTop
					7,  // MaterialID_SkirtingLeftCap
					8,  // MaterialID_SkirtingRight
					9,  // MaterialID_SkirtingRightTop
					10  // MaterialID_SkirtingRightCap
				);
			}
			
			// Apply materials from finish catalog (walls with openings)
			ApplyWallMaterials(WallMeshComp, Wall);
			
			// Apply selection highlight
			bool bIsSelected = SelectedWallIds.Contains(Wall.Id);
			WallMeshComp->SetRenderCustomDepth(bIsSelected);
			WallMeshComp->SetCustomDepthStencilValue(bIsSelected ? SelectionStencilValue : 0);
		}
		else
		{
			// Full Wall (No Openings)
			// Transform at Start (ExtendedA)
			
			FTransform WallTransform;
			WallTransform.SetLocation(FVector(ExtendedA.X, ExtendedA.Y, 0));
			WallTransform.SetRotation(WallRotation);

			UE_LOG(LogRTPlanShell, Verbose, TEXT("Building Wall: Start=(%s), Height=%f, ExtendedLength=%f"), 
				*ExtendedA.ToString(), Wall.HeightCm, ExtendedLength);

			FRTPlanMeshBuilder::AppendWallMesh(
				Mesh,
				WallTransform,
				ExtendedLength,
				Wall.ThicknessCm,
				Wall.HeightCm,
				Wall.BaseZCm,
				Wall.bHasLeftSkirting ? Wall.LeftSkirtingHeightCm : 0.0f,
				Wall.bHasLeftSkirting ? Wall.LeftSkirtingThicknessCm : 0.0f,
				Wall.bHasRightSkirting ? Wall.RightSkirtingHeightCm : 0.0f,
				Wall.bHasRightSkirting ? Wall.RightSkirtingThicknessCm : 0.0f,
				Wall.bHasCapSkirting ? Wall.CapSkirtingHeightCm : 0.0f,
				Wall.bHasCapSkirting ? Wall.CapSkirtingThicknessCm : 0.0f,
				0,  // MaterialID_Left
				1,  // MaterialID_Right
				2,  // MaterialID_Top
				3,  // MaterialID_LeftCap
				4,  // MaterialID_RightCap
				5,  // MaterialID_SkirtingLeft
				6,  // MaterialID_SkirtingLeftTop
				7,  // MaterialID_SkirtingLeftCap
				8,  // MaterialID_SkirtingRight
				9,  // MaterialID_SkirtingRightTop
				10  // MaterialID_SkirtingRightCap
			);
		}

		// Apply materials from finish catalog
		ApplyWallMaterials(WallMeshComp, Wall);

		// Apply selection highlight if this wall is selected
		bool bIsSelected = SelectedWallIds.Contains(Wall.Id);
		WallMeshComp->SetRenderCustomDepth(bIsSelected);
		WallMeshComp->SetCustomDepthStencilValue(bIsSelected ? SelectionStencilValue : 0);
	}

	// Hide the legacy combined mesh component since we're using per-wall components
	if (WallMeshComponent)
	{
		WallMeshComponent->SetVisibility(false);
	}
}
