#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "ArchVisInputConfig.generated.h"

/**
 * Data Asset to hold Input Actions for the ArchVis project.
 */
UCLASS()
class ARCHVIS_API UArchVisInputConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mapping")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// --- Mouse/View Actions ---
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mouse")
	TObjectPtr<UInputAction> IA_LeftClick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mouse")
	TObjectPtr<UInputAction> IA_RightClick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mouse")
	TObjectPtr<UInputAction> IA_MouseMove;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mouse")
	TObjectPtr<UInputAction> IA_MouseWheel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "View")
	TObjectPtr<UInputAction> IA_ToggleView;

	// --- Numeric Entry Actions ---
	// Digit keys (0-9). Create one IA with Digital(bool) value type.
	// In IMC, map keys 0-9 with Scalar modifiers outputting 0.0-9.0.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NumericEntry")
	TObjectPtr<UInputAction> IA_NumericDigit;

	// Decimal point (.) key
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NumericEntry")
	TObjectPtr<UInputAction> IA_NumericDecimal;

	// Backspace key - removes last character
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NumericEntry")
	TObjectPtr<UInputAction> IA_NumericBackspace;

	// Enter key - commits the numeric value to the active tool
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NumericEntry")
	TObjectPtr<UInputAction> IA_NumericCommit;

	// Escape key - clears the input buffer and cancels
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NumericEntry")
	TObjectPtr<UInputAction> IA_NumericClear;

	// Tab key - switches between Length and Angle fields
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NumericEntry")
	TObjectPtr<UInputAction> IA_NumericSwitchField;

	// U key (or other) - cycles through unit types (cm, m, in, ft)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "NumericEntry")
	TObjectPtr<UInputAction> IA_CycleUnits;

	// --- Snap Modifier ---
	// Shift key (or other) - toggles/holds snap on/off
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SnapModifier")
	TObjectPtr<UInputAction> IA_SnapModifier;
};
