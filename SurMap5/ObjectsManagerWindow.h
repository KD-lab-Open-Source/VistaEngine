#ifndef __OBJECTS_MANAGER_WINDOW_H_INCLUDED__
#define __OBJECTS_MANAGER_WINDOW_H_INCLUDED__


#include "mfc/SizeLayoutManager.h"
#include "XTL/sigslot.h"

class PopupMenu;
class WorldTreeObject;
class CObjectsManagerTree;
class CMainFrame;
class Player;

#include "EventListeners.h"

class CObjectsManagerWindow : public CWnd, public sigslot::has_slots,
	public WorldObserver, public ObjectObserver, public SelectionObserver
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


	BOOL Create(DWORD style, const CRect& rect, CWnd* parentWnd);

	CObjectsManagerWindow(CMainFrame* mainFrame);
	virtual ~CObjectsManagerWindow();

	int currentTab();

	// events
	void onWorldChanged(WorldObserver* changer);
	void onObjectChanged(ObjectObserver* changer);
	void onSelectionChanged(SelectionObserver* changer);
	// ^^^

	PopupMenu& popupMenu() { return *popupMenu_; }

	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnTabChanged(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);

	afx_msg void OnObjectsTreeRClick(NMHDR *nm, LRESULT *result);

	void onMenuTabDeselectOthers();
	void onMenuTabSelectAll();
	void onMenuTabDeselectAll();

	CObjectsManagerTree* tree();
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
