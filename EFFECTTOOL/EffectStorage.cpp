
#include "stdafx.h"
#include "EffectTool.h"
#include "EffectToolDoc.h"
#include "EffectStorage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT UnicalID::current_id = 0;
const float one_size_model=40.0f;

///////////////////////////////////////////////////////////////////////////
void CEmitterData::Clear()
{
	if (data)
		delete data;
	if (data_light)
		delete data_light;
	if (column_light)
		delete column_light;
	if (lighting)
		delete lighting;
	Init();
}
void CEmitterData::Init()
{
	data = NULL;
	data_light = NULL;
	column_light = NULL;
	lighting = NULL;
	bActive = true;
	bDirty = true;
	bSave = false;
}
CEmitterData::CEmitterData(EMITTER_CLASS cls)
{
	Init();
	Reset(cls);
}
CEmitterData::CEmitterData(CEmitterData* emitter_)
{
	Init();
	Reset(emitter_);
}
CEmitterData::CEmitterData(EmitterKeyInterface* pk)
{
//	xassert(0);
	Init();
	switch(pk->GetType())
	{
	case EMC_INTEGRAL_Z:
		data = (EmitterKeyZ*)pk->Clone();
		break;
	case EMC_LIGHT:
		data_light = (EmitterKeyLight*)pk->Clone();
		break;
	case EMC_INTEGRAL:
		data = (EmitterKeyInt*)pk->Clone();
		break;
	case EMC_SPLINE:
		data = (EmitterKeySpl*)pk->Clone();
		break;
	case EMC_COLUMN_LIGHT:
		column_light = (EmitterKeyColumnLight*)pk->Clone();
		break;
	case EMC_LIGHTING:
		lighting = (EmitterLightingKey*)pk->Clone();
		break;
	default:
		xassert(0);
	}

}
void CEmitterData::Reset(CEmitterData* edat)
{
	xassert(edat);
	Clear();
	bActive = edat->bActive;
	bDirty = edat->bDirty;
	bSave = edat->bSave;
	switch(edat->Class())
	{
	case EMC_INTEGRAL:
	case EMC_INTEGRAL_Z:
	case EMC_SPLINE:
		data = (EmitterKeyBase*)edat->emitter()->Clone();
		break;
	case EMC_LIGHT:
		data_light = (EmitterKeyLight*)edat->emitter()->Clone();
		break;
	case EMC_COLUMN_LIGHT:
		column_light = (EmitterKeyColumnLight*)edat->emitter()->Clone();
		break;
	case EMC_LIGHTING:
		lighting = (EmitterLightingKey*)edat->emitter()->Clone();
		break;
	default:
		xassert(0);
	}
	*((UnicalID*)this)= *((UnicalID*)edat);
}

EmitterKeyInterface* CEmitterData::emitter() const 
{
	if(data)
		return data;
	if(data_light)
		return data_light;
	if (column_light)
		return column_light;
	if (lighting)
		return lighting;
	xassert(0&&"EmitterData without EmitterKey");
	return NULL;
}
CEmitterData::~CEmitterData()
{
	Clear();
}
void CEmitterData::Reset(EMITTER_CLASS cls)
{
	bDirty = true;
	static int _unkn = 1;

	string ename;
	if (IsValid())
		ename = name();
	Clear();
	switch(cls)
	{
	case EMC_INTEGRAL_Z:
		data = new EmitterKeyZ;

		GetZ()->begin_speed.push_back(EffectBeginSpeed());
		GetZ()->begin_speed.push_back(EffectBeginSpeed());
		GetZ()->begin_speed.back().time = 1.f;

		GetZ()->p_velocity[0].f = 0;
		GetZ()->p_velocity.push_back(KeyFloat(1, 0));
		GetZ()->p_gravity.push_back(KeyFloat(1, GetInt()->p_gravity[0].f));
		break;
	case EMC_LIGHT:
		{
			data_light = new EmitterKeyLight;
			KeyPos pos;
			pos.pos.set(0,0,0);
			pos.time=0;
			emitter_position().push_back(pos);

			KeyFloat size;
			size.f=20;
			size.time=0;
			emitter_size().push_back(size);
			size.f=20;
			size.time=1.0f;
			emitter_size().push_back(size);

			KeyColor color;
			color.set(1,1,1,1);
			color.time=0;
			emitter_color().push_back(color);
			color.set(1,1,1,1);
			color.time=1.0f;
			emitter_color().push_back(color);
			break;
		}
	case EMC_INTEGRAL:
		data = new EmitterKeyInt;

		GetInt()->begin_speed.push_back(EffectBeginSpeed());
		GetInt()->begin_speed.push_back(EffectBeginSpeed());
		GetInt()->begin_speed.back().time = 1.f;

		GetInt()->p_velocity[0].f = 0;
		GetInt()->p_velocity.push_back(KeyFloat(1, 0));
		GetInt()->p_gravity.push_back(KeyFloat(1, GetInt()->p_gravity[0].f));
		break;
	case EMC_SPLINE:
		data = new EmitterKeySpl;
		data->num_particle[0].f = 10;
		break;
	case EMC_COLUMN_LIGHT:
		{
			column_light = new EmitterKeyColumnLight;
			KeyPos pos;
			pos.pos.set(0,0,0);
			pos.time=0;
			emitter_position().push_back(pos);

			KeyFloat size;
			size.f=20;
			size.time=0;
			emitter_size().push_back(size);
			emitter_size2().push_back(size);
			height().push_back(size);
			size.f=20;
			size.time=1.0f;
			emitter_size().push_back(size);
			emitter_size2().push_back(size);
			height().push_back(size);


			KeyColor color;
			color.set(1,1,1,1);
			color.time=0;
			emitter_color().push_back(color);
			emitter_alpha().push_back(color);
			color.set(1,1,1,1);
			color.time=1.0f;
			emitter_color().push_back(color);
			emitter_alpha().push_back(color);

			u_vel().push_back(KeyFloat(0,0));
			u_vel().push_back(KeyFloat(1,0));

			v_vel().push_back(KeyFloat(0,0));
			v_vel().push_back(KeyFloat(1,0));
			break;
		}
	case EMC_LIGHTING:
		lighting = new EmitterLightingKey;
		pos_begin().set(0,0,0);
		pos_end().push_back(Vect3f(100,0,0));
		break;
	default:
		xassert(0);
	}

	if(	IsBase()){
		data->generate_prolonged = false;
		data->particle_life_time = 1.0f;
		data->p_size.push_back(KeyFloat(1, data->p_size[0].f));
		data->p_angle_velocity.push_back(KeyFloat(1, data->p_angle_velocity[0].f));
	}

	if (ename.empty())
	{
		char _cb[100];
		sprintf(_cb, "emitter %d", _unkn++);
		name() = _cb;
	}else
		name() = ename;
}

EMITTER_CLASS CEmitterData::Class()
{
	return emitter()->GetType();
}
bool CEmitterData::IsBase(){	return bool(data);}
bool CEmitterData::IsInt(){		return Class() == EMC_INTEGRAL;}
bool CEmitterData::IsZ(){		return Class() == EMC_INTEGRAL_Z;}
bool CEmitterData::IsIntZ(){	return Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z;}
bool CEmitterData::IsSpl(){		return Class() == EMC_SPLINE;}
bool CEmitterData::IsLight(){	return bool(data_light);}
bool CEmitterData::IsCLight(){	return bool(column_light);}
bool CEmitterData::IsLighting(){return bool(lighting);}

bool CEmitterData::IsValid(){	return data || data_light || column_light || lighting;}

EmitterKeyBase*	CEmitterData::GetBase()
{
	return data;
}
EmitterKeyInt* CEmitterData::GetInt()
{
	return dynamic_cast<EmitterKeyInt*>(data);
}
EmitterKeySpl* CEmitterData::GetSpl()
{
	return dynamic_cast<EmitterKeySpl*>(data);
}
EmitterKeyZ* CEmitterData::GetZ()
{
	return dynamic_cast<EmitterKeyZ*>(data);
}
EmitterKeyLight* CEmitterData::GetLight()
{
	return dynamic_cast<EmitterKeyLight*>(data_light);
}
EmitterKeyColumnLight* CEmitterData::GetColumnLight()
{
	return column_light;
}
EmitterLightingKey*	CEmitterData::GetLighting()
{
	return lighting;
}
float CEmitterData::Effect2EmitterTime(float fEffectTime)
{
	float f = 0;
	if(fEffectTime < emitter_create_time())
		f = 0;
	else if(fEffectTime > emitter_life_time() + emitter_create_time())
		f = 1.0f;
	else
		f = (fEffectTime - emitter_create_time())/emitter_life_time();
	return f;
}

float CEmitterData::GenerationLifeTime(int nGeneration)
{
	if(IsBase())
		return life_time()[nGeneration].f;
	else
		return emitter_life_time();
}
float CEmitterData::GenerationPointTime(int nGenPoint)
{
	if(IsBase())
		return num_particle()[nGenPoint].time;
	else
		return emitter_position()[nGenPoint].time;
}
float CEmitterData::GenerationPointGlobalTime(int nGenPoint)
{
	if(IsBase())
		return GenerationPointTime(nGenPoint)*emitter_life_time() + emitter_create_time();
	else
		return emitter_life_time();
}
float CEmitterData::EmitterEndTime()
{
	return emitter_life_time() + emitter_create_time();
}
/*
float& CEmitterData::EmitterLifeTime()
{
	return emitter()->emitter_life_time;
}
float& CEmitterData::EmitterCreateTime()
{
	return emitter()->emitter_create_time;
}

CKeyPos& CEmitterData::emitter_pos()
{
	return emitter()->emitter_position;
}
*/
float CEmitterData::ParticleLifeTime()
{
	float f, fMax = 0;

	if (IsBase())
	{
		int sz = num_particle().size();
		for(int i=0; i<sz; i++)
		{
			f = life_time()[i].f;

			if (generate_prolonged()) {
				f += emitter_life_time();
			}else
				f += num_particle()[i].time*emitter_life_time();

			if(fMax < f)
				fMax = f;
		}
		return fMax;
	}
	else return emitter_life_time();
}

////////////////////////////////
void ResizeKey(int nPoints, CKey& key)
{
	float val = key[0].f;

	key.resize(nPoints);
	if(nPoints > 1)
	{
		key[nPoints-1].time = 1; key[nPoints-1].f = val;

		for(int i=1; i<nPoints-1; i++)
		{
			key[i].time = float(i)/(nPoints-1);
			key[i].f = val;
		}
	}
}
/*void ResizePosRotationKey(const CKey& key_ref, CKeyPos& pos, CKeyRotate& rot)
{
	pos.clear(); rot.clear();
	int n = key_ref.size();
	for(int i=0; i<n; i++)
	{
		pos.push_back(KeyPos());
		KeyPos& p = pos.back();
		p.pos.set(0, 0, 0); p.time = key_ref[i].time;

		rot.push_back(KeyRotate());
		KeyRotate& r = rot.back();
		r.pos = QuatF::ID; r.time = key_ref[i].time;
	}
}
*/

template <class T>
void SetPosRotationKeyTime(int nPoint, float tm, float tm_old, T& key)
{
	T::value_type* pp = 0;
	T::iterator i;
	FOR_EACH(key, i)
		if(fabs(i->time - tm_old) < 0.001f)
		{
			pp = &*i;
			break;
		}

	if(pp)
		pp->time = tm;
}

template <class T>
void RemovePosRotationKey(float tm, T& key)
{
	T::iterator i;
	FOR_EACH(key, i)
		if(fabs(i->time - tm) < 0.001f)
		{
			key.erase(i);
			break;
		}
}

template <class T>
void ClearPosRotationKey(T& key)
{
	if(key.size() > 1)
		key.erase(key.begin() + 1, key.end());
}

template <class T>
void InsertKey(int nBefore, T& key)
{
	T::iterator i;
	FOR_EACH(key, i)
	{
		if(nBefore == 0)
		{
			key.insert(i, *i);
			break;
		}
		nBefore--;
	}
}

template <class T>
void RemoveKey(int nPoint, T& key)
{
	ASSERT(nPoint < key.size());

	key.erase(key.begin() + nPoint);
}

template <class T>
void SetKeyTime(int nKey, float tm, T& key)
{
	if(nKey<key.size())
		key[nKey].time = tm;
}

template <class T>
void SetKeyTimes(T& key, vector<float>& vTimes, float tm)
{
	int nPoints = key.size() - 1;
	for(int i=0; i<nPoints; i++)
	{
		key[i].time = vTimes[i]/tm;

		if(key[i].time > 1.0f)
			key[i].time = 1.0f;
	}
}

template <class T>
void SetKeyTimesPosRot(T& key, float tm_old, float tm)
{
	T::iterator i;
	FOR_EACH(key, i)
	{
		float t = i->time*tm_old/tm;
		if(t > 1.0f)
			t = 1.0f;

		i->time = t;
	}
}

////////////////////////////////////////////////

void CEmitterData::SetGenerationPointCount(int nPoints)
{
	if(data)
	{
		ResizeKey(nPoints, data->num_particle);
		ResizeKey(nPoints, data->life_time);
		ResizeKey(nPoints, data->life_time_delta);
		ResizeKey(nPoints, data->begin_size);
		ResizeKey(nPoints, data->begin_size_delta);
		ResizeKey(nPoints, data->emitter_scale);

		if(Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z)
			ResizeKey(nPoints, GetInt()->velocity_delta);
/*
		ClearPosRotationKey(data->emitter_position);
		ClearPosRotationKey(data->emitter_rotation);
	/// 
		for(int i=0;i<nPoints;++i)
		{
			float fTime = GenerationPointGlobalTime(i);
			bool bCreate;
			data->GetOrCreatePosKey(fTime, &bCreate);
		}
*/	/// 
	}
//	else
	{
		int oldSize = emitter_position().size();
		emitter_position().resize(nPoints);
		float t=0;
		float dt = 1.0/(nPoints-1);
		CKeyPos::iterator it;
		FOR_EACH(emitter_position(), it)
		{
			it->time  = t;
			if ((it-emitter_position().begin())>oldSize-1)
				it->pos.set(0,0,0);
			t+=dt;
		}
/*		ClearPosRotationKey(data_light->emitter_position);
		for(int i=0;i<nPoints;++i)
		{
			data_light->emitter_position.resize(nPoints);
			float fTime = GenerationPointGlobalTime(i);
			bool create;
			data_light->emitter_position.GetOrCreateKey(fTime,data_light->emitter_life_time,data_light->emitter_create_time,&create);		
		}*/
	}
	bDirty = true;
}
#define LINER(p1,p2,k) ((p1) + ((p2)-(p1))*(k))
/*void Liner(CKey& key,int ix,float k)
{
	ASSERT(ix>0);
	ASSERT(ix<key.size());
	key[ix].f = key[ix-1].f + (key[ix+1].f - key[ix-1].f)*k;
}
*/
void CEmitterData::InsertGenerationPoint(int nBefore, float tg)
{
	if(data)
	{
		InsertKey(nBefore, data->num_particle);
		InsertKey(nBefore, data->life_time);
		InsertKey(nBefore, data->life_time_delta);
		InsertKey(nBefore, data->begin_size);
		InsertKey(nBefore, data->begin_size_delta);
		InsertKey(nBefore, data->emitter_scale);

		data->num_particle[nBefore].f = LINER(data->num_particle[nBefore-1].f,data->num_particle[nBefore+1].f,tg);
		data->life_time[nBefore].f = LINER(data->life_time[nBefore-1].f,data->life_time[nBefore+1].f,tg);
		data->life_time_delta[nBefore].f = LINER(data->life_time_delta[nBefore-1].f,data->life_time_delta[nBefore+1].f,tg);
		data->begin_size[nBefore].f = LINER(data->begin_size[nBefore-1].f,data->begin_size[nBefore+1].f,tg);
		data->begin_size_delta[nBefore].f = LINER(data->begin_size_delta[nBefore-1].f,data->begin_size_delta[nBefore+1].f,tg);
		data->emitter_scale[nBefore].f = LINER(data->emitter_scale[nBefore-1].f,data->emitter_scale[nBefore+1].f,tg);

		if(Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z)
		{
			InsertKey(nBefore, GetInt()->velocity_delta);
			GetInt()->velocity_delta[nBefore].f = LINER(GetInt()->velocity_delta[nBefore-1].f,GetInt()->velocity_delta[nBefore+1].f,tg);
		}
	}
	emitter_position().insert(emitter_position().begin()+nBefore,emitter_position()[nBefore]);
	if(nBefore)
		emitter_position()[nBefore].pos = (emitter_position()[nBefore-1].pos + 
												emitter_position()[nBefore+1].pos)/2;
	else
		emitter_position()[nBefore].pos = Vect3f(0,0,0);

	bDirty = true;
}
void CEmitterData::SetGenerationPointTime(int nPoint, float tm)
{
//		float  tm_old = GenerationPointTime(nPoint);
//		SetPosRotationKeyTime(nPoint, tm, tm_old, emitter_pos());
	ASSERT((UINT)nPoint<emitter_position().size());
	emitter_position()[nPoint].time = tm;
		//SetPosRotationKeyTime(nPoint, tm, tm_old, rotation);
	if(data)
	{
		SetKeyTime(nPoint, tm, data->num_particle);
		SetKeyTime(nPoint, tm, data->life_time);
		SetKeyTime(nPoint, tm, data->life_time_delta);
		SetKeyTime(nPoint, tm, data->begin_size);
		SetKeyTime(nPoint, tm, data->begin_size_delta);
		SetKeyTime(nPoint, tm, data->emitter_scale);

		if(Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z)
			SetKeyTime(nPoint, tm, GetInt()->velocity_delta);
	}
	bDirty = true;
}

/// Calc spline point between two points
void CalcSPPos(CKeyPosHermit& p_pos, int before)
{
	float dt = (p_pos[before+1].time - p_pos[before-1].time);
	float dt1 = (p_pos[before].time - p_pos[before-1].time);
	float k = dt1/dt;

	float dx = (p_pos[before+1].pos.x - p_pos[before-1].pos.x);
	float dy = (p_pos[before+1].pos.y - p_pos[before-1].pos.y);
	float dz = (p_pos[before+1].pos.z - p_pos[before-1].pos.z);

	p_pos[before].pos.x = p_pos[before-1].pos.x + .5*dx;
	p_pos[before].pos.y = p_pos[before-1].pos.y + .5*dy;
	p_pos[before].pos.z = p_pos[before-1].pos.z + .5*dz;
}
///
/// Calc light positio between two positions
void CalcLightPos(CKeyPos& p_pos, int before)
{
	float dt = (p_pos[before+1].time - p_pos[before-1].time);
	float dt1 = (p_pos[before].time - p_pos[before-1].time);
	float k = dt1/dt;

	float dx = (p_pos[before+1].pos.x - p_pos[before-1].pos.x);
	float dy = (p_pos[before+1].pos.y - p_pos[before-1].pos.y);
	float dz = (p_pos[before+1].pos.z - p_pos[before-1].pos.z);

	p_pos[before].pos.x = p_pos[before-1].pos.x + .5*dx;
	p_pos[before].pos.y = p_pos[before-1].pos.y + .5*dy;
	p_pos[before].pos.z = p_pos[before-1].pos.z + .5*dz;
}
///

void CEmitterData::InsertParticleKey(int nBefore,float tg)
{
	switch(Class())
	{
	case EMC_SPLINE:
		InsertKey(nBefore, GetSpl()->p_position);
		CalcSPPos(GetSpl()->p_position, nBefore);
		break;

	case EMC_INTEGRAL:
	case EMC_INTEGRAL_Z:
		InsertKey(nBefore, GetInt()->p_velocity);
		InsertKey(nBefore, GetInt()->p_gravity);
		InsertKey(nBefore, GetInt()->begin_speed);

		GetInt()->p_velocity[nBefore].f = LINER(GetInt()->p_velocity[nBefore-1].f,GetInt()->p_velocity[nBefore+1].f,tg);
		GetInt()->p_gravity[nBefore].f = LINER(GetInt()->p_gravity[nBefore-1].f,GetInt()->p_gravity[nBefore+1].f,tg);
		GetInt()->begin_speed[nBefore].mul = LINER(GetInt()->begin_speed[nBefore-1].mul,GetInt()->begin_speed[nBefore+1].mul,tg);
		GetInt()->begin_speed[nBefore].velocity = EMV_INVARIABLY;
		break;

	case EMC_LIGHT:
//		InsertKey(nBefore, GetLight()->emitter_position);
//		CalcLightPos(GetLight()->emitter_position, nBefore);

		InsertKey(nBefore, GetLight()->emitter_size);
		GetLight()->emitter_size[nBefore].f = LINER(GetLight()->emitter_size[nBefore-1].f,GetLight()->emitter_size[nBefore+1].f,tg);

//		InsertKey(nBefore, GetLight()->emitter_color);
		break;
	case EMC_COLUMN_LIGHT:
		{
			CKey* keys = &emitter_size();
			InsertKey(nBefore, *keys);
			(*keys)[nBefore].f = LINER((*keys)[nBefore-1].f,(*keys)[nBefore+1].f,tg);

			keys = &emitter_size2();
			InsertKey(nBefore, *keys);
			(*keys)[nBefore].f = LINER((*keys)[nBefore-1].f,(*keys)[nBefore+1].f,tg);

			InsertKey(nBefore, height());
			height()[nBefore].f = LINER(height()[nBefore-1].f,height()[nBefore+1].f,tg);

			InsertKey(nBefore, u_vel());
			u_vel()[nBefore].f = LINER(u_vel()[nBefore-1].f, u_vel()[nBefore+1].f, tg);
			InsertKey(nBefore, v_vel());
			v_vel()[nBefore].f = LINER(v_vel()[nBefore-1].f, v_vel()[nBefore+1].f, tg);
			break;
		}
	}
	if(data){
		InsertKey(nBefore, data->p_size);
		InsertKey(nBefore, data->p_angle_velocity);

		data->p_size[nBefore].f = LINER(data->p_size[nBefore-1].f,data->p_size[nBefore+1].f,tg);
		data->p_angle_velocity[nBefore].f = LINER(data->p_angle_velocity[nBefore-1].f,data->p_angle_velocity[nBefore+1].f,tg);
	}

/*
	InsertKey(nBefore, data->p_size);
	InsertKey(nBefore, data->p_angle_velocity);
//	InsertKey(nBefore, key_dummy);

	if(Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z)
	{
		InsertKey(nBefore, GetInt()->p_velocity);
		InsertKey(nBefore, GetInt()->p_gravity);
		InsertKey(nBefore, GetInt()->begin_speed);
	}else{// Spline Emitter
		InsertKey(nBefore, GetSpl()->p_position);
		CalcSPPos(GetSpl()->p_position, nBefore);
	}
*/
	bDirty = true;
}
void CEmitterData::SetParticleKeyTime(int nKey, float tm)
{
	switch(Class())
	{
	case EMC_SPLINE:
		SetKeyTime(nKey, tm, GetSpl()->p_position);
		break;

	case EMC_INTEGRAL:
	case EMC_INTEGRAL_Z:
		SetKeyTime(nKey, tm, GetInt()->p_velocity);
		SetKeyTime(nKey, tm, GetInt()->p_gravity);
		SetKeyTime(nKey, tm, GetInt()->begin_speed);
		break;

	case EMC_LIGHT:
//		SetKeyTime(nKey, tm, GetLight()->emitter_position);
		SetKeyTime(nKey, tm, GetLight()->emitter_size);
//		SetKeyTime(nKey, tm, GetLight()->emitter_color);
		break;
	case EMC_COLUMN_LIGHT:
		SetKeyTime(nKey, tm, emitter_size());
		SetKeyTime(nKey, tm, emitter_size2());
		SetKeyTime(nKey, tm, height());
		SetKeyTime(nKey, tm, u_vel());
		SetKeyTime(nKey, tm, v_vel());
		break;
	}

	if(data){
		SetKeyTime(nKey, tm, data->p_size);
		SetKeyTime(nKey, tm, data->p_angle_velocity);
	}
/*
	SetKeyTime(nKey, tm, data->p_size);
	SetKeyTime(nKey, tm, data->p_angle_velocity);
//	SetKeyTime(nKey, tm, key_dummy);

	if(Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z)
	{
		SetKeyTime(nKey, tm, GetInt()->p_velocity);
		SetKeyTime(nKey, tm, GetInt()->p_gravity);
		SetKeyTime(nKey, tm, GetInt()->begin_speed);
	}else{// Spline Emitter
		SetKeyTime(nKey, tm, GetSpl()->p_position);
	}
*/
	bDirty = true;
}
void CEmitterData::DeleteGenerationPoint(int nPoint)
{
	if (data)
	{
		ASSERT(nPoint<data->emitter_position.size());
		data->emitter_position.erase(data->emitter_position.begin()+nPoint);
	//	float  tm_old = GenerationPointTime(nPoint);

	//	RemovePosRotationKey(tm_old, data->emitter_position);
		//RemovePosRotationKey(tm_old, rotation);

		RemoveKey(nPoint, data->num_particle);
		RemoveKey(nPoint, data->life_time);
		RemoveKey(nPoint, data->life_time_delta);
		RemoveKey(nPoint, data->begin_size);
		RemoveKey(nPoint, data->begin_size_delta);
		RemoveKey(nPoint, data->emitter_scale);

		if(Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z)
			RemoveKey(nPoint, GetInt()->velocity_delta);
	}else
	{
		ASSERT(nPoint<emitter()->emitter_position.size());
		emitter()->emitter_position.erase(emitter()->emitter_position.begin()+nPoint);
	}
	bDirty = true;
}
void CEmitterData::DeleteParticleKey(int nPoint)
{
	switch(Class())
	{
	case EMC_SPLINE:
		RemoveKey(nPoint, GetSpl()->p_position);
		break;

	case EMC_INTEGRAL:
	case EMC_INTEGRAL_Z:
		RemoveKey(nPoint, GetInt()->p_velocity);
		RemoveKey(nPoint, GetInt()->p_gravity);
		RemoveKey(nPoint, GetInt()->begin_speed);
		break;

	case EMC_LIGHT:
//		RemoveKey(nPoint, GetLight()->emitter_position);
		RemoveKey(nPoint, GetLight()->emitter_size);
//		RemoveKey(nPoint, GetLight()->emitter_color);
		break;
	case EMC_COLUMN_LIGHT:
		RemoveKey(nPoint, emitter_size());
		RemoveKey(nPoint, emitter_size2());
		RemoveKey(nPoint, height());
		RemoveKey(nPoint, u_vel());
		RemoveKey(nPoint, v_vel());
		break;
	}
	if(data){
		RemoveKey(nPoint, data->p_size);
		RemoveKey(nPoint, data->p_angle_velocity);
	}
/*
	RemoveKey(nPoint, data->p_size);
	RemoveKey(nPoint, data->p_angle_velocity);
//	RemoveKey(nPoint, key_dummy);

	if(Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z)
	{
		RemoveKey(nPoint, GetInt()->p_velocity);
		RemoveKey(nPoint, GetInt()->p_gravity);
		RemoveKey(nPoint, GetInt()->begin_speed);
	}else{
		RemoveKey(nPoint, GetSpl()->p_position);
	}
*/
	bDirty = true;
}
void CEmitterData::ChangeLifetime(float tm)
{
/*	CKeyPos& keypos = (data) ? data->emitter_position : 
							GetLight()->emitter_position;
	int nPoints = keypos.size();
	vector<float> vTimes;
	for(int i=0; i<nPoints; i++)
	{
		float f = keypos[i].time*EmitterLifeTime();
		if((f >=tm)&&(i<nPoints-1)){
			DeleteGenerationPoint(i);//f = tm;
			nPoints--;
			i--;
		}else
			vTimes.push_back(f);
	}

	switch(Class())
	{
	case EMC_SPLINE:
		break;

	case EMC_INTEGRAL:
	case EMC_INTEGRAL_Z:
		break;

	case EMC_LIGHT:
		SetKeyTimesPosRot(GetLight()->emitter_position, GetLight()->emitter_life_time, tm);
		SetKeyTimesPosRot(GetLight()->emitter_size, GetLight()->emitter_life_time, tm);
		SetKeyTimesPosRot(GetLight()->emitter_color, GetLight()->emitter_life_time, tm);
////		SetKeyTimes(GetLight()->emitter_position, vTimes, tm);
//		SetKeyTimes(GetLight()->emitter_size, vTimes, tm);
//		SetKeyTimes(GetLight()->emitter_color, vTimes, tm);
		break;
	}

	if(Class() != EMC_LIGHT){
		SetKeyTimesPosRot(data->emitter_position, data->emitter_life_time, tm);
		SetKeyTimesPosRot(data->emitter_rotation, data->emitter_life_time, tm);
	}
tt*/
	emitter()->emitter_life_time = tm;
/* tt
	if(Class() != EMC_LIGHT){
		SetKeyTimes(data->num_particle, vTimes, tm);
		SetKeyTimes(data->life_time, vTimes, tm);
		SetKeyTimes(data->life_time_delta, vTimes, tm);
		SetKeyTimes(data->begin_size, vTimes, tm);
		SetKeyTimes(data->begin_size_delta, vTimes, tm);
	}

	if(Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z)
		SetKeyTimes(GetInt()->velocity_delta, vTimes, tm);
*/
	bDirty = true;
}
void CEmitterData::ChangeGenerationLifetime(int nGeneration, float tm)
{
	ASSERT(data);
	ASSERT(nGeneration<data->life_time.size());
		data->life_time[nGeneration].f = tm;
/*	vector<float> vTimes;
	float fTime;

	if(data){
		fTime = data->life_time[nGeneration].f;

		int n = data->p_size.size();
		for(int i=0; i<n; i++)
		{
			float f = data->p_size[i].time*fTime;
			if(f > tm)
				f = tm;

			vTimes.push_back(f);
		}
		
		data->life_time[nGeneration].f = tm;

	}else{
		fTime = data_light->emitter_life_time;

		int n = data_light->emitter_size.size();
		for(int i=0; i<n; i++)
		{
			float f = GetLight()->emitter_size[i].time*fTime;
			if(f > tm)
				f = tm;

			vTimes.push_back(f);
		}

		data_light->emitter_life_time = tm;
	}

	switch(Class())
	{
	case EMC_SPLINE:
		SetKeyTimes(GetSpl()->p_position, vTimes, tm);
		break;

	case EMC_INTEGRAL:
	case EMC_INTEGRAL_Z:
		SetKeyTimes(GetInt()->p_velocity, vTimes, tm);
		SetKeyTimes(GetInt()->p_gravity, vTimes, tm);
		SetKeyTimes(GetInt()->begin_speed, vTimes, tm);
		break;

	case EMC_LIGHT:
//		SetKeyTimes(GetLight()->emitter_position, vTimes, tm);
		SetKeyTimes(GetLight()->emitter_size, vTimes, tm);
		SetKeyTimes(GetLight()->emitter_color, vTimes, tm);
		break;
	}

	if(Class() != EMC_LIGHT){
		SetKeyTimes(data->p_size, vTimes, tm);
		SetKeyTimes(data->p_angle_velocity, vTimes, tm);
	}

/*
	SetKeyTimes(data->p_size, vTimes, tm);
	SetKeyTimes(data->p_angle_velocity, vTimes, tm);
//	SetKeyTimes(key_dummy, vTimes, tm);

	if(Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z)
	{
		SetKeyTimes(GetInt()->p_velocity, vTimes, tm);
		SetKeyTimes(GetInt()->p_gravity, vTimes, tm);
		SetKeyTimes(GetInt()->begin_speed, vTimes, tm);
	}else{
		SetKeyTimes(GetSpl()->p_position, vTimes, tm);
	}
*/
	bDirty = true;
}
void CEmitterData::ChangeParticleLifeTime(float t)
{
	if (IsBase())
	{
		if (t<0.01f)
			t = 0.01f;
		float old = GetParticleLifeTime();
		float k = t/old;
		CKey &lf = GetBase()->life_time;
		CKey::iterator it;
		FOR_EACH(lf, it)
			it->f*= k;
	}
}

void CEmitterData::SetParticleLifeTime(float t)
{
	if (IsBase())
	{
		if (t<0.01f)
			t = 0.01f;
		CKey &lf = GetBase()->life_time;
		CKey::iterator it;
		FOR_EACH(lf, it)
			it->f = t;
	}
}

float CEmitterData::GetParticleLifeTime()
{
	if (IsBase())
		return GetBase()->life_time.front().f;
	return 0;
}

int CEmitterData::GetGenerationPointNum()
{
	if (IsBase())
		return emitter_position().size();
	return 2;
}
float CEmitterData::GetGenerationPointTime(int i)
{
	if (IsBase())
	{
		KeyPos p = emitter_position()[i];
		return emitter_create_time() + p.time*emitter_life_time();
	}
	return emitter_create_time() + (i==0 ? 0 : emitter_life_time());
}

void CEmitterData::DeletePositionKey(KeyPos* key)
{
	if(emitter()->emitter_position.size() == 1)
		return;
	CKeyPos::iterator i;
	FOR_EACH(emitter()->emitter_position, i)
		if(&*i == key)
		{
			emitter()->emitter_position.erase(i);
			break;
		}
	bDirty = true;
}

void CEmitterData::GetBeginSpeedMatrix(int nGenPoint, int nPoint, MatXf& mat)
{
	if(Class() == EMC_INTEGRAL || Class() == EMC_INTEGRAL_Z)
	{
		float tm = data->num_particle[nGenPoint].time;

		data->GetPosition(tm, mat);
		mat.rot() = GetInt()->begin_speed[nPoint].rotation;
	}

/*	BSKey::iterator i1 = begin_speed.begin();
	BSKey::iterator i2 = i1;
	while((*i2)->time_begin < tm)
	{
		i1 = i2; i2++;
	}
	ASSERT(i1 != begin_speed.end() && i2 != begin_speed.end());

	QuatF q;
	q.slerp((*i1)->rotation, (*i2)->rotation, (tm - (*i1)->time_begin)/((*i2)->time_begin - (*i1)->time_begin));
	pos.rot() = q;
*/
}
void CEmitterData::ChangeLightingCount(int n_count)
{
	if (n_count==0)
		n_count = 1;
	if (pos_end().size()>n_count)
	{
		int d = pos_end().size()-n_count;
		pos_end().erase(pos_end().end()-d, pos_end().end());
	}else 
	{
		int i = pos_end().size();
		pos_end().resize(n_count);
		Mat3f rot(M_PI/6, Z_AXIS);
		for(;i<pos_end().size();i++)
			pos_end()[i] = rot*(pos_end()[i-1]-pos_begin()) + pos_begin();
	}
}


extern void RemoveSplinePoint(CKeyPosHermit& spline, KeyPos *p);
void CEmitterData::Load(CLoadData* rd)
{
	emitter()->Load(rd);
	if(Class() == EMC_SPLINE){ // delete the rest points
//		CKeyPos::iterator i;
		for(int i=0; i<GetSpl()->p_position.size(); i++)
		{
			if(i>(GetSpl()->p_size.size()-1)){
				RemoveSplinePoint(GetSpl()->p_position, &GetSpl()->p_position[i]);
				i--;
			}
		}
	}
	SetSave(true);

	//
}

void CEmitterData::set_particle_key_time(int nGenPoint, int nParticleKey, float tm)
{
	if (IsBase())
	{
		tm -= num_particle()[nGenPoint].time*emitter_life_time();
		tm /= GenerationLifeTime(nGenPoint);
	}else
		tm /= emitter_life_time();

	if(tm > 1.0f) tm = 1.0f;
	if(tm < 0)	  tm = 0;
	SetParticleKeyTime(nParticleKey, tm);
}

void CEmitterData::change_particle_life_time(int nGenPoint, float tm)
{
	if (IsBase())
	{
		tm -= num_particle()[nGenPoint].time*emitter_life_time();
		if(tm > 100.0f)	tm = 100.0f;
		if(tm < 0.001f)	tm = 0.001f;
		ChangeGenerationLifetime(nGenPoint, tm);
	}
	else
	{
		if(tm > 100.0f)	tm = 100.0f;
		if(tm < 0.001f)	tm = 0.001f;
		emitter_life_time() = tm;
	}
}
float CEmitterData::get_particle_key_time(int nGenPoint, int nParticleKey)
{
	float f = 0;
	if (IsBase())
	{
		f = num_particle()[nGenPoint].time*emitter_life_time() + 
			GenerationLifeTime(nGenPoint)*p_size()[nParticleKey].time;
	}else
		f = emitter_life_time()*emitter_size()[nParticleKey].time;
	return f;
}

void CEmitterData::set_gen_point_time(int nPoint, float tm)
{
	float sc = emitter_life_time();
	sc = tm/sc;
	if(sc > 1.0f)
		sc = 1.0f;
	SetGenerationPointTime(nPoint, sc);
}


///////////////////////////////////////////////////////////////////////////
CEffectData::CEffectData()
{
	static int _unkn = 1;

	char _cb[100];
	sprintf(_cb, "effect_%d", _unkn++);

	name = _cb;

	bDirty = true;

	Show3Dmodel = true;
	
	bExpand = true;
	modelScale=effectScale = 1.0f;
	visibleGroup = 0;
	nChain =0;
	scaleEffectWithModel = true;
}
CEffectData::CEffectData(EffectKey* pk)
{
	name = pk->name;

	vector<EmitterKeyInterface*>::iterator i;
	FOR_EACH(pk->key, i)
		emitters.push_back(new CEmitterData((EmitterKeyInterface*)*i));

	bDirty = true;

	Show3Dmodel = true;

	bExpand = true;
	modelScale=effectScale = 1.0f;
	visibleGroup = 0;
	nChain = 0;
	scaleEffectWithModel = true;
}
CEffectData::CEffectData(CEffectData* efdat)
{
	Reset(efdat);
	modelScale=effectScale = 1.0f;
	visibleGroup = 0;
	nChain = 0;
	scaleEffectWithModel = true;
}


CEffectData::~CEffectData()
{
/*	int i=0;
	if (i==0)
		i=1;*/
	emitters.clear();
	key.clear(); 
/*	EmitterListType::iterator it;
	FOR_EACH(emitters, it)
		delete *it;
*/
}

void CEffectData::Reset(CEffectData* eff)
{
	xassert(eff);
	Clear();
	bDirty = eff->bDirty;
	Path3Dmodel = eff->Path3Dmodel;
	Show3Dmodel = eff->Show3Dmodel;
	modelScale = eff->modelScale;
	effectScale = eff->effectScale;

	bExpand = eff->bExpand;
	*(EffectKey*)this = *(EffectKey*)eff;
	
	emitters.resize(eff->emitters.size());
	for(int i=eff->emitters.size()-1;i>=0;i--)
		emitters[i] = new CEmitterData(eff->emitters[i]);
	*((UnicalID*)this) = *((UnicalID*)eff);
}

int CEffectData::EmitterIndex(CEmitterData* p)
{
	int ix = 0;
	EmitterListType::iterator it;
	FOR_EACH(emitters, it)
	{
		if (*it==p)
			return ix;
		ix++;
	}
	ASSERT(0&&"emitter not found");
	return -1;
}
CEmitterData* CEffectData::Emitter(int ix)
{
	ASSERT((UINT)ix<emitters.size());
	return emitters[ix];
}

void CEffectData::SetExpand(bool b)
{
	bExpand = b;
}


CEmitterData* CEffectData::add_emitter(CEmitterData* em)
{
	CEmitterData* pEmitter = new CEmitterData(em);
	CString name;
	int i=1;
	do
		name.Format("%s (%d)",pEmitter->name().c_str(), i++);
	while(CheckEmitterName(name));
	pEmitter->name() = name;
	emitters.push_back(pEmitter);
	bDirty = true;
	return pEmitter;
}
bool CEffectData::CheckEmitterName(const char* nm)
{
	EmitterListType::iterator it;
	FOR_EACH(emitters, it)
		if ((*it)->name()==nm)
			return true;
	return false;
}
CEmitterData* CEffectData::add_emitter(EMITTER_CLASS cls)
{
	CString name;
	int i=1;
	do
		name.Format("emitter %d", i++);
	while(CheckEmitterName(name));

	CEmitterData* pEmitter = new CEmitterData(cls);
	pEmitter->name() = name;
	emitters.push_back(pEmitter);
	bDirty = true;
	return pEmitter;
}

void CEffectData::del_emitter(CEmitterData* p)
{
	EmitterListType::iterator it = find(emitters.begin(), emitters.end(), p);
	ASSERT(it != emitters.end());

	bDirty = true;

	delete *it;
	emitters.erase(it);
}

void CEffectData::swap_emitters(CEmitterData* p1, CEmitterData* p2)
{
	EmitterListType::iterator it1 = find(emitters.begin(), emitters.end(), p1);
	EmitterListType::iterator it2 = find(emitters.begin(), emitters.end(), p2);
	if((it1!=emitters.end())&&(it2!=emitters.end()))
		swap(*it1, *it2);
	bDirty = true;
}
void CEffectData::move_emitters(CEmitterData* from, CEmitterData* to)
{
	EmitterListType::iterator it_from = find(emitters.begin(), emitters.end(), from);
	ASSERT(it_from != emitters.end());
	emitters.erase(it_from);
	EmitterListType::iterator it_to = find(emitters.begin(), emitters.end(), to);
	ASSERT(it_to != emitters.end());
	emitters.insert(it_to+1,from);
}

void CEffectData::insert_emitter(int i, CEmitterData* emitter)
{
	CEmitterData* pEmitter = new CEmitterData(emitter);
	emitters.insert(emitters.begin()+i, pEmitter);
	bDirty = true;
}

void CEffectData::prepare_effect_data(CEmitterData *pActiveEmitter, bool toExport)
{
	key.clear();

	EmitterListType::iterator it;
	FOR_EACH(emitters, it)
	{
		if(pActiveEmitter)
		{
			if(*it == pActiveEmitter)
			{
				key.push_back((*it)->emitter());
				(*it)->emitter()->BuildKey();
			}
		}
		else
		{
			if((*it)->IsActive()||toExport)
			{
				key.push_back((*it)->emitter());
				(*it)->emitter()->BuildKey();
			}
		}
		
	}
}

float CEffectData::GetTotalLifeTime()
{
	float f = 0;

	EmitterListType::iterator it;
	FOR_EACH(emitters, it)
	{
		CEmitterData* p = *it;

		if(p->EmitterEndTime() > f)
			f = p->EmitterEndTime();
		if (p->ParticleLifeTime()+p->emitter_create_time() > f)
			f = p->ParticleLifeTime()+p->emitter_create_time();
	}

	return f;
}
/*
bool CEffectData::CheckName(LPCTSTR lpsz, CEmitterData* p)
{
	EmitterListType::iterator it;
	FOR_EACH(emitters, it)
		if((*it) != p && (*it)->name() == lpsz)
				return false;
	return true;

}
*/
bool CEffectData::HasModelEmitter()
{
	EmitterListType::iterator it;
	FOR_EACH(emitters, it)
		if((*it)->IsBase() && (*it)->particle_position().type == EMP_3DMODEL)
			return true;
	return false;
}
void CEffectData::ModelScale(float sc)
{
	modelScale = sc;
	if (scaleEffectWithModel)
		effectScale = sc;
	if (_pDoc->ActiveEffect()==this && theApp.scene.m_pModel)
	{//Этот кусок кода такой же как в VistaEngine
		cObject3dx* p=theApp.scene.m_pModelLogic?theApp.scene.m_pModelLogic:theApp.scene.m_pModel;
		sBox6f boundBox;
		p->GetBoundBoxUnscaled(boundBox);
		float boundScale = (modelScale*one_size_model)/max(boundBox.max.z - boundBox.min.z, FLT_EPS);
		if (scaleEffectWithModel)
			effectScale=modelScale;
		theApp.scene.m_pModel->SetScale(boundScale);
		if (theApp.scene.m_pModelLogic)
			theApp.scene.m_pModelLogic->SetScale(boundScale);
	}
}
void CEffectData::RelativeScale(float sc)
{
	EffectKey::RelativeScale(sc);
}




	//EmitterKeyInterface
SpiralData& CEmitterData::spiral_data(){			return emitter()->spiral_data;}
string& CEmitterData::name(){				return emitter()->name;}
string& CEmitterData::texture_name()
{		
	return emitter()->texture_name;
}
string& CEmitterData::texture2_name(){		return GetColumnLight()->texture2_name;}

bool& CEmitterData::cycled(){				return emitter()->cycled;}
float& CEmitterData::emitter_create_time(){	return emitter()->emitter_create_time;}
float& CEmitterData::emitter_life_time(){	return emitter()->emitter_life_time;}
CKeyPos& CEmitterData::emitter_position(){	return emitter()->emitter_position;}
bool& CEmitterData::randomFrame() {return emitter()->randomFrame;}
//int& CEmitterData::texturesCount() {return emitter()->textureCount;}
//vector<string>& CEmitterData::textureNames() {return emitter()->textureNames;}

//other
CKey& CEmitterData::emitter_size()
{
	if (GetLight())			return GetLight()->emitter_size;
	if (GetColumnLight())	return GetColumnLight()->emitter_size;
	return GetBase()->begin_size;
}
CKeyColor& CEmitterData::emitter_color()
{
	if (GetLight())			return GetLight()->emitter_color;
	if (GetColumnLight())	return GetColumnLight()->emitter_color;
	return GetBase()->p_color;
}

//EmitterKeyBase
EMITTER_BLEND& CEmitterData::sprite_blend()
{
	if (IsCLight())
		return GetColumnLight()->sprite_blend;
	return GetBase()->sprite_blend; 
}
bool& CEmitterData::generate_prolonged(){							return GetBase()->generate_prolonged;}
float& CEmitterData::particle_life_time(){							return GetBase()->particle_life_time;}
EMITTER_TYPE_ROTATION_DIRECTION& CEmitterData::rotation_direction(){return GetBase()->rotation_direction;}
CKeyRotate& CEmitterData::emitter_rotation(){						return GetBase()->emitter_rotation;}
EmitterType& CEmitterData::particle_position(){						return GetBase()->particle_position;}
CKey& CEmitterData::life_time(){									return GetBase()->life_time;}

CKey& CEmitterData::life_time_delta(){								return GetBase()->life_time_delta;}
CKey& CEmitterData::begin_size(){									return GetBase()->begin_size;}
CKey& CEmitterData::begin_size_delta(){								return GetBase()->begin_size_delta;}
CKey& CEmitterData::emitter_scale(){								return GetBase()->emitter_scale;}
CKey& CEmitterData::num_particle(){									return GetBase()->num_particle;}
string& CEmitterData::other(){										return GetBase()->other;}
bool& CEmitterData::relative(){										return GetBase()->relative;}

bool& CEmitterData::chFill(){										return GetBase()->chFill;}
bool& CEmitterData::chPlume(){										return GetBase()->chPlume;}
int& CEmitterData::TraceCount(){									return GetBase()->TraceCount;}
float& CEmitterData::PlumeInterval(){								return GetBase()->PlumeInterval;}
bool& CEmitterData::smooth(){										return GetBase()->smooth;}

bool& CEmitterData::sizeByTexture(){								return GetBase()->sizeByTexture;}

bool& CEmitterData::velNoise(){										return GetBase()->velNoiseEnable;}
CKey& CEmitterData::velOctaves(){									return GetBase()->velOctavesAmplitude;}
float& CEmitterData::velFrequency(){								return GetBase()->velFrequency;}
float& CEmitterData::velAmplitude(){								return GetBase()->velAmplitude;}
bool& CEmitterData::velOnlyPositive(){								return GetBase()->velOnlyPositive;}
string& CEmitterData::velNoiseOther(){								return GetBase()->velNoiseOther;}
bool& CEmitterData::IsVelNoiseOther(){								return GetBase()->isVelNoiseOther;}

bool& CEmitterData::dirNoise(){										return GetBase()->dirNoiseEnable;}
CKey& CEmitterData::dirOctaves(){									return GetBase()->dirOctavesAmplitude;}
float& CEmitterData::dirFrequency(){								return GetBase()->dirFrequency;}
float& CEmitterData::dirAmplitude(){								return GetBase()->dirAmplitude;}
bool& CEmitterData::dirOnlyPositive(){								return GetBase()->dirOnlyPositive;}
string& CEmitterData::dirNoiseOther(){								return GetBase()->dirNoiseOther;}
bool& CEmitterData::IsDirNoiseOther(){								return GetBase()->isDirNoiseOther;}
bool& CEmitterData::BlockX(){										return GetBase()->noiseBlockX;}
bool& CEmitterData::BlockY(){										return GetBase()->noiseBlockY;}
bool& CEmitterData::BlockZ(){										return GetBase()->noiseBlockZ;}
bool& CEmitterData::noiseReplace(){									return GetBase()->noiseReplace;}
bool& CEmitterData::mirage(){										return GetBase()->mirage;}
bool& CEmitterData::softSmoke(){									return GetBase()->softSmoke;}

char& CEmitterData::draw_first_no_zbuffer(){						return GetBase()->draw_first_no_zbuffer;}

CVectVect3f& CEmitterData::begin_position(){						return GetBase()->begin_position;}
CVectVect3f& CEmitterData::normal_position(){						return GetBase()->normal_position;}

CKey&	   CEmitterData::p_size(){									return GetBase()->p_size;}
//CKeyColor& CEmitterData::p_color(){									return GetBase()->p_color;}
CKeyColor& CEmitterData::emitter_alpha()
{
	if (IsCLight())
		return GetColumnLight()->emitter_alpha;
	return GetBase()->p_alpha;
}
CKey& CEmitterData::p_angle_velocity(){						return GetBase()->p_angle_velocity;}

bool& CEmitterData::ignoreParticleRate() {	return GetBase()->ignoreParticleRate;}
float& CEmitterData::k_wind_min() {	return GetBase()->k_wind_min;}
float& CEmitterData::k_wind_max() {	return GetBase()->k_wind_max;}
bool& CEmitterData::need_wind(){	return GetBase()->need_wind;}
bool& CEmitterData::toObjects(){    return GetLight()->toObjects;}
bool& CEmitterData::toTerrain(){    return GetLight()->toTerrain;}
bool& CEmitterData::cone() {return GetBase()->cone;}
bool& CEmitterData::bottom() {return GetBase()->bottom;}

//EmitterKeyInt
bool& CEmitterData::use_light(){					return GetInt()->use_light;}
CKey&  CEmitterData::p_velocity(){					return GetInt()->p_velocity;}
CKey&  CEmitterData::p_gravity(){					return GetInt()->p_gravity;}

CKey& CEmitterData::velocity_delta(){				return GetInt()->velocity_delta;}

vector<EffectBeginSpeed>& CEmitterData::begin_speed(){	return GetInt()->begin_speed;}
vector<KeyParticleInt>& CEmitterData::keyInt(){			return GetInt()->GetKey();}

//EmitterKeyZ
float& CEmitterData::add_z(){				return GetZ()->add_z;}	
bool& CEmitterData::planar(){				return GetBase()->planar;}
bool& CEmitterData::oriented(){				return GetBase()->oriented;}
bool& CEmitterData::planar_turn(){			return GetBase()->turn;}
bool& CEmitterData::orientedCenter(){		return GetBase()->orientedCenter;}
bool& CEmitterData::orientedAxis(){		return GetBase()->orientedAxis;}
bool& CEmitterData::angle_by_center(){		return GetInt()->angle_by_center;}
float& CEmitterData::base_angle(){			return GetBase()->base_angle;}
bool& CEmitterData::use_force_field(){		return GetZ()->use_force_field;}
bool& CEmitterData::OnWater() {				return GetZ()->use_water_plane;}

//EmitterKeySpl
bool& CEmitterData::p_position_auto_time(){				return GetSpl()->p_position_auto_time;}
CKeyPosHermit&    CEmitterData::p_position(){			return GetSpl()->p_position;}
EMITTER_TYPE_DIRECTION_SPL& CEmitterData::direction(){	return GetSpl()->direction;}
vector<KeyParticleSpl>& CEmitterData::keySpl(){			return GetSpl()->GetKey();}



CKey& CEmitterData::u_vel(){	return GetColumnLight()->u_vel;}
CKey& CEmitterData::v_vel(){	return GetColumnLight()->v_vel;}
CKey& CEmitterData::height(){	return GetColumnLight()->height;}
EMITTER_BLEND& CEmitterData::color_mode(){return GetColumnLight()->color_mode;}
EMITTER_BLEND& CEmitterData::light_blend() 
{
	return GetLight()->light_blend;
}



//Lighting

float& CEmitterData::generate_time(){			return lighting->param.generate_time;}
float& CEmitterData::strip_width_begin(){		return lighting->param.strip_width_begin;}
float& CEmitterData::strip_width_time(){		return lighting->param.strip_width_time;}
float& CEmitterData::strip_length(){			return lighting->param.strip_length;}
float& CEmitterData::fade_time(){				return lighting->param.fade_time;}
float& CEmitterData::lighting_amplitude(){		return lighting->param.lighting_amplitude;}
Vect3f& CEmitterData::pos_begin(){				return lighting->pos_begin;}
vector<Vect3f>& CEmitterData::pos_end(){		return lighting->pos_end;}

//ColumnLight
CKey& CEmitterData::emitter_size2(){return column_light->emitter_size2;}
bool& CEmitterData::turn(){			return column_light->turn;}
bool& CEmitterData::plane(){		return column_light->plane;}
QuatF& CEmitterData::rot(){			return column_light->rot;}
bool& CEmitterData::laser(){		return column_light->laser;}

void CEffectData::Clear()
{
//	EmitterListType::iterator em;
//	FOR_EACH(emitters, em)
//		delete (*em);
	key.clear();
	emitters.clear();
}

void CEffectData::Serialize(CArchive& ar, const CString& folder, const CString& texture_folder, int version)
{
	int idummy=0;
	CString sdummy;
	int count;
	int Expand;
	if (ar.IsStoring())
	{
		CString cs_name = name.c_str();
		ar << cs_name;
		ar << Path3Dmodel << sdummy << sdummy;
		Expand = bExpand;
		ar << Expand << idummy << idummy;		
		// array
		count = emitters.size();
		ar << count;
		CString emitter_folder = folder + "\\" + name.c_str();
		mkdir(emitter_folder);
		for(int i=0; i<count; i++)
			emitters[i]->Serialize(ar, emitter_folder, texture_folder,version);
		ar << modelScale;
		ar << scaleEffectWithModel;
	}
	else
	{	// loading code

		CString cs_name;
		ar >> cs_name;
		name = cs_name;
		ar >> Path3Dmodel >> sdummy >> sdummy;
		ar >> Expand >> idummy >> idummy;
		bExpand = Expand;
		// array
		ar >> count;
		Clear();
		emitters.resize(count);
		CString emitter_folder = folder + "\\" + name.c_str();
		for(int i=0; i<count; i++)
		{
			emitters[i] = new CEmitterData();
			emitters[i]->Serialize(ar, emitter_folder, texture_folder, version);
		}
		switch(version)
		{
		case 1:
			{
				ar >> modelScale;
				break;
			}
		case 2:
			{
				ar >> modelScale;
				ar >> scaleEffectWithModel;
				break;
			}
		default:
			break;
		}
	}
}

void CEmitterData::Serialize(CArchive& ar, const CString& folder, const CString& texture_folder, int version)
{
	int idummy=0;
	CString sdummy;
	if (ar.IsStoring())
	{
		m_name = name().c_str();
		ar << m_name;
		ar << sdummy << sdummy << sdummy;
		ar << idummy << idummy << idummy;
		Save(folder+"\\"+m_name);

		CString s = texture_name().c_str();
		s = s.Right(s.GetLength()-s.ReverseFind('\\')-1);
		CString f1 = _pDoc->m_StorePathOld;
		if (!f1.IsEmpty())
			f1+="\\";
		f1+=texture_name().c_str();
		CString f2 = texture_folder+"\\"+s;
		CopyFile(f1, f2, false);

		for(int i=0; i<10; i++)
		{
			CString s = textureName(i).c_str();
			s = s.Right(s.GetLength()-s.ReverseFind('\\')-1);
			CString f1 = _pDoc->m_StorePathOld;
			if (!f1.IsEmpty())
				f1+="\\";
			f1+=textureName(i).c_str();
			CString f2 = texture_folder+"\\"+s;
			CopyFile(f1, f2, false);
		}

		if (IsCLight())
		{
			CString s = texture2_name().c_str();
			s = s.Right(s.GetLength()-s.ReverseFind('\\')-1);
			CString f1 = _pDoc->m_StorePathOld;
			if (!f1.IsEmpty())
				f1+="\\";
			f1+=texture2_name().c_str();
			CString f2 = texture_folder+"\\"+s;
			CopyFile(f1, f2, false);
		}
	}
	else
	{	// loading code
		ar >> m_name;
		ar >> sdummy >> sdummy >> sdummy;
		ar >> idummy >> idummy >> idummy;
		Load(folder+"\\"+m_name, texture_folder);
	}
}
/*
void CEmitterData::Save(CSaver &sv)
{
	string s1,s2; 
	if (!IsSave())
	{
		s1 = texture_name();
		s2 = texture2_name();
		string &tex1 = texture_name();
		if (!tex1.empty()) 
			tex1 = CString(FOLDER_TEXTURES) + _pDoc->CopySprite(tex1.c_str(), FOLDER_TEXTURES);
		if (IsCLight() && !texture2_name().empty())
			texture2_name() = FOLDER_TEXTURES + _pDoc->CopySprite(texture2_name().c_str(), FOLDER_TEXTURES);
	}
	emitter()->Save(sv);
	if (!IsSave())
	{
		texture_name() = s1;
		texture2_name() = s2;
	}
	SetDirty(false);
	SetSave(true);
}
EmitterKeyInterface* CEmitterData::prepareSave()
{
	if (!IsSave())
	{
//		s1 = texture_name();
//		s2 = texture2_name();
		string &tex1 = texture_name();
		if (!tex1.empty()) 
			tex1 = CString(FOLDER_TEXTURES) + _pDoc->CopySprite(tex1.c_str(), FOLDER_TEXTURES);
		if (IsCLight() && !texture2_name().empty())
			texture2_name() = FOLDER_TEXTURES + _pDoc->CopySprite(texture2_name().c_str(), FOLDER_TEXTURES);
	}
	return emitter();
}
*/
void CEmitterData::Save(LPCTSTR path)
{
	if (!IsSave())
	{
		for (int i=0; i<10; i++)
		{
			if (!textureName(i).empty())
				textureName(i) = FOLDER_SPRITE + _pDoc->CopySprite(textureName(i).c_str());

		}
		string &tex1 = texture_name();
		if (!tex1.empty()) 
			tex1 = FOLDER_SPRITE + _pDoc->CopySprite(tex1.c_str());
		if (IsCLight() && !texture2_name().empty())
			texture2_name() = FOLDER_SPRITE + _pDoc->CopySprite(texture2_name().c_str());
	}
	FileSaver sv(path);
	emitter()->Save(sv);
	SetDirty(false);
	SetSave(true);
}

void CEmitterData::Load(LPCTSTR path, LPCTSTR szFolderSprite)
{
	xassert(!m_name.IsEmpty());
	Clear();
	Init();
	CLoadDirectoryFile rd;

	if(rd.Load(path))
	{
		CLoadData* ID = rd.next();
		switch(ID->id)
		{
		case IDS_BUILDKEY_Z:
			data = new  EmitterKeyZ;
			break;
		case IDS_BUILDKEY_LIGHT:
			data_light = new EmitterKeyLight;
			break;
		case IDS_BUILDKEY_COLUMN_LIGHT:
			column_light = new EmitterKeyColumnLight;
			break;
		case IDS_BUILDKEY_SPL:
			data = new EmitterKeySpl;
			break;
		case IDS_BUILDKEY_INT:
			data = new EmitterKeyInt;
			break;
		case IDS_BUILDKEY_LIGHTING:
			lighting = new EmitterLightingKey;
			break;
		default:
			xassert(0);
			return;
		}

		emitter()->Load(ID);
		
		// Copy Sprite
		CString sprite_path(szFolderSprite);
		if(!sprite_path.IsEmpty()){
			if(GetFileAttributes(sprite_path) != FILE_ATTRIBUTE_DIRECTORY){
				sprite_path = sprite_path.Left(sprite_path.ReverseFind('\\'));
			}
			if (!texture_name().empty())
				texture_name() = FOLDER_SPRITE + _pDoc->CopyTexture((string)sprite_path, texture_name());
			for (int i=0; i<10; i++)
			{
				if (!textureName(i).empty())
					textureName(i) = FOLDER_SPRITE + _pDoc->CopyTexture((string)sprite_path, textureName(i));
			}
			if (IsCLight()&&!texture2_name().empty())
				texture2_name() = FOLDER_SPRITE + _pDoc->CopyTexture((string)sprite_path, texture2_name());
		}
		//

//		int n = 1;
//		char cb[255];
//		
//		strcpy(cb, name().c_str());
//		while(!pEffect->CheckName(cb, this))
//			sprintf(cb, "%s%d", p->name().c_str(), n++);
//		name() = cb;
		name() =m_name; 
	}
	else xassert(0 && "bad emitter path");
	SetSave(true);
}
/*
bool CEffectData::Load(LPCTSTR szFolder, LPCTSTR szFolderSprite)//empty emitters already make Serialize()
{
	EmitterListType::iterator em;
	CString folder = CString(szFolder) + "\\" + name.c_str();
	FOR_EACH(emitters, em)
		(*em)->Load(folder, szFolderSprite);
	return true;
/*
	Clear();
	if(GetFileAttributes(szFolder) == 0xFFFFFFFF)
		return false;
	CString sFolder(szFolder);
	int n = sFolder.ReverseFind('\\');
	name = sFolder.Right(sFolder.GetLength() - n - 1);
	sFolder += "\\*.";

	WIN32_FIND_DATA ff;
	HANDLE fh = ::FindFirstFile(sFolder, &ff);
	if(fh != INVALID_HANDLE_VALUE)
	{
		do
		{
			if((ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				CString s(szFolder);
				s += "\\";
				s += ff.cFileName;
				emitters.push_back(new CEmitterData());
				emitters.back()->Load(s, szFolderSprite);
			}
		}
		while(::FindNextFile(fh, &ff));
		::FindClose(fh);
	}
	return true;
* /
}*/
/*
void CEffectData::Save(LPCTSTR szFolder)
{
	CString effect_path = CString(szFolder) + "\\" + name.c_str();
//	RMDIR(effect_path);
	_mkdir(effect_path);

	EmitterListType::iterator i;
	FOR_EACH(emitters, i)
		(*i)->Save(effect_path);
	SetDirty(false);
}
*/

CGroupData::CGroupData()
{
//	id = cur_id++;
	m_bShow3DBack = true; 
	bExpand = true;
}
CGroupData::CGroupData(LPCTSTR name)
{ 
//	id = cur_id++; 
	m_name = name; 
	m_bShow3DBack = true; 
	bExpand = true;
}

void CGroupData::SetExpand(bool b)
{
	bExpand = b;
}

int CGroupData::EffectIndex(CEffectData* p)
{
	int ix = 0;
	EffectStorageType::iterator it;
	FOR_EACH(m_effects, it)
	{
		if (*it==p)
			return ix;
		ix++;
	}
	ASSERT(0&&"effect not found");
	return -1;
}

CEffectData* CGroupData::Effect(int ix)
{
	ASSERT((UINT)ix<m_effects.size());
	return m_effects[ix];
}
CEffectData* CGroupData::AddEffect(CEffectData* eff)
{
	if (!eff)
	{
		m_effects.push_back(new CEffectData());
		CString name;
		int i=1;
		do
			name.Format("Effect_%d", i++);
		while(!CheckEffectName(name));
		m_effects.back()->name = name;
	}
	else 
		m_effects.push_back(eff);
	return m_effects.back();
}

void CGroupData::DeleteEffect(CEffectData* eff)
{
	EffectStorageType::iterator it = find(m_effects.begin(), m_effects.end(), eff);
	ASSERT(it!=m_effects.end());
	m_effects.erase(it);//verify what delete *it
}



/*
void CGroupData::AddEffect(CEffectData* eff)//LPCTSTR name, LPCTSTR model_name, bool bExpand)
{
	CEffectData* pNewEffect;
	m_effects.push_back(pNewEffect = new CEffectData);
	m_effects.back()->name = name;
}
*/
void CGroupData::operator =(CGroupData& group)
{
	Clear();
	UnicalID::operator =(group);
	m_name			=group.m_name;
	m_Path3DBack	=group.m_Path3DBack;
	m_bShow3DBack	=group.m_bShow3DBack;
	bExpand			=group.bExpand;
	m_effects.resize(group.m_effects.size());
	for(int i=m_effects.size()-1; i>=0; i--)
		m_effects[i] = new CEffectData(group.m_effects[i]);
}
void RMDIR(LPCTSTR lpszDir)
{
	CString sFolder(lpszDir);
	sFolder += "*.*";

	WIN32_FIND_DATA ff;
	HANDLE fh = ::FindFirstFile(sFolder, &ff);
	if(fh != INVALID_HANDLE_VALUE)
	{
		do
		{
			if((ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				CString s(lpszDir);
				s += ff.cFileName;

				DeleteFile(s);
			}
		}
		while(::FindNextFile(fh, &ff));
		::FindClose(fh);
	}

	_rmdir(lpszDir);
}

void CGroupData::Serialize(CArchive& ar, const CString& path, int version)
{
	CString sdummy;
	int idummy=0;
	int count = 0;
	if (ar.IsStoring())
	{
		ar << m_name;
		ar << m_Path3DBack << sdummy << sdummy;
		ar << bExpand << idummy << idummy;
		count = m_effects.size();
		ar << count;
		CString group_path = path + "\\" + m_name;
		CString texture_path = path + "\\Sprite";
		RMDIR(group_path);
		mkdir(group_path);
		mkdir(texture_path);
		for(int i=0; i<count; i++)
			m_effects[i]->Serialize(ar, group_path, texture_path,version);
	}
	else
	{	// loading code
		ar >> m_name;
		ar >> m_Path3DBack >> sdummy >> sdummy;
		ar >> bExpand >> idummy >> idummy;
		ar >> count;
		Clear();
		m_effects.resize(count);
		CString group_path = path + "\\" + m_name;
		CString texture_path = path + "\\Sprite";
		for(int i=0; i<count; i++)
		{
			m_effects[i] = new CEffectData();
			m_effects[i]->Serialize(ar, group_path, texture_path,version);
		}
	}
}

void CGroupData::Clear()
{
//	EffectStorageType::iterator fx;
//	FOR_EACH(m_effects, fx)
//		(*fx)->Clear();
	m_effects.clear();
}

bool CGroupData::CheckEffectName(LPCTSTR lpsz)
{
	EffectStorageType::iterator i;
	FOR_EACH(m_effects, i)
		if((*i)->name == lpsz)
			return false;
	return true;
}
/*
bool CGroupData::Load(LPCTSTR szFolder) // empty effects must be from Serialize()
{
//	Clear();
	CString group_path = CString(szFolder) + "\\" + m_name;
	CString texture_path = CString(szFolder) + "\\Sprite";
	EffectStorageType::iterator fx;
	FOR_EACH(m_effects, fx)
		(*fx)->Load(group_path, texture_path);
	return true;
}

void CGroupData::Save(LPCTSTR szFolder)
{
	CString group_path = CString(szFolder) + "\\" + m_name;
	_mkdir(group_path);

	EffectStorageType::iterator it;
	FOR_EACH(m_effects, it)
		(*it)->Save(szFolder);
}
*/
