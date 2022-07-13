#ifndef __CONDITION_EDITOR_DLG_H_INCLUDED__
#define __CONDITION_EDITOR_DLG_H_INCLUDED__

#include "ConditionEditor.h"
#include "..\Util\MFC\SizeLayoutManager.h"

#include "ConditionList.h"

class CConditionEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CConditionEditorDlg)

public:
	CConditionEditorDlg(EditableCondition& condition, CWnd* pParent = NULL);
	virtual ~CConditionEditorDlg();
	const EditableCondition& condition() { return m_ctlEditor.condition(); }

	enum { IDD = IDD_CONDITION_EDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
public:
	EditableCondition& condition_;
	CSizeLayoutManager layout_;
	CConditionEditor m_ctlEditor;
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	CConditionList m_ctlConditionList;
};

#endif
