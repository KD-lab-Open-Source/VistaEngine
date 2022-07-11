#ifndef __EVENT_PARAMETERS_H__
#define __EVENT_PARAMETERS_H__

#include "Triggers.h"
#include "Parameters.h"

class EventParameters : public Event
{
public:
	EventParameters(Type type, const ParameterSet& parameters, const RequestResourceType requestResourceType = NEED_RESOURCE_SILENT_CHECK) : Event(type), parameters_(parameters), requestResourceType_(requestResourceType) {}
	const ParameterSet& parameters() const { return parameters_; }
	const RequestResourceType requestResourceType() const { return requestResourceType_; }

private:
	const ParameterSet parameters_;
	const RequestResourceType requestResourceType_;
};

#endif //__EVENT_PARAMETERS_H__
