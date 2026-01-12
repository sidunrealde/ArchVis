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

	if ((0 < Lambda && Lambda < 1) && (0 < Gamma && Gamma < 1))
	{
		OutIntersection = A1 + Lambda * (B1 - A1);
		return true;
	}

	return false;
}
