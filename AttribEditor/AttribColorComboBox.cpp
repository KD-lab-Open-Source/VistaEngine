#include "stdafx.h"
#include "..\Util\MFC\TreeListCtrl.h"
#include "AttribColorComboBox.h"
#include "AttribEditorCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CAttribColorComboBox::CAttribColorComboBox()
{
}

CAttribColorComboBox::~CAttribColorComboBox()
{
}

void DDX_ColourPickerCB( CDataExchange *pDX, int nIDC, COLORREF& rgbColor )
{
	HWND hWndCtrl = pDX->PrepareCtrl( nIDC );
	ASSERT( hWndCtrl );

	CAttribColorComboBox *pPicker = (CAttribColorComboBox*)CWnd::FromHandle( hWndCtrl );
	ASSERT( pPicker );

	// only support getting of colour.
	if( pDX->m_bSaveAndValidate )
	{
		rgbColor = pPicker->GetSelectedColourValue();
	}
}


BEGIN_MESSAGE_MAP(CAttribColorComboBox, CComboBox)
	ON_CONTROL_REFLECT(CBN_CLOSEUP, OnCbnCloseup)
	ON_CONTROL_REFLECT(CBN_SETFOCUS, OnCbnSetfocus)
	ON_WM_CREATE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAttribColorComboBox message handlers

void CAttribColorComboBox::PreSubclassWindow() 
{
	CComboBox::PreSubclassWindow();

	_ASSERTE( GetStyle() & CBS_OWNERDRAWFIXED );			// Assert Proper Style Set
	_ASSERTE( GetStyle() & CBS_DROPDOWNLIST );				// Assert Proper Style Set
	_ASSERTE( GetStyle() & CBS_HASSTRINGS );				// Assert Proper Style Set

	Initialise();
}

void CAttribColorComboBox::Initialise()
{
/*
	int iNumColours = sizeof( g_arrColours ) / sizeof( g_arrColours[0] );
	for( int iNum = 0; iNum < iNumColours; iNum++ )
	{
		Colour& colour = g_arrColours[iNum];
		AddColour( colour.m_strName, colour.m_crColour );
	}

	// add a custom item on the end.
	AddColour( "Custom...", RGB( 255, 255, 255 ) );
	SetCurSel( 0 );
*/
}

void CAttribColorComboBox::DrawItem( LPDRAWITEMSTRUCT pDIStruct )
{
	CString strColour;
	CDC dcContext;
	CRect rItemRect( pDIStruct->rcItem );
	CRect rBlockRect( rItemRect );
	CRect rTextRect( rBlockRect );
	CBrush brFrameBrush;
	int iFourthWidth = 0;
	int iItem = pDIStruct->itemID;
	int iAction = pDIStruct->itemAction;
	int iState = pDIStruct->itemState;
	COLORREF crColour = NULL;
	COLORREF crNormal = GetSysColor( COLOR_WINDOW );
	COLORREF crSelected = GetSysColor( COLOR_HIGHLIGHT );
	COLORREF crText = GetSysColor( COLOR_WINDOWTEXT );

	if( !dcContext.Attach( pDIStruct->hDC ) )
	{
		return;
	}

	//iFourthWidth = ( rBlockRect.Width() / 4 );
	iFourthWidth = ( rBlockRect.Width() );
	brFrameBrush.CreateStockObject( BLACK_BRUSH );

	if( iState & ODS_SELECTED )
	{
		dcContext.SetTextColor(	( 0x00FFFFFF & ~( crText ) ) );
		dcContext.SetBkColor( crSelected );
		dcContext.FillSolidRect( &rBlockRect, crSelected );
	}
	else
	{
		dcContext.SetTextColor( crText );
		dcContext.SetBkColor( crNormal );
		dcContext.FillSolidRect( &rBlockRect, crNormal );
	}

	if( iState & ODS_FOCUS )
	{
		dcContext.DrawFocusRect( &rItemRect );
	}

	// calculate text area.
	rTextRect.left += ( iFourthWidth + 2 );
	rTextRect.top += 2;

	// calculate colour block area.
	rBlockRect.DeflateRect( CSize( 2, 2 ) );
	//rBlockRect.right = iFourthWidth;

	// draw colour text and block.
	if( iItem != -1 )
	{
		GetLBText( iItem, strColour );

		if( iState & ODS_DISABLED )
		{
			crColour = GetSysColor( COLOR_INACTIVECAPTIONTEXT );
			dcContext.SetTextColor( crColour );
		}
		else
		{
			crColour = GetItemData( iItem );
		}

		dcContext.SetBkMode( TRANSPARENT );
		//dcContext.TextOut( rTextRect.left, rTextRect.top,	strColour );

		dcContext.FillSolidRect( &rBlockRect, crColour );
				
		dcContext.FrameRect( &rBlockRect, &brFrameBrush );
	}

	dcContext.Detach();
}

COLORREF CAttribColorComboBox::GetSelectedColourValue()
{
	return GetItemData( GetCurSel() );
}

void CAttribColorComboBox::AddColour( CString strName, COLORREF crColour )
{
	int iIndex = InsertString ( -1, strName );

	if( iIndex >= 0 )
	{
		SetItemData( iIndex, crColour );
	}
}

int CAttribColorComboBox::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	_ASSERTE( GetStyle() & CBS_OWNERDRAWFIXED );			// Assert Proper Style Set
	_ASSERTE( GetStyle() & CBS_DROPDOWNLIST );				// Assert Proper Style Set
	_ASSERTE( GetStyle() & CBS_HASSTRINGS );				// Assert Proper Style Set

	return 0;
}

void CAttribColorComboBox::OnCbnCloseup()
{
	if (::IsWindowVisible (GetSafeHwnd ())) {
		CTreeListCtrl* pTree = static_cast<CTreeListCtrl*>(GetParent ());
		pTree->FinishModify ();
	}
}

void CAttribColorComboBox::OnCbnSetfocus()
{
	ShowDropDown (TRUE);
}
