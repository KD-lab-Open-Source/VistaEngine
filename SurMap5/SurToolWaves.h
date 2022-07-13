#ifndef __SUR_TOOL_WAVES_H_INCLUDED__
#define __SUR_TOOL_WAVES_H_INCLUDED__

#include "Environment\Environment.h"
#include "EScroll.h"
#include "SurToolAux.h"
#include "WaveDlg.h"

class CSurToolWaves : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolWaves)

public:
	CSurToolWaves(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolWaves();

	virtual bool onLMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord);
	virtual bool onLMBUp (const Vect3f& worldCoord, const Vect2i& screenCoord);
	virtual bool onRMBUp (const Vect3f& worldCoord, const Vect2i& screenCoord);
	virtual bool onTrackingMouse (const Vect3f& worldCoord, const Vect2i& scrCoord);

	virtual bool onDrawAuxData(void);


	int getIDD() const { return IDD_BARDLG_WAVES; }
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()

protected:
	CWaveDlg* waveDlg_;
	bool leftButtonDown_;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
};

#endif
