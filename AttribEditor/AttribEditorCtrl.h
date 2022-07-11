#ifndef __ATTRIB_EDITOR_CTRL_H_INCLUDED__
#define __ATTRIB_EDITOR_CTRL_H_INCLUDED__

#include <list>

class CTreeListCtrl;
class CTreeListItem;
class ICustomDrawNotifyListener;

class TreeNode;

class PopupMenu;
class PopupMenuItem;

#include "Serializeable.h"
#include "Serialization.h"

class TreeNode;

class TreeNodeStrings : public ShareHandleBase{
public:
	const char* load(XBuffer& buf);
	void clear();

	typedef std::list<std::string> Strings;
protected:
	Strings strings_;
};

class TreeNodeClipboard{
public:
	TreeNodeClipboard();
	~TreeNodeClipboard();

	bool typeNameInClipboard(const char* typeName, HWND obtainerWnd);
	bool empty() { return false; }
	void clear();

	int smartPaste(Serializeable serializeable, HWND obtainerWnd);
	void get(Serializeable serializeable, HWND obtainerWnd);
	void set(Serializeable serializeable, HWND publisherWnd);

	//int smartPaste(TreeNode* node, HWND obtainerWnd);
	const TreeNode* get(HWND obtainerWnd, TreeNodeStrings& strings);
	void set(const TreeNode* node, HWND publisherWnd);

	static TreeNodeClipboard& instance();
protected:
	bool publish(HWND wnd);
	bool retrieve(HWND wnd, TreeNodeStrings& strings);

	void saveTreeNode(XBuffer& buf, const TreeNode* node);
	TreeNode* loadTreeNode(XBuffer& buf, TreeNodeStrings& strings);

	ShareHandle<TreeNode> rootNode_;
	DWORD currentContentTicks_;
	UINT clipboardFormat_;
};

inline void FillComboMenu(const char* comboList, CMenu* parentMenu, UINT firstMenuID)
{
	ComboStrings::iterator it;
	ComboStrings strings;
	splitComboList(strings, comboList);
	unsigned int uPos = 0;
	FOR_EACH(strings, it)
		parentMenu->AppendMenu (MF_STRING, firstMenuID + uPos++, it->c_str ());

	parentMenu->RemoveMenu (firstMenuID, MF_BYCOMMAND);
}

class CAttribEditorState{
};

class CAttribEditorCtrl : public CWnd
{
	DECLARE_DYNCREATE(CAttribEditorCtrl)
public:
	typedef CTreeListItem* ItemType;
	typedef std::vector<ItemType> Items;
	enum{
		COLUMN_NAME,
		COLUMN_VALUE
	};
	enum{
		EXPAND_ALL      = 1 << 0,
        HIDE_ROOT_NODE  = 1 << 1,
        NO_DIGEST       = 1 << 2,
        COMPACT         = 1 << 3,
        AUTO_SIZE       = 1 << 4,
        DEEP_EXPAND     = 1 << 5,
        NO_HEADER       = 1 << 6,
        DISABLE_MENU    = 1 << 6,
	};

	CAttribEditorCtrl();
	virtual ~CAttribEditorCtrl();

	static const char* className() { return "SurMapAttribEditorCtrl"; }

	BOOL Create(DWORD style, const CRect& rect, CWnd* parent_wnd = 0, UINT id = 0);
	int initControl();

	void attachSerializeable(Serializeable& holder);
	void detachData();

	void setRootNode(const TreeNode* rootNode);
    const TreeNode* tree();
	bool isDataAttached() const;

	CTreeListCtrl& treeControl() { return tree_; }
	
	void mixIn(Serializeable& holder);
	void showMix();
	void resave();
	static ItemType rootItem();
	void expandNodes(int levels, ItemType item = rootItem());

	void loadExpandState(XBuffer& file);
    void saveExpandState(XBuffer& file);
	void loadExpandState(XStream& file);
    void saveExpandState(XStream& file);

	bool getItemPath(ComboStrings& result, ItemType item);
	bool selectItemByPath(const ComboStrings& path);
    
	void setStatusLabel(CStatic* label) { statusLabel_ = label; }

	// Options
    void setStyle(int newStyle) { style_ = newStyle; }
    void addStyle(int add) { style_ |= add; }
    void removeStyle(int remove) { style_ ^= remove; }
    int  style() { return style_; }

	static void PasteSingleNode (TreeNode& dest, const TreeNode& source);

	static TreeNodeClipboard& clipboard();
	CImageList& imageList() { return imageList_; }

	void copyToClipboard(const TreeNode* node);
protected:
    inline TreeNode* getNode(ItemType item) const;

	void onMenuFollowElement(ItemType item);
	virtual bool beforeElementEdit(ItemType item, bool middleButton) { return true; };
    virtual void onElementChanged(ItemType item);

	virtual void onMenuConstruction(ItemType item, PopupMenuItem& root){}
	virtual void onElementSelected(){}
	virtual void beforeResave() {};
    virtual void onBeforeJumping(ItemType item);

    void onMenuRemoveSelectedElements();
    void onMenuInsertElement(ItemType beforeItem);

	void onMenuPointerCreate(int type_index);
    void onMenuPointerDelete();
    void onMenuEdit();

	void onMenuMoveUp();
	void onMenuMoveDown();

    void onMenuAppendElement();
    void onMenuClearContainer();

	// afx
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();

	afx_msg void OnRClick (NMHDR* pNotifyStruct, LRESULT* plResult);
	afx_msg void OnClick (NMHDR* pNotifyStruct, LRESULT* plResult);
	afx_msg void OnKeyDown (NMHDR* pNotifyStruct, LRESULT* plResult);

	afx_msg void OnSelChanged (NMHDR* pNotifyStruct, LRESULT* plResult);
    afx_msg void OnItemExpanding (NMHDR* pNotifyStruct, LRESULT* plResult);
	afx_msg void OnBeginLabelEdit (NMHDR* pNotifyStruct, LRESULT* plResult);
	afx_msg void OnSubItemUpdated (NMHDR* pNotifyStruct, LRESULT* plResult);
	afx_msg void OnBeginControl (NMHDR* pNotifyStruct, LRESULT* plResult);
	afx_msg void OnEndControl (NMHDR* pNotifyStruct, LRESULT* plResult);
	afx_msg void OnRequestCtrlType (NMHDR* pNotifyStruct, LRESULT* plResult);

	afx_msg void OnBeginDrag(NMHDR* nm, LRESULT* result);
	afx_msg void OnDragLeave(NMHDR* nm, LRESULT* result);
	afx_msg void OnDragEnter(NMHDR* nm, LRESULT* result);
	afx_msg void OnDragOver(NMHDR* nm, LRESULT* result);
	afx_msg void OnDrop(NMHDR* nm, LRESULT* result);

	virtual void PreSubclassWindow();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	// ^^^

	typedef std::vector<Serializeable> DataHolders;
	DataHolders dataHolders_;

	DECLARE_MESSAGE_MAP()
private:
	void disconnectNodes(ItemType root = rootItem());

	void load(Serializeable& holder);
	void save(Serializeable& holder);
	void setStatusText(const char* text);

	void onAttachData ();


	void onMenuCopy();
    void onMenuPaste();
	bool OnClick(ItemType item, int column, bool middleButton);

    bool CanBePastedOn(const TreeNode& dest, const TreeNode& src) const;
    bool CanBePastedInto(const TreeNode& dest, const TreeNode& src) const;
	bool CanBePastedOnSingleTypeSelection (const TreeNode& source);

    void InsertIntoContainer(ItemType, TreeNode* beforeNode, TreeNode* insertWhat);
	void AppendContainer(ItemType, TreeNode* appendWhat);
    void removeFromContainer(ItemType, TreeNode* node);
	void swapContainerItems(ItemType containterItem, TreeNode* firstNode, TreeNode* secondNode);
    void ClearContainer(ItemType);

	ItemType findItemByNode(const TreeNode* node, ItemType root);
	ItemType childByIndex(ItemType item, int index);
	void updateContainerItem(ItemType containerItem);

	void fillItem(TreeNode* node, ItemType item, bool deepExpand = false);
	void updateItemText(ItemType item);
	void rebuildClickedItem();
	void RemoveChilds(ItemType parent);

	void setItemImage(TreeNode* node, ItemType new_item);
	ItemType insertItem(const char* name, const char* type, ItemType parent, ItemType previous, Items* unusedItems = 0);
	ItemType makeItemByNode(TreeNode* element, ItemType root, ItemType previous, bool deepExpand = false, Items* unusedItems = 0);
	CWnd* makeCustomEditor(ItemType pItem);

	void spawnRootMenu();
	void spawnContextMenu();
	
    bool isEditableContainer(ItemType pItem) const;
	bool isEditableContainerElement(ItemType pItem) const;
	bool isPointer(ItemType pItem) const;

	ItemType								draggedItem_;
    static TreeNodeClipboard			    clipboard_;
    CFont		                    		defaultFont_;
    CWnd*		                    		ctrlWnd_;
	CTreeListItem*                  		editItem_;
	PtrHandle<PopupMenu>					popupMenu_;
	PtrHandle<ICustomDrawNotifyListener>	drawNotifyListener_;

	CTreeListCtrl&							tree_;
	TreeNodeStrings							strings_;
    ShareHandle<TreeNode>					rootNode_;
    ShareHandle<TreeNode>					oldRootNode_;

	CImageList								imageList_;

    int										style_;
	CStatic*								statusLabel_;
};


void saveTreeNode(TreeNode* node, XBuffer& buf);

TreeNode* getNode(CAttribEditorCtrl::ItemType pItem);

#endif
