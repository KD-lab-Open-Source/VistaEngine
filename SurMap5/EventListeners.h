#ifndef __EVENT_LISTENERS_H_INCLUDED__
#define __EVENT_LISTENERS_H_INCLUDED__

#include "XTL/sigslot.h"

struct SelectionObserver{};
struct ObjectObserver{};
struct WorldObserver{};

class EventMaster{
public:
	typedef sigslot::signal1<WorldObserver*> SignalWorldChanged;
	SignalWorldChanged& signalWorldChanged(){ return signalWorldChanged_; }
	typedef sigslot::signal1<ObjectObserver*> SignalObjectChanged;
	SignalObjectChanged& signalObjectChanged(){ return signalObjectChanged_; }
	typedef sigslot::signal1<SelectionObserver*> SignalSelectionChanged;
	SignalSelectionChanged& signalSelectionChanged(){ return signalSelectionChanged_; }

protected:
	SignalWorldChanged signalWorldChanged_;
	SignalSelectionChanged signalSelectionChanged_;
	SignalObjectChanged signalObjectChanged_;
};

EventMaster& eventMaster();

#endif
