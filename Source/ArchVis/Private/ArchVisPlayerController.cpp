#include "ArchVisPlayerController.h"
#include "ArchVisGameMode.h"
#include "ArchVisCameraController.h"
#include "ArchVisInputConfig.h"
#include "ArchVisPawnBase.h"
#include "ArchVisDraftingPawn.h"
#include "ArchVisOrbitPawn.h"
#include "ArchVisFlyPawn.h"
#include "ArchVisFirstPersonPawn.h"
#include "ArchVisThirdPersonPawn.h"
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
		// Check if it's one of our ArchVis pawns
		if (Cast<AArchVisDraftingPawn>(ExistingPawn))
		{
			CurrentPawnType = EArchVisPawnType::Drafting2D;
			UE_LOG(LogArchVisPC, Log, TEXT("Using existing Drafting2D pawn"));
		}
		else if (Cast<AArchVisOrbitPawn>(ExistingPawn))
		{
			CurrentPawnType = EArchVisPawnType::Orbit3D;
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
		}
	}

	// Legacy: Find Camera Controller (for backward compatibility)
	TArray<AActor*> Cameras;
	UGameplayStatics::GetAllActorsOfClass(this, AArchVisCameraController::StaticClass(), Cameras);
	if (Cameras.Num() > 0)
	{
		CameraController = Cast<AArchVisCameraController>(Cameras[0]);
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

		// ============================================
		// VIEW ACTIONS (IMC_2D_Base / IMC_3D_Base)
		// ============================================
		
		if (InputConfig->IA_Pan)
		{
			EIC->BindAction(InputConfig->IA_Pan, ETriggerEvent::Started, this, &AArchVisPlayerController::OnPan);
			EIC->BindAction(InputConfig->IA_Pan, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnPan);
		}
		if (InputConfig->IA_PanDelta)
		{
			EIC->BindAction(InputConfig->IA_PanDelta, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnPanDelta);
		}
		if (InputConfig->IA_Zoom)
		{
			EIC->BindAction(InputConfig->IA_Zoom, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnZoom);
		}
		
		// Fly pawn movement
		if (InputConfig->IA_Move)
		{
			EIC->BindAction(InputConfig->IA_Move, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnMove);
			EIC->BindAction(InputConfig->IA_Move, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnMove);
		}
		if (InputConfig->IA_MoveUp)
		{
			EIC->BindAction(InputConfig->IA_MoveUp, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnMoveUp);
			EIC->BindAction(InputConfig->IA_MoveUp, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnMoveUp);
		}
		if (InputConfig->IA_MoveDown)
		{
			EIC->BindAction(InputConfig->IA_MoveDown, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnMoveDown);
			EIC->BindAction(InputConfig->IA_MoveDown, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnMoveDown);
		}
		if (InputConfig->IA_Look)
		{
			EIC->BindAction(InputConfig->IA_Look, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnLook);
		}
		
		if (InputConfig->IA_Orbit)
		{
			EIC->BindAction(InputConfig->IA_Orbit, ETriggerEvent::Started, this, &AArchVisPlayerController::OnOrbit);
			EIC->BindAction(InputConfig->IA_Orbit, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnOrbit);
		}
		if (InputConfig->IA_OrbitDelta)
		{
			EIC->BindAction(InputConfig->IA_OrbitDelta, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnOrbitDelta);
		}
		if (InputConfig->IA_FlyMode)
		{
			EIC->BindAction(InputConfig->IA_FlyMode, ETriggerEvent::Started, this, &AArchVisPlayerController::OnFlyMode);
			EIC->BindAction(InputConfig->IA_FlyMode, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnFlyMode);
		}
		if (InputConfig->IA_DollyZoom)
		{
			EIC->BindAction(InputConfig->IA_DollyZoom, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnDollyZoom);
		}
		if (InputConfig->IA_ResetView)
		{
			EIC->BindAction(InputConfig->IA_ResetView, ETriggerEvent::Started, this, &AArchVisPlayerController::OnResetView);
		}
		if (InputConfig->IA_FocusSelection)
		{
			EIC->BindAction(InputConfig->IA_FocusSelection, ETriggerEvent::Started, this, &AArchVisPlayerController::OnFocusSelection);
		}
		if (InputConfig->IA_PointerPosition)
		{
			EIC->BindAction(InputConfig->IA_PointerPosition, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnPointerPosition);
		}
		if (InputConfig->IA_SnapToggle)
		{
			EIC->BindAction(InputConfig->IA_SnapToggle, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSnapToggle);
		}
		if (InputConfig->IA_GridToggle)
		{
			EIC->BindAction(InputConfig->IA_GridToggle, ETriggerEvent::Started, this, &AArchVisPlayerController::OnGridToggle);
		}

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
		// 3D NAVIGATION ACTIONS (IMC_3D_Navigation)
		// ============================================
		
		if (InputConfig->IA_ViewTop)
		{
			EIC->BindAction(InputConfig->IA_ViewTop, ETriggerEvent::Started, this, &AArchVisPlayerController::OnViewTop);
		}
		if (InputConfig->IA_ViewFront)
		{
			EIC->BindAction(InputConfig->IA_ViewFront, ETriggerEvent::Started, this, &AArchVisPlayerController::OnViewFront);
		}
		if (InputConfig->IA_ViewRight)
		{
			EIC->BindAction(InputConfig->IA_ViewRight, ETriggerEvent::Started, this, &AArchVisPlayerController::OnViewRight);
		}
		if (InputConfig->IA_ViewPerspective)
		{
			EIC->BindAction(InputConfig->IA_ViewPerspective, ETriggerEvent::Started, this, &AArchVisPlayerController::OnViewPerspective);
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
		// Clear any sticky button/nav flags on mode transition.
		bPanActive = false;
		bOrbitActive = false;
		bLMBDown = false;
		bRMBDown = false;
		bMMBDown = false;
		CurrentMoveInput = FVector2D::ZeroVector;
		CurrentVerticalMoveInput = 0.0f;

		CurrentInteractionMode = NewMode;
		UpdateInputMappingContexts();

		if (bInputDebugEnabled)
		{
			UE_LOG(LogArchVisPC, Log, TEXT("SetInteractionMode -> %s (states cleared)"),
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

	// Get current mouse position
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		FVector2D NewMousePos(MouseX, MouseY);

		// Calculate mouse delta
		FVector2D MouseDelta = NewMousePos - LastMousePosition;

		// Robust 2D pan fallback: if MMB is down and we're in 2D, pan using raw mouse delta.
		// This covers cases where Enhanced Input chorded axis (IA_PanDelta) stops firing due to IMC layering/consumption.
		if (CurrentInteractionMode == EArchVisInteractionMode::Drafting2D && bPanActive && MouseDelta.SizeSquared() > 0.01f)
		{
			if (AArchVisPawnBase* ArchVisPawn = GetArchVisPawn())
			{
				DebugLogInputSnapshot(TEXT("Tick2DPan"), MouseDelta);
				ArchVisPawn->Pan(MouseDelta);
			}
			else if (CameraController)
			{
				DebugLogInputSnapshot(TEXT("Tick2DPan_Legacy"), MouseDelta);
				CameraController->Pan(MouseDelta);
			}
		}

		// IMPORTANT: 3D navigation is handled via Enhanced Input actions (OnPanDelta/OnOrbitDelta/OnLook)
		// to avoid double-applying deltas and to ensure consistent behaviour across click/hold cycles.

		// Update virtual cursor position (for 2D mode crosshair)
		VirtualCursorPos = NewMousePos;
		LastMousePosition = NewMousePos;
	}

	// Handle backspace key repeat
	if (bBackspaceHeld)
	{
		BackspaceHoldTime += DeltaTime;
		
		if (BackspaceHoldTime >= BackspaceNextRepeatTime)
		{
			if (!NumericInputBuffer.Buffer.IsEmpty())
			{
				// Buffer has content - perform numeric backspace
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
			// After initial delay, use faster repeat rate
			BackspaceNextRepeatTime = BackspaceHoldTime + BackspaceRepeatRate;
		}
	}

	// Don't send move events when numeric input is active
	if (NumericInputBuffer.bIsActive && !NumericInputBuffer.Buffer.IsEmpty())
	{
		return;
	}

	// Check if tool still has numeric input active
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
			
			// Send move event
			ToolMgr->ProcessInput(GetPointerEvent(ERTPointerAction::Move));
		}
	}
}

// ============================================
// VIEW HANDLERS
// ============================================

void AArchVisPlayerController::OnPan(const FInputActionValue& Value)
{
	const bool bDown = Value.Get<bool>();

	// In 2D, we use bPanActive to gate panning.
	// In 3D, we want pan to be driven strictly by actual MMB state (bMMBDown), so it never gets "stuck".
	if (CurrentInteractionMode == EArchVisInteractionMode::Drafting2D)
	{
		bPanActive = bDown;
	}

	bMMBDown = bDown;

	if (bInputDebugEnabled)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("OnPan (MMB): %s  Mode=%s  bPanActive=%d  bMMBDown=%d"),
			bDown ? TEXT("Down") : TEXT("Up"),
			(CurrentInteractionMode == EArchVisInteractionMode::Drafting2D) ? TEXT("2D") : TEXT("3D"),
			bPanActive ? 1 : 0,
			bMMBDown ? 1 : 0);
	}
}

void AArchVisPlayerController::OnPanDelta(const FInputActionValue& Value)
{
	const FVector2D Delta = Value.Get<FVector2D>();

	if (bInputDebugEnabled)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("OnPanDelta: Mode=%s  bPanActive=%d  bMMBDown=%d  Delta=%s"),
			(CurrentInteractionMode == EArchVisInteractionMode::Drafting2D) ? TEXT("2D") : TEXT("3D"),
			bPanActive ? 1 : 0,
			bMMBDown ? 1 : 0,
			*Delta.ToString());
	}

	// Gate by explicit action held-state.
	// If IA_Pan is not held, do nothing (prevents any "sticky"/unexpected panning in 3D).
	if (!bMMBDown)
	{
		return;
	}

	if (AArchVisPawnBase* ArchVisPawn = GetArchVisPawn())
	{
		ArchVisPawn->Pan(Delta);
	}
	else if (CameraController)
	{
		CameraController->Pan(Delta);
	}
}

void AArchVisPlayerController::OnZoom(const FInputActionValue& Value)
{
	float Scroll = Value.Get<float>();
	if (Scroll == 0.0f) return;
	
	UE_LOG(LogArchVisPC, Log, TEXT("OnZoom: %f"), Scroll);
	
	// Try new pawn system first
	if (AArchVisPawnBase* ArchVisPawn = GetArchVisPawn())
	{
		UE_LOG(LogArchVisPC, Log, TEXT("OnZoom: Calling Pawn->Zoom on %s"), *ArchVisPawn->GetClass()->GetName());
		ArchVisPawn->Zoom(Scroll);
	}
	else if (CameraController)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("OnZoom: Using CameraController fallback"));
		CameraController->Zoom(Scroll);
	}
	else
	{
		UE_LOG(LogArchVisPC, Warning, TEXT("OnZoom: No pawn or camera controller found!"));
	}
}

void AArchVisPlayerController::OnOrbit(const FInputActionValue& Value)
{
	bOrbitActive = Value.Get<bool>();
	UE_LOG(LogArchVisPC, Log, TEXT("OnOrbit: %s"), bOrbitActive ? TEXT("Started") : TEXT("Ended"));
}

void AArchVisPlayerController::OnOrbitDelta(const FInputActionValue& Value)
{
	if (!bOrbitActive) return;
	
	FVector2D Delta = Value.Get<FVector2D>();
	
	UE_LOG(LogArchVisPC, Log, TEXT("OnOrbitDelta: Delta=%s"), *Delta.ToString());
	
	// Try new pawn system first
	if (AArchVisPawnBase* ArchVisPawn = GetArchVisPawn())
	{
		ArchVisPawn->Orbit(Delta);
	}
}

void AArchVisPlayerController::OnFlyMode(const FInputActionValue& Value)
{
	bool bActive = Value.Get<bool>();
	bRMBDown = bActive;

	UE_LOG(LogArchVisPC, Log, TEXT("OnFlyMode: %s"), bActive ? TEXT("Started") : TEXT("Ended"));
	
	// Route to Orbit pawn for fly mode
	if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
	{
		OrbitPawn->SetFlyModeActive(bActive);
	}
}

void AArchVisPlayerController::OnDollyZoom(const FInputActionValue& Value)
{
	float Amount = Value.Get<float>();
	
	// Route to Orbit pawn for dolly zoom
	if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
	{
		OrbitPawn->DollyZoom(Amount);
	}
}

void AArchVisPlayerController::OnResetView(const FInputActionValue& Value)
{
	// Try new pawn system first
	if (AArchVisPawnBase* ArchVisPawn = GetArchVisPawn())
	{
		ArchVisPawn->ResetView();
	}
	else if (CameraController)
	{
		// Fallback to legacy camera controller
		CameraController->ResetView();
	}
}

void AArchVisPlayerController::OnMove(const FInputActionValue& Value)
{
	FVector2D Input = Value.Get<FVector2D>();
	
	// Store the input for continuous movement
	CurrentMoveInput = Input;
	
	static int32 MoveLogCount = 0;
	if (MoveLogCount < 5)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("OnMove: Input=%s"), *Input.ToString());
		MoveLogCount++;
	}
	
	// Route to Orbit pawn (only when fly mode is active via RMB)
	if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
	{
		if (OrbitPawn->IsFlyModeActive())
		{
			OrbitPawn->SetMovementInput(Input);
		}
	}
	// Route to Fly pawn (always active)
	else if (AArchVisFlyPawn* FlyPawn = Cast<AArchVisFlyPawn>(GetPawn()))
	{
		FlyPawn->SetMovementInput(Input);
	}
	// For first/third person, use AddMovementInput
	else if (AArchVisFirstPersonPawn* FPPawn = Cast<AArchVisFirstPersonPawn>(GetPawn()))
	{
		FPPawn->Move(Input);
	}
	else if (AArchVisThirdPersonPawn* TPPawn = Cast<AArchVisThirdPersonPawn>(GetPawn()))
	{
		TPPawn->Move(Input);
	}
}

void AArchVisPlayerController::OnMoveUp(const FInputActionValue& Value)
{
	float Input = Value.Get<float>();
	CurrentVerticalMoveInput = Input;
	
	// Route to Orbit pawn (only when fly mode active)
	if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
	{
		if (OrbitPawn->IsFlyModeActive())
		{
			OrbitPawn->SetVerticalInput(Input);
		}
	}
	else if (AArchVisFlyPawn* FlyPawn = Cast<AArchVisFlyPawn>(GetPawn()))
	{
		FlyPawn->SetVerticalInput(Input);
	}
}

void AArchVisPlayerController::OnMoveDown(const FInputActionValue& Value)
{
	float Input = Value.Get<float>();
	CurrentVerticalMoveInput = -Input;
	
	// Route to Orbit pawn (only when fly mode active)
	if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
	{
		if (OrbitPawn->IsFlyModeActive())
		{
			OrbitPawn->SetVerticalInput(-Input);
		}
	}
	else if (AArchVisFlyPawn* FlyPawn = Cast<AArchVisFlyPawn>(GetPawn()))
	{
		FlyPawn->SetVerticalInput(-Input);
	}
}

void AArchVisPlayerController::OnLook(const FInputActionValue& Value)
{
	FVector2D Delta = Value.Get<FVector2D>();
	
	static int32 LookLogCount = 0;
	if (LookLogCount < 5)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("OnLook: Delta=%s"), *Delta.ToString());
		LookLogCount++;
	}
	
	// Route to Orbit pawn (when in fly mode)
	if (AArchVisOrbitPawn* OrbitPawn = Cast<AArchVisOrbitPawn>(GetPawn()))
	{
		if (OrbitPawn->IsFlyModeActive())
		{
			OrbitPawn->Look(Delta);
		}
	}
	// Route to Fly pawn
	else if (AArchVisFlyPawn* FlyPawn = Cast<AArchVisFlyPawn>(GetPawn()))
	{
		FlyPawn->Look(Delta);
	}
	// For first/third person pawns
	else if (AArchVisFirstPersonPawn* FPPawn = Cast<AArchVisFirstPersonPawn>(GetPawn()))
	{
		FPPawn->Look(Delta);
	}
	else if (AArchVisThirdPersonPawn* TPPawn = Cast<AArchVisThirdPersonPawn>(GetPawn()))
	{
		TPPawn->Look(Delta);
	}
}

void AArchVisPlayerController::OnFocusSelection(const FInputActionValue& Value)
{
	// Focus on selection or on point under cursor
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			// Get selection bounds
			// For now, focus on the point under the cursor
			FVector WorldLoc, WorldDir;
			if (DeprojectScreenPositionToWorld(VirtualCursorPos.X, VirtualCursorPos.Y, WorldLoc, WorldDir))
			{
				// Trace to ground plane
				float T = -WorldLoc.Z / WorldDir.Z;
				if (T > 0)
				{
					FVector GroundPoint = WorldLoc + WorldDir * T;
					
					if (AArchVisPawnBase* ArchVisPawn = GetArchVisPawn())
					{
						ArchVisPawn->FocusOnLocation(GroundPoint);
						UE_LOG(LogArchVisPC, Log, TEXT("Focus on location: %s"), *GroundPoint.ToString());
					}
				}
			}
		}
	}
}

void AArchVisPlayerController::OnPointerPosition(const FInputActionValue& Value)
{
	const FVector2D RawDelta = Value.Get<FVector2D>();
	const FVector2D ScaledDelta = RawDelta * MouseSensitivity;

	// Debug: Log first few pointer position updates
	static int32 PointerLogCount = 0;
	if (PointerLogCount < 5)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("OnPointerPosition: RawDelta=%s, Scaled=%s"), *RawDelta.ToString(), *ScaledDelta.ToString());
		PointerLogCount++;
	}

	// Update Virtual Cursor
	VirtualCursorPos.X += ScaledDelta.X;
	VirtualCursorPos.Y -= ScaledDelta.Y;

	// Clamp to Viewport
	int32 SizeX, SizeY;
	GetViewportSize(SizeX, SizeY);
	VirtualCursorPos.X = FMath::Clamp(VirtualCursorPos.X, 0.0f, (float)SizeX);
	VirtualCursorPos.Y = FMath::Clamp(VirtualCursorPos.Y, 0.0f, (float)SizeY);

	// Clear numeric input if mouse moves significantly
	if (NumericInputBuffer.bIsActive && !NumericInputBuffer.Buffer.IsEmpty())
	{
		if (RawDelta.Size() > 1.0f)
		{
			NumericInputBuffer.Clear();
			// Don't deactivate numeric entry context for drawing tools
			// so user can continue entering digits after moving the mouse
			
			if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
			{
				if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
				{
					if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
					{
						LineTool->ClearNumericInputOverride();
					}
					ToolMgr->ProcessInput(GetPointerEvent(ERTPointerAction::Move));
				}
			}
		}
	}
}

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
	bLMBDown = true;
	
	// Don't process selection if Alt is held (we're orbiting)
	if (bAltDown)
	{
		return;
	}
	
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			URTPlanToolBase* ActiveTool = ToolMgr->GetActiveTool();
			UE_LOG(LogArchVisPC, Log, TEXT("OnSelectStarted: ActiveTool=%s, ToolType=%d"), 
				ActiveTool ? *ActiveTool->GetClass()->GetName() : TEXT("NULL"),
				static_cast<int32>(ToolMgr->GetActiveToolType()));
			
			ToolMgr->ProcessInput(GetPointerEvent(ERTPointerAction::Down));
		}
	}
}

void AArchVisPlayerController::OnSelectCompleted(const FInputActionValue& Value)
{
	bLMBDown = false;
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
	UE_LOG(LogArchVisPC, Log, TEXT("Alt Started - bAltDown=true"));
}

void AArchVisPlayerController::OnModifierAltCompleted(const FInputActionValue& Value)
{
	bAltDown = false;
	UE_LOG(LogArchVisPC, Log, TEXT("Alt Ended - bAltDown=false"));
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

	// Clear any sticky input state before switching pawns.
	bPanActive = false;
	bOrbitActive = false;
	bLMBDown = false;
	bRMBDown = false;
	bMMBDown = false;
	CurrentMoveInput = FVector2D::ZeroVector;
	CurrentVerticalMoveInput = 0.0f;

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
}

void AArchVisPlayerController::ArchVisSetInputDebug(int32 bEnabled)
{
	bInputDebugEnabled = (bEnabled != 0);
	UE_LOG(LogArchVisPC, Log, TEXT("Input debug: %s"), bInputDebugEnabled ? TEXT("ON") : TEXT("OFF"));
}

AArchVisPlayerController::FNav3DState AArchVisPlayerController::GetNav3DState() const
{
	FNav3DState S;
	S.bPan = bMMBDown;
	S.bOrbit = bAltDown && bLMBDown;
	S.bFly = bRMBDown;
	S.bDolly = bAltDown && bRMBDown;
	return S;
}

void AArchVisPlayerController::DebugLogInputSnapshot(const TCHAR* Context, const FVector2D& MouseDelta) const
{
	if (!bInputDebugEnabled)
	{
		return;
	}

	UE_LOG(LogArchVisPC, Log, TEXT("[%s] Mode=%s Pawn=%s Alt=%d Ctrl=%d Shift=%d LMB=%d MMB=%d RMB=%d PanActive=%d OrbitActive=%d MouseDelta=%s"),
		Context,
		(CurrentInteractionMode == EArchVisInteractionMode::Drafting2D) ? TEXT("2D") : TEXT("3D"),
		GetPawn() ? *GetPawn()->GetClass()->GetName() : TEXT("NULL"),
		bAltDown ? 1 : 0,
		bCtrlDown ? 1 : 0,
		bShiftDown ? 1 : 0,
		bLMBDown ? 1 : 0,
		bMMBDown ? 1 : 0,
		bRMBDown ? 1 : 0,
		bPanActive ? 1 : 0,
		bOrbitActive ? 1 : 0,
		*MouseDelta.ToString());
}

void AArchVisPlayerController::DebugLogNavState(const TCHAR* Context, const FVector2D& MouseDelta) const
{
	if (!bInputDebugEnabled)
	{
		return;
	}

	const FNav3DState Nav = GetNav3DState();
	UE_LOG(LogArchVisPC, Log, TEXT("[%s] Nav3D: Pan=%d Orbit=%d Fly=%d Dolly=%d MouseDelta=%s"),
		Context,
		Nav.bPan ? 1 : 0,
		Nav.bOrbit ? 1 : 0,
		Nav.bFly ? 1 : 0,
		Nav.bDolly ? 1 : 0,
		*MouseDelta.ToString());
}

