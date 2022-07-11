#include "stdafx.h"
#include "Runtime3D.h"
#include "Render\inc\fps.h"

#include "Render\Inc\IRenderDevice.h"
#include "Render\Src\cCamera.h"
#include "Render\Src\Scene.h"
#include "Render\Src\VisGeneric.h"
#include "Render\D3d\D3dRender.h"
#include "UserInterface\UI_RenderBase.h"
#include "Util\Win32\DebugSymbolManager.h"

HWND g_hWnd=0;
Runtime3D* runtime3D=0;
cInterfaceRenderDevice	*renderDevice=0;
BOOL g_bActive=TRUE;//FALSE;

Runtime3D::Runtime3D() 
:	mousePos_(0,0),
	leftButtonPressed_(false),
	rightButtonPressed_(false),
	camera_(0),
	scene_(0)
{

}

Runtime3D::~Runtime3D()
{
}

bool Runtime3D::init(int xScr,int yScr,bool bwindow)
{
	DebugSymbolManager::create();

	renderDevice = CreateIRenderDevice(true);
	//Option_DrawNumberPolygon = true;

	if(!renderDevice->Initialize(xScr,yScr,RENDERDEVICE_MODE_STENCIL|(bwindow?RENDERDEVICE_MODE_WINDOW:0),g_hWnd))
		return false;

	createScene();

	UI_RenderBase::create();

	return true;
}

void Runtime3D::finit()
{
	destroyScene();
	
	RELEASE(gb_VisGeneric);
	RELEASE(renderDevice);
}

void Runtime3D::createScene()
{
	scene_ = gb_VisGeneric->CreateScene(); 
	camera_ = scene_->CreateCamera();
	camera_->setAttribute(ATTRCAMERA_PERSPECTIVE); // перспектива
	setCameraPosition(MatXf(Mat3f::ID, Vect3f(0,0,-512)));

	camera_->SetFrustum(							// устанавливается пирамида видимости
		&Vect2f(0.5f,0.5f),							// центр камеры
		&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),		// видимая область камеры
		&Vect2f(1.f,1.f),							// фокус камеры
		&Vect2f(30.0f,1200.0f)
		);

	scene_->SetSunDirection(Vect3f(-1,0,-1));
	scene_->SetSunColor(Color4f(1,1,1,1),Color4f(1,1,1,1),Color4f(1,1,1,1));
}

void Runtime3D::destroyScene()
{
	RELEASE(camera_);
	RELEASE(scene_);
}

void Runtime3D::setCameraPosition(const MatXf &Matrix)
{
	MatXf ml=MatXf::ID;
	ml.rot()[2][2]=-1;

	MatXf mr=MatXf::ID;
	mr.rot()[1][1]=-1;
	MatXf CameraMatrix;
	CameraMatrix=mr*ml*Matrix;

	camera_->SetPosition(CameraMatrix);
}


//Функция обработки сообщений Windows
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY://выйти из приложения, если нажали на крестик
            PostQuitMessage( 0 );
            return 0;
		case WM_SETCURSOR:
			SetCursor(LoadCursor(0,IDC_ARROW));
			break;
		case WM_ACTIVATEAPP:
			g_bActive = (wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE);
			return 0;
		case WM_MOUSEMOVE:
			if(runtime3D)
				runtime3D->setMousePos(Vect2i(LOWORD(lParam),HIWORD(lParam))); 
			break;
		case WM_GETMINMAXINFO:
			{
				MINMAXINFO *pMinMax;
				pMinMax = (MINMAXINFO *)lParam;
				POINT& p=pMinMax->ptMaxTrackSize;
				p.x=GetSystemMetrics(SM_CXSCREEN);
				p.y=GetSystemMetrics(SM_CYSCREEN);
				p.x += GetSystemMetrics(SM_CXSIZEFRAME)*2;
				p.y += GetSystemMetrics(SM_CYSIZEFRAME)*2 + GetSystemMetrics(SM_CYCAPTION);
				return 0;
			}
		case WM_MOUSEWHEEL:
			runtime3D->onWheel((int)wParam > 0);
			break;
		case WM_KEYDOWN:
			runtime3D->KeyDown(wParam);
			break;
		case WM_LBUTTONDOWN:
			runtime3D->OnLButtonDown();
			break;
		case WM_LBUTTONUP:
			runtime3D->OnLButtonUp();
			break;
		case WM_RBUTTONDOWN:
			runtime3D->OnRButtonDown();
			break;
		case WM_RBUTTONUP:
			runtime3D->OnRButtonUp();
			break;
    }
    return DefWindowProc( hWnd, msg, wParam, lParam );
}

Vect2i winmain_size_window(-1,-1);

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
	LPCSTR classname="3d demo";
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC|CS_HREDRAW |CS_VREDRAW, MsgProc, 0L, 0L,
                      hInst, 0, 0, 0, 0,
                      classname, 0 };

	bool bwindow=!check_command_line("fullscreen");
	int xPos=0,yPos=0;
//	Vect2i in(GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
	Vect2i in(1280, 1024);
	if(winmain_size_window.x>0)
		in=winmain_size_window;
	int xScr=in.x,yScr=in.y;
	if(bwindow)
	{
		xPos-=GetSystemMetrics(SM_CXSIZEFRAME);
		yPos-=GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYCAPTION);
		xScr += GetSystemMetrics(SM_CXSIZEFRAME)*2;
		yScr += GetSystemMetrics(SM_CYSIZEFRAME)*2 + GetSystemMetrics(SM_CYCAPTION);
//		console().registerListener(&ConsoleWindow::instance());
	}
    RegisterClassEx( &wc );
    g_hWnd = bwindow?CreateWindow( classname, classname,
                              WS_OVERLAPPEDWINDOW, xPos, yPos, xScr, yScr,
							  GetDesktopWindow(), 0, hInst, 0 ):
				CreateWindow( classname, classname,
                              0, 0, 0, 1, 1,
							  GetDesktopWindow(), 0, hInst, 0 );

	ShowWindow( g_hWnd, SW_SHOWNORMAL );
	UpdateWindow( g_hWnd );

	runtime3D=CreateRuntime3D();
	if(!runtime3D->init(in.x,in.y,bwindow)){
		MessageBox(g_hWnd, "Cannot initialize 3d graphics","EasyMap",MB_OK);
		return 0;
	}

	MSG msg;
	ZeroMemory( &msg, sizeof(msg) );
	while( msg.message!=WM_QUIT )
	{
		if( PeekMessage( &msg, 0, 0U, 0U, PM_NOREMOVE ) )
		{
            if(!GetMessage(&msg, 0, 0, 0))
				break;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}else
		if(g_bActive)
			runtime3D->quant();
		else
			WaitMessage();
	}

	if(runtime3D){
		runtime3D->finit();
		delete runtime3D;
	}

	UnregisterClass( classname, hInst );
    return 0;
}

////////////////////////////Runtime3D///////////////////////

FPS fps(5000);

void Runtime3D::quant()
{
	profiler_quant();
	fps.quant();
}

void Runtime3D::DrawFps(int x,int y)
{
	char str[200];
	float cur_fps=fps.GetFPS();
	float fpsmin,fpsmax;
	fps.GetFPSminmax(fpsmin,fpsmax);

	if(cur_fps>100.0f)
		sprintf(str,"fps=%i",round(cur_fps));
	else 
	if(cur_fps>10.0f)
		sprintf(str,"fps=%.1f",round(cur_fps*10)*0.1f);
	else
		sprintf(str,"fps=%.2f",round(cur_fps*100)*0.01f);

	sprintf(str+strlen(str)," min=%.1f, max=%.1f",round(fpsmin*10)*0.1f,round(fpsmax*10)*0.1f);
	sprintf(str+strlen(str)," poly=%i",gb_RenderDevice->GetDrawNumberPolygon());

	renderDevice->OutText(x,y,str,Color4f::WHITE);
}

Vect2f Runtime3D::mousePosRelative() const
{
	return Vect2f((float)mousePos_.x/gb_RenderDevice->GetSizeX() - 0.5f, (float)mousePos_.y/gb_RenderDevice->GetSizeY() - 0.5f);
}
