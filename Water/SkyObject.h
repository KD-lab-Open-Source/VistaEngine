#pragma once

#include "Render\src\RenderCubemap.h"
#include "Environment\EnvironmentColors.h"
#include "Render\Src\cCamera.h"

class EnvironmentTime;
class cSkyObj;
class cObject3dx;
class FogOfWar;

class cSunMoonObj : public BaseGraphObject
{
public:
	cSunMoonObj();
	~cSunMoonObj();

	const MatXf& GetPosition() const{ return position_; }
	void PreDraw(Camera* camera){};
	void Draw(Camera* camera);	

	void serialize(Archive& ar);
	void setAttribute(const SunMoonAttribute& attribute);
	void SetDay(bool day) {isDay = day;}
	void SetPosition(MatXf& pos){position_ = pos;}
	float Size() const{ return isDay ? attribute_.sunSize : attribute_.moonSize; }
protected:
	void SetTextures();
	MatXf position_;
	cTexture* SunTexture;
	cTexture* MoonTexture;
	bool isDay;
	float scale;
	SunMoonAttribute attribute_;
};

class cSkyCamera : public Camera
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
	Color4c fog_color;
	EnvironmentTime* time;
	enum 
	{
		hord_count = 36,
	};
	int height;
public:
	cFogCircleEX(EnvironmentTime* time);
	~cFogCircleEX();
	void Draw(Camera* camera);

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
	cSkyObj(cScene* pScene, EnvironmentTime* pEnviromentTime);
	~cSkyObj();
	void DrawSkyAndAnimate(Camera* pGlobalCamera);
	void DrawSky(Camera* pGlobalCamera,bool hdr_alpha);

	void serialize(Archive& ar);
	void SetSkyModel();
	void SetDay(bool is_day);
	int GetSceneCount(){return sky_elements_count;};
	cScene* scene() { return pSkyScene; }

	void AddSkyModel(const char* sky_model_name);
	void SetFogCircle(bool need,EnvironmentTime* pEnviromentTime);
	void setFogHeight(int height);
	const MatXf& GetSunPosition() {return sunMoonObj.GetPosition();}
	void SetSunPosition(MatXf& mat) {sunMoonObj.SetPosition(mat);}

	void SetReflectFoneColor(Color4c color){reflect_fone_color=color;};
	float SunSize() const{ return sunMoonObj.Size(); }

	void setAttribute(const SkyObjAttribute& attribute);
	void setSunMoonAttribute(const SunMoonAttribute& attribute);
protected:
	//void SetTextures();
	void DrawSun(Camera* pGlobalCamera);
	vector<SkyElement> sky_elements;
	EnvironmentTime* time;
	cScene* pWorldScene;
	cScene* pSkyScene;
	cSkyCamera* pNormalCamera;//Камера
	cFogCircleEX* pFogCircle;
	float CalcNormalScale();//Расчет увеличения нормалей?
	int sky_elements_count;
	bool cur_is_day;
	Vect3f sunPosition;
	Color4c reflect_fone_color;
	cSunMoonObj sunMoonObj;
	SkyObjAttribute attribute_;
};

class EnvironmentTime : public EnvironmentTimeColors
{
public:
	EnvironmentTime(cScene* pScene);
	~EnvironmentTime();

	void logicQuant();
	void Draw();//Рисовать cubemap
	void DrawEnviroment(Camera* pGlobalCamera);//Рисовать на основной rendertarget

	void Save();
	cTexture* GetCubeMap();
	cSkyObj* skyObj(){ return skyObj_; }

	void setTimeScale(float dayTimeScale, float nightTimeScale);
	void SetTime(float time, bool init = false);//В часах
	float GetTime(){return day_time;}
	float GetLightAngle();
	Color4c GetCurSunColor(){return cur_sun_color;}
	Color4c GetCurFoneColor(){return cur_fone_color;}
	Color4c GetCurFogColor();
	void setEnableChangeReflectSkyColor(bool enable) { enableChangeReflectSkyColor = enable; } 
	Color4c GetCurReflectSkyColor(){return cur_reflect_sky_color;}
	void SetCurReflectSkyColor(Color4c color){cur_reflect_sky_color=Color4f(color);}
	bool isDay() { return is_day; }
	bool DayChanged() { return is_day != prev_is_day; }
	bool CheckIsDay();
	void serialize(Archive& ar);
	void SetFogCircle(bool need);
	void setFogHeight(int height);
	const Vect3f& sunPosition() const;
	float sunSize() const;

	float GetShadowFactor()const{return shadow_factor;}

protected:
	cScene* pScene;
	class cRenderSky* pCubeRender;
	FogOfWar* pFogOfWar;
	float day_time;
	float time_angle;
	float dayTimeScale_;
	float nightTimeScale_;

	bool is_day;

	Color4c cur_sun_color;
	Color4c cur_fone_color;
	Color4c current_fog_color;
	Color4c cur_reflect_sky_color;
	bool prev_is_day;

	float phase_cloud;
	int iag_clouds;
	float sun_radius;

	float shadow_factor;

	bool enableChangeReflectSkyColor;
	
	class cSkyObj*	skyObj_;

	float CalcNormalScale();
};
