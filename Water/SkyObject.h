#include "..\Render\src\RenderCubemap.h"
#include "..\Environment\EnvironmentColors.h"

class cEnvironmentTime;
class cSkyObj;
class cObject3dx;

class cSunMoonObj:public cBaseGraphObject
{
public:
	cSunMoonObj();
	~cSunMoonObj();

	const MatXf& GetPosition() const{ return position_; }
	void PreDraw(cCamera* camera){};
	void Draw(cCamera* camera);	

	void serialize(Archive& ar);
	void SetDay(bool day) {isDay = day;}
	void SetPosition(MatXf& pos){position_ = pos;}
	float Size() const{ return isDay ? sunSize : moonSize; }
protected:
	void SetTextures();
	MatXf position_;
	string SunName;
	string MoonName;
	cTexture* SunTexture;
	cTexture* MoonTexture;
	bool isDay;
	float sunSize;
	float moonSize;
	float scale;

};

class cSkyCamera:public cCamera
{
public:
	cSkyCamera(cScene* scene);
	virtual void DrawScene();

	void AddObject(cObject3dx*, bool write_alpha);
	void SetHDRAlpha(bool enable_hdr_alpha_){enable_hdr_alpha=enable_hdr_alpha_;};
	void SetSunMoonObj(cSunMoonObj* obj){sunMoonObj = obj;}
protected:
	struct OneObject
	{
		cObject3dx* obj;
		bool write_alpha;
	};
	vector<OneObject> objects;
	bool enable_hdr_alpha;
	cSunMoonObj* sunMoonObj;
};

struct SkyNameOld {
	string Name;
	void serialize(Archive& ar);
};
enum SkyElementType{
	SKY_ELEMENT_NIGHT,
	SKY_ELEMENT_DAY,
	SKY_ELEMENT_DAYNIGHT
};

struct SkyName
{
	SkyName()
	{
		type = SKY_ELEMENT_DAYNIGHT;
	}
	string Name;
	SkyElementType type;
	void serialize(Archive& ar);
};
struct SkyElement
{
	cObject3dx* object;
	int anim_group;
	float time;
	float phase_cloud;
	SkyElementType type;
};

class cFogCircleEX
{
	sPtrVertexBuffer cborder_vb;
	sPtrIndexBuffer cborder_ib;
	int size_vb, size_ib;
	typedef sVertexXYZD VType;
	sColor4c fog_color;
	cEnvironmentTime* time;
	enum 
	{
		hord_count = 36,
	};
	int height;
public:
	cFogCircleEX(cEnvironmentTime* time);
	~cFogCircleEX();
	void Draw(cCamera *pCamera);

	void SetHeight(int height);
};

class cRenderSky : public cRenderCubemap {
public:
	cRenderSky(cSkyObj* pSkyObj);
	//~cRenderSky();
protected:
	cSkyObj* pSkyObj;
	void DrawOne(int i);
};

class cSkyObj {
public:
	cSkyObj(cScene* pScene, cEnvironmentTime* pEnviromentTime);
	~cSkyObj();
	void DrawSkyAndAnimate(cCamera* pGlobalCamera);
	void DrawSky(cCamera* pGlobalCamera,bool hdr_alpha);

	void serialize(Archive& ar);
	void SetSkyModel();
	void SetDay(bool is_day);
	int GetSceneCount(){return sky_elements_count;};
	cScene* GetScene() { return pSkyScene; }

	void AddSkyModel(const char* sky_model_name);
	void SetFogCircle(bool need,cEnvironmentTime* pEnviromentTime);
	void setFogHeight(int height);
	const MatXf& GetSunPosition() {return sunMoonObj.GetPosition();}
	void SetSunPosition(MatXf& mat) {sunMoonObj.SetPosition(mat);}

	void SetReflectFoneColor(sColor4c color){reflect_fone_color=color;};
	float SunSize() const{ return sunMoonObj.Size(); }
protected:
	//void SetTextures();
	void DrawSun(cCamera* pGlobalCamera);
	vector<SkyElement> sky_elements;
	cEnvironmentTime* time;
	cScene* pWorldScene;
	cScene* pSkyScene;
	cSkyCamera* pNormalCamera;//Камера
	cFogCircleEX* pFogCircle;
	float CalcNormalScale();//Расчет увеличения нормалей?
	//cScene* pSkyScene;
	int sky_elements_count;
	bool cur_is_day;
	//SkyName sun_name;
	Vect3f sunPosition;
	sColor4c reflect_fone_color;
	vector<SkyName> sky_elements_names;
	cSunMoonObj sunMoonObj;
};

struct ShadowingOptions {
	ShadowingOptions ();
	ShadowingOptions (float _user_ambient_factor, float _user_ambient_maximal, float _user_diffuse_factor);
	inline bool operator==(const ShadowingOptions& rhs) const {
		const float small_value = 0.0001f;
		return (abs (user_ambient_factor - rhs.user_ambient_factor) < small_value &&
				abs (user_ambient_maximal - rhs.user_ambient_maximal) < small_value &&
				abs (user_diffuse_factor - rhs.user_diffuse_factor) < small_value);
	}
	void serialize (Archive& ar);

	float user_ambient_factor;
	float user_ambient_maximal;
	float user_diffuse_factor;
};

class cEnvironmentTime:public EnvironmentTimeColors
{
public:
	cEnvironmentTime(cScene* pScene);
	~cEnvironmentTime();
	void Init();

	void Draw();//Рисовать cubemap
	void DrawEnviroment(cCamera* pGlobalCamera);//Рисовать на основной rendertarget

	void Save();
	cTexture* GetCubeMap();

	void SetTime(float time, bool init = false);//В часах
	float GetTime(){return day_time;}
	float GetLightAngle();
	sColor4c GetCurSunColor(){return cur_sun_color;};
	sColor4c GetCurFoneColor(){return cur_fone_color;};
	sColor4c GetCurFogColor();
	void setEnableChangeReflectSkyColor(bool enable) { enableChangeReflectSkyColor = enable; } 
	sColor4c GetCurReflectSkyColor(){return cur_reflect_sky_color;}
	void SetCurReflectSkyColor(sColor4c color){cur_reflect_sky_color=sColor4f(color);}
	bool IsDay() { return is_day; }
	bool DayChanged() { return is_day != prev_is_day; }
	bool CheckIsDay();
	void serialize(Archive& ar);
	void serializeColors(Archive& ar);
	void serializeParameters(Archive& ar);
	class cSkyObj*	pSkyObj;//Temp Added by @!!ex
	void SetShadowIntensity(float f){shadow_intensity=f;}
	void SetFogCircle(bool need);
	void setFogHeight(int height);
	const Vect3f& sunPosition() const;
	float sunSize() const;

	float GetShadowFactor()const{return shadow_factor;}

	void ExportSunParameters(const char* file_name);

protected:
	cScene* pScene;
	class cRenderSky* pCubeRender;
	cFogOfWar* pFogOfWar;
	float day_time;
	float time_angle;

	bool is_day;

	float shadow_intensity;
	ShadowingOptions shadowing;

	float time_shadow_off;
	float speed_shadow_off;

	sColor4c cur_sun_color;
	sColor4c cur_fone_color;
	sColor4c current_fog_color;
	sColor4c cur_reflect_sky_color;
	bool prev_is_day;

	float phase_cloud;
	int iag_clouds;
	float sun_radius;
	float slant_angle;

	float shadow_factor;
	float latitude_angle;
	float CalcNormalScale();

	bool enableChangeReflectSkyColor;
};
