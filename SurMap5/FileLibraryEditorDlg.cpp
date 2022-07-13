#include "stdafx.h"
#include "FileLibraryEditorDlg.h"
#include "SurToolAux.h" // for requestResourceAndPut2InternalResource
#include "FileUtils\FileUtils.h"
#include "filelibraryeditordlg.h"

IMPLEMENT_DYNAMIC(CFileLibraryEditorDlg, CDialog)

CFileLibraryEditorDlg::CFileLibraryEditorDlg(const char* libraryPath, const char* filter)
: CDialog(CFileLibraryEditorDlg::IDD, 0)
, libraryPath_ (libraryPath)
, filter_ (filter)
{
}

CFileLibraryEditorDlg::~CFileLibraryEditorDlg()
{
}

void CFileLibraryEditorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILE_LIST, m_ctlFileList);
}


BEGIN_MESSAGE_MAP(CFileLibraryEditorDlg, CDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_ADD_BUTTON, OnAddButtonClicked)
	ON_BN_CLICKED(IDC_REMOVE_BUTTON, OnRemoveButtonClicked)
END_MESSAGE_MAP()


// CFileLibraryEditorDlg message handlers

void CFileLibraryEditorDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (!m_Layout.isInitialized ()) {
		m_Layout.init(this);
		m_Layout.add(true, true, true, true, IDC_FILE_LIST);
		m_Layout.add(true, false, true, true, IDC_HLINE);
		m_Layout.add(true, false, false, true, IDC_ADD_BUTTON);
		m_Layout.add(true, false, false, true, IDC_REMOVE_BUTTON);
		m_Layout.add(false, false, true, true, IDOK);
	}
	m_Layout.onSize (cx, cy);

	CRect rect;
	m_ctlFileList.GetClientRect (&rect);
	m_ctlFileList.SetColumnWidth (0, rect.Width () - GetSystemMetrics (SM_CXVSCROLL));
	Invalidate ();
}

BOOL CFileLibraryEditorDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ctlFileList.SetStyle (TLC_HEADER | TLC_SHOWSELALWAYS);

	m_ctlFileList.InsertColumn ("File", TLF_DEFAULT_LEFT, 200);
	m_ctlFileList.SetColumnModify (0, TLM_REQUEST);

    updateList ();

	return TRUE;
}

void CFileLibraryEditorDlg::updateList ()
{
	m_ctlFileList.DeleteAllItems();
	std::string path = libraryPath_ + "\\" + filter_;
    DirIterator iter (path.c_str());
	for(; iter != DirIterator::end; ++iter) {
        if (iter.isFile ()) {
            m_ctlFileList.InsertItem (*iter);
        }
    }
}

void CFileLibraryEditorDlg::OnAddButtonClicked()
{
	std::string result = requestResourceAndPut2InternalResource (libraryPath_.c_str(), filter_.c_str(), "",
											TRANSLATE("Пожалуйста, выберите файл который вы желаете добавить в библиотеку..."));
	if (!result.empty()) {
		updateList ();
	}
}

void CFileLibraryEditorDlg::OnRemoveButtonClicked()
{
	if (m_ctlFileList.GetSelectedItem ()) {
		std::string fullPath = libraryPath_ + "\\" + m_ctlFileList.GetItemText (m_ctlFileList.GetSelectedItem ());
		int result = MessageBox (TRANSLATE("Вы действительно желаете удалить этот файл из библиотеки и с жесткого диска?"), 0, MB_ICONQUESTION | MB_YESNO);
		if (result = IDYES) {
			DeleteFile (fullPath.c_str());
			updateList ();
		} else {
		}
	}
}
