// ToolInputComponent.cpp
// Input component for tool interactions - attached to PlayerController

#include "Input/ToolInputComponent.h"
#include "ArchVisPlayerController.h"
#include "ArchVisInputConfig.h"
#include "ArchVisGameMode.h"
#include "RTPlanToolManager.h"
#include "RTPlanToolBase.h"
#include "Tools/RTPlanLineTool.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogToolInput, Log, All);

UToolInputComponent::UToolInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UToolInputComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UToolInputComponent::Initialize(UArchVisInputConfig* InInputConfig, UEnhancedInputComponent* InputComponent)
{
	if (bIsInitialized)
	{
		UE_LOG(LogToolInput, Warning, TEXT("ToolInputComponent::Initialize called but already initialized"));
		return;
	}

	InputConfig = InInputConfig;
	CachedInputComponent = InputComponent;

	if (!InputConfig || !InputComponent)
	{
		UE_LOG(LogToolInput, Error, TEXT("ToolInputComponent::Initialize - InputConfig or InputComponent is null!"));
		return;
	}

	// Bind tool switching
	if (InputConfig->IA_ToolSelect)
	{
		InputComponent->BindAction(InputConfig->IA_ToolSelect, ETriggerEvent::Triggered, this, &UToolInputComponent::OnToolSelect);
	}
	if (InputConfig->IA_ToolLine)
	{
		InputComponent->BindAction(InputConfig->IA_ToolLine, ETriggerEvent::Triggered, this, &UToolInputComponent::OnToolLine);
	}
	if (InputConfig->IA_ToolPolyline)
	{
		InputComponent->BindAction(InputConfig->IA_ToolPolyline, ETriggerEvent::Triggered, this, &UToolInputComponent::OnToolPolyline);
	}

	// Bind drawing actions
	if (InputConfig->IA_DrawPlacePoint)
	{
		InputComponent->BindAction(InputConfig->IA_DrawPlacePoint, ETriggerEvent::Triggered, this, &UToolInputComponent::OnDrawPlacePoint);
	}
	if (InputConfig->IA_DrawConfirm)
	{
		InputComponent->BindAction(InputConfig->IA_DrawConfirm, ETriggerEvent::Triggered, this, &UToolInputComponent::OnDrawConfirm);
	}
	if (InputConfig->IA_DrawCancel)
	{
		InputComponent->BindAction(InputConfig->IA_DrawCancel, ETriggerEvent::Triggered, this, &UToolInputComponent::OnDrawCancel);
	}
	if (InputConfig->IA_DrawClose)
	{
		InputComponent->BindAction(InputConfig->IA_DrawClose, ETriggerEvent::Triggered, this, &UToolInputComponent::OnDrawClose);
	}
	if (InputConfig->IA_DrawRemoveLastPoint)
	{
		InputComponent->BindAction(InputConfig->IA_DrawRemoveLastPoint, ETriggerEvent::Triggered, this, &UToolInputComponent::OnDrawRemoveLastPoint);
	}
	if (InputConfig->IA_OrthoLock)
	{
		InputComponent->BindAction(InputConfig->IA_OrthoLock, ETriggerEvent::Started, this, &UToolInputComponent::OnOrthoLock);
		InputComponent->BindAction(InputConfig->IA_OrthoLock, ETriggerEvent::Completed, this, &UToolInputComponent::OnOrthoLockReleased);
	}
	if (InputConfig->IA_AngleSnap)
	{
		InputComponent->BindAction(InputConfig->IA_AngleSnap, ETriggerEvent::Triggered, this, &UToolInputComponent::OnAngleSnap);
	}

	// Bind selection actions
	if (InputConfig->IA_Select)
	{
		InputComponent->BindAction(InputConfig->IA_Select, ETriggerEvent::Started, this, &UToolInputComponent::OnSelect);
		InputComponent->BindAction(InputConfig->IA_Select, ETriggerEvent::Completed, this, &UToolInputComponent::OnSelectCompleted);
	}

	bIsInitialized = true;

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("ToolInputComponent::Initialize complete"));
	}
}

void UToolInputComponent::Cleanup()
{
	if (!bIsInitialized)
	{
		return;
	}

	if (ActiveToolIMC)
	{
		RemoveMappingContext(ActiveToolIMC);
		ActiveToolIMC = nullptr;
	}

	bIsInitialized = false;

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("ToolInputComponent::Cleanup complete"));
	}
}

void UToolInputComponent::SetActiveToolContext(UInputMappingContext* ToolIMC)
{
	// Remove old tool IMC
	if (ActiveToolIMC && ActiveToolIMC != ToolIMC)
	{
		RemoveMappingContext(ActiveToolIMC);
	}

	// Add new tool IMC
	if (ToolIMC && ToolIMC != ActiveToolIMC)
	{
		AddMappingContext(ToolIMC, IMC_Priority_Tool);
	}

	ActiveToolIMC = ToolIMC;

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("SetActiveToolContext: %s"), 
			ToolIMC ? *ToolIMC->GetName() : TEXT("None"));
	}
}

// --- Selection Handlers ---

void UToolInputComponent::OnSelect(const FInputActionValue& Value)
{
	AArchVisPlayerController* PC = GetArchVisController();
	if (!PC) return;

	// Route to tool manager
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			FRTPointerEvent Event;
			// TODO: Populate event from PC
			ToolMgr->ProcessInput(Event);
		}
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnSelect"));
	}
}

void UToolInputComponent::OnSelectCompleted(const FInputActionValue& Value)
{
	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnSelectCompleted"));
	}
}

void UToolInputComponent::OnBoxSelectStart(const FInputActionValue& Value)
{
	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnBoxSelectStart"));
	}
}

void UToolInputComponent::OnBoxSelectDrag(const FInputActionValue& Value)
{
	// TODO: Update box selection rectangle
}

void UToolInputComponent::OnBoxSelectEnd(const FInputActionValue& Value)
{
	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnBoxSelectEnd"));
	}
}

// --- Drawing Handlers ---

void UToolInputComponent::OnDrawPlacePoint(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			FRTPointerEvent Event;
			Event.Action = ERTPointerAction::Down;
			// TODO: Get cursor position from PC
			ToolMgr->ProcessInput(Event);
		}
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnDrawPlacePoint"));
	}
}

void UToolInputComponent::OnDrawConfirm(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			// Send Confirm action to the active tool
			FRTPointerEvent Event;
			Event.Action = ERTPointerAction::Confirm;
			ToolMgr->ProcessInput(Event);
		}
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnDrawConfirm"));
	}
}

void UToolInputComponent::OnDrawCancel(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
			{
				LineTool->CancelDrawing();
			}
		}
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnDrawCancel"));
	}
}

void UToolInputComponent::OnDrawClose(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
			{
				LineTool->ClosePolyline();
			}
		}
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnDrawClose"));
	}
}

void UToolInputComponent::OnDrawRemoveLastPoint(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			if (URTPlanLineTool* LineTool = Cast<URTPlanLineTool>(ToolMgr->GetActiveTool()))
			{
				LineTool->RemoveLastPoint();
			}
		}
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnDrawRemoveLastPoint"));
	}
}

void UToolInputComponent::OnOrthoLock(const FInputActionValue& Value)
{
	// TODO: Set ortho lock state on controller when method is implemented
	// AArchVisPlayerController* PC = GetArchVisController();
	// if (PC) PC->SetOrthoLockActive(true);

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnOrthoLock: Started"));
	}
}

void UToolInputComponent::OnOrthoLockReleased(const FInputActionValue& Value)
{
	// TODO: Set ortho lock state on controller when method is implemented
	// AArchVisPlayerController* PC = GetArchVisController();
	// if (PC) PC->SetOrthoLockActive(false);

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnOrthoLock: Released"));
	}
}

void UToolInputComponent::OnAngleSnap(const FInputActionValue& Value)
{
	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnAngleSnap toggled"));
	}
}

// --- Tool Switching ---

void UToolInputComponent::OnToolSelect(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SelectToolByType(ERTPlanToolType::Select);
		}
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnToolSelect"));
	}
}

void UToolInputComponent::OnToolLine(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SelectToolByType(ERTPlanToolType::Line);
		}
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnToolLine"));
	}
}

void UToolInputComponent::OnToolPolyline(const FInputActionValue& Value)
{
	if (AArchVisGameMode* GM = Cast<AArchVisGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		if (URTPlanToolManager* ToolMgr = GM->GetToolManager())
		{
			ToolMgr->SelectToolByType(ERTPlanToolType::Polyline);
		}
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogToolInput, Log, TEXT("OnToolPolyline"));
	}
}

// --- Helpers ---

AArchVisPlayerController* UToolInputComponent::GetArchVisController() const
{
	return Cast<AArchVisPlayerController>(GetOwner());
}

UEnhancedInputLocalPlayerSubsystem* UToolInputComponent::GetInputSubsystem() const
{
	AArchVisPlayerController* PC = GetArchVisController();
	if (!PC) return nullptr;

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer) return nullptr;

	return LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}

void UToolInputComponent::AddMappingContext(UInputMappingContext* Context, int32 Priority)
{
	if (!Context) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem();
	if (!Subsystem) return;

	if (!Subsystem->HasMappingContext(Context))
	{
		Subsystem->AddMappingContext(Context, Priority);
		
		if (bDebugEnabled)
		{
			UE_LOG(LogToolInput, Log, TEXT("Added IMC: %s (Priority: %d)"), *Context->GetName(), Priority);
		}
	}
}

void UToolInputComponent::RemoveMappingContext(UInputMappingContext* Context)
{
	if (!Context) return;

	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem();
	if (!Subsystem) return;

	if (Subsystem->HasMappingContext(Context))
	{
		Subsystem->RemoveMappingContext(Context);
		
		if (bDebugEnabled)
		{
			UE_LOG(LogToolInput, Log, TEXT("Removed IMC: %s"), *Context->GetName());
		}
	}
}

