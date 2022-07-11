#ifndef __CONTROLS_TREE_CTRL_H__INCLUDED__
#define __CONTROLS_TREE_CTRL_H__INCLUDED__

#include "..\..\Util\MFC\ObjectsTreeCtrl.h"

class UI_ControlState;
class UI_ControlContainer;
class UI_ControlBase;
class UI_Screen;

class PopupMenu;
class UITreeObject;
class CControlsTreeCtrl : public CObjectsTreeCtrl
{
public:
    CControlsTreeCtrl();
    virtual ~CControlsTreeCtrl();

    void buildTree();	

	void buildContainerSubtree(UITreeObject*);
    void buildContainerSubtree(UI_ControlContainer&);

    void updateSelected ();
    bool selectControl (UI_ControlBase* control);
    bool selectControl (UI_ControlBase* control, ItemType parent);
    void selectControlInScreen (UI_ControlBase* control, UI_Screen* screen);
    void selectState (UI_ControlBase* control, int stateIndex);
    void selectScreen (UI_Screen* screen);
	bool canBeDroppedIn (ItemType dest, ItemType item);

    int getControlImage (const std::type_info& info);

    struct Dummy{
        template<class Archive>
          void serialize(Archive& ar){
          }
    };
	void onRightClick();     

	void spawnMenu(TreeObject* object);
	PopupMenu& popupMenu() { return *popupMenu_; }

	void sortControls(UITreeObject* object);
    DECLARE_MESSAGE_MAP()
private:

	PopupMenu* popupMenu_;

    UI_ControlContainer* drop_target_;
    UI_ControlBase* dragged_control_;

    afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDragEnter(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDragLeave(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDragOver(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDrop(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemCheck(NMHDR* pNMHDR, LRESULT* pResult);
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};

#endif
