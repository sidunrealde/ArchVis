// OrbitInputComponent.cpp
// Input component for 3D Orbit pawn - Unreal Editor-style navigation

#include "Input/OrbitInputComponent.h"
#include "ArchVisOrbitPawn.h"
#include "ArchVisInputConfig.h"
#include "EnhancedInputSubsystems.h"
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

	// Process WASD movement during fly mode
	ENavMode NavMode = GetCurrentNavMode();
	if (NavMode == ENavMode::Look && (!MoveInput.IsNearlyZero() || !FMath::IsNearlyZero(VerticalInput)))
	{
		if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
		{
			Pawn->SetMovementInput(MoveInput);
			Pawn->SetVerticalInput(VerticalInput);
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
	
	// Select action (used with Alt modifier for orbit)
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
	
	UpdateMouseLockState();

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitInput, Log, TEXT("IA_Select: Started Alt=%d"), bAltDown);
	}

	// Update pawn orbit mode (Alt + Select = Orbit)
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		if (bAltDown)
		{
			Pawn->SetOrbitModeActive(true);
		}
	}
}

void UOrbitInputComponent::OnSelectActionCompleted(const FInputActionValue& Value)
{
	bSelectActionActive = false;
	UpdateMouseLockState();

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitInput, Log, TEXT("IA_Select: Completed"));
	}

	// End orbit mode
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		Pawn->SetOrbitModeActive(false);
	}
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
	UpdateMouseLockState();

	// Always log for debugging
	UE_LOG(LogOrbitInput, Log, TEXT("IA_FlyMode: Completed - bFlyModeActive=%d"), bFlyModeActive);

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
	UpdateMouseLockState();

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitInput, Log, TEXT("IA_ModifierAlt: Started Select=%d FlyMode=%d"), bSelectActionActive, bFlyModeActive);
	}

	// Update pawn mode when Alt is pressed while other actions are held
	if (AArchVisOrbitPawn* Pawn = GetOrbitPawn())
	{
		if (bFlyModeActive)
		{
			// Switch from fly to dolly
			Pawn->SetFlyModeActive(false);
			Pawn->SetDollyModeActive(true);
		}
		if (bSelectActionActive)
		{
			Pawn->SetOrbitModeActive(true);
		}
	}
}

void UOrbitInputComponent::OnAltModifierCompleted(const FInputActionValue& Value)
{
	bAltDown = false;
	UpdateMouseLockState();

	if (bDebugEnabled)
	{
		UE_LOG(LogOrbitInput, Log, TEXT("IA_ModifierAlt: Completed Select=%d FlyMode=%d"), bSelectActionActive, bFlyModeActive);
	}

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
			Pawn->SetOrbitModeActive(false);
		}
	}
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

void UOrbitInputComponent::OnMove(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
}

void UOrbitInputComponent::OnMoveUp(const FInputActionValue& Value)
{
	VerticalInput = Value.Get<float>();
}

void UOrbitInputComponent::OnMoveDown(const FInputActionValue& Value)
{
	VerticalInput = -Value.Get<float>();
}

// --- Private Helpers ---

UOrbitInputComponent::ENavMode UOrbitInputComponent::GetCurrentNavMode() const
{
	// Priority: Dolly > Orbit > Pan > Look
	if (bAltDown && bFlyModeActive)
	{
		return ENavMode::Dolly;
	}
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
		// Lock mouse - game only input, hide cursor
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
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
		// Unlock mouse - game and UI input, show cursor
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
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

