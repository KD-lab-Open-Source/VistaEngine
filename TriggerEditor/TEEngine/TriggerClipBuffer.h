#ifndef __TRIGGER_CLIP_BUFFER_H_INCLUDED__
#define __TRIGGER_CLIP_BUFFER_H_INCLUDED__

#include <list>
#include "TriggerExport.h"

class TriggerClipBuffer  
{
private:
	typedef std::list<Trigger> Container;
public:
	TriggerClipBuffer();
	~TriggerClipBuffer();

	typedef Container::const_iterator iterator;
	typedef Container::value_type value_type;

	bool empty();
	void clear();

	iterator begin() const;
	iterator end() const;

	void push_back(Trigger const& trigger);
	static TriggerClipBuffer& instance();
private:
	Container list_;
};

#endif
