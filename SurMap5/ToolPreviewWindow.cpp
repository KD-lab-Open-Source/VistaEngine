#include "stdafx.h"
#include "ToolPreviewWindow.h"
#include "..\Game\RenderObjects.h"
#include "SurToolAux.h"
#include "MainFrame.h"


// CToolPreviewWindow

IMPLEMENT_DYNAMIC(CToolPreviewWindow, CWnd)
CToolPreviewWindow::CToolPreviewWindow()
: m_pRenderWindow (0)
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if( !::GetClassInfo (hInst, TOOL_PREVIEW_CLASSNAME, &wndclass) )
	{
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
		wndclass.lpfnWndProc	= ::DefWindowProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
		wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= TOOL_PREVIEW_CLASSNAME;

		if (!AfxRegisterClass (&wndclass))
			AfxThrowResourceException ();
	}
}

CToolPreviewWindow::~CToolPreviewWindow()
{
}


BEGIN_MESSAGE_MAP(CToolPreviewWindow, CWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CToolPreviewWindow::Create( DWORD dwStyle, const CRect& rect, CWnd* pParentWnd )
{
	return CWnd::Create (TOOL_PREVIEW_CLASSNAME, 0, dwStyle, rect, pParentWnd, 0);
}

// CToolPreviewWindow message handlers


void CToolPreviewWindow::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	if (m_pRenderWindow) {
		OnDraw ();
	} else {
		/*
		CRect clientRect;
		GetClientRect (&clientRect);
		dc.FillSolidRect (&clientRect, RGB(0, 128, 0));
		*/
	}
}

int CToolPreviewWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
    
	return 0;
}

void CToolPreviewWindow::InitRenderDevice()
{
	CRect clientRect;
	GetClientRect (&clientRect);
	xassert (gb_RenderDevice != 0);
	m_pRenderWindow = gb_RenderDevice->CreateRenderWindow (GetSafeHwnd ());
	m_pScene = gb_VisGeneric->CreateScene ();
	m_pCamera = m_pScene->CreateCamera ();

	UpdateCameraFrustum (clientRect.Width(), clientRect.Height());
	OnSize (0, clientRect.Width(), clientRect.Height());
}


void CToolPreviewWindow::DoneRenderDevice()
{
	RELEASE (m_pCamera);
	RELEASE (m_pScene);
	RELEASE (m_pRenderWindow);
}

void CToolPreviewWindow::UpdateCameraFrustum (int _width, int _height)
{
    gb_RenderDevice->SelectRenderWindow (m_pRenderWindow);
	float width = 1.0f;
	float height = 1.0f;

	Vect2f focus (1.0f, 1.0f);

	if (_width > _height) {
		focus.x = static_cast<float>(_height) / static_cast<float>(_width);
		focus.y = static_cast<float>(_height) / static_cast<float>(_width);
	}

	m_pCamera->SetAttr (ATTRCAMERA_PERSPECTIVE);
    m_pCamera->SetFrustum (&Vect2f (0.5f, 0.5f),
                           &sRectangle4f (-0.5f * width, -0.5f * height,
										   0.5f * width,  0.5f * height), 
                           &focus,
                           &Vect2f (1.0f, 3000.0f)
                          );

	Vect3f position (Vect3f::ZERO);
	float _cameraPsi = M_PI / 12.0f;
	float _cameraTheta = M_PI / 3.0f;
	float _cameraDistance = 300.0f;
	position.setSpherical(_cameraPsi, _cameraTheta, _cameraDistance);
	MatXf matrix = MatXf::ID;
	matrix.rot() = Mat3f(_cameraTheta, X_AXIS) * Mat3f(M_PI/2 - _cameraPsi, Z_AXIS);
	matrix *= MatXf(Mat3f::ID, -position);	

	MatXf ml=MatXf::ID;
	ml.rot()[2][2]=-1;

	MatXf mr=MatXf::ID;
	mr.rot()[1][1]=-1;
	MatXf cameraMatrix;
	cameraMatrix = mr * ml * matrix;

	m_pCamera->SetPosition(cameraMatrix);

	gb_RenderDevice->SelectRenderWindow (0);
}


void CToolPreviewWindow::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	if (gb_RenderDevice) {
		gb_RenderDevice->ChangeSize (cx, cy, RENDERDEVICE_MODE_WINDOW);
        UpdateCameraFrustum (cx, cy);
	}
	m_WindowSize.set (cx, cy);
}


void CToolPreviewWindow::OnDraw()
{
	if(gb_RenderDevice->IsInBeginEndScene())
		return;

	gb_RenderDevice->SelectRenderWindow (m_pRenderWindow);
	gb_RenderDevice->Fill (0, 0, 0);
	
	gb_RenderDevice->BeginScene ();
	gb_RenderDevice->SetRenderState (RS_CULLMODE, 1);
	gb_RenderDevice->SetSamplerDataVirtual(0,sampler_wrap_linear);

	gb_RenderDevice->SetDrawTransform(m_pCamera);
	
	CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd ();
	if(CSurToolBase* currentToolzer = pMainFrame->getToolWindow().currentTool())
		currentToolzer->CallBack_DrawPreview (m_WindowSize.x, m_WindowSize.y);
	m_pScene->Draw (m_pCamera);

	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();
	gb_RenderDevice->SelectRenderWindow (0);
}

