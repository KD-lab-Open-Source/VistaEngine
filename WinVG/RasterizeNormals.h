#pragma once

class RasterizeNormals
{
public:
	RasterizeNormals();
	~RasterizeNormals();
	void Init(int dx,int dy,cObject3dx* obj, int nmaterial);
	void GetMap(vector<Vect3f>& nmap);//dx*dy
protected:
	cTexture* pRenderTarget;
	cTexture* pDestTexture;
	int dx,dy;
	cTexture* CreateRenderTexture(int width,int height,bool render);
	void Render(cObject3dx* obj, int nmaterial);
};
