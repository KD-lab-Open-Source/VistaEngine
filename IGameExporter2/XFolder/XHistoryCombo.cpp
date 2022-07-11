////////////////////////////////////////////////////////////////////////////
// File:	XHistoryCombo.cpp
// Version:	2
// Created:	01-May-2002
//
// Author:	Paul S. Vickery
// E-mail:	paul@vickeryhome.freeserve.co.uk
//
// Implementation of CHistoryCombo which incorporates functionality to help
// with Loading and Saving of history in a combo box
//
// You are free to use or modify this code, with no restrictions, other than
// you continue to acknowledge me as the original author in this source code,
// or any code derived from it.
//
// If you use this code, or use it as a base for your own code, it would be 
// nice to hear from you simply so I know it's not been a waste of time!
//
// Copyright (c) 2001-2002 Paul S. Vickery
//
////////////////////////////////////////////////////////////////////////////
// Version History:
//
// Version 2 - 01-May-2002
// =======================
// Produced new version with changes as below:
// * removed CBS_SORT on creation if specified
// * added option to allow the sort style to be set if required
// * fixed SetMaxHistoryItems, so it removes old entries from the list to 
//   ensure that there are no more than the maximum. Also made SaveHistory
//   remove redundant profile entries above the maximum.
// * use WriteProfileString to remove profile entries rather than CRegKey.
//
// Version 1 - 12-Apr-2001
// =======================
// Initial version
// 
////////////////////////////////////////////////////////////////////////////
// PLEASE LEAVE THIS HEADER INTACT
////////////////////////////////////////////////////////////////////////////
//
// Modified 20-Jun-2003 by Hans Dietrich - see "+++hd" lines for details.  
// Changed name to XHistoryCombo to avoid confusion with original.
//
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XHistoryCombo.h"
#include "io.h"		// needed for _taccess		//+++hd

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_HISTORY_ITEMS	100
#define MAX_DROPDOWN_ITEMS	10					//+++hd
#define KEY_PREFIX_FORMAT	_T("%s%04d")		//+++hd

/////////////////////////////////////////////////////////////////////////////
// CHistoryCombo

CHistoryCombo::CHistoryCombo(BOOL bAllowSortStyle/*=FALSE*/)
{
	TRACE(_T("in CHistoryCombo::CHistoryCombo\n"));
	m_nMaxHistoryItems        = MAX_HISTORY_ITEMS;
	m_bSaveRestoreLastCurrent = TRUE;
	m_bAllowSortStyle         = bAllowSortStyle;
	m_bCheckAccess            = FALSE;						//+++hd
	m_nDropSize               = MAX_DROPDOWN_ITEMS;			//+++hd
}

BOOL CHistoryCombo::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (! m_bAllowSortStyle)  // turn off CBS_SORT style
		cs.style &= ~CBS_SORT;
	return CComboBox::PreCreateWindow(cs);
}

void CHistoryCombo::PreSubclassWindow() 
{
	// warn if creating with CBS_SORT style
	// (unfortunately we can't turn it off)
	if (! m_bAllowSortStyle && GetStyle() & CBS_SORT)
		TRACE(_T("WARNING: Creating History combo with CBS_SORT style\n"));

	CComboBox::PreSubclassWindow();
}

BEGIN_MESSAGE_MAP(CHistoryCombo, CComboBox)
	//{{AFX_MSG_MAP(CHistoryCombo)
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropdown)		//+++hd
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHistoryCombo message handlers

// this version of AddString adds a string only if it doesn't already exist
// in the list, and in any case, makes sure that the string is the first
// in the list (ie most recent in history)
// also makes sure number of items in the list doesn't exceed the maximum allowed
int CHistoryCombo::AddString(LPCTSTR lpszString)
{
	CString sString(lpszString);

	// if it's not set up as a history combo then call base class
	if (m_sSection.IsEmpty() || m_sKeyPrefix.IsEmpty())
		return CComboBox::AddString(lpszString);

	int nRet = -1;

	// don't add if already there
	sString.TrimLeft();
	sString.TrimRight();

	if (sString.IsEmpty())
	{
		TRACE(_T("ERROR  string is empty\n"));
		return CB_ERR;
	}

	nRet = CComboBox::InsertString(0, sString);
	int nIndex = FindStringExact(0, sString);
	if (nIndex != -1 && nIndex != 0)
		DeleteString(nIndex);

	// if we've got too many items then delete the last one
	// truncate list to m_nMaxHistoryItems
	int nNumItems = GetCount();
	for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
		DeleteString(m_nMaxHistoryItems);

	SetCurSel(nRet);
	return nRet;
}

// loads the history from the specified MRU list, and populates the combo list
// if bSelectMostRecent is TRUE, selects the most recent into the edit control
// returns the selected text
// if using this overload of LoadHistory then SaveHistory will not function
// but you should simply save the MRU in the normal way instead
// note that the MRU should have already been read from the profile before
// being passed to this function, as this function will not call MRU->ReadList()
CString CHistoryCombo::LoadHistory(CRecentFileList* pListMRU, BOOL bSelectMostRecent/*=TRUE*/)
{
  if (pListMRU == NULL)
    return _T("");

  int nNumItems = pListMRU->GetSize();
  for (int n = 0; n < nNumItems; n++)
    CComboBox::AddString((*pListMRU)[n]);
  if (bSelectMostRecent)
    SetCurSel(0);
  CString sText;
  GetWindowText(sText);
  return sText;
}

// loads the history from the specified profile area, and returns the 
// text selected
// the profile area is kept so that in doesn't need to specified again
// when saving the history
CString CHistoryCombo::LoadHistory(LPCTSTR lpszSection, 
								   LPCTSTR lpszKeyPrefix,    
								   BOOL bSaveRestoreLastCurrent/*=TRUE*/,    
								   LPCTSTR lpszKeyCurItem/*=NULL*/)
{
	if (lpszSection == NULL || lpszKeyPrefix == NULL || *lpszSection == _T('\0'))
		return _T("");

	TRACE(_T("lpszSection=%s  lpszKeyPrefix=%s\n"), lpszSection, lpszKeyPrefix);

	m_sSection = lpszSection;
	m_sKeyPrefix = lpszKeyPrefix;
	m_sKeyCurItem = lpszKeyCurItem == NULL ? _T("") : lpszKeyCurItem;
	m_bSaveRestoreLastCurrent = bSaveRestoreLastCurrent;
	CWinApp* pApp = AfxGetApp();

	int n = 0;
	CString sText;
	do
	{
		CString sKey;
		sKey.Format(KEY_PREFIX_FORMAT, m_sKeyPrefix, n++);
		sText = pApp->GetProfileString(m_sSection, sKey);
		if (m_bCheckAccess)							//+++hd
		{											//+++hd
			if (_taccess(sText, 00) != -1)			//+++hd
			{										//+++hd
				if (!sText.IsEmpty())				//+++hd
					CComboBox::AddString(sText);	//+++hd
			}										//+++hd
		}											//+++hd
		else										//+++hd
		{
			if (!sText.IsEmpty())
				CComboBox::AddString(sText);
		}

	} while (!sText.IsEmpty() && n < m_nMaxHistoryItems);

	if (m_bSaveRestoreLastCurrent)
	{
		CString sKey;
		if (!m_sKeyCurItem.IsEmpty())
			sKey = m_sKeyCurItem;
		else if (m_sKeyPrefix.IsEmpty())
			sKey = _T("Last");
		else
			sKey = m_sKeyPrefix;
		sText = pApp->GetProfileString(m_sSection, sKey);

		if (!sText.IsEmpty())
		{
			int nIndex = FindStringExact(-1, sText);
			if (nIndex != -1)
			SetCurSel(nIndex);
			else if (GetStyle() & CBS_DROPDOWN)
			SetWindowText(sText);
		}
	}

	return sText;
}

// saves the history to the profile specified when calling LoadHistory
// if no profile information (ie LoadHistory() wasn't called with it) then
// this function does nothing
void CHistoryCombo::SaveHistory(BOOL bAddCurrentItemToHistory/*=TRUE*/)
{
	TRACE(_T("in CHistoryCombo::SaveHistory\n"));

  if (m_sSection.IsEmpty())
    return;

  CWinApp* pApp = AfxGetApp();
  ASSERT(pApp);

  if (bAddCurrentItemToHistory)
  {
    CString sCurItem;
    GetWindowText(sCurItem);
    // trim it, so we items which differ only by a leading/trailing space
    sCurItem.TrimLeft();
    sCurItem.TrimRight();
    if (! sCurItem.IsEmpty())
      AddString(sCurItem);
  }

  // save history to info cached earlier
  int nMax = min(GetCount(), m_nMaxHistoryItems + 1);
  for (int n = 0; n < nMax; n++)
  {
    CString sKey;
    sKey.Format(KEY_PREFIX_FORMAT, m_sKeyPrefix, n);
    CString sText;
    GetLBText(n, sText);
    pApp->WriteProfileString(m_sSection, sKey, sText);
	//TRACE(_T("m_sSection=%s  sKey=%s  sText=%s\n"), m_sSection, sKey, sText);
  }
  // remove redundant items
  for (n = nMax; n < 1000/* prevent runaway*/; n++)
  {
    CString sKey;
    sKey.Format(KEY_PREFIX_FORMAT, m_sKeyPrefix, n);
	TRACE(_T("m_sKeyPrefix=%s\n"), m_sKeyPrefix);
    CString sText = pApp->GetProfileString(m_sSection, sKey);
    if (sText.IsEmpty())
      break;
    pApp->WriteProfileString(m_sSection, sKey, NULL); // remove entry
  }
  if (m_bSaveRestoreLastCurrent)
  {
    CString sText;
    GetWindowText(sText);
    CString sKey;
    if (!m_sKeyCurItem.IsEmpty())
      sKey = m_sKeyCurItem;
    else if (m_sKeyPrefix.IsEmpty())
      sKey = _T("Last");
    else
      sKey = m_sKeyPrefix;
    pApp->WriteProfileString(m_sSection, sKey, sText);
  }
}

// removes all the items from the history list, and optionally deletes
// the registry items. Note that if the history list is generated from
// a CRecentFileList, then registry entries will not be deleted
void CHistoryCombo::ClearHistory(BOOL bDeleteRegistryEntries/*=TRUE*/)
{
	ResetContent();
	if (! m_sSection.IsEmpty() && bDeleteRegistryEntries)
	{
		// remove profile entries
		CWinApp* pApp = AfxGetApp();
		ASSERT(pApp);
		CString sKey;

		for (int n = 0; n < 1000/* prevent runaway*/; n++)
		{
			sKey.Format(KEY_PREFIX_FORMAT, m_sKeyPrefix, n);
			CString sText = pApp->GetProfileString(m_sSection, sKey);
			if (sText.IsEmpty())
				break;
			pApp->WriteProfileString(m_sSection, sKey, NULL); // remove entry
		}

		if (! m_sKeyCurItem.IsEmpty())
			sKey = m_sKeyCurItem;
		else if (m_sKeyPrefix.IsEmpty())
			sKey = _T("Last");
		else
			sKey = m_sKeyPrefix;

		pApp->WriteProfileString(m_sSection, sKey, NULL);
	}
}

void CHistoryCombo::SetMaxHistoryItems(int nMaxItems)
{
  m_nMaxHistoryItems = nMaxItems;

  // truncate list to nMaxItems
  int nNumItems = GetCount();
  for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
    DeleteString(m_nMaxHistoryItems);
}

void CHistoryCombo::StoreValue(BOOL bIgnoreIfEmpty/*=TRUE*/)
{
  // add the current value to the list
  CString sValue;
  GetWindowText(sValue);
  if (bIgnoreIfEmpty && sValue.IsEmpty())
    return;
  AddString(sValue);
}

// +++hd:  set size of dropdown list 
void CHistoryCombo::OnDropdown() 
{
	CRect rect;
	GetWindowRect(&rect);

	int nHeight = rect.Height() + 2 * ::GetSystemMetrics(SM_CXBORDER);

	int nItemHeight = GetItemHeight(0);

	// m_nDropSize = No. of items in dropdown list
	nHeight += m_nDropSize * nItemHeight;

	SetWindowPos(NULL,
				 0,
				 0,
				 rect.Width(),
				 nHeight,
				 SWP_NOMOVE | SWP_NOZORDER);
}
