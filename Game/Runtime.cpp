#include "StdAfx.h"
#include "Platform/Window.h"
#include "CDKey.h"
#include "GameOptions.h"
#include "SoundApp.h"
#include "CameraManager.h"
#include "Sound.h"
#include "RenderObjects.h"
#include "UserInterface/UI_Render.h"

#include "GameOptions.h"
#ifndef _FINAL_VERSION_
#include "Serialization/Dictionary.h"
#endif
#include "version.h" 
#include "Console.h"
#include "ConsoleWindow.h"

#include "RenderObjects.h"
#include "terra.h"
#include "CameraManager.h"

#include "Game/Universe.h"
#include "Water/CircleManager.h"
#include "GameOptions.h"
#include "UserInterface/UI_Render.h"
#include "UserInterface/UserInterface.h"
#include "Game/Runtime.h"
#include "Game/IniFile.h"
#include "Render/src/Scene.h"
#include "Render/src/VisGeneric.h"
#include "Util/Win32/DebugSymbolManager.h"
#include "Util/FileUtils/FileUtils.h"

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
	// Get the game out of the way before an error dialog goes up.
	if(gb_RenderDevice && terFullScreen && Runtime::instance())
		PlatformWindow::minimize();
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

	hWnd_ = createWindow(
		GlobalAttributes::instance().windowTitle.c_str(),
		GameOptions::instance().getScreenSize().x, GameOptions::instance().getScreenSize().y);

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
		PlatformWindow::setSize(800, 600);
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
	// The swapchain size is the client size: SDL's window has no non-client frame of
	// its own to subtract, and this is what mouse coords normalize against
	// (GameShell::convert).
	windowClientSize_.set(gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY());

	UI_Render::instance().setWindowPosition(aspectedWorkArea(Rectf(0,0, gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY()), 4.0f / 3.0f));
}

void Runtime::repositionWindow(Vect2i size)
{
	PlatformWindow::setSize(size.x, size.y);
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

#ifndef _WIN32
	// Cross-platform bring-up: _beginthread is a no-op off-Windows, so a threaded
	// (useHT) load would never actually run GameLoad. Always load synchronously,
	// single-threaded, so the world (and its trigger chains) actually build.
	GameLoad(mission);
	GameRelaxLoading();
#else
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
#endif

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
	PlatformWindow::focus();
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

HWND Runtime::createWindow(const char* title, int xScr, int yScr)
{
	// One window, created by SDL3 on every platform: the SDL GPU device claims its
	// swapchain on it, and its events are the engine's input. What we hold on to
	// here is the OS handle -- a real HWND on Windows, since DirectSound,
	// DirectInput and the kdw dialogs are handed it; the renderer takes the
	// SDL_Window* from PlatformWindow::current() instead.
	if(!PlatformWindow::create(title, xScr, yScr))
		return 0;
	return PlatformWindow::nativeHandle();
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
// Feed a translated SDL input event into the window procedure. It is no longer a
// WNDPROC registered with Win32 -- SDL owns the window now -- but it is still the
// function that turns a WM_* triple into a GameShell event. See Platform/Window.h.
static void dispatchWindowEvent(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HWND hWnd = Runtime::instance() ? Runtime::instance()->hWnd() : 0;
	runtimeWndProc(hWnd, uMsg, wParam, lParam);
}

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
	Win32::_setGlobalInstance(hInst);

	//profiler_start_stop();
    Runtime* runtime = createRuntime(hInst);
	//profiler_start_stop();

	// SDL's is the only pump: on Windows SDL_PollEvent drains the OS message queue
	// itself, so a PeekMessage loop next to it would race it for messages.
	while(true){
		if(!PlatformWindow::pumpEvents(&dispatchWindowEvent))
			break;

		if(runtime->applicationRuns()){
			if(!runtime->quant())
				break;
		}
		else
			PlatformWindow::waitEvents();
	}

	delete runtime;

	return 0;
}


void updateResolution(Vect2i size, bool change_size)
{
	Runtime::instance()->updateResolution(size, change_size);
}
