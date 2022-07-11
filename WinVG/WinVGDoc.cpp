// WinVGDoc.cpp : implementation of the CWinVGDoc class
//

#include "stdafx.h"
#include "WinVG.h"

#include "WinVGDoc.h"
#include "WinVGView.h"
#include "TabDialog.h"
#include "DirectoryTree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWinVGDoc

IMPLEMENT_DYNCREATE(CWinVGDoc, CDocument)

BEGIN_MESSAGE_MAP(CWinVGDoc, CDocument)
	//{{AFX_MSG_MAP(CWinVGDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWinVGDoc construction/destruction
CWinVGDoc* pDoc=NULL;

CWinVGDoc::CWinVGDoc()
{
	pDoc=this;
	m_pView=0;
	m_pHierarchyObj=0;
	strcpy(CurrentChannel,"main");
	m_pDirectoryTree=0;
	ModelCamera = false;
	Camera43 = false;
	current_time_light = 10;
	need_time_light = false;
}

CWinVGDoc::~CWinVGDoc()
{
}

/////////////////////////////////////////////////////////////////////////////
// CWinVGDoc serialization

void CWinVGDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{

	}
	else
	{

	}
}

/////////////////////////////////////////////////////////////////////////////
// CWinVGDoc diagnostics

#ifdef _DEBUG
void CWinVGDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWinVGDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWinVGDoc commands

void CWinVGDoc::OnChangedViewList() 
{
	CDocument::OnChangedViewList();

	m_pHierarchyObj=0; m_pView=0; m_pDirectoryTree=0;
	POSITION pos=GetFirstViewPosition();
	while(pos!=0)
	{
		CView* pView=GetNextView(pos);
		if(pView->IsKindOf(RUNTIME_CLASS(CWinVGView)))
			m_pView=DYNAMIC_DOWNCAST(CWinVGView,pView);
		else if(pView->IsKindOf(RUNTIME_CLASS(CTabDialog)))
		{
			if(m_pHierarchyObj==0)
				m_pHierarchyObj=DYNAMIC_DOWNCAST(CTabDialog,pView);
		}
		else if(pView->IsKindOf(RUNTIME_CLASS(CDirectoryTree)))
		{
			if(m_pDirectoryTree==0)
			{
				m_pDirectoryTree=DYNAMIC_DOWNCAST(CDirectoryTree,pView);
				strcpy(m_pDirectoryTree->FileExtension,"*.3dx");
			}
		}
	}
}

BOOL CWinVGDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;
	
	m_pView->LoadObject((char*)lpszPathName);
	
	return TRUE;
}

