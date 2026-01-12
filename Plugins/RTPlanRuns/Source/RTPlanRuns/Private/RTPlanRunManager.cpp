#include "RTPlanRunManager.h"
#include "RTPlanRunSolver.h"
#include "RTPlanCommand.h" // Need to emit commands to update objects

ARTPlanRunManager::ARTPlanRunManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARTPlanRunManager::BeginPlay()
{
	Super::BeginPlay();
}

void ARTPlanRunManager::SetDocument(URTPlanDocument* InDoc)
{
	if (Document)
	{
		Document->OnPlanChanged.RemoveDynamic(this, &ARTPlanRunManager::OnPlanChanged);
	}

	Document = InDoc;

	if (Document)
	{
		Document->OnPlanChanged.AddDynamic(this, &ARTPlanRunManager::OnPlanChanged);
		// Initial generation? 
		// Note: Regenerating runs modifies the document. 
		// We must be careful not to cause infinite loops if we listen to OnPlanChanged.
		// Strategy: Only regenerate if Run definitions changed, or explicit call.
		// For V1, let's assume explicit call or careful dirty checking.
	}
}

void ARTPlanRunManager::OnPlanChanged()
{
	// Check if any Runs are dirty?
	// For now, we won't auto-regenerate on every change to avoid loops.
	// The Tool or UI should call RegenerateRuns() when a Run is edited.
}

void ARTPlanRunManager::RegenerateRuns()
{
	if (!Document) return;

	FRTPlanData& Data = Document->GetDataMutable(); // Direct access for V1 logic
	
	// 1. Clear existing generated objects
	TArray<FGuid> ToRemove;
	for (const auto& Pair : Data.Objects)
	{
		if (Pair.Value.GeneratedByRunId.IsValid())
		{
			ToRemove.Add(Pair.Key);
		}
	}
	for (const FGuid& Id : ToRemove)
	{
		Data.Objects.Remove(Id);
	}

	// 2. Solve and Generate new objects
	for (const auto& Pair : Data.Runs)
	{
		const FRTCabinetRun& Run = Pair.Value;
		
		// Calculate Length
		// If Wall hosted, we need wall length.
		// For V1, let's assume Start/End offsets define the run length directly if WallId is invalid,
		// or relative to wall if valid.
		
		float RunLength = 0.0f;
		FTransform RunStartTransform = FTransform::Identity;

		if (Data.Walls.Contains(Run.HostWallId))
		{
			const FRTWall& Wall = Data.Walls[Run.HostWallId];
			if (Data.Vertices.Contains(Wall.VertexAId) && Data.Vertices.Contains(Wall.VertexBId))
			{
				FVector2D A = Data.Vertices[Wall.VertexAId].Position;
				FVector2D B = Data.Vertices[Wall.VertexBId].Position;
				FVector2D Dir = (B - A).GetSafeNormal();
				float WallLen = FVector2D::Distance(A, B);
				
				// Clamp offsets
				float Start = FMath::Max(0.0f, Run.StartOffsetCm);
				float End = FMath::Min(WallLen, WallLen - Run.EndOffsetCm);
				
				if (End > Start)
				{
					RunLength = End - Start;
					
					// Calculate Start Transform (Position + Rotation)
					FVector2D Pos2D = A + Dir * Start;
					float Angle = FMath::Atan2(Dir.Y, Dir.X);
					
					RunStartTransform.SetLocation(FVector(Pos2D.X, Pos2D.Y, 0)); // Floor Z
					RunStartTransform.SetRotation(FQuat(FVector::UpVector, Angle));
				}
			}
		}

		if (RunLength > 1.0f)
		{
			FRTPlanRunSolver::FRunInput Input;
			Input.TotalLength = RunLength;
			Input.Depth = Run.DepthCm;
			Input.Height = Run.HeightCm;

			auto Modules = FRTPlanRunSolver::Solve(Input);

			for (const auto& Mod : Modules)
			{
				FRTInteriorInstance NewObj;
				NewObj.Id = FGuid::NewGuid();
				NewObj.GeneratedByRunId = Run.Id;
				NewObj.ProductTypeId = Mod.ProductId;
				NewObj.HostType = ERTHostType::Floor; // Or Wall?
				
				// Calculate Transform relative to Run Start
				// Offset along X (Run direction)
				// Center of module? Or Pivot at corner?
				// Assume Pivot is Bottom-Left-Back
				FVector LocalPos(Mod.Offset, 0, 0); 
				
				NewObj.Transform = FTransform(LocalPos) * RunStartTransform;
				
				Data.Objects.Add(NewObj.Id, NewObj);
			}
		}
	}

	// Notify change so ObjectManager can spawn actors
	Document->OnPlanChanged.Broadcast();
}
