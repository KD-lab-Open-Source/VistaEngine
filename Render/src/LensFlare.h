#ifndef __LENS_FLARE_H_INCLUDED__
#define __LENS_FLARE_H_INCLUDED__

#include "XMath\xmath.h"
#include "XMath\Colors.h"
#include "Render\Inc\IVisGenericInternal.h"

class Archive;
class cTexture;
class cObject3dx;
class cOcclusionQuery;

class LensFlareSprite{
public:
    LensFlareSprite(float _position = 0.0f, float _radius = 0.1f, const char* _filename = "", const Color4f& _color = Color4f(1.0f, 1.0f, 1.0f, 1.0f))
    : position_(_position)
    , radius_(_radius)
	, filename_(_filename)
    , texture_(0)
    , additiveBlending_(true)
	, color_(_color)
    {}
	~LensFlareSprite();

    void serialize(Archive& ar);
	void init();

	float radius() const{ return radius_; }
	float position() const{ return position_; }
	const Color4f& color() const{ return color_; }
	cTexture* texture() const;
	bool additiveBlending() const{ return additiveBlending_; }
private:
    float position_;
    float radius_;
    Color4f color_;
    bool additiveBlending_;
    std::string filename_;
    cTexture* texture_;
};

class LensFlare : public std::vector<LensFlareSprite>{
public:
    LensFlare();

    bool serialize(Archive& ar, const char* name, const char* nameAlt);
};

class RENDER_API LensFlareRenderer : public BaseGraphObject{
public:
    LensFlareRenderer();
	virtual ~LensFlareRenderer();
	
	void init();
	void serialize(Archive& ar);

	// virtual
	const MatXf& GetPosition() const{ return position_; }
	void PreDraw(Camera* camera);
	void Draw(Camera* camera);	
	// ^^^^

	void setFlareSource(const Vect3f& position, float radius);
	void setVisible(bool show);

	void setCameraClip(float clipMin, float clipMax){
		cameraClipMin_ = clipMin;
		cameraClipMax_ = clipMax;
	}

	LensFlare lensFlare_;
private:
	void drawFlare2D(const Vect2f& screenPoint, const LensFlare& flare, float alpha);
	void drawSprite(Camera* camera, const Vect3f& point, const LensFlareSprite& sprite, float alpha);

	bool isEnabled_;
	bool isVisible_;
	bool showGlowSprite_;

	MatXf position_;
	Vect3f sourcePosition_;
	float sourceRadius_;
	float cameraClipMin_;
	float cameraClipMax_;
	float opacity_;
	vector<Vect3f> points_;

	cOcclusionQuery& occlusionQuery_;

	LensFlareSprite glowSprite_;
};

#endif
