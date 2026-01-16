// ArchVisInputComponent.h
// Base input component for all ArchVis pawn input handling

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "ArchVisInputComponent.generated.h"

class UInputMappingContext;
class UInputAction;
class UArchVisInputConfig;
class AArchVisPawnBase;
class UEnhancedInputLocalPlayerSubsystem;

/**
 * Base class for pawn-specific input components.
 * Each pawn type has its own derived input component that handles
 * navigation and interaction specific to that pawn's mode.
 * 
 * This replaces the monolithic input handling in AArchVisPlayerController
 * with a modular, pawn-centric approach.
 */
UCLASS(Abstract, ClassGroup=(ArchVis), meta=(BlueprintSpawnableComponent))
class ARCHVIS_API UArchVisInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	UArchVisInputComponent();

	// Initialize with the input config (call from pawn's BeginPlay)
	virtual void Initialize(UArchVisInputConfig* InInputConfig);

	// Cleanup when pawn is unpossessed
	virtual void Cleanup();

	// Called when this pawn is possessed by a player controller
	virtual void OnPawnPossessed(AController* NewController);

	// Called when this pawn is unpossessed
	virtual void OnPawnUnpossessed(AController* OldController);

	// Enable/disable debug logging
	void SetDebugEnabled(bool bEnabled) { bDebugEnabled = bEnabled; }
	bool IsDebugEnabled() const { return bDebugEnabled; }

protected:
	// Override in derived classes to bind pawn-specific input actions
	virtual void SetupInputBindings() PURE_VIRTUAL(UArchVisInputComponent::SetupInputBindings, );

	// Override in derived classes to add pawn-specific IMCs
	virtual void AddInputMappingContexts() PURE_VIRTUAL(UArchVisInputComponent::AddInputMappingContexts, );

	// Override in derived classes to remove pawn-specific IMCs
	virtual void RemoveInputMappingContexts() PURE_VIRTUAL(UArchVisInputComponent::RemoveInputMappingContexts, );

	// Helper to get the Enhanced Input subsystem
	UEnhancedInputLocalPlayerSubsystem* GetInputSubsystem() const;

	// Helper to add an IMC with priority
	void AddMappingContext(UInputMappingContext* Context, int32 InPriority);

	// Helper to remove an IMC
	void RemoveMappingContext(UInputMappingContext* Context);

	// Get the owning pawn (cast helper)
	template<typename T>
	T* GetOwningPawn() const
	{
		return Cast<T>(GetOwner());
	}

	// Get the owning player controller
	APlayerController* GetPlayerController() const;

	// --- IMC Priority Constants ---
	static constexpr int32 IMC_Priority_Global = 0;
	static constexpr int32 IMC_Priority_ModeBase = 1;
	static constexpr int32 IMC_Priority_Tool = 2;
	static constexpr int32 IMC_Priority_NumericEntry = 3;

	// The input config data asset
	UPROPERTY(Transient)
	TObjectPtr<UArchVisInputConfig> InputConfig;

	// Cached player controller
	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> CachedPlayerController;

	// Debug flag
	bool bDebugEnabled = false;

	// Track if we're initialized
	bool bIsInitialized = false;
};

