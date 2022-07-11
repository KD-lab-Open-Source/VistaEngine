#ifndef __POST_EFFECTS_H_INCLUDED__
#define __POST_EFFECTS_H_INCLUDED__

#include "Render\Inc\rd.h"
#include "Render\Src\Texture.h"

class PSMonochrome;
class PSCombine;
class PSBloomVertical;
class PSBloomHorizontal;
class PSColorBright;
class PSUnderWater;
class PSDown4;
class PSFiledDistort;
class PSFillColor;
class PSMirage;
class PSColorDodge;
class cWater;
class Camera;
class PSDOFCombine;
class PostEffect;
class PSBlurMap;

enum PostEffectType
{
	PE_MIRAGE=0,
	PE_DOF,
	PE_BLOOM,
	PE_UNDER_WATER,
	PE_COLOR_DODGE,
	PE_MONOCHROME,

	PE_EFFECT_NUM
};

enum PostEffectTextureID
{
	PE_TEXTURE_BACKBUFFER,
	PE_TEXTURE_FULL_SCREEN,

	PE_COLOR_DODGE_TEXTURE2,

	PE_BLOOM_TEXTURE2,
	PE_BLOOM_TEXTURE3,
	PE_BLOOM_TEXTURE_TEMP,

	PE_UNDER_WATER_TEXTURE_WAVE,

	PE_TEXTURE_NUM
};

class RENDER_API PostEffectManager
{
public:
	PostEffectManager();
	~PostEffectManager();

	void init(PostEffectType type = PE_EFFECT_NUM);
	void draw(float dt=0);
	PostEffect* getEffect(PostEffectType type);
	bool updateSize();
	bool isActive(PostEffectType type) const;
	void setActive(PostEffectType type, bool active);
	bool isEnabled(PostEffectType type) const { return (enabledEffects_ & (1 << type)) != 0; }

	cTexture* backBufferTexture();
	int texturesSize(PostEffectType type) const;
	int baseTexturesSize() const;

	cTexture* createTexture(PostEffectTextureID id, const char* texture_name, bool no_release = false);
	cTexture* createTexture(PostEffectTextureID id, int width, int height, bool no_release = false);
	cTexture* getTexture(PostEffectTextureID id) const { xassert(id >= 0 && id < PE_TEXTURE_NUM); return textures_[id]; }
	int textureSize(PostEffectTextureID id) const { if(cTexture* texture = getTexture(id)) return texture->CalcTextureSize(); else return 0; }

protected:

	void createTextures();

	PostEffect* effects_[PE_EFFECT_NUM];
	cTexture* textures_[PE_TEXTURE_NUM];

	bool isPS20_;
	bool isEnabled_;

	int width_;
	int height_;

	int enabledEffects_;

	bool createEffect(PostEffectType type);
};

class PostEffect
{
public:
	PostEffect(PostEffectManager* manager) : manager_(manager)
	{
		isActive_ = false; 
		width_ = height_ = 0;
		isEnabled_ = false;
	}

	virtual ~PostEffect() {}
	virtual void init() = 0;
	virtual void redraw(float dt) = 0;

	virtual void setActive(bool active = true){ isActive_ = active; }
	virtual bool isActive() const { return isActive_; }
	bool isEnabled() const { return isEnabled_; }

	virtual int texturesSize() const { return 0; }
	virtual void createTextures() = 0;

	void setSize(int width, int height){ width_ = width; height_ = height; }

protected:

	virtual void setSamplerState(){ }

	bool isActive_;
	bool isEnabled_;

	int width_;
	int height_;

	PostEffectManager* manager_;
};

class PostEffectColorDodge : public PostEffect
{
public:
	PostEffectColorDodge(PostEffectManager* manager);
	~PostEffectColorDodge();

	void init();
	void redraw(float dt);

	void setFadeSpeed(float s){ fadeSpeed_ = s; }

	void setSpeed(float s){ speed_ = s; }
	float speed() const { return speed_; }

	void setActive(bool switchOn){ process_ = true; fadeIn_ = switchOn; }
	void setTexture(const char* name);

	int texturesSize() const;

protected:

	float speed_;
	float fadeSpeed_;
	Vect2f center_;
	float size_;
	float phase_;
	bool process_;
	bool fadeIn_;

	string textureName_;
	PSColorDodge* shader_;

	void createTextures();
	void setSamplerState();
};

class PostEffectMonochrome : public PostEffect
{
public:
	PostEffectMonochrome(PostEffectManager* manager);
	~PostEffectMonochrome();

	void init();
	void redraw(float dt);
	void setActive(bool switchOn){process_ = true; fadeIn_=switchOn;}

protected:

	PSMonochrome* shader_;

	float speed_;
	float phase_;
	int timePrev_;
	bool process_;
	bool fadeIn_;

	void createTextures(){ };
	void setSamplerState();
};

class RENDER_API PostEffectBloom : public PostEffect
{
public:
	PostEffectBloom(PostEffectManager* manager);
	~PostEffectBloom();

	void init();
	void SetLuminace(float luminance){ luminance_ = luminance; }
	void SetColor(Color4f color){ addColor = color; }
	void SetBloomScale(float bloomScale){ bloomScale_ = bloomScale; }
	void SetDefaultLuminance(float luminance){ defaultLuminance_ = luminance; }
	void RestoreDefaults();
	float defaultLuminance() const { return defaultLuminance_; }
	void SetExplode(float enable);
	void redraw(float dt);

	int texturesSize() const;

protected:

	PSColorBright* psColorBright;
	PSCombine* psCombine;
	PSBloomHorizontal* psBloomH;
	PSBloomVertical* psBloomV;
	PSMonochrome* psMonochrome_;
	PSDown4* psDown4;

	float luminance_;
	float defaultLuminance_;
	float bloomScale_;
	bool exlpodeEnable_;
	Color4f addColor;

	void createTextures();
	void setSamplerState();
};

class RENDER_API PostEffectUnderWater : public PostEffect
{
public:
	PostEffectUnderWater(PostEffectManager* manager);
	~PostEffectUnderWater();

	void init();
	void redraw(float dt);
	void setColor(Color4f color){ color_ = color; }
	void setEnvironmentFog(Vect2f &fog){ environmentFog_ = fog; }
	void setFog(Color4f& fog_color);
	void setFogParameters(Vect2f& fog_planes){ fogPlanes_ = fog_planes; }
	void setTexture(const char* name);
	bool isActive() const { return (activeAlways_ || underWater_) && isActive_; }
	void setWaveSpeed(float speed){ waveSpeed_ = speed; }
	void setActiveAlways(bool active){ activeAlways_ = active; }

	bool isUnderWater() const {return underWater_; }
	int texturesSize() const;
	void setUnderWater(bool under_water);

protected:

	PSUnderWater* psUnderWater_;
	Color4f color_;
	float shift_;
	float scale_;
	float delta_;

	bool underWater_;
	bool activeAlways_;
	Vect2f environmentFog_;
	Vect2f fogPlanes_;
	float waveSpeed_;
	bool activateUnderWater_;

	void createTextures();
	void setSamplerState();
};

class RENDER_API PostEffectDOF : public PostEffect
{
public:
	PostEffectDOF(PostEffectManager* manager);
	~PostEffectDOF();

	void init();
	void redraw(float dt);
	void setDofParams(Vect2f &params);
	void setDofPower(float power);
	int texturesSize() const;

protected:

	PSDOFCombine* psDOFCombine_;
	PSBlurMap* psBlur_;
	Vect2f dofParams_;
	float dofPower_;

	void createTextures();
};

class PostEffectMirage : public PostEffect
{
public:
	PostEffectMirage(PostEffectManager* manager);
	~PostEffectMirage();

	void init();
	void redraw(float dt);

protected:

	PSMirage* psMirage_;

	void createTextures();
	void setSamplerState();
	int texturesSize() const;
};

#endif
