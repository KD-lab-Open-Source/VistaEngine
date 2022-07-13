#include "StdAfx.h"
#include "LibraryTabCtrl.h"

IMPLEMENT_DYNAMIC(CLibraryTabCtrl, CTabCtrl)
BEGIN_MESSAGE_MAP(CLibraryTabCtrl, CTabCtrl)
	ON_WM_MBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

CLibraryTabCtrl::CLibraryTabCtrl()
{
    
}

CLibraryTabCtrl::~CLibraryTabCtrl()
{
}

void CLibraryTabCtrl::OnRButtonDown(UINT flags, CPoint point)
{
	TCHITTESTINFO htinfo;
	ZeroMemory(&htinfo, sizeof(htinfo));
	htinfo.pt = point;
	htinfo.flags = TCHT_ONITEM;
	int index = HitTest(&htinfo);


	if(index >= 0 && htinfo.flags & TCHT_ONITEMLABEL){
		if(signalMouseButtonDown_)
			signalMouseButtonDown_(index, flags);
	}
	CTabCtrl::OnRButtonDown(flags, point);
}

void CLibraryTabCtrl::OnMButtonDown(UINT flags, CPoint point)
{
	TCHITTESTINFO htinfo;
	ZeroMemory(&htinfo, sizeof(htinfo));
	htinfo.pt = point;
	htinfo.flags = TCHT_ONITEM;
	int index = HitTest(&htinfo);


	if(index >= 0 && htinfo.flags & TCHT_ONITEMLABEL){
		if(signalMouseButtonDown_)
			signalMouseButtonDown_(index, flags);
	}
	CTabCtrl::OnMButtonDown(flags, point);
}
