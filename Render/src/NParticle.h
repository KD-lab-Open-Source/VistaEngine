#ifndef __N_PARTICLE_H_INCLUDED__
#define __N_PARTICLE_H_INCLUDED__
/*
  Известные баги (фичи), которые правиться не будут:
      У нас сейчас нет вечно живущих частиц, поэтому приходится извращаться.
      Если сделать зацикленный спецэффект в котором частица живет столько же сколько и эмиттер,
	  то при пересоздании угол частицы будет такой же как и во время смерти. 
	  Но! Это все не очень корректно работает, если продолжительность кадра больше, чем время жизни частицы.
*/

#include "..\..\Util\Serialization\Saver.h"
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
	void SetParameters(int countValues, float frequency,float amplitude,CKey octavesAmplitude,bool onlyPositive,bool refresh);
protected:
	float Noise(int x);
	void InitRndValues(int oct,bool first =false);
	float Interpolate(float x,int oct);
	vector<vector<float> > rndValues_;
	vector<float> oldx;
	CKey octavesAmplitude_;
	int countValues_;
	float frequency_;
	float amplitude_;
	int numOctaves_;
	bool positive_;
};

struct KeyParticle
{
	float dtime;
	sColor4f color;
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
	friend class cEmitterSpl;
	float inv_dtime;//inv_dtime=1/dtime
};

enum EMITTER_CLASS
{
	EMC_BAD=0,
	EMC_INTEGRAL,
	EMC_INTEGRAL_Z,
	EMC_SPLINE,
	EMC_LIGHT,
	EMC_COLUMN_LIGHT,
	EMC_LIGHTING, 
};

enum EMITTER_TYPE_POSITION
{
	EMP_BOX=0,
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
	EMV_BOX=0,
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

enum EMITTER_TYPE_DIRECTION_SPL
{
	ETDS_ID,
	ETDS_ROTATEZ,
	ETDS_BURST1,
	ETDS_BURST2,
};

enum EMITTER_TYPE_ROTATION_DIRECTION
{
	ETRD_CW=0,
	ETRD_CCW=1,
	ETRD_RANDOM=2,
};

enum EMITTER_BLEND
{
	EMITTER_BLEND_MULTIPLY=0,
	EMITTER_BLEND_ADDING=1,
	EMITTER_BLEND_SUBSTRACT=2,
};

enum COLUMN_TYPE
{
	CT_NONE=0,
	CT_PLANE=1,
	CT_PLANE_ROTATE=2, 
	CT_TRUNC_CONE=4,
};

struct EmitterType
{
	EMITTER_TYPE_POSITION type;
	//Последующие значения зависят от типа эмиттера
	Vect3f size;
	bool fix_pos;
	struct CountXYZ{short x,y,z;}num;
	//r=size.x
	float alpha_min,alpha_max,teta_min,teta_max;

	EmitterType()
	{
		type=EMP_BOX;
		size.set(0,0,0);
		alpha_min=-M_PI;
		alpha_max=+M_PI;
		teta_min=-M_PI/2;
		teta_max=+M_PI/2;
		fix_pos = false;
		num.x = num.y = 5;
		num.z =1;
	}

	void Save(Saver& s,int id);
	void Load(CLoadIterator& rd);
};

struct EffectSpeedParam
{
	Vect3f pos;
	Vect3f dpos;
};

struct EffectBeginSpeed:KeyGeneral
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
		time=0;
		esp.pos.set(1,1,1);
		esp.dpos.set(0,0,0);
	}

	void Save(Saver& s);
	void Load(CLoadIterator& rd);

};

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
			dalpha=0.5;
			r1=0;
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

struct EmitterKeyInterface
{
	EmitterKeyInterface();
	virtual ~EmitterKeyInterface(){};
	virtual void Save(Saver& s)=0;
	virtual void Load(CLoadDirectory rd)=0;
	virtual void RelativeScale(float scale)=0;
	virtual void MulToColor(sColor4c color)=0;
	virtual EMITTER_CLASS GetType()=0;
	virtual void BuildKey()=0;

	SpiralData spiral_data;
	string name;
	string texture_name;//Дублируется с textureNames, не вычистили БЛИН.

	enum { num_texture_names=10 };
	string textureNames[num_texture_names];
	bool cycled;//После emitter_life_time эмиттер начинает работать заново
	float emitter_create_time,emitter_life_time;
	CKeyPos   emitter_position;
	bool randomFrame;

	//eBlendMode sprite_blend;

	float LocalTime(float t);
	float GlobalTime(float t);

	virtual void GetTextureNames(vector<string>& names);

	virtual EmitterKeyInterface* Clone()=0;

	virtual void preloadTexture();

	virtual void BuildRuntimeData() {}	//Пересчитывает данные, которые создаются из известных по
};

struct EmitterKeyColumnLight : public EmitterKeyInterface
{
	EmitterKeyColumnLight();
	virtual void Save(Saver& s);
	virtual void Load(CLoadDirectory rd);
	virtual void RelativeScale(float scale);
	virtual EmitterKeyInterface* Clone();
	virtual EMITTER_CLASS GetType(){return EMC_COLUMN_LIGHT;};
	virtual void BuildKey();

	void MulToColor(sColor4c color)
	{
		emitter_color.MulToColor(sColor4f(color));
	}

	string texture2_name;

	EMITTER_BLEND sprite_blend;
	EMITTER_BLEND color_mode;

	CKey	  emitter_size;
	CKey	  emitter_size2;
	CKeyColor emitter_color;
	CKeyColor emitter_alpha;

	CKey u_vel;
	CKey v_vel;
	CKey height;
	bool turn;
	bool plane;
	QuatF rot;
	bool laser;

	void preloadTexture();
	void GetTextureNames(vector<string>& names);
};

struct EmitterKeyLight:public EmitterKeyInterface
{
public:
	EmitterKeyLight();
	virtual void Save(Saver& s);
	virtual void Load(CLoadDirectory rd);
	virtual void RelativeScale(float scale);
	virtual EmitterKeyInterface* Clone();
	virtual EMITTER_CLASS GetType(){return EMC_LIGHT;};
	virtual void BuildKey();
	void MulToColor(sColor4c color)
	{
		emitter_color.MulToColor(sColor4f(color));
	}

	CKey	  emitter_size;
	CKeyColor emitter_color;
	bool toObjects;
	bool toTerrain;
	EMITTER_BLEND light_blend;
};

struct EmitterKeyBase:public EmitterKeyInterface//EmitterKeyLight
{
	EmitterKeyBase();
	virtual ~EmitterKeyBase();
	virtual void Save(Saver& s)=0;
	virtual void Load(CLoadDirectory rd)=0;
	virtual void RelativeScale(float scale);
	virtual EMITTER_CLASS GetType()=0;
	virtual void BuildKey()=0;
	void MulToColor(sColor4c color)
	{
		p_color.MulToColor(sColor4f(color));
	}

	//Параметры эмиттера
	EMITTER_BLEND sprite_blend;
	bool generate_prolonged;
	float particle_life_time;
	EMITTER_TYPE_ROTATION_DIRECTION rotation_direction;

	CKeyRotate emitter_rotation;
	EmitterType particle_position;

	///Общие параметры частиц
	CKey life_time;
	CKey life_time_delta;
	CKey inv_life_time;
	CKey begin_size;
	CKey begin_size_delta;
	CKey num_particle;
	CKey emitter_scale;
	string other;
	bool relative;
	bool planar;
	bool oriented;
	float base_angle;
	bool ignoreParticleRate;
	bool orientedCenter;
	bool orientedAxis;

	bool dirNoiseEnable;//
	CKey dirOctavesAmplitude;//
	float dirFrequency;//
	float dirAmplitude;//
	bool dirOnlyPositive;//

	bool velNoiseEnable;
	CKey velOctavesAmplitude;
	float velFrequency;
	float velAmplitude;
	bool velOnlyPositive;

	bool isDirNoiseOther;
	bool isVelNoiseOther;
	string dirNoiseOther;
	string velNoiseOther;
	bool noiseBlockX;
	bool noiseBlockY;
	bool noiseBlockZ;
	bool noiseReplace;

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

	char draw_first_no_zbuffer;//0- с учётом z-буфера, 1 - после травы, 2 - перед травой.

	CVectVect3f begin_position; //Распределение по 3D модели for EMP_3DMODEL_INSIDE
	CVectVect3f normal_position;//Распределение по 3D модели  for EMP_3DMODEL_INSIDE

	//Параметры отдельной частицы
	CKey	  p_size;
	CKeyColor p_color;
	CKeyColor p_alpha;
	CKey	  p_angle_velocity;

	float k_wind_min;
	float k_wind_max;
	bool need_wind;


	//Возвращает в секундах, принимает в LocalTime
	void GetParticleLifeTime(float t,float& mid_t,float& min_t,float& max_t);

	KeyPos* GetOrCreatePosKey(float t,bool* create);
	KeyRotate* GetOrCreateRotateKey(float t,bool* create);
	void GetPosition(float t,MatXf& m);
	void Load3DModelPos(c3dx* model/*LPCTSTR fname*/);

	virtual void BuildRuntimeData() { BuildInvLifeTime(); }

protected:
	virtual void SaveInternal(Saver& s);
	virtual void LoadInternal(CLoadData* ld);
	void add_sort(vector<float>& xsort,CKey& c);
	void add_sort(vector<float>& xsort,CKeyColor& c);
	void end_sort(vector<float>& xsort);
	void BuildInvLifeTime();
};

struct EmitterKeyInt:public EmitterKeyBase
{
	EmitterKeyInt();
	~EmitterKeyInt();

	void Save(Saver& s);
	void Load(CLoadDirectory rd);
	void RelativeScale(float scale);
	EmitterKeyInterface* Clone();
public:
	bool use_light;//Освещение частиц, только при EMP_3DMODEL,EMP_3DMODEL_INSIDE. 
				//Используется из первого попавшегося материала diffuse и ambient.

	Vect3f g;	//ускорение

	//Параметры отдельной частицы
	CKey	  p_velocity;
	CKey	  p_gravity;
	bool angle_by_center;//Не имеет смысла ставить true, если planar=false
	
	EMITTER_CLASS GetType(){return EMC_INTEGRAL;}
	void BuildKey();
	vector<KeyParticleInt>& GetKey(){return key;};

public:
	CKey velocity_delta;
	vector<EffectBeginSpeed> begin_speed;
protected:
	vector<KeyParticleInt> key;
	virtual void SaveInternal(Saver& s);
};


struct EmitterKeyZ:public EmitterKeyInt
{
	EmitterKeyZ();

	void Save(Saver& s);
	void RelativeScale(float scale);
	EmitterKeyInterface* Clone();

	EMITTER_CLASS GetType(){return EMC_INTEGRAL_Z;}

	void BuildKey();

	float add_z;
	bool use_force_field;
	bool use_water_plane;

protected:
	void LoadInternal(CLoadData* ld);
};

struct EmitterKeySpl:public EmitterKeyBase
{
public:
	//#define EMITTER_TCONTROL
	struct HeritKey
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

public:
	EmitterKeySpl();
	~EmitterKeySpl();

	void Save(Saver& s);
	void Load(CLoadDirectory rd);
	void RelativeScale(float scale);
	EmitterKeyInterface* Clone();
public:	
	bool p_position_auto_time;//Автоматически прределять время для наиболее равномерного движения
	CKeyPosHermit    p_position;

	EMITTER_TYPE_DIRECTION_SPL direction;

	EMITTER_CLASS GetType(){return EMC_SPLINE;}
	void BuildKey();
	vector<KeyParticleSpl>& GetKey(){return key;};
	vector<HeritKey>& GetHeritKey(){return hkeys;};
protected:
	vector<KeyParticleSpl>	key;
	vector<HeritKey>		hkeys;
};
struct EmitterLightingKey : EmitterKeyInterface
{
	LightingParameters param;
	Vect3f pos_begin;
	vector<Vect3f> pos_end;

	void Save(Saver& s);
	void Load(CLoadDirectory rd);
	void RelativeScale(float scale);
	EMITTER_CLASS GetType(){return EMC_LIGHTING;}
	EmitterKeyInterface* Clone();
	void BuildKey();
	void MulToColor(sColor4c color){ }
};
class EffectKey
{
public:
	string name;
	string filename;
	vector<EmitterKeyInterface*> key;

	EffectKey();
	~EffectKey();

	void Save(Saver& s);
	void Load(CLoadDirectory rd);

	void RelativeScale(float scale);
	void MulToColor(sColor4c color);

	bool IsNeed3DModel();
	bool IsCycled();

	void operator=(const EffectKey& effect_key);
	bool GetNeedTilemap();

	void preloadTexture();
	void changeTexturePath(const char* path);
	void BuildRuntimeData();

protected:
	bool need_tilemap;
	bool delete_assert;
	void Clear();
};

class FunctorGetZ:public cUnknownClass
{
public:
	virtual float GetZ(float x,float y)=0;
};

/////////////////////////////cEmitter/////////////////////////////////////////
class cEmitterInterface
{
	friend class cEffect;
public:
	cEmitterInterface();
	virtual ~cEmitterInterface();

	virtual void PreDraw(cCamera *pCamera)=0;
	virtual void Draw(cCamera *pCamera)=0;
	virtual void Animate(float dt)=0;

	virtual bool IsLive()=0;
	virtual bool IsVisible(cCamera *pCamera)=0;
	virtual bool IsMirage(){return false;}
	virtual void SetDummyTime(float t)=0;

	void SetPause(bool b);
	bool GetPause()const{return b_pause;};
	virtual void SetParent(cEffect* parent);
	void SetCycled(bool cycled);
	bool IsCycled()const{return cycled;}

	float GetStartTime()const { return emitter_key->emitter_create_time; }
	float GetLiveTime()const{return emitter_life_time;}
	EmitterKeyInterface * GetUnicalID() { return emitter_key; }
	void SetShowObjEditor(bool b){no_show_obj_editor=!b;}

	virtual void SetFunctorGetZ(FunctorGetZ* func){}
	virtual void AddZ(float z){}
	char IsDrawNoZBuffer(){return /*emitter_key->*/draw_first_no_zbuffer;}
	inline const MatXf& GetLocalMatrix() const 										{ return LocalMatrix; }
	virtual void SetTarget(const Vect3f* pos_end,int pos_end_size){};
	virtual int GetParticleCount(){return 0;}
	void SetTexture(int n,cTexture *pTexture);
	inline cTexture* GetTexture(int n=0)											{ return Texture[n]; }
protected:
	friend class cEffect;
	virtual void Show(bool show){}
	virtual void DisableEmitProlonged(){}
	cEffect* parent;
	float time,old_time;
	float totaltime;
	bool b_pause;
	bool cycled;
	float emitter_life_time;
	eBlendMode blend_mode;
	EmitterKeyInterface* emitter_key;
	bool no_show_obj_editor;
	char draw_first_no_zbuffer;
	MatXf			LocalMatrix;	// локальная матрица объекта относительно родителя
	MatXf			GlobalMatrix;
	cTexture		*Texture[2];
};

class cEmitterColumnLight : public cEmitterInterface
{
public:
	cEmitterColumnLight();
	virtual bool IsLive();
	virtual bool IsVisible(cCamera *pCamera);
	virtual void SetDummyTime(float t);
	void SetEmitterKey(EmitterKeyColumnLight& k);

	void PreDraw(cCamera *pCamera);
	void Draw(cCamera *pCamera);
	void Animate(float dt);
	void SetTarget(const Vect3f* pos_end,int pos_end_size);
protected:
	eColorMode color_mode;
	sBox6f Bound;
	QuatF quat_rotation;
	CKey height;
	float ut,vt;

private:
	EmitterKeyColumnLight * GetEmitterKeyColumnLight() const { return static_cast<EmitterKeyColumnLight*>(emitter_key); }
};

class cEmitterBase:public cEmitterInterface
{
public:
	bool isOther;
	cEmitterBase* other;
	cEmitterBase* velNoiseOther;
	cEmitterBase* dirNoiseOther;
	cEmitterBase();
	~cEmitterBase();

	virtual void PreDraw(cCamera *pCamera);
	virtual void Animate(float dt);

	virtual bool IsLive()=0;

	void SetMaxTime(float emitter_life,float particle_life);
	void SetDummyTime(float t){dummy_time=t;};

	bool IsVisible(cCamera *pCamera);

	float GetVelNoise();
	Vect3f GetDirNoise();
	bool GetNoiseReplace();
	bool IsMirage(){return mirage;}

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
	virtual Vect3f GetVdir(int i)=0;

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
	virtual void DummyQuant()=0;
	virtual void EmitInstantly(float tmin,float tmax)=0;
	virtual void EmitProlonged(float dt)=0;
	int CalcProlongedNum(float dt);

	void OneOrderedPos(int i,Vect3f& pos);
	bool OnePos(int i, Vect3f& pos, Vect3f* norm = NULL, int num_pos = -1);
	virtual bool GetRndPos(Vect3f& pos, Vect3f* norm)=0;
	Vect3f* GetNormal(const int& ix);

	void SetEmitterKey(EmitterKeyBase& k,c3dx* models);
	void DisableEmitProlonged(){disable_emit_prolonged=true;}

	bool init_prev_matrix;
	MatXf prev_matrix;
	MatXf next_matrix;
	void BeginInterpolateGlobalMatrix();
	void InterpolateGlobalMatrix(float f);
	void EndInterpolateGlobalMatrix();
public:
	virtual void CalculatePos(bool mode){;}
	virtual Vect3f& GetParticlePos(int ix){static Vect3f t;return t;}
	virtual void ResetPlumePos(int i){};

private:
	EmitterKeyBase * GetEmitterKeyBase() const { return static_cast<EmitterKeyBase*>(emitter_key); }
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
		begin_index=0;
		plumes_size=0;
		cPlumeOne p;
		p.pos=begin_pos;
		p.time_summary=0;
		p.scale=1;
		plumes.resize(num_plumes,p);
	}

	bool PutToBuf(Vect3f& npos, float& dt,
				cQuadBuffer<sVertexXYZDT1>*& pBuf, 
				const sColor4c& color, const Vect3f& PosCamera,
				const float& size, const sRectangle4f& rt,
				const UCHAR mode,
				float PlumeInterval,//GetEmitterKeyBase()->PlumeInterval
				float time_summary//nParticle::time_summary
				);
};

class cEmitterInt:public cEmitterBase
{
public:
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
		sColor4c begin_color;

		Vect3f normal;
		int nframe;

		cPlume plume;
	};
protected:
	BackVector<nParticle>	Particle;

	// если объединить EffectBeginSpeed и EffectBeginSpeedMatrix можно целиком избавиться
	// от этого вектора и брать значения из EmitterKeyInt
	vector<EffectBeginSpeedMatrix> begin_speed;

public:
	cEmitterInt();
	~cEmitterInt();

	virtual void Draw(cCamera *pCamera);

	bool IsLive(){return !Particle.is_empty() || time<emitter_life_time || cycled;}

	void SetEmitterKey(EmitterKeyInt& k,c3dx* models);
	virtual int GetParticleCount(){return Particle.size();}
protected:
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

private:
	EmitterKeyInt * GetEmitterKeyInt() const { return static_cast<EmitterKeyInt*>(emitter_key); }
};

class cEmitterZ:public cEmitterInt
{
	float add_z;
	FunctorGetZ* func_getz;
public:
	cEmitterZ();
	~cEmitterZ();
	void Draw(cCamera *pCamera);
	virtual void ProcessTime(nParticle& p,float dt,int i,Vect3f& cur_pos);
	void SetEmitterKey(EmitterKeyZ& k,c3dx* models);

	void SetParent(cEffect* parent);
	float CalcZ(float pos_x,float pos_y);
	void SetFunctorGetZ(FunctorGetZ* func){	RELEASE(func_getz);func_getz=func;func_getz->AddRef();};
	virtual void AddZ(float z){add_z+=z;}
protected:
	virtual bool GetRndPos(Vect3f& pos, Vect3f* norm);
	virtual void EmitOne(int ix_cur/*nParticle& cur*/,float begin_time, int num_pos = -1);

private:
	EmitterKeyZ * GetEmitterKeyZ() const { return static_cast<EmitterKeyZ*>(emitter_key); }
};

class cEmitterSpl:public cEmitterBase
{
	typedef EmitterKeySpl::HeritKey HeritKey;

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

	BackVector<nParticle>	Particle;

public:
	cEmitterSpl();
	~cEmitterSpl();

	virtual void Draw(cCamera *pCamera);
	bool IsLive(){return !Particle.is_empty() || time<emitter_life_time || cycled;}

	void SetEmitterKey(EmitterKeySpl& k,c3dx* models);

protected:

	void EmitInstantly(float tmin,float tmax);
	void EmitProlonged(float dt);
	void EmitOne(int ix_cur/*nParticle& cur*/,float begin_time);
	virtual bool GetRndPos(Vect3f& pos, Vect3f* norm);
	virtual Vect3f GetVdir(int i);
	void ProcessTime(nParticle& p,float dt,int i);
	void DummyQuant();

private:
	EmitterKeySpl * GetEmitterKeySpl() const { return static_cast<EmitterKeySpl*>(emitter_key); }
};


class cEmitterLight:public cEmitterInterface
{
public:
	cEmitterLight();
	~cEmitterLight();
	void Animate(float dt);
	virtual void PreDraw(cCamera *pCamera){};
	virtual void Draw(cCamera *pCamera){};

	bool IsLive(){return time<emitter_life_time || cycled;}
	bool IsVisible(cCamera *pCamera){return false;}

	void SetEmitterKey(EmitterKeyLight& k);
	void SetDummyTime(float t){};

protected:
	class cUnkLight* light;
	void Show(bool show);

private:
	EmitterKeyLight * GetEmitterKeyLight() const { return static_cast<EmitterKeyLight*>(emitter_key); }
};

class cLightingEmitter
{
	cLighting* lighting;
	Vect3f pos_beg;
	vector<Vect3f> pos_end;
	float time;
	bool cycled;
	float emitter_create_time;
	float emitter_life_time;
	cEffect *parent;
	bool attached;
	EmitterKeyInterface* unical_id;
	bool no_show_obj_editor;
public:
	cLightingEmitter();
	~cLightingEmitter();
	void SetPosition(const MatXf &mat);
	void SetEmitterKey(EmitterLightingKey& key, cEffect *parent);
	void Animate(float dt);
	bool IsLive();
	bool IsCycled()const{return cycled;}
	void SetTarget(const Vect3f* pos_end,int pos_end_size);
	void SetCycled(bool cycled_);
	EmitterKeyInterface* GetUnicalID(){return unical_id;};
	void SetShowObjEditor(bool b){no_show_obj_editor=!b;}
};
class cEffect:public cIUnkObj
{
	friend class cEmitterBase;
	vector<cEmitterInterface*> emitters;
	vector<cLightingEmitter*> lightings;
	float time;
	bool auto_delete_after_life;
	float particle_rate;
	float distance_rate;
	bool isTarget;
	class EffectObserverLink3dx:protected ObserverLink
	{
	public:
		class c3dx* object;
		int node;
		cEffect* effect;
	public:
		EffectObserverLink3dx():object(0),node(-1),effect(0){}
		void SetParent(cEffect* effect_){effect=effect_;}

		void Link(class c3dx* object,int inode);
		virtual void Update();
		void DeInitialization(){if(observer)observer->BreakLink(this);object = NULL;}
		bool IsInitialized(){return object!=0 && IsLink();}
		const MatXf& GetRootMatrix();
		bool IsLink() {return observer?true:false;}
	} link3dx;
	 
	void Clear();
public:
	cEffect();
	~cEffect();

	virtual void Animate(float dt);
	virtual void PreDraw(cCamera *pCamera);
	virtual void Draw(cCamera *pCamera);
	virtual	int Release();

	bool IsLive();

	float GetTime()const {return time;};
	float GetSummaryTime();
	void SetPosition(const MatXf& Matrix);
	void AddZ(float z)
	{
		vector<cEmitterInterface*>::iterator it;
		FOR_EACH(emitters,it)
			(*it)->AddZ(z);
	}
	void SetCycled(bool cycled);
	bool IsCycled()const;
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
	void SetAutoDeleteAfterLife(bool auto_delete_after_life_);
	bool IsAutoDeleteAfterLife()const{return auto_delete_after_life;}

	//Остановить генерацию спрайтов и удалить спецэффект после исчезновения спрайтов 
	void StopAndReleaseAfterEnd();

	//Модулирует количество генерируемых частиц, для эмиттеров, в которых происходит генерация частиц.
	//Если частица только одна, то она не исчезнет, пока rate не будет равно точно 0.
	//Для эмиттеров, не генерирующих частицы имеет смысл триггера 0 выключено, больше 0 - включенно.
	void SetParticleRate(float rate);
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
	static bool test_far_visible;
	static float near_distance;
	static float far_distance;

	string GetName();
	string effect_name;
#ifdef C_CHECK_DELETE
	string check_file_name;
	string check_effect_name;
#endif


	bool IsUseFogOfWar()const{return no_fog_of_war;};
	void SetUseFogOfWar(bool use){no_fog_of_war=!use;}

	virtual void SetAttr(int attribute);
	virtual void ClearAttr(int attribute);
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
};

class EffectLibrary2
{
	struct Entry
	{
		float scale;
		string texture_path;
		sColor4c skin_color;
		EffectKey * pEffect;
	};

	MTSection mtlock;
public:
	~EffectLibrary2();
	//!!!!!!!В этом интерфейсе есть одна большая проблемма - у EffectKey нет Release поэтому нельзя узнать когда эффекты перестают использоваться.
	EffectKey * Get(const char * filename, float scale = 1.0f,const char* texture_path=NULL,sColor4c skin_color=sColor4c(255,255,255));
	void preloadLibrary(const char * filename, const char* texture_path=NULL);
private:
	EffectKey * Load(const char * filename, const char* texture_path);
	EffectKey* Copy(EffectKey* ref,float scale,const char* texture_path,sColor4c skin_color);

	vector<Entry> list;
};

extern EffectLibrary2* gb_EffectLibrary;


bool GetAllTextureNamesEffectLibrary(const char* filename, vector<string>& names);

#endif
