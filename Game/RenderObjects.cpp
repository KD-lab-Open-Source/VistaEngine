#include "stdafx.h"
#include "RenderObjects.h"
#include "terra.h"
#include "CameraManager.h"

#include "..\Game\Universe.h"
#include "..\Units\ExternalShow.h"
#include "..\Water\CircleManager.h"
#include "GameOptions.h"
#include "..\UserInterface\UI_Render.h"

cScene* terScene;
cTileMap* terMapPoint;
cFont* pDefaultFont;
CameraManager* cameraManager = NULL;

void setWindowPicture(const char* file);
void updateResolution(Vect2i size, bool change_size);

int terFullScreen = 0;

void SetShadowType(int shadow_map,int shadow_size,bool update)
{
	static int old_shadow_control=-1;
	// 0 - planar map
	// 1- shadow map
	// 2 - self shadow map;
	if(!gb_RenderDevice->IsEnableSelfShadow() && shadow_map==2)
		shadow_map=1;
	if(shadow_size==0)
	{
		if(shadow_map!=0)
			update=true;
		shadow_map=0;
	}

	switch(shadow_map)
	{
	case 1:
		gb_VisGeneric->SetShadowType(SHADOW_MAP,shadow_size);
		break;
	case 2:
		gb_VisGeneric->SetShadowType(SHADOW_MAP_SELF,shadow_size);
		break;
	default://planar map
		gb_VisGeneric->SetShadowType(SHADOW_NONE,shadow_size);
		break;
	}

	bool shadow_control=shadow_map!=2;
	vMap.ShadowControl(shadow_control);
	if(old_shadow_control>=0)
	{
		update=shadow_control!=(bool)old_shadow_control;
	}
	old_shadow_control=shadow_control;

	if(update) {
		//vMap.regRender(0,0,vMap.H_SIZE-1,vMap.V_SIZE-1);
		vMap.WorldRender();
	}
}

void setSilhouetteColors()
{
	int colorsCount = 0;
	if(universe()) {
		PlayerVect& players = universe()->Players;
		if(players.empty()) {
			gb_VisGeneric->SetSilhouetteColor(0, sColor4c(255, 255, 255, 255));
		} else {
			for(int i = 0; i < players.size(); ++i) {
				sColor4c color(255, 255, 255, 255);
				if(players[i]->silhouetteColorIndex() >= 0) 
					color = GlobalAttributes::instance().silhouetteColors.at(players[i]->silhouetteColorIndex());
				gb_VisGeneric->SetSilhouetteColor(i, color);
			}
		}
	}
}

void createRenderContext(bool multiThread)
{
	CreateIRenderDevice(multiThread);
	GameOptions::instance().filterBaseGraphOptions();
	UI_Render::instance().setInterfaces(gb_VisGeneric, gb_RenderDevice);
}

bool initRenderObjects(int renderMode, HWND hwnd)
{
	gb_VisGeneric->SetUseTextureCache(true);
	gb_VisGeneric->SetUseMeshCache(true);
	gb_VisGeneric->SetFavoriteLoadDDS(true);
	gb_VisGeneric->SetEffectLibraryPath("RESOURCE\\FX","RESOURCE\\FX\\TEXTURES");

	gb_VisGeneric->EnableOcclusion(true);
	gb_VisGeneric->SetWorkCacheDir("cacheData");
	gb_VisGeneric->SetBaseCacheDir("resource\\cacheData");

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

	setWindowPicture(GlobalAttributes::instance().startScreenPicture_.c_str());

	//---------------------
	pDefaultFont=gb_VisGeneric->CreateFont(default_font_name.c_str(),24);
	xassert(pDefaultFont);
	gb_RenderDevice->SetDefaultFont(pDefaultFont);

	setSilhouetteColors();

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
	AttributeBase::releaseModel();

	gb_RenderDevice->SetDefaultFont(NULL);
	gb_RenderDevice->SetFont(NULL);
	RELEASE(pDefaultFont);
	RELEASE(gb_RenderDevice);
}

void restoreFocus()
{ 
	SetFocus(gb_RenderDevice->GetWindowHandle()); 
}

void attachSmart(cBaseGraphObject* object)
{
	if(!MT_IS_GRAPH()){
		object->SetAttr(ATTRUNKOBJ_IGNORE);
		streamLogicCommand.set(fCommandSetIgnored, object) << false;
	}
	object->Attach();
}

void setWindowPicture(const char* file)
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