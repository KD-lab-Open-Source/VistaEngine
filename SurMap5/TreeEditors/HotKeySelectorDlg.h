#ifndef __HOT_KEY_SELECTOR_H_INCLUDED__
#define __HOT_KEY_SELECTOR_H_INCLUDED__

#include "sKey.h"

class CHotKeySelectorDlg : public CDialog
{
	DECLARE_DYNAMIC(CHotKeySelectorDlg)

public:
	CHotKeySelectorDlg(const sKey& key, CWnd* pParent = 0);
	virtual ~CHotKeySelectorDlg();

// Dialog Data
	enum { IDD = IDD_DLG_HOTKEY_SELECTOR };
	const sKey& key() { return key_; }
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	void OnOK();

	DECLARE_MESSAGE_MAP()
private:
	sKey key_;
	bool pressed_;
	int ticksLeft_;

	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
public:
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
protected:
	virtual void OnCancel();
};

#endif
