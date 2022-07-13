#ifndef __TIME_SLIDER_DLG_H_INCLUDED__
#define __TIME_SLIDER_DLG_H_INCLUDED__

#include "EScroll.h"
#include "EventListeners.h"
class CMainFrame;

class CTimeSliderDlg : public CDialog, public CExtMouseCaptureSink, public WorldChangedListener
{
	DECLARE_DYNAMIC(CTimeSliderDlg)

public:
	CTimeSliderDlg(CMainFrame* mainFrame, CWnd* parent = 0);
	virtual ~CTimeSliderDlg();

	enum {
		SLIDER_MIN = 0,
		SLIDER_MAX = 240
	};
	enum { IDD = IDD_TIME_SLIDER };
	void quant();
	CSliderCtrl& slider();

	void onWorldChanged();

	afx_msg void OnTimeEditChange();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL OnInitDialog();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	virtual void OnOK();
	virtual void OnCancel();
	DECLARE_MESSAGE_MAP()
private:
	void updateSlider();
	void updateText();

//	CEScroll timeSlider_;
	bool wasTimeFlowEnabled_;
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

#endif
