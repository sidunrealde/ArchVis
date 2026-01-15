#include "ArchVisPlayerController.h"
#include "ArchVisGameMode.h"
#include "ArchVisCameraController.h"
#include "ArchVisInputConfig.h"
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

	// Initialize numeric buffer with default unit
	NumericInputBuffer.CurrentUnit = DefaultUnit;

	// Find or Spawn Camera Controller
	TArray<AActor*> Cameras;
	UGameplayStatics::GetAllActorsOfClass(this, AArchVisCameraController::StaticClass(), Cameras);
	if (Cameras.Num() > 0)
	{
		CameraController = Cast<AArchVisCameraController>(Cameras[0]);
	}
	else
	{
		CameraController = GetWorld()->SpawnActor<AArchVisCameraController>(AArchVisCameraController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}

	// Set up initial Input Mapping Contexts
	UpdateInputMappingContexts();
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
		if (InputConfig->IA_Orbit)
		{
			EIC->BindAction(InputConfig->IA_Orbit, ETriggerEvent::Started, this, &AArchVisPlayerController::OnOrbit);
			EIC->BindAction(InputConfig->IA_Orbit, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnOrbit);
		}
		if (InputConfig->IA_OrbitDelta)
		{
			EIC->BindAction(InputConfig->IA_OrbitDelta, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnOrbitDelta);
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
			EIC->BindAction(InputConfig->IA_Select, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSelect);
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
	if (!Context) return;
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (!Subsystem->HasMappingContext(Context))
		{
			Subsystem->AddMappingContext(Context, Priority);
			UE_LOG(LogArchVisPC, Log, TEXT("Added IMC: %s (Priority: %d)"), *Context->GetName(), Priority);
		}
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
	if (!InputConfig) return;

	// Always add Global context (Priority 0)
	AddMappingContext(InputConfig->IMC_Global, IMC_Priority_Global);

	// Remove old mode base context
	RemoveMappingContext(ActiveModeBaseIMC);
	ClearAllToolContexts();
	
	if (CurrentInteractionMode == EArchVisInteractionMode::Drafting2D)
	{
		// Add 2D Base context (Priority 1)
		ActiveModeBaseIMC = InputConfig->IMC_2D_Base;
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

	// Remove current tool context
	ClearAllToolContexts();
	Current2DToolMode = NewToolMode;

	switch (NewToolMode)
	{
	case EArchVis2DToolMode::Selection:
		ActiveToolIMC = InputConfig->IMC_2D_Selection;
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
		CurrentInteractionMode = NewMode;
		UpdateInputMappingContexts();
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
	bPanActive = Value.Get<bool>();
}

void AArchVisPlayerController::OnPanDelta(const FInputActionValue& Value)
{
	if (!bPanActive) return;
	
	FVector2D Delta = Value.Get<FVector2D>();
	if (CameraController)
	{
		CameraController->Pan(Delta);
	}
}

void AArchVisPlayerController::OnZoom(const FInputActionValue& Value)
{
	float Scroll = Value.Get<float>();
	if (CameraController && Scroll != 0.0f)
	{
		CameraController->Zoom(Scroll);
	}
}

void AArchVisPlayerController::OnOrbit(const FInputActionValue& Value)
{
	bOrbitActive = Value.Get<bool>();
}

void AArchVisPlayerController::OnOrbitDelta(const FInputActionValue& Value)
{
	if (!bOrbitActive) return;
	
	FVector2D Delta = Value.Get<FVector2D>();
	// TODO: Implement orbit in camera controller
}

void AArchVisPlayerController::OnResetView(const FInputActionValue& Value)
{
	if (CameraController)
	{
		CameraController->ResetView();
	}
}

void AArchVisPlayerController::OnFocusSelection(const FInputActionValue& Value)
{
	// TODO: Focus camera on selected objects
}

void AArchVisPlayerController::OnPointerPosition(const FInputActionValue& Value)
{
	const FVector2D RawDelta = Value.Get<FVector2D>();
	const FVector2D ScaledDelta = RawDelta * MouseSensitivity;

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
	if (CameraController)
	{
		CameraController->ToggleViewMode();
		
		SetInteractionMode(CurrentInteractionMode == EArchVisInteractionMode::Drafting2D 
			? EArchVisInteractionMode::Navigation3D 
			: EArchVisInteractionMode::Drafting2D);
	}
}

// ============================================
// SELECTION HANDLERS
// ============================================

void AArchVisPlayerController::OnSelect(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->ProcessInput(GetPointerEvent(ERTPointerAction::Down));
		}
	}
}

void AArchVisPlayerController::OnSelectAdd(const FInputActionValue& Value)
{
	// Shift+Click - handled via modifier flags in pointer event
	OnSelect(Value);
}

void AArchVisPlayerController::OnSelectToggle(const FInputActionValue& Value)
{
	// Ctrl+Click - handled via modifier flags in pointer event
	OnSelect(Value);
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
	// TODO: Implement undo functionality
	UE_LOG(LogTemp, Log, TEXT("Undo triggered"));
}

void AArchVisPlayerController::OnRedo(const FInputActionValue& Value)
{
	// TODO: Implement redo functionality
	UE_LOG(LogTemp, Log, TEXT("Redo triggered"));
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
}

void AArchVisPlayerController::OnModifierAltCompleted(const FInputActionValue& Value)
{
	bAltDown = false;
}

