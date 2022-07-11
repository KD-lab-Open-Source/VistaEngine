#ifndef __RENDER_OBJECTS_H__
#define __RENDER_OBJECTS_H__

template<typename scalar_type, class vect_type> struct Rect;
typedef Rect<float, Vect2f> Rectf;

extern int terFullScreen;
extern class cScene* terScene;
extern class CameraManager* cameraManager;

Rectf aspectedWorkArea(const Rectf& windowPosition, float aspect);
void setSilhouetteColors();

void loadAllLibraries();
void saveAllLibraries();

////////////////////////////////////////////////////////
// Устаревшие функции для совместимости с редактором
void createRenderContext(bool multiThread);
bool initRenderObjects(int renderMode, HWND hwnd);
void finitRenderObjects();
void initScene();
void finitScene();
void updateResolution(Vect2i size, bool change_size);
////////////////////////////////////////////////////////

#endif //__RENDER_OBJECTS_H__
