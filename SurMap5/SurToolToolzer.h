#ifndef __SUR_TOOL_TOOLZER_H_INCLUDED__
#define __SUR_TOOL_TOOLZER_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"

// CSurToolToolzer dialog

class CSurToolToolzer : public CSurToolBase
{
	DECLARE_DYNAMIC(CSurToolToolzer)
public:
	CSurToolToolzer(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSurToolToolzer();

	CEScroll m_RadiusToolzer;
	CEScrollVx m_DeltaH;
	CEScroll m_LevelingH;
	CEScroll m_Roughness;
	unsigned int state_radio_button_DigPut;
	unsigned int state_level_ZL_or_level_h;
	unsigned int state_free_or_level_h;

	int idxCurToolzerType;

	virtual bool CallBack_OperationOnMap(int x, int y);
	virtual bool CallBack_DrawAuxData(void);

	void serialize(Archive& ar);

// Dialog Data
	int getIDD() const { return IDD_BARDLG_TOOLZER; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL Create(UINT nIDTemplate, CWnd* pParentWnd = NULL);
	afx_msg void OnBnClickedRadioDigput1();
	afx_msg void OnBnClickedRadioDigput2();
	afx_msg void OnBnClickedRdbLeveling1();
	afx_msg void OnBnClickedRdbLeveling2();
	afx_msg void OnCbnSelchangeCboxexToolzertype();
	afx_msg void OnDestroy();
};

#endif
