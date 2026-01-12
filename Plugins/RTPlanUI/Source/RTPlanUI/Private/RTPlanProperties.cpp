#include "RTPlanProperties.h"

void URTPlanProperties::SetDocument(URTPlanDocument* InDoc)
{
	if (Document)
	{
		Document->OnPlanChanged.RemoveDynamic(this, &URTPlanProperties::OnPlanChanged);
	}

	Document = InDoc;

	if (Document)
	{
		Document->OnPlanChanged.AddDynamic(this, &URTPlanProperties::OnPlanChanged);
		Refresh();
	}
}

void URTPlanProperties::OnPlanChanged()
{
	Refresh();
}
