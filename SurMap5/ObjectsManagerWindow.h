#ifndef __OBJECTS_MANAGER_WINDOW_H_INCLUDED__
#define __OBJECTS_MANAGER_WINDOW_H_INCLUDED__


#include "../Util/MFC/SizeLayoutManager.h"

class PopupMenu;
class WorldTreeObject;
class CObjectsManagerTree;
class CMainFrame;
class Player;

#include "EventListeners.h"

class CObjectsManagerWindow : public CWnd, 
	public WorldChangedListener, public ObjectChangedListener, public SelectionChangedListener
{
public:
	typedef CObjectsManagerWindow Self;
	struct UpdateLock{
		UpdateLock(CObjectsManagerWindow& objectsManager);
        ~UpdateLock();
	private:
		CObjectsManagerWindow& objectsManager_;
	};

    static const char* className() { return "VistaEngineObjectsManagerWindow"; }

	enum{
		TAB_SOURCES = 0,
		TAB_ENVIRONMENT,
		TAB_UNITS,
		TAB_CAMERA,
		TAB_ANCHORS,
        TAB_ALL
	};

	BOOL Create(DWORD style, const CRect& rect, CWnd* parentWnd);

	CObjectsManagerWindow(CMainFrame* mainFrame);
	virtual ~CObjectsManagerWindow();

	void selectInList();
	void selectOnWorld();
	void updateList();
    void rebuildList();

	int currentTab();

	// events
	void onWorldChanged();
	void onObjectChanged();
	void onSelectionChanged();
	// ^^^

	void onMenuApplyPreset();
	void onMenuPaste();
	void onMenuSortByName();
	void onMenuSortByTime();
	bool sortByName()const { return sortByName_; }

	PopupMenu& popupMenu() { return *popupMenu_; }

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTabChanged(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	afx_msg void OnObjectsTreeBeforeSelChanged(NMHDR *nm, LRESULT *result);
	afx_msg void OnObjectsTreeAfterSelChanged(NMHDR *nm, LRESULT *result);

	afx_msg void OnObjectsTreeKeyDown(NMHDR *nm, LRESULT *result);
	afx_msg void OnObjectsTreeClick(NMHDR *nm, LRESULT *result);
	afx_msg void OnObjectsTreeRClick(NMHDR *nm, LRESULT *result);
	afx_msg void OnObjectsTreeBeginLabelEdit(NMHDR* nm, LRESULT* result);
	void onObjectsTreeRightClick();

	void onMenuTabDeselectOthers();
	void onMenuTabSelectAll();
	void onMenuTabDeselectAll();
	void onMenuChangePlayer(Player* player);

	//Functor1<void, unsigned int,  signalKeyDown()
protected:
	CSizeLayoutManager layout_;
	bool skipSelChanged_;

	bool sortByName_;
	CTabCtrl typeTabs_;
	
	PtrHandle<CObjectsManagerTree> tree_;
	PtrHandle<PopupMenu> popupMenu_;

	DECLARE_MESSAGE_MAP()
private:
	typedef std::vector<CObjectsManagerWindow*> Instances;
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};

#endif
