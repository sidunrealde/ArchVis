#include "RTPlanRunSolver.h"

TArray<FRTPlanRunSolver::FGeneratedModule> FRTPlanRunSolver::Solve(const FRunInput& Input)
{
	TArray<FGeneratedModule> Modules;
	float RemainingLength = Input.TotalLength;
	float CurrentOffset = 0.0f;

	// Standard widths in descending preference
	const float StandardWidths[] = { 60.0f, 45.0f, 30.0f };

	while (RemainingLength > 0.1f) // Tolerance
	{
		bool bPlaced = false;

		// Try to fit standard modules
		for (float Width : StandardWidths)
		{
			if (RemainingLength >= Width)
			{
				FGeneratedModule Mod;
				Mod.ProductId = FName(*FString::Printf(TEXT("Cabinet_%d"), (int32)Width));
				Mod.Width = Width;
				Mod.Offset = CurrentOffset;
				
				Modules.Add(Mod);
				
				RemainingLength -= Width;
				CurrentOffset += Width;
				bPlaced = true;
				break; // Restart greedy search (or continue same width? V1: restart)
			}
		}

		if (!bPlaced)
		{
			// Filler for the rest
			if (RemainingLength > 0)
			{
				FGeneratedModule Filler;
				Filler.ProductId = FName("Filler");
				Filler.Width = RemainingLength;
				Filler.Offset = CurrentOffset;
				Modules.Add(Filler);
			}
			break;
		}
	}

	return Modules;
}
