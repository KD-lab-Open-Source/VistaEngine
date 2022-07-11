#include "stdafx.h"
#include "XTL\Rect.h"
#include "CameraManager.h"
#include "GameOptions.h"
#include "UserInterface\UI_Render.h"
#include "Terra\terra.h"
#include "Universe.h"
#include "Game\IniFile.h"
#include "Render\src\FT_Font.h"
#include "Render\src\Scene.h"
#include "Render\src\VisGeneric.h"
#include "Terra\vMap.h"

int terFullScreen = 0;

cScene* terScene;
FT::Font* pDefaultFont;
CameraManager* cameraManager;

Rectf aspectedWorkArea(const Rectf& windowPosition, float aspect)
{
	Rectf aspectWindowPosition;
	if(float(windowPosition.width()) / float(windowPosition.height()) < aspect){
		float height = windowPosition.width() / aspect;
		aspectWindowPosition.set(windowPosition.left(),
								  windowPosition.top() + (windowPosition.height() - height) * 0.5f,
								  windowPosition.width(),
								  height);
	}
	else{
		float width = windowPosition.height() * aspect;
		aspectWindowPosition.set(windowPosition.left() + (windowPosition.width() - width) * 0.5f,
								  windowPosition.top(),
								  width,
								  windowPosition.height());
	}
	return aspectWindowPosition;
}

void setSilhouetteColors()
{
	int colorsCount = 0;
	if(universe()) {
		PlayerVect& players = universe()->Players;
		if(players.empty()) {
			gb_VisGeneric->SetSilhouetteColor(0, Color4c(255, 255, 255, 255));
		} else {
			for(int i = 0; i < players.size(); ++i) {
				Color4c color(255, 255, 255, 255);
				if(players[i]->silhouetteColorIndex() >= 0) 
					color = GlobalAttributes::instance().silhouetteColors.at(players[i]->silhouetteColorIndex());
				gb_VisGeneric->SetSilhouetteColor(i, color);
			}
		}
	}
}

////////////////////////////////////////////////////////
// Устаревшие функции для совместимости с редактором
////////////////////////////////////////////////////////
void createRenderContext(bool multiThread)
{
	CreateIRenderDevice(multiThread);
	GameOptions::instance().filterBaseGraphOptions();
}

bool initRenderObjects(int renderMode, HWND hwnd)
{
	gb_VisGeneric->SetUseTextureCache(true);
	gb_VisGeneric->SetUseMeshCache(true);
	gb_VisGeneric->SetFavoriteLoadDDS(true);
	gb_VisGeneric->SetEffectLibraryPath("RESOURCE\\FX","RESOURCE\\FX\\TEXTURES");

	gb_VisGeneric->EnableOcclusion(true);

	if(GlobalAttributes::instance().enableSilhouettes)
		renderMode |= RENDERDEVICE_MODE_STENCIL;

	gb_RenderDevice->SetMultisample(GameOptions::instance().getInt(OPTION_ANTIALIAS));

	if(!gb_RenderDevice->Initialize(GameOptions::instance().getScreenSize().x, GameOptions::instance().getScreenSize().y, renderMode, hwnd, 0))
	{
		gb_RenderDevice->SetMultisample(0);
		SetWindowPos(hwnd,NULL,0,0,800,600,SWP_SHOWWINDOW| SWP_FRAMECHANGED);
		if(!gb_RenderDevice->Initialize(800,600,renderMode,hwnd, 0))
			return false;
	}

	GameOptions::instance().filterGraphOptions();

//	gb_VisGeneric->SetRestrictionLOD(GameOptions::instance().getInt(OPTION_TEXTURE_DETAIL_LEVEL) == 0 ? 1 : 0);

	GameOptions::instance().graphSetup();

	UI_Render::instance().init();
	//setWindowPicture(GlobalAttributes::instance().startScreenPicture_.c_str());

	//---------------------
	pDefaultFont = FT::fontManager().createFont(default_font_name.c_str(), round(18.f / 768.f * float(gb_RenderDevice->GetSizeY())));
	xassert(pDefaultFont);
	gb_RenderDevice->SetDefaultFont(pDefaultFont);

	//setSilhouetteColors();

//	gb_VisGeneric->SetLodDistance(GlobalAttributes::instance().lod12, GlobalAttributes::instance().lod23);

//For AVI
/*	if(terWinVideoEnable){
		gb_RenderDevice->Fill(0,0,0);
		gb_RenderDevice->Flush();
	};*/

	return true;
}

void initScene()
{
	terScene = gb_VisGeneric->CreateScene();
	Vect3f dir(-234,0,-180);
	dir.normalize();
	terScene->SetSunDirection(dir);

	cameraManager = new CameraManager(terScene->CreateCamera());
}

void finitScene()
{

	if(cameraManager){
		delete cameraManager;
		cameraManager = NULL;
	}

	RELEASE(terScene);
}

void finitRenderObjects()
{
	//AttributeBase::releaseModel();

	gb_RenderDevice->SetDefaultFont(NULL);
	gb_RenderDevice->SetFont(NULL);
	FT::fontManager().releaseFont(pDefaultFont);
	RELEASE(gb_RenderDevice);
}

////////////////////////////////////////////////////////
