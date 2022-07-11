#include "StdAfx.h"
#include "CDKey.h"
#include "GameOptions.h"
#include "SoundApp.h"
#include "CameraManager.h"
#include "Sound.h"
#include "RenderObjects.h"
#include "UserInterface\UI_Render.h"

#include "GameOptions.h"
#ifndef _FINAL_VERSION_
#include "Serialization\Dictionary.h"
#endif
#include "version.h" 
#include "Console.h"
#include "ConsoleWindow.h"

#include "RenderObjects.h"
#include "terra.h"
#include "CameraManager.h"

#include "Game\Universe.h"
#include "Water\CircleManager.h"
#include "GameOptions.h"
#include "UserInterface\UI_Render.h"
#include "UserInterface\UserInterface.h"
#include "Game\Runtime.h"
#include "Game\IniFile.h"
#include "Render\src\Scene.h"
#include "Render\src\VisGeneric.h"
#include "Util\Win32\DebugSymbolManager.h"
#include "Util\FileUtils\FileUtils.h"

#include <process.h>
#include <malloc.h>
#include <CommCtrl.h>
#include "kdw/Win32/Window.h"

#include "UnicodeConverter.h"

const DWORD Runtime::bad_thread_id=0xFFFFFFFF;

extern FT::Font* pDefaultFont;

Runtime* Runtime::instance_;
bool Runtime::applicationHasFocus_ = true;

void __cdecl logic_thread( void * argument)
{
	if(argument){
		MissionDescription* pmission = (MissionDescription*)argument;
		MissionDescription mission = *pmission;	
		delete pmission;
		Runtime::instance()->logic_thread(&mission);
	}
	else
		Runtime::instance()->logic_thread(0);
}

const char* currentVersion = 
"Ver " VISTA_ENGINE_VERSION " (" __DATE__ " " __TIME__ ")";

LRESULT CALLBACK runtimeWndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

bool applicationHasFocus()
{
	return Runtime::applicationHasFocus_;
}

void RestoreGDI()
{
	if(gb_RenderDevice && terFullScreen && Runtime::instance())
		ShowWindow(Runtime::instance()->hWnd(),SW_MINIMIZE);
}	

void InternalErrorHandler()
{
	FinitSound();
	RestoreGDI();
	if(Runtime::instance())
		Runtime::instance()->onAbort();
}

Runtime::Runtime(HINSTANCE hInstance, bool ht)
: windowClientSize_(800, 600)
{
	DebugSymbolManager::create();

	GameOptions::instance().setTranslate();

	hInstance_ = hInstance;
	hWnd_ = 0;
	useHT_ = ht && PossibilityHT() ? true : false;

	alwaysRun_ = check_command_line("active");
	GameContinue = true;

	load_mode = false;
	load_finish = 0;

#ifndef _FINAL_VERSION_
	TranslationManager::instance().setTranslationsDir("Scripts\\Engine\\Translations");
	TranslationManager::instance().setLanguage(GameOptions::instance().getLanguage());

	if(iniFile.enableConsole && !terFullScreen)
		Console::instance().registerListener(&ConsoleWindow::instance());
#endif

	//CurrentDirectorySaver currentDir;
	//if(!CDKeyChecker().check())
	//	ErrH.Exit();
	//currentDir.restore();

#ifdef _FINAL_VERSION_
	checkSingleRunning();
#endif

	logic_thread_id = bad_thread_id;

	init_logic=false;
	end_logic=0;

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

	SetThreadAffinityMask(GetCurrentThread(), 1);

	xt_get_cpuid();
	xt_getMMXstatus();

	init();

	xassert(!(alwaysRun() && terFullScreen));
	instance_ = this;
}

Runtime::~Runtime()
{
	MT_SET_TLS(MT_LOGIC_THREAD|MT_GRAPH_THREAD);
	
	done();

	FinitSound();

	CoUninitialize();

	instance_ = 0;
}

void Runtime::init()
{
	CreateIRenderDevice(useHT());
	GameOptions::instance().filterBaseGraphOptions();

	DWORD overlap = WS_OVERLAPPED| WS_CAPTION| WS_SYSMENU | WS_MINIMIZEBOX;
	if(GameOptions::instance().getBool(OPTION_DEBUG_WINDOW))
		overlap |= WS_THICKFRAME | WS_MAXIMIZEBOX;
	
	hWnd_ = createWindow(
		GlobalAttributes::instance().windowTitle.c_str(),
		GlobalAttributes::instance().icon.c_str(),
		0, 0,
		GameOptions::instance().getScreenSize().x, GameOptions::instance().getScreenSize().y,
		runtimeWndProc,
		(GameOptions::instance().getBool(OPTION_FULL_SCREEN) ? 0 : overlap) | WS_POPUP | WS_VISIBLE);

	int renderMode = 0;
	if(!GameOptions::instance().getBool(OPTION_FULL_SCREEN))
		renderMode |= RENDERDEVICE_MODE_WINDOW;

	gb_VisGeneric->SetUseTextureCache(true);
	gb_VisGeneric->SetUseMeshCache(true);
	gb_VisGeneric->SetFavoriteLoadDDS(true);
	gb_VisGeneric->SetEffectLibraryPath("RESOURCE\\FX","RESOURCE\\FX\\TEXTURES");

	gb_VisGeneric->EnableOcclusion(true);

	if(GlobalAttributes::instance().enableSilhouettes)
		renderMode |= RENDERDEVICE_MODE_STENCIL;

	gb_RenderDevice->SetMultisample(GameOptions::instance().getInt(OPTION_ANTIALIAS));

	if(!gb_RenderDevice->Initialize(GameOptions::instance().getScreenSize().x, GameOptions::instance().getScreenSize().y, renderMode, hWnd_, 0))
	{
		gb_RenderDevice->SetMultisample(0);
		SetWindowPos(hWnd_,0,0,0,800,600,SWP_SHOWWINDOW| SWP_FRAMECHANGED);
		if(!gb_RenderDevice->Initialize(800,600,renderMode,hWnd_, 0))
			ErrH.Abort(w2a(GET_LOC_STR(UI_COMMON_TEXT_ERROR_GRAPH_INIT)).c_str());
	}

	GameOptions::instance().filterGraphOptions();

//	gb_VisGeneric->SetRestrictionLOD(GameOptions::instance().getInt(OPTION_TEXTURE_DETAIL_LEVEL) == 0 ? 1 : 0);

	GameOptions::instance().graphSetup();

	UI_Render::create();

	setWindowPicture(GlobalAttributes::instance().startScreenPicture_.c_str());

	updateDefaultFont();

	//setSilhouetteColors();

	//	gb_VisGeneric->SetLodDistance(GlobalAttributes::instance().lod12, GlobalAttributes::instance().lod23);

	createScene();
	
	updateWindowSize();
}

void Runtime::done()
{
	destroyScene();

	gb_RenderDevice->SetDefaultFont(0);
	gb_RenderDevice->SetFont(0);
	FT::fontManager().releaseFont(pDefaultFont);
	RELEASE(gb_RenderDevice);
}

void Runtime::updateWindowSize()
{
	if(!terFullScreen)
	{
		RECT rc;
		GetClientRect(hWnd_, &rc);
		windowClientSize_.x = rc.right - rc.left;
		windowClientSize_.y = rc.bottom - rc.top;
	}
	else
		windowClientSize_.set(gb_RenderDevice->GetSizeX(),gb_RenderDevice->GetSizeY());

	UI_Render::instance().setWindowPosition(aspectedWorkArea(Rectf(0,0, gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY()), 4.0f / 3.0f));
}

void Runtime::repositionWindow(Vect2i size)
{
	Vect2i real_pos;
	Vect2i real_size;

	DWORD style = WS_OVERLAPPED| WS_CAPTION| WS_SYSMENU | WS_MINIMIZEBOX;
	if(GameOptions::instance().getBool(OPTION_DEBUG_WINDOW))
		style |= WS_THICKFRAME | WS_MAXIMIZEBOX;

	style = (terFullScreen ? 0 : style)|WS_POPUP|WS_VISIBLE;

	SetWindowLong(gb_RenderDevice->GetWindowHandle(),GWL_STYLE, style);

	calcRealWindowPos(
		0, 0,
		size.x, size.y,
		terFullScreen,
		style,
		real_pos,
		real_size
		);

	SetWindowPos(
		gb_RenderDevice->GetWindowHandle(),
		HWND_NOTOPMOST,
		real_pos.x,
		real_pos.y,
		real_size.x,
		real_size.y,
		SWP_SHOWWINDOW | SWP_FRAMECHANGED
		);
}

void Runtime::updateDefaultFont()
{
	gb_RenderDevice->SetDefaultFont(0);
	gb_RenderDevice->SetFont(0);
	FT::fontManager().releaseFont(pDefaultFont);
	
	int size = DebugPrm::instance().debugFontSize / 768.f * gb_RenderDevice->GetSizeY();
	FT::FontParam prm;
	if(size <= 8 || size > 16)
		prm.antialiasing = true;

	pDefaultFont = FT::fontManager().createFont(default_font_name.c_str(), DebugPrm::instance().debugFontSize / 768.f * gb_RenderDevice->GetSizeY(), &prm);
	xassert(pDefaultFont);

	gb_RenderDevice->SetDefaultFont(pDefaultFont);
}

void Runtime::updateResolution(Vect2i size, bool change_size)
{
	repositionWindow(size);
	gb_RenderDevice->SetMultisample(GameOptions::instance().getInt(OPTION_ANTIALIAS));

	int mode = RENDERDEVICE_MODE_RETURNERROR;
	if(!terFullScreen)
		mode |= RENDERDEVICE_MODE_WINDOW;

	if(!gb_RenderDevice->ChangeSize(size.x, size.y, mode)){
		gb_RenderDevice->SetMultisample(0);
		if(!gb_RenderDevice->ChangeSize(800,600,mode))
			ErrH.Abort(w2a(GET_LOC_STR(UI_COMMON_TEXT_ERROR_GRAPH_INIT)).c_str());
	}

	updateDefaultFont();

	repositionWindow(Vect2i(gb_RenderDevice->GetSizeX(),gb_RenderDevice->GetSizeY()));

	if(cameraManager)
		cameraManager->SetFrustumGame();

	updateWindowSize();
}

void Runtime::checkSingleRunning()
{
	static HANDLE hSingularEvent = 0;
	static char psSingularEventName[] = "Perimeter 2";

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

void Runtime::GameStart(const MissionDescription& mission)
{
	setLogicFp();
	MT_SET_TLS(MT_GRAPH_THREAD | MT_LOGIC_THREAD);

	UI_Dispatcher::instance().setLoadingScreen();
	UI_Dispatcher::instance().setEnabled(true);
	UI_Dispatcher::instance().quickRedraw();

	if(useHT_){
		xassert(logic_thread_id==bad_thread_id);
		load_mode = true;
		MissionDescription* pmission = new MissionDescription;
		*pmission = mission;
		logic_thread_id=_beginthread(::logic_thread, 1000000, pmission);
	}
	else {
		GameLoad(mission);
		GameRelaxLoading();
	}

	MT_SET_TLS(MT_GRAPH_THREAD);
}

void Runtime::GameClose()
{
	MTG();
	if(useHT_ && logic_thread_id!=bad_thread_id)
	{
		end_logic=CreateEvent(NULL,FALSE,FALSE,NULL);

		DWORD ret=WaitForSingleObject(end_logic,INFINITE);
		xassert(ret==WAIT_OBJECT_0);
		
		CloseHandle(end_logic);
		end_logic=NULL;
		logic_thread_id=bad_thread_id;
	}

	MT_SET_TLS(MT_LOGIC_THREAD|MT_GRAPH_THREAD);
}

void Runtime::logic_thread(const MissionDescription* mission)
{
	_alloca(4096+128);
	
	SetThreadAffinityMask(GetCurrentThread(), 2);

	CoInitialize(NULL);
	init_logic = true;
	SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
	init_logic = false;

	setLogicFp();

	if(mission){
		MT_SET_TLS(MT_GRAPH_THREAD | MT_LOGIC_THREAD);
		GameLoad(*mission);

		load_finish = CreateEvent(0, false, false, 0); // строго однопотоковые действия после загрузки
		DWORD ret = WaitForSingleObject(load_finish, INFINITE);
		xassert(ret == WAIT_OBJECT_0);
		CloseHandle(load_finish);
		load_finish = 0;
	}

	MT_SET_TLS(MT_LOGIC_THREAD);

	while(end_logic == NULL){
		start_timer_auto();

		if(applicationRuns())
			logicQuantHT();
		else
			Sleep(10);
	}

	SetEvent(end_logic);
}

bool Runtime::quant()
{
	if(useHT()){
		if(init_logic)
			Sleep(100);

		if(load_finish == 0) {
			MT_SET_TLS(MT_GRAPH_THREAD);
			graphicsQuant();
		}
		else {
			MT_SET_TLS(MT_GRAPH_THREAD | MT_LOGIC_THREAD);
			GameRelaxLoading();
			load_mode = false;
			SetEvent(load_finish);
			while(load_finish != 0)
				Sleep(100);
		}
	}
	else{
		MT_SET_TLS(MT_LOGIC_THREAD);//Для MTG, MTL ассертов.

		logicQuantST();

		MT_SET_TLS(MT_GRAPH_THREAD);

		graphicsQuant();
	}
	
	return GameContinue;
}

bool Runtime::PossibilityHT()
{
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);

	return sys_info.dwNumberOfProcessors>=2;
}

void Runtime::createScene()
{
	terScene = gb_VisGeneric->CreateScene();
	Vect3f dir(-234,0,-180);
	dir.normalize();
	terScene->SetSunDirection(dir);

	cameraManager = new CameraManager(terScene->CreateCamera());
}

void Runtime::destroyScene()
{
	AttributeBase::releaseModel();

	if(cameraManager){
		delete cameraManager;
		cameraManager = NULL;
	}

	RELEASE(terScene);
}

void Runtime::restoreFocus()
{ 
	SetFocus(gb_RenderDevice->GetWindowHandle()); 
}

void Runtime::setWindowPicture(const char* file)
{
	xassert(gb_VisGeneric);
	xassert(gb_RenderDevice);

	gb_RenderDevice->Fill(0, 0, 0, 255);
	gb_RenderDevice->BeginScene();

	if(file && *file)
		if(cTexture* texture = UI_Render::instance().createTexture(file)){
			int screenWidth = gb_RenderDevice->GetSizeX();
			int screenHeight = gb_RenderDevice->GetSizeY();

			Vect2i size(texture->GetWidth(), texture->GetHeight());

			if(size.x > screenWidth || size.y > screenHeight){
				float textureRatio = (float)size.x / (float)size.y;
				if(textureRatio < (float)screenWidth / (float)screenHeight){ // пустые места по бокам
					size.y = screenHeight;
					size.x = textureRatio * size.y;
				}
				else { // пустое поле сверху и снизу
					size.x = screenWidth;
					size.y = size.x / textureRatio;
				}
			}
		
			gb_RenderDevice->DrawSprite(
				(screenWidth - size.x) / 2, (screenHeight - size.y) / 2,
				size.x, size.y,
				0, 0,
				1, 1,
				texture);

			UI_Render::instance().releaseTexture(texture);
		}

	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();
}

void Runtime::calcRealWindowPos(int xPos,int yPos,int xScr,int yScr,bool fullscreen, unsigned int windowStyle, Vect2i& pos,Vect2i& size)
{
	Vect2i screenSize(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	if(xScr<0) xScr = screenSize.x;
	if(yScr<0) yScr = screenSize.y;
	if(!fullscreen){
		if(!GameOptions::instance().getBool(OPTION_DEBUG_WINDOW)){
			xPos = (screenSize.x-xScr)/2;
			yPos = (screenSize.y-yScr)/2;
		}
		RECT rect = { xPos, yPos, xPos + xScr, yPos + yScr };
		AdjustWindowRect(&rect, windowStyle, FALSE);
		if(!GameOptions::instance().getBool(OPTION_DEBUG_WINDOW) &&
		   (xScr != screenSize.x || yScr != screenSize.y))
		{
			if(rect.left < 0){
				rect.right -= rect.left;
				rect.left = 0;
			}
			if(rect.top < 0){
				rect.bottom -= rect.top;
				rect.top = 0;
			}
		}
		pos.x = rect.left;
		pos.y = rect.top;
		size.x = rect.right - rect.left;
		size.y = rect.bottom - rect.top;
	}
	else{
		pos.x=xPos;
		pos.y=yPos;
		size.x=xScr;
		size.y=yScr;
	}
}

HWND Runtime::createWindow(const char* title, const char* icon, int xPos,int yPos,int xScr,int yScr,WNDPROC lpfnWndProc,int dwStyle)
{
	HICON hIconSm, hIcon;
	if(!strlen(icon)){
		hIconSm = (HICON)LoadImage(hInstance_, "GAME", IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
		hIcon = (HICON)LoadImage(hInstance_, "GAME", IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	}
	else{
		hIconSm = (HICON)LoadImage(0, icon, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR | LR_LOADFROMFILE);
		hIcon = (HICON)LoadImage(0, icon, IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR | LR_LOADFROMFILE);
	}
	
	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),
		CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW,
		lpfnWndProc,
		0,
		0,
		hInstance_,
		hIcon,
		0,
		(HBRUSH)GetStockObject(BLACK_BRUSH),
		0,
		title, 
		hIconSm
	};
    if(RegisterClassEx(&wc) == 0)
		return 0;

	Vect2i real_pos, real_size;
	calcRealWindowPos(xPos, yPos, xScr, yScr, terFullScreen, dwStyle, real_pos, real_size);
	HWND hWnd = CreateWindow(title, title, dwStyle, real_pos.x, real_pos.y, real_size.x, real_size.y, 0, 0, hInstance_, 0);
	if(hWnd == 0){
		UnregisterClass(title,hInstance_);
		return 0;
	}
	ShowWindow(hWnd,SW_SHOWNORMAL);
	return hWnd;
}

void Runtime::onSetFocus(bool focus)
{
	applicationHasFocus_ = focus;
}

//--------------------------------

LRESULT CALLBACK runtimeWndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	Vect2i screenSize(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	if(uMsg == WM_GETMINMAXINFO){
		int sx = GameOptions::instance().getScreenSize().x;
		int sy = GameOptions::instance().getScreenSize().y;
		if(gb_RenderDevice && gb_RenderDevice->currentRenderWindow()){
			sx=gb_RenderDevice->GetSizeX();
			sy=gb_RenderDevice->GetSizeY();
		}

		if(sx == screenSize.x && sy == screenSize.y){
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

	if(Runtime::instance()){
		Runtime::instance()->eventHandler(uMsg, wParam, lParam);

		switch(uMsg){
		case WM_SIZE:
			Runtime::instance()->updateWindowSize();
			//HTManager::instance()->onSetFocus(wParam != SIZE_MAXHIDE && wParam != SIZE_MINIMIZED);
			break;
		case WM_ACTIVATEAPP:
			Runtime::instance()->onSetFocus((bool)wParam);
			return 0;
		case WM_SETCURSOR:
			if(applicationHasFocus()){
				int nHittest = LOWORD(lParam);  // hit-test code 
				if(nHittest==HTCLIENT){
					Runtime::instance()->onSetCursor();
					return TRUE;
				}
			}
			break;
		case WM_CLOSE:
			Runtime::instance()->onClose();
			return 0;
		}
	}

    switch(uMsg){
	case WM_CREATE:
		break;
    case WM_PAINT:
        break;
    case WM_MOVE:
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0L;
	case WM_MOUSEMOVE: {
		TRACKMOUSEEVENT tme;
		tme.cbSize  = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hWnd;
		_TrackMouseEvent(&tme);
		break;
		}
   	case WM_MOUSELEAVE:
		break;
	case WM_KEYUP:		
	case WM_SYSKEYUP:
		if(wParam == VK_MENU) 
			return 0;
		break;
    }
    return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

//------------------------------
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
	Win32::_setGlobalInstance(hInst);

	//profiler_start_stop();
    Runtime* runtime = createRuntime(hInst);
	//profiler_start_stop();

	MSG msg;
	while(true){
		if(PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)){
			if(!GetMessage(&msg, 0, 0, 0))
				break;
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else{
			if(runtime->applicationRuns()){
				if(!runtime->quant())
					break;
			}
			else
				WaitMessage();
		}
	}

	delete runtime;

	return 0;
}


void updateResolution(Vect2i size, bool change_size)
{
	Runtime::instance()->updateResolution(size, change_size);
}
