// SortListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "SortListCtrl.h"
#include ".\sortlistctrl.h"


IMPLEMENT_DYNAMIC(CSortListCtrl, CListCtrl)
CSortListCtrl::CSortListCtrl()
{
	m_iSortColumn = 0;
	m_bSortAscending = true;
}

CSortListCtrl::~CSortListCtrl()
{
}


BEGIN_MESSAGE_MAP(CSortListCtrl, CListCtrl)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnclick)
END_MESSAGE_MAP()



bool IsNumber( LPCTSTR pszText )
{
	for( int i = 0; i < lstrlen( pszText ); i++ )
		if( !_istdigit( pszText[ i ] ) )
			if (!_istspace( pszText[ i ] ))
				return false;

	return true;
}


int NumberCompare( CString str1, CString str2 )
{
	str1.Remove(' ');
	str2.Remove(' ');

	const int iNumber1 = atoi( str1 );
	const int iNumber2 = atoi( str2 );

	if( iNumber1 < iNumber2 )
		return -1;

	if( iNumber1 > iNumber2 )
		return 1;

	return 0;
}
void CSortListCtrl::Sort( int iColumn, BOOL bAscending )
{
	m_iSortColumn = iColumn;
	m_bSortAscending = bAscending;
	SortItems( CompareFunction, reinterpret_cast<DWORD>(this));
	for (int i=0; i<GetItemCount(); i++)
	{
		SetItemData(i,i);
	}
}
int CALLBACK CSortListCtrl::CompareFunction( LPARAM lParam1, LPARAM lParam2, LPARAM lParamData )
{
	CSortListCtrl* pListCtrl = reinterpret_cast<CSortListCtrl*>( lParamData );

	CString pszText1 = pListCtrl->GetItemText(lParam1,pListCtrl->m_iSortColumn);
	CString pszText2 = pListCtrl->GetItemText(lParam2,pListCtrl->m_iSortColumn);
	//LPCSTR pszText1 = (LPCSTR)str1;
	//LPCSTR pszText2 = (LPCSTR)str2;

	if( IsNumber( pszText1 ) )
		return pListCtrl->m_bSortAscending ? NumberCompare( pszText1, pszText2 ) : NumberCompare( pszText2, pszText1 );
	else
		return pListCtrl->m_bSortAscending ? lstrcmp( pszText1, pszText2 ) : lstrcmp( pszText2, pszText1 );

}
void CSortListCtrl::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	Sort( pNMLV->iSubItem, pNMLV->iSubItem == m_iSortColumn ? !m_bSortAscending : TRUE );
	*pResult = 0;
}

