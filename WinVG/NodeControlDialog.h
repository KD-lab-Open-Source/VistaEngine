#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// CNodeControlDialog dialog

class CNodeControlDialog : public CDialog
{
	DECLARE_DYNAMIC(CNodeControlDialog)

public:
	CNodeControlDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CNodeControlDialog();

// Dialog Data
	enum { IDD = IDD_NODE_CONTROL_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
	DECLARE_MESSAGE_MAP()
public:
	void ModelChange(cObject3dx* obj,cObject3dx* logic_obj);
	void UpdateInfo();
	Se3f nodePose_X(float ang);
	Se3f nodePose_Y(float ang);
	Se3f nodePose_Z(float ang);
	cObject3dx* pObject3dx;
	cObject3dx* pLogicObject3dx;
	CListBox m_NodeList;
	CSliderCtrl m_XSlider;
	CSliderCtrl m_YSlider;
	CSliderCtrl m_ZSlider;
	float r_x,r_y,r_z;
	afx_msg void OnNMThemeChangedXSlider(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLbnSelchangeNodeList();
	afx_msg void OnBnClickedResetButton();
	afx_msg void OnBnClickedReselSelectedButton();
	CString m_XStatic;
	CString m_YStatic;
	CString m_ZStatic;
	CListBox m_LogicNodeList;
	afx_msg void OnBnClickedDeselectLogicNode();
	afx_msg void OnBnClickedDeselectNode();
	CButton check_graph_by_logic;
	afx_msg void OnBnClickedCheckGraphByLogicNode();

	void Update();
//	virtual BOOL OnInitDialog();
};
