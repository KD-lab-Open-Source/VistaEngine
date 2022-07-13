#include "stdafx.h"
#include "MiniMapWindow.h"
#include "Game\RenderObjects.h"
#include "MainFrame.h"
#include "GeneralView.h"

#include "SurToolAux.h"
#include "UserInterface\UI_Render.h"
#include "UserInterface\UI_Minimap.h"

#include "Environment\Environment.h"
#include "Game\CameraManager.h"
#include "Render\Src\cCamera.h"
#include "Render\Src\Scene.h"
#include "Render\Src\VisGeneric.h"

CMiniMapWindow::CMiniMapWindow(CMainFrame* mainFrame)
: renderWindow_(0)
, sizing_(false)
, camera_(0)
, scene_(0)
, zoom_(1.0f)
{
	WNDCLASS wndclass;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if(!::GetClassInfo (hInst, className(), &wndclass) )
	{
        wndclass.style			= CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW|WS_TABSTOP;
		wndclass.lpfnWndProc	= ::DefWindowProc;
		wndclass.cbClsExtra		= 0;
		wndclass.cbWndExtra		= 0;
		wndclass.hInstance		= hInst;
		wndclass.hIcon			= NULL;
		wndclass.hCursor		= AfxGetApp ()->LoadStandardCursor (IDC_ARROW);
		wndclass.hbrBackground	= reinterpret_cast<HBRUSH> (COLOR_WINDOW);
		wndclass.lpszMenuName	= NULL;
		wndclass.lpszClassName	= className();

		if (!AfxRegisterClass (&wndclass))
			AfxThrowResourceException ();
	}

	flag_CameraRestrictionDrag_ = 0;
	mainFrame->signalWorldChanged().connect(this, &CMiniMapWindow::onWorldChanged);
}

CMiniMapWindow::~CMiniMapWindow()
{
}


BEGIN_MESSAGE_MAP(CMiniMapWindow, CWnd)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL CMiniMapWindow::Create(DWORD dwStyle, const CRect& rect, CWnd* pParentWnd)
{
	return CWnd::Create(className(), 0, dwStyle, rect, pParentWnd, 0);
}

// CMiniMapWindow message handlers


void CMiniMapWindow::OnPaint()
{
	CPaintDC dc(this);

	redraw(dc);
}

int CMiniMapWindow::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	   
	return 0;
}

void CMiniMapWindow::initRenderDevice()
{
	xassert (::IsWindow (GetSafeHwnd ()));

	CRect clientRect;
	GetClientRect (&clientRect);
	xassert (gb_RenderDevice != 0);
	renderWindow_ = gb_RenderDevice->createRenderWindow (GetSafeHwnd ());
	scene_ = gb_VisGeneric->CreateScene ();
	xassert (scene_ != 0);
	camera_ = scene_->CreateCamera ();
	xassert (camera_ != 0);

	updateCameraFrustum (clientRect.Width(), clientRect.Height());

	OnSize(0, clientRect.Width(), clientRect.Height());
}


void CMiniMapWindow::doneRenderDevice()
{
	minimap().releaseMapTexture();
	RELEASE (camera_);
	RELEASE (scene_);
	RELEASE (renderWindow_);
}

void CMiniMapWindow::updateCameraFrustum (int _width, int _height)
{
	xassert (gb_RenderDevice);
	if (!gb_RenderDevice || !_width || !_height)
		return;

    gb_RenderDevice->selectRenderWindow (renderWindow_);
	
	float width = 1.0f;
	float height = 1.0f;

	Vect2f focus (1.0f, 1.0f);

	if (_width > _height) {
		focus.x = static_cast<float>(_height) / static_cast<float>(_width);
		focus.y = static_cast<float>(_height) / static_cast<float>(_width);
	}

	xassert (camera_);
	if (!camera_)
		return;

	camera_->setAttribute (ATTRCAMERA_PERSPECTIVE);
    camera_->SetFrustum (&Vect2f (0.5f, 0.5f),
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

	camera_->SetPosition(cameraMatrix);

	gb_RenderDevice->selectRenderWindow (0);
}


void CMiniMapWindow::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	if(cx == 0 || cy == 0)
		return;

	sizing_ = true;
	if (::IsWindow(GetSafeHwnd()) && gb_RenderDevice) {
		if (!renderWindow_)
			initRenderDevice();

		gb_RenderDevice->selectRenderWindow(renderWindow_);
		renderWindow_->ChangeSize();
		UI_Render::instance().setWindowPosition(Vect2i (cx, cy));
        updateCameraFrustum (cx, cy);
		updateMinimapPosition (cx, cy);
	
		gb_RenderDevice->selectRenderWindow(0);
	}
	windowSize_.set (cx, cy);
	sizing_ = false;
}

void CMiniMapWindow::redraw(CDC& dc)
{
	CRect clientRect;
	GetClientRect(&clientRect);
	if(!gb_RenderDevice || !renderWindow_){
		dc.FillSolidRect(&clientRect, RGB(0, 0, 0));
		return;
	}

	if (!renderWindow_ || gb_RenderDevice->IsInBeginEndScene() || sizing_ || clientRect.Width() == 0 || clientRect.Height() == 0)
		return;

	gb_RenderDevice->selectRenderWindow (renderWindow_);
	gb_RenderDevice->Fill (0, 0, 0);
	
	gb_RenderDevice->BeginScene ();
	gb_RenderDevice->SetRenderState (RS_CULLMODE, 1);
	gb_RenderDevice->SetSamplerDataVirtual(0,sampler_wrap_linear);

	gb_RenderDevice->SetDrawTransform (camera_);
	
	CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd ();
	if(CSurToolBase* currentToolzer = pMainFrame->view().getCurCtrl()){
        if(!currentToolzer->onDrawPreview(windowSize_.x, windowSize_.y))
            drawMiniMap ();
	}
	else
        drawMiniMap ();

    scene_->Draw(camera_);

	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();
	gb_RenderDevice->selectRenderWindow (0);
}

void CMiniMapWindow::drawMiniMap ()
{
	if (vMap.isWorldLoaded () && cameraManager) {
		minimap().redraw();
	}
}


void CMiniMapWindow::PreSubclassWindow()
{

	CWnd::PreSubclassWindow();
}

void CMiniMapWindow::onWorldChanged(WorldObserver* changer)
{
	if(!renderWindow_)
		return;
	std::string minimap_filename = vMap.getTargetName("map.tga");
	if (vMap.isWorldLoaded () && !vMap.getWorldName().empty() && PathFileExists (minimap_filename.c_str())) {
		minimap().init (Vect2f (vMap.H_SIZE, vMap.V_SIZE), minimap_filename.c_str());
		minimap().waterColor(environment->minimapWaterColor());
		CRect client_rect;
		GetClientRect (&client_rect);
		updateMinimapPosition (client_rect.Width(), client_rect.Height());
	}
}

void CMiniMapWindow::updateMinimapPosition (int cx, int cy)
{
	if (vMap.isWorldLoaded () && cx != 0 && cy != 0) {
		float aspect = float(vMap.H_SIZE) / float (vMap.V_SIZE);
		float width  = 1.0f;
		float height = 1.0f;
		if (float(cx) > float(cy) * aspect) {
			width = float(cy) * aspect / float(cx);
		} else {
			height = float(cx) / (float(cy) * aspect);
		}
		width *= zoom_;
		height *= zoom_;
		minimap().setPosition(Rectf (0.5f * (1.0f - width), 0.5f * (1.0f - height), width, height));
		//minimap().setPosition(Rectf(0.0f, 0.0f, 1.0f, 1.0f));
	}
}



void CMiniMapWindow::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect client_rect;
	GetClientRect (&client_rect);
	Vect2f coord((float)point.x/(float)client_rect.Width(), (float)point.y/(float)client_rect.Height());

	CSurToolBase* tool = currentToolzer();
	if(tool && tool->onPreviewLMBDown(coord)){
		SetCapture();
	}
	else{
		minimap().pressEvent(coord);
	}
	CWnd::OnLButtonDown(nFlags, point);
}

void CMiniMapWindow::OnLButtonUp(UINT nFlags, CPoint point)
{
	CRect client_rect;
	GetClientRect (&client_rect);
	Vect2f coord((float)point.x/(float)client_rect.Width(), (float)point.y/(float)client_rect.Height());

	CSurToolBase* tool = currentToolzer();
	if(tool && tool->onPreviewLMBUp(coord)){

	}
	if(GetCapture() == this)
		ReleaseCapture();
	CWnd::OnLButtonUp(nFlags, point);
}

void CMiniMapWindow::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect client_rect;
	GetClientRect (&client_rect);
	Vect2f coord((float)point.x/(float)client_rect.Width(), (float)point.y/(float)client_rect.Height());

	CSurToolBase* tool = currentToolzer();
	if(tool && tool->onPreviewTrackingMouse(coord)){

	}

	CWnd::OnMouseMove(nFlags, point);
}

BOOL CMiniMapWindow::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

CSurToolBase* CMiniMapWindow::currentToolzer()
{
	return ((CMainFrame*)AfxGetMainWnd())->view().getCurCtrl();
}

void CMiniMapWindow::setZoom(float zoom)
{
	CRect rect;
	GetClientRect(&rect);
	zoom_ = zoom;
	updateMinimapPosition(rect.Width(), rect.Height());
}
