#ifndef __FILE_LIBRARY_EDITOR_DLG_H_INCLUDED__
#define __FILE_LIBRARY_EDITOR_DLG_H_INCLUDED__

#include "MFC\SizeLayoutManager.h"
#include "MFC\TreeListCtrl.h"

// CFileLibraryEditorDlg dialog

class CFileLibraryEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CFileLibraryEditorDlg)

public:
	CFileLibraryEditorDlg (const char* libraryPath, const char* filter);   // standard constructor
	virtual ~CFileLibraryEditorDlg();

	void updateList ();

// Dialog Data
	enum { IDD = IDD_FILE_LIBRARY_EDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	CTreeListCtrl m_ctlFileList;

private:
	CSizeLayoutManager m_Layout;

	std::string libraryPath_;
	std::string filter_;
public:
	afx_msg void OnAddButtonClicked();
	afx_msg void OnRemoveButtonClicked();
};

#endif
