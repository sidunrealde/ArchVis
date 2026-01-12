#pragma once

#include "CoreMinimal.h"
#include "RTPlanSchema.h"

/**
 * Utilities for calculating wall intervals with openings.
 */
class RTPLANOPENINGS_API FRTPlanOpeningUtils
{
public:
	struct FInterval
	{
		float Start;
		float End;

		bool operator<(const FInterval& Other) const { return Start < Other.Start; }
	};

	/**
	 * Computes the solid intervals of a wall by subtracting openings.
	 * @param WallLength Total length of the wall.
	 * @param Openings List of openings on this wall.
	 * @return Sorted list of solid intervals (Start, End).
	 */
	static TArray<FInterval> ComputeSolidIntervals(float WallLength, const TArray<FRTOpening>& Openings);
};
