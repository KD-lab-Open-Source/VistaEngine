
#pragma once

#include "..\render\src\NParticle.h"
#define FOLDER_SPRITE "Sprite\\"
#define FOLDER_TEXTURES "Textures\\"

//#define DEFAULT_TXT_FIRE "fire01.tga"
//#define DEFAULT_TXT_LIGHT "lightmap.tga"
extern const float one_size_model;

///////////////////////////////////////////////////////////////////////////
//
class UnicalID 
{
	static UINT current_id;
protected:
	UINT id;
public:
	void operator =(const UnicalID& obj){ id = obj.id;}
	UINT GetID(){return id;}
//	void SetID(UINT ID){id = ID;}
	UnicalID(){id = current_id++;}
};
class CEmitterData : public UnicalID//, public CObject
{
	CString m_name;
	EmitterKeyBase* data;
	EmitterKeyLight* data_light;
	EmitterKeyColumnLight* column_light;
	EmitterLightingKey* lighting;
	bool            bActive;
	bool            bDirty;
	bool			bSave;
	string			tempTextureNames[10];

//	CKey            key_dummy;
	EmitterKeyInt*			GetInt();
	EmitterKeySpl*			GetSpl();
	EmitterKeyZ*			GetZ();
	EmitterKeyLight*		GetLight();
	EmitterKeyColumnLight*	GetColumnLight();
	EmitterLightingKey*		GetLighting();
	void Clear();
	void Init();

	void Load(LPCTSTR szPath, LPCTSTR szFolderSprite);
	void Save(LPCTSTR szPath);
public:

	EmitterKeyInterface*    emitter() const;
	EmitterKeyBase*			GetBase();

	CEmitterData(){Init();}
	CEmitterData(CEmitterData* emitter_);
	CEmitterData(EMITTER_CLASS cls);
	CEmitterData(EmitterKeyInterface* pk);
	~CEmitterData();

	void Serialize(CArchive& ar, const CString& path, const CString& texture_path,int version);
	void Reset(EMITTER_CLASS cls);
	void Reset(CEmitterData* edat);
//	EmitterKeyInterface* prepareSave();

	bool IsBase();
	bool IsInt();
	bool IsZ();
	bool IsIntZ();
	bool IsSpl();
	bool IsLight();
	bool IsCLight();
	bool IsLighting();
	bool IsValid();
	EMITTER_CLASS Class();

	void SetDirty(bool dirty){bDirty = dirty;}
	bool GetDirty(){return bDirty;}
	void SetActive(bool active){bActive = active;}
	bool IsActive(){return bActive;}
	bool IsSave(){return bSave;}
	void SetSave(bool b){bSave = b;}

	float Effect2EmitterTime(float fEffectTime);
	float ParticleLifeTime();
	float GenerationLifeTime(int nGeneration);
	float GenerationPointTime(int nGenPoint);
	float GenerationPointGlobalTime(int nGenPoint);
	float EmitterEndTime();
//	float& EmitterLifeTime();
//	float& EmitterCreateTime();
//	CKeyPos& emitter_pos();

	void SetGenerationPointCount(int nPoints);
	void InsertGenerationPoint(int nBefore, float k =0);
	void SetGenerationPointTime(int nPoint, float tm);
	void InsertParticleKey(int nBefore,float); //temp
	void SetParticleKeyTime(int nKey, float tm);
	void DeleteGenerationPoint(int nPoint);
	void DeleteParticleKey(int nPoint);
	void ChangeLifetime(float tm);
	void ChangeGenerationLifetime(int nGeneration, float tm);
	void DeletePositionKey(KeyPos* key);
	void ChangeParticleLifeTime(float t);
	float GetParticleLifeTime();
	void SetParticleLifeTime(float t);
	
	int GetGenerationPointNum();
	float GetGenerationPointTime(int i);

	void GetBeginSpeedMatrix(int nGenPoint, int nPoint, MatXf& mat);

	void ChangeLightingCount(int n_count);

	void change_particle_life_time(int nGenPoint, float tm);
	void set_particle_key_time(int nGenPoint, int nParticleKey, float tm);
	float get_particle_key_time(int nGenPoint, int nParticleKey);
	void set_gen_point_time(int nPoint, float tm);
//	void Save(CSaver &sv);
	void Load(CLoadData* rd);

	//EmitterKeyInterface
	SpiralData& spiral_data();
	string& name();
	string& texture_name();
	bool& cycled();
	float& emitter_create_time();
	float& emitter_life_time();
	CKeyPos& emitter_position();
	bool& randomFrame();
	int& texturesCount();
	string& textureName(int i) {return emitter()->textureNames[i];}
	//string& TempTextureName(int i) {return tempTextureNames[i];};

	//other
	CKey& emitter_size();
	CKeyColor& emitter_color();
	string& texture2_name();
	CKey& u_vel();
	CKey& v_vel();
	CKey& height();
	EMITTER_BLEND& color_mode();
	EMITTER_BLEND& light_blend();

	//EmitterKeyBase
	EMITTER_BLEND& sprite_blend();
	bool& generate_prolonged();
	float& particle_life_time();
	EMITTER_TYPE_ROTATION_DIRECTION& rotation_direction();
	CKeyRotate& emitter_rotation();
	EmitterType& particle_position();
	CKey& life_time();

	CKey& life_time_delta();
	CKey& begin_size();
	CKey& begin_size_delta();
	CKey& num_particle();
	string& other();
	bool& relative();
	CKey& emitter_scale();

	bool& chFill();
	bool& chPlume();
	int& TraceCount();
	float& PlumeInterval();
	bool& smooth();
	
	bool& sizeByTexture();
	
	bool& velNoise();
	CKey& velOctaves();
	float& velFrequency();
	float& velAmplitude();
	bool& velOnlyPositive();
	string& velNoiseOther();
	bool& IsVelNoiseOther();

	bool& dirNoise();
	CKey& dirOctaves();
	float& dirFrequency();
	float& dirAmplitude();
	bool& dirOnlyPositive();
	string& dirNoiseOther();
	bool& IsDirNoiseOther();
	bool& BlockX();
	bool& BlockY();
	bool& BlockZ();
	bool& noiseReplace();
	bool& mirage();
	bool& softSmoke();

	char& draw_first_no_zbuffer();

	CVectVect3f& begin_position(); 
	CVectVect3f& normal_position();

	CKey&	   p_size();
//	CKeyColor& p_color();
	CKeyColor& emitter_alpha();
	CKey&	   p_angle_velocity();

	bool& ignoreParticleRate();
	float& k_wind_min();
	float& k_wind_max();
	bool& need_wind();
	bool& toObjects();
	bool& toTerrain();
	bool& cone();
	bool& bottom();

	bool& OnWater();

	//EmitterKeyInt
	bool& use_light();
	CKey&  p_velocity();
	CKey&  p_gravity();

	CKey& velocity_delta();

	vector<KeyParticleInt>& keyInt();
	vector<EffectBeginSpeed>& begin_speed();

	//EmitterKeyZ
	float& add_z();
	bool& planar();
	bool& oriented();
	bool& planar_turn();
	bool& orientedCenter();
	bool& orientedAxis();
	bool& angle_by_center();
	float& base_angle();
	bool& use_force_field();

	//EmitterKeySpl
	bool& p_position_auto_time();
	CKeyPosHermit&    p_position();
	EMITTER_TYPE_DIRECTION_SPL& direction();
	vector<KeyParticleSpl>& keySpl();
	
	//Lighting
	float& generate_time();
	float& strip_width_begin();
	float& strip_width_time();
	float& strip_length();
	float& fade_time();
	float& lighting_amplitude();
	Vect3f& pos_begin();
	vector<Vect3f>& pos_end();

	//ColumnLight
	CKey& emitter_size2();
	bool& turn();
	bool& plane();
	QuatF& rot();
	bool& laser();
};										

typedef vector<EffectBeginSpeed> BSKey;

///////////////////////////////////////////////////////////////////////////////////////////
// key wrappers
struct KeyWrapBase
{
	virtual int    size() = 0;
	virtual float& time(int i) = 0;
	virtual float& value(int i) = 0;

	float& operator()(int i){return value(i);}
};

struct KeyDummyWrap : KeyWrapBase
{
	float _f;
	int    size(){return 0;}
	float& time(int i){return _f;}
	float& value(int i){return _f;}
};
struct KeyFltWrap : KeyWrapBase
{
	CKey* k;
	CKeyPosHermit* kh;
	CKeyPos* kl;
	bool hermit;
	bool light;
	float f;

	KeyFltWrap(CKey& _k)			{k = &_k;  kh = NULL;  kl = NULL; hermit = false; light = false; f = 0;}
	KeyFltWrap(CKeyPosHermit& _kh)	{k = NULL; kh = &_kh;  kl = NULL; hermit = true;  light = false; f = 0;}
	KeyFltWrap(CKeyPos& _kl)		{k = NULL; kh = NULL;  kl = &_kl; hermit = false; light = true; f = 0;}

	int size()
	{			
		if(hermit) return (*kh).size();	 
		else{
			if(light)return (*kl).size(); 
			else return (*k).size();
		}
	}
	float& time(int i)
	{	
		if(hermit) return (*kh)[i].time; 
		else{
			if(light)return (*kl)[i].time;
			else return (*k)[i].time;
		}
	}
	float& value(int i) 
	{	
		if(hermit) 
			return f;	
		else 
			return (*k)[i].f;
	}
};
struct KeyBsWrap : KeyWrapBase
{
	BSKey& k;

	KeyBsWrap(BSKey& _k) : k(_k){}

	int    size(){return k.size();}
	float& time(int i){return k[i].time;}
	float& value(int i) {return k[i].mul;}
};

///////////////////////////////////////////////////////////////////////////
//
typedef TAutoReleaseContainer< vector<CEmitterData*> > EmitterListType;

class CEffectData : public EffectKey , public UnicalID//, public CObject
{
	bool              bDirty;
	EmitterListType   emitters;
	bool			bExpand;
	float			modelScale;
	bool			scaleEffectWithModel;
	float			effectScale;
	int				visibleGroup;
	int				nChain;
public:
	float GetModelScale(){return modelScale;}
	float GetEffectScale(){return effectScale;}
	bool IsExpand(){return bExpand;}
	void SetExpand(bool b);
	CString			Path3Dmodel;
	bool			Show3Dmodel;



	CEffectData();
	CEffectData(EffectKey* pk);
	CEffectData(CEffectData* efdat);
	~CEffectData();
	void Serialize(CArchive& ar, const CString& path, const CString& texture_path, int version);
	void Clear();
	void Reset(CEffectData* eff);
	
	int EmittersSize(){return emitters.size();}
	int EmitterIndex(CEmitterData* p);
	CEmitterData* Emitter(int ix);
	CEmitterData* add_emitter(EMITTER_CLASS cls = EMC_INTEGRAL);
	CEmitterData* add_emitter(CEmitterData* em);
	void del_emitter(CEmitterData* p);
	void swap_emitters(CEmitterData* p1, CEmitterData* p2);
	void move_emitters(CEmitterData* p1, CEmitterData* p2);

	void insert_emitter(int i, CEmitterData* emitter);		

	void prepare_effect_data(CEmitterData *pActiveEmitter,bool toExport=false);

	float GetTotalLifeTime();
	bool CheckEmitterName(const char* nm);
//	bool CheckName(LPCTSTR lpsz, CEmitterData* p = 0);
	bool HasModelEmitter();
	void RelativeScale(float scale);
	void ModelScale(float sc);
	bool& ScaleEffectWithModel(){return scaleEffectWithModel;}

	void SetDirty(bool dirty){bDirty = dirty;}
	bool GetDirty(){return bDirty;}
	void SetVisibleGroup(int visGroup){visibleGroup = visGroup;}
	int GetVisibleGroup(){return visibleGroup;}
	void SetChain(int chain){nChain = chain;}
	int GetChain(){return nChain;}
//	void Save(CSaver& saver);
//	bool Load(LPCTSTR szFolder, LPCTSTR szFolderSprite);
//	void Save(LPCTSTR szFolder);
};

typedef  TAutoReleaseContainer< vector<CEffectData*> >  EffectStorageType;

class CGroupData  : public UnicalID// : public CObject
{
	EffectStorageType  m_effects;
	int bExpand;
public:
	CString m_name,	m_Path3DBack;
	bool	m_bShow3DBack;
public:
	CGroupData();
	CGroupData(LPCTSTR name);
	~CGroupData(){Clear();}

	bool IsExpand(){return bExpand;}
	void SetExpand(bool b);

	int EffectIndex(CEffectData* p);
	int EffectsSize(){return m_effects.size();}
	CEffectData* Effect(int ix);
	CEffectData* AddEffect(CEffectData* eff =NULL);
	void DeleteEffect(CEffectData*);
	void Serialize(CArchive& ar, const CString& path, int version);
//	void AddEffect(LPCTSTR name, LPCTSTR model_name, bool bExpand);
	void operator =(CGroupData& group);
	void Clear();
//	bool IDisEqual(CGroupData* gr){return id==gr->id;}
	bool CheckEffectName(LPCTSTR lpsz);
//	void SetID(UINT ID){id = ID;}
//	bool Load(LPCTSTR szFolder);
//	void Save(LPCTSTR szFolder);
};

typedef TAutoReleaseContainer< vector<CGroupData*> >  GroupListType;
