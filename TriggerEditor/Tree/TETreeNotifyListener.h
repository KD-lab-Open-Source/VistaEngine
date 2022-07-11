#ifndef __TE_TREE_NOTIFY_LISTENER_H_INCLUDED__
#define __TE_TREE_NOTIFY_LISTENER_H_INCLUDED__

class TETreeNotifyListener
{
public:
	virtual bool onNotify(WPARAM wParam, LPARAM lParam, LRESULT *pResult) = 0;
	virtual bool onCommand(WPARAM wParam, LPARAM lParam)  = 0;
};

#endif
