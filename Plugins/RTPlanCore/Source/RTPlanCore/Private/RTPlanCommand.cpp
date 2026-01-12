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
