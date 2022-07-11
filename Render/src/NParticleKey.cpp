#include "StdAfxRD.h"
#include "NParticle.h"
#include "scene.h"
#include <algorithm>
#include "NParticleID.h"
#include "TileMap.h"
#include "ForceField.h"
#include "Serialization.h"

KeyFloat::value KeyFloat::none=0;
KeyPos::value KeyPos::none=Vect3f::ZERO;
KeyRotate::value KeyRotate::none=QuatF::ID;
KeyColor::value KeyColor::none=sColor4f(0,0,0,0);
float KeyGeneral::time_delta=0.05f;//в секундах

void KeyGeneral::serialize(Archive& ar)
{
	ar.serialize(time, "time", "time");
}

void KeyColor::serialize(Archive& ar)
{
	KeyGeneral::serialize(ar);
	sColor4f color (r, g, b, a);
	ar.serialize(color, "color", "Цвет");
	r = color.r;
	g = color.g;
	b = color.b;
	a = color.a;
}

void CKeyColor::serialize(Archive& ar)
{
	ar.serialize((vector<KeyColor>&)*this, "color", "color");
}

///////////////////////////EmitterColumnLight////////////////////////////////
EmitterKeyColumnLight::EmitterKeyColumnLight()
{
	color_mode = EMITTER_BLEND_MULTIPLY;
	sprite_blend = EMITTER_BLEND_MULTIPLY;
	turn = true;
	rot.set(Mat3f::ID);
	laser = false;
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
	s.pop();
	spiral_data.Save(s, IDS_BUILDKEY_TEMPLATE_DATA);
}
void EmitterKeyColumnLight::Load(CLoadDirectory rd)
{
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
	case IDS_BUILDKEY_COLUMN_LIGHT_ROT:
		{
			CLoadIterator rd(ld);
			rd>>rot.x(); rd>>rot.y(); rd>>rot.z(); rd>>rot.s();
		}
		break;
	}
	BuildKey();
}
void EmitterKeyColumnLight::RelativeScale(float scale)
{
	CKeyPos::iterator ipos;
	FOR_EACH(emitter_position,ipos)
		ipos->pos*=scale;
	CKey::iterator it;
	FOR_EACH(emitter_size,it)
		it->f*=scale;
	FOR_EACH(emitter_size2,it)
		it->f*=scale;
	FOR_EACH(height,it)
		it->f*=scale;
}
EmitterKeyInterface* EmitterKeyColumnLight::Clone()
{
	EmitterKeyColumnLight* p=new EmitterKeyColumnLight;
	*p=*this;
	return p;
}
void EmitterKeyColumnLight::BuildKey()
{
}

//////////////////////////////////////////////////////////////////////////////
void EmitterLightingKey::Save(Saver& s)
{
	s.push(IDS_BUILDKEY_LIGHTING);
		s.push(IDS_BUILDKEY_LIGHTING_HEADER);
			s<<name;
			s<<texture_name;
			s<<emitter_create_time;
			s<<emitter_life_time;
			s<<cycled;
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
	s.pop();
}
void EmitterLightingKey::Load(CLoadDirectory rd)
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
		}
	}
}
void EmitterLightingKey::RelativeScale(float scale)
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
EmitterKeyInterface* EmitterLightingKey::Clone()
{
	EmitterLightingKey* p= new EmitterLightingKey;
	*p=*this;
	return p;
}
void EmitterLightingKey::BuildKey()
{
}

////////////////////////cEmitterSpl//////////////////////

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

CKeyPosHermit::CKeyPosHermit()
{
	//cbegin=cend=T_CLOSE;
	cbegin=cend=T_FREE;
}

Vect3f CKeyPosHermit::Clamp(int i)
{
	VISASSERT(size()>=2);
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

Vect3f CKeyPosHermit::Get(float t)
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

void CKey::Save(Saver& s,int id)
{
	s.push(id);
	s<<(int)size();
	CKey::iterator it;
	FOR_EACH(*this,it)
	{
		s<<it->time;
		s<<it->f;
	}
	s.pop();
}

void CKey::Load(CLoadIterator& rd)
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

void CKeyRotate::Save(Saver& s,int id)
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

void CKeyRotate::Load(CLoadIterator& rd)
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

void CKeyPos::SaveInternal(Saver& s)
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

void CKeyPos::Save(Saver& s,int id)
{
	s.push(id);
	SaveInternal(s);
	s.pop();
}

void CKeyPos::Load(CLoadIterator& rd)
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

void CKeyPosHermit::Save(Saver& s,int id)
{
	s.push(id);
	CKeyPos::SaveInternal(s);
	s<<(BYTE)cbegin;
	s<<(BYTE)cend;
	s.pop();
}

void CKeyPosHermit::Load(CLoadIterator& rd)
{
	CKeyPos::Load(rd);
	BYTE b=T_FREE;
	rd>>b;
	cbegin=(CLOSING)b;
	rd>>b;
	cend=(CLOSING)b;
}

///////////////////////CKeyColor///////////////////////////////

void CKeyColor::Save(Saver& s,int id)
{
	s.push(id);
	s<<(int)size();
	CKeyColor::iterator it;
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

void CKeyColor::Load(CLoadIterator& rd)
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

void CKeyColor::MulToColor(sColor4f color)
{
	for(iterator it=begin();it!=end();++it)
	{
		sColor4f& c=*it;
		c*=color;
	}
}

///////////////////////EmitterKeySpl//////////////////
EmitterKeySpl::EmitterKeySpl()
{
	emitter_life_time=particle_life_time=2.0f;
	cycled=true;
	generate_prolonged=true;
	begin_size[0]=KeyFloat(0,0.2f);
	num_particle[0]=KeyFloat(0,1000);


//	particle_position.size.set(0.2f,0.2f,0.2f);
	direction=ETDS_ROTATEZ;//ETDS_ID;//
	p_position_auto_time=false;
	KeyPos pos0;
	pos0.time=0;
/*
	pos0.pos.set(0,0,0);
	p_position.push_back(pos0);

	pos0.pos.set(0,0,50);
	p_position.push_back(pos0);

	pos0.pos.set(0,50,100);
	p_position.push_back(pos0);

	pos0.pos.set(0,100,50);
	p_position.push_back(pos0);

	pos0.pos.set(0,0,0);
	p_position.push_back(pos0);
/*/
	pos0.pos.set(0,0,0);
	p_position.push_back(pos0);

	pos0.pos.set(0,0,100);
	pos0.time = 1.0;
	p_position.push_back(pos0);

//	pos0.pos.set(0,0,30);
//	p_position.push_back(pos0);
//	pos0.pos.set(0,60,60);
//	p_position.push_back(pos0);
/*
	const mx=12;
	for(float t=0;t<mx;t+=0.5f)
	{
		float r=25*exp(-t*0.1f);
		pos0.time=1+(t-mx+1)*0.05;
		pos0.pos.set(t*3,25-r*cos(t),50+r*sin(t));
		p_position.push_back(pos0);
	}
*/

/**/
}

EmitterKeySpl::~EmitterKeySpl()
{
}

void EmitterKeySpl::Save(Saver& s)
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

void EmitterKeySpl::Load(CLoadDirectory rd)
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
				direction=(EMITTER_TYPE_DIRECTION_SPL)d;
			}
			break;
		case IDS_BUILDKEY_SPL_POSITION:
			p_position.Load(CLoadIterator(ld));
			break;
		}
	}

	BuildKey();
}

void EmitterKeySpl::BuildKey()
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
		HeritKey & hk = hkeys[i];
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
		HeritKey & hk = hkeys[i];
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
		HeritKey & hk = hkeys[i];
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
		HeritKey & hk = hkeys[i];
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

	//blend_mode = ALPHA_BLEND;
}

float EmitterKeyInterface::LocalTime(float t)
{
	return (emitter_life_time>KeyGeneral::time_delta)?(t-emitter_create_time)/emitter_life_time:0;
}

float EmitterKeyInterface::GlobalTime(float t)
{
	return t*emitter_life_time+emitter_create_time;
}

/////////////////////////////EmitterKeyLight/////////////////////////
EmitterKeyLight::EmitterKeyLight()
{
	toObjects = true;
	toTerrain = true;
	light_blend = EMITTER_BLEND_ADDING;
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

/////////////////////////////EmitterKeyBase/////////////////////////
EmitterKeyBase::EmitterKeyBase()
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
	draw_first_no_zbuffer=0;
	k_wind_min = 1;
	k_wind_max = -1;
	need_wind = false;
	isVelNoiseOther = false;
	isDirNoiseOther = false;
	velNoiseOther.clear();
	dirNoiseOther.clear();
	sizeByTexture = false;
	noiseBlockX = false;
	noiseBlockY = false;
	noiseBlockZ = false;
	noiseReplace = true;

	velNoiseEnable = false;
	velFrequency = 1;
	velAmplitude = 1;
	velOnlyPositive = false;

	dirNoiseEnable = false;
	dirFrequency = 1;
	dirAmplitude = 1;
	dirOnlyPositive = false;

	base_angle = 0;
	mirage = false;
}

EmitterKeyBase::~EmitterKeyBase()
{
}


void EmitterKeyBase::add_sort(vector<float>& xsort,CKey& c)
{
	CKey::iterator it;
	FOR_EACH(c,it)
		xsort.push_back(it->time);
}

void EmitterKeyBase::add_sort(vector<float>& xsort,CKeyColor& c)
{
	CKeyColor::iterator it;
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
	VISASSERT(model); 
	begin_position.clear();
	normal_position.clear();
	if (particle_position.type==EMP_3DMODEL_INSIDE)
	{
		TriangleInfo all;
		model->GetTriangleInfo(all,TIF_POSITIONS|TIF_NORMALS|TIF_ZERO_POS/*|TIF_ONE_SCALE*/);
		begin_position.swap(all.positions);
		normal_position.swap(all.normals);

		VISASSERT(begin_position.size()==normal_position.size());
	}
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
	s<<draw_first_no_zbuffer;
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
	if (velNoiseEnable)
	{
		s.push(IDS_BUILDKEY_VEL_NOISE);
		s<<velAmplitude;
		s<<velFrequency;
		s<<velOnlyPositive;
		s<<velNoiseOther;
		s<<isVelNoiseOther;
		s.pop();
		velOctavesAmplitude.Save(s,IDS_BUILDKEY_VEL_OCTAVES);
	}
	if (dirNoiseEnable)
	{
		s.push(IDS_BUILDKEY_DIR_NOISE);
		s<<dirAmplitude;
		s<<dirFrequency;
		s<<dirOnlyPositive;
		s<<dirNoiseOther;
		s<<isDirNoiseOther;
		s<<noiseBlockX;
		s<<noiseBlockY;
		s<<noiseBlockZ;
		s<<noiseReplace;
		s.pop();
		dirOctavesAmplitude.Save(s,IDS_BUILDKEY_DIR_OCTAVES);
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
			switch(((EmitterKeySpl*)this)->direction)
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
			rd>>draw_first_no_zbuffer;
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
			velNoiseEnable = true;
			rd>>velAmplitude;
			rd>>velFrequency;
			rd>>velOnlyPositive;
			rd>>velNoiseOther;
			rd>>isVelNoiseOther;
		}
		break;
	case IDS_BUILDKEY_VEL_OCTAVES:
		{
			velOctavesAmplitude.Load(CLoadIterator(ld));
		}
		break;
	case IDS_BUILDKEY_DIR_NOISE:
		{
			CLoadIterator rd(ld);
			dirNoiseEnable = true;
			rd>>dirAmplitude;
			rd>>dirFrequency;
			rd>>dirOnlyPositive;
			rd>>dirNoiseOther;
			rd>>isDirNoiseOther;
			rd>>noiseBlockX;
			rd>>noiseBlockY;
			rd>>noiseBlockZ;
			rd>>noiseReplace;
		}
		break;
	case IDS_BUILDKEY_DIR_OCTAVES:
		{
			dirOctavesAmplitude.Load(CLoadIterator(ld));
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
EffectKey::EffectKey()
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
	//В effect maker этот список в деструкторе должен быть всегда пустым
	vector<EmitterKeyInterface*>::iterator it;
	FOR_EACH(key,it)
		delete *it;
	key.clear();
}


void EffectKey::operator= (const EffectKey& effect_key)
{
	name = effect_key.name;
	filename=effect_key.filename;///Совсем недавно не копировалось. Может что-то сломаесться в редакторе эффектов.
	delete_assert = effect_key.delete_assert;
	need_tilemap = effect_key.need_tilemap;

	Clear();

	vector<EmitterKeyInterface*>::const_iterator it;

	FOR_EACH(effect_key.key,it)
	{
		EmitterKeyInterface* p=(*it)->Clone();
		key.push_back(p);
	}
}

void EffectKey::Save(Saver& s)
{
	s.push(IDS_EFFECTKEY);
		s.push(IDS_EFFECTKEY_HEADER);
		s<<name;
		s.pop();

		vector<EmitterKeyInterface*>::iterator it;
		FOR_EACH(key,it)
		{
			EmitterKeyInterface* p=*it;
			p->Save(s);
		}
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
			key.push_back(p);
			p->Load(ld);
		}
		break;
	case IDS_BUILDKEY_SPL:
		{
			EmitterKeySpl* p=new EmitterKeySpl;
			key.push_back(p);
			p->Load(ld);
		}
		break;
	case IDS_BUILDKEY_Z:
		{
			EmitterKeyZ* p=new EmitterKeyZ;
			key.push_back(p);
			p->Load(ld);
			need_tilemap = true;
		}
		break;
	case IDS_BUILDKEY_LIGHT:
		{
			EmitterKeyLight* p=new EmitterKeyLight;
			key.push_back(p);
			p->Load(ld);
		}
		break;
	case IDS_BUILDKEY_COLUMN_LIGHT:
		{
			EmitterKeyColumnLight* p=new EmitterKeyColumnLight;
			key.push_back(p);
			p->Load(ld);
		}
		break;
	case IDS_BUILDKEY_LIGHTING:
		{
			EmitterLightingKey* p=new EmitterLightingKey;
			key.push_back(p);
			p->Load(ld);
		}
		break;
	}
}

void EffectKey::RelativeScale(float scale)
{
	vector<EmitterKeyInterface*>::iterator it;
	FOR_EACH(key,it)
		(*it)->RelativeScale(scale);
}

void EffectKey::MulToColor(sColor4c color)
{
	vector<EmitterKeyInterface*>::iterator it;
	FOR_EACH(key,it)
	{
		(*it)->MulToColor(color);
		(*it)->BuildKey();
	}
}

void EffectKey::BuildRuntimeData()
{
	vector<EmitterKeyInterface*>::iterator it;
	FOR_EACH(key,it)
		(*it)->BuildRuntimeData();
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
	
	vector<EmitterKeyInterface*>::iterator it;
	FOR_EACH(key,it)
	{
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
			string& t= ((EmitterKeyColumnLight*)(*it))->texture2_name;
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
		CKeyPos::iterator it;
		FOR_EACH(emitter_position,it)
		{
			Vect3f& pos=it->pos;
			pos*=scale;
		}
	}

	particle_position.size*=scale;
	CKey::iterator it;
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
		CKeyPos::iterator it;
		FOR_EACH(emitter_position,it)
		{
			Vect3f& pos=it->pos;
			pos*=scale;
		}
	}

	CKey::iterator it;
	FOR_EACH(emitter_size,it)
		it->f*=scale;
}

void EmitterKeyInt::RelativeScale(float scale)
{
	EmitterKeyBase::RelativeScale(scale);

	CKey::iterator it;
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
/*
	FOR_EACH(velocity_delta, it)
		it->f*=scale;
	vector<EffectBeginSpeed>::iterator ibg;
	FOR_EACH(begin_speed, ibg)
	{
		ibg->mul*=scale;
		ibg->esp.pos*=scale;
		ibg->esp.dpos*=scale;
	}
*/

}

void EmitterKeySpl::RelativeScale(float scale)
{
	EmitterKeyBase::RelativeScale(scale);

	CKeyPosHermit::iterator it;
	FOR_EACH(p_position,it)
	{
		Vect3f& pos=it->pos;
		pos*=scale;
	}
	BuildKey();
}


bool EffectKey::IsNeed3DModel()
{
	vector<EmitterKeyInterface*>::iterator it;
	FOR_EACH(key,it)
	{
		EmitterKeyBase* p=dynamic_cast<EmitterKeyBase*>(*it);
		if(p)
		if(p->particle_position.type==EMP_3DMODEL)
			return true;
	}

	return false;
}

bool EffectKey::IsCycled()
{
	vector<EmitterKeyInterface*>::iterator it;
	FOR_EACH(key,it)
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

EmitterKeyInterface* EmitterKeySpl::Clone()
{
	EmitterKeySpl* p=new EmitterKeySpl;
	*p=*this;
	return p;
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
	vector<EmitterKeyInterface*>::iterator it;
	FOR_EACH(key,it)
		(*it)->preloadTexture();
}


void EmitterKeyInterface::preloadTexture()
{
	if(!texture_name.empty())
	{
		cTexture* pTexture=GetTexLibrary()->GetElement3D(texture_name.c_str());
		if(pTexture)pTexture->SetAttribute(TEXTURE_NO_COMPACTED);
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
				cTexture* pTexture = NULL;
				string texture_name;
				normalize_path(names[0].c_str(),texture_name);
				if (strstr(texture_name.c_str(), ".AVI"))
					pTexture=GetTexLibrary()->GetElement3DAviScale(texture_name.c_str());
				else
					pTexture=GetTexLibrary()->GetElement3D(names[0].c_str());
				if(pTexture)pTexture->SetAttribute(TEXTURE_NO_COMPACTED);
				RELEASE(pTexture);
			}
		}else
		{
			cTexture* pTexture=GetTexLibrary()->GetElement3DComplex(names);
			if(pTexture)pTexture->SetAttribute(TEXTURE_NO_COMPACTED);
			RELEASE(pTexture);
		}
	}
}

void EmitterKeyColumnLight::preloadTexture()
{
	__super::preloadTexture();

	if(!texture2_name.empty())
	{
		cTexture* pTexture=GetTexLibrary()->GetElement3D(texture2_name.c_str());
		if(pTexture)pTexture->SetAttribute(TEXTURE_NO_COMPACTED);
		RELEASE(pTexture);
	}
}

void EmitterKeyColumnLight::GetTextureNames(vector<string>& names)
{
	__super::GetTextureNames(names);
	if(!texture2_name.empty())
	{
		names.push_back(texture2_name);
	}
}

void EmitterKeyInterface::GetTextureNames(vector<string>& names)
{
	if(!texture_name.empty())
		names.push_back(texture_name);
	for(int i=0; i<num_texture_names; i++)
		if (!textureNames[i].empty())
			names.push_back(textureNames[i]);
}
