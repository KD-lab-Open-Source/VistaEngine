#ifndef __LIBRARY_EDITOR_WINDOW_H_INCLUDED__
#define __LIBRARY_EDITOR_WINDOW_H_INCLUDED__

#include "Handle.h"
#include "TreeInterface.h"
#include "LibraryBookmark.h"
#include "LibraryTabCtrl.h"

#include "Serializeable.h"

class PopupMenu;
class LayoutWindow;
class LayoutHBox;
class CLibraryEditorTree;
class CLibraryAttribEditor;
class EditorLibraryInterface;
class LibraryTabBase;
class LibraryTabEditable;
class TreeNode;
// --------------------------------------------------------------------------
class CLibraryEditorWindow : public CWnd{
	DECLARE_DYNAMIC(CLibraryEditorWindow)
public:
	typedef CLibraryEditorWindow Self;
	BOOL Create(DWORD style, const RECT& rect, CWnd* parent, UINT id = 0);
	bool doModal(const char* libraryName = "");

    CLibraryEditorWindow(CWnd* parent = 0);
    virtual ~CLibraryEditorWindow();

	void serialize(Archive& ar);

	void openBookmark(LibraryBookmark bookmark, bool enableHistory);

    CLibraryAttribEditor& attribEditor() { return *attribEditor_; }
    CLibraryEditorTree& tree() { return *tree_; }

    void onElementCopied(const char* name, Serializeable serializeable);
    void onElementPasted(Serializeable serializeable);

	static const char* className(){ return "VistaEngineLibraryEditorWindow"; }

    typedef std::vector< ShareHandle<LibraryTabBase> > Tabs;
	typedef std::vector<LibraryBookmark> Bookmarks;
	
    void setLayout(bool singleLayout);

	LibraryTabBase* currentTab() { return currentTab_; }

	void addTab(LibraryTabBase* tab);
	void setCurrentTab(LibraryTabBase* tab, bool enableHistory);
    void updateTabs();

	void onProgress(float progress);
protected:

	void onBackButton();
	void onSaveButton();
	void onCloseButton();

	/// возвращает следующую за ней вкладку
	LibraryTabBase* closeTab(LibraryTabBase* tab);

	void onSelectLibraryButton();
	void openLibrary(const char* libraryName, const char* elementName = "");
	void addToHistory(const LibraryBookmark& bookmark);

    void onTabMouseButtonDown(int tabIndex, UINT flags);
	void onTabClose(LibraryTabBase* tab);
    afx_msg void onTabChange(NMHDR *nmhdr, LRESULT *result);

    DECLARE_MESSAGE_MAP()
private:
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnClose();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);

	bool modalResult_;
	Bookmarks history_;
    Tabs openTabs_;
	LibraryTabBase* currentTab_;
	std::string libraryToOpen_;

    // layout:
	PtrHandle<LayoutWindow> layout_;
	LayoutHBox* hbox_;

    // controls:
    CLibraryTabCtrl tabs_;

	CButton backButton_;
    CButton selectLibraryButton_;
	CButton saveButton_;
	CButton closeButton_;
	CProgressCtrl progressBar_;

	CWnd* parent_;

    PtrHandle<CLibraryEditorTree> tree_;
    PtrHandle<CLibraryAttribEditor> attribEditor_;
	PtrHandle<PopupMenu> popupMenu_;
};

#endif
