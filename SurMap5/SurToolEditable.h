#ifndef __SUR_TOOL_EDITABLE_H_INCLUDED__
#define __SUR_TOOL_EDITABLE_H_INCLUDED__

#include "EScroll.h"
#include "SurToolAux.h"

#include "..\Util\MFC\SizeLayoutManager.h"

class CAttribEditorCtrl;
class Archive;
class CSurToolEditable : public CSurToolBase {
	DECLARE_DYNAMIC(CSurToolEditable)
public:
	CSurToolEditable(CWnd* pParent = 0);
	virtual ~CSurToolEditable();

	void setLabels(const char* left, const char* right);

	CAttribEditorCtrl& attribEditor();

	virtual void onPropertyChanged() {}
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();

	int getIDD() const { return IDD_BARDLG_EDITABLE; }
protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
private:
	CAttribEditorCtrl* attribEditor_;
	CSizeLayoutManager layout_;
};

#endif
