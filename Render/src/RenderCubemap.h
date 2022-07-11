#ifndef __RENDER_CUBEMAP_H_INCLUDED__
#define __RENDER_CUBEMAP_H_INCLUDED__

#include "..\Render\inc\IRenderDevice.h"
struct IDirect3DSurface9;
class cRenderCubemap:public cDeleteDefaultResource
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
	void SetFoneColor(sColor4c color);
protected:
	virtual void DeleteDefaultResource();
	virtual void RestoreDefaultResource();

	enum
	{
		num_camera=6,
	};
	cCamera* pCamera[num_camera];

	cScene* pSceneBefore;

	int linear_size;
	cTexture *pTexture;
	IDirect3DSurface9* pZBuffer;
	int cur_draw_phase;
	virtual void DrawOne(int i);
};

bool Load1DTexture(const char* fname,vector<sColor4c>& colors);

#endif
