#pragma once

#include "CoreMinimal.h"
#include "RTPlanSchema.h"
#include "RTPlanCatalogTypes.h"

/**
 * Solver for procedural cabinet runs.
 * Fills a linear space with modules based on rules.
 */
class RTPLANRUNS_API FRTPlanRunSolver
{
public:
	struct FRunInput
	{
		float TotalLength;
		float Depth;
		float Height;
		// In a real app, we'd pass a Style/Rule set here
	};

	struct FGeneratedModule
	{
		FName ProductId;
		float Width;
		float Offset; // From start
	};

	/**
	 * Solves the run layout.
	 * V1 Strategy: Greedy fill with standard widths (60, 45, 30) + Filler.
	 */
	static TArray<FGeneratedModule> Solve(const FRunInput& Input);
};
