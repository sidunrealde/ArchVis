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
#include "Tools/RTPlanArcTool.h"
#include "Tools/RTPlanSelectTool.h"
#include "RTPlanShellActor.h"
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

	// Note: Selection system setup is deferred to OnGameModeReady() because
	// GameMode::StartPlay creates ToolManager and ShellActor AFTER BeginPlay

	// Debug: Log current state
	UE_LOG(LogArchVisPC, Log, TEXT("BeginPlay Complete:"));
	UE_LOG(LogArchVisPC, Log, TEXT("  - Pawn: %s"), GetPawn() ? *GetPawn()->GetClass()->GetName() : TEXT("NULL"));
	UE_LOG(LogArchVisPC, Log, TEXT("  - PawnType: %d"), static_cast<int32>(CurrentPawnType));
	UE_LOG(LogArchVisPC, Log, TEXT("  - InputConfig: %s"), InputConfig ? *InputConfig->GetName() : TEXT("NULL"));
	UE_LOG(LogArchVisPC, Log, TEXT("  - InteractionMode: %s"), CurrentInteractionMode == EArchVisInteractionMode::Drafting2D ? TEXT("2D") : TEXT("3D"));
}

void AArchVisPlayerController::OnGameModeReady()
{
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	// Set up selection system now that ToolManager and ShellActor exist
	if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
	{
		ToolMgr->SelectToolByType(ERTPlanToolType::Select);
		Current2DToolMode = EArchVis2DToolMode::Selection;

		// Bind to selection changed event
		if (URTPlanSelectTool* SelectTool = ToolMgr->GetSelectTool())
		{
			SelectTool->OnSelectionChanged.AddDynamic(this, &AArchVisPlayerController::HandleSelectionChanged);
		}
	}
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
		
		// Modifiers - use Started/Triggered for initial state, Completed for release
		// NOTE: For chorded actions to work, the IMC binding for IA_ModifierCtrl/Shift/Alt
		// must use a "Down" trigger so they continuously emit Triggered events while held.
		if (InputConfig->IA_ModifierCtrl)
		{
			EIC->BindAction(InputConfig->IA_ModifierCtrl, ETriggerEvent::Started, this, &AArchVisPlayerController::OnModifierCtrlStarted);
			EIC->BindAction(InputConfig->IA_ModifierCtrl, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnModifierCtrlStarted);
			EIC->BindAction(InputConfig->IA_ModifierCtrl, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnModifierCtrlCompleted);
		}
		if (InputConfig->IA_ModifierShift)
		{
			EIC->BindAction(InputConfig->IA_ModifierShift, ETriggerEvent::Started, this, &AArchVisPlayerController::OnModifierShiftStarted);
			EIC->BindAction(InputConfig->IA_ModifierShift, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnModifierShiftStarted);
			EIC->BindAction(InputConfig->IA_ModifierShift, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnModifierShiftCompleted);
		}
		if (InputConfig->IA_ModifierAlt)
		{
			EIC->BindAction(InputConfig->IA_ModifierAlt, ETriggerEvent::Started, this, &AArchVisPlayerController::OnModifierAltStarted);
			EIC->BindAction(InputConfig->IA_ModifierAlt, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnModifierAltStarted);
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
		if (InputConfig->IA_FocusSelection)
		{
			EIC->BindAction(InputConfig->IA_FocusSelection, ETriggerEvent::Started, this, &AArchVisPlayerController::OnFocusSelection);
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
		if (InputConfig->IA_ToolArc)
		{
			EIC->BindAction(InputConfig->IA_ToolArc, ETriggerEvent::Started, this, &AArchVisPlayerController::OnToolArcHotkey);
		}
		if (InputConfig->IA_ToolTrim)
		{
			EIC->BindAction(InputConfig->IA_ToolTrim, ETriggerEvent::Started, this, &AArchVisPlayerController::OnToolTrimHotkey);
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
			EIC->BindAction(InputConfig->IA_SelectAdd, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnSelectAddCompleted);
		}
		if (InputConfig->IA_SelectToggle)
		{
			EIC->BindAction(InputConfig->IA_SelectToggle, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSelectToggle);
			EIC->BindAction(InputConfig->IA_SelectToggle, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnSelectToggleCompleted);
		}
		if (InputConfig->IA_SelectRemove)
		{
			EIC->BindAction(InputConfig->IA_SelectRemove, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSelectRemove);
			EIC->BindAction(InputConfig->IA_SelectRemove, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnSelectRemoveCompleted);
		}
		if (InputConfig->IA_SelectAll)
		{
			EIC->BindAction(InputConfig->IA_SelectAll, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSelectAll);
			// Also bind Triggered to catch it if Started misses due to chord timing
			EIC->BindAction(InputConfig->IA_SelectAll, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnSelectAll);
			UE_LOG(LogArchVisPC, Log, TEXT("Bound IA_SelectAll"));
		}
		if (InputConfig->IA_DeselectAll)
		{
			EIC->BindAction(InputConfig->IA_DeselectAll, ETriggerEvent::Started, this, &AArchVisPlayerController::OnDeselectAll);
			// Also bind Triggered
			EIC->BindAction(InputConfig->IA_DeselectAll, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnDeselectAll);
			UE_LOG(LogArchVisPC, Log, TEXT("Bound IA_DeselectAll"));
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
		// VIEW/GRID ACTIONS
		// ============================================
		if (InputConfig->IA_SnapToggle)
		{
			EIC->BindAction(InputConfig->IA_SnapToggle, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSnapToggle);
		}
		if (InputConfig->IA_GridToggle)
		{
			EIC->BindAction(InputConfig->IA_GridToggle, ETriggerEvent::Started, this, &AArchVisPlayerController::OnGridToggle);
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

		// Add 3D Selection and Navigation (both Priority 3 to override tools for Orbit/Select)
		AddMappingContext(InputConfig->IMC_3D_Selection, IMC_Priority_NumericEntry);
		AddMappingContext(InputConfig->IMC_3D_Navigation, IMC_Priority_NumericEntry);
		
		// Add 2D tool contexts for 3D mode as well, but only if they are active
		if (Current2DToolMode == EArchVis2DToolMode::LineTool)
		{
			AddMappingContext(InputConfig->IMC_2D_LineTool, IMC_Priority_Tool);
			ActivateNumericEntryContext();
		}
		else if (Current2DToolMode == EArchVis2DToolMode::PolylineTool)
		{
			AddMappingContext(InputConfig->IMC_2D_PolylineTool, IMC_Priority_Tool);
			ActivateNumericEntryContext();
		}
	}

	UE_LOG(LogArchVisPC, Log, TEXT("Updated IMCs - Mode: %s, Tool: %s"),
		CurrentInteractionMode == EArchVisInteractionMode::Drafting2D ? TEXT("2D") : TEXT("3D"),
		*UEnum::GetValueAsString(Current2DToolMode));
}

void AArchVisPlayerController::SwitchTo2DToolMode(EArchVis2DToolMode NewToolMode)
{
	if (!InputConfig) return;

	UE_LOG(LogArchVisPC, Log, TEXT("SwitchTo2DToolMode: %s"), *UEnum::GetValueAsString(NewToolMode));

	// Remove current tool context
	ClearAllToolContexts();
	Current2DToolMode = NewToolMode;

	// Restore 3D contexts if in 3D mode (because ClearAllToolContexts removed them)
	if (CurrentInteractionMode == EArchVisInteractionMode::Navigation3D)
	{
		AddMappingContext(InputConfig->IMC_3D_Selection, IMC_Priority_NumericEntry);
		AddMappingContext(InputConfig->IMC_3D_Navigation, IMC_Priority_NumericEntry);
	}

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

	case EArchVis2DToolMode::ArcTool:
		ActiveToolIMC = InputConfig->IMC_2D_ArcTool;
		break;

	case EArchVis2DToolMode::TrimTool:
		ActiveToolIMC = InputConfig->IMC_2D_TrimTool;
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
	if (NewToolMode == EArchVis2DToolMode::LineTool || NewToolMode == EArchVis2DToolMode::PolylineTool || NewToolMode == EArchVis2DToolMode::ArcTool)
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
	RemoveMappingContext(InputConfig->IMC_2D_ArcTool);
	RemoveMappingContext(InputConfig->IMC_2D_TrimTool);
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
	
	// Only show/hide cursor - DON'T change input mode as it disrupts Enhanced Input action tracking
	if (bShouldLockMouse)
	{
		SetShowMouseCursor(false);
		
		if (bInputDebugEnabled)
		{
			UE_LOG(LogArchVisPC, Log, TEXT("UpdateMouseLockState: Mouse LOCKED (orbit)"));
		}
	}
	else
	{
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
	if (!NumericInputBuffer.Buffer.IsEmpty())
	{
		// Check if mouse moved significantly
		float Dist = FVector2D::Distance(VirtualCursorPos, NumericInputStartMousePos);
		if (Dist > NumericInputMoveThreshold)
		{
			// Cancel numeric input
			OnNumericCancel(FInputActionValue());
			UE_LOG(LogArchVisPC, Log, TEXT("Numeric input cancelled by mouse movement (Dist: %.1f)"), Dist);
			// Fall through to allow move event processing
		}
		else
		{
			return;
		}
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
	
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SetSnapEnabled(bSnapToggledOn);
		}
	}
}

void AArchVisPlayerController::OnGridToggle(const FInputActionValue& Value)
{
	bGridVisible = !bGridVisible;
	UE_LOG(LogArchVisPC, Log, TEXT("Grid toggled: %s"), bGridVisible ? TEXT("ON") : TEXT("OFF"));
	
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SetGridEnabled(bGridVisible);
		}
	}
}

void AArchVisPlayerController::OnToggleView(const FInputActionValue& Value)
{
	// Block view toggling while numeric entry is active (Tab should switch fields)
	if (bNumericEntryContextActive)
	{
		return;
	}

	// Only allow view toggling if we are in Selection mode (or no tool)
	// This prevents accidental mode switching while drawing tools are active
	if (Current2DToolMode == EArchVis2DToolMode::LineTool || 
		Current2DToolMode == EArchVis2DToolMode::PolylineTool ||
		Current2DToolMode == EArchVis2DToolMode::ArcTool)
	{
		return;
	}

	// Toggle between 2D and 3D pawns
	ToggleViewPawn();
}

// ============================================
// SELECTION HANDLERS
// ============================================

void AArchVisPlayerController::OnSelectStarted(const FInputActionValue& Value)
{
	// Determine mode based on active modifiers (tracked via Enhanced Input actions)
	// This ensures selection works even if specific chorded actions (like Shift+Click)
	// fail to trigger due to priority or consumption issues.
	if (bShiftDown)
	{
		CurrentSelectionMode = ESelectionMode::Add;
		UE_LOG(LogArchVisPC, Log, TEXT(">>> OnSelectStarted (Add Mode - via Modifier)"));
	}
	else if (bCtrlDown)
	{
		CurrentSelectionMode = ESelectionMode::Toggle;
		UE_LOG(LogArchVisPC, Log, TEXT(">>> OnSelectStarted (Toggle Mode - via Modifier)"));
	}
	else if (bAltDown)
	{
		CurrentSelectionMode = ESelectionMode::Remove;
		UE_LOG(LogArchVisPC, Log, TEXT(">>> OnSelectStarted (Remove Mode - via Modifier)"));
	}
	else
	{
		CurrentSelectionMode = ESelectionMode::Replace;
		UE_LOG(LogArchVisPC, Log, TEXT(">>> OnSelectStarted (Replace Mode)"));
	}
	
	bSelectActionActive = true;
	
	// If we are in 3D mode and orbit is active, we should not process selection
	if (CurrentInteractionMode == EArchVisInteractionMode::Navigation3D && bOrbitActive)
	{
		return;
	}
	
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;
	
	URTPlanToolManager* ToolMgr = GM->GetToolManager();
	if (!ToolMgr) return;
	
	FRTPointerEvent Event = GetPointerEvent(ERTPointerAction::Down);
	Event.bShiftDown = (CurrentSelectionMode == ESelectionMode::Add);
	Event.bAltDown = (CurrentSelectionMode == ESelectionMode::Remove);
	Event.bCtrlDown = (CurrentSelectionMode == ESelectionMode::Toggle);
	ToolMgr->ProcessInput(Event);
}

void AArchVisPlayerController::OnSelectCompleted(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT(">>> OnSelectCompleted: bSelectActionActive=%d, Mode=%d"), 
		bSelectActionActive, static_cast<int32>(CurrentSelectionMode));
	
	if (!bSelectActionActive)
	{
		return;
	}
	
	bSelectActionActive = false;
	
	// If we are in 3D mode and orbit is active, we should not process selection
	if (CurrentInteractionMode == EArchVisInteractionMode::Navigation3D && bOrbitActive)
	{
		return;
	}
	
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;
	
	URTPlanToolManager* ToolMgr = GM->GetToolManager();
	if (!ToolMgr) return;
	
	FRTPointerEvent Event = GetPointerEvent(ERTPointerAction::Up);
	Event.bShiftDown = (CurrentSelectionMode == ESelectionMode::Add);
	Event.bAltDown = (CurrentSelectionMode == ESelectionMode::Remove);
	Event.bCtrlDown = (CurrentSelectionMode == ESelectionMode::Toggle);
	ToolMgr->ProcessInput(Event);
}

void AArchVisPlayerController::OnSelectAdd(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnSelectAdd triggered (Add Mode)"));
	
	CurrentSelectionMode = ESelectionMode::Add;
	bSelectActionActive = true;
	
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;
	
	URTPlanToolManager* ToolMgr = GM->GetToolManager();
	if (!ToolMgr) return;
	
	FRTPointerEvent Event = GetPointerEvent(ERTPointerAction::Down);
	Event.bShiftDown = true;
	Event.bAltDown = false;
	Event.bCtrlDown = false;
	ToolMgr->ProcessInput(Event);
}

void AArchVisPlayerController::OnSelectAddCompleted(const FInputActionValue& Value)
{
	// Delegate to common completion handler
	OnSelectCompleted(Value);
}

void AArchVisPlayerController::OnSelectToggle(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnSelectToggle triggered (Toggle Mode)"));
	
	CurrentSelectionMode = ESelectionMode::Toggle;
	bSelectActionActive = true;
	
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;
	
	URTPlanToolManager* ToolMgr = GM->GetToolManager();
	if (!ToolMgr) return;
	
	FRTPointerEvent Event = GetPointerEvent(ERTPointerAction::Down);
	Event.bShiftDown = false;
	Event.bAltDown = false;
	Event.bCtrlDown = true;
	ToolMgr->ProcessInput(Event);
}

void AArchVisPlayerController::OnSelectToggleCompleted(const FInputActionValue& Value)
{
	// Delegate to common completion handler
	OnSelectCompleted(Value);
}

void AArchVisPlayerController::OnSelectRemove(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnSelectRemove triggered (Remove Mode)"));
	
	CurrentSelectionMode = ESelectionMode::Remove;
	bSelectActionActive = true;
	
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;
	
	URTPlanToolManager* ToolMgr = GM->GetToolManager();
	if (!ToolMgr) return;
	
	FRTPointerEvent Event = GetPointerEvent(ERTPointerAction::Down);
	Event.bShiftDown = false;
	Event.bAltDown = true;
	Event.bCtrlDown = false;
	ToolMgr->ProcessInput(Event);
}

void AArchVisPlayerController::OnSelectRemoveCompleted(const FInputActionValue& Value)
{
	// Delegate to common completion handler
	OnSelectCompleted(Value);
}

void AArchVisPlayerController::OnSelectAll(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnSelectAll triggered"));
	
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) 
	{
		UE_LOG(LogArchVisPC, Warning, TEXT("OnSelectAll: GM is null"));
		return;
	}
	
	URTPlanToolManager* ToolMgr = GM->GetToolManager();
	if (!ToolMgr) 
	{
		UE_LOG(LogArchVisPC, Warning, TEXT("OnSelectAll: ToolMgr is null"));
		return;
	}
	
	URTPlanSelectTool* SelectTool = ToolMgr->GetSelectTool();
	if (SelectTool)
	{
		SelectTool->SelectAll();
	}
	else
	{
		UE_LOG(LogArchVisPC, Warning, TEXT("OnSelectAll: SelectTool is null"));
	}
}

void AArchVisPlayerController::OnDeselectAll(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnDeselectAll triggered"));
	
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) 
	{
		UE_LOG(LogArchVisPC, Warning, TEXT("OnDeselectAll: GM is null"));
		return;
	}
	
	URTPlanToolManager* ToolMgr = GM->GetToolManager();
	if (!ToolMgr) 
	{
		UE_LOG(LogArchVisPC, Warning, TEXT("OnDeselectAll: ToolMgr is null"));
		return;
	}
	
	URTPlanSelectTool* SelectTool = ToolMgr->GetSelectTool();
	if (SelectTool)
	{
		SelectTool->ClearSelection();
	}
	else
	{
		UE_LOG(LogArchVisPC, Warning, TEXT("OnDeselectAll: SelectTool is null"));
	}
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

void AArchVisPlayerController::HandleSelectionChanged()
{
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;

	URTPlanToolManager* ToolMgr = GM->GetToolManager();
	ARTPlanShellActor* ShellActor = GM->GetShellActor();
	
	if (!ToolMgr) return;

	URTPlanSelectTool* SelectTool = ToolMgr->GetSelectTool();
	if (!SelectTool) return;

	// Get selected wall IDs
	TArray<FGuid> SelectedWalls = SelectTool->GetSelectedWallIds();

	// Update shell actor highlighting
	if (ShellActor)
	{
		ShellActor->SetSelectedWalls(SelectedWalls);
	}

	// Log wall properties to output
	if (SelectedWalls.Num() > 0)
	{
		SelectTool->LogSelectedWallProperties();
	}
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

	// If numeric input was active, cancelling the draw should also cancel numeric input.
	if (bNumericEntryContextActive)
	{
		// Only deactivate context if we are NOT in a persistent drawing tool
		bool bIsDrawingTool = (Current2DToolMode == EArchVis2DToolMode::LineTool || 
							   Current2DToolMode == EArchVis2DToolMode::PolylineTool);
		
		if (!bIsDrawingTool)
		{
			DeactivateNumericEntryContext();
			UE_LOG(LogArchVisPC, Log, TEXT("Numeric entry cancelled due to DrawCancel"));
		}
		else
		{
			// For drawing tools, just clear the buffer but keep context active
			// so user can immediately type numbers for the next operation
			NumericInputBuffer.ClearAll();
			UE_LOG(LogArchVisPC, Log, TEXT("Numeric buffer cleared due to DrawCancel (Context remains active)"));
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
	
	// Capture mouse position if starting new input
	if (NumericInputBuffer.Buffer.IsEmpty())
	{
		// Update VirtualCursorPos to ensure we have the latest position
		float MouseX, MouseY;
		if (GetMousePosition(MouseX, MouseY))
		{
			VirtualCursorPos = FVector2D(MouseX, MouseY);
		}
		NumericInputStartMousePos = VirtualCursorPos;
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
	
	// Capture mouse position if starting new input
	if (NumericInputBuffer.Buffer.IsEmpty())
	{
		// Update VirtualCursorPos to ensure we have the latest position
		float MouseX, MouseY;
		if (GetMousePosition(MouseX, MouseY))
		{
			VirtualCursorPos = FVector2D(MouseX, MouseY);
		}
		NumericInputStartMousePos = VirtualCursorPos;
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
	if (NumericInputBuffer.Buffer.IsEmpty())
	{
		// Buffer empty - user pressed Enter likely to confirm drawing
		// Pass through to DrawConfirm
		OnDrawConfirm(Value);
	}
	else
	{
		CommitNumericInput();
		
		// Only deactivate if NOT in a drawing tool that supports continuous input
		bool bIsDrawingTool = (Current2DToolMode == EArchVis2DToolMode::LineTool || 
							   Current2DToolMode == EArchVis2DToolMode::PolylineTool ||
							   Current2DToolMode == EArchVis2DToolMode::ArcTool);
		
		if (!bIsDrawingTool)
		{
			DeactivateNumericEntryContext();
		}
	}
}

void AArchVisPlayerController::OnNumericCancel(const FInputActionValue& Value)
{
	bool bWasEmpty = NumericInputBuffer.Buffer.IsEmpty();
	NumericInputBuffer.ClearAll();
	
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

	// If buffer was empty, we want to exit numeric mode (allow tool switching etc)
	// If buffer was NOT empty, we just cleared the typo, keep context active for retrying
	if (bWasEmpty)
	{
		DeactivateNumericEntryContext();
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
	
	// Restore any previously saved value for the new field
	NumericInputBuffer.RestoreFieldValue();
	
	// Update the tool preview with the restored value
	UpdateToolPreviewFromBuffer();
	
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

void AArchVisPlayerController::OnFocusSelection(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnFocusSelection triggered"));
	
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (!GM) return;
	
	URTPlanToolManager* ToolMgr = GM->GetToolManager();
	if (!ToolMgr) return;
	
	URTPlanSelectTool* SelectTool = ToolMgr->GetSelectTool();
	if (!SelectTool || !SelectTool->HasSelection())
	{
		UE_LOG(LogArchVisPC, Log, TEXT("  No selection to focus on"));
		return;
	}
	
	FVector SelectionCenter = SelectTool->GetSelectionCenter();
	FBox SelectionBounds = SelectTool->GetSelectionBounds();
	
	UE_LOG(LogArchVisPC, Log, TEXT("  Selection center: (%0.1f, %0.1f, %0.1f)"), 
		SelectionCenter.X, SelectionCenter.Y, SelectionCenter.Z);
	
	// Calculate radius of the bounding sphere
	float Radius = SelectionBounds.GetExtent().Size();
	// Ensure minimum radius to avoid getting too close to single points
	Radius = FMath::Max(Radius, 100.0f);

	AArchVisPawnBase* ArchVisPawn = Cast<AArchVisPawnBase>(GetPawn());
	if (!ArchVisPawn)
	{
		return;
	}

	if (CurrentInteractionMode == EArchVisInteractionMode::Drafting2D)
	{
		// For 2D, we want to center on the selection and zoom to fit.
		// SetCameraTransform expects TargetArmLength as ZoomLevel for DraftingPawn.
		// We use a heuristic for zoom level to ensure the object fits with some margin.
		float TargetZoom = Radius * 2.5f; 
		
		FVector NewLocation = FVector(SelectionCenter.X, SelectionCenter.Y, 0.0f);
		
		// Use the pawn's interface to set transform, which handles internal state (TargetLocation, etc.)
		ArchVisPawn->SetCameraTransform(NewLocation, FRotator(-90.0f, 0.0f, 0.0f), TargetZoom);
		
		UE_LOG(LogArchVisPC, Log, TEXT("  2D focus: Moved to (%0.1f, %0.1f), Zoom=%.1f"),
			NewLocation.X, NewLocation.Y, TargetZoom);
	}
	else
	{
		// For 3D, maintain current rotation but move camera to look at center from appropriate distance
		FRotator CurrentRotation = ArchVisPawn->GetCameraRotation();
		
		// Calculate distance to fit object in ~60 deg FOV
		float TargetDistance = Radius * 2.5f; 
		
		FVector NewLocation = SelectionCenter - (CurrentRotation.Vector() * TargetDistance);
		
		// Use the pawn's interface to set transform, which handles internal state (OrbitFocusPoint, etc.)
		ArchVisPawn->SetCameraTransform(NewLocation, CurrentRotation, TargetDistance);
		
		UE_LOG(LogArchVisPC, Log, TEXT("  3D focus: Moved to (%0.1f, %0.1f, %0.1f), Dist=%.1f"),
			NewLocation.X, NewLocation.Y, NewLocation.Z, TargetDistance);
	}
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

void AArchVisPlayerController::OnToolArcHotkey(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnToolArcHotkey triggered"));
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SelectToolByType(ERTPlanToolType::Arc);
			OnToolChanged(ERTPlanToolType::Arc);
			SwitchTo2DToolMode(EArchVis2DToolMode::ArcTool);
		}
	}
}

void AArchVisPlayerController::OnToolTrimHotkey(const FInputActionValue& Value)
{
	UE_LOG(LogArchVisPC, Log, TEXT("OnToolTrimHotkey triggered"));
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SelectToolByType(ERTPlanToolType::Trim);
			OnToolChanged(ERTPlanToolType::Trim);
			SwitchTo2DToolMode(EArchVis2DToolMode::TrimTool);
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
			// Handle Line Tool
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
			// Handle Arc Tool
			else if (URTPlanArcTool* ArcTool = Cast<URTPlanArcTool>(ToolMgr->GetActiveTool()))
			{
				if (NumericInputBuffer.ActiveField == ERTNumericField::Length)
				{
					ArcTool->UpdatePreviewFromLength(Value);
				}
				else
				{
					// Clamp angle to 0-180 degrees
					Value = FMath::Clamp(Value, 0.0f, 180.0f);
					ArcTool->UpdatePreviewFromAngle(Value);
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
	
	// Clear the buffer but keep the input mode ready for the next value
	// This allows multi-step tools (like arc) to continue accepting numeric input
	NumericInputBuffer.Buffer.Empty();
	NumericInputBuffer.bIsActive = false;
	// Don't clear saved values - they may be needed for switching fields
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
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->DeleteSelection();
			UE_LOG(LogArchVisPC, Log, TEXT("Delete selection triggered"));
		}
	}
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
	UE_LOG(LogArchVisPC, Log, TEXT("Modifier Ctrl Started"));
}

void AArchVisPlayerController::OnModifierCtrlCompleted(const FInputActionValue& Value)
{
	bCtrlDown = false;
	UE_LOG(LogArchVisPC, Log, TEXT("Modifier Ctrl Completed"));
}

void AArchVisPlayerController::OnModifierShiftStarted(const FInputActionValue& Value)
{
	bShiftDown = true;
	bSnapModifierHeld = true;
	UE_LOG(LogArchVisPC, Log, TEXT("Modifier Shift Started"));
}

void AArchVisPlayerController::OnModifierShiftCompleted(const FInputActionValue& Value)
{
	bShiftDown = false;
	bSnapModifierHeld = false;
	UE_LOG(LogArchVisPC, Log, TEXT("Modifier Shift Completed"));
}

void AArchVisPlayerController::OnModifierAltStarted(const FInputActionValue& Value)
{
	bAltDown = true;
	UE_LOG(LogArchVisPC, Log, TEXT("Modifier Alt Started"));
	
	if (bInputDebugEnabled)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("IA_ModifierAlt Started - bAltDown=true Select=%d"), bSelectActionActive);
	}
}

void AArchVisPlayerController::OnModifierAltCompleted(const FInputActionValue& Value)
{
	bAltDown = false;
	UE_LOG(LogArchVisPC, Log, TEXT("Modifier Alt Completed"));
	
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

void AArchVisPlayerController::SetSelectionDebugEnabled(bool bEnabled)
{
	bSelectionDebugEnabled = bEnabled;
	UE_LOG(LogArchVisPC, Log, TEXT("Selection debug: %s"), bSelectionDebugEnabled ? TEXT("ON") : TEXT("OFF"));
	
	// Sync with SelectTool
	AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this));
	if (GM)
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanSelectTool* SelectTool = ToolMgr->GetSelectTool())
			{
				SelectTool->SetDebugEnabled(bSelectionDebugEnabled);
			}
		}
	}
}

void AArchVisPlayerController::ArchVisToggleSelectionDebug()
{
	SetSelectionDebugEnabled(!bSelectionDebugEnabled);
}
