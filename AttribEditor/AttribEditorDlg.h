#ifndef __ATTRIB_EDITOR_DLG_H_INCLUDED__
#define __ATTRIB_EDITOR_DLG_H_INCLUDED__

#include "AttribEditorCtrl.h"
#include "MFC\SizeLayoutManager.h"
#include "Serialization\Serializer.h"

class TreeNode;
struct TreeControlSetup {};

class CAttribEditorDlg : public CDialog, public ShareHandleBase
{
	DECLARE_DYNAMIC(CAttribEditorDlg)
public:
	CAttribEditorDlg(CWnd* pParent = NULL);
	virtual ~CAttribEditorDlg();

    const TreeNode* edit(const TreeNode* oldRoot, HWND wndParent, const TreeControlSetup& treeControlSetup); 
	
    bool edit(Serializer serializeable, HWND hwndParent, const TreeControlSetup& treeControlSetup,
			  const char* okLabel = 0, const char* cancelLabel = 0);

	CAttribEditorCtrl& attribEditorControl() {
		return attribEditor_;
	}

	void loadEditorState ();
	void saveEditorState ();
	
	void loadWindowGeometry(XStream& file);
	void saveWindowGeometry(XStream& file);
// Dialog Data
	enum { IDD = IDD_ATTRIB_EDITOR };
private:
    ShareHandle<TreeNode> rootNode_;
	TreeControlSetup treeControlSetup_;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
	CAttribEditorCtrl attribEditor_;
    Serializer serializeable_;
    CSizeLayoutManager layout_;

	const char* okLabel_;
	const char* cancelLabel_;
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
protected:
	virtual void OnOK();
};

#endif
