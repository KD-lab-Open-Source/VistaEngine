#include "stdafx.h"
#include "Runtime3D.h"
#include <queue>
#include "fps.h"

#include "..\..\Util\console.h"
#include "..\..\Util\ConsoleWindow.h"


HWND g_hWnd=NULL;

Runtime3D* g_runtime=NULL;
cInterfaceRenderDevice	*terRenderDevice=0;
cScene					*terScene=0;
cCamera					*gb_Camera=0;

BOOL g_bActive=TRUE;//FALSE;

void SetCameraPosition(cCamera *UCamera,const MatXf &Matrix)
{
	MatXf ml=MatXf::ID;
	ml.rot()[2][2]=-1;

	MatXf mr=MatXf::ID;
	mr.rot()[1][1]=-1;
	MatXf CameraMatrix;
	CameraMatrix=mr*ml*Matrix;

	UCamera->SetPosition(CameraMatrix);
}

int InitRenderDevice(int xScr,int yScr,bool bwindow)
{
	terRenderDevice=CreateIRenderDevice(true);

	//vector<DWORD> multisamplemode;
	//terRenderDevice->CheckDeviceType(xScr,yScr,RENDERDEVICE_MODE_WINDOW|RENDERDEVICE_MODE_STENCIL,&multisamplemode);

	if(!terRenderDevice->Initialize(xScr,yScr,
		RENDERDEVICE_MODE_STENCIL|(bwindow?RENDERDEVICE_MODE_WINDOW:0)
		,g_hWnd))
		return 1;
/*
	// создание сцены
	terScene=gb_VisGeneric->CreateScene(); 
	// создание камеры
	gb_Camera=terScene->CreateCamera();
	gb_Camera->SetAttr(ATTRCAMERA_PERSPECTIVE); // перспектива
	MatXf CameraMatrix;
	Identity(CameraMatrix);
	Vect3f CameraPos(0,0,-512);
	SetPosition(CameraMatrix,CameraPos,Vect3f(0,0,0));
	SetCameraPosition(gb_Camera,CameraMatrix);

	gb_Camera->SetFrustum(							// устанавливается пирамида видимости
		&Vect2f(0.5f,0.5f),							// центр камеры
		&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),		// видимая область камеры
		&Vect2f(1.f,1.f),							// фокус камеры
		&Vect2f(30.0f,13000.0f)
		//&Vect2f(30.0f,2000.0f)
		);

//	gb_Camera->SetFrustrumPositionAutoCenter(sRectangle4f(200/1280.0f,200/1024.0f,300/1280.0f,600/1024.0f),1);

	terScene->SetSun(Vect3f(-1,0,-1),sColor4f(1,1,1,1),sColor4f(1,1,1,1),sColor4f(1,1,1,1));
*/
	return 0;
}

void DoneRenderDevice()
{
	RELEASE(gb_Camera);
	RELEASE(terScene);
	// закрытие окна вывода
	RELEASE(gb_VisGeneric);
	RELEASE(terRenderDevice);
}

//Функция обработки сообщений Windows
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY://выйти из приложения, если нажали на крестик
            PostQuitMessage( 0 );
            return 0;
		case WM_ACTIVATEAPP:
			g_bActive = (wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE);
			return 0;
		case WM_MOUSEMOVE:
			if(g_runtime)
				g_runtime->SetMousePos(LOWORD(lParam),HIWORD(lParam)); 
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
		case WM_KEYDOWN:
			g_runtime->KeyDown(wParam);
			break;
		case WM_SETCURSOR:
			SetCursor(LoadCursor(NULL,IDC_ARROW));
			break;
		case WM_LBUTTONDOWN:
			g_runtime->OnLButtonDown();
			break;
		case WM_LBUTTONUP:
			g_runtime->OnLButtonUp();
			break;
		case WM_RBUTTONDOWN:
			g_runtime->OnRButtonDown();
			break;
		case WM_RBUTTONUP:
			g_runtime->OnRButtonUp();
			break;
    }
    return DefWindowProc( hWnd, msg, wParam, lParam );
}

Vect2i winmain_size_window(-1,-1);

INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
	LPCSTR classname="3d demo";
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC|CS_HREDRAW |CS_VREDRAW, MsgProc, 0L, 0L,
                      hInst, NULL, NULL, NULL, NULL,
                      classname, NULL };

	bool bwindow=true;
	int xPos=0,yPos=0;
//	Vect2i in(GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));
//	Vect2i in(800,600);
	Vect2i in(1024,768);
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
    HWND hWnd = bwindow?CreateWindow( classname, classname,
                              WS_OVERLAPPEDWINDOW, xPos, yPos, xScr, yScr,
							  GetDesktopWindow(), NULL, hInst, NULL ):
				CreateWindow( classname, classname,
                              0, 0, 0, 1, 1,
							  GetDesktopWindow(), NULL, hInst, NULL );
	g_hWnd=hWnd;

	ShowWindow( hWnd, SW_SHOWNORMAL );
	UpdateWindow( hWnd );

	if(InitRenderDevice(in.x,in.y,bwindow))
	{
		MessageBox(hWnd,"Cannot initialize 3d graphics","EasyMap",MB_OK);
		return 0;
	}

	g_runtime=CreateRuntime3D();
	g_runtime->Init();

	MSG msg;
	ZeroMemory( &msg, sizeof(msg) );
	while( msg.message!=WM_QUIT )
	{
		if( PeekMessage( &msg, NULL, 0U, 0U, PM_NOREMOVE ) )
		{
            if(!GetMessage(&msg, NULL, 0, 0))
				break;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}else
		if(g_bActive)
			g_runtime->Quant();
		else
			WaitMessage();
	}

	if(g_runtime)
		delete g_runtime;

	DoneRenderDevice();

    UnregisterClass( classname, hInst );
    return 0;
}

////////////////////////////Runtime3D///////////////////////

FPS fps;

void Runtime3D::Quant()
{
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

	terRenderDevice->OutText(x,y,str,sColor4f(1,1,0,1));
}
