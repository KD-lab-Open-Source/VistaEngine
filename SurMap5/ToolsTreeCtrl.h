#ifndef __TOOLS_TREE_CTRL_H_INCLUDED__
#define __TOOLS_TREE_CTRL_H_INCLUDED__

#include "Handle.h"

#include "TreeInterface.h"

class CSurToolBase;
class CMainFrame;
class Archive;
class PopupMenu;

typedef vector< ShareHandle<CSurToolBase> > SurTools;

class CToolsTreeCtrl : public CTreeCtrl
{
	DECLARE_DYNAMIC(CToolsTreeCtrl)

public:
	CToolsTreeCtrl();
	virtual ~CToolsTreeCtrl();
	void init(CMainFrame* mainFrame);
	void save();

	PopupMenu& popupMenu() { return *popupMenu_; }

	CSurToolBase* findNode(HTREEITEM item);
	CSurToolBase* findTool(const ComboStrings& path);

	CSurToolBase* getSelectedTool(){ return pCurrentNode; }
	bool getToolPath(ComboStrings& path, CSurToolBase* tool);

	void destroyToolWindows();
	void DestroyAllTreeWndAux(CSurToolBase* pNode);

	void DeleteTreeNode(CSurToolBase* pDeleteNode);

	void AddNode2Tree(CSurToolBase* pSTB);
	void Create_PushBrowse_And_DestroyIfErr(CSurToolBase* pSTB);

	void serialize(Archive& ar);

	void BuildTree();
	void buildNode(CSurToolBase* pNode, HTREEITEM parent, TV_INSERTSTRUCT& ins);//служебная функция построения дерева

	void onMenuCreateTool(int index);
	void onMenuDeleteTool(CSurToolBase* tool);
	void onMenuProperties(CSurToolBase* tool);

	virtual BOOL DestroyWindow();

	afx_msg void OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnRClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTvnBeginLabelEdit (NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnPopupDelete();
	afx_msg void OnPopupCreate3dm();
	afx_msg void OnPopupCreateColorPic();
	afx_msg void OnPopupCreate3dmOnWorld();

	afx_msg void OnPopupAddm3d();
	afx_msg void OnPopAddColorPic();
	afx_msg void OnPopAddM3d2w();
	afx_msg void OnPopSort();
	afx_msg void OnDestroy();
	
	ComboStrings shortcuts_[10];
protected:
	CSurToolBase* findNodeAux(CSurToolBase* tool, HTREEITEM item);
	bool getToolPathAux(ComboStrings& path, SurTools* tools, CSurToolBase* tool);

	CImageList m_ImgList;
	PtrHandle<PopupMenu> popupMenu_;
	CMainFrame* mainFrame_;
	SurTools tools_;
	CSurToolBase* pCurrentNode;
	HTREEITEM curTreeItemRClick;
	bool flag_tree_build;

	DECLARE_MESSAGE_MAP()
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};

#endif
