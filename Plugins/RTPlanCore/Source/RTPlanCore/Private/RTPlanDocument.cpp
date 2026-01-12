#include "RTPlanDocument.h"
#include "RTPlanCommand.h"
#include "JsonObjectConverter.h"

URTPlanDocument::URTPlanDocument()
{
}

bool URTPlanDocument::SubmitCommand(URTCommand* Command)
{
	if (!Command)
	{
		return false;
	}

	Command->Document = this;

	if (Command->Execute())
	{
		UndoStack.Add(Command);
		
		// Clear redo stack on new action
		RedoStack.Empty();

		// Limit stack size
		if (UndoStack.Num() > MaxUndoSteps)
		{
			UndoStack.RemoveAt(0);
		}

		OnPlanChanged.Broadcast();
		return true;
	}

	return false;
}

void URTPlanDocument::Undo()
{
	if (UndoStack.Num() > 0)
	{
		URTCommand* Command = UndoStack.Pop();
		Command->Undo();
		RedoStack.Add(Command);
		OnPlanChanged.Broadcast();
	}
}

void URTPlanDocument::Redo()
{
	if (RedoStack.Num() > 0)
	{
		URTCommand* Command = RedoStack.Pop();
		if (Command->Execute())
		{
			UndoStack.Add(Command);
			OnPlanChanged.Broadcast();
		}
		else
		{
			// If redo fails, we might be in an inconsistent state.
			// For now, just drop it.
		}
	}
}

bool URTPlanDocument::CanUndo() const
{
	return UndoStack.Num() > 0;
}

bool URTPlanDocument::CanRedo() const
{
	return RedoStack.Num() > 0;
}

FString URTPlanDocument::ToJson() const
{
	FString OutputString;
	FJsonObjectConverter::UStructToJsonObjectString(Data, OutputString);
	return OutputString;
}

bool URTPlanDocument::FromJson(const FString& JsonString)
{
	FRTPlanData NewData;
	if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &NewData, 0, 0))
	{
		Data = NewData;
		UndoStack.Empty();
		RedoStack.Empty();
		OnPlanChanged.Broadcast();
		return true;
	}
	return false;
}
