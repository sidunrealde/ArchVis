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
	// Hide OS Cursor
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

	// Add Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputConfig && InputConfig->DefaultMappingContext)
		{
			Subsystem->AddMappingContext(InputConfig->DefaultMappingContext, 0);
		}
	}

	// Find or Spawn Camera Controller
	TArray<AActor*> Cameras;
	UGameplayStatics::GetAllActorsOfClass(this, AArchVisCameraController::StaticClass(), Cameras);
	if (Cameras.Num() > 0)
	{
		CameraController = Cast<AArchVisCameraController>(Cameras[0]);
	}
	else
	{
		FVector Loc = FVector(0, 0, 0);
		FRotator Rot = FRotator::ZeroRotator;
		CameraController = GetWorld()->SpawnActor<AArchVisCameraController>(AArchVisCameraController::StaticClass(), Loc, Rot);
	}
}

void AArchVisPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (InputConfig)
		{
			// --- Mouse/View Actions ---
			if (InputConfig->IA_LeftClick)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_LeftClick, ETriggerEvent::Started, this, &AArchVisPlayerController::OnLeftClick);
				EnhancedInputComponent->BindAction(InputConfig->IA_LeftClick, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnLeftClick);
			}
			if (InputConfig->IA_RightClick)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_RightClick, ETriggerEvent::Started, this, &AArchVisPlayerController::OnRightClick);
				EnhancedInputComponent->BindAction(InputConfig->IA_RightClick, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnRightClick);
			}
			if (InputConfig->IA_MouseMove)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_MouseMove, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnMouseMove);
			}
			if (InputConfig->IA_MouseWheel)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_MouseWheel, ETriggerEvent::Triggered, this, &AArchVisPlayerController::OnMouseWheel);
			}
			if (InputConfig->IA_ToggleView)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_ToggleView, ETriggerEvent::Started, this, &AArchVisPlayerController::OnToggleView);
			}

			// --- Numeric Entry Actions ---
			if (InputConfig->IA_NumericDigit)
			{
				// Scalar values are 1-10 (10=digit0) to avoid 0.0 threshold issue
				EnhancedInputComponent->BindAction(InputConfig->IA_NumericDigit, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericDigit);
			}
			if (InputConfig->IA_NumericDecimal)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_NumericDecimal, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericDecimal);
			}
			if (InputConfig->IA_NumericBackspace)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_NumericBackspace, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericBackspace);
			}
			if (InputConfig->IA_NumericCommit)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_NumericCommit, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericCommit);
			}
			if (InputConfig->IA_NumericClear)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_NumericClear, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericClear);
			}
			if (InputConfig->IA_NumericSwitchField)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_NumericSwitchField, ETriggerEvent::Started, this, &AArchVisPlayerController::OnNumericSwitchField);
			}
			if (InputConfig->IA_CycleUnits)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_CycleUnits, ETriggerEvent::Started, this, &AArchVisPlayerController::OnCycleUnits);
			}

			// --- Snap Modifier ---
			if (InputConfig->IA_SnapModifier)
			{
				EnhancedInputComponent->BindAction(InputConfig->IA_SnapModifier, ETriggerEvent::Started, this, &AArchVisPlayerController::OnSnapModifierStarted);
				EnhancedInputComponent->BindAction(InputConfig->IA_SnapModifier, ETriggerEvent::Completed, this, &AArchVisPlayerController::OnSnapModifierCompleted);
			}
		}
	}
}

void AArchVisPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Don't send move events when numeric input is active - let the typed values control the cursor
	// Check both the buffer state AND the tool's numeric input flag
	if (NumericInputBuffer.bIsActive)
	{
		// If buffer has content, definitely don't send move events
		if (!NumericInputBuffer.Buffer.IsEmpty())
		{
			return;
		}
		
		// Buffer is empty but bIsActive is true (e.g., just switched fields)
		// Check if the tool still has numeric input active
		if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
		{
			if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
			{
				if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
				{
					if (LineTool->IsNumericInputActive())
					{
						return; // Tool still has numeric position set, don't override
					}
				}
			}
		}
	}

	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->ProcessInput(GetPointerEvent(ERTPointerAction::Move));
		}
	}
}

void AArchVisPlayerController::OnLeftClick(const FInputActionValue& Value)
{
	bool bPressed = Value.Get<bool>();
	bLeftMouseDown = bPressed;

	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->ProcessInput(GetPointerEvent(bPressed ? ERTPointerAction::Down : ERTPointerAction::Up));
		}
	}
}

void AArchVisPlayerController::OnRightClick(const FInputActionValue& Value)
{
	bRightMouseDown = Value.Get<bool>();
	
	// If releasing right click and we have an active buffer, clear it
	if (!bRightMouseDown && NumericInputBuffer.bIsActive)
	{
		NumericInputBuffer.Clear();
	}
}

void AArchVisPlayerController::OnMouseMove(const FInputActionValue& Value)
{
	const FVector2D RawDelta = Value.Get<FVector2D>();
	const FVector2D ScaledDelta = RawDelta * MouseSensitivity;

	// Update Virtual Cursor Position
	// Enhanced Input mouse delta uses +Y for moving the mouse up.
	// Screen space uses +Y for moving down, so invert Y to match the actual cursor movement.
	VirtualCursorPos.X += ScaledDelta.X;
	VirtualCursorPos.Y -= ScaledDelta.Y;

	// Clamp to Viewport
	int32 SizeX, SizeY;
	GetViewportSize(SizeX, SizeY);
	VirtualCursorPos.X = FMath::Clamp(VirtualCursorPos.X, 0.0f, (float)SizeX);
	VirtualCursorPos.Y = FMath::Clamp(VirtualCursorPos.Y, 0.0f, (float)SizeY);

	// If mouse moved significantly and numeric input is active, clear it and resume mouse control
	if (NumericInputBuffer.bIsActive && !NumericInputBuffer.Buffer.IsEmpty())
	{
		float MoveDist = RawDelta.Size();
		if (MoveDist > 1.0f) // Threshold to ignore micro-movements
		{
			// Clear numeric input and resume mouse control
			NumericInputBuffer.Clear();
			
			// Clear the tool's numeric input flag
			if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
			{
				if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
				{
					if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
					{
						LineTool->ClearNumericInputOverride();
					}
					
					// Send a move event immediately so cursor updates
					ToolMgr->ProcessInput(GetPointerEvent(ERTPointerAction::Move));
				}
			}
		}
	}

	// Handle Camera Pan (Right Drag)
	if (bRightMouseDown && CameraController)
	{
		// Note: camera panning sensitivity remains in the camera controller (PanSpeed).
		CameraController->Pan(RawDelta);
	}
}

void AArchVisPlayerController::OnMouseWheel(const FInputActionValue& Value)
{
	float Scroll = Value.Get<float>();
	if (CameraController && Scroll != 0.0f)
	{
		CameraController->Zoom(Scroll);
	}
}

void AArchVisPlayerController::OnToggleView(const FInputActionValue& Value)
{
	if (CameraController)
	{
		CameraController->ToggleViewMode();
	}
}

// --- Numeric Entry Handlers ---

void AArchVisPlayerController::OnNumericDigit(const FInputActionValue& Value)
{
	// The value should be 1.0-10.0 based on which digit key was pressed
	// (configured via Scalar modifier in the Input Mapping Context)
	// Values: 1=digit1, 2=digit2, ... 9=digit9, 10=digit0
	// This offset avoids the issue where 0.0 doesn't trigger Started events
	float DigitValue = Value.Get<float>();
	int32 Digit = FMath::RoundToInt(DigitValue);
	
	// Map 10 back to 0
	if (Digit == 10)
	{
		Digit = 0;
	}
	
	Digit = FMath::Clamp(Digit, 0, 9);
	
	TCHAR DigitChar = TEXT('0') + Digit;
	NumericInputBuffer.AppendChar(DigitChar);
	
	UE_LOG(LogArchVisPC, Log, TEXT("Numeric Digit: %d, Buffer: %s"), Digit, *NumericInputBuffer.Buffer);
	
	// Update tool preview in real-time
	UpdateToolPreviewFromBuffer();
}

void AArchVisPlayerController::OnNumericDecimal(const FInputActionValue& Value)
{
	NumericInputBuffer.AppendChar(TEXT('.'));
	UE_LOG(LogArchVisPC, Verbose, TEXT("Numeric Decimal, Buffer: %s"), *NumericInputBuffer.Buffer);
	
	// Update tool preview in real-time
	UpdateToolPreviewFromBuffer();
}

void AArchVisPlayerController::OnNumericBackspace(const FInputActionValue& Value)
{
	NumericInputBuffer.Backspace();
	UE_LOG(LogArchVisPC, Verbose, TEXT("Numeric Backspace, Buffer: %s"), *NumericInputBuffer.Buffer);
	
	// If buffer is now empty, clear the tool's numeric input flag
	if (NumericInputBuffer.Buffer.IsEmpty())
	{
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
	else
	{
		// Update tool preview in real-time
		UpdateToolPreviewFromBuffer();
	}
}

void AArchVisPlayerController::OnNumericCommit(const FInputActionValue& Value)
{
	CommitNumericInput();
}

void AArchVisPlayerController::OnNumericClear(const FInputActionValue& Value)
{
	NumericInputBuffer.Clear();
	
	// Clear the tool's numeric input flag so mouse control resumes
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
	
	UE_LOG(LogArchVisPC, Log, TEXT("Numeric Input Cleared"));
}

void AArchVisPlayerController::OnNumericSwitchField(const FInputActionValue& Value)
{
	// Apply current buffer value to the tool before switching fields
	// This preserves the typed value when switching
	if (!NumericInputBuffer.Buffer.IsEmpty())
	{
		UpdateToolPreviewFromBuffer();
	}
	
	// Toggle between Length and Angle
	if (NumericInputBuffer.ActiveField == ERTNumericField::Length)
	{
		NumericInputBuffer.ActiveField = ERTNumericField::Angle;
	}
	else
	{
		NumericInputBuffer.ActiveField = ERTNumericField::Length;
	}
	
	// Clear only the buffer text, keep bIsActive true so we can type in the new field
	// IMPORTANT: Don't clear the tool's bNumericInputActive flag - we want to keep the position
	NumericInputBuffer.Buffer.Empty();
	
	// Mark that we're still in numeric input mode even with empty buffer
	// This is handled by checking bIsActive, not buffer content
	NumericInputBuffer.bIsActive = true;
	
	UE_LOG(LogArchVisPC, Log, TEXT("Switched to field: %s"), 
		NumericInputBuffer.ActiveField == ERTNumericField::Length ? TEXT("Length") : TEXT("Angle"));
}

void AArchVisPlayerController::OnCycleUnits(const FInputActionValue& Value)
{
	NumericInputBuffer.CycleUnit();
	UE_LOG(LogArchVisPC, Log, TEXT("Unit changed to: %s"), *NumericInputBuffer.GetUnitSuffix());
}

void AArchVisPlayerController::UpdateToolPreviewFromBuffer()
{
	if (!NumericInputBuffer.bIsActive || NumericInputBuffer.Buffer.IsEmpty())
	{
		return;
	}

	// Update the active tool's preview
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
			{
				if (NumericInputBuffer.ActiveField == ERTNumericField::Length)
				{
					// Convert length to cm based on current unit
					float LengthValue = NumericInputBuffer.GetValueInCm();
					if (LengthValue >= 0.1f)
					{
						LineTool->UpdatePreviewFromLength(LengthValue);
					}
				}
				else
				{
					// For angle, use the raw value in degrees (no conversion)
					float AngleValue = FCString::Atof(*NumericInputBuffer.Buffer);
					// Angle is valid even if small (e.g., 1 degree)
					LineTool->UpdatePreviewFromAngle(AngleValue);
				}
			}
		}
	}
}

void AArchVisPlayerController::CommitNumericInput()
{
	if (!NumericInputBuffer.bIsActive || NumericInputBuffer.Buffer.IsEmpty())
	{
		return;
	}

	float Value;
	if (NumericInputBuffer.ActiveField == ERTNumericField::Length)
	{
		// Convert length to cm based on current unit
		Value = NumericInputBuffer.GetValueInCm();
	}
	else
	{
		// Angle is in degrees, no conversion needed
		Value = FCString::Atof(*NumericInputBuffer.Buffer);
		// Clamp angle to 0-180
		Value = FMath::Clamp(Value, 0.0f, 180.0f);
	}
	
	UE_LOG(LogArchVisPC, Log, TEXT("Committing numeric input: %s (Field: %s, Value: %.2f)"),
		*NumericInputBuffer.Buffer,
		NumericInputBuffer.ActiveField == ERTNumericField::Length ? TEXT("Length") : TEXT("Angle"),
		Value);

	// Send to active tool with field type
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

	// Clear buffer after commit
	NumericInputBuffer.Clear();
}

FRTPointerEvent AArchVisPlayerController::GetPointerEvent(ERTPointerAction Action) const
{
	FRTPointerEvent Event;
	Event.Source = ERTInputSource::Mouse;
	Event.Action = Action;
	Event.ScreenPosition = VirtualCursorPos;
	Event.bSnapEnabled = IsSnapEnabled();

	// Deproject from Virtual Cursor Position
	FVector WorldLoc, WorldDir;
	if (DeprojectScreenPositionToWorld(VirtualCursorPos.X, VirtualCursorPos.Y, WorldLoc, WorldDir))
	{
		Event.WorldOrigin = WorldLoc;
		Event.WorldDirection = WorldDir;
	}

	return Event;
}

// --- Snap Modifier ---

bool AArchVisPlayerController::IsSnapEnabled() const
{
	switch (SnapModifierMode)
	{
	case ESnapModifierMode::Toggle:
		return bSnapToggledOn;
		
	case ESnapModifierMode::HoldToDisable:
		// Snap is on by default, holding modifier disables it
		return bSnapEnabledByDefault && !bSnapModifierHeld;
		
	case ESnapModifierMode::HoldToEnable:
		// Snap is off by default, holding modifier enables it
		return !bSnapEnabledByDefault || bSnapModifierHeld;
		
	default:
		return bSnapEnabledByDefault;
	}
}

void AArchVisPlayerController::OnSnapModifierStarted(const FInputActionValue& Value)
{
	bSnapModifierHeld = true;
	
	// For Toggle mode, flip the toggle state
	if (SnapModifierMode == ESnapModifierMode::Toggle)
	{
		bSnapToggledOn = !bSnapToggledOn;
		UE_LOG(LogArchVisPC, Log, TEXT("Snap toggled: %s"), bSnapToggledOn ? TEXT("ON") : TEXT("OFF"));
	}
	else
	{
		UE_LOG(LogArchVisPC, Log, TEXT("Snap modifier held, snap now: %s"), IsSnapEnabled() ? TEXT("ON") : TEXT("OFF"));
	}
}

void AArchVisPlayerController::OnSnapModifierCompleted(const FInputActionValue& Value)
{
	bSnapModifierHeld = false;
	
	if (SnapModifierMode != ESnapModifierMode::Toggle)
	{
		UE_LOG(LogArchVisPC, Log, TEXT("Snap modifier released, snap now: %s"), IsSnapEnabled() ? TEXT("ON") : TEXT("OFF"));
	}
}

