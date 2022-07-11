#include "StdAfx.h"

#include "Save.h"
#include "XPrmArchive.h"
#include "TestFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC (CTestFrame, CFrameWnd)

CTestFrame::CTestFrame ()
{
	m_pAttribEditor = 0;
}

CTestFrame::~CTestFrame ()
{
	delete m_pAttribEditor;
	m_pAttribEditor = 0;
}

BEGIN_MESSAGE_MAP(CTestFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_ACTIVATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

int CTestFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
    
	m_pAttribEditor = new CAttribEditorCtrl ();
	CRect rt;
	GetClientRect (&rt);

    m_pAttribEditor->Create (WS_VISIBLE | WS_CHILD, rt, this);
	
	return 0;
}

void CTestFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CFrameWnd::OnActivate(nState, pWndOther, bMinimized);
	
	static bool bFirstActivate = true;
	
	if (bFirstActivate)
	{
		LoadData ("Scripts\\InterfaceAttributes");	
		//MessageBox ("Данные загружены, строим дерево...");
		m_pAttribEditor->AttachData (m_InterfaceAttributes, "Root Tree");
		bFirstActivate = false;
	}
}

void CTestFrame::LoadData (const char* filename)
{
	CWaitCursor wait_cursor;
	InterfaceAttributes& atts = m_InterfaceAttributes;

	XPrmIArchive ia;
	
	if (! ia.open (filename))
	{
		CString msg;
		msg.Format ("Unable to open archive!");
		MessageBox (msg, "Error!", MB_ICONERROR | MB_OK);
		return;
	}

	ia >> makeObjectWrapper (atts, "InterfaceAttributes", 0);
	return;
}

void CTestFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	if (m_pAttribEditor)
	{
		m_pAttribEditor->MoveWindow (0, 0, cx, cy);
	}
}
