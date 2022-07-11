#ifndef __CONFIGURATOR_DLG_H_INCLUDED__
#define __CONFIGURATOR_DLG_H_INCLUDED__

#include "Handle.h"
class CAttribEditorCtrl;
class LayoutWindow;

class CConfiguratorDlg : public CDialog
{
public:
	DECLARE_DYNAMIC(CConfiguratorDlg)

	CConfiguratorDlg(CWnd* pParent = NULL);
	~CConfiguratorDlg();
	enum { IDD = IDD_CONFIGURATOR_DIALOG };

    void onConfigChanged();
protected:
	void updateControlsLanguage();

    PtrHandle<LayoutWindow> layout_;
    PtrHandle<CAttribEditorCtrl> attribEditor_;
	HICON icon_;

    // virtuals:
	BOOL OnInitDialog();
	void DoDataExchange(CDataExchange* pDX);

    // handlers:
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
public:
	afx_msg void OnCloseButtonClicked();
	afx_msg void OnSaveButtonClicked();
};


#endif
