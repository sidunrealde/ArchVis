#include "ArchVisPlayerController.h"
#include "ArchVisGameMode.h"
#include "ArchVisInputConfig.h"
#include "ArchVisPawnBase.h"
#include "ArchVisDraftingPawn.h"
#include "ArchVisOrbitPawn.h"
#include "ArchVisFlyPawn.h"
#include "ArchVisFirstPersonPawn.h"
#include "ArchVisThirdPersonPawn.h"
#include "Input/ToolInputComponent.h"
#include "RTPlanToolManager.h"
#include "RTPlanToolBase.h"
#include "Tools/RTPlanLineTool.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"

DEFINE_LOG_CATEGORY_STATIC(LogArchVisPC, Log, All);

AArchVisPlayerController::AArchVisPlayerController()
{
	bShowMouseCursor = false; 
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	// Create tool input component for handling drawing/selection
	ToolInput = CreateDefaultSubobject<UToolInputComponent>(TEXT("ToolInput"));
}

void AArchVisPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Initialize Virtual Cursor to center of screen
	int32 SizeX, SizeY;
	GetViewportSize(SizeX, SizeY);
	VirtualCursorPos = FVector2D(SizeX * 0.5f, SizeY * 0.5f);
	
	// Initialize last mouse position
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		LastMousePosition = FVector2D(MouseX, MouseY);
		VirtualCursorPos = LastMousePosition;
	}

	// Initialize numeric buffer with default unit
	NumericInputBuffer.CurrentUnit = DefaultUnit;

	// Check if we already have a pawn (spawned by GameMode)
	APawn* ExistingPawn = GetPawn();
	if (ExistingPawn)
	{
		// Check if it's one of our ArchVis pawns and initialize their input
		if (AArchVisDraftingPawn* DraftingPawn = Cast<AArchVisDraftingPawn>(ExistingPawn))
		{
			CurrentPawnType = EArchVisPawnType::Drafting2D;
			DraftingPawn->InitializeInput(InputConfig);
			UE_LOG(LogArchVisPC, Log, TEXT("Using existing Drafting2D pawn"));
		}
		else if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(ExistingPawn))
		{
			CurrentPawnType = EArchVisPawnType::Orbit3D;
			OrbitPawn->InitializeInput(InputConfig);
			UE_LOG(LogArchVisPC, Log, TEXT("Using existing Orbit3D pawn"));
		}
		else if (Cast<AArchVisFlyPawn>(ExistingPawn))
		{
			CurrentPawnType = EArchVisPawnType::Fly;
			UE_LOG(LogArchVisPC, Log, TEXT("Using existing Fly pawn"));
		}
		else if (Cast<AArchVisFirstPersonPawn>(ExistingPawn))
		{
			CurrentPawnType = EArchVisPawnType::FirstPerson;
			UE_LOG(LogArchVisPC, Log, TEXT("Using existing FirstPerson pawn"));
		}
		else if (Cast<AArchVisThirdPersonPawn>(ExistingPawn))
		{
			CurrentPawnType = EArchVisPawnType::ThirdPerson;
			UE_LOG(LogArchVisPC, Log, TEXT("Using existing ThirdPerson pawn"));
		}
		else
		{
			// Not an ArchVis pawn - destroy it and spawn our default
			UE_LOG(LogArchVisPC, Log, TEXT("Replacing default pawn with Drafting2D pawn"));
			UnPossess();
			ExistingPawn->Destroy();
			
			APawn* NewDraftingPawn = SpawnArchVisPawn(EArchVisPawnType::Drafting2D, FVector::ZeroVector, FRotator::ZeroRotator);
			if (NewDraftingPawn)
			{
				Possess(NewDraftingPawn);
				CurrentPawnType = EArchVisPawnType::Drafting2D;
				if (AArchVisDraftingPawn* DPawn = Cast<AArchVisDraftingPawn>(NewDraftingPawn))
				{
					DPawn->InitializeInput(InputConfig);
				}
			}
		}
	}
	else
	{
		// No pawn - spawn our default
		APawn* NewDraftingPawn = SpawnArchVisPawn(EArchVisPawnType::Drafting2D, FVector::ZeroVector, FRotator::ZeroRotator);
		if (NewDraftingPawn)
		{
			Possess(NewDraftingPawn);
			CurrentPawnType = EArchVisPawnType::Drafting2D;
			if (AArchVisDraftingPawn* DPawn = Cast<AArchVisDraftingPawn>(NewDraftingPawn))
			{
				DPawn->InitializeInput(InputConfig);
			}
		}
	}


	// Set up initial Input Mapping Contexts
	UpdateInputMappingContexts();

	// Select default tool (Select tool for 2D mode)
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SelectToolByType(ERTPlanToolType::Select);
			Current2DToolMode = EArchVis2DToolMode::Selection;
			UE_LOG(LogArchVisPC, Log, TEXT("Default tool set to Select"));
		}
	}

	// Debug: Log current state
	UE_LOG(LogArchVisPC, Log, TEXT("BeginPlay Complete:"));
	UE_LOG(LogArchVisPC, Log, TEXT("  - Pawn: %s"), GetPawn() ? *GetPawn()->GetClass()->GetName() : TEXT("NULL"));
	UE_LOG(LogArchVisPC, Log, TEXT("  - PawnType: %d"), static_cast<int32>(CurrentPawnType));
	UE_LOG(LogArchVisPC, Log, TEXT("  - InputConfig: %s"), InputConfig ? *InputConfig->GetName() : TEXT("NULL"));
	UE_LOG(LogArchVisPC, Log, TEXT("  - InteractionMode: %s"), CurrentInteractionMode == EArchVisInteractionMode::Drafting2D ? TEXT("2D") : TEXT("3D"));
}

void AArchVisPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (!InputConfig)
		{
			UE_LOG(LogArchVisPC, Warning, TEXT("InputConfig is null! Assign the data asset in Blueprint."));
			return;
		}

		// ============================================
		// GLOBAL ACTIONS (IMC_Global)
		// ============================================
		
		// Modifiers
		if (InputConfig->IA_ModifierCtrl)
		{
			EIC->BindAction(InputConfig->IA_ModifierCtrl, ETriggerEvent::Started, this, &AArchVisPlayerController::OnModifierCtrlStarted);
			EIC->BindAction(InputConfig->IA_ModifierCtrl, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnModifierCtrlCompleted);
		}
		if (InputConfig->IA_ModifierShift)
		{
			EIC->BindAction(InputConfig->IA_ModifierShift, ETriggerEvent::Started, this, &AArchVisPlayerController::OnModifierShiftStarted);
			EIC->BindAction(InputConfig->IA_ModifierShift, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnModifierShiftCompleted);
		}
		if (InputConfig->IA_ModifierAlt)
		{
			EIC->BindAction(InputConfig->IA_ModifierAlt, ETriggerEvent::Started, this, &AArchVisPlayerController::OnModifierAltStarted);
			EIC->BindAction(InputConfig->IA_ModifierAlt, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnModifierAltCompleted);
		}

		// Global commands
		if (InputConfig->IA_Undo)
		{
			EIC->BindAction(InputConfig->IA_Undo, ETriggerEvent::Started, this, &AArchVisPlayerController::OnUndo);
		}
		if (InputConfig->IA_Redo)
		{
			EIC->BindAction(InputConfig->IA_Redo, ETriggerEvent::Started, this, &AArchVisPlayerController::OnRedo);
		}
		if (InputConfig->IA_Delete)
		{
			EIC->BindAction(InputConfig->IA_Delete, ETriggerEvent::Started, this, &AArchVisPlayerController::OnDelete);
		}
		if (InputConfig->IA_Escape)
		{
			EIC->BindAction(InputConfig->IA_Escape, ETriggerEvent::Started, this, &AArchVisPlayerController::OnEscape);
		}
		if (InputConfig->IA_ToggleView)
		{
			EIC->BindAction(InputConfig->IA_ToggleView, ETriggerEvent::Started, this, &AArchVisPlayerController::OnToggleView);
		}
		if (InputConfig->IA_Save)
		{
			EIC->BindAction(InputConfig->IA_Save, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSave);
		}

		// Tool hotkeys
		if (InputConfig->IA_ToolSelect)
		{
			EIC->BindAction(InputConfig->IA_ToolSelect, ETriggerEvent::Started, this, &AArchVisPlayerController::OnToolSelectHotkey);
		}
		if (InputConfig->IA_ToolLine)
		{
			EIC->BindAction(InputConfig->IA_ToolLine, ETriggerEvent::Started, this, &AArchVisPlayerController::OnToolLineHotkey);
		}
		if (InputConfig->IA_ToolPolyline)
		{
			EIC->BindAction(InputConfig->IA_ToolPolyline, ETriggerEvent::Started, this, &AArchVisPlayerController::OnToolPolylineHotkey);
		}

		// NOTE: Navigation actions (Pan, Zoom, Orbit, Fly, etc.) are now handled by
		// pawn-specific input components:
		// - UDraftingInputComponent for 2D pan/zoom
		// - UOrbitInputComponent for 3D orbit/pan/dolly/fly

		// ============================================
		// SELECTION ACTIONS (IMC_2D_Selection / IMC_3D_Selection)
		// ============================================
		
		if (InputConfig->IA_Select)
		{
			EIC->BindAction(InputConfig->IA_Select, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSelectStarted);
			EIC->BindAction(InputConfig->IA_Select, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnSelectCompleted);
		}
		if (InputConfig->IA_SelectAdd)
		{
			EIC->BindAction(InputConfig->IA_SelectAdd, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSelectAdd);
		}
		if (InputConfig->IA_SelectToggle)
		{
			EIC->BindAction(InputConfig->IA_SelectToggle, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSelectToggle);
		}
		if (InputConfig->IA_SelectAll)
		{
			EIC->BindAction(InputConfig->IA_SelectAll, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSelectAll);
		}
		if (InputConfig->IA_DeselectAll)
		{
			EIC->BindAction(InputConfig->IA_DeselectAll, ETriggerEvent::Started, this, &AArchVisPlayerController::OnDeselectAll);
		}
		if (InputConfig->IA_BoxSelectStart)
		{
			EIC->BindAction(InputConfig->IA_BoxSelectStart, ETriggerEvent::Started, this, &AArchVisPlayerController::OnBoxSelectStart);
		}
		if (InputConfig->IA_BoxSelectDrag)
		{
			EIC->BindAction(InputConfig->IA_BoxSelectDrag, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnBoxSelectDrag);
		}
		if (InputConfig->IA_BoxSelectEnd)
		{
			EIC->BindAction(InputConfig->IA_BoxSelectEnd, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnBoxSelectEnd);
		}
		if (InputConfig->IA_CycleSelection)
		{
			EIC->BindAction(InputConfig->IA_CycleSelection, ETriggerEvent::Started, this, &AArchVisPlayerController::OnCycleSelection);
		}

		// ============================================
		// DRAWING ACTIONS (IMC_2D_LineTool / IMC_2D_PolylineTool)
		// ============================================
		
		if (InputConfig->IA_DrawPlacePoint)
		{
			EIC->BindAction(InputConfig->IA_DrawPlacePoint, ETriggerEvent::Started, this, &AArchVisPlayerController::OnDrawPlacePoint);
		}
		if (InputConfig->IA_DrawConfirm)
		{
			EIC->BindAction(InputConfig->IA_DrawConfirm, ETriggerEvent::Started, this, &AArchVisPlayerController::OnDrawConfirm);
		}
		if (InputConfig->IA_DrawCancel)
		{
			EIC->BindAction(InputConfig->IA_DrawCancel, ETriggerEvent::Started, this, &AArchVisPlayerController::OnDrawCancel);
		}
		if (InputConfig->IA_DrawClose)
		{
			EIC->BindAction(InputConfig->IA_DrawClose, ETriggerEvent::Started, this, &AArchVisPlayerController::OnDrawClose);
		}
		if (InputConfig->IA_DrawRemoveLastPoint)
		{
			EIC->BindAction(InputConfig->IA_DrawRemoveLastPoint, ETriggerEvent::Started, this, &AArchVisPlayerController::OnDrawRemoveLastPoint);
		}
		if (InputConfig->IA_OrthoLock)
		{
			EIC->BindAction(InputConfig->IA_OrthoLock, ETriggerEvent::Started, this, &AArchVisPlayerController::OnOrthoLock);
			EIC->BindAction(InputConfig->IA_OrthoLock, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnOrthoLockReleased);
		}
		if (InputConfig->IA_AngleSnap)
		{
			EIC->BindAction(InputConfig->IA_AngleSnap, ETriggerEvent::Started, this, &AArchVisPlayerController::OnAngleSnap);
		}

		// ============================================
		// NUMERIC ENTRY ACTIONS (IMC_NumericEntry)
		// ============================================
		
		// Single digit action with 1D Axis - receives 1-9 for digits 1-9, 10 for digit 0
		if (InputConfig->IA_NumericDigit)
		{
			EIC->BindAction(InputConfig->IA_NumericDigit, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericDigitInput);
		}

		if (InputConfig->IA_NumericDecimal)
		{
			EIC->BindAction(InputConfig->IA_NumericDecimal, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericDecimal);
		}
		if (InputConfig->IA_NumericBackspace)
		{
			EIC->BindAction(InputConfig->IA_NumericBackspace, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericBackspaceStarted);
			EIC->BindAction(InputConfig->IA_NumericBackspace, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnNumericBackspaceCompleted);
		}
		if (InputConfig->IA_NumericCommit)
		{
			EIC->BindAction(InputConfig->IA_NumericCommit, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericCommit);
		}
		if (InputConfig->IA_NumericCancel)
		{
			EIC->BindAction(InputConfig->IA_NumericCancel, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericCancel);
		}
		if (InputConfig->IA_NumericSwitchField)
		{
			EIC->BindAction(InputConfig->IA_NumericSwitchField, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericSwitchField);
		}
		if (InputConfig->IA_NumericCycleUnits)
		{
			EIC->BindAction(InputConfig->IA_NumericCycleUnits, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericCycleUnits);
		}

		// Arithmetic operators
		if (InputConfig->IA_NumericAdd)
		{
			EIC->BindAction(InputConfig->IA_NumericAdd, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericAdd);
		}
		if (InputConfig->IA_NumericSubtract)
		{
			EIC->BindAction(InputConfig->IA_NumericSubtract, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericSubtract);
		}
		if (InputConfig->IA_NumericMultiply)
		{
			EIC->BindAction(InputConfig->IA_NumericMultiply, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericMultiply);
		}
		if (InputConfig->IA_NumericDivide)
		{
			EIC->BindAction(InputConfig->IA_NumericDivide, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericDivide);
		}


		// ============================================
		// TOOL INPUT COMPONENT INITIALIZATION
		// ============================================
		// The ToolInputComponent handles tool-specific actions (drawing, selection)
		// that are pawn-agnostic. It's initialized here with the shared InputConfig.
		if (ToolInput)
		{
			ToolInput->Initialize(InputConfig, EIC);
			UE_LOG(LogArchVisPC, Log, TEXT("ToolInputComponent initialized"));
		}
	}
}

// ============================================
// INPUT MAPPING CONTEXT MANAGEMENT
// ============================================

void AArchVisPlayerController::AddMappingContext(UInputMappingContext* Context, int32 Priority)
{
	if (!Context) 
	{
		UE_LOG(LogArchVisPC, Warning, TEXT("AddMappingContext: Context is NULL"));
		return;
	}
	
	ULocalPlayer* LP = GetLocalPlayer();
	if (!LP)
	{
		UE_LOG(LogArchVisPC, Warning, TEXT("AddMappingContext: LocalPlayer is NULL"));
		return;
	}
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
	{
		if (!Subsystem->HasMappingContext(Context))
		{
			Subsystem->AddMappingContext(Context, Priority);
			UE_LOG(LogArchVisPC, Log, TEXT("Added IMC: %s (Priority: %d)"), *Context->GetName(), Priority);
		}
		else
		{
			UE_LOG(LogArchVisPC, Log, TEXT("IMC already active: %s"), *Context->GetName());
		}
	}
	else
	{
		UE_LOG(LogArchVisPC, Warning, TEXT("AddMappingContext: Subsystem is NULL"));
	}
}

void AArchVisPlayerController::RemoveMappingContext(UInputMappingContext* Context)
{
	if (!Context) return;
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (Subsystem->HasMappingContext(Context))
		{
			Subsystem->RemoveMappingContext(Context);
			UE_LOG(LogArchVisPC, Log, TEXT("Removed IMC: %s"), *Context->GetName());
		}
	}
}

void AArchVisPlayerController::UpdateInputMappingContexts()
{
	if (!InputConfig) 
	{
		UE_LOG(LogArchVisPC, Error, TEXT("UpdateInputMappingContexts: InputConfig is NULL!"));
		return;
	}

	UE_LOG(LogArchVisPC, Log, TEXT("UpdateInputMappingContexts: Starting..."));

	// Always add Global context (Priority 0)
	AddMappingContext(InputConfig->IMC_Global, IMC_Priority_Global);

	// Remove old mode base context
	RemoveMappingContext(ActiveModeBaseIMC);
	ClearAllToolContexts();
	
	if (CurrentInteractionMode == EArchVisInteractionMode::Drafting2D)
	{
		// Add 2D Base context (Priority 1)
		ActiveModeBaseIMC = InputConfig->IMC_2D_Base;
		if (!ActiveModeBaseIMC)
		{
			UE_LOG(LogArchVisPC, Error, TEXT("IMC_2D_Base is NULL in InputConfig!"));
		}
		AddMappingContext(ActiveModeBaseIMC, IMC_Priority_ModeBase);

		// Add appropriate tool context (Priority 2)
		SwitchTo2DToolMode(Current2DToolMode);
	}
	else // Navigation3D
	{
		// Add 3D Base context (Priority 1)
		ActiveModeBaseIMC = InputConfig->IMC_3D_Base;
		AddMappingContext(ActiveModeBaseIMC, IMC_Priority_ModeBase);

		// Add 3D Selection and Navigation (both Priority 2)
		AddMappingContext(InputConfig->IMC_3D_Selection, IMC_Priority_Tool);
		AddMappingContext(InputConfig->IMC_3D_Navigation, IMC_Priority_Tool);
	}

	UE_LOG(LogArchVisPC, Log, TEXT("Updated IMCs - Mode: %s, Tool: %s"),
		CurrentInteractionMode == EArchVisInteractionMode::Drafting2D ? TEXT("2D") : TEXT("3D"),
		*UEnum::GetValueAsString(Current2DToolMode));
}

void AArchVisPlayerController::SwitchTo2DToolMode(EArchVis2DToolMode NewToolMode)
{
	if (!InputConfig) return;
	if (CurrentInteractionMode != EArchVisInteractionMode::Drafting2D) return;

	UE_LOG(LogArchVisPC, Log, TEXT("SwitchTo2DToolMode: %s"), *UEnum::GetValueAsString(NewToolMode));

	// Remove current tool context
	ClearAllToolContexts();
	Current2DToolMode = NewToolMode;

	switch (NewToolMode)
	{
	case EArchVis2DToolMode::Selection:
		ActiveToolIMC = InputConfig->IMC_2D_Selection;
		if (!ActiveToolIMC)
		{
			UE_LOG(LogArchVisPC, Error, TEXT("IMC_2D_Selection is NULL!"));
		}
		break;

	case EArchVis2DToolMode::LineTool:
		ActiveToolIMC = InputConfig->IMC_2D_LineTool;
		break;

	case EArchVis2DToolMode::PolylineTool:
		ActiveToolIMC = InputConfig->IMC_2D_PolylineTool;
		break;

	default:
		ActiveToolIMC = nullptr;
		break;
	}

	if (ActiveToolIMC)
	{
		AddMappingContext(ActiveToolIMC, IMC_Priority_Tool);
	}

	// For drawing tools (Line/Polyline), enable numeric entry context immediately
	// so digit keys are available for precision input without needing a bootstrap trigger
	if (NewToolMode == EArchVis2DToolMode::LineTool || NewToolMode == EArchVis2DToolMode::PolylineTool)
	{
		ActivateNumericEntryContext();
	}
}

void AArchVisPlayerController::ClearAllToolContexts()
{
	if (!InputConfig) return;

	// Remove all possible tool contexts
	RemoveMappingContext(InputConfig->IMC_2D_Selection);
	RemoveMappingContext(InputConfig->IMC_2D_LineTool);
	RemoveMappingContext(InputConfig->IMC_2D_PolylineTool);
	RemoveMappingContext(InputConfig->IMC_3D_Selection);
	RemoveMappingContext(InputConfig->IMC_3D_Navigation);
	
	// Also deactivate numeric entry if active
	DeactivateNumericEntryContext();
	
	ActiveToolIMC = nullptr;
}

void AArchVisPlayerController::UpdateMouseLockState()
{
	// Lock mouse during orbit (Alt + Select action) which is triggered via OnSelectStarted
	// Other navigation modes are handled by pawn input components which manage their own mouse state
	bool bShouldLockMouse = bOrbitActive;
	
	UGameViewportClient* GameViewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!GameViewport)
	{
		return;
	}
	
	if (bShouldLockMouse)
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		SetShowMouseCursor(false);
		
		if (bInputDebugEnabled)
		{
			UE_LOG(LogArchVisPC, Log, TEXT("UpdateMouseLockState: Mouse LOCKED (orbit)"));
		}
	}
	else
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		SetShowMouseCursor(true);
		
		if (bInputDebugEnabled)
		{
			UE_LOG(LogArchVisPC, Log, TEXT("UpdateMouseLockState: Mouse RELEASED"));
		}
	}
}

void AArchVisPlayerController::ActivateNumericEntryContext()
{
	if (!InputConfig || bNumericEntryContextActive) return;

	AddMappingContext(InputConfig->IMC_NumericEntry, IMC_Priority_NumericEntry);
	bNumericEntryContextActive = true;
	UE_LOG(LogArchVisPC, Log, TEXT("Numeric Entry context activated"));
}

void AArchVisPlayerController::DeactivateNumericEntryContext()
{
	if (!InputConfig || !bNumericEntryContextActive) return;

	RemoveMappingContext(InputConfig->IMC_NumericEntry);
	bNumericEntryContextActive = false;
	NumericInputBuffer.ClearAll();
	UE_LOG(LogArchVisPC, Log, TEXT("Numeric Entry context deactivated"));
}

void AArchVisPlayerController::SetInteractionMode(EArchVisInteractionMode NewMode)
{
	if (CurrentInteractionMode != NewMode)
	{
		// Clear orbit state on mode transition
		bOrbitActive = false;
		bSelectActionActive = false;

		CurrentInteractionMode = NewMode;
		UpdateInputMappingContexts();

		if (bInputDebugEnabled)
		{
			UE_LOG(LogArchVisPC, Log, TEXT("SetInteractionMode -> %s"),
				(CurrentInteractionMode == EArchVisInteractionMode::Drafting2D) ? TEXT("2D") : TEXT("3D"));
		}
	}
}

void AArchVisPlayerController::OnToolChanged(ERTPlanToolType NewToolType)
{
	if (CurrentToolType != NewToolType)
	{
		CurrentToolType = NewToolType;
		UpdateInputMappingContexts();
	}
}

// ============================================
// TICK
// ============================================

void AArchVisPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update virtual cursor position (for HUD crosshair)
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		VirtualCursorPos = FVector2D(MouseX, MouseY);
		LastMousePosition = VirtualCursorPos;
	}

	// Handle backspace key repeat for numeric input
	if (bBackspaceHeld)
	{
		BackspaceHoldTime += DeltaTime;
		
		if (BackspaceHoldTime >= BackspaceNextRepeatTime)
		{
			if (!NumericInputBuffer.Buffer.IsEmpty())
			{
				PerformBackspace();
			}
			else
			{
				// Buffer is empty - delegate to RemoveLastPoint if in polyline mode
				if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
				{
					if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
					{
						if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
						{
							if (LineTool->IsPolylineMode() && LineTool->IsDrawingActive())
							{
								LineTool->RemoveLastPoint();
							}
						}
					}
				}
			}
			BackspaceNextRepeatTime = BackspaceHoldTime + BackspaceRepeatRate;
		}
	}

	// Don't send move events when numeric input is active
	if (NumericInputBuffer.bIsActive && !NumericInputBuffer.Buffer.IsEmpty())
	{
		return;
	}

	// Send move event to active tool
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
			{
				if (LineTool->IsNumericInputActive())
				{
					return;
				}
			}
			
			ToolMgr->ProcessInput(GetPointerEvent(ERTPointerAction::Move));
		}
	}
}

// ============================================
// GLOBAL HANDLERS
// ============================================


void AArchVisPlayerController::OnSnapToggle(const FInputActionValue& Value)
{
	bSnapToggledOn = !bSnapToggledOn;
	UE_LOG(LogArchVisPC, Log, TEXT("Snap toggled: %s"), bSnapToggledOn ? TEXT("ON") : TEXT("OFF"));
}

void AArchVisPlayerController::OnGridToggle(const FInputActionValue& Value)
{
	bGridVisible = !bGridVisible;
	UE_LOG(LogArchVisPC, Log, TEXT("Grid toggled: %s"), bGridVisible ? TEXT("ON") : TEXT("OFF"));
}

void AArchVisPlayerController::OnToggleView(const FInputActionValue& Value)
{
	// Toggle between 2D and 3D pawns
	ToggleViewPawn();
}

// ============================================
// SELECTION HANDLERS
// ============================================

void AArchVisPlayerController::OnSelectStarted(const FInputActionValue& Value)
{
	bSelectActionActive = true;
	
	if (bInputDebugEnabled)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("OnSelectStarted: bAltDown=%d"), bAltDown);
	}
	
	// If Alt is held, this is an orbit action, not a selection
	if (bAltDown)
	{
		// Activate orbit mode on the pawn
		if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
		{
			bOrbitActive = true;
			OrbitPawn->SetOrbitModeActive(true);
		}
		// Update mouse lock for 3D navigation
		if (CurrentInteractionMode == EArchVisInteractionMode::Navigation3D)
		{
			UpdateMouseLockState();
		}
		return;
	}
	
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			URTPlanToolBase* ActiveTool = ToolMgr->GetActiveTool();
			
			if (bInputDebugEnabled)
			{
				UE_LOG(LogArchVisPC, Log, TEXT("OnSelectStarted: ActiveTool=%s, ToolType=%d"), 
					ActiveTool ? *ActiveTool->GetClass()->GetName() : TEXT("NULL"),
					static_cast<int32>(ToolMgr->GetActiveToolType()));
			}
			
			ToolMgr->ProcessInput(GetPointerEvent(ERTPointerAction::Down));
		}
	}
}

void AArchVisPlayerController::OnSelectCompleted(const FInputActionValue& Value)
{
	bSelectActionActive = false;
	
	if (bInputDebugEnabled)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("OnSelectCompleted: bOrbitActive=%d"), bOrbitActive);
	}
	
	// End orbit mode if it was active
	if (bOrbitActive)
	{
		bOrbitActive = false;
		if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
		{
			OrbitPawn->SetOrbitModeActive(false);
		}
		// Update mouse lock for 3D navigation
		if (CurrentInteractionMode == EArchVisInteractionMode::Navigation3D)
		{
			UpdateMouseLockState();
		}
	}
}

void AArchVisPlayerController::OnSelectAdd(const FInputActionValue& Value)
{
	// Shift+Click - handled via modifier flags in pointer event
	OnSelectStarted(Value);
}

void AArchVisPlayerController::OnSelectToggle(const FInputActionValue& Value)
{
	// Ctrl+Click - handled via modifier flags in pointer event
	OnSelectStarted(Value);
}

void AArchVisPlayerController::OnSelectAll(const FInputActionValue& Value)
{
	// TODO: Select all via tool manager
	UE_LOG(LogArchVisPC, Log, TEXT("Select All triggered"));
}

void AArchVisPlayerController::OnDeselectAll(const FInputActionValue& Value)
{
	// TODO: Deselect all via tool manager
	UE_LOG(LogArchVisPC, Log, TEXT("Deselect All triggered"));
}

void AArchVisPlayerController::OnBoxSelectStart(const FInputActionValue& Value)
{
	bBoxSelectActive = true;
	BoxSelectStart = VirtualCursorPos;
}

void AArchVisPlayerController::OnBoxSelectDrag(const FInputActionValue& Value)
{
	if (!bBoxSelectActive) return;
	// Box selection visualization handled in HUD
}

void AArchVisPlayerController::OnBoxSelectEnd(const FInputActionValue& Value)
{
	if (!bBoxSelectActive) return;
	
	bBoxSelectActive = false;
	// TODO: Perform box selection via tool manager
}

void AArchVisPlayerController::OnCycleSelection(const FInputActionValue& Value)
{
	// TODO: Cycle through overlapping selections
}

// ============================================
// DRAWING HANDLERS
// ============================================

void AArchVisPlayerController::OnDrawPlacePoint(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->ProcessInput(GetPointerEvent(ERTPointerAction::Down));
		}
	}
}

void AArchVisPlayerController::OnDrawConfirm(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->ProcessInput(GetPointerEvent(ERTPointerAction::Confirm));
		}
	}
}

void AArchVisPlayerController::OnDrawCancel(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
			{
				LineTool->CancelDrawing();
				UE_LOG(LogArchVisPC, Log, TEXT("Drawing cancelled"));
			}
		}
	}
}

void AArchVisPlayerController::OnDrawClose(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
			{
				if (LineTool->IsPolylineMode())
				{
					LineTool->ClosePolyline();
					UE_LOG(LogArchVisPC, Log, TEXT("Polyline closed"));
				}
			}
		}
	}
}

void AArchVisPlayerController::OnDrawRemoveLastPoint(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
			{
				if (LineTool->IsPolylineMode())
				{
					LineTool->RemoveLastPoint();
					UE_LOG(LogArchVisPC, Log, TEXT("Last point removed"));
				}
			}
		}
	}
}

void AArchVisPlayerController::OnOrthoLock(const FInputActionValue& Value)
{
	bOrthoLockActive = true;
}

void AArchVisPlayerController::OnOrthoLockReleased(const FInputActionValue& Value)
{
	bOrthoLockActive = false;
}

void AArchVisPlayerController::OnAngleSnap(const FInputActionValue& Value)
{
	bAngleSnapEnabled = !bAngleSnapEnabled;
	UE_LOG(LogArchVisPC, Log, TEXT("Angle snap toggled: %s"), bAngleSnapEnabled ? TEXT("ON") : TEXT("OFF"));
}

// ============================================
// NUMERIC ENTRY HANDLERS
// ============================================

void AArchVisPlayerController::OnNumericDigit(int32 Digit)
{
	// Activate numeric entry context if not already active
	if (!bNumericEntryContextActive)
	{
		ActivateNumericEntryContext();
	}
	
	NumericInputBuffer.AppendChar(TEXT('0') + Digit);
	UE_LOG(LogArchVisPC, Log, TEXT("Numeric: %d, Buffer: %s"), Digit, *NumericInputBuffer.Buffer);
	
	UpdateToolPreviewFromBuffer();
}

void AArchVisPlayerController::OnNumericDigitInput(const FInputActionValue& Value)
{
	// Get the axis value - 1-9 for digits 1-9, 10 for digit 0
	float AxisValue = Value.Get<float>();
	int32 Digit = FMath::RoundToInt(AxisValue);
	
	// Special case: 10 maps to 0
	if (Digit == 10)
	{
		Digit = 0;
	}
	
	// Clamp to valid digit range
	Digit = FMath::Clamp(Digit, 0, 9);
	
	OnNumericDigit(Digit);
}

void AArchVisPlayerController::OnNumericDecimal(const FInputActionValue& Value)
{
	if (!bNumericEntryContextActive)
	{
		ActivateNumericEntryContext();
	}
	
	NumericInputBuffer.AppendChar(TEXT('.'));
	UE_LOG(LogArchVisPC, Log, TEXT("Decimal, Buffer: %s"), *NumericInputBuffer.Buffer);
	
	UpdateToolPreviewFromBuffer();
}

void AArchVisPlayerController::OnNumericBackspaceStarted(const FInputActionValue& Value)
{
	// If buffer is empty and we're in a drawing tool, delegate to RemoveLastPoint
	if (NumericInputBuffer.Buffer.IsEmpty())
	{
		if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
			{
				if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
				{
					if (LineTool->IsPolylineMode() && LineTool->IsDrawingActive())
					{
						LineTool->RemoveLastPoint();
						UE_LOG(LogArchVisPC, Log, TEXT("Backspace -> RemoveLastPoint (buffer empty)"));
						return;
					}
				}
			}
		}
	}
	
	bBackspaceHeld = true;
	BackspaceHoldTime = 0.0f;
	BackspaceNextRepeatTime = BackspaceRepeatDelay;
	
	// Perform immediate backspace on key press
	PerformBackspace();
}

void AArchVisPlayerController::OnNumericBackspaceCompleted(const FInputActionValue& Value)
{
	bBackspaceHeld = false;
	BackspaceHoldTime = 0.0f;
}

void AArchVisPlayerController::PerformBackspace()
{
	NumericInputBuffer.Backspace();
	UE_LOG(LogArchVisPC, Log, TEXT("Backspace, Buffer: %s"), *NumericInputBuffer.Buffer);
	
	// If buffer is empty, clear the tool's numeric override but keep numeric context active
	// for drawing tools so user can continue entering digits
	if (NumericInputBuffer.Buffer.IsEmpty())
	{
		// Clear tool's numeric preview
		if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
			{
				if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
				{
					LineTool->ClearNumericInputOverride();
				}
			}
		}
		// Keep numeric context active for drawing tools - don't deactivate
	}
	else
	{
		UpdateToolPreviewFromBuffer();
	}
}

void AArchVisPlayerController::OnNumericCommit(const FInputActionValue& Value)
{
	CommitNumericInput();
	DeactivateNumericEntryContext();
}

void AArchVisPlayerController::OnNumericCancel(const FInputActionValue& Value)
{
	NumericInputBuffer.ClearAll();
	DeactivateNumericEntryContext();
	
	// Also clear tool override
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
			{
				LineTool->ClearNumericInputOverride();
			}
		}
	}
}

void AArchVisPlayerController::OnNumericSwitchField(const FInputActionValue& Value)
{
	// Save current value before switching
	NumericInputBuffer.SaveCurrentFieldValue();
	
	// Toggle between Length and Angle
	if (NumericInputBuffer.ActiveField == ERTNumericField::Length)
	{
		NumericInputBuffer.ActiveField = ERTNumericField::Angle;
	}
	else
	{
		NumericInputBuffer.ActiveField = ERTNumericField::Length;
	}
	
	// Clear buffer for new field input
	NumericInputBuffer.Buffer.Empty();
	
	UE_LOG(LogArchVisPC, Log, TEXT("Switched to field: %s"),
		NumericInputBuffer.ActiveField == ERTNumericField::Length ? TEXT("Length") : TEXT("Angle"));
}

void AArchVisPlayerController::OnNumericCycleUnits(const FInputActionValue& Value)
{
	// Only apply unit conversion to length field, not angle
	if (NumericInputBuffer.ActiveField != ERTNumericField::Length)
	{
		// For angle, just cycle the unit (no conversion needed)
		NumericInputBuffer.CycleUnit();
		UE_LOG(LogArchVisPC, Log, TEXT("Cycled to unit: %s"), *UEnum::GetValueAsString(NumericInputBuffer.CurrentUnit));
		return;
	}
	
	// Get the current value in cm before cycling
	float ValueInCm = NumericInputBuffer.GetValueInCm();
	
	// Cycle to the next unit
	NumericInputBuffer.CycleUnit();
	
	// Convert the cm value to the new unit and update the buffer
	// This keeps the line length the same but shows the equivalent value in the new unit
	if (!NumericInputBuffer.Buffer.IsEmpty() && ValueInCm > 0.0f)
	{
		float NewDisplayValue = NumericInputBuffer.GetDisplayValue(ValueInCm);
		NumericInputBuffer.Buffer = FString::Printf(TEXT("%.2f"), NewDisplayValue);
		
		// Remove trailing zeros for cleaner display
		NumericInputBuffer.Buffer.TrimEndInline();
		if (NumericInputBuffer.Buffer.Contains(TEXT(".")))
		{
			while (NumericInputBuffer.Buffer.EndsWith(TEXT("0")))
			{
				NumericInputBuffer.Buffer.LeftChopInline(1);
			}
			if (NumericInputBuffer.Buffer.EndsWith(TEXT(".")))
			{
				NumericInputBuffer.Buffer.LeftChopInline(1);
			}
		}
	}
	
	UE_LOG(LogArchVisPC, Log, TEXT("Cycled to unit: %s, Buffer: %s"), 
		*UEnum::GetValueAsString(NumericInputBuffer.CurrentUnit), *NumericInputBuffer.Buffer);
	
	// Don't call UpdateToolPreviewFromBuffer() - the line length stays the same
}

void AArchVisPlayerController::OnNumericAdd(const FInputActionValue& Value)
{
	NumericInputBuffer.AppendOperator(ERTArithmeticOp::Add);
	UE_LOG(LogArchVisPC, Log, TEXT("Add operator, Buffer: %s"), *NumericInputBuffer.Buffer);
}

void AArchVisPlayerController::OnNumericSubtract(const FInputActionValue& Value)
{
	NumericInputBuffer.AppendOperator(ERTArithmeticOp::Subtract);
	UE_LOG(LogArchVisPC, Log, TEXT("Subtract operator, Buffer: %s"), *NumericInputBuffer.Buffer);
}

void AArchVisPlayerController::OnNumericMultiply(const FInputActionValue& Value)
{
	NumericInputBuffer.AppendOperator(ERTArithmeticOp::Multiply);
	UE_LOG(LogArchVisPC, Log, TEXT("Multiply operator, Buffer: %s"), *NumericInputBuffer.Buffer);
}

void AArchVisPlayerController::OnNumericDivide(const FInputActionValue& Value)
{
	NumericInputBuffer.AppendOperator(ERTArithmeticOp::Divide);
	UE_LOG(LogArchVisPC, Log, TEXT("Divide operator, Buffer: %s"), *NumericInputBuffer.Buffer);
}

// ============================================
// 3D NAVIGATION HANDLERS
// ============================================

void AArchVisPlayerController::OnViewTop(const FInputActionValue& Value)
{
	// TODO: Set camera to top view
	UE_LOG(LogArchVisPC, Log, TEXT("View Top triggered"));
}

void AArchVisPlayerController::OnViewFront(const FInputActionValue& Value)
{
	// TODO: Set camera to front view
	UE_LOG(LogArchVisPC, Log, TEXT("View Front triggered"));
}

void AArchVisPlayerController::OnViewRight(const FInputActionValue& Value)
{
	// TODO: Set camera to right view
	UE_LOG(LogArchVisPC, Log, TEXT("View Right triggered"));
}

void AArchVisPlayerController::OnViewPerspective(const FInputActionValue& Value)
{
	// TODO: Toggle ortho/perspective
	UE_LOG(LogArchVisPC, Log, TEXT("View Perspective toggle triggered"));
}

// ============================================
// TOOL HOTKEY HANDLERS
// ============================================

void AArchVisPlayerController::OnToolSelectHotkey(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnToolSelectHotkey triggered"));
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SelectToolByType(ERTPlanToolType::Select);
			OnToolChanged(ERTPlanToolType::Select);
			SwitchTo2DToolMode(EArchVis2DToolMode::Selection);
		}
	}
}

void AArchVisPlayerController::OnToolLineHotkey(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnToolLineHotkey triggered"));
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SelectToolByType(ERTPlanToolType::Line);
			OnToolChanged(ERTPlanToolType::Line);
			SwitchTo2DToolMode(EArchVis2DToolMode::LineTool);
		}
	}
}

void AArchVisPlayerController::OnToolPolylineHotkey(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnToolPolylineHotkey triggered"));
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SelectToolByType(ERTPlanToolType::Polyline);
			OnToolChanged(ERTPlanToolType::Polyline);
			SwitchTo2DToolMode(EArchVis2DToolMode::PolylineTool);
		}
	}
}

// ============================================
// HELPER FUNCTIONS
// ============================================

void AArchVisPlayerController::UpdateToolPreviewFromBuffer()
{
	if (NumericInputBuffer.Buffer.IsEmpty()) return;
	
	float Value = NumericInputBuffer.GetValueInCm();
	
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
			{
				if (NumericInputBuffer.ActiveField == ERTNumericField::Length)
				{
					LineTool->UpdatePreviewFromLength(Value);
				}
				else
				{
					// Clamp angle to 0-180 degrees
					Value = FMath::Clamp(Value, 0.0f, 180.0f);
					LineTool->UpdatePreviewFromAngle(Value);
				}
			}
		}
	}
}

void AArchVisPlayerController::CommitNumericInput()
{
	if (NumericInputBuffer.Buffer.IsEmpty()) return;
	
	float Value = NumericInputBuffer.GetValueInCm();
	
	// Clamp angle to 0-180 degrees
	if (NumericInputBuffer.ActiveField == ERTNumericField::Angle)
	{
		Value = FMath::Clamp(Value, 0.0f, 180.0f);
	}
	
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanToolBase* Tool = ToolMgr->GetActiveTool())
			{
				Tool->OnNumericInputWithField(Value, NumericInputBuffer.ActiveField);
			}
		}
	}
	
	NumericInputBuffer.Clear();
}

FRTPointerEvent AArchVisPlayerController::GetPointerEvent(ERTPointerAction Action) const
{
	FRTPointerEvent Event;
	Event.Source = ERTInputSource::Mouse;
	Event.Action = Action;
	Event.ScreenPosition = VirtualCursorPos;
	Event.bSnapEnabled = IsSnapEnabled();
	Event.bShiftDown = bShiftDown;
	Event.bAltDown = bAltDown;
	Event.bCtrlDown = bCtrlDown;
	Event.bOrthoLockActive = bOrthoLockActive;
	Event.bAngleSnapEnabled = bAngleSnapEnabled;

	FVector WorldLoc, WorldDir;
	if (DeprojectScreenPositionToWorld(VirtualCursorPos.X, VirtualCursorPos.Y, WorldLoc, WorldDir))
	{
		Event.WorldOrigin = WorldLoc;
		Event.WorldDirection = WorldDir;
	}

	return Event;
}

bool AArchVisPlayerController::IsSnapEnabled() const
{
	switch (SnapModifierMode)
	{
	case ESnapModifierMode::Toggle:
		return bSnapToggledOn;
	case ESnapModifierMode::HoldToDisable:
		return bSnapEnabledByDefault && !bSnapModifierHeld;
	case ESnapModifierMode::HoldToEnable:
		return !bSnapEnabledByDefault || bSnapModifierHeld;
	default:
		return bSnapEnabledByDefault;
	}
}

// --- Global Input Handlers ---

void AArchVisPlayerController::OnUndo(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanDocument* Doc = GM->GetDocument())
		{
			if (Doc->CanUndo())
			{
				Doc->Undo();
				UE_LOG(LogArchVisPC, Log, TEXT("Undo executed"));
			}
			else
			{
				UE_LOG(LogArchVisPC, Log, TEXT("Nothing to undo"));
			}
		}
	}
}

void AArchVisPlayerController::OnRedo(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanDocument* Doc = GM->GetDocument())
		{
			if (Doc->CanRedo())
			{
				Doc->Redo();
				UE_LOG(LogArchVisPC, Log, TEXT("Redo executed"));
			}
			else
			{
				UE_LOG(LogArchVisPC, Log, TEXT("Nothing to redo"));
			}
		}
	}
}

void AArchVisPlayerController::OnDelete(const FInputActionValue& Value)
{
	// TODO: Delete selected objects
	UE_LOG(LogTemp, Log, TEXT("Delete triggered"));
}

void AArchVisPlayerController::OnEscape(const FInputActionValue& Value)
{
	// Cancel current action or deselect
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanToolBase* Tool = ToolMgr->GetActiveTool())
			{
				// Cancel current drawing operation
				FRTPointerEvent Event = GetPointerEvent(ERTPointerAction::Cancel);
				Tool->OnPointerEvent(Event);
			}
		}
	}
	
	// Clear numeric input if active
	if (!NumericInputBuffer.Buffer.IsEmpty())
	{
		NumericInputBuffer.Clear();
		DeactivateNumericEntryContext();
	}
}

void AArchVisPlayerController::OnSave(const FInputActionValue& Value)
{
	// TODO: Implement save functionality
	UE_LOG(LogTemp, Log, TEXT("Save triggered"));
}

// --- Modifier Key Handlers ---

void AArchVisPlayerController::OnModifierCtrlStarted(const FInputActionValue& Value)
{
	bCtrlDown = true;
}

void AArchVisPlayerController::OnModifierCtrlCompleted(const FInputActionValue& Value)
{
	bCtrlDown = false;
}

void AArchVisPlayerController::OnModifierShiftStarted(const FInputActionValue& Value)
{
	bShiftDown = true;
	bSnapModifierHeld = true;
}

void AArchVisPlayerController::OnModifierShiftCompleted(const FInputActionValue& Value)
{
	bShiftDown = false;
	bSnapModifierHeld = false;
}

void AArchVisPlayerController::OnModifierAltStarted(const FInputActionValue& Value)
{
	bAltDown = true;
	
	if (bInputDebugEnabled)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("IA_ModifierAlt Started - bAltDown=true Select=%d"), bSelectActionActive);
	}
}

void AArchVisPlayerController::OnModifierAltCompleted(const FInputActionValue& Value)
{
	bAltDown = false;
	
	if (bInputDebugEnabled)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("IA_ModifierAlt Ended - bAltDown=false Select=%d"), bSelectActionActive);
	}
	
	// End orbit if it was active (Alt + Select action)
	if (bOrbitActive)
	{
		bOrbitActive = false;
		if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
		{
			OrbitPawn->SetOrbitModeActive(false);
		}
		UpdateMouseLockState();
	}
}

// ============================================
// PAWN MANAGEMENT
// ============================================

void AArchVisPlayerController::SwitchToPawnType(EArchVisPawnType NewPawnType)
{
	if (NewPawnType == CurrentPawnType)
	{
		return;
	}

	// Clear orbit state before switching pawns
	bOrbitActive = false;
	bSelectActionActive = false;

	// Get current camera state to transfer to new pawn
	FVector CurrentLocation = FVector::ZeroVector;
	FRotator CurrentRotation = FRotator::ZeroRotator;
	float CurrentZoom = 5000.0f;

	APawn* PreviousPawn = GetPawn();
	if (PreviousPawn)
	{
		CurrentLocation = PreviousPawn->GetActorLocation();
		CurrentRotation = PreviousPawn->GetActorRotation();
		
		// Try to get zoom from ArchVis pawn
		if (AArchVisPawnBase* ArchVisPawn = Cast<AArchVisPawnBase>(PreviousPawn))
		{
			CurrentZoom = ArchVisPawn->GetZoomLevel();
		}
	}

	// Spawn new pawn
	APawn* NewPawn = SpawnArchVisPawn(NewPawnType, CurrentLocation, CurrentRotation);
	if (NewPawn)
	{
		// Unpossess old pawn
		UnPossess();
		
		// Destroy old pawn
		if (PreviousPawn)
		{
			PreviousPawn->Destroy();
		}

		// Possess new pawn
		Possess(NewPawn);
		
		// Initialize pawn's input component with our InputConfig
		if (AArchVisDraftingPawn* DraftingPawn = Cast<AArchVisDraftingPawn>(NewPawn))
		{
			DraftingPawn->InitializeInput(InputConfig);
		}
		else if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(NewPawn))
		{
			OrbitPawn->InitializeInput(InputConfig);
		}
		
		// Transfer camera state
		if (AArchVisPawnBase* ArchVisPawn = Cast<AArchVisPawnBase>(NewPawn))
		{
			ArchVisPawn->SetCameraTransform(CurrentLocation, CurrentRotation, CurrentZoom);
		}

		CurrentPawnType = NewPawnType;
		
		// Update interaction mode based on pawn type
		if (NewPawnType == EArchVisPawnType::Drafting2D)
		{
			SetInteractionMode(EArchVisInteractionMode::Drafting2D);
		}
		else
		{
			SetInteractionMode(EArchVisInteractionMode::Navigation3D);
		}

		UE_LOG(LogArchVisPC, Log, TEXT("Switched to pawn type: %d"), static_cast<int32>(NewPawnType));
	}
}

void AArchVisPlayerController::ToggleViewPawn()
{
	if (CurrentPawnType == EArchVisPawnType::Drafting2D)
	{
		SwitchToPawnType(EArchVisPawnType::Orbit3D);
	}
	else
	{
		SwitchToPawnType(EArchVisPawnType::Drafting2D);
	}
}

AArchVisPawnBase* AArchVisPlayerController::GetArchVisPawn() const
{
	return Cast<AArchVisPawnBase>(GetPawn());
}

APawn* AArchVisPlayerController::SpawnArchVisPawn(EArchVisPawnType PawnType, FVector Location, FRotator Rotation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APawn* NewPawn = nullptr;

	switch (PawnType)
	{
	case EArchVisPawnType::Drafting2D:
		if (DraftingPawnClass)
		{
			NewPawn = World->SpawnActor<AArchVisDraftingPawn>(DraftingPawnClass, Location, Rotation, SpawnParams);
		}
		else
		{
			NewPawn = World->SpawnActor<AArchVisDraftingPawn>(AArchVisDraftingPawn::StaticClass(), Location, Rotation, SpawnParams);
		}
		break;

	case EArchVisPawnType::Orbit3D:
		if (OrbitPawnClass)
		{
			NewPawn = World->SpawnActor<AArchVisOrbitPawn>(OrbitPawnClass, Location, Rotation, SpawnParams);
		}
		else
		{
			NewPawn = World->SpawnActor<AArchVisOrbitPawn>(AArchVisOrbitPawn::StaticClass(), Location, Rotation, SpawnParams);
		}
		break;

	case EArchVisPawnType::Fly:
		if (FlyPawnClass)
		{
			NewPawn = World->SpawnActor<AArchVisFlyPawn>(FlyPawnClass, Location, Rotation, SpawnParams);
		}
		else
		{
			NewPawn = World->SpawnActor<AArchVisFlyPawn>(AArchVisFlyPawn::StaticClass(), Location, Rotation, SpawnParams);
		}
		break;

	case EArchVisPawnType::FirstPerson:
		if (FirstPersonPawnClass)
		{
			NewPawn = World->SpawnActor<AArchVisFirstPersonPawn>(FirstPersonPawnClass, Location, Rotation, SpawnParams);
		}
		else
		{
			NewPawn = World->SpawnActor<AArchVisFirstPersonPawn>(AArchVisFirstPersonPawn::StaticClass(), Location, Rotation, SpawnParams);
		}
		break;

	case EArchVisPawnType::ThirdPerson:
		if (ThirdPersonPawnClass)
		{
			NewPawn = World->SpawnActor<AArchVisThirdPersonPawn>(ThirdPersonPawnClass, Location, Rotation, SpawnParams);
		}
		else
		{
			NewPawn = World->SpawnActor<AArchVisThirdPersonPawn>(AArchVisThirdPersonPawn::StaticClass(), Location, Rotation, SpawnParams);
		}
		break;

	case EArchVisPawnType::VR:
		// VR pawn is handled by the RTPlanVR plugin
		UE_LOG(LogArchVisPC, Warning, TEXT("VR pawn spawning not implemented in base controller"));
		break;
	}

	return NewPawn;
}

// ============================================
// 3D NAV STATE HELPERS
// ============================================

void AArchVisPlayerController::ArchVisToggleInputDebug()
{
	bInputDebugEnabled = !bInputDebugEnabled;
	UE_LOG(LogArchVisPC, Log, TEXT("Input debug: %s"), bInputDebugEnabled ? TEXT("ON") : TEXT("OFF"));
	
	// Sync with pawn debug flag
	if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
	{
		OrbitPawn->SetDebugEnabled(bInputDebugEnabled);
	}
}

void AArchVisPlayerController::ArchVisSetInputDebug(int32 bEnabled)
{
	bInputDebugEnabled = (bEnabled != 0);
	UE_LOG(LogArchVisPC, Log, TEXT("Input debug: %s"), bInputDebugEnabled ? TEXT("ON") : TEXT("OFF"));
	
	// Sync with pawn debug flag
	if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
	{
		OrbitPawn->SetDebugEnabled(bInputDebugEnabled);
	}
}


