// OrbitInputComponent.cpp
// Input component for 3D Orbit pawn - Unreal Editor-style navigation

#include "Input/OrbitInputComponent.h"
#include "ArchVisOrbitPawn.h"
#include "ArchVisInputConfig.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogOrbitInput, Log, All);

UOrbitInputComponent::UOrbitInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UOrbitInputComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize mouse position
	if (APlayerController* PC = GetPlayerController())
	{
		float MouseX, MouseY;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			LastMousePosition = FVector2D(MouseX, MouseY);
		}
	}
}

void UOrbitInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Process mouse-based navigation
	ProcessMouseNavigation(DeltaTime);

	// Validate movement input against actual key state
	// This prevents stuck movement when Enhanced Input events don't fire correctly
	if (APlayerController* PC = GetPlayerController())
	{
		// Check if WASD keys are actually pressed
		bool bWPressed = PC->IsInputKeyDown(EKeys::W);
		bool bSPressed = PC->IsInputKeyDown(EKeys::S);
		bool bAPressed = PC->IsInputKeyDown(EKeys::A);
		bool bDPressed = PC->IsInputKeyDown(EKeys::D);
		bool bQPressed = PC->IsInputKeyDown(EKeys::Q);
		bool bEPressed = PC->IsInputKeyDown(EKeys::E);
		
		// If no movement keys are pressed, zero out input
		if (!bWPressed && !bSPressed && !bAPressed && !bDPressed)
		{
			MoveInput = FVector2D::ZeroVector;
		}
		if (!bQPressed && !bEPressed)
		{
			VerticalInput = 0.0f;
		}
	}

	// Process WASD movement during fly mode (RMB held)
	ENavMode NavMode = GetCurrentNavMode();
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		if (NavMode == ENavMode::Look)
		{
			// In fly mode - apply movement input
			Pawn->SetMovementInput(MoveInput);
			Pawn->SetVerticalInput(VerticalInput);
		}
		else
		{
			// Not in fly mode - clear movement input
			Pawn->SetMovementInput(FVector2D::ZeroVector);
			Pawn->SetVerticalInput(0.0f);
		}
	}
}

void UOrbitInputComponent::SetupInputBindings()
{
	if (!InputConfig)
	{
		UE_LOG(LogOrbitInput, Error, TEXT("SetupInputBindings: InputConfig is null!"));
		return;
	}

	// --- Navigation Actions ---
	// Use separate lambdas for Started/Completed to explicitly set state
	
	// Orbit action (Alt + LMB via chorded action in IMC)
	if (InputConfig->IA_Orbit)
	{
		BindAction(InputConfig->IA_Orbit, ETriggerEvent::Started, this, &UOrbitInputComponent::OnOrbitActionStarted);
		BindAction(InputConfig->IA_Orbit, ETriggerEvent::Completed, this, &UOrbitInputComponent::OnOrbitActionCompleted);
		UE_LOG(LogOrbitInput, Log, TEXT("SetupInputBindings: IA_Orbit BOUND"));
	}
	else
	{
		UE_LOG(LogOrbitInput, Warning, TEXT("SetupInputBindings: IA_Orbit is NULL - not bound!"));
	}

	// Select action (for other selection purposes, not orbit)
	if (InputConfig->IA_Select)
	{
		BindAction(InputConfig->IA_Select, ETriggerEvent::Started, this, &UOrbitInputComponent::OnSelectActionStarted);
		BindAction(InputConfig->IA_Select, ETriggerEvent::Completed, this, &UOrbitInputComponent::OnSelectActionCompleted);
	}

	// Fly mode action (also dolly when Alt held)
	if (InputConfig->IA_FlyMode)
	{
		BindAction(InputConfig->IA_FlyMode, ETriggerEvent::Started, this, &UOrbitInputComponent::OnFlyModeActionStarted);
		BindAction(InputConfig->IA_FlyMode, ETriggerEvent::Completed, this, &UOrbitInputComponent::OnFlyModeActionCompleted);
	}

	// Pan action
	if (InputConfig->IA_Pan)
	{
		BindAction(InputConfig->IA_Pan, ETriggerEvent::Started, this, &UOrbitInputComponent::OnPanActionStarted);
		BindAction(InputConfig->IA_Pan, ETriggerEvent::Completed, this, &UOrbitInputComponent::OnPanActionCompleted);
	}

	// --- Modifiers ---
	
	if (InputConfig->IA_ModifierAlt)
	{
		BindAction(InputConfig->IA_ModifierAlt, ETriggerEvent::Started, this, &UOrbitInputComponent::OnAltModifierStarted);
		BindAction(InputConfig->IA_ModifierAlt, ETriggerEvent::Completed, this, &UOrbitInputComponent::OnAltModifierCompleted);
	}

	if (InputConfig->IA_ModifierShift)
	{
		BindAction(InputConfig->IA_ModifierShift, ETriggerEvent::Started, this, &UOrbitInputComponent::OnShiftModifierStarted);
		BindAction(InputConfig->IA_ModifierShift, ETriggerEvent::Completed, this, &UOrbitInputComponent::OnShiftModifierCompleted);
	}

	// --- Zoom ---
	
	if (InputConfig->IA_Zoom)
	{
		BindAction(InputConfig->IA_Zoom, ETriggerEvent::Triggered, this, &UOrbitInputComponent::OnZoom);
	}

	// --- Adjust Fly Speed (RMB + Scroll) ---
	
	if (InputConfig->IA_AdjustFlySpeed)
	{
		BindAction(InputConfig->IA_AdjustFlySpeed, ETriggerEvent::Triggered, this, &UOrbitInputComponent::OnAdjustFlySpeed);
	}

	// --- WASD Movement ---
	
	if (InputConfig->IA_Move)
	{
		BindAction(InputConfig->IA_Move, ETriggerEvent::Triggered, this, &UOrbitInputComponent::OnMove);
		BindAction(InputConfig->IA_Move, ETriggerEvent::Completed, this, &UOrbitInputComponent::OnMove);
	}

	if (InputConfig->IA_MoveUp)
	{
		BindAction(InputConfig->IA_MoveUp, ETriggerEvent::Triggered, this, &UOrbitInputComponent::OnMoveUp);
		BindAction(InputConfig->IA_MoveUp, ETriggerEvent::Completed, this, &UOrbitInputComponent::OnMoveUp);
	}

	if (InputConfig->IA_MoveDown)
	{
		BindAction(InputConfig->IA_MoveDown, ETriggerEvent::Triggered, this, &UOrbitInputComponent::OnMoveDown);
		BindAction(InputConfig->IA_MoveDown, ETriggerEvent::Completed, this, &UOrbitInputComponent::OnMoveDown);
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitInput, Log, TEXT("SetupInputBindings complete"));
	}
}

void UOrbitInputComponent::AddInputMappingContexts()
{
	if (!InputConfig)
	{
		return;
	}

	// Add 3D base and navigation contexts
	AddMappingContext(InputConfig->IMC_3D_Base, IMC_Priority_ModeBase);
	AddMappingContext(InputConfig->IMC_3D_Navigation, IMC_Priority_Tool);

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitInput, Log, TEXT("Added 3D Orbit IMCs"));
	}
}

void UOrbitInputComponent::RemoveInputMappingContexts()
{
	if (!InputConfig)
	{
		return;
	}

	RemoveMappingContext(InputConfig->IMC_3D_Base);
	RemoveMappingContext(InputConfig->IMC_3D_Navigation);

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitInput, Log, TEXT("Removed 3D Orbit IMCs"));
	}
}

// --- Input Handlers ---

void UOrbitInputComponent::OnOrbitActionStarted(const FInputActionValue& Value)
{
	bOrbitActionActive = true;
	
	// Reset mouse position on start
	if (APlayerController* PC = GetPlayerController())
	{
		float MouseX, MouseY;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			LastMousePosition = FVector2D(MouseX, MouseY);
		}
	}
	
	UpdateMouseLockState();

	UE_LOG(LogOrbitInput, Log, TEXT("IA_Orbit: Started - bOrbitActionActive=%d"), bOrbitActionActive);

	// Activate orbit mode on pawn
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		Pawn->SetOrbitModeActive(true);
	}
}

void UOrbitInputComponent::OnOrbitActionCompleted(const FInputActionValue& Value)
{
	bOrbitActionActive = false;
	UpdateMouseLockState();

	UE_LOG(LogOrbitInput, Log, TEXT("IA_Orbit: Completed - bOrbitActionActive=%d"), bOrbitActionActive);

	// Deactivate orbit mode on pawn
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		Pawn->SetOrbitModeActive(false);
	}
}

void UOrbitInputComponent::OnSelectActionStarted(const FInputActionValue& Value)
{
	bSelectActionActive = true;
	
	// Reset mouse position on start
	if (APlayerController* PC = GetPlayerController())
	{
		float MouseX, MouseY;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			LastMousePosition = FVector2D(MouseX, MouseY);
		}
	}

	UE_LOG(LogOrbitInput, Log, TEXT("IA_Select: Started - bSelectActionActive=%d bAltDown=%d"), bSelectActionActive, bAltDown);

	// Alt + Select = Orbit mode
	if (bAltDown)
	{
		if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
		{
			UE_LOG(LogOrbitInput, Log, TEXT("  -> Activating ORBIT mode via Alt+Select"));
			Pawn->SetOrbitModeActive(true);
		}
		UpdateMouseLockState();
	}
}

void UOrbitInputComponent::OnSelectActionCompleted(const FInputActionValue& Value)
{
	bSelectActionActive = false;

	UE_LOG(LogOrbitInput, Log, TEXT("IA_Select: Completed - bSelectActionActive=%d"), bSelectActionActive);

	// End orbit mode when Select is released
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		if (Pawn->IsOrbitModeActive())
		{
			UE_LOG(LogOrbitInput, Log, TEXT("  -> Deactivating ORBIT mode"));
			Pawn->SetOrbitModeActive(false);
		}
	}
	UpdateMouseLockState();
}

void UOrbitInputComponent::OnFlyModeActionStarted(const FInputActionValue& Value)
{
	bFlyModeActive = true;
	
	// Reset mouse position on start
	if (APlayerController* PC = GetPlayerController())
	{
		float MouseX, MouseY;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			LastMousePosition = FVector2D(MouseX, MouseY);
		}
	}
	
	UpdateMouseLockState();

	// Always log for debugging
	UE_LOG(LogOrbitInput, Log, TEXT("IA_FlyMode: Started - bFlyModeActive=%d bAltDown=%d"), bFlyModeActive, bAltDown);

	// Update pawn fly/dolly mode
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		if (bAltDown)
		{
			// Alt + FlyMode = Dolly
			Pawn->SetDollyModeActive(true);
			Pawn->SetFlyModeActive(false);
		}
		else
		{
			// FlyMode = Fly/Look
			Pawn->SetFlyModeActive(true);
			Pawn->SetDollyModeActive(false);
		}
	}
}

void UOrbitInputComponent::OnFlyModeActionCompleted(const FInputActionValue& Value)
{
	bFlyModeActive = false;
	
	// Reset movement input to prevent stuck movement when re-entering fly mode
	MoveInput = FVector2D::ZeroVector;
	VerticalInput = 0.0f;
	
	UpdateMouseLockState();

	// Always log for debugging
	UE_LOG(LogOrbitInput, Log, TEXT("IA_FlyMode: Completed - bFlyModeActive=%d, MoveInput reset"), bFlyModeActive);

	// End fly/dolly mode
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		Pawn->SetFlyModeActive(false);
		Pawn->SetDollyModeActive(false);
	}
}

void UOrbitInputComponent::OnPanActionStarted(const FInputActionValue& Value)
{
	bPanActionActive = true;
	
	// Reset mouse position on start
	if (APlayerController* PC = GetPlayerController())
	{
		float MouseX, MouseY;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			LastMousePosition = FVector2D(MouseX, MouseY);
		}
	}
	
	UpdateMouseLockState();

	// Always log for debugging
	UE_LOG(LogOrbitInput, Log, TEXT("IA_Pan: Started - bPanActionActive=%d"), bPanActionActive);
}

void UOrbitInputComponent::OnPanActionCompleted(const FInputActionValue& Value)
{
	bPanActionActive = false;
	UpdateMouseLockState();

	// Always log for debugging
	UE_LOG(LogOrbitInput, Log, TEXT("IA_Pan: Completed - bPanActionActive=%d"), bPanActionActive);
}

void UOrbitInputComponent::OnAltModifierStarted(const FInputActionValue& Value)
{
	bAltDown = true;

	UE_LOG(LogOrbitInput, Log, TEXT("IA_ModifierAlt: Started - bAltDown=%d bSelectActionActive=%d bFlyModeActive=%d"), bAltDown, bSelectActionActive, bFlyModeActive);

	// Update pawn mode when Alt is pressed while other actions are held
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		if (bFlyModeActive)
		{
			// Switch from fly to dolly
			UE_LOG(LogOrbitInput, Log, TEXT("  -> Switching to DOLLY mode"));
			Pawn->SetFlyModeActive(false);
			Pawn->SetDollyModeActive(true);
		}
		if (bSelectActionActive)
		{
			// Alt pressed while Select is held = Orbit
			UE_LOG(LogOrbitInput, Log, TEXT("  -> Activating ORBIT mode (Alt pressed while Select held)"));
			Pawn->SetOrbitModeActive(true);
		}
	}
	
	UpdateMouseLockState();
}

void UOrbitInputComponent::OnAltModifierCompleted(const FInputActionValue& Value)
{
	bAltDown = false;

	UE_LOG(LogOrbitInput, Log, TEXT("IA_ModifierAlt: Completed - bAltDown=%d bSelectActionActive=%d bFlyModeActive=%d"), bAltDown, bSelectActionActive, bFlyModeActive);

	// Update pawn mode when Alt is released while other actions are held
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		if (bFlyModeActive)
		{
			// Switch from dolly to fly
			Pawn->SetDollyModeActive(false);
			Pawn->SetFlyModeActive(true);
		}
		if (bSelectActionActive)
		{
			// Alt released while Select is still held = end orbit
			Pawn->SetOrbitModeActive(false);
		}
	}
	
	UpdateMouseLockState();
}

void UOrbitInputComponent::OnShiftModifierStarted(const FInputActionValue& Value)
{
	bShiftDown = true;

	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		Pawn->SetSpeedBoost(true);
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitInput, Log, TEXT("IA_ModifierShift: Started"));
	}
}

void UOrbitInputComponent::OnShiftModifierCompleted(const FInputActionValue& Value)
{
	bShiftDown = false;

	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		Pawn->SetSpeedBoost(false);
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitInput, Log, TEXT("IA_ModifierShift: Completed"));
	}
}

void UOrbitInputComponent::OnZoom(const FInputActionValue& Value)
{
	const float ZoomAmount = Value.Get<float>();
	
	if (FMath::IsNearlyZero(ZoomAmount))
	{
		return;
	}

	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		Pawn->Zoom(ZoomAmount);

		if (bDebugEnabled)
		{
			UE_LOG(LogOrbitInput, Log, TEXT("IA_Zoom: %f"), ZoomAmount);
		}
	}
}

void UOrbitInputComponent::OnAdjustFlySpeed(const FInputActionValue& Value)
{
	const float ScrollAmount = Value.Get<float>();
	
	if (FMath::IsNearlyZero(ScrollAmount))
	{
		return;
	}

	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		Pawn->AdjustFlySpeed(ScrollAmount);

		if (bDebugEnabled)
		{
			UE_LOG(LogOrbitInput, Log, TEXT("IA_AdjustFlySpeed: %f -> FlySpeed=%f"), ScrollAmount, Pawn->GetFlySpeed());
		}
	}
}

void UOrbitInputComponent::OnMove(const FInputActionValue& Value)
{
	// IA_Move is configured as an Axis2D (Vector2D) action.
	// Expected convention:
	//   MoveInput.X = Right/Left (D=+, A=-)
	//   MoveInput.Y = Forward/Back (W=+, S=-)
	//
	// Input Action provides:
	//   RawInput.X = A/D (left/right)
	//   RawInput.Y = W/S (forward/back)
	const FVector2D RawInput = Value.Get<FVector2D>();
	
	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitInput, Log, TEXT("OnMove: RawInput=%s"), *RawInput.ToString());
	}
	
	// Store the input directly - Enhanced Input handles accumulation
	// When chord is broken or all keys released, we'll get zero
	MoveInput = RawInput;
}

void UOrbitInputComponent::OnMoveUp(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	// Zero out on Completed (when chord is broken or key released)
	VerticalInput = FMath::IsNearlyZero(InputValue, 0.01f) ? 0.0f : InputValue;
}

void UOrbitInputComponent::OnMoveDown(const FInputActionValue& Value)
{
	float InputValue = Value.Get<float>();
	// Zero out on Completed (when chord is broken or key released)  
	VerticalInput = FMath::IsNearlyZero(InputValue, 0.01f) ? 0.0f : -InputValue;
}

// --- Private Helpers ---

void UOrbitInputComponent::ValidateActionStates()
{
	// With proper Enhanced Input trigger configuration (Down trigger), 
	// we don't need to validate against raw key state.
	// This function is kept for potential future debugging but does nothing by default.
	
	// If you experience stuck states, ensure your IMC uses:
	// - IA_Select: Down trigger (not Pressed or Tap)
	// - IA_Pan: Down trigger  
	// - IA_FlyMode: Down trigger
	// - IA_ModifierAlt: Down trigger
	// - IA_ModifierShift: Down trigger
}

UOrbitInputComponent::ENavMode UOrbitInputComponent::GetCurrentNavMode() const
{
	// Priority: Dolly > Orbit > Pan > Look
	if (bAltDown && bFlyModeActive)
	{
		return ENavMode::Dolly;
	}
	// IA_Orbit action (Alt+LMB chorded)
	if (bOrbitActionActive)
	{
		return ENavMode::Orbit;
	}
	// Alt + Select = Orbit
	if (bAltDown && bSelectActionActive)
	{
		return ENavMode::Orbit;
	}
	if (bPanActionActive)
	{
		return ENavMode::Pan;
	}
	if (bFlyModeActive)
	{
		return ENavMode::Look;
	}
	return ENavMode::None;
}

void UOrbitInputComponent::UpdateMouseLockState()
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return;
	}

	ENavMode NavMode = GetCurrentNavMode();
	bool bShouldLock = (NavMode != ENavMode::None);

	if (bShouldLock && !bMouseLocked)
	{
		// Hide cursor during navigation - DON'T change input mode as it disrupts Enhanced Input
		PC->SetShowMouseCursor(false);
		bMouseLocked = true;

		// Reset mouse position tracking
		float MouseX, MouseY;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			LastMousePosition = FVector2D(MouseX, MouseY);
		}

		if (bDebugEnabled)
		{
			UE_LOG(LogOrbitInput, Log, TEXT("Mouse LOCKED (NavMode: %d)"), static_cast<int32>(NavMode));
		}
	}
	else if (!bShouldLock && bMouseLocked)
	{
		// Show cursor when navigation ends
		PC->SetShowMouseCursor(true);
		bMouseLocked = false;

		if (bDebugEnabled)
		{
			UE_LOG(LogOrbitInput, Log, TEXT("Mouse UNLOCKED"));
		}
	}
}

void UOrbitInputComponent::ProcessMouseNavigation(float DeltaTime)
{
	ENavMode NavMode = GetCurrentNavMode();
	if (NavMode == ENavMode::None)
	{
		return;
	}

	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return;
	}

	// Get current mouse position
	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	FVector2D CurrentMousePos(MouseX, MouseY);
	FVector2D MouseDelta = CurrentMousePos - LastMousePosition;
	LastMousePosition = CurrentMousePos;

	// Skip if no movement
	if (MouseDelta.IsNearlyZero())
	{
		return;
	}

	AArchVisOrbitPawn* Pawn = GetOrbitPawn();
	if (!Pawn)
	{
		return;
	}

	// Route to appropriate navigation function based on mode
	switch (NavMode)
	{
	case ENavMode::Pan:
		Pawn->Pan(MouseDelta);
		if (bDebugEnabled)
		{
			UE_LOG(LogOrbitInput, Log, TEXT("Nav Pan: %s"), *MouseDelta.ToString());
		}
		break;

	case ENavMode::Orbit:
		Pawn->Orbit(MouseDelta);
		if (bDebugEnabled)
		{
			UE_LOG(LogOrbitInput, Log, TEXT("Nav Orbit: %s"), *MouseDelta.ToString());
		}
		break;

	case ENavMode::Look:
		Pawn->Look(MouseDelta);
		if (bDebugEnabled)
		{
			UE_LOG(LogOrbitInput, Log, TEXT("Nav Look: %s"), *MouseDelta.ToString());
		}
		break;

	case ENavMode::Dolly:
		// Dolly uses only Y axis
		Pawn->DollyZoom(MouseDelta.Y);
		if (bDebugEnabled)
		{
			UE_LOG(LogOrbitInput, Log, TEXT("Nav Dolly: %f"), MouseDelta.Y);
		}
		break;

	default:
		break;
	}
}

AArchVisOrbitPawn* UOrbitInputComponent::GetOrbitPawn() const
{
	return GetOwningPawn<AArchVisOrbitPawn>();
}

