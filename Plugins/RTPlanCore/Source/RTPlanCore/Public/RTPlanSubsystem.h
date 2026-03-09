// RTPlanSubsystem.h
// Game Instance Subsystem that holds references to Document, Tools, and other core objects.
// Access from anywhere via: URTPlanSubsystem::Get(WorldContextObject)

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RTPlanSubsystem.generated.h"

class URTPlanDocument;

/**
 * Game Instance Subsystem providing centralized access to core RTPlan objects.
 * 
 * Usage in C++:
 *   URTPlanSubsystem* Subsystem = URTPlanSubsystem::Get(this);
 *   URTPlanDocument* Doc = Subsystem->GetDocument();
 *
 * Usage in Blueprint:
 *   Get Game Instance → Get Subsystem (RTPlan Subsystem) → Get Document
 *   OR use the static helper: Get RTPlan Subsystem node
 * 
 * NOTE: Tool Manager is stored as UObject* to avoid circular dependency.
 *       Use Cast<URTPlanToolManager> after getting it.
 */
UCLASS()
class RTPLANCORE_API URTPlanSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End USubsystem Interface

	// ----- Static Accessor -----

	/**
	 * Get the RTPlan Subsystem from any world context object.
	 * Returns nullptr if not available (e.g., no game instance yet).
	 */
	UFUNCTION(BlueprintPure, Category = "RTPlan", meta = (WorldContext = "WorldContextObject", DisplayName = "Get RTPlan Subsystem"))
	static URTPlanSubsystem* Get(const UObject* WorldContextObject);

	// ----- Document -----

	/** Get the plan document. Creates one if it doesn't exist. */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Subsystem")
	URTPlanDocument* GetDocument();

	/** Set a custom document (for loading, etc.) */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Subsystem")
	void SetDocument(URTPlanDocument* InDocument);

	/** Check if a document exists */
	UFUNCTION(BlueprintPure, Category = "RTPlan|Subsystem")
	bool HasDocument() const { return Document != nullptr; }

	// ----- Tool Manager -----
	// NOTE: Stored as UObject* to avoid circular module dependency.
	// The actual class is URTPlanToolManager from RTPlanTools module.

	/** 
	 * Get the tool manager (as UObject). Cast to URTPlanToolManager in your code.
	 * Returns nullptr if not set.
	 */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Subsystem", meta = (DisplayName = "Get Tool Manager"))
	UObject* GetToolManagerObject() const { return ToolManager; }

	/** 
	 * Set the tool manager. Call this from your PlayerController or GameMode.
	 * @param InToolManager - Should be a URTPlanToolManager instance
	 */
	UFUNCTION(BlueprintCallable, Category = "RTPlan|Subsystem")
	void SetToolManager(UObject* InToolManager);

	/** Check if tool manager exists */
	UFUNCTION(BlueprintPure, Category = "RTPlan|Subsystem")
	bool HasToolManager() const { return ToolManager != nullptr; }

	// ----- Events -----

	/** Called when document is created or changed */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDocumentChanged, URTPlanDocument*, NewDocument);
	
	UPROPERTY(BlueprintAssignable, Category = "RTPlan|Subsystem")
	FOnDocumentChanged OnDocumentChanged;

	/** Called when tool manager is set or changed */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnToolManagerChanged, UObject*, NewToolManager);
	
	UPROPERTY(BlueprintAssignable, Category = "RTPlan|Subsystem")
	FOnToolManagerChanged OnToolManagerChanged;

private:
	/** The plan document */
	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;

	/** The tool manager (actually URTPlanToolManager, stored as UObject to avoid circular dependency) */
	UPROPERTY(Transient)
	TObjectPtr<UObject> ToolManager;
};
