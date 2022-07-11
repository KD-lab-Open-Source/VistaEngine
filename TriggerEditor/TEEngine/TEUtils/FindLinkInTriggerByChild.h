#ifndef __FIND_LINK_IN_TRIGGER_BY_CHILD_H_INCLUDED__
#define __FIND_LINK_IN_TRIGGER_BY_CHILD_H_INCLUDED__

class Trigger;
struct TriggerLink;

class FindLinkInTriggerByChild  
{
	class TestLink;
public:
	FindLinkInTriggerByChild(Trigger const* child);
	~FindLinkInTriggerByChild();
	bool operator()(TriggerLink const& link) const;
private:
	Trigger const* child_;
};

#endif
