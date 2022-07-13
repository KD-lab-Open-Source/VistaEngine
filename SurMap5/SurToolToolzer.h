#ifndef __SUR_TOOL_TOOLZER_H_INCLUDED__
#define __SUR_TOOL_TOOLZER_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"
#include "mfc\SizeLayoutManager.h"

class CSurToolToolzer : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolToolzer)
public:
	CSurToolToolzer(CWnd* pParent = 0);
	virtual ~CSurToolToolzer();

	CEScroll m_RadiusToolzer;
	CEScrollVx m_DeltaH;
	//CEScroll m_LevelingH;
	CEScroll m_Roughness;
	unsigned int state_radio_button_DigPut;
	unsigned int state_level_ZL_or_level_h;
	//unsigned int state_free_or_level_h;

	CEScrollVx m_FilterMinH;
	CEScrollVx m_FilterMaxH;
	static int filterMinHValue;
	static int filterMaxHValue;
	static bool flag_EnableFilterH;


	int idxCurToolzerType;

	virtual bool onOperationOnMap(int x, int y);
	virtual bool onDrawAuxData(void);

	void serialize(Archive& ar);
	static void staticSerialize(Archive& ar);

	int getIDD() const { return IDD_BARDLG_TOOLZER; }
	
	virtual BOOL OnInitDialog();
	virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = NULL);
	afx_msg void OnBnClickedRadioDigput1();
	afx_msg void OnBnClickedRadioDigput2();
	afx_msg void OnCbnSelchangeCboxexToolzertype();
	afx_msg void OnBnClickedCheckEnableHFilter();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	
	CSizeLayoutManager layout_;

	DECLARE_MESSAGE_MAP()
};

#endif
