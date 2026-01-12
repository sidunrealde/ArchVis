#pragma once

#include "CoreMinimal.h"
#include "RTPlanInputData.generated.h"

UENUM(BlueprintType)
enum class ERTPointerAction : uint8
{
	None,
	Down,
	Up,
	Move,
	Cancel,
	Confirm
};

UENUM(BlueprintType)
enum class ERTInputSource : uint8
{
	Mouse,
	Touch,
	VRLeft,
	VRRight
};

/**
 * Unified pointer event for Desktop and VR.
 * Represents a ray in world space and the state of the "click".
 */
USTRUCT(BlueprintType)
struct RTPLANINPUT_API FRTPointerEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ERTInputSource Source = ERTInputSource::Mouse;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ERTPointerAction Action = ERTPointerAction::None;

	// World Space Ray Origin
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector WorldOrigin = FVector::ZeroVector;

	// World Space Ray Direction
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector WorldDirection = FVector::ForwardVector;

	// For desktop, the screen position (optional, for UI hit testing)
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	// Helper to get a point along the ray
	FVector GetPointAtDistance(float Distance) const
	{
		return WorldOrigin + (WorldDirection * Distance);
	}
};
