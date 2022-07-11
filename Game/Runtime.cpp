#include "StdAfx.h"
#include "SoundApp.h"

#include "Terra.h"

#include "CameraManager.h"

#include "GameShell.h"
#include "UI_Logic.h"

#include "Sound.h"
#include "PlayOgg.h"
#include "Controls.h"

#include "UnitAttribute.h"
#include "RenderObjects.h"

#include "..\UserInterface\UI_Render.h"
#include "..\UserInterface\UserInterface.h"

#include "UniverseX.h"
#include "..\resource.h"

#include "..\ht\ht.h"
#include "..\\Network\\P2P_interface.h"
#include "GameOptions.h"
#include "TreeInterface.h"
#ifndef _FINAL_VERSION_
#include "Dictionary.h"
#endif
#include "..\version.h" 
#include "..\util\Console.h"
#include "..\util\ConsoleWindow.h"
#include "..\util\ZipConfig.h"
#include <CommCtrl.h>


const char* currentVersion = 
"Ver " VISTA_ENGINE_VERSION " (" __DATE__ " " __TIME__ ")";

#ifdef _SINGLE_DEMO_
const char* currentShortVersion = 
"Комтек'04 демо, v" MULTIPLAYER_VERSION
;
#endif

#ifdef _MULTIPLAYER_DEMO_
const char* currentShortVersion = 
"Multiplayer Demo (for GameSpy internal testing only), v" MULTIPLAYER_VERSION
;
#endif

#ifndef _DEMO_
const char* currentShortVersion = 
"v" MULTIPLAYER_VERSION;
#endif

GameShell* gameShell = 0;

static Vect2i windowClientSize = Vect2i(800, 600);

static bool applicationHasFocus_ = true;

HWND hWndVisGeneric=0;
HINSTANCE gb_hInstance=NULL;

static Vect2i original_screen_size(1,1);//Нужно запоминать, потому как меняется от fullscreen

HWND Win32_CreateWindow(char *WinName,int xPos,int yPos,int xScr,int yScr,WNDPROC lpfnWndProc=DefWindowProc,int dwStyle=WS_OVERLAPPEDWINDOW);
LRESULT CALLBACK VisPerimetrClient_WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

void ErrorInitialize3D();

bool applicationHasFocus()
{
	return applicationHasFocus_;
}

bool applicationIsGo()
{
	return applicationHasFocus() || (gameShell && gameShell->alwaysRun() && (!ErrH.IsErrorOrAssertHandling()) );
}

double clockf()
{
	return 0;
} 

int clocki()
{
	return 0;
}


//------------------------------

class PathInitializer
{
public:
	PathInitializer(const char* path){
		if(path)
			SetCurrentDirectory(path);
	}
};

#pragma warning(disable: 4073)
#pragma init_seg(lib)
static PathInitializer pathInitializer(check_command_line("project_path"));

//------------------------------

void RestoreGDI()
{
	if(gb_RenderDevice && terFullScreen)
	{
		//SetWindowPos(
		//	hWndVisGeneric,
		//	HWND_NOTOPMOST,
		//	0,
		//	0,
		//	0,
		//	0,
		//	SWP_NOMOVE|SWP_NOSIZE| SWP_FRAMECHANGED
		//);
		ShowWindow(hWndVisGeneric,SW_MINIMIZE);
	}

}	

void InternalErrorHandler()
{
	RestoreGDI();
	if(universeX()) 
		universeX()->allSavePlayReel();
}

Rectf aspectedWorkArea(const Rectf& windowPosition, float aspect);

void UpdateGameshellWindowSize()
{
	if(!terFullScreen)
	{
		RECT rc;
		GetClientRect(hWndVisGeneric, &rc);
		windowClientSize.x = rc.right - rc.left;
		windowClientSize.y = rc.bottom - rc.top;
	}
	if(gameShell){
		if(terFullScreen)
			windowClientSize.set(gb_RenderDevice->GetSizeX(),gb_RenderDevice->GetSizeY());

		gameShell->setWindowClientSize(windowClientSize);
		UI_Render::instance().setWindowPosition(aspectedWorkArea(Rectf(0,0, gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY()), 4.0f / 3.0f));
	}
}

#ifndef _FINAL_VERSION_
TimerData* loadTimer;
#endif

void HTManager::init()
{
	CoInitializeEx(0, COINIT_MULTITHREADED);

	static XBuffer errorHeading;
	errorHeading.SetRadix(16);
	errorHeading < currentVersion <
#ifdef _FINAL_VERSION_
		(SecuROM_Tripwire() ? " Final" : "  Final")
#else
		" Release"
#endif
		< " OS: " <= GetVersion();

	ErrH.SetPrefix(errorHeading);
	ErrH.SetRestore(InternalErrorHandler);
	SetAssertRestoreGraphicsFunction(RestoreGDI);

#ifndef _FINAL_VERSION_
	int profilerMode = 0;
	if(check_command_line_parameter("profile:", profilerMode))
		profiler_start_stop((ProfilerMode)profilerMode);
//	profiler_start_stop();
//	loadTimer = new TimerData("loadTimer");
//	loadTimer->start();
#endif

	SetThreadAffinityMask(GetCurrentThread(), 1);

	xt_get_cpuid();
	xt_getMMXstatus();
	//initclock();

	int zip = 1;
	IniManager("Game.ini").getInt("Game", "ZIP", zip);

	if(zip)
		ZipConfig::initArchives();

	initGraphics();

	loadAllLibraries();
	GameOptions::instance().gameSetup();

	gameShell = new GameShell();
	UpdateGameshellWindowSize();
};

void HTManager::done()
{
	// Logic
	delete gameShell;
	gameShell = 0;

	FinitSound();
	finitGraphics();

//	ZIPClose();

	CoUninitialize();
}

//--------------------------------
void HTManager::initGraphics()
{
	createRenderContext(IsUseHT());
	original_screen_size.set(GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN));

	DWORD overlap = WS_OVERLAPPED| WS_CAPTION| WS_SYSMENU | WS_MINIMIZEBOX;
	if(GameOptions::instance().getBool(OPTION_DEBUG_WINDOW))
		overlap |= WS_THICKFRAME | WS_MAXIMIZEBOX;
	
	hWndVisGeneric = Win32_CreateWindow(
		"Maelstrom",
		0, 0,
		GameOptions::instance().getScreenSize().x, GameOptions::instance().getScreenSize().y,
		VisPerimetrClient_WndProc,
		(GameOptions::instance().getBool(OPTION_FULL_SCREEN) ? 0 : overlap) | WS_POPUP | WS_VISIBLE);

	int ModeRender=0;
	if(!GameOptions::instance().getBool(OPTION_FULL_SCREEN))
		ModeRender |= RENDERDEVICE_MODE_WINDOW;

	if(!initRenderObjects(ModeRender, hWndVisGeneric))
		ErrorInitialize3D();
	
	vMap.prepare("RESOURCE\\WORLDS");

	initScene();

	GameShell::SetFontDirectory();
}

void HTManager::finitGraphics()
{
	gameShell->done();
	finitScene();
	finitRenderObjects();
}


//--------------------------------
void checkSingleRunning()
{
	static HANDLE hSingularEvent = 0;
	static char psSingularEventName[] = "Maelstrom";

    hSingularEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, psSingularEventName);
    if(!hSingularEvent){
		hSingularEvent = CreateEvent(0, TRUE, TRUE, psSingularEventName);
    }
	else{
		HWND hwnd = FindWindow(0, psSingularEventName);
		if(hwnd)
			SetForegroundWindow(hwnd);
		ErrH.Exit();
	}
}

//------------------------------
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
	GameOptions::instance().setTranslate();
	GameOptions::instance().loadPresets();

#ifndef _FINAL_VERSION_
	TranslationManager::instance().setTranslationsDir("Scripts\\Engine\\Translations");
	TranslationManager::instance().setLanguage(GameOptions::instance().getLanguage());

	bool enableConsole = false;
	IniManager("Game.ini").getBool("Console", "Enable", enableConsole);
	enableConsole = enableConsole && !terFullScreen;
	if(enableConsole)
		Console::instance().registerListener(&ConsoleWindow::instance());
#endif


#ifdef _FINAL_VERSION_
	checkSingleRunning();
#endif

	gb_hInstance=hInst;
	int ht;
	if(!IniManager("Game.ini").getInt("Game","HT", ht))
		ht = 1;
	HTManager* runtime_object = new HTManager(ht);
	runtime_object->setUseHT(ht?true:false);

	xassert(!(gameShell && gameShell->alwaysRun() && terFullScreen));

	bool run = true;
	MSG msg;
	while(run){
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
			if(!GetMessage(&msg, NULL, 0, 0))
				break;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}else {
			//NetworkPause handler
			static bool runapp=true;
			if(applicationIsGo()!=runapp){
				if(gameShell && (!gameShell->alwaysRun()) ){
					if(gameShell->getNetClient()){
						if(gameShell->getNetClient()->setPause(!applicationHasFocus_))
							runapp=applicationIsGo();
					}
				}
			}
//			if(gameShell && (!gameShell->alwaysRun()) ){
//				if(gameShell->getNetClient())
//					gameShell->getNetClient()->pauseQuant(applicationIsGo()));
//			}

			if(applicationIsGo())
				run = runtime_object->Quant();
			else
				WaitMessage();
		}
	}

	delete runtime_object;

	return 0;
}

//--------------------------------
void CalcRealWindowPos(int xPos,int yPos,int xScr,int yScr,bool fullscreen,Vect2i& pos,Vect2i& size)
{
	if(xScr<0) xScr=original_screen_size.x;
	if(yScr<0) yScr=original_screen_size.y;
	if(!fullscreen)
	{
		xScr += GetSystemMetrics(SM_CXSIZEFRAME)*2;
		yScr += GetSystemMetrics(SM_CYSIZEFRAME)*2 + GetSystemMetrics(SM_CYCAPTION);
		if(GameOptions::instance().getBool(OPTION_DEBUG_WINDOW))
		{
			xPos =-GetSystemMetrics(SM_CXSIZEFRAME);
			yPos =-(GetSystemMetrics(SM_CYSIZEFRAME)+GetSystemMetrics(SM_CYCAPTION));
		}else
		{
			xPos = (original_screen_size.x-xScr)/2;
			yPos = (original_screen_size.y-yScr)/2;
			if(xPos<0) xPos = 0;
			if(yPos<0) yPos = 0;
		}
	}

	pos.x=xPos;
	pos.y=yPos;
	size.x=xScr;
	size.y=yScr;
}

HWND Win32_CreateWindow(char *WinName,int xPos,int yPos,int xScr,int yScr,WNDPROC lpfnWndProc,int dwStyle)
{
	HICON hIconSm=(HICON)LoadImage(gb_hInstance,"GAME",IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
	HICON hIcon=(HICON)LoadImage(gb_hInstance,"GAME",IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_DEFAULTCOLOR);
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW,lpfnWndProc,0,0,gb_hInstance,hIcon,0,(HBRUSH)GetStockObject(BLACK_BRUSH),0,WinName, hIconSm};
    if(RegisterClassEx(&wc)==0) return 0;

	Vect2i real_pos,real_size;
	CalcRealWindowPos(xPos,yPos,xScr,yScr,terFullScreen,real_pos,real_size);
	xPos=real_pos.x;
	yPos=real_pos.y;
	xScr=real_size.x;
	yScr=real_size.y;

	HWND hWnd=CreateWindow(WinName,WinName,dwStyle,xPos,yPos,xScr,yScr,0,0,gb_hInstance,0);
	if(hWnd == 0)	{ UnregisterClass(WinName,gb_hInstance); return 0; }
	if(xScr >= GetSystemMetrics(SM_CXSCREEN) && yScr >= GetSystemMetrics(SM_CYSCREEN)
		&& !GameOptions::instance().getBool(OPTION_DEBUG_WINDOW)) 
		ShowWindow(hWnd,SHOW_FULLSCREEN);
	else
		ShowWindow(hWnd,SW_SHOWNORMAL);

	return hWnd;
}

void Win32_ReleaseWindow(HWND hWnd)
{
	ShowCursor(1);
	if(hWnd) DestroyWindow(hWnd); 
}
LRESULT CALLBACK VisPerimetrClient_WndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if(uMsg == WM_GETMINMAXINFO && GameOptions::instance().getBool(OPTION_DEBUG_WINDOW)) {
		int sx=GameOptions::instance().getScreenSize().x,sy=GameOptions::instance().getScreenSize().y;
		if(gb_RenderDevice && gb_RenderDevice->GetCurRenderWindow())
		{
			sx=gb_RenderDevice->GetSizeX();
			sy=gb_RenderDevice->GetSizeY();
		}

		if(sx == original_screen_size.x &&
		   sy == original_screen_size.y) {
			MINMAXINFO *pMinMax;
			pMinMax = (MINMAXINFO *)lParam;
			POINT& p=pMinMax->ptMaxTrackSize;
			p.x=sx;
			p.y=sy;
			p.x += GetSystemMetrics(SM_CXSIZEFRAME)*2;
			p.y += GetSystemMetrics(SM_CYSIZEFRAME)*2 + GetSystemMetrics(SM_CYCAPTION);
			return 0;
		}
	}

	if(ErrH.IsErrorOrAssertHandling())
		return DefWindowProc(hWnd,uMsg,wParam,lParam);

	if(gameShell)
		gameShell->EventHandler(uMsg, wParam, lParam);

    switch(uMsg) 
	{
	case WM_CREATE:
		break;
    case WM_PAINT:
        break;
    case WM_MOVE:
        break;
    case WM_SIZE:
		UpdateGameshellWindowSize();
/*
		if((SIZE_MAXHIDE==wParam)||(SIZE_MINIMIZED==wParam)) 
			applicationHasFocus_ = false;
		else 
			applicationHasFocus_= true;
*/
		break;
    case WM_SETCURSOR:
		//if(applicationHasFocus() && (!universe() || !universe()->destruction())){
		if(applicationHasFocus()){
			int nHittest = LOWORD(lParam);  // hit-test code 
			if(nHittest==HTCLIENT)
			{
				UI_LogicDispatcher::instance().updateCursor();
				return TRUE;
			}
        }
        break;
    case WM_CLOSE:
		#ifndef _DEMO_
			if(gameShell)
				if(debugDisableSpecialExitProcess)
					gameShell->terminate();
				else
					gameShell->checkEvent(Event(Event::GAME_CLOSE));
			else
				Win32_ReleaseWindow(hWnd);
		#endif
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0L;
	case WM_MOUSELEAVE:
		break;
	case WM_MOUSEMOVE: {
		TRACKMOUSEEVENT tme;
		tme.cbSize  = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hWnd;
		_TrackMouseEvent(&tme);
		break;
		}
    case WM_ACTIVATEAPP:
		applicationHasFocus_ = (bool)wParam;
        return 0;
	case WM_KEYUP:		
	case WM_SYSKEYUP:
		if (wParam == VK_MENU) {
			return 0;
		} else {
			break;
		}
    }
    return DefWindowProc(hWnd,uMsg,wParam,lParam);
}
