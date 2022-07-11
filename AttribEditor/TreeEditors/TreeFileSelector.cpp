#include "stdafx.h"
#include <typeinfo>
#include <direct.h>
#include <stdlib.h>
#include "TreeEditor.h"
#include "GenericFileSelector.h"
#include "EditArchive.h"
#include "..\AttribComboBox.h"

#include "FileUtils.h"

class TreeFileSelector : public TreeEditorImpl<TreeFileSelector, GenericFileSelector> {
public:
    virtual bool hideContent () const { return true; }

    bool invokeEditor (GenericFileSelector& selector, HWND wnd) {
        if (selector.onlyInitialDir ())
            return false;
		string filter = std::string("(") + selector.filter() + ")|" + selector.filter() + "||";
		CFileDialog fileDlg (TRUE, filter.c_str(), ::extractFileName(selector).c_str(), OFN_LONGNAMES|OFN_HIDEREADONLY|OFN_NOCHANGEDIR, filter.c_str(), CWnd::FromHandle(wnd));
        fileDlg.m_ofn.lpstrTitle = selector.title ();
        fileDlg.m_ofn.lpstrInitialDir = selector.initialDir ();

        if (fileDlg.DoModal () == IDOK) {
            selector.setFileName (fileDlg.GetPathName ());
            return true;
        } else {
            return false;
        }
    }

    CWnd* beginControl (CWnd* parent, const CRect& itemRect) {
        const GenericFileSelector& selector = getData ();
        if (!selector.onlyInitialDir ())
            return false;

        CAttribComboBox* pCombo = new CAttribComboBox ();
        CRect rcComboRect (itemRect);
        rcComboRect.bottom += 200;
        if (! pCombo->Create (WS_VISIBLE | WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST,
                              rcComboRect, parent, 0))
        {
            delete pCombo;
            return 0;
        }
        int selected = 0;

        WIN32_FIND_DATA findFileData;
        std::string dir = selector.initialDir ();
        if (dir.empty () || dir[dir.size () - 1] != '\\')		 
            dir += "\\";
        dir += selector.filter ();
        std::string currentFileName = extractFileName (selector);
        int index = 0;
        HANDLE fileHandle = FindFirstFile (dir.c_str (), &findFileData);
        if (fileHandle != INVALID_HANDLE_VALUE) {
            do {
                if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (findFileData.cFileName[0] == '.' &&
                        findFileData.cFileName[1] == '\0')
                        continue;
                    if (findFileData.cFileName[0] == '.' &&
                        findFileData.cFileName[1] == '.' &&
                        findFileData.cFileName[2] =='\0')
                        continue;

                } else {
                    if (stricmp (findFileData.cFileName, currentFileName.c_str ()) == 0)
                        selected = index;
                    pCombo->InsertString (-1, findFileData.cFileName);
                }
                ++index;
            } while (FindNextFile(fileHandle, &findFileData));
            FindClose (fileHandle);
        }
        pCombo->SetCurSel (selected);
        return pCombo;
    }

	bool canBeCleared() const{ return true; }
    void onClear(GenericFileSelector& selector) {
		selector.setFileName("");
	}

    void endControl(GenericFileSelector& selector, CWnd* control) {
        CAttribComboBox* pCombo = static_cast<CAttribComboBox*>(control);
        CString str;
        pCombo->GetWindowText (str);
        selector.setFileName ((std::string(selector.initialDir ()) + "\\" + static_cast<const char*>(str)).c_str ());
    }

    std::string nodeValue () const {
        return extractFileName (getData ());
    }
	Icon buttonIcon() const{
		return TreeEditor::ICON_FILE;
	}
};

REGISTER_CLASS_IN_FACTORY(TreeEditorFactory, typeid(GenericFileSelector).name (), TreeFileSelector);
