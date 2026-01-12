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
 * Unit types for CAD-style numeric input.
 * All internal values are stored in centimeters.
 */
UENUM(BlueprintType)
enum class ERTLengthUnit : uint8
{
	Centimeters,
	Meters,
	Inches,
	Feet
};

/**
 * Which numeric field is currently being edited.
 */
UENUM(BlueprintType)
enum class ERTNumericField : uint8
{
	Length,
	Angle
};

/**
 * Drafting state exposed by tools for HUD visualization.
 * Contains preview line endpoints, current length/angle, and input buffer state.
 */
USTRUCT(BlueprintType)
struct RTPLANINPUT_API FRTDraftingState
{
	GENERATED_BODY()

	// Is a drafting operation in progress (e.g., drawing a line)?
	UPROPERTY(BlueprintReadOnly)
	bool bIsActive = false;

	// Start point of the current line segment (world space, Z=0 for 2D)
	UPROPERTY(BlueprintReadOnly)
	FVector2D StartPoint = FVector2D::ZeroVector;

	// Current end point (snapped cursor position)
	UPROPERTY(BlueprintReadOnly)
	FVector2D EndPoint = FVector2D::ZeroVector;

	// Current length in centimeters
	UPROPERTY(BlueprintReadOnly)
	float LengthCm = 0.0f;

	// Current angle in degrees (0 = +X axis, counter-clockwise)
	UPROPERTY(BlueprintReadOnly)
	float AngleDegrees = 0.0f;

	// Is ortho snap currently active?
	UPROPERTY(BlueprintReadOnly)
	bool bOrthoSnapped = false;
};

/**
 * Numeric input buffer for CAD-style keyboard entry.
 * Collects keystrokes, parses to float, converts units to internal cm.
 */
USTRUCT(BlueprintType)
struct RTPLANINPUT_API FRTNumericInputBuffer
{
	GENERATED_BODY()

	// Current text being typed
	UPROPERTY(BlueprintReadOnly)
	FString Buffer;

	// Which field is being edited
	UPROPERTY(BlueprintReadWrite)
	ERTNumericField ActiveField = ERTNumericField::Length;

	// Current unit for length input
	UPROPERTY(BlueprintReadWrite)
	ERTLengthUnit CurrentUnit = ERTLengthUnit::Centimeters;

	// Is the buffer currently active (user is typing)?
	UPROPERTY(BlueprintReadOnly)
	bool bIsActive = false;

	// Append a character (0-9, '.')
	void AppendChar(TCHAR Char)
	{
		if (FChar::IsDigit(Char) || (Char == TEXT('.') && !Buffer.Contains(TEXT("."))))
		{
			Buffer.AppendChar(Char);
			bIsActive = true;
		}
	}

	// Remove last character
	void Backspace()
	{
		if (Buffer.Len() > 0)
		{
			Buffer.RemoveAt(Buffer.Len() - 1);
		}
		bIsActive = Buffer.Len() > 0;
	}

	// Clear the buffer
	void Clear()
	{
		Buffer.Empty();
		bIsActive = false;
	}

	// Parse and convert to centimeters (for Length) or degrees (for Angle)
	float GetValueInCm() const
	{
		if (Buffer.IsEmpty()) return 0.0f;
		
		float Value = FCString::Atof(*Buffer);
		
		if (ActiveField == ERTNumericField::Angle)
		{
			return Value; // Angles don't need unit conversion
		}

		// Convert to centimeters
		switch (CurrentUnit)
		{
		case ERTLengthUnit::Meters:
			return Value * 100.0f;
		case ERTLengthUnit::Inches:
			return Value * 2.54f;
		case ERTLengthUnit::Feet:
			return Value * 30.48f;
		case ERTLengthUnit::Centimeters:
		default:
			return Value;
		}
	}

	// Get display value (in current unit)
	float GetDisplayValue(float ValueCm) const
	{
		switch (CurrentUnit)
		{
		case ERTLengthUnit::Meters:
			return ValueCm / 100.0f;
		case ERTLengthUnit::Inches:
			return ValueCm / 2.54f;
		case ERTLengthUnit::Feet:
			return ValueCm / 30.48f;
		case ERTLengthUnit::Centimeters:
		default:
			return ValueCm;
		}
	}

	// Get unit suffix string
	FString GetUnitSuffix() const
	{
		switch (CurrentUnit)
		{
		case ERTLengthUnit::Meters:
			return TEXT("m");
		case ERTLengthUnit::Inches:
			return TEXT("in");
		case ERTLengthUnit::Feet:
			return TEXT("ft");
		case ERTLengthUnit::Centimeters:
		default:
			return TEXT("cm");
		}
	}

	// Cycle to next unit
	void CycleUnit()
	{
		CurrentUnit = static_cast<ERTLengthUnit>((static_cast<uint8>(CurrentUnit) + 1) % 4);
	}
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

	// Whether snapping is enabled (controlled by modifier key)
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bSnapEnabled = true;

	// Helper to get a point along the ray
	FVector GetPointAtDistance(float Distance) const
	{
		return WorldOrigin + (WorldDirection * Distance);
	}
};
