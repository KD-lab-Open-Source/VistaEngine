#ifndef __RENDER_CUBEMAP_H_INCLUDED__
#define __RENDER_CUBEMAP_H_INCLUDED__

#include "Render\inc\IRenderDevice.h"

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
	IDirect3DSurface9* pZBuffer;
	int cur_draw_phase;
	virtual void DrawOne(int i);
};

#endif
