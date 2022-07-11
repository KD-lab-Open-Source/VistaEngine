#ifndef __TRIGGER_EDITOR_H_INCLUDED__
#define __TRIGGER_EDITOR_H_INCLUDED__

#ifndef _FINAL_VERSION_

#pragma warning(disable: 4251)
#ifdef _TRIGGER_EDITOR
#define DLL_INTERFACE __declspec(dllexport)
#else
#define DLL_INTERFACE __declspec(dllimport)
//#pragma comment(lib,"TriggerEditor.lib")
#endif

class TEFrame;
class TETreeManager;
class TETreeLogic;
class TriggerEditorLogic;
class TriggerChainProfiler;
class TriggerProfList;

interface ITriggerEditorView;
class TriggerInterface;
class TriggerChain;

class DLL_INTERFACE TriggerEditor  
{
public:
	TriggerEditor(TriggerInterface& triggerInterface);
	virtual ~TriggerEditor();

	bool run(TriggerChain& triggerChain, HWND hWnd, bool hideParent = false);

	static bool isOpen() { return open_; }
	bool isDataSaved() const;
	void setDataChanged(bool value = true);
	bool save();

protected:
	bool createFrame(TEFrame* frame, HWND wndParent);
	void closeFrame(TEFrame* frame);
	bool initTETree(TETreeManager* mgr, TETreeLogic* logic);
	bool initTriggerEditorLogic(TriggerEditorLogic& logic, 
								TEFrame& frame,
								TriggerChainProfiler& triggerChainProfiler);
	bool initTriggerProfList(TriggerProfList* ptrTriggerProfList,
							TriggerChainProfiler& triggerChainProfiler, 
							ITriggerEditorView* ptrTriggerEditorView);
	void setChangesWasOnceSaved(bool value = true);
	bool getChangesWasOnceSaved() const;

private:
	bool dataChanged_;
	static bool open_;
	//! Изменения были хотя бы единожды сохранены
	bool changesWasOnceSaved_;
};

#endif

#endif
