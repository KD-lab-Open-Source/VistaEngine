/*
Original code by : Mihai Filimon
Modifications by S. Sridhar 
1. Added a edit control where the user can type in the path
2. If the path typed in the edit ctrl does not exist then the 
   user will be propmted as to whether he/she wants the path 
   to be created
3. Setting the flag bShowFilesInDir to TRUE will result in all 
   the files in the current folder to be displayed in the dialog
4. If u don't want to display all the files then u can use the 
   file filter to display the file types u want to display
5. Calling API SetTitle with the desired title will set the Title 
   of the dialog. This API has to be invoked before DoModal is called
6. User can pass the Initial Folder to be displayed in the constructor 
   of CSelectFolder

Usage Examples
--------------

CSelectFolderDialog oSelectFolderDialog(FALSE, NULL,
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						NULL, NULL);

CSelectFolderDialog oSelectFolderDialog(FALSE, "c:\\my documents",
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						NULL, NULL);

CSelectFolderDialog oSelectFolderDialog(TRUE, "c:\\my documents",
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						NULL, NULL);

CSelectFolderDialog oSelectFolderDialog(TRUE, "c:\\my documents",
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	"Microsoft Word Documents (*.doc)|*.doc|Microsoft Excel Worksheets (*.xls)|*.xls|", NULL);

CSelectFolderDialog oSelectFolderDialog(TRUE, "c:\\my documents",
						OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
	"HTML Files (*.html, *.htm)|*.html;*.htm||", NULL);
*/


#ifndef __SELECT_FOLDER_DIALOG_H_INCLUDED__
#define __SELECT_FOLDER_DIALOG_H_INCLUDED__

class CSelectFolderDialog : public CFileDialog
{
	DECLARE_DYNAMIC(CSelectFolderDialog)

public:
	CSelectFolderDialog(BOOL bShowFilesInDir = FALSE, 
				LPCSTR lpcstrInitialDir = NULL, 
				DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
				LPCTSTR lpszFilter = NULL, CWnd* pParentWnd = NULL);
	~CSelectFolderDialog();

	static WNDPROC m_wndProc;
	virtual void OnInitDone();

	void OnFolderChange();

	void SetSelectedPath(LPCSTR lpcstrPath);
	CString GetSelectedPath() const;

	void SetTitle(CString cstrTitle);

protected:
	//{{AFX_MSG(CSelectFolderDialog)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	BOOL m_bShowFilesInDir;
	CString m_cstrPath;
	CEdit *m_pEdit;
	CString m_cstrTitle;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
#endif
