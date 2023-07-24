#include "stdafx.h"
#include "SurMap5.h"
#include ".\GeneralView.h"

#include "MainFrame.h"
#include "ToolsTreeWindow.h"
#include "ObjectsManagerWindow.h"
#include "MiniMapWindow.h"
#include "TimeSliderDlg.h"

#include "BaseUniverseObject.h"
#include "..\Render\inc\IVisGeneric.h"
#include "..\Render\inc\TerraInterface.inl"
#include "..\Util\Console.h"
#include "..\Util\ConsoleWindow.h"
#include "..\Water\Water.h"
#include "..\Render\src\postEffects.h"
#include "..\Environment\Environment.h"
#include "..\Game\SoundApp.h"
#include "..\Game\Universe.h"
#include "..\Game\CameraManager.h"
#include "..\UserInterface\UserInterface.h"
#include "..\UserInterface\UI_BackgroundScene.h"
#include "..\UserInterface\UI_CustomControls.h"
#include "GameOptions.h"

#include "SelectionUtil.h"
#include "SurMapOptions.h"
#include "SurToolSelect.h"
#include "SurToolSource.h"
#include "EditorVisual.h"

#include "..\Units\ExternalShow.h"
#include "Dictionary.h"

#include "..\Water\SkyObject.h"

#include "StreamCommand.h"

#include "XPrmArchive.h"
#include "SystemUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class cMonowideFont {
	cFont* pfont;
public:
	cMonowideFont(){
		pfont=0;
	}
	~cMonowideFont(){
		if(pfont) pfont->Release();
	}
	cFont* getFont(){
		if(!pfont) {
			pfont=gb_VisGeneric->CreateFont("Scripts\\Resource\\fonts\\Courier New.font", 16, 1);//Тихая ошибка
			if(!pfont)pfont=gb_VisGeneric->CreateFont("Scripts\\Resource\\fonts\\Arial.font", 16);
		}
		return pfont;
	}
};

cMonowideFont* pMonowideFont;

// CGeneralView

CGeneralView::CGeneralView()
: renderWindow_(0)
, currentMission_(new MissionDescription)
{
	flag_restrictionOnPaint=false;
	renderDeviceInited_ = 0;
	sceneInited_ = 0;

	toolsWindow_ =0;

	//demo3d
	prevtime=0;
	terMapPoint=NULL;
	pFont=NULL;
	///vPosition.set(945.894f,2042.05f,2197.22f);
	vPosition.set(945.894f,1042.05f,2197.22f);
	AnglePosition.set(0.404606f,7.91086f);
	wireframe=false;

	m_Timer4RepetitionID=0;

	pointOnMouse=Vect3f::ZERO;
	pMonowideFont=0;
	draw_num_polygon=0;
	draw_num_tilemappolygon=0;
}

CGeneralView::~CGeneralView()
{
	RELEASE(pFont);
	renderDeviceInited_=0;
	sceneInited_ = 0;
}

CSurToolBase* CGeneralView::getCurCtrl(void) 
{
	if(toolsWindow_ && ::IsWindow(toolsWindow_->GetSafeHwnd()))
		return toolsWindow_->currentTool();
	return 0;
}

void CGeneralView::initRenderDevice()
{
	createRenderContext(false);

	initRenderObjects(RENDERDEVICE_MODE_WINDOW|RENDERDEVICE_MODE_STENCIL, GetSafeHwnd());
	renderWindow_ = gb_RenderDevice->CreateRenderWindow (GetSafeHwnd ());

	GameOptions::instance().gameSetup();

	renderDeviceInited_ = true;

	loadAllLibraries(true);
	surMapOptions.load();
	UI_Dispatcher::instance().init();
	minimap().setViewParameters(1.f, true, false, true, true);
}

void CGeneralView::doneRenderDevice()
{
	UI_BackgroundScene::instance().done();
	UI_Dispatcher::instance().releaseResources();
	FinitSound();
	RELEASE (renderWindow_);
	CMainFrame* pMF=(CMainFrame*)AfxGetMainWnd();
	pMF->miniMapWindow().doneRenderDevice ();
	finitRenderObjects();
	renderDeviceInited_ = 0;
}

void CGeneralView::createScene()
{
	doneScene();

	if(!renderDeviceInited_)
		return;

	initScene();

    CameraCoordinate coord;

	coord.psi() = 90.0f * M_PI / 180.0f;
    coord.theta() = 0.0f;
	coord.distance() = 512.f;
    coord.position() = Vect3f(vMap.H_SIZE * 0.5f, vMap.V_SIZE * 0.5f, 128.0f);

    cameraManager->setCoordinate (coord);

	cameraManager->SetFrustumEditor();

	terScene->SetSunDirection(Vect3f(-1,0,-1));

	CSurToolBase* cc=getCurCtrl();
	if(cc) 
		cc->CallBack_CreateScene();

	CMainFrame* pMainFrame = static_cast<CMainFrame*>(AfxGetMainWnd ());

	if(pMonowideFont) delete pMonowideFont;
	pMonowideFont= new cMonowideFont();

}

void CGeneralView::doneUniverse()
{
	if(CSurToolBase* currentTool = getCurCtrl())
		currentTool->CallBack_ReleaseScene();

	UI_Dispatcher::instance().reset();
	//extern StreamDataManager uiStreamLogicCommand;
	//uiStreamLogicCommand.lock();
	//uiStreamLogicCommand.execute();
	//uiStreamLogicCommand.unlock();
	//uiStreamCommand.execute(); // так можно только при одном потоке

	if(universe())
		delete universe();
}

void CGeneralView::doneScene(void)
{
	delete pMonowideFont;
	pMonowideFont = 0;
	sceneInited_ = 0;

	doneUniverse();
	
	RELEASE(terMapPoint);//Подстраховка

	finitScene();
}

void CGeneralView::reInitWorld()
{
	CWaitCursor waitCursor;

	doneUniverse();

	if(!renderDeviceInited_)
		return;

	if(CSurToolBase* currentTool = getCurCtrl())
		currentTool->CallBack_CreateScene();

	vMap.ShadowControl(true);
	
	pointOnMouse = Vect3f::ZERO;

	std::string spgPath = string(vMap.getWorldsDir()) + "\\" + vMap.worldName + ".spg";

	XPrmIArchive ia;
	if(ia.open(spgPath.c_str())){
		*currentMission_ = MissionDescription(spgPath.c_str());
		new Universe(*currentMission_, &ia);
	}
	else{
		*currentMission_ = MissionDescription();
		new Universe(*currentMission_, 0);
	}

	universe()->setUseHT(false);
	universe()->relaxLoading();

	global_time.set(0, logicTimePeriod, 1000);
	frame_time.set(1, logicTimePeriod, 1000);
	scale_time.set(1, logicTimePeriod, 1000);

	frame_time.adjust();
	scale_time.adjust();

	sceneInited_ = true;

	surMapOptions.apply();

	CMainFrame* mainFrame = (CMainFrame*)(AfxGetMainWnd());
	mainFrame->eventWorldChanged().emit();

	setSilhouetteColors();


	cameraManager->reset();
	CameraCoordinate coord = cameraManager->coordinate();
	if(lastWorldName.empty() || (lastWorldName!=vMap.worldName)){
        coord.position() = To3D(Vect2f(vMap.H_SIZE * 0.5f, vMap.V_SIZE * 0.5f));
	}
	cameraManager->setCoordinate(coord);
	cameraManager->setRestriction(false);
	lastWorldName = vMap.worldName;

	Invalidate(FALSE);
}

void CGeneralView::graphQuant()
{
	if(gb_RenderDevice->IsInBeginEndScene())
		return;

	CMainFrame* pMF=(CMainFrame*)AfxGetMainWnd();
	if(universe()){
		cameraManager->SetFrustum();
		gb_VisGeneric->SetGraphLogicQuant(universe()->quantCounter());

		universe()->streamCommand.process(0.0f);
		universe()->streamCommand.clear();

		uiStreamCommand.execute();

		gb_VisGeneric->SetInterpolationFactor(0);
		universe()->streamInterpolator.process(0);
	}

	gb_RenderDevice->SelectRenderWindow(renderWindow_);

	frame_time.next_frame();

	sColor4c& Color = environment->environmentTime()->GetCurFoneColor();
	gb_RenderDevice->Fill(Color.r,Color.g,Color.b);
	gb_RenderDevice->BeginScene();

	editorVisual().beforeQuant();

	environment->environmentTime()->Draw();
	
	gb_RenderDevice->SetRenderState(RS_FILLMODE, debugWireFrame ? FILL_WIREFRAME : FILL_SOLID);

	float dt = 0.001f*terScene->GetDeltaTime();

	environment->graphQuant(dt);
	cameraManager->GetCamera()->SetAttr(ATTRCAMERA_CLEARZBUFFER);//Потому как в небе могут рисоваться планеты в z buffer.
	terScene->Draw(cameraManager->GetCamera());
	drawGrid();

	environment->drawPostEffects(dt);

	gb_RenderDevice->SetRenderState(RS_FILLMODE, FILL_SOLID);

	cameraManager->showEditor();

	if(universe()){
		universe()->circleShow()->Lock();
		universe()->circleShow()->Clear();
	}
	
	universe()->graphQuant(dt);
	UI_Dispatcher::instance().quant(dt);

	environment->showEditor();
	
	if(universe()){
		universe()->circleShow()->Unlock();
		universe()->circleShow()->Quant(dt);
	}

	CSurToolBase* cc = getCurCtrl();
	if(cc) 
		cc->CallBack_DrawAuxData();

	editorVisual().afterQuant();

	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();

	gb_RenderDevice->SelectRenderWindow(0);
	draw_num_polygon=gb_RenderDevice->GetDrawNumberPolygon();
	draw_num_tilemappolygon=gb_RenderDevice->GetDrawNumberTilemapPolygon();

	pMF->fps.quant();
}

void CGeneralView::CameraQuant(float dt)
{
	cameraManager->quant(0,0,dt);
}

void CGeneralView::Animate(float dt)
{
	terScene->SetDeltaTime(dt);
}


BEGIN_MESSAGE_MAP(CGeneralView, CWnd)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_KEYDOWN()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CGeneralView message handlers

BOOL CGeneralView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CWnd::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	return TRUE;
}

void CGeneralView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	if(!renderDeviceInited_ || !sceneInited_) {
		CRect rt;
		GetClientRect(&rt);
		dc.FillSolidRect(&rt, GetSysColor(COLOR_APPWORKSPACE));
		return;
	}

	static bool flag_recursion=0;
	if(flag_recursion==1 || flag_restrictionOnPaint)
		return;
	flag_recursion=1;


	double curtime=xclock();
	float dt=min(100.0,curtime-prevtime);
	CameraQuant(dt/1000.f);
	Animate(dt);
	prevtime=curtime;

	graphQuant();

	flag_recursion=0;

	UpdateStatusBar();
	return;
}

bool CGeneralView::CoordScr2vMap(const Vect2i& inMouse_pos, Vect3f& outWorld_pos)
{
	Vect2f pos_in(inMouse_pos.x/(float)gb_RenderDevice->GetSizeX()-0.5f,
		inMouse_pos.y/(float)gb_RenderDevice->GetSizeY()-0.5f);

	Vect3f pos,dir;
	cameraManager->GetCamera()->GetWorldRay(pos_in,pos,dir);
	Vect3f trace;
	if(terScene->TraceDir(pos,dir,&trace)){
		outWorld_pos=trace;
		return true;
	}
	else {
		outWorld_pos=Vect3f::ZERO;
		return false;
	}
}

#include "..\Util\SystemUtil.h"
LRESULT CGeneralView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	//Унивепсальная смена фокуса на окно
#ifndef _VISTA_ENGINE_EXTERNAL_
	setLogicFp();
#endif
	if(message==WM_RBUTTONDOWN || message==WM_LBUTTONDOWN){
		SetFocus();
	}
	if(terScene){
	static bool flag_MMouseDown=false;
	static bool flag_LMouseDown=false;
	static bool flag_RMouseDown=false;
	static bool flag_MouseMoved=false;
	static Vect2i CurMousePos(0,0);
	static Vect2i BegMousePos(0,0);
	static float begPsi = cameraManager->coordinate().psi();
	static float begTheta = cameraManager->coordinate().theta();
	const float kMouseMove2Angle=0.25f*M_PI/180.f; //в одной точке 5 градус-ов;
	switch(message){
	case WM_MOUSEWHEEL:
		{
            short delta = HIWORD(wParam);
			WORD flags = LOWORD(wParam);
			
            CameraCoordinate coord = cameraManager->coordinate();

			if(flags & MK_SHIFT){
				Vect3f newPos = coord.position() + Vect3f(0.0f, 0.0f, -SIGN(delta) * ((flags & MK_CONTROL) ? 0.0025f : 0.01f) * coord.distance());
				newPos.z = max(newPos.z, To3D(Vect2f(newPos)).z);
                coord.setPosition(newPos);
            }
            else{
				coord.distance() = max(coord.distance(),  2.0f) * (1.0f - ((flags & MK_CONTROL) ? 0.01f : 0.2f) * SIGN(delta));
			}
            cameraManager->setCoordinate (coord);
		}
		break;
	case WM_MBUTTONDOWN:
		SetCapture();
		flag_MMouseDown=true;
		BegMousePos.x = LOWORD(lParam);
		BegMousePos.y = HIWORD(lParam);
		begPsi = cameraManager->coordinate().psi();
		begTheta = cameraManager->coordinate().theta();
		break;
	case WM_MBUTTONUP:
		flag_MMouseDown=false;
		if(this==GetCapture()){
			CurMousePos.x = LOWORD(lParam);
			CurMousePos.y = HIWORD(lParam);
			Vect2f delta = CurMousePos - BegMousePos;
			delta *= kMouseMove2Angle;
            CameraCoordinate coord = cameraManager->coordinate();
			coord.psi() = begPsi + delta.x;
			coord.theta() = begTheta - delta.y;

            cameraManager->setCoordinate (coord);
			if(!flag_MMouseDown && !flag_RMouseDown && !flag_LMouseDown)
				::ReleaseCapture();
		}
		break;
	case WM_RBUTTONDOWN:
		{
			Vect3f coordMap;
			CoordScr2vMap (CurMousePos, coordMap);
			CSurToolBase* currentToolzer = getCurCtrl();
			if(currentToolzer && currentToolzer->CallBack_RMBDown(coordMap, CurMousePos)){
				Invalidate(FALSE);
			}else{
				SetCapture();
				flag_RMouseDown=true;
				BegMousePos.x = LOWORD(lParam);
				BegMousePos.y = HIWORD(lParam);
				flag_MouseMoved = false;
			}
		}
		break;
	case WM_RBUTTONUP:
		flag_RMouseDown=false;
		if(this==GetCapture()){
			if(!flag_MMouseDown && !flag_RMouseDown && !flag_LMouseDown)
				::ReleaseCapture();
			Invalidate(FALSE);
		}
		if (! flag_MouseMoved) {
			if (CSurToolBase* currentToolzer = getCurCtrl()) {
				Vect3f coordMap;
				CoordScr2vMap (CurMousePos, coordMap );

                currentToolzer->CallBack_RMBUp (coordMap, CurMousePos);
			}
		}
		break;

	case WM_MOUSEMOVE:
		{
			#define LOSHORT(l)           ((short int)((l) & 0xffff))
			#define HISHORT(l)           ((short int)((l) >> 16))

			if((CurMousePos.x == LOSHORT(lParam)) && (CurMousePos.y == HISHORT(lParam)))
				break;
			CurMousePos.x = LOSHORT(lParam);
			CurMousePos.y = HISHORT(lParam);

			Vect3f worldCoord = screenPointToGround(CurMousePos);
			xassert(round(worldCoord.x) >= 0 && round(worldCoord.x) < vMap.H_SIZE &&
					round(worldCoord.y) >= 0 && round(worldCoord.y) < vMap.V_SIZE);					
			CoordScr2vMap(Vect2i(CurMousePos.x, CurMousePos.y), pointOnMouse);
			if(flag_MMouseDown){
				Vect2f delta = CurMousePos - BegMousePos;
				delta *= kMouseMove2Angle;
				CameraCoordinate coord = cameraManager->coordinate ();
				coord.psi () = begPsi + delta.x;
				coord.theta () = begTheta - delta.y;
				cameraManager->setCoordinate (coord);
			}
			else if(flag_RMouseDown){
				Vect2f delta = CurMousePos - BegMousePos;
				
				if(BegMousePos != CurMousePos)
					flag_MouseMoved = true;

				BegMousePos = CurMousePos;
				delta *= Mat2f(M_PI/2.f + cameraManager->coordinate().psi());
				delta *= max(cameraManager->coordinate().distance(), 100.f)/800.;
				CameraCoordinate coord = cameraManager->coordinate();

				Vect3f coord_position = coord.position();
#ifdef _VISTA_ENGINE_EXTERNAL_
				coord.position().z = worldCoord.z;
#else
				float zDelta = To3D(Vect2f(coord_position) + Vect2f(delta)).z - To3D(Vect2f(coord.position())).z;
				coord.position().z = max(0.0f, coord_position.z + zDelta);
#endif
				coord.position().x += delta.x;
				coord.position().y += delta.y;

				cameraManager->setCoordinate (coord);
			}
			else
				UpdateStatusBar(); // Если не поворот камеры
			
			if(CSurToolBase* tool = getCurCtrl()){
				tool->TrackMouse(worldCoord, CurMousePos);
			}
		}
		break;
	case WM_LBUTTONDOWN:
		{
			SetCapture();
			CurMousePos.x = LOWORD(lParam);
			CurMousePos.y = HIWORD(lParam);
loc_repeat_operationOnMap:
			
			
			Vect2i coordScr = CurMousePos;
			Vect3f coordMap = screenPointToGround(CurMousePos);
			//bool result = CoordScr2vMap(Vect2i(CurMousePos.x, CurMousePos.y), coordMap);

			CSurToolBase* tool = getCurCtrl();
			if(tool){
				CRect rect;
				GetClientRect(&rect);

				tool->CallBack_OperationOnMap(round(coordMap.x), round(coordMap.y));
				if(!flag_LMouseDown)
					tool->CallBack_LMBDown(coordMap, CurMousePos); //если первое нажатие

				UpdateStatusBar();
				Invalidate(FALSE);
			}
			flag_LMouseDown=true;
		}
		break;
	case WM_LBUTTONUP:
		flag_LMouseDown=false;
		if(this==GetCapture()){
			if(!flag_MMouseDown && !flag_RMouseDown && !flag_LMouseDown)
				::ReleaseCapture();
			if (CSurToolBase* cc=getCurCtrl()) {
				Vect3f cm;
				CoordScr2vMap(Vect2i(CurMousePos.x, CurMousePos.y), cm);
				cc->CallBack_LMBUp(cm, CurMousePos);
			}
		}
		break;
	case WM_TIMER:
		{
			CSurToolBase* cc=getCurCtrl();
			if( !cc || !cc->flag_repeatOperationEnable) 
				break;
			static int count=0;
			if(this==GetCapture()){
				if(wParam==m_Timer4RepetitionID){ //Проверка- инструментальный таймер?
					if(flag_LMouseDown==0) count=0;//обнуление счетчика повторов
					else count++;
					//Задержка перед повтором
					if(count >=2) goto loc_repeat_operationOnMap;
				}
			}
			else{ 
				count=0; 
			}
		}
		break;
	}// end switch(message)

	}//end if(terScene)
	return CWnd::WindowProc(message, wParam, lParam);
}

void CGeneralView::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);
	
	flag_restrictionOnPaint=true;

	if(cx==0 || cy==0 || !::IsWindow(GetSafeHwnd()))
		return;

	if(renderDeviceInited_){
		gb_RenderDevice->ChangeSize(cx, cy, RENDERDEVICE_MODE_WINDOW);
		//if(cameraManager)
		//	cameraManager->SetFrustumGame();
	}
	else
		initRenderDevice();

	flag_restrictionOnPaint=false;
}

BOOL CGeneralView::DestroyWindow()
{
	
	return CWnd::DestroyWindow();
}
void CGeneralView::OnDestroy()
{
	CWnd::OnDestroy();

	doneScene();
	doneRenderDevice();
	if(m_Timer4RepetitionID) KillTimer(m_Timer4RepetitionID);
}


int CGeneralView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	const int REPET_INTERVAL=200;
	m_Timer4RepetitionID=SetTimer(1, REPET_INTERVAL, 0);

	return 0;
}

unsigned int profileTimeOperation=0;
void CGeneralView::UpdateStatusBar()
{
	if(vMap.isWorldLoaded()){
		int xPointInfo=round(pointOnMouse.x);
		int yPointInfo=round(pointOnMouse.y);
		char str[128];
		char str1[64];
		char str2[64];
		char str3[64];

		CMainFrame* pMF=(CMainFrame*)AfxGetMainWnd();
		CExtStatusControlBar & statusBar = pMF->statusBar();

		unsigned char atr=vMap.GetAtr(xPointInfo, yPointInfo);
		if(Vm_IsGeo(atr))
			strncpy(str1, "Geo", sizeof(str1));
		else 
			strncpy(str1, "Dam", sizeof(str1));
		if(Vm_IsIndestructability(atr))
			strcat(str1, "+Inds");
		if(Vm_IsLeveled(atr))
			strcat(str1, "+Lev");
/*		switch(atr){
		case VmAt_Nrml_Geo:
			strncpy(str1, "    Geo     ", sizeof(str1));
			break;
		case VmAt_Soot_Geo:
			strncpy(str1, "    Geo+soot", sizeof(str1));
			break;
		case VmAt_Lvld_Geo:
			strncpy(str1, "    Geo lev ", sizeof(str1));
			break;
		case VmAt_Lvld_Dam:
			strncpy(str1, "    Dam lev ", sizeof(str1));
			break;
		case VmAt_Nrml_Dam:
			strncpy(str1, "    Dam     ", sizeof(str1));
			break;
		case VmAt_Soot_Dam:
			strncpy(str1, "    Dam+soot", sizeof(str1));
			break;
		case VmAt_Nrml_Dam_Inds:
			strncpy(str1, "indsDam     ", sizeof(str1));
			break;
		case VmAt_Soot_Dam_Inds:
			strncpy(str1, "indsDam+soot", sizeof(str1));
			break;
		}*/
		sprintf(str, "%s/0x%X", str1, vMap.GABuf[vMap.offsetGBufC(xPointInfo>>kmGrid, yPointInfo>>kmGrid)]);
		statusBar.SetPaneText(0, str, 0);
		//высота
		convert_vox2vid(vMap.GetAlt(xPointInfo, yPointInfo), str1);
		itoa(vMap.GetApproxAlt(vMap.XCYCL(xPointInfo), vMap.YCYCL(yPointInfo)), str2, 10);
		itoa(round(environment->water()->GetZ(vMap.XCYCL(xPointInfo), vMap.YCYCL(yPointInfo))), str3, 10);
		sprintf(str, "Alt:%5s/%3s/%3s", str1,str2, str3);
		statusBar.SetPaneText(1, str, 0);
		//2 -mega операций на процедурной карте
		float mop=(double)vMap.procMapOp/1000000.;
		sprintf(str, "MOp:%8f", mop);
		statusBar.SetPaneText(2, str, 0);
		int posMOp=round((mop/MAX_MEGA_PROCEDUR_MAP_OPERATION)*100);
		if(posMOp>100) posMOp=100;
		//progressMOp.SetPos(posMOp);
		pMF->progressBar().SetPos(posMOp);

		///sprintf(str, "X:%4d/%4d", xPointInfo, xPointInfo>>kmGrid);
		sprintf(str, "X:%4d", xPointInfo);
		statusBar.SetPaneText(4, str, 0);
		//sprintf(str, "Y:%4d/%4d", yPointInfo, yPointInfo>>kmGrid);
		sprintf(str, "Y:%4d", yPointInfo);
		statusBar.SetPaneText(5, str, 0);
		//6- separator //Но пока используем
		sprintf(str, "MS:%9d", profileTimeOperation );
		statusBar.SetPaneText(6, str, 0);

		sprintf(str, "Sur:%4d", vMap.GetTer(xPointInfo, yPointInfo));
		statusBar.SetPaneText(7, str, 0);
		//sprintf(str, "Lght:%4d", vMap.RnrBuf[vMap.offsetBuf(xPointInfo, yPointInfo)] );
		//statusBar.SetPaneText(8, str, 0);

		{
			float fps=pMF->fps.GetFPS();
			float lfps=pMF->logicFps.GetFPS();
			sprintf(str, "FPS:%3.1f P:%i M:%i LFPS:%3.1f", fps,draw_num_polygon-draw_num_tilemappolygon,draw_num_tilemappolygon,lfps);
			statusBar.SetPaneText(9, str, 1);
		}

		//sprintf(str, "MS:%8d", profileTimeOperation );
//		sprintf(str, "GH:%4d", vMap.GVBuf[vMap.offsetGBuf(m_SurToolMoveVar.m_XCenterM>>kmGrid, m_SurToolMoveVar.m_YCenterM>>kmGrid)] );
//		statusBar.SetPaneText(5, str, 0);

	}
}

void CGeneralView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CSurToolBase* currentToolzer = getCurCtrl();

	if(nChar == VK_SCROLL)
        ConsoleWindow::instance().show(!ConsoleWindow::instance().isVisible());

	if(!cameraManager){
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
		return;
	}

	if(toolsWindow_->onKeyDown(nChar, nFlags)){
	}
    else if (currentToolzer && !currentToolzer->CallBack_KeyDown (nChar, false, false, false)) {
        const float WORLD_ANGLE_STEP = 15.f * (M_PI/180.f);
        const int WORLD_STEP = 40;

		bool updateCameraCoordinate = true;

        CameraCoordinate coord = cameraManager->coordinate();

        switch (nChar) {
        case VK_UP:
            coord.position().x -= WORLD_STEP * cos(coord.psi());
            coord.position().y -= WORLD_STEP * sin(coord.psi());
            break;
        case VK_DOWN:
            coord.position().x += WORLD_STEP * cos(coord.psi());
            coord.position().y += WORLD_STEP * sin(coord.psi());
            break;
        case VK_LEFT:
            coord.position().x -= WORLD_STEP * sin(coord.psi());
            coord.position().y += WORLD_STEP * cos(coord.psi());
            break;
        case VK_RIGHT:
            coord.position().x += WORLD_STEP * sin(coord.psi());
            coord.position().y -= WORLD_STEP * cos(coord.psi());
            break;
        case VK_HOME:
            coord.theta() -= WORLD_ANGLE_STEP;
            break;
        case VK_END:
            coord.theta() += WORLD_ANGLE_STEP;
            break;
		case VK_NUMPAD0:
			coord.position() = To3D(coord.position());
			break;
        case VK_DELETE:
            if (currentToolzer) {
                currentToolzer->CallBack_Delete();
            }
			updateCameraCoordinate = false;
            break;
        case VK_PRIOR:
            coord.psi() += WORLD_ANGLE_STEP;
            break;
        case VK_NEXT:
            coord.psi() -= WORLD_ANGLE_STEP;
            break;
		case VK_OEM_4: // [
			toolsWindow_->previousBrush();
			updateCameraCoordinate = false;
			break;
		case VK_OEM_6: // ]
			toolsWindow_->nextBrush();
			updateCameraCoordinate = false;
			break;
		default:
			updateCameraCoordinate = false;
        }

		if(updateCameraCoordinate)
			cameraManager->setCoordinate (coord);
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

namespace UniverseObjectActions{

struct CountSelected : public UniverseObjectAction{
	int& count_;

	CountSelected(int& count)
	: count_(count)
	{
		count_ = 0;
	}

	void operator()(BaseUniverseObject& object){
		UniverseObjectClass objectClass = object.objectClass();
		if(object.selected())
			++count_;
	}		
};

struct DeselectDead : public UniverseObjectAction{
	DeselectDead(bool& needUpdate)
	: needUpdate_(needUpdate)
	{}
	void operator()(BaseUniverseObject& object){
		UniverseObjectClass objectClass = object.objectClass();
		if(objectClass == UNIVERSE_OBJECT_UNIT || objectClass == UNIVERSE_OBJECT_ENVIRONMENT || objectClass == UNIVERSE_OBJECT_SOURCE){
			if(object.selected() && object.dead()){
				object.setSelected(false);
				needUpdate_ = true;
			}
		}
	}
	bool& needUpdate_;
};
};

void CGeneralView::quant()
{
	using namespace UniverseObjectActions;
	static int lastSelectedCount = 0;
	bool needUpdate = false;
	forEachSelected(DeselectDead(needUpdate), true);
	int selectedCount = 0;
	forEachSelected(CountSelected(selectedCount), true);
	if(selectedCount != lastSelectedCount)
		needUpdate = true;

	CMainFrame& mainFrame = ::mainFrame();
	if(needUpdate)
		mainFrame.eventObjectChanged().emit();
	if(CSurToolBase* currentTool = getCurCtrl())
		currentTool->quant();
	lastSelectedCount = selectedCount;
}

void drawLineTerrain(const Vect2f& point1, const Vect2f& point2, const sColor4c& color, float segmentLength)
{
	float len = point1.distance(point2);
	segmentLength = len / max(1.0f, float(round(len / segmentLength)));
	Vect2f dir = point2 - point1;
	dir.normalize(1.0f);
	if(segmentLength > FLT_EPS) {
		for(float pos = 0; pos <= len - segmentLength + FLT_COMPARE_TOLERANCE; pos += segmentLength) {
			gb_RenderDevice->DrawLine(To3D(point1 + dir * pos), To3D(point1 + dir * (pos + segmentLength)), color);
		}
	}
}

void CGeneralView::drawGrid()
{
	if(surMapOptions.enableGrid_) {
		int gridStep = max(16, surMapOptions.gridSpacing_);
		int segmentLength = max(8, min(16, gridStep / 4));
		const sColor4c& color = surMapOptions.gridColor_;
		float level = 0.0f;
		if(vMap.isWorldLoaded()) {
			for(int x = 0; x <= vMap.H_SIZE; x += gridStep) {
				drawLineTerrain(Vect2f(x, 0), Vect2f(x, vMap.V_SIZE - 1), color, segmentLength);
			}
			for(int y = 0; y <= vMap.V_SIZE; y += gridStep) {
				drawLineTerrain(Vect2f(0, y), Vect2f(vMap.H_SIZE - 1, y), color, segmentLength);
			}
		}
	}
}

BOOL CGeneralView::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
//	return CWnd::OnEraseBkgnd(pDC);
}

bool CGeneralView::isWorldOpen() const
{
	return vMap.isWorldLoaded();
}

void CGeneralView::updateSurface()
{
	if(vMap.isWorldLoaded()){
		vMap.recalcArea2Grid(0, 0, vMap.H_SIZE - 1, vMap.V_SIZE - 1);
		vMap.regRender(0, 0, vMap.H_SIZE-1, vMap.V_SIZE-1, vrtMap::TypeCh_Height|vrtMap::TypeCh_Texture|vrtMap::TypeCh_Region);
	}
}
