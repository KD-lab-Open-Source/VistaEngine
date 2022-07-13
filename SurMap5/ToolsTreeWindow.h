#ifndef __TOOLS_TREE_WINDOW_H_INCLUDED__
#define __TOOLS_TREE_WINDOW_H_INCLUDED__

#include "Handle.h"
#include "SComboBox.h"

class CSurToolBase;
class CToolsTreeCtrl;
class CMainFrame;
class Archive;

class CToolsTreeWindow : public CFrameWnd
{
public:
    CToolsTreeWindow();
    virtual ~CToolsTreeWindow();
	static const char* className() { return "VistaEngineToolsTreeWindow"; }
	
	CSurToolBase* currentTool();
	const CSurToolBase* currentTool() const;

    void replaceEditorMode(CSurToolBase* tool = 0);
	void pushEditorMode(CSurToolBase* tool);
	void popEditorMode();

	bool onKeyDown(UINT key, UINT flags);

	void rebuildTools();

	int getBrushRadius();
	void setBrushRadius(int rad);

	void enableTransformTools(bool enable);
	bool isToolFromTreeSelected() const;

	void previousBrush();
	void nextBrush();

	CToolsTreeCtrl& getTree() { return tree_; }

	enum {
		TOOL_SELECT,
		TOOL_MOVE,
		TOOL_ROTATE,
		TOOL_SCALE,
		TOOL_MAX
	};
	typedef std::vector < ShareHandle<CSurToolBase> > Tools;
	Tools& tools(){ return tools_; }

	BOOL Create(CMainFrame* parent, CExtControlBar* bar);
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg void OnDestroy();
	void PostNcDestroy() {}

	afx_msg void OnTool(UINT nID);
	afx_msg void OnUpdateTool(CCmdUI *pCmdUI);
	afx_msg void OnUpdateBrushRadius(CCmdUI* pCmdUI);
	afx_msg void OnBrushRadiusComboSelChanged();

    DECLARE_DYNAMIC(CToolsTreeWindow)
	DECLARE_MESSAGE_MAP()
private:

	void setCurrentTool(CSurToolBase* tool);
	void destroyToolWindows();

	bool transformToolsEnabled_;

	Tools tools_;

	CFont defaultFont_;
	CExtToolControlBar toolsBar_;  
	CSComboBox brushRadiusCombo_;
    CToolsTreeCtrl& tree_;
	
	CMainFrame* mainFrame_;

	typedef std::vector< ShareHandle<CSurToolBase> > EditorModeStack;
	EditorModeStack editorModeStack_;

public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};

#endif
