#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "RTPlanSchema.h"
#include "RTPlanCommand.generated.h"

class URTPlanDocument;

/**
 * Base class for all plan modification commands.
 * Supports Execute (Do) and Undo.
 */
UCLASS(Abstract)
class RTPLANCORE_API URTCommand : public UObject
{
	GENERATED_BODY()

public:
	// The document this command operates on.
	UPROPERTY(Transient)
	TObjectPtr<URTPlanDocument> Document;

	// Execute the command. Returns true if successful.
	virtual bool Execute() { return false; }

	// Undo the command.
	virtual void Undo() {}

	// Returns a description for the Undo history UI (e.g. "Add Wall")
	virtual FString GetDescription() const { return TEXT("Unknown Command"); }
};

/**
 * Command to Add or Update a Vertex.
 */
UCLASS()
class RTPLANCORE_API URTCmdAddVertex : public URTCommand
{
	GENERATED_BODY()

public:
	FRTVertex Vertex;
	bool bIsNew = true;
	FRTVertex PreviousVertex; // For Undo if updating

	virtual bool Execute() override;
	virtual void Undo() override;
	virtual FString GetDescription() const override { return bIsNew ? TEXT("Add Vertex") : TEXT("Move Vertex"); }
};

/**
 * Command to Add or Update a Wall.
 */
UCLASS()
class RTPLANCORE_API URTCmdAddWall : public URTCommand
{
	GENERATED_BODY()

public:
	FRTWall Wall;
	bool bIsNew = true;
	FRTWall PreviousWall;

	virtual bool Execute() override;
	virtual void Undo() override;
	virtual FString GetDescription() const override { return bIsNew ? TEXT("Add Wall") : TEXT("Edit Wall"); }
};

/**
 * Command to Delete a Wall.
 */
UCLASS()
class RTPLANCORE_API URTCmdDeleteWall : public URTCommand
{
	GENERATED_BODY()

public:
	FGuid WallId;
	FRTWall DeletedWall; // Stored for Undo

	virtual bool Execute() override;
	virtual void Undo() override;
	virtual FString GetDescription() const override { return TEXT("Delete Wall"); }
};

// TODO: Add commands for Openings, Objects, Runs as we implement those features.
