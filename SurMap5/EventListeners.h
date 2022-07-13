#ifndef __EVENT_LISTENERS_H_INCLUDED__
#define __EVENT_LISTENERS_H_INCLUDED__

#include "Event.h"

DECLARE_EVENT(WorldChanged);
DECLARE_EVENT(ObjectChanged);
DECLARE_EVENT(SelectionChanged);

class EventMaster{
public:
	EventWorldChanged& eventWorldChanged() { return eventWorldChanged_; }
	EventObjectChanged& eventObjectChanged() { return eventObjectChanged_; }
	EventSelectionChanged& eventSelectionChanged() { return eventSelectionChanged_; }
protected:
	EventSelectionChanged eventSelectionChanged_;
	EventObjectChanged eventObjectChanged_;
	EventWorldChanged eventWorldChanged_;
};

EventMaster& eventMaster();

#endif
