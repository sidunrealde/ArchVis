#include "RTPlanGeometryUtils.h"

float FRTPlanGeometryUtils::DistancePointToSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B)
{
	FVector2D Closest = ClosestPointOnSegment(P, A, B);
	return FVector2D::Distance(P, Closest);
}

FVector2D FRTPlanGeometryUtils::ProjectPointToSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B)
{
	FVector2D AB = B - A;
	float LengthSquared = AB.SizeSquared();
	
	if (FMath::IsNearlyZero(LengthSquared))
	{
		return A;
	}

	FVector2D AP = P - A;
	float T = (AP | AB) / LengthSquared; // Dot product

	return A + AB * T;
}

FVector2D FRTPlanGeometryUtils::ClosestPointOnSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B)
{
	FVector2D AB = B - A;
	float LengthSquared = AB.SizeSquared();

	if (FMath::IsNearlyZero(LengthSquared))
	{
		return A;
	}

	FVector2D AP = P - A;
	float T = (AP | AB) / LengthSquared;

	// Clamp T to segment [0, 1]
	T = FMath::Clamp(T, 0.0f, 1.0f);

	return A + AB * T;
}

void FRTPlanGeometryUtils::GetWallNormals(const FVector2D& A, const FVector2D& B, FVector2D& OutLeft, FVector2D& OutRight)
{
	FVector2D Direction = (B - A).GetSafeNormal();
	
	// Rotate 90 degrees CCW (x, y) -> (-y, x)
	OutLeft = FVector2D(-Direction.Y, Direction.X);
	
	// Rotate 90 degrees CW (x, y) -> (y, -x)
	OutRight = FVector2D(Direction.Y, -Direction.X);
}

bool FRTPlanGeometryUtils::SegmentIntersection(const FVector2D& A1, const FVector2D& B1, const FVector2D& A2, const FVector2D& B2, FVector2D& OutIntersection)
{
	// Standard line-line intersection
	float Det = (B1.X - A1.X) * (B2.Y - A2.Y) - (B2.X - A2.X) * (B1.Y - A1.Y);

	if (FMath::IsNearlyZero(Det))
	{
		return false; // Parallel
	}

	float Lambda = ((B2.Y - A2.Y) * (B2.X - A1.X) + (A2.X - B2.X) * (B2.Y - A1.Y)) / Det;
	float Gamma = ((A1.Y - B1.Y) * (B2.X - A1.X) + (B1.X - A1.X) * (B2.Y - A1.Y)) / Det;

	// Use inclusive check with epsilon to handle T-junctions (endpoints touching)
	const float Epsilon = KINDA_SMALL_NUMBER;
	
	if ((Lambda >= -Epsilon && Lambda <= 1.0f + Epsilon) && (Gamma >= -Epsilon && Gamma <= 1.0f + Epsilon))
	{
		// Snap undershoots to endpoints to prevent microscopic ghost segments.
		// We do NOT snap overshoots (Lambda < 0 or Lambda > 1) because those are needed for proper extension.
		
		if (Lambda > 0.0f && Lambda < Epsilon)
		{
			Lambda = 0.0f;
		}
		else if (Lambda < 1.0f && Lambda > 1.0f - Epsilon)
		{
			Lambda = 1.0f;
		}

		OutIntersection = A1 + Lambda * (B1 - A1);
		return true;
	}

	return false;
}

bool FRTPlanGeometryUtils::SegmentsIntersect(const FVector2D& A1, const FVector2D& B1, const FVector2D& A2, const FVector2D& B2)
{
	FVector2D Unused;
	return SegmentIntersection(A1, B1, A2, B2, Unused);
}

FVector2D FRTPlanGeometryUtils::GetPointFromPolar(const FVector2D& Center, float Radius, float AngleDegrees)
{
	float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
	return Center + FVector2D(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians)) * Radius;
}

float FRTPlanGeometryUtils::GetAngleDegrees(const FVector2D& A, const FVector2D& B)
{
	FVector2D Direction = B - A;
	float AngleRadians = FMath::Atan2(Direction.Y, Direction.X);
	return FMath::RadiansToDegrees(AngleRadians);
}

void FRTPlanGeometryUtils::GenerateArcPoints(const FVector2D& Center, float Radius, float StartAngleDegrees, float SweepAngleDegrees, int32 NumSegments, TArray<FVector2D>& OutPoints)
{
	if (NumSegments <= 0)
	{
		return;
	}

	float Step = SweepAngleDegrees / (float)NumSegments;
	for (int32 i = 0; i <= NumSegments; ++i)
	{
		float Angle = StartAngleDegrees + Step * i;
		OutPoints.Add(GetPointFromPolar(Center, Radius, Angle));
	}
}
