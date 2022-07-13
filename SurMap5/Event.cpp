#include "StdAfx.h"
#include "Event.h"

EventBase::~EventBase()
{
	while(!listeners_.empty())
		unregisterListener(listeners_.front());
}

void EventBase::registerListener(ListenerBase* object)
{
	Listeners::iterator it = std::find(listeners_.begin(), listeners_.end(), object);
	if(it == listeners_.end()){
		listeners_.push_back(object);
		object->setEvent(this);
	}
}
void EventBase::unregisterListener(ListenerBase* object)
{
	xassert(object->event() == this);
	Listeners::iterator it = std::find(listeners_.begin(), listeners_.end(), object);
	xassert(it != listeners_.end());
	object->setEvent(0);
	listeners_.erase(it);
}

void EventBase::emit()
{
	Listeners::iterator it;
	for(it = listeners_.begin(); it != listeners_.end(); ++it)
		call(*it);
}

// ---------------------------------------------------------------------------

ListenerBase::ListenerBase() 
: event_(0)
{

}

ListenerBase::~ListenerBase()
{
	if(event_){
		event_->unregisterListener(this);
		event_ = 0;
	}
}

void ListenerBase::setEvent(EventBase* event){
	event_ = event;
}

EventBase* ListenerBase::event()
{
	return event_;
}
