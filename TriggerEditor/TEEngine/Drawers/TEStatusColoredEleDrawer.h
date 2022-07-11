#ifndef __T_E_STATUS_COLORED_ELE_DRAWER_H_INCLUDED__
#define __T_E_STATUS_COLORED_ELE_DRAWER_H_INCLUDED__
#include "TEEngine/Drawers/basetedrawer.h"

interface IDrawData;
class TriggerEditorView;

class TEStatusColoredEleDrawer :
	public BaseTEDrawer
{
public:
	TEStatusColoredEleDrawer(void);
	~TEStatusColoredEleDrawer(void);
	virtual void draw(CDC* pdc, 
					TriggerEditorView* source, 
					IDrawData* pdd, 
					CRect const& rcPaint) const;
protected:
	virtual HBRUSH	SelectTriggerBackBrush(Trigger const& pele) const;
	virtual HBRUSH	SelectLinkBrushColor(TriggerLink const& link) const;
	virtual HPEN	SelectLinkPenColor(TriggerLink const& link) const;
private:
};

#endif
