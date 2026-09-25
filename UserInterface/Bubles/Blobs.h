#ifndef _BLOBS_H_
#define _BLOBS_H_

#include "Serialization/Serialization.h"

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

// The metaball field of the KD-lab logo splash. Ported to SDL GPU: the two draws below
// record into SDLBlobsRenderer, which owns the field target and both passes. What used to
// be here -- the field render target, PSBlobsShader, the unused per-plane textures and the
// DrawBlobsSimply debug view of the target -- went with the D3D9 backend.
class cBlobs
{
public:
	cBlobs();
	~cBlobs();

	void Init(int width,int height);

	// Route the frame into the scene-capture target so the composite can sample it. Must
	// run before anything draws -- i.e. right after BeginScene, not with the cells.
	void BeginFrame();

	void BeginDraw();
	void Draw(int x,int y,int plane, float phase);
	void EndDraw();

	// The composite: the frame refracted and tinted through the field. `phase` is the
	// splash's fade in/out.
	void DrawBlobsShader(float phase, const cBlobsSetting& setting);

	void CreateBlobsTexture(int size);
protected:
	// The cell footprint, built on the CPU by CreateBlobsTexture.
	cTexture *Texture;

	vector<Vect3f> points;
};

#endif _BLOBS_H_