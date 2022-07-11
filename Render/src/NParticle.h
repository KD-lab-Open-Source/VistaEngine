#ifndef __N_PARTICLE_H_INCLUDED__
#define __N_PARTICLE_H_INCLUDED__
/*
  Известные баги (фичи), которые правиться не будут:
      У нас сейчас нет вечно живущих частиц, поэтому приходится извращаться.
      Если сделать зацикленный спецэффект в котором частица живет столько же сколько и эмиттер,
	  то при пересоздании угол частицы будет такой же как и во время смерти. 
	  Но! Это все не очень корректно работает, если продолжительность кадра больше, чем время жизни частицы.
*/

#include "MTSection.h"
#include "Render\3dx\Saver.h"
#include "XMath\XMath.h"
#include "Serialization\StringTableBase.h"
#include "Serialization\StringTableReference.h"
#include "Render\Inc\IUnkObj.h"
#include "NParticleKey.h"
#include "observer.h"
#include "texture.h"
#include "material.h"
#define EXPORT_TO_GAME 1
#ifndef _FINAL_VERSION_
	#define NEED_TREANGLE_COUNT 
#endif
#include "lighting.h"

class PSOverdraw;
class PSOverdrawColor;
class PSOverdrawCalc;
class cEffect;
enum eBlendMode;
enum eColorMode;
template <class vertex> class cQuadBuffer;
struct sVertexXYZDT1;

class PerlinNoise
{
public:
	PerlinNoise();
	~PerlinNoise();

	float Get(float x);
	void SetParameters(int countValues, float frequency,float amplitude,KeysFloat octavesAmplitude,bool onlyPositive,bool refresh);

	void serialize(Archive& ar);

protected:

	vector<vector<float> > rndValues_;
	vector<float> oldx;
	KeysFloat octavesAmplitude_;
	int countValues_;
	float frequency_;
	float amplitude_;
	int numOctaves_;
	bool positive_;

	float Noise(int x);
	void InitRndValues(int oct,bool first =false);
	float Interpolate(float x,int oct);
};

struct KeyParticle
{
	float dtime;
	Color4f color;
	float size;//величина частицы
};

struct KeyParticleInt : public KeyParticle
{
	float vel;//скорость частицы, абсолютное значение
	float angle_vel;
	float gravity;
//protected:
	friend class cEmitterInt;
	friend class cEmitterZ;
	float inv_dtime;//inv_dtime=1/dtime
};

struct KeyParticleSpl : public KeyParticle
{
	float angle_vel;
//protected:
	friend class cEmitterSpline;
	float inv_dtime;//inv_dtime=1/dtime
};

enum EMITTER_CLASS
{
	EMC_BAD = 0,
	EMC_INTEGRAL,
	EMC_INTEGRAL_Z,
	EMC_SPLINE,
	EMC_LIGHT,
	EMC_COLUMN_LIGHT,
	EMC_LIGHTING, 
};

enum EMITTER_TYPE_POSITION
{
	EMP_BOX = 0,
	EMP_CYLINDER,
	EMP_SPHERE,
	EMP_LINE,
	EMP_RING,
	EMP_3DMODEL,
	EMP_3DMODEL_INSIDE,
	EMP_OTHER_EMITTER,
};

enum EMITTER_TYPE_VELOCITY
{
	EMV_BOX = 0,
	EMV_CYLINDER=1,
	EMV_SPHERE=2,
	EMV_LINE=3,
	EMV_NORMAL_IN=4,
	EMV_NORMAL_OUT=5,
	EMV_NORMAL_IN_OUT=6,
	EMV_Z=7,
	EMV_NORMAL_3D_MODEL = 8,
	EMV_INVARIABLY,
};

enum EMITTER_TYPE_DIRECTION_SPLINE
{
	ETDS_ID,
	ETDS_ROTATEZ,
	ETDS_BURST1,
	ETDS_BURST2,
};

enum EMITTER_TYPE_ROTATION_DIRECTION
{
	ETRD_CW = 0,
	ETRD_CCW=1,
	ETRD_RANDOM=2,
};

enum EMITTER_BLEND
{
	EMITTER_BLEND_MULTIPLY = 0,
	EMITTER_BLEND_ADD=1,
	EMITTER_BLEND_SUBSTRACT=2,
};

enum EmitterZMode
{
	EMITTER_USE_ZBUFFER = 0,
	EMITTER_DRAW_AFTER_GRASS,
	EMITTER_DRAW_BEFORE_GRASS,
	EMITTER_DRAW_AFTER_ALL
};

enum COLUMN_TYPE
{
	CT_NONE = 0,
	CT_PLANE=1,
	CT_PLANE_ROTATE=2, 
	CT_TRUNC_CONE=4,
};

class EmitterType
{
public:
	EMITTER_TYPE_POSITION type;
	//Последующие значения зависят от типа эмиттера
	Vect3f size;
	bool fix_pos;

	struct CountXYZ
	{
		short x,y,z;

		void serialize(Archive& ar);
	};

	CountXYZ num;

	float alpha_min;
	float alpha_max;
	float teta_min;
	float teta_max;

	EmitterType()
	{
		type=EMP_BOX;
		size.set(0,0,0);
		alpha_min=-M_PI;
		alpha_max=+M_PI;
		teta_min=-M_PI_2;
		teta_max=+M_PI_2;
		fix_pos = false;
		num.x = num.y = 5;
		num.z =1;
	}

	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);

	void serialize(Archive& ar);
};

struct EffectSpeedParam
{
	Vect3f pos;
	Vect3f dpos;
};

struct EffectBeginSpeed : KeyBase
{
	string name;
	EMITTER_TYPE_VELOCITY velocity;
	float mul;
	QuatF rotation;

	EffectSpeedParam esp;

	EffectBeginSpeed()
	{
		velocity=EMV_BOX;
		mul=1.0f;
		rotation=QuatF::ID;
		time = 0;
		esp.pos.set(1,1,1);
		esp.dpos.set(0,0,0);
	}

	void Save(Saver& s);
	void Load(CLoadIterator& rd);

};
inline float& keyValue(EffectBeginSpeed& value){
	return value.mul;
}
struct EffectBeginSpeedMatrix
{
	EMITTER_TYPE_VELOCITY velocity;
	float mul;
	Mat3f rotation;
	float time_begin;

	EffectSpeedParam esp;
};

inline bool operator <(const EffectBeginSpeedMatrix& s1,const EffectBeginSpeedMatrix& s2)
{
	return s1.time_begin<s2.time_begin;
}

class SpiralData
{
public:
	class Dat
	{
	public:
		MatXf mat;
		float dalpha;
		float r1,r2;
		float h1,h2;
		float compress;
		Dat():mat(MatXf::ID)
		{
			dalpha = 0.5;
			r1 = 0;
			r2 = 100;
			h1= 0;
			h2= 100;
			compress= 1;
		}
	protected:
	friend class SpiralData;
		void Save(Saver& s)
		{
			s<<dalpha;
			s<<r1;
			s<<r2;
			s<<h1;
			s<<h2;
			s<<compress;
		}
		void Load(CLoadIterator &rd)
		{
			rd>>dalpha;
			rd>>r1;
			rd>>r2;
			rd>>h1;
			rd>>h2;
			rd>>compress;
		}
	};
protected:
	Dat dat[2];
	int active;
public:
	SpiralData():active(-1){};
	void SetActive(int ix){ xassert((UINT)ix<2); active = ix;}
	Dat& GetActiveData(){return dat[active];}
	Dat& GetData(int ix){xassert((UINT)ix<2); return dat[ix];}
	void Save(Saver& s, int id)
	{
		if (s.GetData()!=EXPORT_TO_GAME)
		{
			s.push(id);     
			dat[0].Save(s);
			dat[1].Save(s);
			s.pop();
		}
	}
	void Load(CLoadData* ld)
	{
		CLoadIterator rd(ld);
		dat[0].Load(rd);
		dat[1].Load(rd);
	}
};
/////////////////////////////EmitterKey/////////////////////////////////////////

class EmitterKeyInterface : public ShareHandleBase
{
public:
	EmitterKeyInterface();
	virtual ~EmitterKeyInterface(){
		int dummy = 0;
	};

	virtual void Save(Saver& s) = 0;
	virtual void Load(CLoadDirectory rd) = 0;
	virtual void RelativeScale(float scale) = 0;
	virtual void MulToColor(Color4c color) = 0;
	virtual EMITTER_CLASS GetType() = 0;
	virtual void BuildKey() = 0;

	virtual bool needModel() const { return false; }

	SpiralData spiral_data;
	string name;
	string texture_name;//Дублируется с textureNames, не вычистили БЛИН.

	enum { num_texture_names=10 };
	string textureNames[num_texture_names];
	bool cycled;//После emitter_life_time эмиттер начинает работать заново
	bool visibleInEditor;

	float emitter_create_time;
	float emitter_life_time;

	KeysPos emitter_position;

	bool randomFrame;

	EmitterZMode zMode_;
	bool ignoreDistanceCheck_;

	float LocalTime(float t);
	float GlobalTime(float t);

	virtual void GetTextureNames(vector<string>& names, bool omitEmpty = true);
	virtual void setTextureNames(const vector<string>& fileNames);
	virtual int getTextureCount() const;
	virtual EmitterKeyInterface* Clone() = 0;
	virtual void preloadTexture();
	virtual void BuildRuntimeData() {}	//Пересчитывает данные, которые создаются из известных по

	virtual void serialize(Archive& ar);

	// ф-ии используемые только в редакторе
	virtual void changeParticleLifeTime(int generationPoint, float time){ emitter_life_time = clamp(time, 0.01f, 100.0f); }
	virtual void setParticleLifeTime(float time){}
	virtual float getParticleLongestLifeTime() const{ return emitter_life_time; }
	virtual float getParticleLifeTime() const{ return 0.0f; }
	virtual float getGenerationPointTime(int index) const{
		return (index == 0 ? 0 : emitter_life_time);
	}
	virtual int generationPointCount() const{ return 2; }
	virtual void setGenerationPointTime(int index, float time) {};
	virtual float getEmitterEndTime() const{ return emitter_life_time + emitter_create_time; }
	virtual float particleKeyTime(int generation, int keyIndex) const{ xassert(0); return 0.0f; }
	virtual void setParticleKeyTime(int keyIndex, float time);
	virtual void setParticleKeyTime(int generation, int keyIndex, float time);
	virtual void insertParticleKey(int before, float position);
	virtual void deleteParticleKey(int index);
	virtual float generationLifeTime(int generation) const{ return emitter_life_time; }
	virtual KeysColor* gradientColor(){ return 0; }
	virtual KeysColor* gradientAlpha(){ return 0; }
	virtual KeysPosHermit* spline(){ return 0; }
};

struct EmitterKeyColumnLight : public EmitterKeyInterface
{
	EmitterKeyColumnLight();
	void Save(Saver& s);
	void Load(CLoadDirectory rd);
	void RelativeScale(float scale);
	EmitterKeyInterface* Clone();
	EMITTER_CLASS GetType(){return EMC_COLUMN_LIGHT;};
	virtual void BuildKey();

	void MulToColor(Color4c color)
	{
		emitter_color.MulToColor(Color4f(color));
	}

	string texture2_name;

	EMITTER_BLEND sprite_blend;
	EMITTER_BLEND color_mode;

	KeysFloat	  emitter_size;
	KeysFloat	  emitter_size2;
	KeysColor emitter_color;
	KeysColor emitter_alpha;

	KeysFloat u_vel;
	KeysFloat v_vel;
	KeysFloat height;
	KeysFloat length;

	bool turn;
	bool plane;
	QuatF rot;
	bool laser;
	bool discrete_laser;

	float missileSpeed;

	void preloadTexture();
	void GetTextureNames(vector<string>& names, bool omitEmpty = true);
	int getTextureCount() const;
	void setTextureNames(const vector<string>& fileNames);

	void serialize(Archive& ar);
	void setParticleKeyTime(int keyIndex, float time);
	void insertParticleKey(int before, float position);
	void deleteParticleKey(int index);

	float particleKeyTime(int generation, int keyIndex) const{
		return emitter_life_time * emitter_size[keyIndex].time;
	}
	KeysColor* gradientColor(){ return &emitter_color; }
	KeysColor* gradientAlpha(){ return &emitter_alpha; }
};

struct EmitterKeyLight : public EmitterKeyInterface
{
public:
	EmitterKeyLight();
	virtual void Save(Saver& s);
	virtual void Load(CLoadDirectory rd);
	virtual void RelativeScale(float scale);
	virtual EmitterKeyInterface* Clone();
	virtual EMITTER_CLASS GetType(){return EMC_LIGHT;};
	virtual void BuildKey();
	void MulToColor(Color4c color)
	{
		emitter_color.MulToColor(Color4f(color));
	}

	KeysFloat	  emitter_size;
	KeysColor emitter_color;
	bool toObjects;
	bool toTerrain;
	EMITTER_BLEND light_blend;

	void serialize(Archive& ar);
	void setParticleKeyTime(int keyIndex, float time);
	void insertParticleKey(int before, float position);
	float particleKeyTime(int generation, int keyIndex) const{
		return emitter_life_time * emitter_size[keyIndex].time;
	}
	void deleteParticleKey(int index);
	KeysColor* gradientColor(){ return &emitter_color; }
};

struct NoiseParams{
	NoiseParams()
	: onlyPositive(false)
	, frequency(1.0f)
	, amplitude(1.0f)
	{
		octaveAmplitudes.push_back(KeyFloat(0.0f, 0.5f));
	}
	void serialize(Archive& ar);

	bool onlyPositive;
	KeysFloat octaveAmplitudes;
	float frequency;
	float amplitude;
};

struct EmitterNoise{
	EmitterNoise()
	: enabled(false)
	, fromOtherEmitter(false)
	{}
	void serialize(Archive& ar);
	NoiseParams params;
	bool enabled;
	bool fromOtherEmitter;
	string otherEmitterName;
};

struct EmitterNoiseBlockable : EmitterNoise{

	EmitterNoiseBlockable()
	: EmitterNoise()
	, blockX(false)
	, blockY(false)
	, blockZ(false)
	, replace(true)
	{
	}
	void serialize(Archive& ar);
	bool blockX;
	bool blockY;
	bool blockZ;
	bool replace;
};

class EmitterKeyBase : public EmitterKeyInterface
{
public:
	EmitterKeyBase();
	virtual ~EmitterKeyBase();
	void RelativeScale(float scale);
	void MulToColor(Color4c color){	p_color.MulToColor(Color4f(color));	}

	//Параметры эмиттера
	EMITTER_BLEND sprite_blend;
	bool generate_prolonged;
	float particle_life_time;
	EMITTER_TYPE_ROTATION_DIRECTION rotation_direction;

	KeysRotate emitter_rotation;
	EmitterType particle_position;

	///Общие параметры частиц
	KeysFloat life_time;
	KeysFloat life_time_delta;
	KeysFloat inv_life_time;
	KeysFloat begin_size;
	KeysFloat begin_size_delta;
	KeysFloat num_particle;
	KeysFloat emitter_scale;
	string other;
	bool relative;
	bool planar;
	bool oriented;
	float base_angle;
	bool ignoreParticleRate;
	bool orientedCenter;
	bool orientedAxis;

	EmitterNoiseBlockable directionNoise;
	EmitterNoise velocityNoise;

	PerlinNoise velNoise;
	PerlinNoise dirNoiseX;
	PerlinNoise dirNoiseY;
	PerlinNoise dirNoiseZ;

	bool  chFill;
	bool  chPlume;
	int   TraceCount;
	float PlumeInterval;
	bool smooth;
	bool cone;
	bool bottom;
	bool sizeByTexture;
	bool mirage;
	bool softSmoke;
	bool turn;
	bool preciseBound_;

	CVectVect3f begin_position; //Распределение по 3D модели for EMP_3DMODEL_INSIDE
	CVectVect3f normal_position;//Распределение по 3D модели  for EMP_3DMODEL_INSIDE

	//Параметры отдельной частицы
	KeysFloat	  p_size;
	KeysColor p_color;
	KeysColor p_alpha;
	KeysFloat	  p_angle_velocity;

	float k_wind_min;
	float k_wind_max;
	bool need_wind;

	//Возвращает в секундах, принимает в LocalTime
	void GetParticleLifeTime(float t,float& mid_t,float& min_t,float& max_t);

	KeyPos* GetOrCreatePosKey(float t,bool* create);
	KeyRotate* GetOrCreateRotateKey(float t,bool* create);
	void GetPosition(float t,MatXf& m);
	void Load3DModelPos(c3dx* model/*LPCTSTR fname*/);

	bool needModel() const { return particle_position.type == EMP_3DMODEL; }

	virtual void BuildRuntimeData() { BuildInvLifeTime(); }

	void serialize(Archive& ar);

	void setParticleLifeTime(float time);
	void changeParticleLifeTime(int generationPoint, float time);

	float getParticleLifeTime() const;
	float getGenerationPointTime(int index) const;
	void setGenerationPointTime(int index, float time);
	int generationPointCount() const{ return int(emitter_position.size()); }
	virtual void setGenerationPointCount(int count);
	float getParticleLongestLifeTime() const;
	float generationLifeTime(int generation) const;

	float particleKeyTime(int generation, int keyIndex) const;
	void setParticleKeyTime(int generation, int keyIndex, float time);
	void setParticleKeyTime(int keyIndex, float time);
	void insertParticleKey(int before, float position);
	void deleteParticleKey(int index);

	KeysColor* gradientColor(){ return &p_color; }
	KeysColor* gradientAlpha(){ return &p_alpha; }
protected:
	virtual void SaveInternal(Saver& s);
	virtual void LoadInternal(CLoadData* ld);
	void add_sort(vector<float>& xsort,KeysFloat& c);
	void add_sort(vector<float>& xsort,KeysColor& c);
	void end_sort(vector<float>& xsort);
	void BuildInvLifeTime();
};

struct EffectBeginSpeeds : public vector<EffectBeginSpeed>{
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
};

class EmitterKeyInt : public EmitterKeyBase
{
public:
	EmitterKeyInt();
	~EmitterKeyInt();

	void Save(Saver& s);
	void Load(CLoadDirectory rd);
	void RelativeScale(float scale);
	EmitterKeyInterface* Clone();

	void serialize(Archive& ar);

	bool use_light;//Освещение частиц, только при EMP_3DMODEL,EMP_3DMODEL_INSIDE. 
				//Используется из первого попавшегося материала diffuse и ambient.

	Vect3f g;	//ускорение

	//Параметры отдельной частицы
	KeysFloat	  p_velocity;
	KeysFloat	  p_gravity;
	bool angle_by_center;//Не имеет смысла ставить true, если planar=false
	
	EMITTER_CLASS GetType(){return EMC_INTEGRAL;}
	void BuildKey();
	vector<KeyParticleInt>& GetKey(){return key;};

	KeysFloat velocity_delta;
	EffectBeginSpeeds begin_speed;

	void setGenerationPointTime(int index, float time);
	void setGenerationPointCount(int count);
	void setParticleKeyTime(int keyIndex, float time);
	void insertParticleKey(int before, float position);
	void deleteParticleKey(int index);

protected:

	vector<KeyParticleInt> key;
	virtual void SaveInternal(Saver& s);
};


class EmitterKeyZ : public EmitterKeyInt
{
public:
	EmitterKeyZ();

	void Save(Saver& s);
	void RelativeScale(float scale);
	EmitterKeyInterface* Clone();

	EMITTER_CLASS GetType(){ return EMC_INTEGRAL_Z; }

	void BuildKey();

	void serialize(Archive& ar);

	float add_z;
	bool use_force_field;
	bool use_water_plane;

protected:
	void LoadInternal(CLoadData* ld);
};

class EmitterKeySpline : public EmitterKeyBase
{
public:
	//#define EMITTER_TCONTROL
	struct HermitKey
	{
		float dtime;
		float inv_dtime;//inv_dtime=1/dtime
		Vect3f H0,H1,H2,H3;

	#ifdef EMITTER_TCONTROL
		float t0,t1,t2,t3;

		inline void Get(Vect3f& out,float ts)
		{
			float t=t0+t1*ts+t2*ts*ts+t3*ts*ts*ts;
			//VISASSERT(t==ts);
			float tt=t*t,ttt=tt*t;
			out.x=H0.x+H1.x*t+H2.x*tt+H3.x*ttt;
			out.y=H0.y+H1.y*t+H2.y*tt+H3.y*ttt;
			out.z=H0.z+H1.z*t+H2.z*tt+H3.z*ttt;
		}
	#else
		inline void Get(Vect3f& out,float t)
		{
			float tt=t*t,ttt=tt*t;
			out.x=H0.x+H1.x*t+H2.x*tt+H3.x*ttt;
			out.y=H0.y+H1.y*t+H2.y*tt+H3.y*ttt;
			out.z=H0.z+H1.z*t+H2.z*tt+H3.z*ttt;
		}
	#endif
	};

	EmitterKeySpline();
	~EmitterKeySpline();

	void Save(Saver& s);
	void Load(CLoadDirectory rd);
	void RelativeScale(float scale);
	EmitterKeyInterface* Clone();

	void serialize(Archive& ar);

	void setParticleKeyTime(int keyIndex, float time);
	void insertParticleKey(int before, float position);
	void deleteParticleKey(int index);
	KeysPosHermit* spline(){ return &p_position; }
public:	
	bool p_position_auto_time;//Автоматически прределять время для наиболее равномерного движения
	KeysPosHermit    p_position;

	EMITTER_TYPE_DIRECTION_SPLINE direction;

	EMITTER_CLASS GetType(){return EMC_SPLINE;}
	void BuildKey();
	vector<KeyParticleSpl>& GetKey(){return key;};
	vector<HermitKey>& GetHermitKey(){return hkeys;};

protected:
	vector<KeyParticleSpl>	key;
	vector<HermitKey>		hkeys;
};

class EmitterKeyLighting : public EmitterKeyInterface
{
public:
	EmitterKeyLighting();
	void Save(Saver& s);
	void Load(CLoadDirectory rd);
	void RelativeScale(float scale);

	EMITTER_CLASS GetType(){return EMC_LIGHTING;}
	EmitterKeyInterface* Clone();

	void BuildKey();
	void MulToColor(Color4c color){ emitter_color.MulToColor(Color4f(color)); }

	void serialize(Archive& ar);

	LightingParameters param;
	Vect3f pos_begin;
	vector<Vect3f> pos_end;
	KeysColor emitter_color;
};

class EffectKey : public StringTableBase
{
public:
	EffectKey(const char* name = "");
	~EffectKey();

	void Save(Saver& s);
	void Load(CLoadDirectory rd);

	void RelativeScale(float scale);
	void MulToColor(Color4c color);

	bool needModel();
	bool isCycled();

	void operator=(const EffectKey& effect_key);
	bool GetNeedTilemap();

	void preloadTexture();
	void changeTexturePath(const char* path);
	void BuildRuntimeData();

	void serialize(Archive& ar);

	string name;
	string filename;
	typedef vector<ShareHandle<EmitterKeyInterface> > EmitterKeys;
	EmitterKeys emitterKeys;

protected:

	bool need_tilemap;
	bool delete_assert;
	void Clear();
};

typedef StringTable<EffectKey> EffectKeyTable;
typedef StringTableReference<EffectKey, false> EffectKeyReference;

class FunctorGetZ
{
public:
	virtual float getZf(int x, int y) const = 0;
};

class FunctorWindVelocity
{
public:
	virtual const Vect2f& windVelocity(const Vect2i& pos) const = 0;
};

/////////////////////////////cEmitter/////////////////////////////////////////
class cEmitterInterface
{
public:
	cEmitterInterface();
	virtual ~cEmitterInterface();

	virtual void PreDraw(Camera* camera) = 0;
	virtual void Draw(Camera* camera) = 0;
	virtual void Animate(float dt) = 0;

	virtual bool isAlive() = 0;
	virtual bool isVisible(Camera* camera) = 0;
	virtual bool isMirage() {return false; }
	virtual void setDummyTime(float t) = 0;

	virtual void SetPosition(const MatXf &mat){ }

	void setPause(bool b);
	bool isPaused()const{return b_pause;};
	virtual void setParent(cEffect* parent);
	void setCycled(bool cycled);
	bool isCycled()const{return cycled;}

	float startTime()const { return emitterKey_->emitter_create_time; }
	float lifeTime()const{return emitter_life_time;}
	EmitterKeyInterface* ID() { return emitterKey_; }
	void SetShowObjEditor(bool b){no_show_obj_editor=!b;}

	virtual void SetFunctorGetZ(FunctorGetZ* func){}
	virtual void AddZ(float z){}
	EmitterZMode zMode() const { return zMode_; }
	inline const MatXf& GetLocalMatrix() const { return LocalMatrix; }
	virtual void SetTarget(const Vect3f* pos_end,int pos_end_size){};
	virtual int GetParticleCount(){return 0;}
	void SetTexture(int n,cTexture *pTexture);
	inline cTexture* GetTexture(int n = 0) { return Texture[n]; }

	static setTerraFunctor(FunctorGetZ* terraFunctor) { terraFunctor_ = terraFunctor; }
	static setWaterFunctor(FunctorGetZ* waterFunctor) { waterFunctor_ = waterFunctor; }
	static setWindFunctor(FunctorWindVelocity* windFunctor) { windFunctor_ = windFunctor; }

protected:

	virtual void Show(bool show){}
	virtual void DisableEmitProlonged(){}
	cEffect* parent;
	float time,old_time;
	float totaltime;
	bool b_pause;
	bool cycled;
	float emitter_life_time;
	eBlendMode blend_mode;
	ShareHandle<EmitterKeyInterface> emitterKey_;
	bool no_show_obj_editor;
	EmitterZMode zMode_;
	MatXf			LocalMatrix;	// локальная матрица объекта относительно родителя
	MatXf			GlobalMatrix;
	cTexture		*Texture[2];

	static FunctorGetZ* terraFunctor_;
	static FunctorGetZ* waterFunctor_;
	static FunctorWindVelocity* windFunctor_;

	virtual EmitterKeyInterface* emitterKey() const { return emitterKey_; }

	friend class cEffect;
};

class cEmitterColumnLight : public cEmitterInterface
{
public:
	cEmitterColumnLight();
	bool isAlive();
	bool isVisible(Camera* camera);
	void setDummyTime(float t);
	void SetEmitterKey(EmitterKeyColumnLight& k);

	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
	void Animate(float dt);
	void SetTarget(const Vect3f* pos_end,int pos_end_size);

protected:

	eColorMode color_mode;
	sBox6f Bound;
	QuatF quat_rotation;
	KeysFloat height;
	float ut,vt;

	EmitterKeyColumnLight* emitterKey() const { return static_cast<EmitterKeyColumnLight*>(emitterKey_.get()); }
};

class cEmitterBase : public cEmitterInterface
{
public:
	bool isOther;
	cEmitterBase* other;
	cEmitterBase* velNoiseOther;
	cEmitterBase* dirNoiseOther;
	cEmitterBase();
	~cEmitterBase();

	virtual void PreDraw(Camera* camera);
	virtual void Animate(float dt);

	virtual bool isAlive() = 0;

	void SetMaxTime(float emitter_life,float particle_life);
	void setDummyTime(float t){dummy_time=t;};

	bool isVisible(Camera* camera);

	float GetVelNoise();
	Vect3f GetDirNoise();
	bool GetNoiseReplace();
	bool isMirage(){return mirage;}

protected:

	float dummy_time;
	float inv_emitter_life_time;
	float particle_life_time;
	float inv_particle_life_time;
	bool ignoreParticleRate;

	sBox6f Bound;

	bool disable_emit_prolonged;
	float time_emit_prolonged;

	float scaleX;
	float scaleY;

	bool sizeByTexture;

	bool mirage;
	bool softSmoke;

	bool oldZBufferWrite;

public:
	int cur_one_pos;//Индекс в begin_position
	virtual Vect3f GetVdir(int i) = 0;

protected:
	enum 
	{
		rotate_angle_size=512,
		rotate_angle_mask=rotate_angle_size-1,
	};
	void InitRotateAngle();

	Vect3f RndBox(float x,float y,float z);
	Vect3f RndSpere(float r);
	Vect3f RndCylinder(float r1,float r2,float dr,float z,bool btm);
	Vect3f RndRing(float r,float alpha_min,float alpha_max,float teta_min,float teta_max);

	void CalcPosition();
	virtual void DummyQuant() = 0;
	virtual void EmitInstantly(float tmin,float tmax) = 0;
	virtual void EmitProlonged(float dt) = 0;
	int CalcProlongedNum(float dt);

	void OneOrderedPos(int i,Vect3f& pos);
	bool OnePos(int i, Vect3f& pos, Vect3f* norm = 0, int num_pos = -1);
	virtual bool GetRndPos(Vect3f& pos, Vect3f* norm) = 0;
	Vect3f* GetNormal(const int& ix);

	void SetEmitterKey(EmitterKeyBase& k,c3dx* models);
	void DisableEmitProlonged(){disable_emit_prolonged=true;}

	bool init_prev_matrix;
	MatXf prev_matrix;
	MatXf next_matrix;
	void BeginInterpolateGlobalMatrix();
	void InterpolateGlobalMatrix(float f);
	void EndInterpolateGlobalMatrix();

	EmitterKeyBase* emitterKey() const { return static_cast<EmitterKeyBase*>(emitterKey_.get()); }
public:
	virtual void CalculatePos(bool mode) {}
	virtual Vect3f& GetParticlePos(int ix){static Vect3f t;return t;}
	virtual void ResetPlumePos(int i){};
};

struct cPlumeOne
{
	Vect3f pos;
	float time_summary;
	float scale;
};

struct cPlume
{
	int begin_index;
	int plumes_size;
	vector<cPlumeOne> plumes;

	void Init(int num_plumes,Vect3f begin_pos)
	{
		num_plumes=max(num_plumes,3);
		begin_index = 0;
		plumes_size = 0;
		cPlumeOne p;
		p.pos=begin_pos;
		p.time_summary = 0;
		p.scale=1;
		plumes.resize(num_plumes,p);
	}

	bool PutToBuf(Vect3f& npos, float& dt,
				cQuadBuffer<sVertexXYZDT1>*& pBuf, 
				const Color4c& color, const Vect3f& PosCamera,
				const float& size, const sRectangle4f& rt,
				const UCHAR mode,
				float PlumeInterval,//emitterKey()->PlumeInterval
				float time_summary//nParticle::time_summary
				);
};

class cEmitterInt : public cEmitterBase
{
public:
	cEmitterInt();
	~cEmitterInt();

	virtual void Draw(Camera* camera);

	bool isAlive(){return !Particle.is_empty() || time<emitter_life_time || cycled;}

	void SetEmitterKey(EmitterKeyInt& k,c3dx* models);
	virtual int GetParticleCount(){return Particle.size();}

	struct nParticle
	{
		float time;//текущее время в пределах ключа анимации 0..1
		int key;//номер ключа анимации
		float inv_life_time;

		Vect3f pos0;
		Vect3f vdir;//начальное направление движения
		float angle0,angle_dir;
		float baseAngle;
		float gvel0;
		//color0,size0 - константы
		float begin_size;

		float time_summary;
		int key_begin_time;
		Color4c begin_color;

		Vect3f normal;
		int nframe;

		cPlume plume;
	};

protected:
	BackVector<nParticle>	Particle;

	// если объединить EffectBeginSpeed и EffectBeginSpeedMatrix можно целиком избавиться
	// от этого вектора и брать значения из EmitterKeyInt
	vector<EffectBeginSpeedMatrix> begin_speed;

	cObjMaterial material;

	void EmitInstantly(float tmin,float tmax);
	void EmitProlonged(float dt);
	virtual void EmitOne(int ix_cur/*nParticle& cur*/,float begin_time,int num_pos = -1);
	virtual bool GetRndPos(Vect3f& pos, Vect3f* norm);
	virtual Vect3f GetVdir(int i);
	Vect3f CalcVelocity(const EffectBeginSpeedMatrix& s,const nParticle& cur,float mul);

	virtual void ProcessTime(nParticle& p,float dt,int i,Vect3f& cur_pos);
	void DummyQuant();
	void CalcColor(nParticle& cur);

	float real_angle;
/*
	Если частицы генерируются только EmitInstantly в нулевой момент времени и время жизни частиц 
	равно времени жизни эмиттера, то эти частицы не убиваются, а происходит их переинициализация.
	Это нужно ля того, чтобы можно было сделать бесконечно долго живущюю частицу, которая не мигает не прикаких условиях.
*/
	bool is_intantly_infinity_particle;
	void CalcIntantlyInfinityParticle();

	bool SetFreeOrCycle(int index_particle);//Возвращает, удалена ри реально частица

	EmitterKeyInt* emitterKey() const { return static_cast<EmitterKeyInt*>(emitterKey_.get()); }
};

class cEmitterZ : public cEmitterInt
{
public:
	cEmitterZ();
	~cEmitterZ();
	void Draw(Camera* camera);
	virtual void ProcessTime(nParticle& p,float dt,int i,Vect3f& cur_pos);
	void SetEmitterKey(EmitterKeyZ& k,c3dx* models);

	void setParent(cEffect* parent);
	float CalcZ(float pos_x,float pos_y);
	void SetFunctorGetZ(FunctorGetZ* func){	func_getz=func; }
	virtual void AddZ(float z){add_z+=z;}

protected:

	virtual bool GetRndPos(Vect3f& pos, Vect3f* norm);
	virtual void EmitOne(int ix_cur/*nParticle& cur*/,float begin_time, int num_pos = -1);

	EmitterKeyZ* emitterKey() const { return static_cast<EmitterKeyZ*>(emitterKey_.get()); }

private:
	float add_z;
	FunctorGetZ* func_getz;
};

class cEmitterSpline : public cEmitterBase
{
public:
	cEmitterSpline();
	~cEmitterSpline();

	virtual void Draw(Camera* camera);
	bool isAlive(){return !Particle.is_empty() || time<emitter_life_time || cycled;}

	void SetEmitterKey(EmitterKeySpline& k,c3dx* models);

protected:

	struct nParticle
	{
		int   key;//номер ключа анимации
		float time;//текущее время в пределах ключа анимации 0..dtime
		float inv_life_time;
		float time_summary;
		int nframe;

		//То-же, но для сплайнов
		int   hkey;
		float htime;
		MatXf pos;
		float angle0,angle_dir;
		//color0,size0 - константы
		float begin_size;

		cPlume plume;
	};

	void EmitInstantly(float tmin,float tmax);
	void EmitProlonged(float dt);
	void EmitOne(int ix_cur/*nParticle& cur*/,float begin_time);
	virtual bool GetRndPos(Vect3f& pos, Vect3f* norm);
	virtual Vect3f GetVdir(int i);
	void ProcessTime(nParticle& p,float dt,int i);
	void DummyQuant();

	EmitterKeySpline* emitterKey() const { return static_cast<EmitterKeySpline*>(emitterKey_.get()); }

private:
	typedef EmitterKeySpline::HermitKey HermitKey;

	BackVector<nParticle>	Particle;
};


class cEmitterLight : public cEmitterInterface
{
public:
	cEmitterLight();
	~cEmitterLight();
	void Animate(float dt);
	virtual void PreDraw(Camera* camera){};
	virtual void Draw(Camera* camera){};

	bool isAlive(){return time<emitter_life_time || cycled;}
	bool isVisible(Camera* camera){return false;}

	void SetEmitterKey(EmitterKeyLight& k);
	void setDummyTime(float t){};

protected:
	class cUnkLight* light;
	void Show(bool show);

private:

	EmitterKeyLight* emitterKey() const { return static_cast<EmitterKeyLight*>(emitterKey_.get()); }
};

class cEmitterLighting : public cEmitterInterface
{
public:
	cEmitterLighting();
	~cEmitterLighting();

	void PreDraw(Camera* camera);
	void Draw(Camera* camera);

	bool isVisible(Camera* camera){ return true; }
	void setDummyTime(float t){ }

	void SetPosition(const MatXf &mat);
	void SetEmitterKey(EmitterKeyLighting& key, cEffect *parent);
	void Animate(float dt);
	bool isAlive();
	bool isCycled()const{return cycled;}
	void SetTarget(const Vect3f* pos_end,int pos_end_size);
	EmitterKeyInterface* ID() { return emitterKey_; }
	void SetShowObjEditor(bool b){no_show_obj_editor=!b;}

private:
	Lighting* lighting;
	Vect3f pos_beg;
	vector<Vect3f> pos_end;
	float emitter_create_time;
	bool attached;

	EmitterKeyLighting* emitterKey() const { return static_cast<EmitterKeyLighting*>(emitterKey_.get()); }
};

class RENDER_API cEffect : public cIUnkObj
{
public:
	cEffect();
	~cEffect();

	void Attach();
	virtual void Animate(float dt);
	virtual void PreDraw(Camera* camera);
	virtual void Draw(Camera* camera);
	virtual	int Release();

	bool isAlive();

	float GetTime()const {return time;};
	float GetSummaryTime();
	void SetPosition(const MatXf& Matrix);
	void AddZ(float z)
	{
		vector<cEmitterInterface*>::iterator it;
		FOR_EACH(emitters,it)
			(*it)->AddZ(z);
	}
	void setCycled(bool cycled);
	bool isCycled() const;
	void SetTarget(const Vect3f& pos_begin, const vector<Vect3f>& pos_end);
	void SetTarget(const Vect3f& pos_begin, const Vect3f& pos_end);
	void SetTarget(const Vect3f& pos_begin, const Vect3f* pos_end,int pos_end_size);
	
	///////////////////////////Только для редактора
	void ShowEmitter(EmitterKeyInterface* emitter_id,bool show);
	void ShowAllEmitter();
	void HideAllEmitter();

	///////////////////////////////////
	void SetTime(float time);
	void MoveToTime(float time);

	//Этой функцией следует пользоваться с осторожностью, лучше использовать StopAndReleaseAfterEnd
	virtual void SetAutoDeleteAfterLife(bool auto_delete_after_life_);
	virtual bool IsAutoDeleteAfterLife()const{return auto_delete_after_life;}

	//Остановить генерацию спрайтов и удалить спецэффект после исчезновения спрайтов 
	virtual void StopAndReleaseAfterEnd();

	//Модулирует количество генерируемых частиц, для эмиттеров, в которых происходит генерация частиц.
	//Если частица только одна, то она не исчезнет, пока rate не будет равно точно 0.
	//Для эмиттеров, не генерирующих частицы имеет смысл триггера 0 выключено, больше 0 - включенно.
	void SetParticleRate(float rate);
	void setInterfaceEffectFlag(bool flag) { isInterfaceEffect_ = flag; }
	void toggleDistanceCheck(bool state){ ignoreDistanceCheck_ = !state; }
	inline float GetParticleRate()const{return particle_rate;}
	int GetParticleCount();
	void LinkToNode(class c3dx* object,int inode);
	inline float GetParticleRateReal()const;
	inline float GetParticleRateRealInstantly(float num)const;

	vector<Vect3f>& GetPos(){return begin_position;}
	vector<Vect3f>& GetNorm(){return normal_position;}
	cEmitterBase* GetEmitN(int n){xassert((UINT)n<emitters.size()); return (cEmitterBase*)emitters[n];}
	void SetFunctorGetZ(FunctorGetZ* func);//Делается вовремя addref,release

	void RecalcBeginPos(int num);
	inline bool IsTarget() {return isTarget;}

	string GetName();
	string effect_name;
#ifdef C_CHECK_DELETE
	string check_file_name;
	string check_effect_name;
#endif

	bool IsUseFogOfWar()const{return no_fog_of_war;};
	void SetUseFogOfWar(bool use){no_fog_of_war=!use;}

	void setAttribute(int attribute);
	void clearAttribute(int attribute);

public:
#ifdef  NEED_TREANGLE_COUNT
	int count_triangle;
	double square_triangle;
	PSOverdraw* psOverdraw;
	PSOverdrawColor* psOverdrawColor;
	PSOverdrawCalc* psOverdrawCalc;
	bool drawOverDraw;
	bool enableOverDraw;
	bool useOverDraw;
	cTexture* pRenderTexture;
	cTexture* pRenderTextureCalc;
	struct IDirect3DSurface9* pSysSurface;
	void initOverdraw();
#endif
	bool IsLinkIgnored();
public:
	void ClearTriangleCount();
	void AddCountTriangle(int count);
	int GetTriangleCount();
	double GetSquareTriangle();

	static void setVisibleRange(bool testFarVisible, float nearDistance2, float farDistance2) { test_far_visible = testFarVisible; near_distance = nearDistance2; far_distance = farDistance2; }

protected:
	friend class cScene;
	friend EffectKey;
	void Init(EffectKey& el,c3dx* models);
	void Add(cEmitterInterface*);//Предполагается, что эмиттер уже инициализированн
	c3dx* model;
	const MatXf& GetCenter3DModel();
	vector<Vect3f> begin_position;//Распределение по 3D модели
	vector<Vect3f> normal_position;//Распределение по 3D модели

	void CalcDistanceRate(float dist);
	bool no_fog_of_war;

#ifdef _DEBUG
	EffectKey* debug_effect_key;
#endif _DEBUG
	FunctorGetZ* func_getz;

	static bool test_far_visible;
	static float near_distance;
	static float far_distance;

private:
	friend class cEmitterBase;
	vector<cEmitterInterface*> emitters;

	float time;
	bool auto_delete_after_life;
	bool isInterfaceEffect_;
	bool ignoreDistanceCheck_;
	float particle_rate;
	float distance_rate;
	bool isTarget;
	bool attached_;

	class EffectObserverLink3dx:protected ObserverLink
	{
	public:
		class c3dx* object;
		int node;
		cEffect* effect;
	public:
		EffectObserverLink3dx():object(0),node(-1),effect(0){}
		void setParent(cEffect* effect_){effect=effect_;}

		void Link(class c3dx* object,int inode);
		virtual void Update();
		void DeInitialization(){if(observer)observer->BreakLink(this);object = 0;}
		bool IsInitialized(){return object!= 0 && IsLink();}
		const MatXf& GetRootMatrix();
		bool IsLink() {return observer?true:false;}
	} link3dx;
	 
	void Clear();
};

class RENDER_API EffectLibrary2
{
	struct Entry
	{
		float scale;
		string texture_path;
		Color4c skin_color;
		EffectKey * pEffect;
	};

	MTSection mtlock;
public:
	~EffectLibrary2();
	//!!!!!!!В этом интерфейсе есть одна большая проблемма - у EffectKey нет Release поэтому нельзя узнать когда эффекты перестают использоваться.
	EffectKey * Get(const char * filename, float scale = 1.0f,const char* texture_path = 0,Color4c skin_color=Color4c(255,255,255));
	void preloadLibrary(const char * filename, const char* texture_path = 0);
private:
	EffectKey * Load(const char * filename, const char* texture_path);
	EffectKey* Copy(EffectKey* ref,float scale,const char* texture_path,Color4c skin_color);

	vector<Entry> list;
};

extern RENDER_API EffectLibrary2* gb_EffectLibrary;


RENDER_API bool GetAllTextureNamesEffectLibrary(const char* filename, vector<string>& names);

#endif
