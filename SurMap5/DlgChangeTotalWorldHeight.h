#ifndef __DLG_CHANGE_TOTAL_WORLD_HEIGHT_H_INCLUDED__
#define __DLG_CHANGE_TOTAL_WORLD_HEIGHT_H_INCLUDED__

#include "EScroll.h"

#include "Serialization\Serialization.h"
#include "AttribEditor\AttribEditorCtrl.h"
#include "Terra\vmap.h"

struct HistogramDate {
	enum { HISTOGRAM_ARRAY_SIZE=256 };
	int minVxHeight;
	int maxVxHeight;
	unsigned char histogramArr[HISTOGRAM_ARRAY_SIZE];
};

// CDlgChangeTotalWorldHeight dialog

class CDlgChangeTotalWorldHeight : public CDialog
{
	DECLARE_DYNAMIC(CDlgChangeTotalWorldHeight)

public:
	CDlgChangeTotalWorldHeight(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgChangeTotalWorldHeight();

	CEScrollVx m_deltaH;
	CEScroll m_scaleH;

// Dialog Data
	enum { IDD = IDD_DLG_CHANGE_WORLD_HEIGHT };

protected:
	HistogramDate inputHistogram;
	HistogramDate outPutHistogram;
	CBitmap m_bmpHistogram;
	void drawHisogram(CPaintDC& dc, int HISTOGRAM_WND, struct HistogramDate& histogram);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	vrtMapChangeParam m_changeParam;
protected:
	CAttribEditorCtrl m_ctlAttribEditor;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnPaint();
protected:
	virtual void OnOK();
};

#endif
