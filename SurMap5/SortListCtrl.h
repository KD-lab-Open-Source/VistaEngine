#ifndef __SORT_LIST_CTRL_H_INCLUDED__
#define __SORT_LIST_CTRL_H_INCLUDED__

// CSortListCtrl

class CSortListCtrl : public CListCtrl
{
	DECLARE_DYNAMIC(CSortListCtrl)

public:
	CSortListCtrl();
	virtual ~CSortListCtrl();

protected:
	DECLARE_MESSAGE_MAP()

protected:
	int m_iSortColumn;
	BOOL m_bSortAscending;
	void Sort( int iColumn, BOOL bAscending );
	static int CALLBACK CompareFunction( LPARAM lParam1, LPARAM lParam2, LPARAM lParamData );

public:
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
};

#endif
