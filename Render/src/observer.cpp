#include "StdAfxRD.h"
#include "observer.h"

static MTSection observer_section;

Observer::Observer()
{
}

Observer::~Observer()
{
	vector<ObserverLink*>::iterator it;
	FOR_EACH(links,it)
	{
		(*it)->ClearLink(this);
	}
}

void Observer::AddLink(ObserverLink* link)
{
	MTAuto lock(observer_section);
#ifndef _FINAL_VERSION_
	vector<ObserverLink*>::iterator it = find(links.begin(),links.end(),link);
	if(it!=links.end())
		VISASSERT(0);
	else
		links.push_back(link);
#else
	links.push_back(link);
#endif
	link->SetLink(this);
}

void Observer::BreakLink(ObserverLink* link)
{
	MTAuto lock(observer_section);
	MTG();
	vector<ObserverLink*>::iterator it = find(links.begin(),links.end(),link);
	if(it!=links.end())
	{
		links.erase(it);
	}else
		VISASSERT(0);
}
void Observer::UpdateLink()
{
	MTAuto lock(observer_section);
	MTG();
	vector<ObserverLink*>::iterator it;
	FOR_EACH(links,it)
	{
		(*it)->Update();
	}
}

ObserverLink::~ObserverLink()
{
	if(observer)
	{
		MTG();
		observer->BreakLink(this);
	}
}

void ObserverLink::SetLink(Observer* o)
{
	VISASSERT(observer==NULL);
	observer=o;
}

void ObserverLink::ClearLink(Observer* o)
{
	VISASSERT(o==observer);
	observer=NULL;
}
