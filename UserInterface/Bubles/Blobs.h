#ifndef _BLOBS_H_
#define _BLOBS_H_

#include "Serialization\Serialization.h"

class cTexture;

struct cBlobsSetting
{
	Color4f color_;
	Color4f specularColor_;

	cBlobsSetting():specularColor_(0.6875f,1.f,0.898f,1.0f), color_(0.0f, 0.0f, 0.0f, 0.7f) {}

	void serialize(Archive& ar) {
		ar.serialize(color_, "color", "Цвет");
		ar.serialize(specularColor_, "specularColor", "Цвет блика");
	}

};

class cBlobs
{
	enum { num_planes = 1 };
public:
	cBlobs();
	~cBlobs();

	void Init(int width,int height);

	void BeginDraw();
	void Draw(int x,int y,int plane, float phase);
	void EndDraw(const cBlobsSetting& setting);

	cTexture* GetTarget(){return pRenderTarget;};

	void DrawBlobsSimply(int x,int y);
	void DrawBlobsShader(int x,int y, float phase, cTexture* texture, const cBlobsSetting& setting);
	//void SetTexture(const char* name);
	void SetTexture(cTexture* texture);

	void CreateBlobsTexture(int size);
protected:
	cTexture* pRenderTarget;
	cTexture* planeTextures[num_planes];
	cTexture *Texture;

	vector<Vect3f> points;
	class PSBlobsShader* pBlobsShader;
};

#endif _BLOBS_H_