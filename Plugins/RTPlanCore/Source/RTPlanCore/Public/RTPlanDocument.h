#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RTPlanSchema.h"
#include "RTPlanDocument.generated.h"

class URTCommand;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlanChanged);

/**
 * The authoritative container for the interior plan.
 * Manages the data model, command stack (Undo/Redo), and serialization.
 */
UCLASS(BlueprintType)
class RTPLANCORE_API URTPlanDocument : public UObject
{
	GENERATED_BODY()

public:
	URTPlanDocument();

	// --- Data Access ---

	// Read-only access to the plan data.
	const FRTPlanData& GetData() const { return Data; }

	// Direct access for Commands (friend-like access via public for simplicity in plugins)
	FRTPlanData& GetDataMutable() { return Data; }

	// --- Command Stack ---

	// Execute a command and push it to the undo stack.
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Commands")
	bool SubmitCommand(URTCommand* Command);

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Commands")
	void Undo();

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Commands")
	void Redo();

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Commands")
	bool CanUndo() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|Commands")
	bool CanRedo() const;

	// --- Serialization ---

	UFUNCTION(BlueprintCallable, Category = "RTPlan|IO")
	FString ToJson() const;

	UFUNCTION(BlueprintCallable, Category = "RTPlan|IO")
	bool FromJson(const FString& JsonString);

	// --- Events ---

	// Broadcasts whenever the plan changes (after a command or undo/redo).
	UPROPERTY(BlueprintAssignable, Category = "RTPlan|Events")
	FOnPlanChanged OnPlanChanged;

private:
	// The canonical data
	UPROPERTY()
	FRTPlanData Data;

	// Undo/Redo stacks
	UPROPERTY()
	TArray<TObjectPtr<URTCommand>> UndoStack;

	UPROPERTY()
	TArray<TObjectPtr<URTCommand>> RedoStack;

	// Max undo steps
	int32 MaxUndoSteps = 50;
};
