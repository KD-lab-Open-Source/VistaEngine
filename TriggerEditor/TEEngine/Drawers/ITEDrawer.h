#ifndef __I_T_E_DRAWER_H_INCLUDED__
#define __I_T_E_DRAWER_H_INCLUDED__
/********************************************************************
	created:	2003/05/21
	created:	21:5:2003   13:46
	filename: 	d:\Projects\Quest\QuestEditor\ITEDrawer.h
	file path:	d:\Projects\Quest\QuestEditor
	file base:	ITEDrawer
	file ext:	h
	Powerd by:	Илюха
	
	purpose:	Базовый интерфейс для отрисовщика
*********************************************************************/


#include "IDrawData.h"

class TriggerEditorView;

interface ITEDrawer
{
	virtual void draw(CDC* pdc, TriggerEditorView* pwnd, 
						IDrawData* pdd, CRect const& rcPaint) const = 0;
	virtual void drawDraggedRect(HDC dc, CRect const&r, HPEN) const = 0;
protected:
	inline ~ITEDrawer();
};
inline ITEDrawer::~ITEDrawer()
{}

#endif
