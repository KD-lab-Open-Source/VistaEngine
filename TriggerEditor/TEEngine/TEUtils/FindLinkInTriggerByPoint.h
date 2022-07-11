#ifndef __FIND_LINK_IN_TRIGGER_BY_POINT_H_INCLUDED__
#define __FIND_LINK_IN_TRIGGER_BY_POINT_H_INCLUDED__

class Trigger;

class FindLinkInTriggerByPoint  
{
	class TestLink;
public:
	typedef Trigger const argument_type;
	typedef bool result_type;

	FindLinkInTriggerByPoint(CPoint const& testPoint, int &linkIndex);
	~FindLinkInTriggerByPoint();
	bool operator()(argument_type& trigger) ;
	int getLinkIndex() const;
private:
	int& linkIndex_;
	CPoint const& testPoint_;
};

#endif
