#ifndef __EVENT_H_INCLUDED__
#define __EVENT_H_INCLUDED__

#include "SafeCast.h"

class ListenerBase;

class EventBase{
public:
	virtual ~EventBase();
	void registerListener(ListenerBase* object);
	void unregisterListener(ListenerBase* object);
	void emit();
	virtual void call(ListenerBase* listener) = 0;
protected:
	typedef std::vector<ListenerBase*> Listeners;
	Listeners listeners_;
};

class ListenerBase{
public:
	ListenerBase() ;
	virtual ~ListenerBase();

	void setEvent(EventBase* event);
	EventBase* event();
private:
	EventBase* event_;
};
                                                                            
#define DECLARE_EVENT(eventName)											\
struct eventName##Listener : ListenerBase{                                  \
	virtual void on##eventName() = 0;										\
};                                                                          \
struct Event##eventName : EventBase{									    \
	void registerListener(eventName##Listener* listener){                   \
		EventBase::registerListener(listener);								\
	}																		\
protected:                                                                  \
	void call(ListenerBase* listener){										\
		safe_cast<eventName##Listener*>(listener)->on##eventName();			\
	}																		\
};																			\

#endif
