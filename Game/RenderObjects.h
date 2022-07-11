#ifndef __RENDER_OBJECTS_H__
#define __RENDER_OBJECTS_H__

#include "..\Render\inc\Umath.h"

extern class cScene* terScene;
extern class cTileMap* terMapPoint;
extern class cFont* pDefaultFont;
extern class CameraManager* cameraManager;

extern int terFullScreen;

void createRenderContext(bool multiThread);
bool initRenderObjects(int renderMode, HWND hwnd);
void finitRenderObjects();
void initScene();
void finitScene();
void restoreFocus();
void SetShadowType(int shadow_map,int shadow_size,bool update);
void setSilhouetteColors();

void attachSmart(class cBaseGraphObject* object);

void loadAllLibraries(bool preload = false);
void saveAllLibraries();

#endif //__RENDER_OBJECTS_H__
