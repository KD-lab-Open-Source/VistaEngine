#include "StdAfxRD.h"
#include "NParticle.h"
#include "TexLibrary.h"
#include "scene.h"
#include "NParticleID.h"
#include "TileMap.h"
#include "VisGeneric.h"
#include "Serialization\Serialization.h"
#include "Serialization\GenericFileSelector.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\StringTableImpl.h"
#include "Serialization\SerializationFactory.h"
#include "Serialization\EnumDescriptor.h"
#include "FileUtils\FileUtils.h"
#include "CurveWrapper.h"

KeyFloat::value KeyFloat::none=0;
KeyPos::value KeyPos::none=Vect3f::ZERO;
KeyRotate::value KeyRotate::none=QuatF::ID;

BEGIN_ENUM_DESCRIPTOR(EMITTER_BLEND, "EMITTER_BLEND")
REGISTER_ENUM(EMITTER_BLEND_MULTIPLY, "Умножение")
REGISTER_ENUM(EMITTER_BLEND_ADD, "Сложение")
REGISTER_ENUM(EMITTER_BLEND_SUBSTRACT, "Вычитание")
END_ENUM_DESCRIPTOR(EMITTER_BLEND)

BEGIN_ENUM_DESCRIPTOR(EmitterZMode, "EmitterZMode")
REGISTER_ENUM(EMITTER_USE_ZBUFFER, "Использовать Z")
REGISTER_ENUM(EMITTER_DRAW_AFTER_GRASS, "Выше травы")
REGISTER_ENUM(EMITTER_DRAW_BEFORE_GRASS, "Ниже травы")
END_ENUM_DESCRIPTOR(EmitterZMode)

BEGIN_ENUM_DESCRIPTOR(EMITTER_TYPE_POSITION, "EMITTER_TYPE_POSITION")
REGISTER_ENUM(EMP_BOX, "Параллелепипед")
REGISTER_ENUM(EMP_CYLINDER, "Цилиндр")
REGISTER_ENUM(EMP_SPHERE, "Сфера")
REGISTER_ENUM(EMP_LINE, "Линия")
REGISTER_ENUM(EMP_RING, "Кольцо")
REGISTER_ENUM(EMP_3DMODEL, "По нормалям")
REGISTER_ENUM(EMP_3DMODEL_INSIDE, "Нормалями внутрь")
REGISTER_ENUM(EMP_OTHER_EMITTER, "По другому эмиттеру")
END_ENUM_DESCRIPTOR(EMITTER_TYPE_POSITION);

BEGIN_ENUM_DESCRIPTOR(EMITTER_TYPE_ROTATION_DIRECTION, "EMITTER_TYPE_ROTATION_DIRECTION")
REGISTER_ENUM(ETRD_CW, "ETRD_CW")
REGISTER_ENUM(ETRD_CCW, "ETRD_CCW")
REGISTER_ENUM(ETRD_RANDOM, "ETRD_RANDOM")
END_ENUM_DESCRIPTOR(EMITTER_TYPE_ROTATION_DIRECTION)

BEGIN_ENUM_DESCRIPTOR(EMITTER_TYPE_DIRECTION_SPLINE, "EMITTER_TYPE_DIRECTION_SPLINE")
REGISTER_ENUM(ETDS_ID, "Перемещение")
REGISTER_ENUM(ETDS_ROTATEZ, "Фигура вращения")
REGISTER_ENUM(ETDS_BURST1, "Фигура 1")
REGISTER_ENUM(ETDS_BURST2, "Фигура 2")
END_ENUM_DESCRIPTOR(EMITTER_TYPE_DIRECTION_SPLINE)

REGISTER_CLASS(EffectKey, EffectKey, "эффект");
WRAP_LIBRARY(EffectKeyTable, "EffectKeyTable", "Библиотека эффектов", "Scripts\\Content\\EffectKeyTable", 0, LIBRARY_EDITABLE);

REGISTER_CLASS(EmitterKeyInterface, EmitterKeyInt, "Базовый");
REGISTER_CLASS(EmitterKeyInterface, EmitterKeyZ, "Поверхность");
REGISTER_CLASS(EmitterKeyInterface, EmitterKeySpline, "Сплайн");
REGISTER_CLASS(EmitterKeyInterface, EmitterKeyLight, "Свет");
REGISTER_CLASS(EmitterKeyInterface, EmitterKeyColumnLight, "Столб света");
REGISTER_CLASS(EmitterKeyInterface, EmitterKeyLighting, "Молния");


void resizeKey(int points, KeysFloat& key)
{
	float val = key[0].f;

	key.resize(points);
	if(points > 1){
		key[points-1].time = 1;
		key[points-1].f = val;

		for(int i=1; i<points-1; i++){
			key[i].time = float(i) / (points - 1);
			key[i].f = val;
		}
	}
}

template<class T>
void setKeyTime(T& keys, int index, float time){
	if(index < keys.size())
		keys[index].time = time;
}

void insertKey(KeysPosHermit& keys, int before, float position)
{
	int count = keys.size();
	for(int i = 0; i < count; ++i){
		if(before == 0){
			KeyPos val = keys[i];
			float dt = (keys[i].time - keys[i-1].time);
			float dt1 = (keys[before].time - keys[i-1].time);
			float k = dt1/dt;

			float dx = (keys[i].pos.x - keys[i-1].pos.x);
			float dy = (keys[i].pos.y - keys[i-1].pos.y);
			float dz = (keys[i].pos.z - keys[i-1].pos.z);

			val.pos.x = keys[i-1].pos.x + .5*dx;
			val.pos.y = keys[i-1].pos.y + .5*dy;
			val.pos.z = keys[i-1].pos.z + .5*dz;
			keys.insert(keys.begin() + i, val);
			break;
		}
		before--;
	}
}

template <class T>
void removeKey(T& keys, int index)
{
	xassert(index < keys.size());
	keys.erase(keys.begin() + index);
}

void insertKey(EffectBeginSpeeds& keys, int before, float position)
{
	int count = keys.size();
	for(int i = 0; i < count; ++i){
		if(before == 0){
			EffectBeginSpeed val = keys[i];
			val.time = keys[i - 1].time + (keys[i].time - keys[i - 1].time) * position;
			val.mul = keys[i - 1].mul + (keys[i].mul - keys[i - 1].mul) * position;
			val.velocity = EMV_INVARIABLY;
			keys.insert(keys.begin() + i, val);
			break;
		}
		before--;
	}
}

void insertKey(KeysFloat& keys, int before, float position)
{
	int count = keys.size();
	for(int i = 0; i < count; ++i){
		if(before == 0){
			KeyFloat val = keys[i];
			val.time = keys[i - 1].time + (keys[i].time - keys[i - 1].time) * position;
			val.f = keys[i - 1].f + (keys[i].f - keys[i - 1].f) * position;
			keys.insert(keys.begin() + i, val);
			break;
		}
		before--;
	}
}

struct Amplitude{
	explicit Amplitude(float value = 1.0f)
	: value_(value)
	{}
	bool serialize(Archive& ar, const char* name, const char* nameAlt){
		return ar.serialize(RangedWrapperf(value_, 0.0f, 1.0f), name, nameAlt);
	}
	float& value(){ return value_; }

	float value_;
};

void NoiseParams::serialize(Archive& ar)
{
 	ar.serialize(onlyPositive, "onlyPositive", "Только положительные значения");
	ar.serialize(RangedWrapperf(amplitude, 0.001f, 100.0f), "amplitude", "Амплитуда");
	ar.serialize(RangedWrapperf(frequency, 0.001f, 100.0f), "frequency", "Частота");
	if(ar.isEdit()){
		std::vector<Amplitude> amplitudes;
		amplitudes.resize(octaveAmplitudes.size());
		for(int i = 0; i < octaveAmplitudes.size(); ++i)
			amplitudes[i] = Amplitude(octaveAmplitudes[i].f);
		ar.serialize(amplitudes, "octaveAmplitudes", "Амплитуды октав");
		if(ar.isInput()){
			octaveAmplitudes.resize(amplitudes.size());
			for(int i = 0; i < octaveAmplitudes.size(); ++i)
				octaveAmplitudes[i].f = amplitudes[i].value();
		}        
	}
	else
		ar.serialize(octaveAmplitudes, "octaveAmplitudes", "Амплитуды октав");
	if(octaveAmplitudes.empty())
		octaveAmplitudes.push_back(KeyFloat(0.5f, 0.5f));
}

void EmitterNoise::serialize(Archive& ar)
{
	ar.serialize(enabled, "enabled", "^");
	if(!ar.isEdit() || enabled){
		ar.serialize(fromOtherEmitter, "fromOtherEmitter", "Из другого эмиттера");
		if(!ar.isEdit() || fromOtherEmitter)
			ar.serialize(otherEmitterName, "otherEmitterName", "Эмиттер");
		if(!ar.isEdit() || !fromOtherEmitter)
			ar.serialize(params, "params", "Параметры");
	}
}

void EmitterNoiseBlockable::serialize(Archive& ar)
{
	__super::serialize(ar);
	if(!ar.isEdit() || enabled){
		ar.serialize(blockX, "blockX", "Блокировать по X");
		ar.serialize(blockY, "blockY", "Блокировать по Y");
		ar.serialize(blockZ, "blockZ", "Блокировать по Z");
		ar.serialize(replace, "replace", "Заменить");
	}
}

CurveCollector* CurveWrapperBase::currentCollector_ = 0;
CurveCollector::CurveCollector()
{
	xassert(CurveWrapperBase::currentCollector_ == 0);
	CurveWrapperBase::currentCollector_ = this;
}

CurveCollector::~CurveCollector()
{
	xassert(CurveWrapperBase::currentCollector_ == this);
	CurveWrapperBase::currentCollector_ = 0;
}

///////////////////////////EmitterColumnLight////////////////////////////////
EmitterKeyColumnLight::EmitterKeyColumnLight()
{
	color_mode = EMITTER_BLEND_MULTIPLY;
	sprite_blend = EMITTER_BLEND_MULTIPLY;
	turn = true;
	rot.set(Mat3f::ID);

	laser = false;
	discrete_laser = false;

	missileSpeed = 300.f;

	KeyPos pos;
	pos.pos.set(0,0,0);
	pos.time=0;
	emitter_position.push_back(pos);

	KeyFloat size;
	size.f=20;
	size.time=0;
	emitter_size.push_back(size);
	emitter_size2.push_back(size);
	length.push_back(size);
	height.push_back(size);

	size.f=20;
	size.time=1.0f;
	emitter_size.push_back(size);
	emitter_size2.push_back(size);
	length.push_back(size);
	height.push_back(size);


	KeyColor color;
	color.set(1,1,1,1);
	color.time=0;
	emitter_color.push_back(color);
	emitter_alpha.push_back(color);
	color.set(1,1,1,1);
	color.time=1.0f;
	emitter_color.push_back(color);
	emitter_alpha.push_back(color);

	u_vel.push_back(KeyFloat(0,0));
	u_vel.push_back(KeyFloat(1,0));

	v_vel.push_back(KeyFloat(0,0));
	v_vel.push_back(KeyFloat(1,0));
}

void EmitterKeyColumnLight::Save(Saver& s)
{
	s.push(IDS_BUILDKEY_COLUMN_LIGHT);
		s.push(IDS_BUILDKEY_COLUMN_LIGHT_HEAD);
			s<<emitter_create_time;
			s<<emitter_life_time;
			s<<cycled;
			s<<texture_name;
			s<<name;
			s<<sprite_blend;
			s<<color_mode;
			s<<texture2_name;
			UINT type = 0;
			if (plane)
			{
				if (turn)	type = CT_PLANE_ROTATE;
				else		type = CT_PLANE;
			}else			type = CT_TRUNC_CONE;
			s<<type;
			s<<laser;
			s<<discrete_laser;
			s<<missileSpeed;
			char mode = (char)zMode_;
			s<<mode;
			s<<ignoreDistanceCheck_;
		s.pop();
		height.Save(s, IDS_BUILDKEY_COLUMN_LIGHT_HEIGHT);
		s.push(IDS_BUILDKEY_COLUMN_LIGHT_ROT);
			s<<rot.x(); s<<rot.y(); s<<rot.z(); s<<rot.s();
		s.pop();
		u_vel.Save(s, IDS_BUILDKEY_COLUMN_LIGHT_U_VEL);
		v_vel.Save(s, IDS_BUILDKEY_COLUMN_LIGHT_V_VEL);
		emitter_position.Save(s,IDS_BUILDKEY_POSITION);
		emitter_size.Save(s,IDS_BUILDKEY_SIZE);
		emitter_size2.Save(s,IDS_BUILDKEY_SIZE2);
		emitter_color.Save(s,IDS_BUILDKEY_COLOR);
		emitter_alpha.Save(s,IDS_BUILDKEY_ALPHA);
		length.Save(s, IDS_BUILDKEY_COLUMN_LIGHT_LENGTH);
	s.pop();
	spiral_data.Save(s, IDS_BUILDKEY_TEMPLATE_DATA);
}

void EmitterKeyColumnLight::Load(CLoadDirectory rd)
{
	bool length_loaded = false;
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case IDS_BUILDKEY_COLUMN_LIGHT_HEAD:
		{
			CLoadIterator rd(ld);
			rd>>emitter_create_time;
			rd>>emitter_life_time;
			rd>>cycled;
			rd>>texture_name;
			rd>>name;
			int blend;
			rd>>blend;
			sprite_blend = (EMITTER_BLEND)blend;
			rd>>blend;
			color_mode = (EMITTER_BLEND)blend;
			rd>>texture2_name;
			UINT type;
			rd>>type;
			switch(type)
			{
			case CT_PLANE:			plane = true; turn = false;	break;
			case CT_PLANE_ROTATE:	plane = true; turn = true;	break;
			case CT_TRUNC_CONE:		plane = false; turn = false;break;
			};
			rd>>laser;
			rd>>discrete_laser;
			rd>>missileSpeed;
			char mode = EMITTER_USE_ZBUFFER;
			rd>>mode;
			zMode_ = EmitterZMode(mode);
			rd>>ignoreDistanceCheck_;
		}
		break;
	case IDS_BUILDKEY_TEMPLATE_DATA:
		spiral_data.Load(ld);
		break;
	case IDS_BUILDKEY_POSITION:
		emitter_position.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_SIZE:
		emitter_size.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_SIZE2:
		emitter_size2.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_COLOR:
		emitter_color.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_ALPHA:
		emitter_alpha.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_COLUMN_LIGHT_U_VEL:
		u_vel.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_COLUMN_LIGHT_V_VEL:
		v_vel.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_COLUMN_LIGHT_HEIGHT:
		height.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_COLUMN_LIGHT_LENGTH:
		length.Load(CLoadIterator(ld));
		length_loaded = true;
		break;
	case IDS_BUILDKEY_COLUMN_LIGHT_ROT:
		{
			CLoadIterator rd(ld);
			rd>>rot.x(); rd>>rot.y(); rd>>rot.z(); rd>>rot.s();
		}
		break;
	}

	if(!length_loaded){
		KeyFloat size;
		size.f=20;
		size.time=0;
		length.push_back(size);
		size.f=20;
		size.time=1.0f;
		length.push_back(size);
	}

	BuildKey();
}
void EmitterKeyColumnLight::RelativeScale(float scale)
{
	KeysPos::iterator ipos;
	FOR_EACH(emitter_position,ipos)
		ipos->pos*=scale;
	KeysFloat::iterator it;
	FOR_EACH(emitter_size,it)
		it->f*=scale;
	FOR_EACH(emitter_size2,it)
		it->f*=scale;
	FOR_EACH(height,it)
		it->f*=scale;
	FOR_EACH(length,it)
		it->f*=scale;
}
EmitterKeyInterface* EmitterKeyColumnLight::Clone()
{
	return new EmitterKeyColumnLight(*this);
}

void EmitterKeyColumnLight::BuildKey()
{
}

//////////////////////////////////////////////////////////////////////////////
void EmitterKeyLighting::Save(Saver& s)
{
	s.push(IDS_BUILDKEY_LIGHTING);
		s.push(IDS_BUILDKEY_LIGHTING_HEADER);
			s<<name;
			s<<texture_name;
			s<<emitter_create_time;
			s<<emitter_life_time;
			s<<cycled;
			char mode = (char)zMode_;
			s<<mode;
			s<<ignoreDistanceCheck_;
		s.pop();
		s.push(IDS_BUILDKEY_LIGHTING_PARAMETERS);
			s<<param.fade_time;
			s<<param.generate_time;
			s<<param.lighting_amplitude;
			s<<param.strip_length;
			s<<param.strip_width_begin;
			s<<param.strip_width_time;
		s.pop();
		s.push(IDS_BUILDKEY_LIGHTING_POSITIONS);
			s<<pos_begin;
			s<<pos_end.size();
			vector<Vect3f>::iterator it;
			FOR_EACH(pos_end, it)
				s<<(*it);
		s.pop();
		emitter_color.Save(s,IDS_BUILDKEY_COLOR);
	s.pop();
}

EmitterKeyLighting::EmitterKeyLighting()
{
	pos_begin.set(0,0,0);
	pos_end.push_back(Vect3f(100,0,0));

	KeyColor color;
	color.set(1,1,1,1);
	color.time=0;
	emitter_color.push_back(color);
	color.set(1,1,1,1);
	color.time=1.0f;
	emitter_color.push_back(color);
}

void EmitterKeyLighting::Load(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	{
		switch(ld->id)
		{
		case IDS_BUILDKEY_LIGHTING_HEADER:
			{
				CLoadIterator rd(ld);
				rd>>name;
				rd>>texture_name;
				rd>>emitter_create_time;
				rd>>emitter_life_time;
				rd>>cycled;
				char mode = EMITTER_USE_ZBUFFER;
				rd>>mode;
				zMode_ = EmitterZMode(mode);
				rd>>ignoreDistanceCheck_;
			}
			break;
		case IDS_BUILDKEY_LIGHTING_PARAMETERS:
			{
				CLoadIterator rd(ld);
				rd>>param.fade_time;
				rd>>param.generate_time;
				rd>>param.lighting_amplitude;
				rd>>param.strip_length;
				rd>>param.strip_width_begin;
				rd>>param.strip_width_time;
			}
			break;
		case IDS_BUILDKEY_LIGHTING_POSITIONS:
			{
				CLoadIterator rd(ld);
				rd>>pos_begin;
				UINT sz;
				rd>>sz;
				pos_end.resize(sz);
				for(int i=0;i<sz;i++)
					rd>>pos_end[i];
			}
			break;
		case IDS_BUILDKEY_COLOR:
			emitter_color.Load(CLoadIterator(ld));
			break;
		}
	}
}
void EmitterKeyLighting::RelativeScale(float scale)
{
	param.lighting_amplitude*=scale;
	param.strip_length*=scale;
	param.strip_width_begin*=scale;
	param.strip_width_time*=scale;
	pos_begin*=scale;
	vector<Vect3f>::iterator it;
	FOR_EACH(pos_end, it)
		(*it)*=scale;
}
EmitterKeyInterface* EmitterKeyLighting::Clone()
{
	EmitterKeyLighting* p= new EmitterKeyLighting;
	*p=*this;
	return p;
}
void EmitterKeyLighting::BuildKey()
{
}

void EmitterKeyLighting::serialize(Archive& ar)
{
	ar.serialize(param, "param", "param");
	ar.serialize(pos_begin, "pos_begin", "pos_begin");
	ar.serialize(pos_end, "pos_end", "pos_end");
	ar.serialize(emitter_color, "emitter_color", 0);
}

////////////////////////cEmitterSpline//////////////////////

void HermitCoef(float p0,float p1,float p2,float p3,float& t0,float& t1,float& t2,float& t3)
{
	t3=(1.5f*(p1-p2)+(-p0+p3)*0.5f);
	t2=(2.0f*p2+p0-p3*0.5f-2.5f*p1);
	t1=(p2-p0)*0.5f;
	t0=p1;
}

void HermitCoef(const Vect3f& p0,const Vect3f& p1,const Vect3f& p2,const Vect3f& p3,
				Vect3f& t0,Vect3f& t1,Vect3f& t2,Vect3f& t3)
{
	HermitCoef(p0.x,p1.x,p2.x,p3.x, t0.x,t1.x,t2.x,t3.x);
	HermitCoef(p0.y,p1.y,p2.y,p3.y, t0.y,t1.y,t2.y,t3.y);
	HermitCoef(p0.z,p1.z,p2.z,p3.z, t0.z,t1.z,t2.z,t3.z);
}

KeysPosHermit::KeysPosHermit()
{
	//cbegin=cend=T_CLOSE;
	cbegin=cend=T_FREE;
}

Vect3f KeysPosHermit::Clamp(int i)
{
	xassert(size()>=2);
	if(i<0)
	{
		switch(cbegin)
		{
		case T_CLOSE:
			return front().pos;
		case T_FREE:
			{
				Vect3f p0=front().pos,p1=(*this)[1].pos;
				return p0-(p1-p0);
			}
		case T_CYCLE:
			return back().pos;
		}
	}

	if(i>=size())
	{
		switch(cend)
		{
		case T_CLOSE:
			return back().pos;
		case T_FREE:
			{
				Vect3f p0=back().pos,p1=(*this)[size()-2].pos;
				return p0-(p1-p0);
			}
		case T_CYCLE:
			return front().pos;
		}
	}

	return (*this)[i].pos;
}

Vect3f KeysPosHermit::Get(float t)
{
	if(empty())return KeyPos::none;
	if(size()==1)return (*this)[0].Val();

	if(t<(*this)[0].time)
		return (*this)[0].Val();

	for(int i=1;i<size();i++)
	if(t<(*this)[i].time)
	{
		Vect3f fm=Clamp(i-2);
		KeyPos& f0=(*this)[i-1];
		KeyPos& f1=(*this)[i];
		Vect3f fp=Clamp(i+1);
		float dx=f1.time-f0.time;
		xassert(dx>0);
		xassert(t>=f0.time);
		float tx=(t-f0.time)/max(dx,1e-3f);

		return HermitSpline(tx,fm,f0.pos,f1.pos,fp);
	}

	return back().Val();
}

/////////////////////misc///////////////////////
void CVectVect3f::Save(Saver& s,int id)
{
	s.push(id);
	s<<(int)size();
	iterator it;
	FOR_EACH(*this,it)
		s<<*it;
	s.pop();
}
void CVectVect3f::Load(CLoadIterator& rd)
{
	int sz;
	rd>>sz;
	resize(sz);
	iterator it;
	FOR_EACH(*this,it)
		rd>>*it;
}

void KeysFloat::Save(Saver& s,int id)
{
	s.push(id);
	s<<(int)size();
	KeysFloat::iterator it;
	FOR_EACH(*this,it)
	{
		s<<it->time;
		s<<it->f;
	}
	s.pop();
}

void KeysFloat::Load(CLoadIterator& rd)
{
	int sz;
	rd>>sz;
	resize(sz);
	for(int i=0;i<sz;i++)
	{
		KeyFloat& p=(*this)[i];
		rd>>p.time;
		rd>>p.f;
	}
}

void EmitterType::Save(Saver& s,int id)
{
	s.push(id);
	s<<(int)type;
	s<<size;
	s<<alpha_min;
	s<<alpha_max;
	s<<teta_min;
	s<<teta_max;

	s<<fix_pos;
	s<<num.x;
	s<<num.y;
	s<<num.z;
	s.pop();
}

void EmitterType::Load(CLoadIterator& rd)
{
	int itmp;
	rd>>itmp;type=(EMITTER_TYPE_POSITION)itmp;
	rd>>size;
	rd>>alpha_min;
	rd>>alpha_max;
	rd>>teta_min;
	rd>>teta_max;

	rd>>fix_pos;
	rd>>num.x;
	rd>>num.y;
	rd>>num.z;
}

void EmitterType::CountXYZ::serialize(Archive& ar)
{
	ar.serialize(x, "x", "x");
	ar.serialize(y, "y", "y");
	ar.serialize(z, "z", "z");
}

void EmitterType::serialize(Archive& ar)
{
	ar.serialize(type, "type", "Распределение");

	//ar.serialize(size, "size", "Размер");
	ar.serialize(size.x, "sizeX", "X");
	if(type == EMP_BOX){
		ar.serialize(size.y, "sizeY", "Y");
		ar.serialize(size.z, "sizeZ", "Z");
	}
	// admix: не хочу делить все на ветки по ar.isEdit() т.к.
	// в одну из них обязательно будут забывать что-нить добавить
	if(!ar.isEdit() || type == EMP_RING){ 
		ar.serialize(alpha_min, "alpha_min", "Alpha min");
		ar.serialize(alpha_max, "alpha_max", "Alpha max");
		ar.serialize(teta_min, "teta_min", "Teta min");
		ar.serialize(teta_max, "teta_max", "Teta max");
	}
	ar.serialize(fix_pos, "fix_pos", "Фиксировать");
	// ar.serialize(num, "num", "num");
	if(!ar.isEdit() || fix_pos){
		ar.serialize(num.x, "numX", "dx");
		ar.serialize(num.y, "numY", "dy");
		if(!ar.isEdit() || type == EMP_BOX)
			ar.serialize(num.y, "numZ", "dz");
	}
}

void EffectBeginSpeed::Save(Saver& s)
{
	s.push(IDS_BUILDKEY_BEGIN_SPEED);
	s<<name;
	s<<(DWORD)velocity;
	s<<mul;
	s<<rotation.s();
	s<<rotation.x();
	s<<rotation.y();
	s<<rotation.z();
	s<<time;
	s<<esp.pos;
	s<<esp.dpos;
	s.pop();
}

void EffectBeginSpeed::Load(CLoadIterator& rd)
{
	DWORD itemp;
	rd>>name;
	rd>>itemp;velocity=(EMITTER_TYPE_VELOCITY)itemp;
	rd>>mul;
	rd>>rotation.s();
	rd>>rotation.x();
	rd>>rotation.y();
	rd>>rotation.z();
	rd>>time;
	rd>>esp.pos;
	rd>>esp.dpos;
}


bool EffectBeginSpeeds::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	if(ar.isEdit()){
		if(ar.openStruct(*this, name, nameAlt)){
			ar.serialize(static_cast<vector<EffectBeginSpeed>&>(*this), "keys", "Keys");
			ar.closeStruct(name);
			return true;
		}
		else
			return false;
	}
	else
		return ar.serialize(static_cast<vector<EffectBeginSpeed>&>(*this), name, nameAlt);
}

void KeysRotate::Save(Saver& s,int id)
{
	s.push(id);
	s<<(int)size();
	iterator it;
	FOR_EACH(*this,it)
	{
		s<<it->time;
		s<<it->pos.s();
		s<<it->pos.x();
		s<<it->pos.y();
		s<<it->pos.z();
	}
	s.pop();
}

void KeysRotate::Load(CLoadIterator& rd)
{
	int sz;
	rd>>sz;
	resize(sz);
	for(int i=0;i<sz;i++)
	{
		KeyRotate& p=(*this)[i];
		rd>>p.time;
		rd>>p.pos.s();
		rd>>p.pos.x();
		rd>>p.pos.y();
		rd>>p.pos.z();
	}
}

void KeysPos::SaveInternal(Saver& s)
{
	s<<(int)size();
	iterator it;
	FOR_EACH(*this,it)
	{
		s<<it->time;
		s<<it->pos.x;
		s<<it->pos.y;
		s<<it->pos.z;
	}
}

void KeysPos::Save(Saver& s,int id)
{
	s.push(id);
	SaveInternal(s);
	s.pop();
}

void KeysPos::Load(CLoadIterator& rd)
{
	int sz;
	rd>>sz;
	resize(sz);
	for(int i=0;i<sz;i++)
	{
		KeyPos& p=(*this)[i];
		rd>>p.time;
		rd>>p.pos.x;
		rd>>p.pos.y;
		rd>>p.pos.z;
	}
}

void KeysPosHermit::Save(Saver& s,int id)
{
	s.push(id);
	KeysPos::SaveInternal(s);
	s<<(BYTE)cbegin;
	s<<(BYTE)cend;
	s.pop();
}

void KeysPosHermit::Load(CLoadIterator& rd)
{
	KeysPos::Load(rd);
	BYTE b=T_FREE;
	rd>>b;
	cbegin=(CLOSING)b;
	rd>>b;
	cend=(CLOSING)b;
}

///////////////////////KeysColor///////////////////////////////

void KeysColor::Save(Saver& s,int id)
{
	s.push(id);
	s<<(int)size();
	KeysColor::iterator it;
	FOR_EACH(*this,it)
	{
		s<<it->time;
		s<<it->r;
		s<<it->g;
		s<<it->b;
		s<<it->a;
	}
	s.pop();
}

void KeysColor::Load(CLoadIterator& rd)
{
	int sz;
	rd>>sz;
	resize(sz);
	for(int i=0;i<sz;i++)
	{
		KeyColor& p=(*this)[i];
		rd>>p.time;
		rd>>p.r;
		rd>>p.g;
		rd>>p.b;
		rd>>p.a;
	}
}

///////////////////////EmitterKeySpline//////////////////
EmitterKeySpline::EmitterKeySpline()
{
	emitter_life_time=particle_life_time=2.0f;
	cycled=true;
	generate_prolonged=true;
	begin_size[0]=KeyFloat(0,0.2f);
	num_particle[0]=KeyFloat(0,1000);

	direction=ETDS_ROTATEZ;//ETDS_ID;//
	p_position_auto_time=false;
	KeyPos pos0;
	pos0.time=0;

	pos0.pos.set(0,0,0);
	p_position.push_back(pos0);

	pos0.pos.set(0,0,100);
	pos0.time = 1.0;
	p_position.push_back(pos0);

	num_particle[0].f = 10;
	HermitKey hkey;
	hkeys.push_back(hkey);
	hkeys.push_back(hkey);

    KeyParticleSpl keyParticleSpline;
	key.push_back(keyParticleSpline);
	key.push_back(keyParticleSpline);
}

EmitterKeySpline::~EmitterKeySpline()
{
}

void EmitterKeySpline::Save(Saver& s)
{
	s.push(IDS_BUILDKEY_SPL);
		EmitterKeyBase::SaveInternal(s);

		s.push(IDS_BUILDKEY_SPL_HEADER);
		s<<p_position_auto_time;
		s.pop();

		s.push(IDS_BUILDKEY_SPL_DIRECTION);
		s<<(DWORD)direction;
		s.pop();

		p_position.Save(s,IDS_BUILDKEY_SPL_POSITION);
	s.pop();

}

void EmitterKeySpline::Load(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	{
		EmitterKeyBase::LoadInternal(ld);
		switch(ld->id)
		{
		case IDS_BUILDKEY_SPL_HEADER:
			{
				CLoadIterator rd(ld);
				rd>>p_position_auto_time;
			}
			break;
		case IDS_BUILDKEY_SPL_DIRECTION:
			{
				CLoadIterator rd(ld);
				DWORD d;
				rd>>d;
				direction=(EMITTER_TYPE_DIRECTION_SPLINE)d;
			}
			break;
		case IDS_BUILDKEY_SPL_POSITION:
			p_position.Load(CLoadIterator(ld));
			break;
		}
	}

	BuildKey();
}

void EmitterKeySpline::BuildKey()
{
	vector<float> xtime;
	xtime.push_back(0);
	xtime.push_back(1);
	add_sort(xtime,p_size);
	add_sort(xtime,p_color);
	add_sort(xtime,p_alpha);
	add_sort(xtime,p_angle_velocity);
	end_sort(xtime);

	key.clear();
	KeyParticleSpl k;
	k.angle_vel=0;
	k.color.set(1,1,1,1);

	for(int i=0;i<xtime.size();i++)
	{
		if(i<xtime.size()-1)
		{
			k.dtime=xtime[i+1]-xtime[i];
		}else
		{
			k.dtime=0;
		}

		float t=xtime[i];
		k.size=p_size.Get(t)*20;
		k.color=p_color.Get(t);
		k.color.a=p_alpha.Get(t).a;
		k.angle_vel=p_angle_velocity.Get(t)*5;

		k.inv_dtime = 1 / max(k.dtime, 0.001f);
		k.color.r *= 255.0f;
		k.color.g *= 255.0f;
		k.color.b *= 255.0f;
		k.color.a *= 255.0f;

		key.push_back(k);
	}


	if(p_position.size() == 0)
		return;

	hkeys.resize(p_position.size() - 1);
	float dt = 1 / (float)(hkeys.size());

	for(int i = 0; i < hkeys.size(); i++)
	{
		HermitKey & hk = hkeys[i];
		if(p_position_auto_time)
			hk.dtime = dt;
		else
			hk.dtime = p_position[i + 1].time - p_position[i].time;

		hk.inv_dtime = 1 / max(hk.dtime, 1e-20f);
		Vect3f p0 = p_position.Clamp(i - 1);
		Vect3f p1 = p_position.Clamp(i);
		Vect3f p2 = p_position.Clamp(i + 1);
		Vect3f p3 = p_position.Clamp(i + 2);

		HermitCoef(p0, p1, p2, p3, hk.H0, hk.H1, hk.H2, hk.H3);

#ifdef EMITTER_TCONTROL
		hk.t0 = 0;
		hk.t1 = 1;
		hk.t2 = hk.t3 =0;
#endif
	}

	if(!p_position_auto_time)
		return;

	vector<float> klen;
	float sum = 0;
	klen.resize(hkeys.size());

	for(int i = 0; i < hkeys.size(); i++)
	{
		HermitKey & hk = hkeys[i];
		Vect3f p0, p1;
		hk.Get(p0, 0);

		const int mx = 16;
		float len = 0, dt = 1 / (float)mx;

		for(int j = 0; j < mx; j++)
		{
			float t = (j + 1) * dt;
			hk.Get(p1, t);
			len += p0.distance(p1);
			p0 = p1;
		}

		klen[i] = len;
		sum += len;
	}

#ifdef EMITTER_TCONTROL

	for(int i = 0; i < hkeys.size(); i++)
	{
		HermitKey & hk = hkeys[i];
		float d0, d1, d2;
		hk.dtime = klen[i] / sum;
		hk.inv_dtime = 1 / max(hk.dtime, 1e-20f);

		d0= klen[(i - 1 > 0) ? i-1 : 0] / sum;
		d1 = klen[i] / sum;
		d2 = klen[(i + 1 < hkeys.size()) ? (i + 1) : (hkeys.size() - 1)] / sum;

		float p0 = -d0 / d1;
		float p1 = 0;
		float p2 = 1;
		float p3 = (d1 + d2) / d1;

		HermitCoef(p0, p1, p2, p3, hk.t0, hk.t1, hk.t2, hk.t3);
		//hk.t0=0;hk.t1=1;hk.t2=hk.t3=0;
	}

#else

	for(int i = 0; i < hkeys.size(); i++)
	{
		HermitKey & hk = hkeys[i];
		hk.dtime = klen[i] / sum;
		hk.inv_dtime = 1 / max(hk.dtime, 1e-20f);
	}

#endif

}
/////////////////////////////EmitterKeyInterface/////////////////////////
EmitterKeyInterface::EmitterKeyInterface()
{
	texture_name="";
	emitter_create_time=0;
	emitter_life_time=1;
	cycled=false;
	randomFrame = false;

	zMode_ = EMITTER_USE_ZBUFFER;
	ignoreDistanceCheck_ = false;

	//blend_mode = ALPHA_BLEND;
}

float EmitterKeyInterface::LocalTime(float t)
{
	return (emitter_life_time>KeyBase::time_delta)?(t-emitter_create_time)/emitter_life_time:0;
}

float EmitterKeyInterface::GlobalTime(float t)
{
	return t*emitter_life_time+emitter_create_time;
}

void EmitterKeyInterface::serialize(Archive& ar)
{
	if(ar.openBlock("common", "Общие")){
		ar.serialize(randomFrame, "randomFrame", "Случайный кадр");
		ar.serialize(zMode_, "zMode", "Сквозь землю");

		// жуть
		static ComboStrings textureName;
		static ComboStrings textureNameAlt;
		if(textureName.empty()){
			textureName.resize(num_texture_names);
			textureNameAlt.resize(num_texture_names);
			for(int i = 0; i < num_texture_names; ++i){
				XBuffer buf;
				buf <= i;
				textureName[i] = "texture";
				textureName[i] += buf;
				textureNameAlt[i] = "Текстура ";
				textureNameAlt[i] += buf;
			}
		}

		static GenericFileSelector::Options options("*.tga", "");
		if(ar.isEdit() && !randomFrame){
			ar.serialize(GenericFileSelector(textureNames[0], options), "texture", "Текстура");
		}
		else{
			for(int i = 0; i < num_texture_names; ++i){
				ar.serialize(GenericFileSelector(textureNames[i], options), textureName[i].c_str(), textureNameAlt[i].c_str());
			}
		}
		ar.closeBlock();
	}
	//ar.serialize() Текстура

}

/////////////////////////////EmitterKeyLight/////////////////////////
EmitterKeyLight::EmitterKeyLight()
{
	toObjects = true;
	toTerrain = true;
	light_blend = EMITTER_BLEND_ADD;

	KeyPos pos;
	pos.pos.set(0,0,0);
	pos.time=0;
	emitter_position.push_back(pos);

	KeyFloat size;
	size.f=20;
	size.time=0;
	emitter_size.push_back(size);
	size.f=20;
	size.time=1.0f;
	emitter_size.push_back(size);

	KeyColor color;
	color.set(1,1,1,1);
	color.time=0;
	emitter_color.push_back(color);
	color.set(1,1,1,1);
	color.time=1.0f;
	emitter_color.push_back(color);
}


void EmitterKeyLight::Save(Saver& s)
{
	s.push(IDS_BUILDKEY_LIGHT);
		s.push(IDS_BUILDKEY_LIGHT_HEAD);
		s<<emitter_create_time;
		s<<emitter_life_time;
		s<<cycled;
		s<<texture_name;
		s<<name;
		s<<toObjects;
		s<<light_blend;
		s<<toTerrain;
		s<<ignoreDistanceCheck_;
		s.pop();

		emitter_position.Save(s,IDS_BUILDKEY_POSITION);
		emitter_size.Save(s,IDS_BUILDKEY_SIZE);
		emitter_color.Save(s,IDS_BUILDKEY_COLOR);
	s.pop();
	spiral_data.Save(s, IDS_BUILDKEY_TEMPLATE_DATA);
}

void EmitterKeyLight::Load(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case IDS_BUILDKEY_LIGHT_HEAD:
		{
			CLoadIterator rd(ld);
			rd>>emitter_create_time;
			rd>>emitter_life_time;
			rd>>cycled;
			rd>>texture_name;
			rd>>name;
			rd>>toObjects;
			int blend = 1;
			rd>>blend;
			if (blend > 2 || blend < 1)
				blend = 1;
			light_blend = (EMITTER_BLEND)blend;
			rd>>toTerrain;
			rd>>ignoreDistanceCheck_;
		}
		break;
	case IDS_BUILDKEY_TEMPLATE_DATA:
		spiral_data.Load(ld);
		break;
	case IDS_BUILDKEY_POSITION:
		emitter_position.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_SIZE:
		emitter_size.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_COLOR:
		emitter_color.Load(CLoadIterator(ld));
		break;
	}

	BuildKey();
}

void EmitterKeyLight::BuildKey()
{
}

void EmitterKeyLight::serialize(Archive& ar)
{
	ar.serialize(wrapCurve(emitter_size, true, 1.0f), "emitter_size", "Размер");
	ar.serialize(emitter_color, "emitter_color", 0);
	// свет
	ar.serialize(toObjects, "toObjects", "На объекты");
	ar.serialize(toTerrain, "toTerrain", "На Землю");
	ar.serialize(light_blend, "light_blend", "Blending");
}


void EmitterKeyLight::insertParticleKey(int before, float position)
{
	__super::insertParticleKey(before, position);
	insertKey(emitter_size, before, position);
}

void EmitterKeyLight::setParticleKeyTime(int keyIndex, float time)
{
	__super::setParticleKeyTime(keyIndex, time);
	setKeyTime(emitter_size, keyIndex, time);
}

void EmitterKeyLight::deleteParticleKey(int index)
{
	__super::deleteParticleKey(index);
	removeKey(emitter_size, index);
}


/////////////////////////////EmitterKeyBase/////////////////////////
EmitterKeyBase::EmitterKeyBase()
: preciseBound_(false)
{
	sprite_blend=EMITTER_BLEND_MULTIPLY;
	generate_prolonged=false;
	particle_life_time=1;

	rotation_direction=ETRD_CW;

	KeyPos pos0;
	pos0.pos.set(0,0,0);
	pos0.time=0;
	emitter_position.push_back(pos0);
	KeyRotate rot0;
	rot0.pos=QuatF::ID;
	rot0.time=0;
	emitter_rotation.push_back(rot0);

	///Общие параметры частиц
	life_time.push_back(KeyFloat(0,1));
	life_time_delta.push_back(KeyFloat(0,0));
	begin_size.push_back(KeyFloat(0,1));
	begin_size_delta.push_back(KeyFloat(0,0));
	num_particle.push_back(KeyFloat(0,100));
	emitter_scale.push_back(KeyFloat(0,1));

	relative = false;
	chFill = false;
	chPlume = false;
	TraceCount = 1;
	PlumeInterval = 0.1f;
	smooth = false;
	cone = false;
	bottom = false;
	planar = false;
	oriented = false;
	softSmoke = false;
	ignoreParticleRate = false;
	orientedCenter = false;
	orientedAxis = false;
	turn = false;
//	PlumeTimeScaling = 1.0f;
//	PlumeSizeScaling = 1.0f;

	//Параметры отдельной частицы
	p_size.push_back(KeyFloat(0,0.5f));
	KeyColor c;
	c.r=c.g=c.b=c.a=1;
	c.time=0;
	p_color.push_back(c);
	p_alpha.push_back(c);
	c.r=c.g=c.b=c.a=1;
	c.time=1;
	p_color.push_back(c);
	p_alpha.push_back(c);

	p_angle_velocity.push_back(KeyFloat(0,0));
//	ix_other = -1;
	zMode_ = EMITTER_USE_ZBUFFER;
	k_wind_min = 1;
	k_wind_max = -1;
	need_wind = false;
	sizeByTexture = false;

	base_angle = 0;
	mirage = false;

	generate_prolonged = false;
	particle_life_time = 1.0f;
	p_size.push_back(KeyFloat(1, p_size[0].f));
	p_angle_velocity.push_back(KeyFloat(1, p_angle_velocity[0].f));

}

EmitterKeyBase::~EmitterKeyBase()
{
}


void EmitterKeyBase::add_sort(vector<float>& xsort,KeysFloat& c)
{
	KeysFloat::iterator it;
	FOR_EACH(c,it)
		xsort.push_back(it->time);
}

void EmitterKeyBase::add_sort(vector<float>& xsort,KeysColor& c)
{
	KeysColor::iterator it;
	FOR_EACH(c,it)
		xsort.push_back(it->time);
}

void EmitterKeyBase::end_sort(vector<float>& xsort)
{
	sort(xsort.begin(),xsort.end());
	for(int i=1;i<xsort.size();i++)
	{
		if(xsort[i]-xsort[i-1]<1e-3)
		{
			xsort.erase(xsort.begin()+i);
			i--;
		}
	}
}

void EmitterKeyBase::GetParticleLifeTime(float t,float& mid_t,float& min_t,float& max_t)
{
	//см. EmitOne
	float inv_particle_life_time=1/max(particle_life_time,1e-3f);
	float inv_life=inv_life_time.Get(t);
	float dlife=life_time_delta.Get(t);

	float inv_mid=max(inv_particle_life_time*inv_life*(1+dlife*(0.5f-0.5f)),0.1f);
	float inv_min=max(inv_particle_life_time*inv_life*(1+dlife*(0.0f-0.5f)),0.1f);
	float inv_max=max(inv_particle_life_time*inv_life*(1+dlife*(1.0f-0.5f)),0.1f);

	mid_t=1/inv_mid;
	min_t=1/inv_min;
	max_t=1/inv_max;
}

KeyPos* EmitterKeyBase::GetOrCreatePosKey(float t,bool* create)
{
	return emitter_position.GetOrCreateKey(t,emitter_life_time,emitter_create_time,create);
}

KeyRotate* EmitterKeyBase::GetOrCreateRotateKey(float t,bool* create)
{
	return emitter_rotation.GetOrCreateKey(t,emitter_life_time,emitter_create_time,create);
}

void EmitterKeyBase::GetPosition(float t,MatXf& m)
{
	m.trans()=emitter_position.Get(t);
	m.rot()=emitter_rotation.Get(t);
}

void EmitterKeyBase::Load3DModelPos(c3dx* model)
{
	xassert(model); 
	begin_position.clear();
	normal_position.clear();
	if (particle_position.type==EMP_3DMODEL_INSIDE)
	{
		TriangleInfo all;
		model->GetTriangleInfo(all,TIF_POSITIONS|TIF_NORMALS|TIF_ZERO_POS/*|TIF_ONE_SCALE*/);
		begin_position.swap(all.positions);
		normal_position.swap(all.normals);

		xassert(begin_position.size()==normal_position.size());
	}
}

void EmitterKeyBase::serialize(Archive& ar)
{
	if(ar.openBlock("emitter", "Эмиттер")){
		ar.serialize(mirage, "mirage", "Мираж");
		ar.serialize(softSmoke, "softSmoke", "Размывать");


		int generationPointCount = this->generationPointCount();
		ar.serialize(generationPointCount, "generationPointCount", "Кол-во точек");
		if(ar.isInput()){
			if(generationPointCount != this->generationPointCount())
				setGenerationPointCount(generationPointCount);
		}

		ar.serialize(emitter_life_time, "emitter_life_time", "Время жизни");

		ar.serialize(life_time, "life_time", 0);
		ar.serialize(inv_life_time, "inv_life_time", 0);

		ar.serialize(wrapCurve(num_particle, true, 100.0f), "num_particle", "Кол-во частиц");
		ar.serialize(generate_prolonged, "generate_prolonged", "Непрерывно");
		ar.serialize(cycled, "cycled", "Зациклить");

		ar.serialize(relative, "relative", "Жесткая привязка");
		ar.serialize(need_wind, "need_wind", "Сночиться ветром");
		if(need_wind || !ar.isEdit() || ar.isInput()){
			ar.serialize(k_wind_min, "k_wind_min", "Инертность min");
			ar.serialize(k_wind_max, "k_wind_max", "Инертность max");
		}

		ar.serialize(chPlume, "chPlume", "Шлейф");
		if(chPlume || !ar.isEdit() || ar.isInput()){
			ar.serialize(TraceCount, "TraceCount", "Гибкость");
			ar.serialize(PlumeInterval, "PlumeInterval", "Интервал");
			ar.serialize(smooth, "smooth", "Сглаживать");
		}


		ar.serialize(planar, "planar", "Плоский");
		if(!ar.isEdit() || planar)
			ar.serialize(turn, "turn", "Поворачиваться");
		if(!ar.isEdit() || !planar)
			ar.serialize(oriented, "oriented", "Ориентированный");

		ar.serialize(orientedCenter, "orientedCenter", "Ориент. центр");
		if(orientedCenter || !ar.isEdit() || ar.isInput())
			ar.serialize(orientedAxis, "orientedAxis", "Ориент. ось");

		ar.serialize(wrapCurve(emitter_scale, true), "emitter_scale", "Масштаб");
		ar.serialize(base_angle, "base_angle", "Угол");




		ar.closeBlock();
	}

	ar.serialize(velocityNoise, "velocityNoise", "Шум скорости");
	ar.serialize(directionNoise, "directionNoise", "Шум направления");


	if(ar.openBlock("noise", "Разброс")){
		bool angleChaos = rotation_direction == ETRD_RANDOM;
		ar.serialize(angleChaos, "angleChaos", "Ротор");
		rotation_direction = angleChaos ? ETRD_RANDOM : ETRD_CW;

		ar.serialize(wrapCurve(begin_size_delta, true), "begin_size_delta", "Размер");
		ar.serialize(wrapCurve(life_time_delta, true), "life_time_delta", "Время");
		ar.closeBlock();
	}
	if(ar.openBlock("particle", "Частица")){
		ar.serialize(begin_position, "begin_position", 0);
		ar.serialize(normal_position, "normal_position", 0);
		ar.serialize(particle_life_time, "particle_life_time", 0);
		ar.serialize(wrapCurve(p_size), "p_size", "Размер");
		ar.serialize(wrapCurve(p_angle_velocity), "p_angle_velocity", "Вращение");
		ar.closeBlock();
	}

	if(ar.openBlock("common", "Общие")){
		ar.serialize(emitter_rotation, "emitter_rotation", 0);
		ar.serialize(wrapCurve(begin_size), "begin_size", 0);

		ar.serialize(ignoreParticleRate, "ignoreParticleRate", "Игнорировать ParticleRate");
		ar.serialize(preciseBound_, "preciseBound", "Считать Bound точно");
		ar.serialize(sprite_blend, "sprite_blend", "Blending");
	
		if(ar.isEdit())
			particle_position.serialize(ar);
		else
			ar.serialize(particle_position, "particle_position", "Распределение");

		if(!ar.isEdit() || particle_position.type == EMP_OTHER_EMITTER) // надо занести в particle_position
			ar.serialize(other, "other", "Эмиттер");
		if(!ar.isEdit() || particle_position.type != EMP_OTHER_EMITTER) // надо занести в particle_position
			ar.serialize(chFill, "chFill", "Заполнение");
		if(!ar.isEdit() || particle_position.type == EMP_CYLINDER){
			ar.serialize(cone, "cone", "Конус");
			ar.serialize(bottom, "bottom", "Ставить на дно");
		}

		ar.serialize(ignoreDistanceCheck_, "ignoreDistanceCheck", "Не исчезать при удалении камеры");


		ar.serialize(sizeByTexture, "sizeByTexture", "Размер по текстуре");
		ar.closeBlock();
	}
	ar.serialize(p_color, "p_color", 0);
	ar.serialize(p_alpha, "p_alpha", 0);

	__super::serialize(ar);

}

void EmitterKeyBase::setParticleLifeTime(float time)
{
	if(generate_prolonged || generationPointCount() > 1)
		time -= emitter_life_time;

	if(time < 0.01f)
		time = 0.01f;
	KeysFloat::iterator it;
	FOR_EACH(life_time, it)
		it->f = time;
}

void EmitterKeyBase::changeParticleLifeTime(int generationPoint, float time)
{
	float t = clamp(time - num_particle[generationPoint].time * emitter_life_time, 0.01f, 100.0f);
	life_time[generationPoint].f = t;
}

float EmitterKeyBase::getParticleLifeTime() const
{
	xassert(life_time.empty());
	return life_time.front().f;
}

float EmitterKeyBase::getGenerationPointTime(int index) const
{
	xassert(index >= 0 && index < emitter_position.size());
	KeyPos key = emitter_position[index];
	return /*emitter_create_time + */key.time * emitter_life_time;
}


void EmitterKeyBase::setGenerationPointTime(int index, float time)
{
	time = clamp((time/* - emitter_create_time*/) / emitter_life_time, 0.0f, 1.0f);

	xassert((size_t)index < emitter_position.size());
	emitter_position[index].time = time;

	setKeyTime(num_particle, index, time);
	setKeyTime(life_time, index, time);
	setKeyTime(life_time_delta, index, time);
	setKeyTime(begin_size, index, time);
	setKeyTime(begin_size_delta, index, time);
	setKeyTime(emitter_scale, index, time);
}

void EmitterKeyBase::setGenerationPointCount(int count)
{
	resizeKey(count, num_particle);
	resizeKey(count, life_time);
	resizeKey(count, life_time_delta);
	resizeKey(count, begin_size);
	resizeKey(count, begin_size_delta);
	resizeKey(count, emitter_scale);

	int oldSize = emitter_position.size();
	emitter_position.resize(count);
	float t = 0;
	float dt = 1.0 / (count - 1);
	KeysPos::iterator it;
	FOR_EACH(emitter_position, it){
		it->time = t;
		if(std::distance(emitter_position.begin(), it) > oldSize - 1)
			it->pos.set(0.0f, 0.0f, 0.0f);
		t += dt;
	}
	//bDirty = true;
}

float EmitterKeyBase::getParticleLongestLifeTime() const
{
	float f, fMax = 0;

	int sz = num_particle.size();
	for(int i = 0; i < sz; i++)	{
		f = life_time[i].f;
		if(generate_prolonged)
			f += emitter_life_time;
		else
			f += num_particle[i].time * emitter_life_time;

		if(fMax < f)
			fMax = f;
	}
	return fMax;
}

float EmitterKeyBase::generationLifeTime(int generation) const
{
	return life_time[generation].f;
}

float EmitterKeyBase::particleKeyTime(int generation, int keyIndex) const
{
	return num_particle[generation].time * emitter_life_time + generationLifeTime(generation) * p_size[keyIndex].time;
}



void EmitterKeyBase::insertParticleKey(int before, float position)
{
	__super::insertParticleKey(before, position);
	insertKey(p_size, before, position);
	insertKey(p_angle_velocity, before, position);
}

void EmitterKeyBase::setParticleKeyTime(int keyIndex, float time)
{
	__super::setParticleKeyTime(keyIndex, time);
	setKeyTime(p_size, keyIndex, time);
	setKeyTime(p_angle_velocity, keyIndex, time);
}

void EmitterKeyBase::deleteParticleKey(int index)
{
	__super::deleteParticleKey(index);
	removeKey(p_size, index);
	removeKey(p_angle_velocity, index);
}

void EmitterKeyBase::setParticleKeyTime(int generation, int keyIndex, float time)
{
	time -= num_particle[generation].time * emitter_life_time;
	time /= generationLifeTime(generation);

	time = clamp(time, 0.0f, 1.0f);
	setParticleKeyTime(keyIndex, time);
}


void EmitterKeyBase::SaveInternal(Saver& s)
{
	s.push(IDS_BUILDKEY_HEADER);
	s<<name;
	s<<(char)sprite_blend;
	s<<generate_prolonged;
	s<<texture_name;
	s<<emitter_create_time;
	s<<emitter_life_time;
	s<<particle_life_time;
	s<<cycled;
	s<<(DWORD)rotation_direction;
	s<<chFill;
	s<<relative;
	char mode = (char)zMode_;
	s<<mode;
	s<<cone;
	s<<bottom;
	s<<randomFrame;
	s<<planar;
	s<<sizeByTexture;
	s<<oriented;
	s<<base_angle;
	s<<mirage;
	s<<softSmoke;
	s<<ignoreParticleRate;
	s<<preciseBound_;
	s<<ignoreDistanceCheck_;
	s.pop();
	s.push(IDS_BUILDKEY_ORIENTATION);
	s<<orientedCenter;
	s<<orientedAxis;
	s.pop();
	s.push(IDS_BUILDKEY_RANDOM_TEXTURES);
	s<<(int)num_texture_names;
	for(int i=0; i<num_texture_names; i++)
	{
		s<<textureNames[i];
	}
	s.pop();
	s.push(IDS_BUILDKEY_PLANAR_TURN);
	s<<turn;
	s.pop();
	if (velocityNoise.enabled)
	{
		s.push(IDS_BUILDKEY_VEL_NOISE);
		s<<velocityNoise.params.amplitude;
		s<<velocityNoise.params.frequency;
		s<<velocityNoise.params.onlyPositive;
		s<<velocityNoise.otherEmitterName;
		s<<velocityNoise.fromOtherEmitter;
		s.pop();
		velocityNoise.params.octaveAmplitudes.Save(s,IDS_BUILDKEY_VEL_OCTAVES);
	}
	if (directionNoise.enabled)
	{
		s.push(IDS_BUILDKEY_DIR_NOISE);
		s<<directionNoise.params.amplitude;
		s<<directionNoise.params.frequency;
		s<<directionNoise.params.onlyPositive;
		s<<directionNoise.otherEmitterName;
		s<<directionNoise.fromOtherEmitter;
		s<<directionNoise.blockX;
		s<<directionNoise.blockY;
		s<<directionNoise.blockZ;
		s<<directionNoise.replace;
		s.pop();
		directionNoise.params.octaveAmplitudes.Save(s,IDS_BUILDKEY_DIR_OCTAVES);
	}
	if (chPlume)
	{
		s.push(IDS_BUILDKEY_PLUME);
		s<<TraceCount;
		s<<PlumeInterval;
		s<<smooth;
		s.pop();
	}
	if (other!="")
	{
		s.push(IDS_BUILDKEY_OTHER_NAME);
		s<<other;
		s.pop();
	}
	s.push(IDS_BUILDKEY_WIND);
		s<<need_wind;
		s<<k_wind_min;
		s<<k_wind_max;
	s.pop();
	emitter_scale.Save(s,IDS_BUILDKEY_EMITTER_SCALE);
	emitter_position.Save(s,IDS_BUILDKEY_POSITION);
	emitter_rotation.Save(s,IDS_BUILDKEY_ROTATION);
	particle_position.Save(s,IDS_BUILDKEY_PARTICLE_POS);

	life_time.Save(s,IDS_BUILDKEY_LIFE_TIME);
	life_time_delta.Save(s,IDS_BUILDKEY_LIFE_TIME_DELTA);
	begin_size.Save(s,IDS_BUILDKEY_BEGIN_SIZE);
	begin_size_delta.Save(s,IDS_BUILDKEY_BEGIN_SIZE_DELTA);
	num_particle.Save(s,IDS_BUILDKEY_NUM_PARTICLE);
	if (particle_position.type == EMP_3DMODEL_INSIDE)
	{
		if(!begin_position.empty())
			begin_position.Save(s,IDS_BUILDKEY_BEGIN_POSITION);
		bool need_normals = s.GetData()!=EXPORT_TO_GAME;
		switch(GetType())
		{
		case EMC_INTEGRAL:
			{
				need_normals |=  ((EmitterKeyInt*)this)->use_light;
				vector<EffectBeginSpeed>::iterator it;
				FOR_EACH(((EmitterKeyInt*)this)->begin_speed, it)
					need_normals |= it->velocity == EMV_NORMAL_3D_MODEL;
			}
			break;
		case EMC_SPLINE:
			switch(((EmitterKeySpline*)this)->direction)
			{
			case ETDS_BURST1: case ETDS_BURST2:
				need_normals = true; 
				break;
			}
			break;
		}
		if (need_normals && !normal_position.empty())
			normal_position.Save(s,IDS_BUILDKEY_NORMAL_POSITION);
	}
	p_size.Save(s,IDS_BUILDKEY_SIZE);
	p_color.Save(s,IDS_BUILDKEY_COLOR);
	p_alpha.Save(s,IDS_BUILDKEY_ALPHA);
	p_angle_velocity.Save(s,IDS_BUILDKEY_ANGLE_VELOCITY);

	spiral_data.Save(s, IDS_BUILDKEY_TEMPLATE_DATA);
}

void EmitterKeyBase::LoadInternal(CLoadData* ld)
{
	switch(ld->id)
	{
	case IDS_BUILDKEY_HEADER:
		{
			CLoadIterator rd(ld);
			rd>>name;
			char blend;
			rd>>blend;
			sprite_blend=(EMITTER_BLEND)blend;
			rd>>generate_prolonged;
			rd>>texture_name;
			rd>>emitter_create_time;
			rd>>emitter_life_time;
			rd>>particle_life_time;
			rd>>cycled;

			DWORD d=rotation_direction;
			rd>>d;
			rotation_direction=(EMITTER_TYPE_ROTATION_DIRECTION)d;
			rd>>chFill;
			rd>>relative;
			char mode = EMITTER_USE_ZBUFFER;
			rd>>mode;
			zMode_ = EmitterZMode(mode);
			rd>>cone;
			rd>>bottom;
			rd>>randomFrame;
			rd>>planar;
			rd>>sizeByTexture;
			rd>>oriented;
			rd>>base_angle;
			rd>>mirage;
			rd>>softSmoke;
			rd>>ignoreParticleRate;
			rd>>preciseBound_;
			rd>>ignoreDistanceCheck_;
		}
		break;
	case IDS_BUILDKEY_PLANAR_TURN:
		{
			CLoadIterator rd(ld);
			rd>>turn;
		}
		break;
	case IDS_BUILDKEY_ORIENTATION:
		{
			CLoadIterator rd(ld);
			rd>>orientedCenter;
			rd>>orientedAxis;
			break;
		}
	case IDS_BUILDKEY_RANDOM_TEXTURES:
		{
			CLoadIterator rd(ld);
			int count;
			rd>>count;
			for (int i=0; i<count; i++)
				rd>>textureNames[i];
		}
		break;
	case IDS_BUILDKEY_VEL_NOISE:
		{
			CLoadIterator rd(ld);
			velocityNoise.enabled = true;
			rd>>velocityNoise.params.amplitude;
			rd>>velocityNoise.params.frequency;
			rd>>velocityNoise.params.onlyPositive;
			rd>>velocityNoise.otherEmitterName;
			rd>>velocityNoise.fromOtherEmitter;
		}
		break;
	case IDS_BUILDKEY_VEL_OCTAVES:
		{
			velocityNoise.params.octaveAmplitudes.Load(CLoadIterator(ld));
		}
		break;
	case IDS_BUILDKEY_DIR_NOISE:
		{
			CLoadIterator rd(ld);
			directionNoise.enabled = true;
			rd>>directionNoise.params.amplitude;
			rd>>directionNoise.params.frequency;
			rd>>directionNoise.params.onlyPositive;
			rd>>directionNoise.otherEmitterName;
			rd>>directionNoise.fromOtherEmitter;
			rd>>directionNoise.blockX;
			rd>>directionNoise.blockY;
			rd>>directionNoise.blockZ;
			rd>>directionNoise.replace;
		}
		break;
	case IDS_BUILDKEY_DIR_OCTAVES:
		{
			directionNoise.params.octaveAmplitudes.Load(CLoadIterator(ld));
		}
		break;
	case IDS_BUILDKEY_TEMPLATE_DATA:
		spiral_data.Load(ld);
		break;
	case IDS_BUILDKEY_OTHER_NAME:
		{
			CLoadIterator rd(ld);
			rd>>other;
		}
		break;
	case IDS_BUILDKEY_PLUME:
		{
			CLoadIterator rd(ld);
			chPlume = true;
			rd>>TraceCount;
			rd>>PlumeInterval;
			rd>>smooth;
		}
		break;
	case IDS_BUILDKEY_EMITTER_SCALE:
		emitter_scale.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_POSITION:
		emitter_position.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_ROTATION:
		emitter_rotation.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_PARTICLE_POS:
		particle_position.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_LIFE_TIME:
		life_time.Load(CLoadIterator(ld));
		BuildInvLifeTime();
		break;
	case IDS_BUILDKEY_LIFE_TIME_DELTA:
		life_time_delta.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_BEGIN_SIZE:
		begin_size.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_BEGIN_SIZE_DELTA:
		begin_size_delta.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_NUM_PARTICLE:
		{
			num_particle.Load(CLoadIterator(ld));
//Дикие строчки наверняка тупая затычка для редактора какаято!
//			if (num_particle.size() != emitter_position.size())
//				emitter_scale.resize(num_particle.size());
		}
		break;
	case IDS_BUILDKEY_BEGIN_POSITION:
		begin_position.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_NORMAL_POSITION:
		normal_position.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_SIZE:
		p_size.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_COLOR:
		p_color.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_ALPHA: //for compatibility with old effects
		p_alpha.Load(CLoadIterator(ld));
		break;
	case IDS_BUILDKEY_ANGLE_VELOCITY:
		p_angle_velocity.Load(CLoadIterator(ld));
		break;

	case IDS_BUILDKEY_WIND:
		{
			CLoadIterator rd(ld);
			rd>>need_wind;
			rd>>k_wind_min;
			rd>>k_wind_max;
		}
		break;
	}

	if (textureNames[0].size()==0)
		textureNames[0] = texture_name;
}

void EmitterKeyBase::BuildInvLifeTime()
{
	inv_life_time.resize(life_time.size());

	for(int i = 0; i < life_time.size(); i++)
	{
		inv_life_time[i].time = life_time[i].time;
		float l = max(life_time[i].f, 1e-2f);
		inv_life_time[i].f = 1 / l;
	}
}


//////////////////////////////EmitterKey////////////////////////////
EmitterKeyInt::EmitterKeyInt()
{
	p_velocity.push_back(KeyFloat(0,1));
	p_gravity.push_back(KeyFloat(0,0));

	velocity_delta.push_back(KeyFloat(0,0));
	use_light=false;
	angle_by_center=false;
	g.set(0, 0, 1);

	begin_speed.push_back(EffectBeginSpeed());
	begin_speed.push_back(EffectBeginSpeed());
	begin_speed.back().time = 1.f;

	p_velocity[0].f = 0;
	p_velocity.push_back(KeyFloat(1, 0));
	p_gravity.push_back(KeyFloat(1, p_gravity[0].f));
	KeyParticleInt k;
	key.push_back(k);
}

EmitterKeyInt::~EmitterKeyInt()
{
}

void EmitterKeyInt::BuildKey()
{
	vector<float> xtime;
	xtime.push_back(0);
	xtime.push_back(1);
	add_sort(xtime,p_velocity);
	add_sort(xtime,p_size);
	add_sort(xtime,p_color);
	add_sort(xtime,p_alpha);
	add_sort(xtime,p_angle_velocity);
	add_sort(xtime,p_gravity);
	end_sort(xtime);

	key.clear();
	KeyParticleInt k;
	k.angle_vel=0;
	k.color.set(1,1,1,1);

	for(int i=0;i<xtime.size();i++)
	{
		if(i<xtime.size()-1)
		{
			k.dtime=xtime[i+1]-xtime[i];
		}else
		{
			k.dtime=0;
		}

		float t=xtime[i];
		k.vel=p_velocity.Get(t)*200;
		k.size=p_size.Get(t)*20;
		k.color=p_color.Get(t);
		k.color.a=p_alpha.Get(t).a;
		k.color.a *= 255;
		k.angle_vel=p_angle_velocity.Get(t)*5;
		k.gravity=p_gravity.Get(t)*100;
		k.inv_dtime = 1 / max(k.dtime, 1e-20f);

		key.push_back(k);
	}
}

void EmitterKeyInt::setGenerationPointCount(int count)
{
	resizeKey(count, velocity_delta);

	EmitterKeyBase::setGenerationPointCount(count);
}

void EmitterKeyInt::SaveInternal(Saver& s)
{
	EmitterKeyBase::SaveInternal(s);

	velocity_delta.Save(s,IDS_BUILDKEY_VELOCITY_DELTA);

	p_velocity.Save(s,IDS_BUILDKEY_VELOCITY);
	p_gravity.Save(s,IDS_BUILDKEY_GRAVITY);

	for(int i=0;i<begin_speed.size();i++)
		begin_speed[i].Save(s);
/*	if (particle_position.type==EMP_3DMODEL_INSIDE && !normal_position.empty()&&
		(s.GetData()==EXPORT_TO_GAME ? use_light : true))
		normal_position.Save(s,IDS_BUILDKEY_NORMAL_POSITION);
*/
	s.push(IDS_BUILDKEY_INT_LIGHT);
	s<<use_light;
	s.pop();
	s.push(IDS_BUILDKEY_DIR_ANGLE_BY_CENTER);
	s<<angle_by_center;
	s.pop();
}

void EmitterKeyInt::Save(Saver& s)
{
	s.push(IDS_BUILDKEY_INT);
	SaveInternal(s);
	s.pop();
}

void EmitterKeyInt::Load(CLoadDirectory rd)
{
	begin_speed.clear();
	while(CLoadData* ld=rd.next())
	{
		LoadInternal(ld);
		switch(ld->id)
		{
		case IDS_BUILDKEY_VELOCITY_DELTA:
			velocity_delta.Load(CLoadIterator(ld));
			break;
		case IDS_BUILDKEY_VELOCITY:
			p_velocity.Load(CLoadIterator(ld));
			break;
		case IDS_BUILDKEY_GRAVITY:
			p_gravity.Load(CLoadIterator(ld));
			break;
		case IDS_BUILDKEY_BEGIN_SPEED:
			{
				EffectBeginSpeed ps;
				ps.Load(CLoadIterator(ld));
				begin_speed.push_back(ps);
			}
			break;
		case IDS_BUILDKEY_INT_LIGHT:
			{
				CLoadIterator rd(ld);
				rd>>use_light;
				break;
			}
		case IDS_BUILDKEY_DIR_ANGLE_BY_CENTER:
			{
				CLoadIterator rd(ld);
				rd>>angle_by_center;
				break;
			}
		}
	}

	BuildKey();
}

/////////////////////EffectKey////////////////////////
EffectKey::EffectKey(const char* name) : StringTableBase(name)
{
	need_tilemap = false;
	delete_assert=true;
}

EffectKey::~EffectKey()
{
	Clear();
}

void EffectKey::Clear()
{
	emitterKeys.clear();
}


void EffectKey::operator= (const EffectKey& effect_key)
{
	name = effect_key.name;
	filename=effect_key.filename;///Совсем недавно не копировалось. Может что-то сломается в редакторе эффектов.
	delete_assert = effect_key.delete_assert;
	need_tilemap = effect_key.need_tilemap;

	Clear();

	EmitterKeys::const_iterator it;
	FOR_EACH(effect_key.emitterKeys, it)
		emitterKeys.push_back((*it)->Clone());
}

void EffectKey::Save(Saver& s)
{
	s.push(IDS_EFFECTKEY);
	s.push(IDS_EFFECTKEY_HEADER);
	s<<name;
	s.pop();

	EmitterKeys::iterator it;
	FOR_EACH(emitterKeys,it)
		(*it)->Save(s);

	s.pop();
}

void EffectKey::Load(CLoadDirectory rd)
{
	while(CLoadData* ld=rd.next())
	switch(ld->id)
	{
	case IDS_EFFECTKEY_HEADER:
		{
			CLoadIterator rd(ld);
			rd>>name;
		}
		break;
	case IDS_BUILDKEY_INT:
		{
			EmitterKeyInt* p=new EmitterKeyInt;
			emitterKeys.push_back(p);
			p->Load(ld);
		}
		break;
	case IDS_BUILDKEY_SPL:
		{
			EmitterKeySpline* p=new EmitterKeySpline;
			emitterKeys.push_back(p);
			p->Load(ld);
		}
		break;
	case IDS_BUILDKEY_Z:
		{
			EmitterKeyZ* p=new EmitterKeyZ;
			emitterKeys.push_back(p);
			p->Load(ld);
			need_tilemap = true;
		}
		break;
	case IDS_BUILDKEY_LIGHT:
		{
			EmitterKeyLight* p=new EmitterKeyLight;
			emitterKeys.push_back(p);
			p->Load(ld);
		}
		break;
	case IDS_BUILDKEY_COLUMN_LIGHT:
		{
			EmitterKeyColumnLight* p=new EmitterKeyColumnLight;
			emitterKeys.push_back(p);
			p->Load(ld);
		}
		break;
	case IDS_BUILDKEY_LIGHTING:
		{
			EmitterKeyLighting* p=new EmitterKeyLighting;
			emitterKeys.push_back(p);
			p->Load(ld);
		}
		break;
	}
}

void EffectKey::RelativeScale(float scale)
{
	EmitterKeys::iterator it;
	FOR_EACH(emitterKeys, it)
		(*it)->RelativeScale(scale);
}

void EffectKey::MulToColor(Color4c color)
{
	EmitterKeys::iterator it;
	FOR_EACH(emitterKeys, it){
		(*it)->MulToColor(color);
		(*it)->BuildKey();
	}
}

void EffectKey::BuildRuntimeData()
{
	EmitterKeys::iterator it;
	FOR_EACH(emitterKeys, it)
		(*it)->BuildRuntimeData();
}

void EffectKey::serialize(Archive& ar)
{
	ar.serialize(emitterKeys, "emitterKeys", 0);
}

void EffectKey::changeTexturePath(const char* texture_path)
{
	string path;
	if(texture_path && strlen(texture_path)>0)
	{
		char c=texture_path[strlen(texture_path)-1];
		path=texture_path;
		if(c!='\\' && c!='/')
		{
			path+='\\';
		}
	}

	char path_buffer[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	
	EmitterKeys::iterator it;
	FOR_EACH(emitterKeys, it){
		string& t=(*it)->texture_name;
		if (!t.empty())
		{
			_splitpath(t.c_str(),drive,dir,fname,ext);
			sprintf(path_buffer,"%s%s%s",path.c_str(),fname,ext);
			t=path_buffer;
		}
		for(int i=0; i<EmitterKeyInterface::num_texture_names; i++)
		{
			string& t=(*it)->textureNames[i];
			if (!t.empty())
			{
				_splitpath(t.c_str(),drive,dir,fname,ext);
				sprintf(path_buffer,"%s%s%s",path.c_str(),fname,ext);
				t=path_buffer;
			}
		}
		if ((*it)->GetType()==EMC_COLUMN_LIGHT)
		{
			string& t= ((EmitterKeyColumnLight*)&*(*it))->texture2_name;
			if (!t.empty())
			{
				_splitpath(t.c_str(),drive,dir,fname,ext);
				sprintf(path_buffer,"%s%s%s",path.c_str(),fname,ext);
				t=path_buffer;
			}
		}
	}
}

///////////////////////////EmitterKeyBase///////////////////////////
void EmitterKeyBase::RelativeScale(float scale)
{
	{
		KeysPos::iterator it;
		FOR_EACH(emitter_position,it)
		{
			Vect3f& pos=it->pos;
			pos*=scale;
		}
	}

	particle_position.size*=scale;
	KeysFloat::iterator it;
	FOR_EACH(begin_size,it)
		it->f*=scale;
//	FOR_EACH(begin_size_delta,it)
//		it->f*=scale;

	CVectVect3f::iterator v3;
	FOR_EACH(begin_position,v3)
		(*v3)*=scale;

}

void EmitterKeyLight::RelativeScale(float scale)
{
	{
		KeysPos::iterator it;
		FOR_EACH(emitter_position,it)
		{
			Vect3f& pos=it->pos;
			pos*=scale;
		}
	}

	KeysFloat::iterator it;
	FOR_EACH(emitter_size,it)
		it->f*=scale;
}

void EmitterKeyInt::RelativeScale(float scale)
{
	EmitterKeyBase::RelativeScale(scale);

	KeysFloat::iterator it;
	FOR_EACH(p_velocity,it)
		it->f*=scale;
	FOR_EACH(p_gravity,it)
		it->f*=scale;

	vector<KeyParticleInt>::iterator pkey;
	FOR_EACH(key, pkey)
	{
		pkey->vel*=scale;
//		pkey->size*=scale;
		pkey->gravity*=scale;
	}

}

void EmitterKeySpline::RelativeScale(float scale)
{
	EmitterKeyBase::RelativeScale(scale);

	KeysPosHermit::iterator it;
	FOR_EACH(p_position,it)
	{
		Vect3f& pos=it->pos;
		pos*=scale;
	}
	BuildKey();
}


bool EffectKey::needModel()
{
	EmitterKeys::iterator it;
	FOR_EACH(emitterKeys, it){
		if((*it)->needModel())
			return true;
	}

	return false;
}

bool EffectKey::isCycled()
{
	EmitterKeys::iterator it;
	FOR_EACH(emitterKeys, it)
		if((*it)->cycled)
			return true;

	return false;
}

bool EffectKey::GetNeedTilemap()
{
	return need_tilemap;
}


EmitterKeyZ::EmitterKeyZ()
{
	add_z=20;
	planar=true;
	angle_by_center=true;
	base_angle=0;
	use_force_field=false;
	use_water_plane = false;

	begin_speed.push_back(EffectBeginSpeed());
	begin_speed.push_back(EffectBeginSpeed());
	begin_speed.back().time = 1.f;

	p_velocity[0].f = 0;
	p_velocity.push_back(KeyFloat(1, 0));
	p_gravity.push_back(KeyFloat(1, p_gravity[0].f));
}

void EmitterKeyZ::Save(Saver& s)
{
	s.push(IDS_BUILDKEY_Z);
	SaveInternal(s);
	bool reserved_planar = false;
		s.push(IDS_BUILDKEY_ZEMITTER);
		s<<add_z;
		s<<reserved_planar;
		s<<angle_by_center;
		s<<base_angle;
		s<<use_force_field;
		s<<use_water_plane;
		s.pop();
	s.pop();
}

void EmitterKeyZ::LoadInternal(CLoadData* ld)
{
	EmitterKeyInt::LoadInternal(ld);
	switch(ld->id)
	{
	case IDS_BUILDKEY_ZEMITTER:
		{
			CLoadIterator rd(ld);
			bool reserved_planar = false;
			rd>>add_z;
			rd>>reserved_planar;
			rd>>angle_by_center;
			rd>>base_angle;
			rd>>use_force_field;
			rd>>use_water_plane;
		}
		break;
	}
}

void EmitterKeyZ::BuildKey()
{
	
	EmitterKeyInt::BuildKey();

	for(int i = 0; i < key.size(); i++)
	{
		KeyParticleInt & k = key[i];

		k.color.r *= 255.0f;
		k.color.g *= 255.0f;
		k.color.b *= 255.0f;
	}
}

void EmitterKeyZ::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(add_z, "add_z", "Высота");
	ar.serialize(use_force_field, "use_force_field", "Поле");
	ar.serialize(use_water_plane, "use_water_plane", "use_water_plane");
}

void EmitterKeyZ::RelativeScale(float scale)
{
	EmitterKeyInt::RelativeScale(scale);
	add_z*=scale;
}

//////////////////
EmitterKeyInterface* EmitterKeyInt::Clone()
{
	EmitterKeyInt* p=new EmitterKeyInt;
	*p=*this;
	return p;
}

void EmitterKeyInt::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(g, "g", 0);

	if(ar.openBlock("emitter", "Эмиттер")){
		ar.serialize(angle_by_center, "angle_by_center", "Центр");
		ar.closeBlock();
	}

	if(ar.openBlock("particle", "Частица")){
		ar.serialize(wrapCurve(p_velocity, false), "p_velocity", "Скорость");
		ar.serialize(wrapCurve(begin_speed, false), "begin_speed", "Коэф. скорости");
		ar.serialize(wrapCurve(p_gravity, false), "p_gravity", "Гравитация");
		ar.serialize(use_light, "use_light", "Освещение");
		ar.closeBlock();
	}
	if(ar.openBlock("noise", "Разброс")){
		ar.serialize(wrapCurve(velocity_delta, true), "velocity_delta", "Скорость");
		ar.closeBlock();
	}
}

void EmitterKeyInt::setGenerationPointTime(int index, float time)
{
	EmitterKeyBase::setGenerationPointTime(index, time);
	time = clamp((time/* - emitter_create_time*/) / emitter_life_time, 0.0f, 1.0f) - emitter_create_time;

	if(index < velocity_delta.size())
		velocity_delta[index].time = time;
}


void EmitterKeyInt::setParticleKeyTime(int keyIndex, float time)
{
	__super::setParticleKeyTime(keyIndex, time);

	setKeyTime(p_velocity, keyIndex, time);
	setKeyTime(p_gravity, keyIndex, time);
	setKeyTime(begin_speed, keyIndex, time);
}

void EmitterKeyInt::insertParticleKey(int before, float position)
{
	__super::insertParticleKey(before, position);

	insertKey(p_velocity,  before, position);
	insertKey(p_gravity,   before, position);
	insertKey(begin_speed, before, position);
}

void EmitterKeyInt::deleteParticleKey(int index)
{
	__super::deleteParticleKey(index);
	removeKey(p_velocity, index);
	removeKey(p_gravity, index);
	removeKey(begin_speed, index);
}

EmitterKeyInterface* EmitterKeySpline::Clone()
{
	EmitterKeySpline* p=new EmitterKeySpline;
	*p=*this;
	return p;
}

void EmitterKeySpline::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(p_position_auto_time, "p_position_auto_time", 0); // более равномерная скорость частиц
	ar.serialize(p_position, "p_position", 0);

	ar.serialize(direction, "direction", "Движение");
}

void EmitterKeySpline::setParticleKeyTime(int keyIndex, float time)
{
	__super::setParticleKeyTime(keyIndex, time);
	setKeyTime(p_position, keyIndex, time);
}

void EmitterKeySpline::insertParticleKey(int before, float position)
{
	__super::insertParticleKey(before, position);
	insertKey(p_position, before, position);
}

void EmitterKeySpline::deleteParticleKey(int index)
{
	__super::deleteParticleKey(index);
	removeKey(p_position, index);
}

EmitterKeyInterface* EmitterKeyZ::Clone()
{
	EmitterKeyZ* p=new EmitterKeyZ;
	*p=*this;
	return p;
}

EmitterKeyInterface* EmitterKeyLight::Clone()
{
	EmitterKeyLight* p=new EmitterKeyLight;
	*p=*this;
	return p;
}

void EffectKey::preloadTexture()
{
	if(!Option_EnablePreload3dx)
		return;
	EmitterKeys::iterator it;
	FOR_EACH(emitterKeys, it)
		(*it)->preloadTexture();
}


void EmitterKeyInterface::preloadTexture()
{
	if(!texture_name.empty())
	{
		cTexture* pTexture=GetTexLibrary()->GetElement3D(texture_name.c_str());
		if(pTexture)pTexture->setAttribute(TEXTURE_NO_COMPACTED);
		RELEASE(pTexture);
	}

	vector<string> names;
	GetTextureNames(names);
	if(!names.empty())
	{
		if (names.size()==1 || !randomFrame)
		{
			if(!names[0].empty())
			{
				cTexture* pTexture = 0;
				string texture_name = normalizePath(names[0].c_str());
				if (strstr(texture_name.c_str(), ".AVI"))
					pTexture=GetTexLibrary()->GetElement3DAviScale(texture_name.c_str());
				else
					pTexture=GetTexLibrary()->GetElement3D(names[0].c_str());
				if(pTexture)pTexture->setAttribute(TEXTURE_NO_COMPACTED);
				RELEASE(pTexture);
			}
		}else
		{
			cTexture* pTexture=GetTexLibrary()->GetElement3DComplex(names);
			if(pTexture)pTexture->setAttribute(TEXTURE_NO_COMPACTED);
			RELEASE(pTexture);
		}
	}
}

void EmitterKeyInterface::setParticleKeyTime(int keyIndex, float time)
{
    
}

void EmitterKeyInterface::setParticleKeyTime(int generation, int keyIndex, float time)
{
	time /= emitter_life_time;
	time = clamp(time, 0.0f, 1.0f);
	setParticleKeyTime(keyIndex, time);
}

void EmitterKeyInterface::insertParticleKey(int before, float position)
{
}

void EmitterKeyInterface::deleteParticleKey(int index)
{
}

void EmitterKeyColumnLight::preloadTexture()
{
	__super::preloadTexture();

	if(!texture2_name.empty())
	{
		cTexture* pTexture=GetTexLibrary()->GetElement3D(texture2_name.c_str());
		if(pTexture)pTexture->setAttribute(TEXTURE_NO_COMPACTED);
		RELEASE(pTexture);
	}
}

int EmitterKeyColumnLight::getTextureCount() const
{
	return __super::getTextureCount() + 1;
}

void EmitterKeyColumnLight::GetTextureNames(vector<string>& names, bool omitEmpty)
{
	__super::GetTextureNames(names, omitEmpty);
	if(!omitEmpty || !texture2_name.empty())
		names.push_back(texture2_name);
}

void EmitterKeyColumnLight::setTextureNames(const vector<string>& names)
{
	xassert(names.size() == getTextureCount());
	__super::setTextureNames(names);
	texture2_name = names[__super::getTextureCount()];
}

void EmitterKeyColumnLight::serialize(Archive& ar)
{
	ar.serialize(texture2_name, "texture2_name", "texture2_name");

	ar.serialize(sprite_blend, "sprite_blend", "Blending текстур");
	ar.serialize(color_mode, "color_mode", "Blending");

	ar.serialize(wrapCurve(emitter_size), "emitter_size", "Ширина 1");
	ar.serialize(wrapCurve(emitter_size2), "emitter_size2", "Ширина 2");
	ar.serialize(emitter_color, "emitter_color", 0);
	ar.serialize(emitter_alpha, "emitter_alpha", 0);

	ar.serialize(wrapCurve(u_vel), "u_vel", "Скорость текстуры по X");
	ar.serialize(wrapCurve(v_vel), "v_vel", "Скорость текстуры по Y");
	ar.serialize(wrapCurve(height), "height", "Длина");

	ar.serialize(turn, "turn", "Зациклить");
	ar.serialize(plane, "plane", "Плоскость");
	ar.serialize(laser, "laser", "Лазер");
	ar.serialize(discrete_laser, "discrete_laser", "Дискретный лазер");

	if(discrete_laser){
		ar.serialize(wrapCurve(length), "length", "Длина отрезка лазера");
		ar.serialize(missileSpeed, "missileSpeed", "Скорость полёта отрезка лазера");
	}

	ar.serialize(rot, "rot", 0);
}

void EmitterKeyColumnLight::insertParticleKey(int before, float position)
{
	__super::insertParticleKey(before, position);

	insertKey(emitter_size, before, position);
	insertKey(emitter_size2, before, position);
	insertKey(height, before, position);
	insertKey(length, before, position);
	insertKey(u_vel, before, position);
	insertKey(v_vel, before, position);
}

void EmitterKeyColumnLight::setParticleKeyTime(int keyIndex, float time)
{
	__super::setParticleKeyTime(keyIndex, time);

	setKeyTime(emitter_size, keyIndex, time);
	setKeyTime(emitter_size2, keyIndex, time);
	setKeyTime(height, keyIndex, time);
	setKeyTime(length, keyIndex, time);
	setKeyTime(u_vel, keyIndex, time);
	setKeyTime(v_vel, keyIndex, time);
}

void EmitterKeyColumnLight::deleteParticleKey(int index)
{
	__super::deleteParticleKey(index);

	removeKey(emitter_size, index);
	removeKey(emitter_size2, index);
	removeKey(height, index);
	removeKey(length, index);
	removeKey(u_vel, index);
	removeKey(v_vel, index);
}

void EmitterKeyInterface::GetTextureNames(vector<string>& names, bool omitEmpty)
{
	if(!omitEmpty || !texture_name.empty())
		names.push_back(texture_name);
	for(int i = 0; i < num_texture_names; i++)
		if(!omitEmpty || !textureNames[i].empty())
			names.push_back(textureNames[i]);
}


int EmitterKeyInterface::getTextureCount() const
{
	return 1 + num_texture_names;
}

void EmitterKeyInterface::setTextureNames(const vector<string>& fileNames)
{
	xassert(fileNames.size() == getTextureCount());
	texture_name = fileNames[0];
	for(int i = 0; i < num_texture_names; ++i){
		textureNames[i] = fileNames[i];
	}
}

