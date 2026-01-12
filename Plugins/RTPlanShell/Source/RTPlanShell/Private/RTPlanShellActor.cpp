#include "RTPlanShellActor.h"
#include "Components/DynamicMeshComponent.h"
#include "RTPlanMeshBuilder.h"
#include "RTPlanGeometryUtils.h"
#include "RTPlanOpeningUtils.h"
#include "UDynamicMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogRTPlanShell, Log, All);

ARTPlanShellActor::ARTPlanShellActor()
{
	PrimaryActorTick.bCanEverTick = false;

	WallMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("WallMesh"));
	RootComponent = WallMeshComponent;
	
	// Enable complex collision for the dynamic mesh
	WallMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

void ARTPlanShellActor::BeginPlay()
{
	Super::BeginPlay();
}

void ARTPlanShellActor::SetDocument(URTPlanDocument* InDoc)
{
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
	RebuildAll();
}

void ARTPlanShellActor::RebuildAll()
{
	if (!Document || !WallMeshComponent) return;

	UDynamicMesh* Mesh = WallMeshComponent->GetDynamicMesh();
	if (!Mesh) return;

	// Clear existing mesh
	Mesh->Reset();

	const FRTPlanData& Data = Document->GetData();

	// Pre-process Openings: Map WallID -> List of Openings
	TMap<FGuid, TArray<FRTOpening>> WallOpenings;
	for (const auto& Pair : Data.Openings)
	{
		WallOpenings.FindOrAdd(Pair.Value.WallId).Add(Pair.Value);
	}

	// Iterate Walls
	for (const auto& Pair : Data.Walls)
	{
		const FRTWall& Wall = Pair.Value;
		
		if (Data.Vertices.Contains(Wall.VertexAId) && Data.Vertices.Contains(Wall.VertexBId))
		{
			FVector2D A = Data.Vertices[Wall.VertexAId].Position;
			FVector2D B = Data.Vertices[Wall.VertexBId].Position;

			float Length = FVector2D::Distance(A, B);
			if (Length < 1.0f) continue;

			// Calculate Transform Base
			FVector2D Dir = (B - A).GetSafeNormal();
			float Angle = FMath::Atan2(Dir.Y, Dir.X);
			FQuat WallRotation(FVector::UpVector, Angle);
			
			// Pivot Logic:
			// GeometryScript AppendBox creates a box centered at (0,0,0) with extents X/Y/Z.
			// So the bottom of the box is at Z = -Height/2.
			// To place the bottom at BaseZCm, we need to shift the location up by Height/2.
			float ZCenter = Wall.BaseZCm + (Wall.HeightCm * 0.5f);
			
			FVector WallStart(A.X, A.Y, ZCenter);

			// Get Openings for this wall
			TArray<FRTOpening>* Ops = WallOpenings.Find(Wall.Id);
			
			if (Ops && Ops->Num() > 0)
			{
				// Split Wall Logic
				TArray<FRTPlanOpeningUtils::FInterval> Solids = FRTPlanOpeningUtils::ComputeSolidIntervals(Length, *Ops);

				for (const auto& Solid : Solids)
				{
					float SegLength = Solid.End - Solid.Start;
					if (SegLength < 0.1f) continue;

					float MidDist = (Solid.Start + Solid.End) * 0.5f;
					
					// Calculate center of this segment in world space
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
				}
			}
			else
			{
				// Full Wall (No Openings)
				FVector2D Mid = (A + B) * 0.5f;
				
				FTransform WallTransform;
				WallTransform.SetLocation(FVector(Mid.X, Mid.Y, ZCenter));
				WallTransform.SetRotation(WallRotation);

				UE_LOG(LogRTPlanShell, Verbose, TEXT("Building Wall: Mid=(%s), ZCenter=%f, Height=%f"), *Mid.ToString(), ZCenter, Wall.HeightCm);

				FRTPlanMeshBuilder::AppendWallMesh(
					Mesh,
					WallTransform,
					Length,
					Wall.ThicknessCm,
					Wall.HeightCm,
					0, 0, 0
				);
			}
		}
	}
}
