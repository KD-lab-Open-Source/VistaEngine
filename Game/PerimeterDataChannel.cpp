#include "StdAfx.h"
//#include "NetPlayer.h"
//#include "RenderObjects.h"
//#include "GameShell.h"

//extern float CAMERA_SCROLL_SPEED_DELTA,CAMERA_BORDER_SCROLL_SPEED_DELTA, CAMERA_MOUSE_ANGLE_SPEED_DELTA, CAMERA_KBD_ANGLE_SPEED_DELTA;

//extern int terCurrentServerIP;
//extern int terCurrentServerPort;
extern char* terCurrentServerName;

//extern float terMapLevelLOD;
//extern int terDrawMeshShadow;
//extern int terShadowType;
//extern int terShadowSamples;
//extern bool terTileMapTypeNormal;
//extern bool terTilemapDetail;
//
//extern int terMipMapLevel;
//extern int terShowTips;

//extern float terNearDistanceLOD;
//extern float terFarDistanceLOD;

//bool terEnableBumpChaos = true;

void PerimeterDataChannelLoad()
{
	IniManager ini("Game.ini");
	IniManager ini_no_check("Game.ini", false);

	//GraphicsSection
//	terFullScreen = ini.getInt("Graphics","FullScreen");
//	terScreenSizeX = ini.getInt("Graphics","ScreenSizeX");
//	terScreenSizeY = ini.getInt("Graphics","ScreenSizeY");
//	terMapLevelLOD = ini.getInt("Graphics","MapLevelLOD");/
//
//	terDrawMeshShadow = ini.getInt("Graphics","DrawMeshShadow");/
//	xassert(terDrawMeshShadow>=0 && terDrawMeshShadow<=3);
//	terShadowSamples=ini.getInt("Graphics","ShadowSamples");/
//	xassert(terShadowSamples==4 || terShadowSamples==16);
//
//	terShadowType = ini.getInt("Graphics","ShadowType");/
//	terEnableBumpChaos = ini.getInt("Graphics","EnableBumpChaos");/
//	terTileMapTypeNormal = ini.getInt("Graphics","TileMapTypeNormal");
//	terTilemapDetail =ini.getInt("Graphics","TilemapDetail");
//	gb_VisGeneric->SetFavoriteLoadDDS(ini.getInt("Graphics","FavoriteLoadDDS"));/
//
//	terNearDistanceLOD = ini.getInt("Graphics","NearDistanceLOD");
//	terFarDistanceLOD = ini.getInt("Graphics","FarDistanceLOD");
//
//	terMipMapLevel = ini.getInt("Graphics","MipMapLevel");
////	terMapReflection = ini.getInt("Graphics","MapReflection");
////	terObjectReflection = ini.getInt("Graphics","ObjectReflection");
//	terGraphicsGamma = ini.getFloat("Graphics","Gamma");

	//Network
	const char* s = ini_no_check.get("Network","ServerName");
	if(s){
		terCurrentServerName = new char[strlen(s) + 1];
		strcpy(terCurrentServerName,s);
	}
	else 
		terCurrentServerName = NULL;

	//CAMERA_SCROLL_SPEED_DELTA = CAMERA_BORDER_SCROLL_SPEED_DELTA = ini.getInt("Game","ScrollRate");
	//CAMERA_MOUSE_ANGLE_SPEED_DELTA = ini_no_check.getFloat("Game","MouseLookRate");
	//if (CAMERA_MOUSE_ANGLE_SPEED_DELTA == 0) {
	//	CAMERA_MOUSE_ANGLE_SPEED_DELTA = 3;
	//}
	//CAMERA_KBD_ANGLE_SPEED_DELTA = CAMERA_MOUSE_ANGLE_SPEED_DELTA / 30.0f;
}

void PerimeterDataChannelSave()
{
	IniManager ini("Game.ini");

	//GameSection
	//ini.putInt("Game", "ShowTips", terShowTips);
	//ini.putInt("Game", "ScrollRate", round(CAMERA_SCROLL_SPEED_DELTA));
	//ini.putFloat("Game", "MouseLookRate", round(CAMERA_MOUSE_ANGLE_SPEED_DELTA));
	
	//Network
	ini.put("Network","ServerName", terCurrentServerName);
}

