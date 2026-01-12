#include "ArchVisPlayerController.h"
#include "ArchVisGameMode.h"
#include "ArchVisCameraController.h"
#include "ArchVisInputConfig.h"
#include "RTPlanToolManager.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameUserSettings.h"

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
		}
	}
}

void AArchVisPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
}

void AArchVisPlayerController::OnMouseMove(const FInputActionValue& Value)
{
	FVector2D RawDelta = Value.Get<FVector2D>();

	// Update Virtual Cursor Position
	// The Y-axis from mouse input is inverted relative to screen space (mouse up is positive).
	// We add the delta directly as screen space Y increases downwards.
	VirtualCursorPos += RawDelta;

	// Clamp to Viewport
	int32 SizeX, SizeY;
	GetViewportSize(SizeX, SizeY);
	VirtualCursorPos.X = FMath::Clamp(VirtualCursorPos.X, 0.0f, (float)SizeX);
	VirtualCursorPos.Y = FMath::Clamp(VirtualCursorPos.Y, 0.0f, (float)SizeY);

	// Handle Camera Pan (Right Drag)
	if (bRightMouseDown && CameraController)
	{
		// Pass the raw delta, the camera controller will interpret it.
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

FRTPointerEvent AArchVisPlayerController::GetPointerEvent(ERTPointerAction Action) const
{
	FRTPointerEvent Event;
	Event.Source = ERTInputSource::Mouse;
	Event.Action = Action;
	Event.ScreenPosition = VirtualCursorPos;

	// Deproject from Virtual Cursor Position
	FVector WorldLoc, WorldDir;
	if (DeprojectScreenPositionToWorld(VirtualCursorPos.X, VirtualCursorPos.Y, WorldLoc, WorldDir))
	{
		Event.WorldOrigin = WorldLoc;
		Event.WorldDirection = WorldDir;
	}

	return Event;
}
