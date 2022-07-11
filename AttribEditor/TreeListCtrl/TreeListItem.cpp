// TreeListItem.cpp: implementation of the CTreeListItem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TreeListItem.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTreeListItem::CTreeListItem( int nSubItem ) :
	m_pParent( NULL ),
	m_pChild( NULL ),
	m_pPrev( NULL ),
	m_pNext( NULL ),
	m_dwState( TLIS_SHOWCHECKBOX | TLIS_SHOWLOCKBOX | TLIS_CHECKED_NULL | TLIS_LOCKED_NULL ),
	m_dwData( 0 ),
//	m_pImageList( NULL ),
	m_nLevel( 0 ),
	m_nChild( 1 ),
	m_nVisibleChild( 1 )
{
	for( int iSubItem = 0; iSubItem < nSubItem; iSubItem++ )
	{
		CString* pSubItem = new CString;
		m_arSubItems.Add( pSubItem );
	}
}

CTreeListItem::~CTreeListItem()
{
	while( m_arSubItems.GetSize() > 0 )
	{
		CString* pSubItem;
		pSubItem = (CString*)m_arSubItems[m_arSubItems.GetUpperBound()];
		m_arSubItems.RemoveAt( m_arSubItems.GetUpperBound() );
		delete pSubItem;
	}
}

void CTreeListItem::Expand()
{
	SetState( TLIS_EXPANDED, 0 );
}

void CTreeListItem::Collapse()
{
	SetState( 0, TLIS_EXPANDED );
}

DWORD CTreeListItem::GetState( DWORD dwStateMask ) const
{
	return m_dwState&dwStateMask;
}

void CTreeListItem::SetState( DWORD dwAddStatus, DWORD dwRemoveStatus )
{
	if( dwAddStatus&TLIS_EXPANDED )
		dwAddStatus |= TLIS_EXPANDEDONCE;

	if( dwRemoveStatus&TLIS_EXPANDEDONCE )
		dwRemoveStatus |= TLIS_EXPANDED;

	m_dwState |= dwAddStatus;
	m_dwState &=~dwRemoveStatus;
	return;
}

void CTreeListItem::GetImage( int& nImage, 
							 int& nSelectedImage, 
							 int& nExpandImage, 
							 int& nExpandSelectedImage ) const
{
	nImage					= m_nImage;
	nSelectedImage			= m_nSelectedImage;
	nExpandImage			= nExpandImage;
	nExpandSelectedImage	= nExpandSelectedImage;
}

void CTreeListItem::SetImage( int nImage, int nSelectedImage, int nExpandImage, int nExpandSelectedImage )
{
	m_nImage				= nImage;
	m_nSelectedImage		= nSelectedImage;
	m_nExpandImage			= nExpandImage;
	m_nExpandSelectedImage	= nExpandSelectedImage;
}

LPCTSTR CTreeListItem::GetText( int nIndex ) const
{
	if( nIndex >= m_arSubItems.GetSize() )
		return NULL;

	CString* pString = (CString*)m_arSubItems[nIndex];
	return *pString;
}

bool CTreeListItem::SetText( LPCTSTR lpszItem, int nIndex )
{
	if (lpszItem == 0) {
		::AfxMessageBox("Error in CTreeListItem::SetText(): lpszItem == 0");
		return false;
	}

	if( nIndex >= m_arSubItems.GetSize() )
		return false;

	CString* pString = (CString*)m_arSubItems[nIndex];
	*pString = lpszItem;

	return true;
}

DWORD_PTR CTreeListItem::GetData() const
{
	return m_dwData;
}

void CTreeListItem::SetData( DWORD_PTR dwData )
{
	m_dwData = dwData;
}

bool CTreeListItem::IsShowTreeImage() const
{
	if( m_dwState&TLIS_SHOWTREEIMAGE )
		return TRUE;
	else
		return FALSE;
}

void CTreeListItem::ShowTreeImage( bool bShow )
{
	if( bShow )
		m_dwState |= TLIS_SHOWTREEIMAGE;
	else
		m_dwState &=~TLIS_SHOWTREEIMAGE;
}

bool CTreeListItem::IsShowCheckBox() const
{
	if( m_dwState&TLIS_SHOWCHECKBOX )
		return TRUE;
	else
		return FALSE;
}

void CTreeListItem::ShowCheckBox( bool bShow )
{
	if( bShow )
		m_dwState |= TLIS_SHOWCHECKBOX;
	else
		m_dwState &=~TLIS_SHOWCHECKBOX;
}

bool CTreeListItem::IsShowLockBox() const
{
	if( m_dwState&TLIS_SHOWLOCKBOX )
		return TRUE;
	else
		return FALSE;
}

void CTreeListItem::ShowLockBox( bool bShow )
{
	if( bShow )
		m_dwState |= TLIS_SHOWLOCKBOX;
	else
		m_dwState &=~TLIS_SHOWLOCKBOX;
}

bool CTreeListItem::GetSelected() const
{
	if( GetState()&TLIS_SELECTED )
		return true;
	return false;
}

void CTreeListItem::SetSelected( bool bSelected )
{
	if( bSelected )
		SetState( TLIS_SELECTED, 0 );
	else
		SetState( 0, TLIS_SELECTED|TLIS_READYTOEDIT);
}

bool CTreeListItem::IsReadyToEdit() const
{
	if( GetState()&TLIS_READYTOEDIT)
		return true;
	return false;
}
void CTreeListItem::SetReadyToEdit(bool bSet /* = true */)
{
	if( bSet )
	{
		ASSERT(GetSelected());
		SetState( TLIS_READYTOEDIT, 0 );
	}
	else
		SetState( 0, TLIS_READYTOEDIT );
}

int CTreeListItem::GetCheck() const
{
	ASSERT( GetState()&TLIS_CHECKED || GetState()&TLIS_CHECKED_NULL );
	
	if( ( GetState()&TLIS_CHECKEDMASK) == (DWORD)TLIS_CHECKEDPART )
		return 2;
	
	if( GetState()&TLIS_CHECKED )
		return 1;

	return 0;
}

void CTreeListItem::SetCheck( bool bCheck )
{
	if( bCheck )
		SetState( TLIS_CHECKED, TLIS_CHECKED_NULL );
	else
		SetState( TLIS_CHECKED_NULL, TLIS_CHECKED );
}

int CTreeListItem::GetLock() const
{
	ASSERT( GetState()&TLIS_LOCKED || GetState()&TLIS_LOCKED_NULL );

	if( ( GetState()&TLIS_LOCKEDMASK) == (DWORD)TLIS_LOCKEDPART )
		return 2;
	
	if( GetState()&TLIS_LOCKED )
		return 1;

	return 0;
}

void CTreeListItem::SetLock( bool bLock )
{
	if( bLock )
		SetState( TLIS_LOCKED, TLIS_LOCKED_NULL );
	else
		SetState( TLIS_LOCKED_NULL, TLIS_LOCKED );
}

bool CTreeListItem::GetFocused() const{
	return (GetState(TLIS_FOCUSED) == TLIS_FOCUSED);
}

void CTreeListItem::SetFocused(bool bFocused /*= true*/){
	if (bFocused)
		SetState(TLIS_FOCUSED);
	else
		SetState(0, TLIS_FOCUSED);
}

/*
void CTreeListItem::SetImageList( CImageList* pImageList )
{
	ASSERT( pImageList != NULL );
	m_pImageList = pImageList;
}

void CTreeListItem::SetImage( int nIndex )
{
	return;
}

int CTreeListItem::GetImage()
{
	return 0;
}
*/
