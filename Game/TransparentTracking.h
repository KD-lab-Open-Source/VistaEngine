#ifndef __TRANSPARENT_TRACKING_H_INCLUDED__
#define __TRANSPARENT_TRACKING_H_INCLUDED__

#include "..\UserInterface\UI_enums.h"
class cTransparentTracking
{
	struct TransparentObj
	{
		class UnitBase* unit;
		float camera_distance2;
		bool already_transparent;
		bool need_transparent;
		TransparentObj(UnitBase*);
		bool operator <(const TransparentObj& cobj) const { return unit<cobj.unit;}
	};
	Vect2i grid_size;
	list<TransparentObj> transpObj_container;
	struct Node
	{
		vector<TransparentObj*> objs;
		UINT quant_num;
		Node():quant_num(0){}
	};
	Node* grid;
	UINT quant_num;
	typedef list<TransparentObj> UnitContainer;
	UnitContainer equitant;
	UnitContainer transparent;
	UnitContainer::iterator find(UnitContainer& con, const TransparentObj& obj)
	{
		if (con.empty())
			return con.begin();
		return lower_bound(con.begin(), con.end(), obj);
	}
public:
	cTransparentTracking();
	~cTransparentTracking();
	void Tracking(cCamera *pCamera);
	void RewriteUnit(UnitBase* unit, bool prev_tranparent);
	void RegisterUnit(UnitBase* unit);
	void UnRegisterUnit(UnitBase* unit);
};


#endif
