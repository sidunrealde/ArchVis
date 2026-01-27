// ToolInputComponent.h
// Input component for tool interactions - attached to PlayerController
// Handles drawing, selection, and tool-specific inputs that are pawn-agnostic

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "ToolInputComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class UArchVisInputConfig;
class UEnhancedInputLocalPlayerSubsystem;
class AArchVisPlayerController;

/**
 * Input component for tool interactions.
 * Attached to PlayerController (not pawns) because tool input is consistent
 * regardless of which pawn is active.
 * 
 * Handles:
 * - Selection (click, box select)
 * - Drawing (place point, confirm, cancel, close, remove last)
 * - Numeric entry (digits, operators, commit)
 * - Tool switching hotkeys
 */
UCLASS(ClassGroup=(ArchVis), meta=(BlueprintSpawnableComponent))
class ARCHVIS_API UToolInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UToolInputComponent();

	// Initialize with the input config
	void Initialize(UArchVisInputConfig* InInputConfig, class UEnhancedInputComponent* InputComponent);

	// Cleanup bindings
	void Cleanup();

	// Update tool-specific IMCs based on current tool
	void SetActiveToolContext(UInputMappingContext* ToolIMC);

	// Enable/disable debug logging
	void SetDebugEnabled(bool bEnabled) { bDebugEnabled = bEnabled; }
	bool IsDebugEnabled() const { return bDebugEnabled; }

protected:
	virtual void BeginPlay() override;

	// --- Selection Handlers ---
	void OnSelect(const FInputActionValue& Value);
	void OnSelectCompleted(const FInputActionValue& Value);
	void OnBoxSelectStart(const FInputActionValue& Value);
	void OnBoxSelectDrag(const FInputActionValue& Value);
	void OnBoxSelectEnd(const FInputActionValue& Value);

	// --- Drawing Handlers ---
	void OnDrawPlacePoint(const FInputActionValue& Value);
	void OnDrawConfirm(const FInputActionValue& Value);
	void OnDrawCancel(const FInputActionValue& Value);
	void OnDrawClose(const FInputActionValue& Value);
	void OnDrawRemoveLastPoint(const FInputActionValue& Value);
	void OnOrthoLock(const FInputActionValue& Value);
	void OnOrthoLockReleased(const FInputActionValue& Value);
	void OnAngleSnap(const FInputActionValue& Value);

	// --- Tool Switching ---
	void OnToolSelect(const FInputActionValue& Value);
	void OnToolLine(const FInputActionValue& Value);
	void OnToolPolyline(const FInputActionValue& Value);
	void OnToolArc(const FInputActionValue& Value);

	// Helper to get the owning player controller
	AArchVisPlayerController* GetArchVisController() const;

	// Helper to get input subsystem
	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const;

	// Add/Remove mapping context helpers
	void AddMappingContext(UInputMappingContext* Context, int32 Priority);
	void RemoveMappingContext(UInputMappingContext* Context);

private:
	// The input config data asset
	UPROPERTY(Transient)
	TObjectPtr<UArchVisInputConfig> InputConfig;

	// Currently active tool IMC
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ActiveToolIMC;

	// Cached input component
	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> CachedInputComponent;

	// Priority constants
	static constexpr int32 IMC_Priority_Tool = 2;

	bool bDebugEnabled = false;
	bool bIsInitialized = false;
};
