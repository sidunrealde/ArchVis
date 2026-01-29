// DraftingInputComponent.cpp
// Input component for 2D Drafting pawn

#include "Input/DraftingInputComponent.h"
#include "ArchVisDraftingPawn.h"
#include "ArchVisInputConfig.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogDraftingInput, Log, All);

UDraftingInputComponent::UDraftingInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UDraftingInputComponent::BeginPlay()
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

void UDraftingInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Process pan based on mouse delta
	ProcessPan(DeltaTime);
}

void UDraftingInputComponent::SetupInputBindings()
{
	if (!InputConfig)
	{
		UE_LOG(LogDraftingInput, Error, TEXT("SetupInputBindings: InputConfig is null!"));
		return;
	}

	// Pan action (IA_Pan) - use separate handlers for Started/Completed
	if (InputConfig->IA_Pan)
	{
		BindAction(InputConfig->IA_Pan, ETriggerEvent::Started, this, &UDraftingInputComponent::OnPanActionStarted);
		BindAction(InputConfig->IA_Pan, ETriggerEvent::Completed, this, &UDraftingInputComponent::OnPanActionCompleted);
	}

	// Zoom (scroll wheel)
	if (InputConfig->IA_Zoom)
	{
		BindAction(InputConfig->IA_Zoom, ETriggerEvent::Triggered, this, &UDraftingInputComponent::OnZoom);
	}

	if (bDebugEnabled)
	{
		UE_LOG(LogDraftingInput, Log, TEXT("SetupInputBindings complete"));
	}
}

void UDraftingInputComponent::AddInputMappingContexts()
{
	if (!InputConfig)
	{
		return;
	}

	// Add 2D base context (now called IMC_Drafting_Navigation)
	AddMappingContext(InputConfig->IMC_Drafting_Navigation, IMC_Priority_ModeBase);

	if (bDebugEnabled)
	{
		UE_LOG(LogDraftingInput, Log, TEXT("Added 2D Drafting IMCs"));
	}
}

void UDraftingInputComponent::RemoveInputMappingContexts()
{
	if (!InputConfig)
	{
		return;
	}

	RemoveMappingContext(InputConfig->IMC_Drafting_Navigation);

	if (bDebugEnabled)
	{
		UE_LOG(LogDraftingInput, Log, TEXT("Removed 2D Drafting IMCs"));
	}
}

void UDraftingInputComponent::OnPanActionStarted(const FInputActionValue& Value)
{
	bPanActionActive = true;

	// Reset mouse position when pan starts
	if (APlayerController* PC = GetPlayerController())
	{
		float MouseX, MouseY;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			LastMousePosition = FVector2D(MouseX, MouseY);
		}
	}

	// Always log pan start for debugging
	UE_LOG(LogDraftingInput, Log, TEXT("IA_Pan: Started - bPanActionActive=%d"), bPanActionActive);
}

void UDraftingInputComponent::OnPanActionCompleted(const FInputActionValue& Value)
{
	bPanActionActive = false;

	// Always log pan end for debugging
	UE_LOG(LogDraftingInput, Log, TEXT("IA_Pan: Completed - bPanActionActive=%d"), bPanActionActive);
}

void UDraftingInputComponent::OnZoom(const FInputActionValue& Value)
{
	const float ZoomAmount = Value.Get<float>();

	if (FMath::IsNearlyZero(ZoomAmount))
	{
		return;
	}

	if (AArchVisDraftingPawn* Pawn = GetDraftingPawn())
	{
		Pawn->Zoom(ZoomAmount);

		if (bDebugEnabled)
		{
			UE_LOG(LogDraftingInput, Log, TEXT("IA_Zoom: %f"), ZoomAmount);
		}
	}
}

void UDraftingInputComponent::ProcessPan(float DeltaTime)
{
	if (!bPanActionActive)
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
	FVector2D Delta = CurrentMousePos - LastMousePosition;
	LastMousePosition = CurrentMousePos;

	// Skip if no movement
	if (Delta.IsNearlyZero())
	{
		return;
	}

	if (AArchVisDraftingPawn* Pawn = GetDraftingPawn())
	{
		Pawn->Pan(Delta);

		if (bDebugEnabled)
		{
			UE_LOG(LogDraftingInput, Log, TEXT("Pan: Delta=%s"), *Delta.ToString());
		}
	}
}

AArchVisDraftingPawn* UDraftingInputComponent::GetDraftingPawn() const
{
	return GetOwningPawn<AArchVisDraftingPawn>();
}
