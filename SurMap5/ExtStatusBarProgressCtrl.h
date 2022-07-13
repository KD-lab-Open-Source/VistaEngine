#ifndef __EXT_STATUS_BAR_PROGRESS_CTRL__
#define __EXT_STATUS_BAR_PROGRESS_CTRL__

class CExtStatusBarProgressCtrl : public CProgressCtrl{
protected:
	virtual LRESULT WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif
