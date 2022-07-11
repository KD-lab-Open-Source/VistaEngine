#ifndef __TRIGGER_PROF_LIST_H_INCLUDED__
#define __TRIGGER_PROF_LIST_H_INCLUDED__

#include "TEEngine/Profiler/ITriggerProflist.h"
#include <memory>

class CExtControlBar;
class TriggerDbgDlg;
interface ITriggerEditorView;
interface ITriggerChainProfiler;

class TriggerProfList :
	public ITriggerProfList
{
public:
	TriggerProfList(void);
	~TriggerProfList(void);

	bool create(CFrameWnd* pParent, 
								DWORD dwStyle = WS_CHILD |CBRS_FLYBY | 
								CBRS_RIGHT | CBRS_SIZE_DYNAMIC);

	bool show()const;
	bool hide()const;
	bool isVisible()const;

	bool load();
	bool next()const;
	bool prev()const;
	bool activate();
	bool canBeActivated() const;

	void dock(UINT uiDocBarId);
	void enableDocking(bool enable);
	void setTriggerEditorView(ITriggerEditorView* ptrTEView);
	void setTriggerChainProfiler(ITriggerChainProfiler* ptrITriggerChainProf);

private:
	TriggerDbgDlg& triggerDbgDlg_;
	CExtControlBar& controlBar_;
	CFrameWnd* parentFrame_;
};

#endif
