#ifndef __SCALE_BAR_H_INCLUDED__
#define __SCALE_BAR_H_INCLUDED__

#include <prof-uis.h>

#include "ScaleInterfaces.h"
class CScaleBar :  public CExtToolControlBar
{
public:
	CScaleBar();
	bool Create(CWnd* pparent, DWORD id);
	void SetScalable(IScalable* psclbl);
	void UpdateScaleInfo();

private:
	void AddCombo();

	void OnComboSelChange();
	void OnScaleMinus();
	void OnScalePlus();
	DECLARE_MESSAGE_MAP();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void InitCombo();

	void OnUpdateScaleMinus(CCmdUI* pCmdUI);
	void OnUpdateScalePlus(CCmdUI* pCmdUI);

	void ReadAndSetScale();
private:
	CExtComboBox m_wndCombo;
	CFont m_font;

	IScalable* m_pScalable;
};

#endif
