// ArchVisPawnBase.cpp

#include "ArchVisPawnBase.h"

AArchVisPawnBase::AArchVisPawnBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root scene component
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootComponent = RootScene;
}

void AArchVisPawnBase::BeginPlay()
{
	Super::BeginPlay();
}

void AArchVisPawnBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

