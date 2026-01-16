// DraftingInputComponent.h
// Input component for 2D Drafting pawn - handles pan, zoom, and drawing inputs

#pragma once

#include "CoreMinimal.h"
#include "Input/ArchVisInputComponent.h"
#include "DraftingInputComponent.generated.h"

class AArchVisDraftingPawn;

/**
 * Input component for the 2D Drafting Pawn.
 * Handles AutoCAD-style navigation:
 * - IA_Pan: Pan camera (default: MMB)
 * - IA_Zoom: Zoom in/out (default: Scroll)
 * - Mouse movement updates virtual cursor
 */
UCLASS(ClassGroup=(ArchVis), meta=(BlueprintSpawnableComponent))
class ARCHVIS_API UDraftingInputComponent : public UArchVisInputComponent
{
	GENERATED_BODY()

public:
	UDraftingInputComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputBindings() override;
	virtual void AddInputMappingContexts() override;
	virtual void RemoveInputMappingContexts() override;

	// --- Navigation Handlers ---

	void OnPanActionStarted(const FInputActionValue& Value);
	void OnPanActionCompleted(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);

	// Get the drafting pawn owner
	AArchVisDraftingPawn* GetDraftingPawn() const;

private:
	// Action state
	bool bPanActionActive = false;
	
	// Last mouse position for delta calculation
	FVector2D LastMousePosition = FVector2D::ZeroVector;

	// Process pan based on mouse delta
	void ProcessPan(float DeltaTime);
};

