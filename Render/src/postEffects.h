#ifndef __POST_EFFECTS_H_INCLUDED__
#define __POST_EFFECTS_H_INCLUDED__
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
class cCamera;
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
	PE_END,
};

class PostEffectManager
{
public:
	PostEffectManager();
	~PostEffectManager();

	void Init(int type = -1);
	void Draw(float dt=0);
	PostEffect* GetEffect(PostEffectType type);
	bool ChangeSize();
	bool IsActive(PostEffectType type);
	void SetActive(PostEffectType type, bool active);
	cTexture* GetBackBufferTexture();
	int GetTexturesSize(PostEffectType type);
	int GetBaseTexturesSize();
protected:
	void CreateTextures();
	PostEffect* effects[PE_END];
	bool isPS20_;
	cTexture* backBufferTexture;
	bool enable_;
	int width_;
	int height_;
};

class PostEffect
{
public:
	PostEffect(PostEffectManager* manager) {
		manager_ = manager;
		isActive_ = false; 
		width_ = height_ =0;
		enable_ = false;
	}
	virtual ~PostEffect() {}
	virtual void Init();
	virtual void Draw(float dt);
	virtual void DrawEffect(float dt){};
	virtual void SetActive(bool active=true) 
	{
		isActive_ = active;
	};
	virtual bool IsActive() {return isActive_;}
	static void DrawQuad(float x1, float y1, float dx, float dy, float u1, float v1, float du, float dv, sColor4c color = sColor4c(255,255,255,255));
	virtual int GetTexturesSize() {return 0;}
	virtual void CreateTextures() = 0;
	void SetSize(int width,int height);
protected:
	virtual void SetSamplerState(){};
	void SetRenderState();
	void RestoreRenderState();
	bool isActive_;
	bool enable_;
	DWORD oldZ_;
	DWORD oldAlpha_;
	DWORD oldAlphaTest_;
	DWORD width_;
	DWORD height_;
	PostEffectManager* manager_;
};

class ColorDodgeEffect : public PostEffect
{
public:
	ColorDodgeEffect(PostEffectManager* manager);
	~ColorDodgeEffect();
	void Init();
	void DrawEffect(float dt);
	int GetTexturesSize(){return 0;};
	void SetSpeed(float s) {speed = s;}
	void SetFadeSpeed(float s){fadeSpeed_ = s;}
	float GetSpeed(){return speed;}
	void SetActive(bool switchOn){process = true; fadeIn=switchOn;}
	void SetTexture(const char* name);
protected:
	void CreateTextures();
	void SetSamplerState();
	float speed;
	float fadeSpeed_;
	Vect2f center;
	float size;
	float phase_;
	bool process;
	bool fadeIn;
	string textureName;
	PSColorDodge* psShader;
	cTexture* texture1;
	cTexture* texture2;
};

class MonochromeEffect : public PostEffect
{
public:
	MonochromeEffect(PostEffectManager* manager);
	~MonochromeEffect();
	void Init();
	void DrawEffect(float dt);
	void SetPhase(float phase) {phase_ = phase;}
	void SetActive(bool switchOn){process = true; fadeIn=switchOn;}
	int GetTexturesSize();
protected:
	void CreateTextures();
	void SetSamplerState();
	PSMonochrome* psShader;
	float speed;
	float phase_;
	int timePrev_;
	bool process;
	bool fadeIn;
};

class BloomEffect : public PostEffect
{
public:
	BloomEffect(PostEffectManager* manager);
	~BloomEffect();
	void Init();
	void SetLuminace(float luminance) {luminance_ = luminance;};
	void SetColor(sColor4f color){addColor = color;};
	void SetBloomScale(float bloomScale) {bloomScale_ = bloomScale;};
	void SetDefaultLuminance(float luminance) {defaultLuminance_ = luminance;}
	void RestoreDefaults();
	float GetDefaultLuminance() {return defaultLuminance_;}
	void SetExplode(float enable);
	void DrawEffect(float dt);
	int GetTexturesSize();
protected:
	void CreateTextures();
	void SetSamplerState();
	PSColorBright* psColorBright;
	PSCombine* psCombine;
	PSBloomHorizontal* psBloomH;
	PSBloomVertical* psBloomV;
	PSMonochrome* psMonochrome;
	PSDown4* psDown4;

	cTexture* texture2;
	cTexture* texture3;
	cTexture* tempTexture;
	float luminance_;
	float defaultLuminance_;
	float bloomScale_;
	bool exlpodeEnable_;
	sColor4f addColor;
};

class UnderWaterEffect : public PostEffect
{
public:
	UnderWaterEffect(PostEffectManager* manager);
	~UnderWaterEffect();
	void Init();
	void DrawEffect(float dt);
	void SetColor(sColor4f color) {color_ = color;}
	void SetEnvironmentFog(Vect2f &fog) {environmentFog = fog;};
	void SetFog(sColor4f& fogColor);
	void SetFogParameters(Vect2f& fogPlanes_){fogPlanes = fogPlanes_;}
	void SetTexture(string &name);
	bool IsActive() {return (activeAlways_||underWater_)&&isActive_;}
	void SetWaveSpeed(float speed){waveSpeed_ = speed;}
	void SetActiveAlways(bool active){activeAlways_ = active;}

	bool isUnderWater() const {return underWater_;}
	int GetTexturesSize();
	void SetWater(cWater* water){pWater = water;}
	void SetIsUnderWater(bool under_water);
protected:
	void CreateTextures();
	void SetSamplerState();
	PSBloomHorizontal* psBlurH;
	PSBloomVertical* psBlurV;
	PSUnderWater* psUnderWater;
	PSMonochrome* psMonochrome;
	cTexture* waveTexture;
	cTexture* workTexture;
	sColor4f color_;
	float shift_;
	float scale_;
	float delta_;
	cWater* pWater;
	bool underWater_;
	bool activeAlways_;
	Vect2f environmentFog;
	sColor4f fogColor;
	Vect2f fogPlanes;
	float waveSpeed_;
	bool activateUnderWater;
};
class DOFEffect : public PostEffect
{
public:
	DOFEffect(PostEffectManager* manager);
	~DOFEffect();
	void Init();
	void DrawEffect(float dt);
	void SetDofParams(Vect2f &params);
	void SetDofPower(float power);
	int GetTexturesSize();
protected:
	void CreateTextures();
	//cTexture* texture2;
	//cTexture* texture3;
	cTexture* tempTexture;
	//PSBloomHorizontal* psBloomH;
	//PSBloomVertical* psBloomV;
	//PSDown4* psDown4;
	PSDOFCombine* psDOFCombine;
	PSBlurMap* psBlur;
	Vect2f dofParams;
	float dofPower;
};

class MirageEffect : public PostEffect
{
public:
	MirageEffect(PostEffectManager* manager);
	~MirageEffect();
	void Init();
	void DrawEffect(float dt);
protected:
	void CreateTextures();
	void SetSamplerState();
	int GetTexturesSize();
	cTexture* waveTexture;
	PSMirage* psMirage;
	PSFillColor* psFillColor;
};
#endif
