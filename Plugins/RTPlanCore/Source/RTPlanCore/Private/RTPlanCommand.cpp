#include "RTPlanCommand.h"
#include "RTPlanDocument.h"

// --- URTCmdAddVertex ---

bool URTCmdAddVertex::Execute()
{
	if (!Document) return false;

	FRTPlanData& Data = Document->GetDataMutable();

	if (Data.Vertices.Contains(Vertex.Id))
	{
		bIsNew = false;
		PreviousVertex = Data.Vertices[Vertex.Id];
	}
	else
	{
		bIsNew = true;
	}

	Data.Vertices.Add(Vertex.Id, Vertex);
	return true;
}

void URTCmdAddVertex::Undo()
{
	if (!Document) return;

	FRTPlanData& Data = Document->GetDataMutable();

	if (bIsNew)
	{
		Data.Vertices.Remove(Vertex.Id);
	}
	else
	{
		Data.Vertices.Add(PreviousVertex.Id, PreviousVertex);
	}
}

// --- URTCmdAddWall ---

bool URTCmdAddWall::Execute()
{
	if (!Document) return false;

	FRTPlanData& Data = Document->GetDataMutable();

	if (Data.Walls.Contains(Wall.Id))
	{
		bIsNew = false;
		PreviousWall = Data.Walls[Wall.Id];
	}
	else
	{
		bIsNew = true;
	}

	Data.Walls.Add(Wall.Id, Wall);
	return true;
}

void URTCmdAddWall::Undo()
{
	if (!Document) return;

	FRTPlanData& Data = Document->GetDataMutable();

	if (bIsNew)
	{
		Data.Walls.Remove(Wall.Id);
	}
	else
	{
		Data.Walls.Add(PreviousWall.Id, PreviousWall);
	}
}

// --- URTCmdDeleteWall ---

bool URTCmdDeleteWall::Execute()
{
	if (!Document) return false;

	FRTPlanData& Data = Document->GetDataMutable();

	if (Data.Walls.Contains(WallId))
	{
		DeletedWall = Data.Walls[WallId];
		Data.Walls.Remove(WallId);
		return true;
	}

	return false;
}

void URTCmdDeleteWall::Undo()
{
	if (!Document) return;

	FRTPlanData& Data = Document->GetDataMutable();
	Data.Walls.Add(DeletedWall.Id, DeletedWall);
}

// --- URTCmdDeleteVertex ---

bool URTCmdDeleteVertex::Execute()
{
	if (!Document) return false;

	FRTPlanData& Data = Document->GetDataMutable();

	if (Data.Vertices.Contains(VertexId))
	{
		DeletedVertex = Data.Vertices[VertexId];
		Data.Vertices.Remove(VertexId);
		return true;
	}

	return false;
}

void URTCmdDeleteVertex::Undo()
{
	if (!Document) return;

	FRTPlanData& Data = Document->GetDataMutable();
	Data.Vertices.Add(DeletedVertex.Id, DeletedVertex);
}

// --- URTCmdMacro ---

bool URTCmdMacro::Execute()
{
	if (!Document) return false;

	// Execute all commands in order
	for (URTCommand* Cmd : Commands)
	{
		if (Cmd)
		{
			Cmd->Document = Document;
			if (!Cmd->Execute())
			{
				// If any command fails, we should probably rollback the ones that succeeded
				// But for now, we'll just return false and leave it in a partial state (TODO: fix this)
				return false;
			}
		}
	}
	return true;
}

void URTCmdMacro::Undo()
{
	if (!Document) return;

	// Undo all commands in reverse order
	for (int32 i = Commands.Num() - 1; i >= 0; --i)
	{
		if (URTCommand* Cmd = Commands[i])
		{
			Cmd->Undo();
		}
	}
}
