#include "RTPlanVRPawn.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "IXRTrackingSystem.h"
#include "HeadMountedDisplayFunctionLibrary.h"

ARTPlanVRPawn::ARTPlanVRPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	RootComponent = VROrigin;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VROrigin);

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController"));
	LeftController->SetupAttachment(VROrigin);
	LeftController->SetTrackingSource(EControllerHand::Left);

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController"));
	RightController->SetupAttachment(VROrigin);
	RightController->SetTrackingSource(EControllerHand::Right);

	LeftInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("LeftInteraction"));
	LeftInteraction->SetupAttachment(LeftController);
	LeftInteraction->PointerIndex = 0;

	RightInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("RightInteraction"));
	RightInteraction->SetupAttachment(RightController);
	RightInteraction->PointerIndex = 1;
}

void ARTPlanVRPawn::BeginPlay()
{
	Super::BeginPlay();
	
	// Ensure HMD is tracking
	if (GEngine->XRSystem.IsValid())
	{
		// In UE 5.x, EHMDTrackingOrigin::Floor might be deprecated or moved.
		// Using the newer IXRTrackingSystem interface or checking the enum definition.
		// It seems EHMDTrackingOrigin::Stage is the equivalent for Floor/Stage tracking in some versions,
		// or it's simply EHMDTrackingOrigin::Type::Stage.
		// Let's try Stage, which is the standard for Room-Scale VR.
		
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Stage);
	}
}

void ARTPlanVRPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ARTPlanVRPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("VR_Trigger_Right", IE_Pressed, this, &ARTPlanVRPawn::OnRightTriggerPressed);
	PlayerInputComponent->BindAction("VR_Trigger_Right", IE_Released, this, &ARTPlanVRPawn::OnRightTriggerReleased);
}

void ARTPlanVRPawn::OnRightTriggerPressed()
{
	bRightTriggerDown = true;
	// Dispatch to ToolManager via PlayerController or direct reference
}

void ARTPlanVRPawn::OnRightTriggerReleased()
{
	bRightTriggerDown = false;
}

FRTPointerEvent ARTPlanVRPawn::GetPointerEvent(ERTInputSource Source) const
{
	FRTPointerEvent Event;
	Event.Source = Source;
	
	if (Source == ERTInputSource::VRRight)
	{
		Event.WorldOrigin = RightController->GetComponentLocation();
		Event.WorldDirection = RightController->GetForwardVector();
		Event.Action = bRightTriggerDown ? ERTPointerAction::Down : ERTPointerAction::Move;
		// Note: "Move" is continuous, "Down" is state. 
		// Real system needs to track state changes (Down vs Move vs Up).
	}
	
	return Event;
}
