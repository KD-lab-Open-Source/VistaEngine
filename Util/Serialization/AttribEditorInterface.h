#ifndef __ATTRIB_EDITOR_INTERFAFCE_H_INCLUDED__
#define __ATTRIB_EDITOR_INTERFAFCE_H_INCLUDED__

class TreeEditor;
class TreeNode;
class TreeNodeStorage;
class TreeNodeClipboard;
class TranslationManager;
struct TreeControlSetup;

class AttribEditorInterface{
public:
    virtual TreeEditor* createTreeEditor(const char* name) = 0;
	virtual bool isTreeEditorRegistered(const char* name) = 0;
	virtual void free(TreeNode const* node) = 0;
    virtual TreeNode* edit(const TreeNode* node, HWND wnd, TreeControlSetup& treeControlSetup) = 0;
	virtual TreeNodeClipboard& clipboard() = 0;

	virtual TreeNodeStorage& treeNodeStorage() = 0;
};

#endif
