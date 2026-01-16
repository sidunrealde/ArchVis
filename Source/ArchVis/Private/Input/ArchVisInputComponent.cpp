// ArchVisInputComponent.cpp
// Base input component implementation

#include "Input/ArchVisInputComponent.h"
#include "ArchVisInputConfig.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

DEFINE_LOG_CATEGORY_STATIC(LogArchVisInput, Log, All);

UArchVisInputComponent::UArchVisInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UArchVisInputComponent::Initialize(UArchVisInputConfig* InInputConfig)
{
	if (bIsInitialized)
	{
		UE_LOG(LogArchVisInput, Warning, TEXT("UArchVisInputComponent::Initialize called but already initialized"));
		return;
	}

	InputConfig = InInputConfig;
	
	if (!InputConfig)
	{
		UE_LOG(LogArchVisInput, Error, TEXT("UArchVisInputComponent::Initialize - InputConfig is null!"));
		return;
	}

	// Setup input bindings (derived class implements this)
	SetupInputBindings();

	bIsInitialized = true;

	if (bDebugEnabled)
	{
		UE_LOG(LogArchVisInput, Log, TEXT("UArchVisInputComponent::Initialize complete for %s"), 
			*GetOwner()->GetName());
	}
}

void UArchVisInputComponent::Cleanup()
{
	if (!bIsInitialized)
	{
		return;
	}

	RemoveInputMappingContexts();
	ClearActionBindings();
	
	bIsInitialized = false;
	CachedPlayerController.Reset();

	if (bDebugEnabled)
	{
		UE_LOG(LogArchVisInput, Log, TEXT("UArchVisInputComponent::Cleanup complete for %s"), 
			GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));
	}
}

void UArchVisInputComponent::OnPawnPossessed(AController* NewController)
{
	APlayerController* PC = Cast<APlayerController>(NewController);
	if (!PC)
	{
		UE_LOG(LogArchVisInput, Warning, TEXT("OnPawnPossessed: NewController is not a PlayerController"));
		return;
	}

	CachedPlayerController = PC;

	// Add our IMCs
	AddInputMappingContexts();

	if (bDebugEnabled)
	{
		UE_LOG(LogArchVisInput, Log, TEXT("OnPawnPossessed: Added IMCs for %s"), *GetOwner()->GetName());
	}
}

void UArchVisInputComponent::OnPawnUnpossessed(AController* OldController)
{
	// Remove our IMCs
	RemoveInputMappingContexts();
	CachedPlayerController.Reset();

	if (bDebugEnabled)
	{
		UE_LOG(LogArchVisInput, Log, TEXT("OnPawnUnpossessed: Removed IMCs for %s"), 
			GetOwner() ? *GetOwner()->GetName() : TEXT("NULL"));
	}
}

UEnhancedInputLocalPlayerSubsystem* UArchVisInputComponent::GetInputSubsystem() const
{
	APlayerController* PC = GetPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	ULocalPlayer* LocalPlayer = PC->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return nullptr;
	}

	return LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}

void UArchVisInputComponent::AddMappingContext(UInputMappingContext* Context, int32 InPriority)
{
	if (!Context)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem();
	if (!Subsystem)
	{
		UE_LOG(LogArchVisInput, Warning, TEXT("AddMappingContext: Could not get input subsystem"));
		return;
	}

	if (!Subsystem->HasMappingContext(Context))
	{
		Subsystem->AddMappingContext(Context, InPriority);
		
		if (bDebugEnabled)
		{
			UE_LOG(LogArchVisInput, Log, TEXT("Added IMC: %s (Priority: %d)"), 
				*Context->GetName(), InPriority);
		}
	}
}

void UArchVisInputComponent::RemoveMappingContext(UInputMappingContext* Context)
{
	if (!Context)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetInputSubsystem();
	if (!Subsystem)
	{
		return;
	}

	if (Subsystem->HasMappingContext(Context))
	{
		Subsystem->RemoveMappingContext(Context);
		
		if (bDebugEnabled)
		{
			UE_LOG(LogArchVisInput, Log, TEXT("Removed IMC: %s"), *Context->GetName());
		}
	}
}

APlayerController* UArchVisInputComponent::GetPlayerController() const
{
	if (CachedPlayerController.IsValid())
	{
		return CachedPlayerController.Get();
	}

	// Fallback: try to get from pawn
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		return Cast<APlayerController>(Pawn->GetController());
	}

	return nullptr;
}

