#ifndef __TIME_SLIDER_DLG_H_INCLUDED__
#define __TIME_SLIDER_DLG_H_INCLUDED__

#include "EScroll.h"
#include "EventListeners.h"
class CMainFrame;

class CTimeSliderDlg : public CDialog, public CExtMouseCaptureSink, public WorldObserver, public sigslot::has_slots
{
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

	void onWorldChanged(WorldObserver* changer);

	afx_msg void OnTimeEditChange();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL OnInitDialog();

	DECLARE_DYNAMIC(CTimeSliderDlg)
	DECLARE_MESSAGE_MAP()
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	virtual void OnOK();
	virtual void OnCancel();

	void updateSlider();
	void updateText();

	bool wasTimeFlowEnabled_;
};

#endif
