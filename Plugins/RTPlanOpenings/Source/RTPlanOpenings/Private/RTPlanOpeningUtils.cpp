#include "RTPlanOpeningUtils.h"

TArray<FRTPlanOpeningUtils::FInterval> FRTPlanOpeningUtils::ComputeSolidIntervals(float WallLength, const TArray<FRTOpening>& Openings)
{
	TArray<FInterval> Holes;
	
	// 1. Collect Holes
	for (const FRTOpening& Op : Openings)
	{
		float Start = Op.OffsetCm;
		float End = Op.OffsetCm + Op.WidthCm;

		// Clamp to wall
		Start = FMath::Max(0.0f, Start);
		End = FMath::Min(WallLength, End);

		if (End > Start)
		{
			Holes.Add({ Start, End });
		}
	}

	// 2. Sort Holes
	Holes.Sort();

	// 3. Merge Overlapping Holes
	TArray<FInterval> MergedHoles;
	if (Holes.Num() > 0)
	{
		FInterval Current = Holes[0];
		for (int32 i = 1; i < Holes.Num(); ++i)
		{
			if (Holes[i].Start < Current.End) // Overlap or Abut
			{
				Current.End = FMath::Max(Current.End, Holes[i].End);
			}
			else
			{
				MergedHoles.Add(Current);
				Current = Holes[i];
			}
		}
		MergedHoles.Add(Current);
	}

	// 4. Compute Solids (Complement)
	TArray<FInterval> Solids;
	float CurrentPos = 0.0f;

	for (const FInterval& Hole : MergedHoles)
	{
		if (Hole.Start > CurrentPos)
		{
			Solids.Add({ CurrentPos, Hole.Start });
		}
		CurrentPos = FMath::Max(CurrentPos, Hole.End);
	}

	// Final segment
	if (CurrentPos < WallLength)
	{
		Solids.Add({ CurrentPos, WallLength });
	}

	return Solids;
}
