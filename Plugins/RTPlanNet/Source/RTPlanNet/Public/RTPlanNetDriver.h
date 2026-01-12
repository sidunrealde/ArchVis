#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RTPlanDocument.h"
#include "RTPlanCommand.h"
#include "RTPlanNetDriver.generated.h"

/**
 * Actor responsible for replicating PlanDocument state.
 * Server is authoritative. Clients send RPCs to execute commands.
 */
UCLASS()
class RTPLANNET_API ARTPlanNetDriver : public AActor
{
	GENERATED_BODY()
	
public:	
	ARTPlanNetDriver();

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Net")
	void SetDocument(URTPlanDocument* InDoc);

	// Client -> Server: Request to execute a command
	// Note: We can't replicate UObjects (Commands) easily via RPC unless they are subobjects or structs.
	// For V1, we will serialize the command to a string or struct.
	// Let's use a simplified approach: Specific RPCs for common actions, or a generic JSON command wrapper.
	// Generic JSON is flexible but heavier. Let's try specific RPCs for V1 Core (AddWall, etc).
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SubmitAddWall(FRTWall Wall);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SubmitDeleteWall(FGuid WallId);

	// Replicated Property: The entire Plan Data (as JSON string for simplicity in V1)
	// In production, we would use FastArraySerializer or partial updates.
	UPROPERTY(ReplicatedUsing = OnRep_PlanJson)
	FString ReplicatedPlanJson;

	UFUNCTION()
	void OnRep_PlanJson();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnPlanChanged();

	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;

	// Debounce timer for replication updates
	FTimerHandle UpdateTimer;
};
