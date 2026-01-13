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
 * Arithmetic operators for CAD-style numeric expressions.
 */
UENUM(BlueprintType)
enum class ERTArithmeticOp : uint8
{
	None,
	Add,
	Subtract,
	Multiply,
	Divide
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
 * Numeric input buffer for CAD-style keyboard entry with arithmetic support.
 * Collects keystrokes, parses expressions (e.g., "100+50*2"), converts units to internal cm.
 * 
 * Supports simple arithmetic: +, -, *, /
 * Example inputs: "150", "100+50", "200/2", "3*12+6"
 */
USTRUCT(BlueprintType)
struct RTPLANINPUT_API FRTNumericInputBuffer
{
	GENERATED_BODY()

	// Current text being typed (may contain arithmetic operators)
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

	// Saved value for the non-active field (preserves value when switching)
	UPROPERTY(BlueprintReadOnly)
	float SavedLengthCm = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float SavedAngleDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bHasSavedLength = false;

	UPROPERTY(BlueprintReadOnly)
	bool bHasSavedAngle = false;

	// Append a character (0-9, '.', or arithmetic operators)
	void AppendChar(TCHAR Char)
	{
		// Allow digits
		if (FChar::IsDigit(Char))
		{
			Buffer.AppendChar(Char);
			bIsActive = true;
			return;
		}
		
		// Allow single decimal point per number segment
		if (Char == TEXT('.'))
		{
			// Find the last operator position
			int32 LastOpIdx = FMath::Max3(
				Buffer.Find(TEXT("+"), ESearchCase::IgnoreCase, ESearchDir::FromEnd),
				Buffer.Find(TEXT("-"), ESearchCase::IgnoreCase, ESearchDir::FromEnd),
				FMath::Max(
					Buffer.Find(TEXT("*"), ESearchCase::IgnoreCase, ESearchDir::FromEnd),
					Buffer.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd)
				)
			);
			
			// Check if there's already a decimal in the current number segment
			FString CurrentSegment = (LastOpIdx >= 0) ? Buffer.RightChop(LastOpIdx + 1) : Buffer;
			if (!CurrentSegment.Contains(TEXT(".")))
			{
				Buffer.AppendChar(Char);
				bIsActive = true;
			}
			return;
		}
	}

	// Append an arithmetic operator
	void AppendOperator(ERTArithmeticOp Op)
	{
		if (Buffer.IsEmpty()) return;
		
		// Don't allow operator at start or consecutive operators
		TCHAR LastChar = Buffer[Buffer.Len() - 1];
		if (LastChar == TEXT('+') || LastChar == TEXT('-') || 
			LastChar == TEXT('*') || LastChar == TEXT('/'))
		{
			// Replace the last operator
			Buffer.RemoveAt(Buffer.Len() - 1);
		}
		
		switch (Op)
		{
		case ERTArithmeticOp::Add:
			Buffer.AppendChar(TEXT('+'));
			break;
		case ERTArithmeticOp::Subtract:
			Buffer.AppendChar(TEXT('-'));
			break;
		case ERTArithmeticOp::Multiply:
			Buffer.AppendChar(TEXT('*'));
			break;
		case ERTArithmeticOp::Divide:
			Buffer.AppendChar(TEXT('/'));
			break;
		default:
			break;
		}
		bIsActive = true;
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

	// Clear all saved values and buffer
	void ClearAll()
	{
		Buffer.Empty();
		bIsActive = false;
		SavedLengthCm = 0.0f;
		SavedAngleDegrees = 0.0f;
		bHasSavedLength = false;
		bHasSavedAngle = false;
	}

	// Save current value before switching fields
	void SaveCurrentFieldValue()
	{
		if (!Buffer.IsEmpty())
		{
			float Value = EvaluateExpression();
			if (ActiveField == ERTNumericField::Length)
			{
				SavedLengthCm = ConvertToInternalUnits(Value);
				bHasSavedLength = true;
			}
			else
			{
				SavedAngleDegrees = Value;
				bHasSavedAngle = true;
			}
		}
	}

	// Evaluate the arithmetic expression in the buffer
	float EvaluateExpression() const
	{
		if (Buffer.IsEmpty()) return 0.0f;

		// Simple expression parser with operator precedence
		// Supports: +, -, *, /
		TArray<float> Numbers;
		TArray<TCHAR> Operators;
		
		FString CurrentNumber;
		bool bNegativeStart = false;
		
		for (int32 i = 0; i < Buffer.Len(); ++i)
		{
			TCHAR C = Buffer[i];
			
			// Handle negative numbers at start or after operator
			if (C == TEXT('-') && (i == 0 || (i > 0 && IsOperator(Buffer[i-1]))))
			{
				CurrentNumber.AppendChar(C);
				continue;
			}
			
			if (FChar::IsDigit(C) || C == TEXT('.'))
			{
				CurrentNumber.AppendChar(C);
			}
			else if (IsOperator(C))
			{
				if (!CurrentNumber.IsEmpty())
				{
					Numbers.Add(FCString::Atof(*CurrentNumber));
					CurrentNumber.Empty();
				}
				Operators.Add(C);
			}
		}
		
		// Add the last number
		if (!CurrentNumber.IsEmpty())
		{
			Numbers.Add(FCString::Atof(*CurrentNumber));
		}
		
		if (Numbers.Num() == 0) return 0.0f;
		if (Numbers.Num() == 1) return Numbers[0];
		
		// First pass: handle * and /
		TArray<float> IntermediateNumbers;
		TArray<TCHAR> IntermediateOperators;
		
		IntermediateNumbers.Add(Numbers[0]);
		
		for (int32 i = 0; i < Operators.Num(); ++i)
		{
			if (i + 1 >= Numbers.Num()) break;
			
			if (Operators[i] == TEXT('*'))
			{
				float Last = IntermediateNumbers.Pop();
				IntermediateNumbers.Add(Last * Numbers[i + 1]);
			}
			else if (Operators[i] == TEXT('/'))
			{
				float Last = IntermediateNumbers.Pop();
				float Divisor = Numbers[i + 1];
				IntermediateNumbers.Add(FMath::IsNearlyZero(Divisor) ? 0.0f : Last / Divisor);
			}
			else
			{
				IntermediateNumbers.Add(Numbers[i + 1]);
				IntermediateOperators.Add(Operators[i]);
			}
		}
		
		// Second pass: handle + and -
		float Result = IntermediateNumbers[0];
		for (int32 i = 0; i < IntermediateOperators.Num(); ++i)
		{
			if (i + 1 >= IntermediateNumbers.Num()) break;
			
			if (IntermediateOperators[i] == TEXT('+'))
			{
				Result += IntermediateNumbers[i + 1];
			}
			else if (IntermediateOperators[i] == TEXT('-'))
			{
				Result -= IntermediateNumbers[i + 1];
			}
		}
		
		return Result;
	}

	// Helper to check if character is an operator
	static bool IsOperator(TCHAR C)
	{
		return C == TEXT('+') || C == TEXT('-') || C == TEXT('*') || C == TEXT('/');
	}

	// Convert value to internal units (cm for length, degrees for angle)
	float ConvertToInternalUnits(float Value) const
	{
		if (ActiveField == ERTNumericField::Angle)
		{
			return Value; // Angles are already in degrees
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

	// Parse expression and convert to centimeters (for Length) or degrees (for Angle)
	float GetValueInCm() const
	{
		float Value = EvaluateExpression();
		return ConvertToInternalUnits(Value);
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

	// Get display string with evaluated result (for showing "100+50 = 150")
	FString GetDisplayString() const
	{
		if (Buffer.IsEmpty()) return TEXT("");
		
		// If expression contains operators, show result
		bool bHasOperators = Buffer.Contains(TEXT("+")) || Buffer.Contains(TEXT("-")) ||
							 Buffer.Contains(TEXT("*")) || Buffer.Contains(TEXT("/"));
		
		if (bHasOperators)
		{
			float Result = EvaluateExpression();
			return FString::Printf(TEXT("%s = %.2f"), *Buffer, Result);
		}
		
		return Buffer;
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

	// --- Modifier Key States (for selection) ---
	// Shift is held - add to selection
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bShiftDown = false;

	// Alt is held - remove from selection
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bAltDown = false;

	// Ctrl is held - reserved for future use
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCtrlDown = false;

	// Helper to get a point along the ray
	FVector GetPointAtDistance(float Distance) const
	{
		return WorldOrigin + (WorldDirection * Distance);
	}
};
