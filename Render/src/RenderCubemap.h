#ifndef __RENDER_CUBEMAP_H_INCLUDED__
#define __RENDER_CUBEMAP_H_INCLUDED__

#include "Render/inc/IRenderDevice.h"

struct IDirect3DSurface9;
class cScene;

class RENDER_API cRenderCubemap : public ManagedResource
{
public:
	cRenderCubemap();
	~cRenderCubemap();

	void Init(int linear_size,Vect3f camera_pos);
	void Animate(float dt);
	virtual void Draw();

	void Save(const char* file_name);

	cScene* GetSceneBefore(){return pSceneBefore;};
	cTexture* GetCubeMap(){return pTexture;};
	void SetFoneColor(Color4c color);
protected:
	virtual void deleteManagedResource();
	virtual void restoreManagedResource();

	enum
	{
		num_camera=6,
	};
	Camera* camera[num_camera];

	cScene* pSceneBefore;

	int linear_size;
	cTexture *pTexture;
	// Where a face is drawn before it is copied into pTexture's layer. D3D pointed each
	// camera straight at a cube face surface; the SDL backend has no per-pass layer
	// argument, so the faces go through one ordinary offscreen 2D target instead --
	// cSDLRenderDevice::copyToCubeFace explains why.
	cTexture *pFaceTarget;
	IDirect3DSurface9* pZBuffer;
	int cur_draw_phase;
	virtual void DrawOne(int i);
	void DrawFace(int i);
};

#endif
