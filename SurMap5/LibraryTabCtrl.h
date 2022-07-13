#ifndef __LIBRARY_TAB_CTRL_H_INCLUDED__
#define __LIBRARY_TAB_CTRL_H_INCLUDED__

#include "XTL\Functor.h"

class CLibraryTabCtrl : public CTabCtrl{
    DECLARE_DYNAMIC(CLibraryTabCtrl);
public:
    CLibraryTabCtrl();
    ~CLibraryTabCtrl();

	Functor2<void, int, UINT>& signalMouseButtonDown() { return signalMouseButtonDown_; }

	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP();
private:
	Functor2<void, int, UINT> signalMouseButtonDown_;
};

#endif
