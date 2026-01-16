// OrbitInputComponent.h
// Input component for 3D Orbit pawn - handles all 3D navigation

#pragma once

#include "CoreMinimal.h"
#include "Input/ArchVisInputComponent.h"
#include "OrbitInputComponent.generated.h"

class AArchVisOrbitPawn;

/**
 * Input component for the 3D Orbit Pawn.
 * Handles Unreal Editor-style navigation:
 * - IA_FlyMode + Mouse: Look around (default: RMB)
 * - Alt + IA_Select + Mouse: Orbit around target (default: Alt+LMB)
 * - IA_Pan + Mouse: Pan camera (default: MMB)
 * - Alt + IA_FlyMode + Mouse Y: Dolly zoom (default: Alt+RMB)
 * - IA_Zoom: Zoom (default: Scroll)
 * - IA_FlyMode + IA_Move: Fly movement (default: RMB+WASD)
 * - IA_FlyMode + IA_MoveUp/Down: Move up/down (default: RMB+Q/E)
 */
UCLASS(ClassGroup=(ArchVis), meta=(BlueprintSpawnableComponent))
class ARCHVIS_API UOrbitInputComponent : public UArchVisInputComponent
{
	GENERATED_BODY()

public:
	UOrbitInputComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputBindings() override;
	virtual void AddInputMappingContexts() override;
	virtual void RemoveInputMappingContexts() override;

	// --- Input Action Handlers ---
	
	// Orbit action (Alt + LMB via chorded action)
	void OnOrbitActionStarted(const FInputActionValue& Value);
	void OnOrbitActionCompleted(const FInputActionValue& Value);

	// Selection action (for other selection purposes)
	void OnSelectActionStarted(const FInputActionValue& Value);
	void OnSelectActionCompleted(const FInputActionValue& Value);

	// Fly mode action (also dolly with Alt)
	void OnFlyModeActionStarted(const FInputActionValue& Value);
	void OnFlyModeActionCompleted(const FInputActionValue& Value);

	// Pan action
	void OnPanActionStarted(const FInputActionValue& Value);
	void OnPanActionCompleted(const FInputActionValue& Value);
	
	// Modifier keys
	void OnAltModifierStarted(const FInputActionValue& Value);
	void OnAltModifierCompleted(const FInputActionValue& Value);
	void OnShiftModifierStarted(const FInputActionValue& Value);
	void OnShiftModifierCompleted(const FInputActionValue& Value);

	// Zoom
	void OnZoom(const FInputActionValue& Value);

	// WASD Movement (during fly mode)
	void OnMove(const FInputActionValue& Value);
	void OnMoveUp(const FInputActionValue& Value);
	void OnMoveDown(const FInputActionValue& Value);

	// Get the orbit pawn owner
	AArchVisOrbitPawn* GetOrbitPawn() const;

private:
	// --- Navigation State ---
	
	// Current navigation mode (computed from action/modifier state)
	enum class ENavMode : uint8
	{
		None,
		Pan,        // Pan action
		Orbit,      // Alt + Select action
		Look,       // Fly mode action
		Dolly       // Alt + Fly mode action
	};

	ENavMode GetCurrentNavMode() const;
	void UpdateMouseLockState();

	// Validate action states against actual key states to prevent stuck states
	void ValidateActionStates();

	// Action states (triggered by Enhanced Input actions)
	bool bOrbitActionActive = false;    // IA_Orbit (Alt + LMB chorded)
	bool bSelectActionActive = false;   // IA_Select
	bool bFlyModeActive = false;        // IA_FlyMode
	bool bPanActionActive = false;      // IA_Pan
	
	// Modifier states
	bool bAltDown = false;
	bool bShiftDown = false;

	// Movement input (accumulated from WASD)
	FVector2D MoveInput = FVector2D::ZeroVector;
	float VerticalInput = 0.0f;

	// Mouse tracking
	FVector2D LastMousePosition = FVector2D::ZeroVector;
	bool bMouseLocked = false;

	// Process raw mouse movement and route to appropriate navigation function
	void ProcessMouseNavigation(float DeltaTime);
};

