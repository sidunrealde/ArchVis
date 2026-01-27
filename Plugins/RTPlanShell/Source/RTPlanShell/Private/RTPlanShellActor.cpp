#include "RTPlanShellActor.h"
#include "Components/DynamicMeshComponent.h"
#include "RTPlanMeshBuilder.h"
#include "RTPlanGeometryUtils.h"
#include "RTPlanOpeningUtils.h"
#include "UDynamicMesh.h"

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
				0, 0, 0  // Material IDs
			);
			
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
				
				FVector2D SegCenter2D = A + Dir * MidDist;
				FVector SegCenter(SegCenter2D.X, SegCenter2D.Y, ZCenter);

				FTransform SegTransform;
				SegTransform.SetLocation(SegCenter);
				SegTransform.SetRotation(WallRotation);

				FRTPlanMeshBuilder::AppendWallMesh(
					Mesh,
					SegTransform,
					SegLength,
					Wall.ThicknessCm,
					Wall.HeightCm,
					0, 0, 0
				);

				// Skirting for wall segments with openings
				if (Wall.bHasLeftSkirting)
				{
					float OffsetY = (Wall.ThicknessCm * 0.5f) + (Wall.LeftSkirtingThicknessCm * 0.5f);
					float SkirtingZCenter = Wall.BaseZCm + (Wall.LeftSkirtingHeightCm * 0.5f);
					
					FVector SkirtingCenter = SegCenter + WallRotation.RotateVector(FVector(0, OffsetY, SkirtingZCenter - ZCenter));
					
					FTransform SkirtingTransform;
					SkirtingTransform.SetLocation(SkirtingCenter);
					SkirtingTransform.SetRotation(WallRotation);

					FRTPlanMeshBuilder::AppendWallMesh(
						Mesh,
						SkirtingTransform,
						SegLength,
						Wall.LeftSkirtingThicknessCm,
						Wall.LeftSkirtingHeightCm,
						0, 0, 0
					);
				}

				if (Wall.bHasRightSkirting)
				{
					float OffsetY = -((Wall.ThicknessCm * 0.5f) + (Wall.RightSkirtingThicknessCm * 0.5f));
					float SkirtingZCenter = Wall.BaseZCm + (Wall.RightSkirtingHeightCm * 0.5f);
					
					FVector SkirtingCenter = SegCenter + WallRotation.RotateVector(FVector(0, OffsetY, SkirtingZCenter - ZCenter));
					
					FTransform SkirtingTransform;
					SkirtingTransform.SetLocation(SkirtingCenter);
					SkirtingTransform.SetRotation(WallRotation);

					FRTPlanMeshBuilder::AppendWallMesh(
						Mesh,
						SkirtingTransform,
						SegLength,
						Wall.RightSkirtingThicknessCm,
						Wall.RightSkirtingHeightCm,
						0, 0, 0
					);
				}
			}
		}
		else
		{
			// Full Wall (No Openings)
			FVector2D Mid = (ExtendedA + ExtendedB) * 0.5f;
			
			FTransform WallTransform;
			WallTransform.SetLocation(FVector(Mid.X, Mid.Y, ZCenter));
			WallTransform.SetRotation(WallRotation);

			UE_LOG(LogRTPlanShell, Verbose, TEXT("Building Wall: Mid=(%s), ZCenter=%f, Height=%f, ExtendedLength=%f"), 
				*Mid.ToString(), ZCenter, Wall.HeightCm, ExtendedLength);

			FRTPlanMeshBuilder::AppendWallMesh(
				Mesh,
				WallTransform,
				ExtendedLength,
				Wall.ThicknessCm,
				Wall.HeightCm,
				0, 0, 0
			);

			// Skirting for full walls
			if (Wall.bHasLeftSkirting)
			{
				float OffsetY = (Wall.ThicknessCm * 0.5f) + (Wall.LeftSkirtingThicknessCm * 0.5f);
				float SkirtingZCenter = Wall.BaseZCm + (Wall.LeftSkirtingHeightCm * 0.5f);
				
				FVector SkirtingCenter = WallTransform.GetLocation() + WallRotation.RotateVector(FVector(0, OffsetY, SkirtingZCenter - ZCenter));
				
				FTransform SkirtingTransform;
				SkirtingTransform.SetLocation(SkirtingCenter);
				SkirtingTransform.SetRotation(WallRotation);

				FRTPlanMeshBuilder::AppendWallMesh(
					Mesh,
					SkirtingTransform,
					ExtendedLength,
					Wall.LeftSkirtingThicknessCm,
					Wall.LeftSkirtingHeightCm,
					0, 0, 0
				);
			}

			if (Wall.bHasRightSkirting)
			{
				float OffsetY = -((Wall.ThicknessCm * 0.5f) + (Wall.RightSkirtingThicknessCm * 0.5f));
				float SkirtingZCenter = Wall.BaseZCm + (Wall.RightSkirtingHeightCm * 0.5f);
				
				FVector SkirtingCenter = WallTransform.GetLocation() + WallRotation.RotateVector(FVector(0, OffsetY, SkirtingZCenter - ZCenter));
				
				FTransform SkirtingTransform;
				SkirtingTransform.SetLocation(SkirtingCenter);
				SkirtingTransform.SetRotation(WallRotation);

				FRTPlanMeshBuilder::AppendWallMesh(
					Mesh,
					SkirtingTransform,
					ExtendedLength,
					Wall.RightSkirtingThicknessCm,
					Wall.RightSkirtingHeightCm,
					0, 0, 0
				);
			}
		}

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
