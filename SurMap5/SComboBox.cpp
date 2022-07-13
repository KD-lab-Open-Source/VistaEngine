// SComboBox.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "SComboBox.h"
#include "scombobox.h"



// CSComboBox

IMPLEMENT_DYNAMIC(CSComboBox, CExtComboBox)
CSComboBox::CSComboBox()
{
}

CSComboBox::~CSComboBox()
{
}


BEGIN_MESSAGE_MAP(CSComboBox, CExtComboBox)
END_MESSAGE_MAP()



// CSComboBox message handlers


void CSComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	ASSERT(lpDrawItemStruct->CtlType == ODT_COMBOBOX);
//	int di=GetItemData(lpDrawItemStruct->itemID);
//	void* pdi=GetItemDataPtr(lpDrawItemStruct->itemID);
//	SetItemData(lpDrawItemStruct->itemID, di);
//	SetItemDataPtr(lpDrawItemStruct->itemID, pdi);
//	LPCTSTR lpszText = (LPCTSTR) lpDrawItemStruct->itemData;
	///ASSERT(lpszText != NULL);
	CDC dc;

	dc.Attach(lpDrawItemStruct->hDC);

	// Save these value to restore them when done drawing.
	COLORREF crOldTextColor = dc.GetTextColor();
	COLORREF crOldBkColor = dc.GetBkColor();

	// If this item is selected, set the background color 
	// and the text color to appropriate values. Erase
	// the rect by filling it with the background color.
	if ((lpDrawItemStruct->itemAction | ODA_SELECT) && (lpDrawItemStruct->itemState  & ODS_SELECTED)){
		dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
		dc.FillSolidRect(&lpDrawItemStruct->rcItem, ::GetSysColor(COLOR_HIGHLIGHT));
	}
	else
        dc.FillSolidRect(&lpDrawItemStruct->rcItem, crOldBkColor);

	// Draw the text.
	//dc.DrawText( lpszText, strlen(lpszText), &lpDrawItemStruct->rcItem, DT_CENTER|DT_SINGLELINE|DT_VCENTER);
	int rad=lpDrawItemStruct->itemData;
	char buf[20];
	itoa(rad, buf,10);
	dc.DrawText( buf, (int)strlen(buf), &lpDrawItemStruct->rcItem, DT_LEFT/*DT_CENTER*/|DT_SINGLELINE|DT_VCENTER);
	CRect drawArea=lpDrawItemStruct->rcItem;
	if(rad<=15){
		CPoint cntr=drawArea.CenterPoint();
		cntr.x+=10;
		CRect cyrcle(CPoint(cntr.x-rad-1, cntr.y-rad-1), CPoint(cntr.x+rad, cntr.y+rad));
		dc.Ellipse(&cyrcle);
	}

	// Reset the background color and the text color back to their
	// original values.
	dc.SetTextColor(crOldTextColor);
	dc.SetBkColor(crOldBkColor);

	dc.Detach();

}

BOOL CSComboBox::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style|=CBS_OWNERDRAWVARIABLE;
	return __super::PreCreateWindow(cs);
}

void CSComboBox::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	ASSERT(lpMeasureItemStruct->CtlType == ODT_COMBOBOX);
	lpMeasureItemStruct->itemHeight=16;

}

void CSComboBox::DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct)
{
	__super::DeleteItem(lpDeleteItemStruct);
}
