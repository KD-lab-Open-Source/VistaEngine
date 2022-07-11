#include "StdAfxRD.h"
#include "NParticle.h"
#include "scene.h"
#include <algorithm>
#include "NParticleID.h"
#include "TileMap.h"
#include "Serialization.h"
#include "SafeMath.h"
extern bool enableMirage;
static vector<Vect2f> rotate_angle;

static const float INV_2_PI=1/(2*M_PI);

float cEffect::GetParticleRateReal()const
{
	return particle_rate*Option_ParticleRate*distance_rate;
}

float cEffect::GetParticleRateRealInstantly(float num)const
{
	int inum=round(num);
	float rate=GetParticleRateReal();
	if(inum==1)
	{
		if(rate==0)
			return 0;
		return 1;
	}
		

	return num*rate;
}

class EffectKeyNoAssert:public EffectKey
{
public:
	EffectKeyNoAssert()
	{
		delete_assert=false;
	}
};

///////////////////////////cEmitterInterface///////////////////////////
cEmitterInterface::cEmitterInterface()
{
	GlobalMatrix=LocalMatrix=MatXf::ID;
	parent=NULL;

	time=0;
	totaltime = 0;
	old_time=0;
	b_pause=false;

	cycled=false;
	emitter_life_time=1;
	emitter_key = NULL;
	no_show_obj_editor=false;
	draw_first_no_zbuffer=0;
	for(int i=0;i<sizeof(Texture)/sizeof(Texture[0]);i++)
		Texture[i]=NULL;
}

cEmitterInterface::~cEmitterInterface()
{
	MTG();
	for(int i=0;i<sizeof(Texture)/sizeof(Texture[0]);i++)
		RELEASE(Texture[i]);
}

void cEmitterInterface::SetTexture(int n,cTexture *pTexture)
{
	VISASSERT(n>=0 && n<NUMBER_OBJTEXTURE);

	RELEASE(Texture[n]);
	Texture[n]=pTexture;
}


void cEmitterInterface::SetPause(bool b)
{
	b_pause=b;
}

void cEmitterInterface::SetParent(cEffect* parent_)
{
	parent=parent_;
}

void cEmitterInterface::SetCycled(bool cycled_)
{
	cycled=cycled_;
}


///////////////////////////cEmitterColumnLight////////////////////////////////

cEmitterColumnLight::cEmitterColumnLight()
{
	ut = vt =0;
	Bound.SetInvalidBox();
}
bool cEmitterColumnLight::IsLive()
{
	return time<emitter_life_time || cycled;
}
bool cEmitterColumnLight::IsVisible(cCamera *pCamera)
{
	return true;//!!!!!ѕотенциальна€ проблемма с производительностью, исправить!
}
void cEmitterColumnLight::SetDummyTime(float t)
{
}

void cEmitterColumnLight::SetEmitterKey(EmitterKeyColumnLight& k)
{
	SetTexture(0,GetTexLibrary()->GetElement3D(k.texture_name.c_str()));
	SetTexture(1,GetTexLibrary()->GetElement3D(k.texture2_name.c_str()));

	emitter_key = &k;
	cycled = k.cycled;

	switch(k.sprite_blend)
	{
	case EMITTER_BLEND_ADDING:
		blend_mode = ALPHA_ADDBLENDALPHA; break;

	case EMITTER_BLEND_SUBSTRACT:
		blend_mode = ALPHA_SUBBLEND; break;

	case EMITTER_BLEND_MULTIPLY:
		blend_mode = ALPHA_BLEND; break;
	}

	color_mode = (k.color_mode == EMITTER_BLEND_ADDING) ? COLOR_ADD : COLOR_MOD;

	emitter_life_time = k.emitter_life_time;
	height = k.height;
	ut = vt =0;
	quat_rotation = k.rot;
}

void cEmitterColumnLight::PreDraw(cCamera *pCamera)
{
}
void cEmitterColumnLight::Animate(float dt)
{
	if(b_pause)return;
	dt*=1e-3f;
	if(dt>0.1f)
		dt=0.1f;

//	if (cycled && time>emitter_life_time)
//		time = 0;
//	else 
//		time+=dt;
	float t=time/emitter_life_time;
	if(t<1.0f)
	{
		LocalMatrix.trans() = GetEmitterKeyColumnLight()->emitter_position.Get(t);
		//LocalMatrix.rot()=Mat3f::ID;
		LocalMatrix.rot()=Mat3f(quat_rotation);
		GlobalMatrix=parent->GetGlobalMatrix()*GetLocalMatrix();
		ut += GetEmitterKeyColumnLight()->u_vel.Get(t)*dt;
		vt += GetEmitterKeyColumnLight()->v_vel.Get(t)*dt;
	}else
	{
		if(cycled)
			time = 0;
	}
	time+=dt;
}
void cEmitterColumnLight::Draw(cCamera *pCamera)
{
	if(no_show_obj_editor)
		return;
	if(parent->GetParticleRateReal()<=0)
		return;
	if (!IsLive())
		return;
	float t=time/emitter_life_time;
	float size = GetEmitterKeyColumnLight()->emitter_size.Get(t);
	float size2 = GetEmitterKeyColumnLight()->emitter_size2.Get(t);
	float hgt = height.Get(t);
	sColor4f color = GetEmitterKeyColumnLight()->emitter_color.Get(t);
	color.a = GetEmitterKeyColumnLight()->emitter_alpha.Get(t).a;
	Bound.SetInvalidBox();
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	MatXf wm(GlobalMatrix);
//	if (parent->IsTarget())
//		wm.rot() = Mat3f(rot);
//	else
//		wm.rot() *= Mat3f(rot);

	cVertexBuffer<sVertexXYZDT2>* pBuf  = rd->GetBufferXYZDT2();
	float ut1 = 0;
	float vt1 = 0;
	gb_RenderDevice->SetSamplerDataVirtual(0,sampler_clamp_linear);
	gb_RenderDevice->SetSamplerDataVirtual(1,sampler_wrap_linear);
	//gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE );
	bool reflectionz=pCamera->GetAttr(ATTRCAMERA_REFLECTION);
	{//Ќемного не к месту, зато быстро по скорости, дл€ отражений.
		gb_RenderDevice3D->SetTexture(5,pCamera->GetZTexture());
		gb_RenderDevice3D->SetSamplerData(5,sampler_clamp_linear);
	}
	gb_RenderDevice3D->SetWorldMaterial(blend_mode, wm, 0, GetTexture(0), GetTexture(1), color_mode,false,reflectionz);
	if (!GetTexture(0))
	{
		gb_RenderDevice3D->SetWorldMaterial(blend_mode, wm, 0, GetTexture(1), NULL);
		gb_RenderDevice->SetSamplerDataVirtual(0,sampler_wrap_linear);
		ut1 = ut;
		vt1 = vt;
	}
#ifdef NEED_TREANGLE_COUNT
	if (parent->drawOverDraw)
	{
		gb_RenderDevice3D->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
		gb_RenderDevice3D->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
		gb_RenderDevice3D->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
		parent->psOverdraw->Select();
	}
#endif
	if(!GetEmitterKeyColumnLight()->plane)
	{
		static const int na = 13;
		static Vect3f rv[na] = 
		{
			Vect3f(0, 1, 0),
			Vect3f(0.5f, 0.866f, 0),
			Vect3f(0.866f, 0.5f, 0),
			Vect3f(1, 0, 0),
			Vect3f(0.866f, -0.5f, 0),
			Vect3f(0.5f, -0.866f, 0),
			Vect3f(0, -1, 0),
			Vect3f(-0.5f, -0.866f, 0),
			Vect3f(-0.866f, -0.5f, 0),
			Vect3f(-1, 0, 0),
			Vect3f(-0.866f, 0.5f, 0),
			Vect3f(-0.5f, 0.866f, 0),
			Vect3f(0, 1, 0),
		};

		const int nVertex = na*2;
		sVertexXYZDT2* vertex = pBuf->Lock(nVertex);
		sVertexXYZDT2 *v = vertex;
		Vect3f* cr = rv;
		float u = 0;
		static const float du = 1.0f/(na-1);
		for(int i=0; i<na; i++, cr++)
		{
			v->pos = size*(*cr);
			v->diffuse = color;
			//v->GetTexel().set(u,0);
			v->GetTexel().set(u+ut1,vt1);
			v->GetTexel2().set(u+ut,vt);
			v++; 
			v->pos = size2*(*cr);
			v->pos.z+=hgt;
			v->diffuse = color;
			//v->GetTexel().set(u,1.0f);
			v->GetTexel().set(u+ut1,1.0f+vt1);
			v->GetTexel2().set(u+ut,1.0f+vt);
			v++;
			u+=du;
		}
		pBuf->Unlock(nVertex);
		if(nVertex>0)
			pBuf->DrawPrimitive(PT_TRIANGLESTRIP,nVertex-2);
		#ifdef  NEED_TREANGLE_COUNT
			parent->AddCountTriangle(na);
		#endif
	}else
	{
		Vect3f sx1,sx2,sx3;
		if(GetEmitterKeyColumnLight()->laser)
		{
			sx1.set(1,0,0);
			sx2.set(0.5f,0.86f,0);
			sx3.set(-0.5f,0.86f,0);
			//sx2 *= size;
			//sx3 *= size;

			sVertexXYZDT2* vx = pBuf->Lock(18);

			vx[0].pos=-sx1*size;						 
			vx[1].pos=-sx1*size2+Vect3f(0,0,hgt);	 
			vx[2].pos=sx1*size;						 

			vx[3].pos=sx1*size;						 
			vx[4].pos=-sx1*size2+Vect3f(0,0,hgt);	 
			vx[5].pos=sx1*size2+Vect3f(0,0,hgt);	 

			//vx[0].GetTexel().set(0,0);
			//vx[1].GetTexel().set(0,1);
			//vx[2].GetTexel().set(1,0);

			//vx[3].GetTexel().set(1,0);
			//vx[4].GetTexel().set(0,1);
			//vx[5].GetTexel().set(1,1);

			vx[0].GetTexel().set(ut1,vt1);
			vx[1].GetTexel().set(ut1,vt1+1);
			vx[2].GetTexel().set(ut1+1,vt1);

			vx[3].GetTexel().set(ut1+1,vt1);
			vx[4].GetTexel().set(ut1,vt1+1);
			vx[5].GetTexel().set(ut1+1,vt1+1);

			vx[0].GetTexel2().set(ut,vt);
			vx[1].GetTexel2().set(ut,vt+1);
			vx[2].GetTexel2().set(ut+1,vt);

			vx[3].GetTexel2().set(ut+1,vt);
			vx[4].GetTexel2().set(ut,vt+1);
			vx[5].GetTexel2().set(ut+1,vt+1);

			vx[6].pos=-sx2*size;						 
			vx[7].pos=-sx2*size2+Vect3f(0,0,hgt);	 
			vx[8].pos=sx2*size;						 

			vx[9].pos=sx2*size;						 
			vx[10].pos=-sx2*size2+Vect3f(0,0,hgt);	 
			vx[11].pos=sx2*size2+Vect3f(0,0,hgt);	 

			//vx[6].GetTexel().set(0,0);
			//vx[7].GetTexel().set(0,1);
			//vx[8].GetTexel().set(1,0);

			//vx[9].GetTexel().set(1,0);
			//vx[10].GetTexel().set(0,1);
			//vx[11].GetTexel().set(1,1);

			vx[6].GetTexel().set(ut1,vt1);
			vx[7].GetTexel().set(ut1,vt1+1);
			vx[8].GetTexel().set(ut1+1,vt1);

			vx[9].GetTexel().set(ut1+1,vt1);
			vx[10].GetTexel().set(ut1,vt1+1);
			vx[11].GetTexel().set(ut1+1,vt1+1);

			vx[6].GetTexel2().set(ut,vt);
			vx[7].GetTexel2().set(ut,vt+1);
			vx[8].GetTexel2().set(ut+1,vt);

			vx[9].GetTexel2().set(ut+1,vt);
			vx[10].GetTexel2().set(ut,vt+1);
			vx[11].GetTexel2().set(ut+1,vt+1);

			vx[12].pos=-sx3*size;						 
			vx[13].pos=-sx3*size2+Vect3f(0,0,hgt);	 
			vx[14].pos=sx3*size;						 

			vx[15].pos=sx3*size;						 
			vx[16].pos=-sx3*size2+Vect3f(0,0,hgt);	 
			vx[17].pos=sx3*size2+Vect3f(0,0,hgt);	 

			//vx[12].GetTexel().set(0,0);
			//vx[13].GetTexel().set(0,1);
			//vx[14].GetTexel().set(1,0);

			//vx[15].GetTexel().set(1,0);
			//vx[16].GetTexel().set(0,1);
			//vx[17].GetTexel().set(1,1);

			vx[12].GetTexel().set(ut1,vt1);
			vx[13].GetTexel().set(ut1,vt1+1);
			vx[14].GetTexel().set(ut1+1,vt1);

			vx[15].GetTexel().set(ut1+1,vt1);
			vx[16].GetTexel().set(ut1,vt1+1);
			vx[17].GetTexel().set(ut1+1,vt1+1);

			vx[12].GetTexel2().set(ut,vt);
			vx[13].GetTexel2().set(ut,vt+1);
			vx[14].GetTexel2().set(ut+1,vt);

			vx[15].GetTexel2().set(ut+1,vt);
			vx[16].GetTexel2().set(ut,vt+1);
			vx[17].GetTexel2().set(ut+1,vt+1);

			for(int i=0;i<18;i++)
				vx[i].diffuse=color;

			pBuf->Unlock(18);
			pBuf->DrawPrimitive(PT_TRIANGLELIST, 6);
		}else
		{
			if(GetEmitterKeyColumnLight()->turn)
			{
				Vect3f ctp = pCamera->GetPos(); ctp.z = 0;
				ctp = wm.invXformPoint(ctp);
				FastNormalize(ctp);
				sx1 = (ctp%wm.rot().zrow());
			}else
				sx1.set(0,1,0);
			float dsize = size-size2;
			sVertexXYZDT2* vx;
			vx = pBuf->Lock(8);
			vx[0].pos = Vect3f(0,0,0);
			vx[1].pos = -sx1*size;
			vx[2].pos = Vect3f(0,0,0.33f*hgt);
			vx[3].pos = -sx1*(size-dsize*0.33f);
			vx[3].pos.z = 0.33f*hgt;
			vx[4].pos = Vect3f(0,0,0.66f*hgt);
			vx[5].pos = -sx1*(size-dsize*0.66f);
			vx[5].pos.z = 0.66f*hgt;
			vx[6].pos = Vect3f(0,0,hgt);
			vx[7].pos = -sx1*size2;
			vx[7].pos.z = hgt;
			
			//vx[0].GetTexel().set(0.5f,0);
			//vx[1].GetTexel().set(0,0);
			//vx[2].GetTexel().set(0.5f,0.33f);
			//vx[3].GetTexel().set(0,0.33f);
			//vx[4].GetTexel().set(0.5f,0.66f);
			//vx[5].GetTexel().set(0,0.66f);
			//vx[6].GetTexel().set(0.5f,1);
			//vx[7].GetTexel().set(0,1);
			
			vx[0].GetTexel().set(ut1+0.5f,vt1);
			vx[1].GetTexel().set(ut1,vt1);
			vx[2].GetTexel().set(ut1+0.5f,vt1+0.33f);
			vx[3].GetTexel().set(ut1,vt1+0.33f);
			vx[4].GetTexel().set(ut1+0.5f,vt1+0.66f);
			vx[5].GetTexel().set(ut1,vt1+0.66f);
			vx[6].GetTexel().set(ut1+0.5f,vt1+1);
			vx[7].GetTexel().set(ut1,vt1+1);

			vx[0].GetTexel2().set(ut+0.5f,vt);
			vx[1].GetTexel2().set(ut,vt);
			vx[2].GetTexel2().set(ut+0.5f,vt+0.33f);
			vx[3].GetTexel2().set(ut,vt+0.33f);
			vx[4].GetTexel2().set(ut+0.5f,vt+0.66f);
			vx[5].GetTexel2().set(ut,vt+0.66f);
			vx[6].GetTexel2().set(ut+0.5f,vt+1);
			vx[7].GetTexel2().set(ut,vt+1);
			for(int i=0;i<8;i++)
				vx[i].diffuse=color;
			pBuf->Unlock(8);
			pBuf->DrawPrimitive(PT_TRIANGLESTRIP, 6);

			vx = pBuf->Lock(8);
			vx[0].pos = Vect3f(0,0,0);
			vx[1].pos = sx1*size;
			vx[2].pos = Vect3f(0,0,0.33f*hgt);
			vx[3].pos = sx1*(size-dsize*0.33f);
			vx[3].pos.z = 0.33f*hgt;
			vx[4].pos = Vect3f(0,0,0.66f*hgt);
			vx[5].pos = sx1*(size-dsize*0.66f);
			vx[5].pos.z = 0.66f*hgt;
			vx[6].pos = Vect3f(0,0,hgt);
			vx[7].pos = sx1*size2;
			vx[7].pos.z = hgt;
			//vx[0].GetTexel().set(0.5f,0);
			//vx[1].GetTexel().set(1,0);
			//vx[2].GetTexel().set(0.5f,0.33f);
			//vx[3].GetTexel().set(1,0.33f);
			//vx[4].GetTexel().set(0.5f,0.66f);
			//vx[5].GetTexel().set(1,0.66f);
			//vx[6].GetTexel().set(0.5f,1);
			//vx[7].GetTexel().set(1,1);
			
			vx[0].GetTexel().set(ut1+0.5f,vt1);
			vx[1].GetTexel().set(ut1+1,vt1);
			vx[2].GetTexel().set(ut1+0.5f,vt1+0.33f);
			vx[3].GetTexel().set(ut1+1,vt1+0.33f);
			vx[4].GetTexel().set(ut1+0.5f,vt1+0.66f);
			vx[5].GetTexel().set(ut1+1,vt1+0.66f);
			vx[6].GetTexel().set(ut1+0.5f,vt1+1);
			vx[7].GetTexel().set(ut1+1,vt1+1);

			vx[0].GetTexel2().set(ut+0.5f,vt);
			vx[1].GetTexel2().set(ut+1,vt);
			vx[2].GetTexel2().set(ut+0.5f,vt+0.33f);
			vx[3].GetTexel2().set(ut+1,vt+0.33f);
			vx[4].GetTexel2().set(ut+0.5f,vt+0.66f);
			vx[5].GetTexel2().set(ut+1,vt+0.66f);
			vx[6].GetTexel2().set(ut+0.5f,vt+1);
			vx[7].GetTexel2().set(ut+1,vt+1);
			for(int i=0;i<8;i++)
				vx[i].diffuse=color;
			pBuf->Unlock(8);
			pBuf->DrawPrimitive(PT_TRIANGLESTRIP, 6);
		}
		

		#ifdef  NEED_TREANGLE_COUNT
			parent->AddCountTriangle(2);
		#endif
	}
}
void cEmitterColumnLight::SetTarget(const Vect3f* pos_end,int pos_end_size)
{
	Vect3f n = pos_end[0] - parent->GetPosition().trans();
	float maxHeight = n.norm();
	float k = maxHeight / max(height.back().f, 1e-20f);
	for (int i=0; i<height.size(); i++)
	{
		height[i].f *= k;
	}

	//quat_rotation.set(QuatF::ID);
	//quat_rotation.set(M_PI/2,Vect3f::J);
	quat_rotation.set(-M_PI/2,Vect3f::I);
}
///////////////////////////cEmitterBase////////////////////////////////
cEmitterBase::cEmitterBase()
{
	parent=NULL;
	SetMaxTime(4.0f,4.0f);
	Bound.SetInvalidBox();
	InitRotateAngle();
	SetDummyTime(0.2f);
	time_emit_prolonged=0;
	disable_emit_prolonged=false;
	init_prev_matrix=false;
	cur_one_pos=-1;
	other = NULL;
	isOther = false;
	velNoiseOther = NULL;
	dirNoiseOther = NULL;
	scaleX = scaleY = 1.0f;
	sizeByTexture = false;
	mirage = false;
	oldZBufferWrite = false;
	softSmoke = false;
	ignoreParticleRate = false;
}

cEmitterBase::~cEmitterBase()
{
}

void cEmitterBase::InitRotateAngle()
{
	if(!rotate_angle.empty())return;
	rotate_angle.resize(rotate_angle_size);
	for(int i=0;i<rotate_angle_size;i++)
	{
		float a=(i*2*M_PI)/rotate_angle_size;
		Vect2f& p=rotate_angle[i];
		p.x=cos(a);
		p.y=sin(a);
	}
}

void cEmitterBase::SetMaxTime(float emitter_life,float particle_life)
{
	emitter_life_time=emitter_life;
	inv_emitter_life_time=1/max(emitter_life,0.001f);

	particle_life_time=particle_life;
	inv_particle_life_time=1/max(particle_life,0.001f);
}

Vect3f cEmitterBase::RndBox(float x,float y,float z)
{
	Vect3f f;
	if(0)
	{
		f.x=graphRnd.frnd(x*0.5f);
		f.y=graphRnd.frnd(y*0.5f);
		f.z=graphRnd.frnd(z*0.5f);
	}else
	{
		int rrr=graphRnd();
		float sgn=rrr&1?0.5f:-0.5f;
		int n=(rrr>>1)%3;
		switch(n)
		{
		case 0:
			f.x=graphRnd.frnd(x*0.5f);
			f.y=graphRnd.frnd(y*0.5f);
			f.z=z*sgn;
			break;
		case 1:
			f.x=graphRnd.frnd(x*0.5f);
			f.y=y*sgn;
			f.z=graphRnd.frnd(z*0.5f);
			break;
		case 2:
			f.x=x*sgn;
			f.y=graphRnd.frnd(y*0.5f);
			f.z=graphRnd.frnd(z*0.5f);
			break;
		}
	}

	return f;
}

Vect3f cEmitterBase::RndSpere(float r)
{
	Vect3f v(graphRnd.frand()-0.5f,graphRnd.frand()-0.5f,graphRnd.frand()-0.5f);
	v.Normalize();
	return r*v;
}

Vect3f cEmitterBase::RndCylinder(float r1,float r2,float dr,float z,bool btm)
{
	Vect3f f;
	float a=graphRnd.frand()*2*M_PI;
	float rr=r1+dr*(graphRnd.frand()-0.5f);
	float ddr = graphRnd.frand();
	rr = rr + ddr*(r2-r1);
	f.z=z*(ddr-(btm?0:0.5f));
	f.x=rr*sin(a);
	f.y=rr*cos(a);
	return f;
}

Vect3f cEmitterBase::RndRing(float r,float alpha_min,float alpha_max,float teta_min,float teta_max)
{
	Vect3f f;
	float teta=teta_min+graphRnd.frand()*(teta_max-teta_min);
	float alpha=alpha_min+graphRnd.frand()*(alpha_max-alpha_min);
	f.x=r*sin(alpha)*cos(teta);
	f.y=r*cos(alpha)*cos(teta);
	f.z=r*sin(teta);
	return f;
}

bool cEmitterBase::IsVisible(cCamera *pCamera)
{
	if(Bound.min.x>Bound.max.x)
		return false;
	if(GetEmitterKeyBase()->relative)
		return pCamera->TestVisible(GlobalMatrix, Bound.min, Bound.max);
	else 
		return pCamera->TestVisible(Bound.min, Bound.max);
	return false;
}

void cEmitterBase::PreDraw(cCamera *pCamera)
{
}
void cEmitterBase::Animate(float dt)
{
	if(b_pause)return;
	dt*=1e-3f;
	if(dt>0.1f)
		dt=0.1f;
	CalcPosition();

	if(cycled && time+dt>emitter_life_time)
	{
		DummyQuant();
//		time-=emitter_life_time;
		time = 0;
		old_time=time;
	}

	if(!GetEmitterKeyBase()->generate_prolonged)
	{
		float tmin=time*inv_emitter_life_time;
		float tmax=(time+dt)*inv_emitter_life_time;
		EmitInstantly(tmin,tmax);
	}

	if(!disable_emit_prolonged)
		if(GetEmitterKeyBase()->generate_prolonged)
			EmitProlonged(dt);


	if(time-old_time>dummy_time || time==0 || isOther)
		DummyQuant();

	time+=dt;
	totaltime += dt;
	init_prev_matrix=true;
	prev_matrix=GlobalMatrix;
}

void cEmitterBase::BeginInterpolateGlobalMatrix()
{
	next_matrix=GlobalMatrix;
}

void cEmitterBase::InterpolateGlobalMatrix(float f)
{
	if(!init_prev_matrix)
		return;

	MatrixInterpolate(GlobalMatrix,prev_matrix,next_matrix,f);
}

void cEmitterBase::EndInterpolateGlobalMatrix()
{
	GlobalMatrix=next_matrix;
}

void cEmitterBase::CalcPosition()
{
	float t=time*inv_emitter_life_time;
	LocalMatrix.trans() = GetEmitterKeyBase()->emitter_position.Get(t);
	LocalMatrix.rot() = GetEmitterKeyBase()->emitter_rotation.Get(t);
	switch(GetEmitterKeyBase()->particle_position.type)
	{
	case EMP_3DMODEL:
		GlobalMatrix=parent->GetCenter3DModel()*GetLocalMatrix();
		break;
	default:
	case EMP_3DMODEL_INSIDE:
		GlobalMatrix=parent->GetGlobalMatrix()*GetLocalMatrix();
		break;
	}
}

void cEmitterBase::SetEmitterKey(EmitterKeyBase& k,c3dx* models)
{
	start_timer_auto();
	vector<string> textureNames;
	for(int i=0; i<k.num_texture_names; i++)
		if (!k.textureNames[i].empty())
			textureNames.push_back(k.textureNames[i]);

	if (textureNames.size()==0)
		SetTexture(0,NULL);
	else
	if (textureNames.size()==1 || !k.randomFrame)
	{
		string texture_name;
		normalize_path(textureNames[0].c_str(),texture_name);
		if (strstr(texture_name.c_str(), ".AVI"))
			SetTexture(0,GetTexLibrary()->GetElement3DAviScale(textureNames[0].c_str()));
		else
			SetTexture(0,GetTexLibrary()->GetElement3D(textureNames[0].c_str()));
	}else
	{
		SetTexture(0,GetTexLibrary()->GetElement3DComplex(textureNames));
	}

	sizeByTexture = k.sizeByTexture;
	cTexture* texture = GetTexture();
	if(sizeByTexture && texture!=NULL)
	{
		int w = texture->GetWidth();
		int h = texture->GetHeight();
		if (h>w)
		{
			scaleY = float(h)/float(w);
		}else if(w>h)
		{
			scaleX = float(w)/float(h);
		}
	}

	emitter_key = &k;

	mirage = k.mirage;
	softSmoke = k.softSmoke;

	SetMaxTime(k.emitter_life_time,k.particle_life_time);

	cycled=k.cycled;

	switch(k.sprite_blend)
	{
	case EMITTER_BLEND_ADDING:
		blend_mode = ALPHA_ADDBLENDALPHA; break;

	case EMITTER_BLEND_SUBSTRACT:
		blend_mode=ALPHA_SUBBLEND; break;

	case EMITTER_BLEND_MULTIPLY:
		blend_mode=ALPHA_BLEND; break;
	}

	GlobalMatrix.trans() = LocalMatrix.trans() = GetEmitterKeyBase()->emitter_position.Get(0);
	GlobalMatrix.rot() = LocalMatrix.rot() = GetEmitterKeyBase()->emitter_rotation.Get(0);
	draw_first_no_zbuffer=k.draw_first_no_zbuffer;
	k.velNoise.SetParameters(14,  k.velFrequency, k.velAmplitude, k.velOctavesAmplitude, k.velOnlyPositive, true);
	k.dirNoiseX.SetParameters(14, k.dirFrequency, k.dirAmplitude, k.dirOctavesAmplitude, k.dirOnlyPositive, true);
	k.dirNoiseY.SetParameters(14, k.dirFrequency, k.dirAmplitude, k.dirOctavesAmplitude, k.dirOnlyPositive, true);
	k.dirNoiseZ.SetParameters(14, k.dirFrequency, k.dirAmplitude, k.dirOctavesAmplitude, k.dirOnlyPositive, true);
	if (k.k_wind_max <0)
		k.k_wind_max = k.k_wind_min;

	ignoreParticleRate = k.ignoreParticleRate;

}
inline Vect3f* cEmitterBase::GetNormal(const int& ix)
{
	switch(GetEmitterKeyBase()->particle_position.type)
	{
	case EMP_3DMODEL:
		if ((UINT)ix<parent->GetNorm().size())
			return &parent->GetNorm()[ix];
		break;
	case EMP_3DMODEL_INSIDE:
		if ((UINT)ix < GetEmitterKeyBase()->begin_position.size())
			return &GetEmitterKeyBase()->normal_position[ix];
		break;
	}
	return NULL;
}

void cEmitterBase::OneOrderedPos(int i,Vect3f& pos)
{
	EmitterType & particle_position = GetEmitterKeyBase()->particle_position;
	EmitterType::CountXYZ & num = particle_position.num;
	float size_pos=100;
	switch(particle_position.type)
	{
		case EMP_BOX:
		{
			i %= num.x*num.y*num.z;
			int ix = i%num.x;
			int iy = (i/num.x)%num.y;
			int iz = i/(num.x*num.y);
			pos.x = ix*(particle_position.size.x/(num.x<=1? 1: num.x-1)); 
			pos.y = iy*(particle_position.size.y/(num.y<=1? 1: num.y-1)); 
			pos.z = iz*(particle_position.size.z/(num.z<=1? 1: num.z-1)); 
			if(!GetEmitterKeyBase()->chFill)
			{
				if ((ix!=0&&iy!=0&&iz!=0)&&(ix!=num.x-1&&iy!=num.y-1&&iz!=num.z-1))
				{
					int t = graphRnd(3);
					if (t!=0)pos.x = ix%2 ? 0 : particle_position.size.x;
					if (t!=1)pos.y = iy%2 ? 0 : particle_position.size.y;
					if (t!=2)pos.z = iz%2 ? 0 : particle_position.size.z;
				}
			}
			pos -= particle_position.size/2;
		}
			break;
		case EMP_CYLINDER:
		{
			i %= num.y*num.x*num.z;
			float alpha = (i%num.x)*2*M_PI/num.x;
			pos.z = ((i / num.x) % num.y) * (particle_position.size.y / (num.y <= 1 ? 1 : num.y - 1)) - (GetEmitterKeyBase()->bottom ? 0 : particle_position.size.y / 2); 
			float dx=0,dz= 0; 
			if(GetEmitterKeyBase()->cone)
			{
				dx = particle_position.size.z-particle_position.size.x;
				dz = pos.z / particle_position.size.y;
			}
			float r = (i/(num.x*num.y)+1)*((particle_position.size.x+dx*dz)/num.z);

			pos.x = cos(alpha)*r;
			pos.y = -sin(alpha)*r;
		}
			break;
		case EMP_SPHERE:
		{
			i %= num.x*num.y*num.z;
			float alpha = (i%num.x)*2*M_PI/num.x;
			float theta = ((i/num.x)%num.y)*2*M_PI/num.y;
			float r = (i/(num.x*num.y)+1)*(particle_position.size.x/num.z);
			pos.setSpherical(alpha,theta,r);
		}
			break;
		case EMP_RING:
		{
			i %= num.x*num.y*num.z;
			float alpha = (i%num.x)*(particle_position.alpha_max-particle_position.alpha_min)/num.x + particle_position.alpha_min;
			float theta = ((i/num.x)%num.y)*(particle_position.teta_max - particle_position.teta_min)/num.y+ particle_position.teta_min;
			float r = (i/(num.x*num.y)+1)*(particle_position.size.x/num.z);
			pos.x=r*sin(alpha)*cos(theta);
			pos.y=r*cos(alpha)*cos(theta);
			pos.z=r*sin(theta);
		}
			break;
	}
	pos*=size_pos;
}
bool cEmitterBase::OnePos(int i,Vect3f& pos, Vect3f* norm /*= NULL*/, int num_pos)
{
	EmitterType & particle_position = GetEmitterKeyBase()->particle_position;

	cur_one_pos=-1;
	float size_pos=100;
	if(GetEmitterKeyBase()->chFill)
		size_pos = graphRnd(size_pos);
	if (other)
		return !other->GetRndPos(pos, norm);
	else switch(particle_position.type)
	{
	default:
	case EMP_BOX:
		if (particle_position.fix_pos)	OneOrderedPos(i, pos);
		else pos=size_pos*RndBox(particle_position.size.x,particle_position.size.y,particle_position.size.z);
		if(norm) 
		{
			*norm = pos;
		}
		break;
	case EMP_SPHERE:
		if (particle_position.fix_pos)	OneOrderedPos(i, pos);
		else pos=size_pos*RndSpere(particle_position.size.x);
		if(norm) 
		{
			*norm = pos;
		}
		break;
	case EMP_CYLINDER:
		if (particle_position.fix_pos)	OneOrderedPos(i, pos);
		else pos = size_pos * RndCylinder(particle_position.size.x, GetEmitterKeyBase()->cone ? particle_position.size.z : particle_position.size.x, 0, particle_position.size.y, GetEmitterKeyBase()->bottom);
		if(norm) 
		{
			norm->set(pos.x,pos.y,0);
		}
		break;
	case EMP_LINE:
		pos.x=0;
		pos.y=0;
		pos.z=(graphRnd.frand()-0.5f)*size_pos*particle_position.size.x;
		if(norm)
		{
			norm->set(graphRnd.frand(),graphRnd.frand(),0);
		}
		break;
	case EMP_RING:
		if (particle_position.fix_pos)	OneOrderedPos(i, pos);
		else pos=size_pos*RndRing(particle_position.size.x,
						particle_position.alpha_min,particle_position.alpha_max,
						particle_position.teta_min,particle_position.teta_max);
		if(norm)
		{
			*norm = pos;
		}
		break;
	case EMP_3DMODEL:
		if(!parent->GetPos().empty())
		{
			if (num_pos >= 0)
				cur_one_pos = num_pos;
			else
				cur_one_pos=graphRnd()%parent->GetPos().size();
			xassert(cur_one_pos<parent->GetPos().size());
			pos=parent->GetPos()[cur_one_pos];
		}else
			pos.set(0,0,0);
		if (norm)
		{
			if (parent->GetNorm().empty())
				norm->set(0,0,0);
			else *norm = parent->GetNorm()[cur_one_pos];
		}
		break;
	case EMP_3DMODEL_INSIDE:
		if(!GetEmitterKeyBase()->begin_position.empty())
		{
			cur_one_pos = graphRnd() % GetEmitterKeyBase()->begin_position.size();
			pos = GetEmitterKeyBase()->begin_position[cur_one_pos];
		}else
			pos.set(0,0,0);
		if (norm && cur_one_pos < GetEmitterKeyBase()->normal_position.size())
			if (parent->GetNorm().empty())
				norm->set(0,0,0);
			else 
				*norm = GetEmitterKeyBase()->normal_position[cur_one_pos];
		break;
	}
	xassert(pos.x<1e20);
	return true;
}
int cEmitterBase::CalcProlongedNum(float dt)
{
	if(time>=emitter_life_time)return 0;
	float t=time*inv_emitter_life_time;
	float rate=max(GetEmitterKeyBase()->num_particle.Get(t),1.0f)*((ignoreParticleRate&&parent->GetParticleRateReal()>FLT_EPS)?1:parent->GetParticleRateReal());
	time_emit_prolonged+=dt;
	int n=round(time_emit_prolonged*rate);
	if(rate>1e-3f)
		time_emit_prolonged-=n/rate;
	else
		time_emit_prolonged=0;

	return n;
}

//////////////////////////cEmitterInt/////////////////////////////////////
cEmitterInt::cEmitterInt()
{
	real_angle = 0;
	is_intantly_infinity_particle=false;
}
cEmitterInt::~cEmitterInt()
{
}

void cEmitterInt::CalcIntantlyInfinityParticle()
{
	is_intantly_infinity_particle=false;

	if(!IsCycled())
		return;

	if(GetEmitterKeyInt()->generate_prolonged)
		return;

	CKey & num_particle = GetEmitterKeyInt()->num_particle;
	if(!(num_particle.size()==1 || num_particle.size()==2))
		return;

	KeyFloat pos=num_particle[0];
	int num=round(pos.f);
	if(num!=1)
		return;

	xassert(pos.time<1e-3f);
	
	float inv_life = GetEmitterKeyInt()->inv_life_time.Get(pos.time);
	float dlife = GetEmitterKeyInt()->life_time_delta.Get(pos.time);
	if(inv_life<1e-3f)
		return;

	float particle_life_time=1/inv_life;

	if(fabsf(emitter_life_time-particle_life_time)>1e-3f)
		return;

	vector<KeyParticleInt> & keys = GetEmitterKeyInt()->GetKey();
	for(int i=0; i<keys.size(); i++)
	{
		KeyParticleInt* k0=&keys[i];
		if(k0->vel>0)
			return;
	}

	is_intantly_infinity_particle=true;
}

bool cEmitterInt::SetFreeOrCycle(int index)
{
	nParticle& p=Particle[index];
	if(is_intantly_infinity_particle && IsCycled())
	{
		p.angle0 = real_angle;
		p.key=0;
		p.time=0;
		return false;
	}else
	{
		Particle.SetFree(index);
		p.key = -1;
		return true;
	}
}

bool cPlume::PutToBuf(Vect3f& npos, float& dt,
					cQuadBuffer<sVertexXYZDT1>*& pBuf, 
					const sColor4c& color, const Vect3f& PosCamera,
					const float& scale, const sRectangle4f& rt,
					const UCHAR mode,float PlumeInterval,float time_summary)
{
	float InvPlumeInterval=1/PlumeInterval;
	//ƒобавл€ем новые точки.
	float time_one_segment=PlumeInterval/(plumes.size()-2);
	{
		if(plumes_size==0)
		{
			plumes[0].pos=npos;
			plumes[0].time_summary=time_summary;
			plumes[0].scale=scale;
			plumes[1]=plumes[0];
			plumes_size=2;
		}else
		{
			xassert(begin_index+plumes_size-2>=0);
			cPlumeOne& pre_last=plumes[(begin_index+plumes_size-2)%plumes.size()];
			if(time_summary-pre_last.time_summary>=time_one_segment)
			{
				cPlumeOne& last=plumes[(begin_index+plumes_size-1)%plumes.size()];
				last.pos=npos;
				last.time_summary=time_summary;
				last.scale=scale;

				if(plumes_size<plumes.size())
					plumes_size++;
				else
					begin_index=(begin_index+1)%plumes.size();
			}
		}

		xassert(begin_index+plumes_size-1>=0);
		cPlumeOne& last=plumes[(begin_index+plumes_size-1)%plumes.size()];
		last.pos=npos;
		last.time_summary=time_summary;
		last.scale=scale;
	}


	Vect3f Tangent(0,0,0);
	int iprev=begin_index;
	sVertexXYZDT1 p1,p2;
	for(int counter=0;counter<plumes_size;counter++)
	{
		int i=begin_index+counter;
		if(i>=plumes.size())
			i-=plumes.size();
		cPlumeOne& p=plumes[i];

		sVertexXYZDT1* v;
		if(counter>0)
		{
			v = pBuf->Get();
			v[0]=p1;
			v[1]=p2;

			Tangent.sub(p.pos,plumes[iprev].pos);
		}else
		{
			Tangent.sub(plumes[(i+1)%plumes.size()].pos,p.pos);
		}
		FastNormalize(Tangent);

		float phase=(time_summary-p.time_summary)*InvPlumeInterval;
		//ѕо хорошему надо клиповать phase и сдвигать начальную точку,
		//тогда можно будет корректно пользоватьс€ sRectangle4f& rt
		//Ќужно только тестиков по этому поводу написать.
		
		p1.u1()=p2.u1()=phase;
		p1.v1()=0;p2.v1()=1;

		p1.diffuse=color;
	//	p1.diffuse.a=round(color.a*(1-phase));
		p2.diffuse=p1.diffuse;

		Vect3f vCameraToObject;
		if(mode&1)
			vCameraToObject.set(0,0,1);
		else
			vCameraToObject.sub(PosCamera,p.pos);

		FastNormalize(vCameraToObject);
		Vect3f Orientation;
		Orientation.cross(vCameraToObject,Tangent);
		Orientation*=p.scale;

		p1.pos.sub(p.pos,Orientation);
		p2.pos.add(p.pos,Orientation);

		if(counter>0)
		{//Ёмулируем квадами стрип.
			v[2]=p1;
			v[3]=p2;
		}

		iprev=i;
	}

	return false;
}

struct SHORT_INDEX
{
	WORD z;
	WORD index;
};

inline bool operator<(SHORT_INDEX a,SHORT_INDEX b)
{
	return a.z>b.z;
}

void cEmitterInt::Draw(cCamera *pCamera)
{
	MTG();
	if(no_show_obj_editor)
		return;
	if(is_intantly_infinity_particle && parent->GetParticleRateReal()<=0 )
		return;
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	float dtime_global=(time-old_time);
	MatXf mat=pCamera->GetMatrix();
	//Bound.SetInvalidBox();
	if(GetEmitterKeyInt()->relative)
	{
		Bound.max.set(0,0,0);
		Bound.min.set(0,0,0);
	}else
	{
		Bound.max = GlobalMatrix.trans();
		Bound.min = GlobalMatrix.trans();
	}

	cTextureAviScale* texture = NULL;
	cTextureComplex* textureComplex = NULL;
//	cTextureAviScale* plume_texture = NULL;
	if (GetTexture(0) && GetTexture(0)->IsAviScaleTexture())
		texture = (cTextureAviScale*)GetTexture(0);
	if (GetTexture(0) && GetTexture(0)->IsComplexTexture())
		textureComplex = (cTextureComplex*)GetTexture(0);

	TerraInterface* terra = NULL;
	if (pCamera->GetScene()->GetTileMap())
		terra = pCamera->GetScene()->GetTileMap()->GetTerra();
	cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();

	gb_RenderDevice->SetSamplerDataVirtual(0,GetEmitterKeyInt()->chPlume?sampler_clamp_linear:sampler_wrap_linear);
	bool reflectionz=pCamera->GetAttr(ATTRCAMERA_REFLECTION);
	{//Ќемного не к месту, зато быстро по скорости, дл€ отражений.
		gb_RenderDevice3D->SetTexture(5,pCamera->GetZTexture());
		gb_RenderDevice3D->SetSamplerData(5,sampler_clamp_linear);
	}
	gb_RenderDevice3D->SetWorldMaterial(blend_mode, GetEmitterKeyInt()->relative ? GlobalMatrix:MatXf::ID, 0, GetTexture(0),0,COLOR_MOD,softSmoke,reflectionz);
#ifdef NEED_TREANGLE_COUNT
	if (parent->drawOverDraw)
	{
		gb_RenderDevice3D->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
		gb_RenderDevice3D->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
		gb_RenderDevice3D->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
		parent->psOverdraw->Select();
	}
#endif
	Vect3f CameraPos;
	if(GetEmitterKeyInt()->relative)
	{
		if(GetEmitterKeyInt()->chPlume)
		{
			MatXf invGlobMatrix(GlobalMatrix);
			invGlobMatrix.invert();
			CameraPos = invGlobMatrix*pCamera->GetPos();
		}
		mat = mat*GlobalMatrix;
		pBuf->BeginDraw();
	}else
	{
		if(GetEmitterKeyInt()->chPlume) CameraPos = pCamera->GetPos();
		pBuf->BeginDraw();
	}

	vector<KeyParticleInt> & keys = GetEmitterKeyInt()->GetKey();

	int keys_size2 = keys.size()-2;
	int size=Particle.size();
//#define SHORT_SORT  //Ётот макрос дл€ теста!!!!!! Ќ≈ включать!
#ifdef SHORT_SORT
	//“упо пока и медленно, просто дл€ теста качества получающейс€ картинки.
	vector<SHORT_INDEX> sort_index(size);
	vector<float> zfloat_index(size);
	float zfloat_index_min=0,zfloat_index_max=1;
	for(int index=0;index<size;index++)
	{
		if(Particle.IsFree(index))
			continue;
		nParticle& p=Particle[index];

		xassert(p.key>=0);

		Vect3f pos;

		float dtime=dtime_global*p.inv_life_time;
		float& t=p.time;
		KeyParticleInt& k0=keys[p.key];
		KeyParticleInt& k1=keys[p.key+1];
		if (p.key==keys_size2&&t+dtime>k0.dtime)
		{
			SetFreeOrCycle(index);
			continue;
		}
		float ts=t*k0.inv_dtime;
		if (GetEmitterKeyInt()->need_wind && terra)
		{
			const Vect2f& wvel = terra->GetWindVelocity(p.pos0.x, p.pos0.y);
			float k_wind = graphRnd.fabsRnd(GetEmitterKeyInt()->k_wind_min,GetEmitterKeyInt()->k_wind_max);
			//const float & k_wind = GetEmitterKeyInt()->k_wind;
			p.pos0.x += k_wind*wvel.x*dtime_global;
			p.pos0.y += k_wind*wvel.y*dtime_global;
		}
		const Vect3f & g = GetEmitterKeyInt()->g;
		pos.x = p.pos0.x+p.vdir.x*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.x*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.y = p.pos0.y+p.vdir.y*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.y*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.z = p.pos0.z+p.vdir.z*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.z*((p.gvel0+t*k0.gravity*0.5f)*t);
		zfloat_index[index]=pCamera->xformZ(pos);
		if(index==0)
		{
			zfloat_index_min=zfloat_index[index];
			zfloat_index_max=zfloat_index[index]+1;
		}else
		{
			zfloat_index_min=min(zfloat_index_min,zfloat_index[index]);
			zfloat_index_max=max(zfloat_index_max,zfloat_index[index]);
		}
	}
	for(int index=0;index<size;index++)
	{
		SHORT_INDEX& s=sort_index[index];
		s.index=index;
		if(Particle.IsFree(index))
		{
			s.z=65535;
			continue;
		}

		s.z=round((zfloat_index[index]-zfloat_index_min)/(zfloat_index_max-zfloat_index_min)*60000);
		int k=0;
	}

	sort(sort_index.begin(),sort_index.end());

	for(int indexsort=0;indexsort<size;indexsort++)
	{
		int isorted=sort_index[indexsort].index;
		if(Particle.IsFree(isorted))
			continue;
		nParticle& p=Particle[isorted];
#else SHORT_SORT
	for(int isorted=size-1;isorted>=0;isorted--)
	{
		if(Particle.IsFree(isorted))
			continue;
		nParticle& p=Particle[isorted];
#endif SHORT_SORT
		xassert(p.key>=0);

		Vect3f pos;
		float angle;
		sColor4f fcolor;
		float psize;

		float dtime=dtime_global*p.inv_life_time;
		float& t=p.time;
		KeyParticleInt& k0=keys[p.key];
		KeyParticleInt& k1=keys[p.key+1];
		if (p.key==keys_size2&&t+dtime>k0.dtime)
		{
			if(SetFreeOrCycle(isorted))
				continue;
		}
		float ts=t*k0.inv_dtime;
		if(GetEmitterKeyInt()->need_wind && terra)
		{
			const Vect2f& wvel = terra->GetWindVelocity(p.pos0.xi(), p.pos0.yi());
			float k_wind = graphRnd.fabsRnd(GetEmitterKeyInt()->k_wind_min,GetEmitterKeyInt()->k_wind_max);
			//const float & k_wind = GetEmitterKeyInt()->k_wind;
			p.pos0.x += k_wind*wvel.x*dtime_global;
			p.pos0.y += k_wind*wvel.y*dtime_global;
		}

		const Vect3f & g = GetEmitterKeyInt()->g;
		pos.x = p.pos0.x+p.vdir.x*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.x*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.y = p.pos0.y+p.vdir.y*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.y*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.z = p.pos0.z+p.vdir.z*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.z*((p.gvel0+t*k0.gravity*0.5f)*t);
		
		Bound.AddBound(pos);
		angle = p.angle0 + p.angle_dir*(k0.angle_vel*t+t*ts*0.5f*(k1.angle_vel-k0.angle_vel));
		real_angle = angle;
		angle += -p.baseAngle;
		fcolor.b = k0.color.b+(k1.color.b-k0.color.b)*ts;
		fcolor.g = k0.color.g+(k1.color.g-k0.color.g)*ts;
		fcolor.r = k0.color.r+(k1.color.r-k0.color.r)*ts;
		fcolor.a = k0.color.a+(k1.color.a-k0.color.a)*ts;
		psize = k0.size+(k1.size-k0.size)*ts;

		if(GetEmitterKeyInt()->angle_by_center)
		{
			Vect3f dir;
			mat.xformVect(-p.vdir,dir);
			angle += atan2(dir.x,dir.y)*INV_2_PI;
		}
		//ƒобавить в массив
		Vect3f sx(Vect3f::ID),sy(Vect3f::ID);
		Vect2f rot=rotate_angle[round(angle*rotate_angle_size)&rotate_angle_mask];
		Vect2f sincos = rot;
		rot*=psize*=p.begin_size;

		if(GetEmitterKeyInt()->orientedCenter)
		{
			Vect3f r = GlobalMatrix.trans()-(GetEmitterKeyInt()->relative?GlobalMatrix*pos:pos);
			if(GetEmitterKeyInt()->orientedAxis)
				r.z=0;
			r.Normalize();
			Vect3f x,y;
			x = r%Vect3f(0,0,1);
			x.Normalize();
			y = r%x;
			y.Normalize();
			x *= psize;
			y *= psize;

			sx = sincos.x*x-sincos.y*y;
			sy = sincos.y*x+sincos.x*y;
			
		}else
		if(GetEmitterKeyInt()->oriented)
		{
			if(!GetEmitterKeyInt()->relative)
			{
				GlobalMatrix.xformVect(Vect3f(-rot.x,+rot.y,0),sy);
				GlobalMatrix.xformVect(Vect3f(-rot.y,-rot.x,0),sx);
			}else
			{
				sx.x=-rot.y;
				sx.y=-rot.x;
				sx.z=0;
				sy.x=-rot.x;
				sy.y=+rot.y;
				sy.z=0;
			}
		} else if(GetEmitterKeyInt()->planar)
		{
			if(GetEmitterKeyInt()->relative)
			{
				GlobalMatrix.invXformVect(Vect3f(-rot.x,+rot.y,0),sy);
				GlobalMatrix.invXformVect(Vect3f(-rot.y,-rot.x,0),sx);
			}else
			{
				sx.x=-rot.y;
				sx.y=-rot.x;
				sx.z=0;
				sy.x=-rot.x;
				sy.y=+rot.y;
				sy.z=0;
			}
		}else
		{
			mat.invXformVect(Vect3f(rot.x,-rot.y,0),sx);
			mat.invXformVect(Vect3f(rot.y,rot.x,0),sy);
		}
		sColor4c color(round(fcolor.r*p.begin_color.r),round(fcolor.g*p.begin_color.g),
					   round(fcolor.b*p.begin_color.b),round(fcolor.a));
		sRectangle4f rt;
		if(GetEmitterKeyInt()->randomFrame && textureComplex)
			rt = ((cTextureComplex*)textureComplex)->GetFramePos(p.nframe);
		else
			rt = texture?((cTextureAviScale*)texture)->GetFramePos(cycle(p.time_summary,1.f)) :
																									sRectangle4f::ID;	
			//rt = texture?((cTextureAviScale*)texture)->GetFramePos(p.time_summary>=1? 0.99f : p.time_summary) :
			//																						sRectangle4f::ID;	
		if(GetEmitterKeyInt()->chPlume)
		{
			#ifdef  NEED_TREANGLE_COUNT
				parent->AddCountTriangle(p.plume.plumes.size()*2);
			#endif
			if (p.plume.PutToBuf(pos,dtime,pBuf,color,CameraPos,psize, rt, false,GetEmitterKeyInt()->PlumeInterval,p.time_summary))
				continue;
		}
		else 
		{
			if (sizeByTexture)
			{
				if(textureComplex)
				{
					scaleX = scaleY = 1.0f;
					float w = textureComplex->sizes[p.nframe].x;
					float h = textureComplex->sizes[p.nframe].y;
					if (h>w)
					{
						scaleY = float(h)/float(w);
					}else if(w>h)
					{
						scaleX = float(w)/float(h);
					}
				}
				
				sx*=scaleX;
				sy*=scaleY;
			}
			sVertexXYZDT1 *v=pBuf->Get();
			
			v[0].pos.x=pos.x-sx.x-sy.x;	v[0].pos.y=pos.y-sx.y-sy.y;	v[0].pos.z=pos.z-sx.z-sy.z;
			v[0].diffuse=color;
			v[0].GetTexel().x = rt.min.x; v[0].GetTexel().y = rt.min.y;	//	(0,0);

			v[1].pos.x=pos.x-sx.x+sy.x;	v[1].pos.y=pos.y-sx.y+sy.y;	v[1].pos.z=pos.z-sx.z+sy.z;
			v[1].diffuse=color;
			v[1].GetTexel().x = rt.min.x; v[1].GetTexel().y = rt.max.y;	//	(0,1);
			
			v[2].pos.x=pos.x+sx.x-sy.x; v[2].pos.y=pos.y+sx.y-sy.y; v[2].pos.z=pos.z+sx.z-sy.z; 
			v[2].diffuse=color;
			v[2].GetTexel().x = rt.max.x; v[2].GetTexel().y = rt.min.y;	//  (1,0);
			
			v[3].pos.x=pos.x+sx.x+sy.x;	v[3].pos.y=pos.y+sx.y+sy.y;	v[3].pos.z=pos.z+sx.z+sy.z;
			v[3].diffuse=color;
			v[3].GetTexel().x = rt.max.x; v[3].GetTexel().y = rt.max.y;	//  (1,1);
/*
			for(int i=0; i<4; i++)
			{
				xassert(v[i].pos.x<100000&&v[i].pos.x>-100000);
				xassert(v[i].pos.y<100000&&v[i].pos.y>-100000);
				xassert(v[i].pos.z<100000&&v[i].pos.z>-100000);
				xassert(v[i].GetTexel().x<10&& v[i].GetTexel().x>-10);
				xassert(v[i].GetTexel().y<10&& v[i].GetTexel().y>-10);
			}
*/
			#ifdef  NEED_TREANGLE_COUNT
				parent->AddCountTriangle(2);
			#endif
		}
		ProcessTime(p,dtime,isorted,pos);
	}

	pBuf->EndDraw();
	Particle.Compress();
	old_time=time;
}

void cEmitterInt::ProcessTime(nParticle& p,float dt,int i,Vect3f& cur_pos)
{
	float& t=p.time;
	vector<KeyParticleInt> & keys = GetEmitterKeyInt()->GetKey();
	KeyParticleInt* k0=&keys[p.key];
	KeyParticleInt* k1=&keys[p.key+1];

	if(p.key_begin_time>=0)
	{
		EffectBeginSpeedMatrix* s=&begin_speed[p.key_begin_time];
		
		while(s->time_begin<=p.time_summary)
		{
			Vect3f v=CalcVelocity(*s,p,1);
			
			float& t=p.time;
			float ts=t*k0->inv_dtime;
			p.pos0 -= v*(t*(k0->vel+ts*0.5f*(k1->vel-k0->vel)));
			p.vdir += v;
			xassert(p.vdir.z>=-100000 && p.vdir.z<=100000);

			p.key_begin_time++;
			if(p.key_begin_time>=begin_speed.size())
			{
				p.key_begin_time=-1;
				break;
			}else
				s=&begin_speed[p.key_begin_time];
		}
	}

	while(t+dt>k0->dtime)
	{
		//t==1
		float tprev=t;
		t=k0->dtime;
		float ts=t*k0->inv_dtime;
		const Vect3f & g = GetEmitterKeyInt()->g;
		p.pos0 = p.pos0+p.vdir*(t*(k0->vel+ts*0.5f*(k1->vel-k0->vel)))+g*((p.gvel0+t*k0->gravity*0.5f)*t);
		p.angle0 = p.angle0+ p.angle_dir*(k0->angle_vel*t+t*ts*0.5f*(k1->angle_vel-k0->angle_vel));
		p.gvel0+=t*k0->gravity;

		dt-=k0->dtime-tprev;
		t=0;
		p.key++;
		if(p.key>=keys.size()-1)
		{
			SetFreeOrCycle(i);
			break;
		}
		
		k0=k1;
		k1=&keys[p.key+1];
	}

	t+=dt;
	p.time_summary+=dt;
}

void cEmitterInt::DummyQuant()
{
	float dtime_global=(time-old_time);
	int size=Particle.size();
	Vect3f pos;
	//Bound.SetInvalidBox();
	if(GetEmitterKeyInt()->relative)
	{
		Bound.max.set(0,0,0);
		Bound.min.set(0,0,0);
	}else
	{
		Bound.max = GlobalMatrix.trans();
		Bound.min = GlobalMatrix.trans();
	}
	
	for(int i=0;i<size;i++)
	{
		if(Particle.IsFree(i))
			continue;
		nParticle& p=Particle[i];
		float dtime=dtime_global*p.inv_life_time;

		vector<KeyParticleInt> & keys = GetEmitterKeyInt()->GetKey();
		KeyParticleInt& k0=keys[p.key];
		KeyParticleInt& k1=keys[p.key+1];
		float& t=p.time;
		float ts=t*k0.inv_dtime;
		const Vect3f & g = GetEmitterKeyInt()->g;
		pos.x = p.pos0.x+p.vdir.x*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.x*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.y = p.pos0.y+p.vdir.y*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.y*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.z = p.pos0.z+p.vdir.z*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.z*((p.gvel0+t*k0.gravity*0.5f)*t);
		Bound.AddBound(pos);
		
		ProcessTime(p,dtime,i,pos);
	}

	Particle.Compress();
	old_time=time;
}

void cEmitterInt::EmitInstantly(float tmin,float tmax)
{
	if(Particle.size()>0 && is_intantly_infinity_particle)
		return;

	CKey & num_particle = GetEmitterKeyInt()->num_particle;
	EmitterType & particle_position = GetEmitterKeyInt()->particle_position;

	CKey::iterator it;
	FOR_EACH(num_particle,it)
	{
		KeyFloat pos=*it;
		if(pos.time>=tmin && pos.time<tmax)
		{
			int num=(ignoreParticleRate&&parent->GetParticleRateReal()>FLT_EPS)?round(pos.f):parent->GetParticleRateRealInstantly(pos.f);
			int prevsize=Particle.size();
			if(particle_position.type == EMP_3DMODEL)
			{
				parent->RecalcBeginPos(num);
				num = parent->GetPos().size();
			}
			Particle.resize(prevsize+num);//ѕотенциально кривой участок кода, некоторый может быть излишний рост буфера.
			for(int i=0;i<num;i++)
			{
//				dprintf("EmitOne tmin=%f, tmax=%f\n",tmin,tmax);
				if(particle_position.type == EMP_3DMODEL)
					EmitOne(i+prevsize,0,i);
				else
					EmitOne(i+prevsize,0);
			}
		}
	}
}
void cEffect::RecalcBeginPos(int num)
{
	if (!model || !link3dx.IsLink())
		return;
	MatXf mat(GlobalMatrix);
	begin_position.resize(num);
	normal_position.resize(num);

	model->GetVisibilityVertex(begin_position,normal_position);

	mat.rot().xrow().Normalize(); // —делано дл€ cSimply3dx
	mat.rot().yrow().Normalize(); // потом думать как сделать лучше
	mat.rot().zrow().Normalize();
	mat.Invert();
	for (int i=0; i<begin_position.size(); i++)
	{
		begin_position[i] = mat * begin_position[i];
	}
}

void cEmitterInt::EmitProlonged(float dt)
{
	int n=CalcProlongedNum(dt);
	if(n<=0)return;

	EmitterType & particle_position = GetEmitterKeyInt()->particle_position;

	float finterpolate=1.0f/n;
	float delta=dt*finterpolate;
	BeginInterpolateGlobalMatrix();
	if(particle_position.type == EMP_3DMODEL)
		parent->RecalcBeginPos(n);
	for(int i=0;i<n;i++)
	{
		int nt=n-1-i;
		InterpolateGlobalMatrix(finterpolate*nt);
		if(particle_position.type == EMP_3DMODEL)
			EmitOne(Particle.GetIndexFree(),0,i);
		else
			EmitOne(Particle.GetIndexFree(),0);
	}

	EndInterpolateGlobalMatrix();
}

bool cEmitterInt::GetRndPos(Vect3f& pos, Vect3f* norm)
{
	if (Particle.is_empty())
	{
		cur_one_pos = -1;
		pos.set(0,0,0);
		if (norm)
			norm->set(0,0,0);
		return false;
	}
	else
	{
		int size;
		int i = graphRnd(size = Particle.size());
		while(Particle.IsFree(i)) 
			i = graphRnd(size);
		cur_one_pos = i;
		nParticle& p = Particle[i];
//		float dtime=dtime_global*p.inv_life_time;
		vector<KeyParticleInt> & keys = GetEmitterKeyInt()->GetKey();
		KeyParticleInt& k0=keys[p.key];
		KeyParticleInt& k1=keys[p.key+1];
		float t=p.time;
		float ts=t*k0.inv_dtime;
		const Vect3f & g = GetEmitterKeyInt()->g;
		pos.x = p.pos0.x+p.vdir.x*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.x*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.y = p.pos0.y+p.vdir.y*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.y*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.z = p.pos0.z+p.vdir.z*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.z*((p.gvel0+t*k0.gravity*0.5f)*t);
		if (norm)
			*norm = Particle[i].normal;
		return true;
	}
}
Vect3f cEmitterInt::GetVdir(int i)
{
	if (i==-1)
		return Vect3f::ZERO;
	xassert((UINT)i<Particle.size());
	nParticle& p = Particle[i];
	vector<KeyParticleInt> & keys = GetEmitterKeyInt()->GetKey();
	KeyParticleInt& k0=keys[p.key];
	KeyParticleInt& k1=keys[p.key+1];
	float t=p.time;
	float ts=t*k0.inv_dtime;
	const Vect3f & g = GetEmitterKeyInt()->g;
	return p.vdir*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g*((p.gvel0+t*k0.gravity*0.5f)*t);
}

void cEmitterInt::EmitOne(int ix_cur/*nParticle& cur*/,float begin_time, int num_pos)
{
	xassert((UINT)ix_cur<Particle.size());
	nParticle& cur = Particle[ix_cur];
	float t=time*inv_emitter_life_time;
	float inv_life = GetEmitterKeyInt()->inv_life_time.Get(t);
	float dlife = GetEmitterKeyInt()->life_time_delta.Get(t);
	cur.key=0;
	cur.gvel0=0;
	cur.inv_life_time=inv_particle_life_time*inv_life*(1+dlife*(graphRnd.frand()-0.5f));

	cur.inv_life_time = max(0.0f,cur.inv_life_time);

	cur.time_summary=cur.time=cur.inv_life_time*begin_time;
	cur.key_begin_time=0;

	float dsz = GetEmitterKeyInt()->begin_size_delta.Get(t);
	cur.begin_size = GetEmitterKeyInt()->begin_size.Get(t) * (1 + dsz * (graphRnd.frand() - 0.5f));
	float dv = 1 + GetEmitterKeyInt()->velocity_delta.Get(t) * (graphRnd.frand() - 0.5f);
	//cur.angle0=0;
	//float a = -atan2f(GlobalMatrix.R[0][1],GlobalMatrix.R[1][1])
	//cur.baseAngle = -GetEmitterKeyInt()->base_angle * INV_2_PI;
	if(GetEmitterKeyInt()->turn)
		cur.baseAngle =(atan2(-GlobalMatrix.R[0][1], GlobalMatrix.R[1][1]) - GetEmitterKeyInt()->base_angle) * INV_2_PI;
	else
		cur.baseAngle = -GetEmitterKeyInt()->base_angle * INV_2_PI;
	cur.angle0 = real_angle;
	if(GetEmitterKeyInt()->randomFrame && GetTexture() && GetTexture()->IsComplexTexture())
		cur.nframe = graphRnd(((cTextureAviScale*)GetTexture())->GetFramesCount());

	switch(GetEmitterKeyInt()->rotation_direction)
	{
	case ETRD_CW:
		cur.angle_dir=1;
		break;
	case ETRD_CCW:
		cur.angle_dir=-1;
		break;
	default:
		cur.angle_dir=(graphRnd()&1)?-1:1;
		break;
	}

	if(OnePos(ix_cur,cur.pos0,&cur.normal,num_pos))
	{
		cur.pos0 = GetEmitterKeyInt()->relative ? cur.pos0 : GlobalMatrix * cur.pos0;
		cur.normal=GlobalMatrix.rot()*cur.normal;
		cur.normal.Normalize();
	}

	{
		cur.vdir.set(0,0,0);
		int i;
		for(i=0;i<begin_speed.size();i++)
		{
			if(begin_speed[i].time_begin<1e-3f)
			{
				cur.vdir+=CalcVelocity(begin_speed[i],cur,dv);
			}else
			{
				break;
			}
		}

		cur.key_begin_time=i;
		if(cur.key_begin_time>=begin_speed.size())
			cur.key_begin_time=-1;
	}
	if (other/*&&begin_speed[0].velocity==EMV_INVARIABLY*/)
	{
		cur.vdir.add(other->GetVdir(other->cur_one_pos));
		xassert(cur.vdir.z>=-100000 && cur.vdir.z<=100000);
		FastNormalize(cur.vdir);
	}

	//cur.baseAngle = (atan2(-cur.vdir.y, cur.vdir.x) - GetEmitterKeyInt()->base_angle) * INV_2_PI;

	CalcColor(cur);
	cur.vdir *= GetEmitterKeyInt()->emitter_scale.Get(t);
	xassert(cur.vdir.z>=-100000 && cur.vdir.z<=100000);
	if(GetEmitterKeyInt()->chPlume)
	{	
		cur.plume.Init(GetEmitterKeyInt()->TraceCount,cur.pos0);
	}
}
float cEmitterBase::GetVelNoise()
{
	return GetEmitterKeyBase()->velNoise.Get(totaltime);
}
Vect3f cEmitterBase::GetDirNoise()
{
	Vect3f v;
	v.set(GetEmitterKeyBase()->noiseBlockX ? 0 : GetEmitterKeyBase()->dirNoiseX.Get(totaltime), 
		  GetEmitterKeyBase()->noiseBlockY ? 0 : GetEmitterKeyBase()->dirNoiseY.Get(totaltime), 
		  GetEmitterKeyBase()->noiseBlockZ ? 0 : GetEmitterKeyBase()->dirNoiseZ.Get(totaltime));
	return v;
}
bool cEmitterBase::GetNoiseReplace()
{
	return GetEmitterKeyBase()->noiseReplace;
}
Vect3f cEmitterInt::CalcVelocity(const EffectBeginSpeedMatrix& s,const nParticle& cur,float mul)
{
	Vect3f v;
	Vect3f ns(0,0,0);
	mul*=s.mul;
	if(GetEmitterKeyInt()->velNoiseEnable)
	{
		if (velNoiseOther)
			mul *= velNoiseOther->GetVelNoise();
		else
			mul *= GetEmitterKeyInt()->velNoise.Get(totaltime);
	}
	bool noiseReplace = GetEmitterKeyInt()->noiseReplace;
	if(GetEmitterKeyInt()->dirNoiseEnable)
	{
		if (dirNoiseOther)
		{
			ns = dirNoiseOther->GetDirNoise();
			noiseReplace = dirNoiseOther->GetNoiseReplace();
		}
		else
			ns.set(GetEmitterKeyInt()->noiseBlockX ? 0 : GetEmitterKeyInt()->dirNoiseX.Get(totaltime), 
			       GetEmitterKeyInt()->noiseBlockY ? 0 : GetEmitterKeyInt()->dirNoiseY.Get(totaltime), 
				   GetEmitterKeyInt()->noiseBlockZ ? 0 : GetEmitterKeyInt()->dirNoiseZ.Get(totaltime));

		if (noiseReplace)
			ns.Normalize();
		//ns = v*mul;
	}

	if(!noiseReplace || !GetEmitterKeyInt()->dirNoiseEnable)
	{
		switch(s.velocity)
		{
		default:
		case EMV_BOX:
			v=ns+RndBox(1,1,1);
			v.Normalize();
			v*=mul;
			break;
		case EMV_SPHERE:
			v=ns+RndSpere(1);
			v.Normalize();
			v*=mul;
			break;
		case EMV_CYLINDER:
			v=ns+RndCylinder(s.esp.pos.x,s.esp.pos.x,s.esp.dpos.x,s.esp.pos.z,false);
			v.Normalize();
			v*=mul;
			break;
		case EMV_LINE:
			v.x=ns.x;
			v.y=ns.y;
			v.z=graphRnd.frand()+ns.z;
			v.Normalize();
			v*=mul;
			break;
		case EMV_NORMAL_OUT:
			v = (GetEmitterKeyInt()->relative) ? cur.pos0 : cur.pos0 - GlobalMatrix.trans();
			v+=ns;
			v.Normalize();
			v*=mul;
			break;
		case EMV_NORMAL_IN:
			v = (GetEmitterKeyInt()->relative) ? cur.pos0 : cur.pos0 - GlobalMatrix.trans();
			v+=ns;
			v.Normalize();
			v*=-mul;
			break;
		case EMV_NORMAL_IN_OUT:
			v = (GetEmitterKeyInt()->relative) ? cur.pos0 : cur.pos0 - GlobalMatrix.trans();
			v+=ns;
			v.Normalize();
			if(graphRnd()&1)
				v*=mul;
			else
				v*=-mul;
			break;
		case EMV_Z:
			v.set(ns.x,ns.y,1+ns.z);
			v.Normalize();
			v*=mul;
			break;
		case EMV_NORMAL_3D_MODEL:
			v  = ns+(cur.normal);
			v.Normalize();
			v*=mul;
			break;
		case EMV_INVARIABLY:
			v.set(0,0,0);
			break;
		}
	}else
	{
		v = ns;
		v = v*mul;
	}
	//v = s.rotation*v;
	v = GetEmitterKeyInt()->relative ? s.rotation * v : GlobalMatrix.rot() * s.rotation * v;
	xassert(v.z>=-100000 && v.z<=100000);
	return v;
}

void cEmitterInt::SetEmitterKey(EmitterKeyInt& k,c3dx* models)
{
	cEmitterBase::SetEmitterKey(k,models);

	begin_speed.resize(k.begin_speed.size());
	for(int i=0;i<begin_speed.size();i++)
	{
		EffectBeginSpeed& in=k.begin_speed[i];
		EffectBeginSpeedMatrix& out=begin_speed[i];
		out.velocity=in.velocity;
		out.mul=in.mul;
		out.rotation.set(in.rotation);
		out.time_begin=in.time;
		out.esp=in.esp;
	}

	sort(begin_speed.begin(),begin_speed.end());

	if(GetEmitterKeyInt()->particle_position.type == EMP_3DMODEL && models && GetEmitterKeyInt()->use_light)
	{
		models->GetEmitterMaterial(material);
	}

	CalcIntantlyInfinityParticle();
}

void cEmitterInt::CalcColor(nParticle& cur)
{
	if(cur_one_pos < 0 || !GetEmitterKeyInt()->use_light)
	{
		//cur.begin_color.set(graphRnd(255),graphRnd(255),graphRnd(255),255);
		cur.begin_color.set(255,255,255,255);
		return;
	}

	Vect3f LightDirection;
	parent->GetScene()->GetLighting(&LightDirection);
	/*Vect3f norm,norm1,norm2;///temp
	if (particle_position.type==EMP_3DMODEL_INSIDE)   
		norm1 = norm = normal_position[cur_one_pos];
	else 
		norm2 = norm = parent->GetNorm()[cur_one_pos];*/
	const Vect3f& norm = (GetEmitterKeyInt()->particle_position.type == EMP_3DMODEL_INSIDE) ? GetEmitterKeyInt()->normal_position[cur_one_pos] : parent->GetNorm()[cur_one_pos];

	Vect3f word_n=Normalize(GlobalMatrix.rot()*norm);
	float lit=-LightDirection.dot(word_n);
	if(lit<0)
		lit=0;
	sColor4f c=material.Diffuse*lit+material.Ambient;

	c.r=clamp(c.r,0.0f,1.0f);
	c.g=clamp(c.g,0.0f,1.0f);
	c.b=clamp(c.b,0.0f,1.0f);
	
	cur.begin_color=c;
}


cEmitterSpl::cEmitterSpl()
{
}

cEmitterSpl::~cEmitterSpl()
{
}

void cEmitterSpl::Draw(cCamera *pCamera)
{
	if(no_show_obj_editor)
		return;
	
	float dtime_global=(time-old_time);
	MatXf mat=pCamera->GetMatrix();
	//Bound.SetInvalidBox();
	if(GetEmitterKeySpl()->relative)
	{
		Bound.max.set(0,0,0);
		Bound.min.set(0,0,0);
	}else
	{
		Bound.max = GlobalMatrix.trans();
		Bound.min = GlobalMatrix.trans();
	}
	TerraInterface* terra = NULL;
	if (pCamera->GetScene()->GetTileMap())
		terra = pCamera->GetScene()->GetTileMap()->GetTerra();
	cInterfaceRenderDevice* rd=gb_RenderDevice;

	cTextureAviScale* texture = NULL;
	cTextureComplex* textureComplex = NULL;
	//	cTextureAviScale* plume_texture = NULL;
	if (GetTexture(0) && GetTexture(0)->IsAviScaleTexture())
		texture = (cTextureAviScale*)GetTexture(0);
	if (GetTexture(0) && GetTexture(0)->IsComplexTexture())
		textureComplex = (cTextureComplex*)GetTexture(0);

	cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();

	gb_RenderDevice->SetSamplerDataVirtual(0,GetEmitterKeySpl()->chPlume?sampler_clamp_linear:sampler_wrap_linear);
	bool reflectionz=pCamera->GetAttr(ATTRCAMERA_REFLECTION);
	{//Ќемного не к месту, зато быстро по скорости, дл€ отражений.
		gb_RenderDevice3D->SetTexture(5,pCamera->GetZTexture());
		gb_RenderDevice3D->SetSamplerData(5,sampler_clamp_linear);
	}
	gb_RenderDevice3D->SetWorldMaterial(blend_mode, GetEmitterKeySpl()->relative ? GlobalMatrix:MatXf::ID, 0, GetTexture(0),0,COLOR_MOD,softSmoke,reflectionz);
	rd->SetRenderState( RS_CULLMODE, D3DCULL_NONE );
#ifdef NEED_TREANGLE_COUNT
	if (parent->drawOverDraw)
	{
		gb_RenderDevice3D->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
		gb_RenderDevice3D->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
		gb_RenderDevice3D->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
		parent->psOverdraw->Select();
	}
#endif
	Vect3f CameraPos;
	if(GetEmitterKeySpl()->relative)
	{
		if(GetEmitterKeySpl()->chPlume)
		{
			MatXf invGlobMatrix(GlobalMatrix);
			invGlobMatrix.invert();
			CameraPos = invGlobMatrix*pCamera->GetPos();
		}
		mat = mat*GlobalMatrix;
		pBuf->BeginDraw(GlobalMatrix);
	}else
	{
		if(GetEmitterKeySpl()->chPlume) CameraPos = pCamera->GetPos();
		pBuf->BeginDraw();
	}
	int size=Particle.size();
	for(int i=size-1;i>=0;i--)
	{
		if(Particle.IsFree(i))
			continue;
		nParticle& p=Particle[i];
		Vect3f pos;
		float angle;
		sColor4f fcolor;
		float psize;

		float dtime=dtime_global*p.inv_life_time;
		if(GetEmitterKeySpl()->need_wind && terra)
		{
			const Vect2f& wvel = terra->GetWindVelocity(p.pos.trans().x, p.pos.trans().y);
			float k_wind = graphRnd.fabsRnd(GetEmitterKeySpl()->k_wind_min,GetEmitterKeySpl()->k_wind_max);
			//const float & k_wind = GetEmitterKeySpl()->k_wind;
			p.pos.trans().x += k_wind*wvel.x*dtime_global;
			p.pos.trans().y += k_wind*wvel.y*dtime_global;
		}

		{
			vector<HeritKey> & hkeys = GetEmitterKeySpl()->GetHeritKey();
			HeritKey& k=hkeys[p.hkey];
			float ts=p.htime*k.inv_dtime;
			k.Get(pos,ts);
			p.pos.xformPoint(pos);
			Bound.AddBound(pos);
		}

		{
			float& t=p.time;
			vector<KeyParticleSpl> & keys = GetEmitterKeySpl()->GetKey();
			KeyParticleSpl& k0=keys[p.key];
			KeyParticleSpl& k1=keys[p.key+1];
			
			float ts=t*k0.inv_dtime;
			angle = p.angle0+ p.angle_dir*(k0.angle_vel*t+t*ts*0.5f*(k1.angle_vel-k0.angle_vel));
			fcolor = k0.color+(k1.color-k0.color)*ts;
			psize = k0.size+(k1.size-k0.size)*ts;
		}

		//ƒобавить в массив
		Vect3f sx,sy;
		Vect2f rot=rotate_angle[round(angle*rotate_angle_size)&rotate_angle_mask];
		Vect2f sincos = rot;
		rot*=psize*=p.begin_size;
		if(GetEmitterKeySpl()->orientedCenter)
		{
			Vect3f r = GlobalMatrix.trans()-(GetEmitterKeySpl()->relative?GlobalMatrix*pos:pos);
			if(GetEmitterKeySpl()->orientedAxis)
				r.z=0;
			r.Normalize();
			Vect3f x,y;
			x = r%Vect3f(0,0,1);
			x.Normalize();
			y = r%x;
			y.Normalize();
			x *= psize;
			y *= psize;

			sx = sincos.x*x-sincos.y*y;
			sy = sincos.y*x+sincos.x*y;

		}else
		if (GetEmitterKeySpl()->planar)
		{
			sx.x=-rot.y;
			sx.y=-rot.x;
			sx.z=0;
			sy.x=-rot.x;
			sy.y=+rot.y;
			sy.z=0;
		}else
		{
			mat.invXformVect(Vect3f(+rot.x,-rot.y,0),sx);
			mat.invXformVect(Vect3f(+rot.y,+rot.x,0),sy);
		}

		sColor4c color(round(fcolor.r),round(fcolor.g),round(fcolor.b),round(fcolor.a));
		//const cTextureAviScale::RECT& rt = texture ?
		//	((cTextureAviScale*)texture)->GetFramePos(p.time_summary>=1? 0.99f : p.time_summary) :
		//				cTextureAviScale::RECT::ID;	
		sRectangle4f rt;
		if(GetEmitterKeySpl()->randomFrame && textureComplex)
			rt = ((cTextureComplex*)textureComplex)->GetFramePos(p.nframe);
		else
			rt = texture?((cTextureAviScale*)texture)->GetFramePos(p.time_summary>=1? 0.99f : p.time_summary) :
																									sRectangle4f::ID;	

		if(GetEmitterKeySpl()->chPlume)
		{
			#ifdef  NEED_TREANGLE_COUNT
				parent->AddCountTriangle(p.plume.plumes.size()*2);
			#endif
			if (p.plume.PutToBuf(pos,dtime,pBuf,color,CameraPos,psize, rt, false,GetEmitterKeySpl()->PlumeInterval,p.time_summary))
				continue;
		}
		else 
		{
			if (sizeByTexture)
			{
				if(textureComplex)
				{
					scaleX = scaleY = 1.0f;
					float w = textureComplex->sizes[p.nframe].x;
					float h = textureComplex->sizes[p.nframe].y;
					if (h>w)
					{
						scaleY = float(h)/float(w);
					}else if(w>h)
					{
						scaleX = float(w)/float(h);
					}
				}

				sx*=scaleX;
				sy*=scaleY;
			}
			sVertexXYZDT1 *v=pBuf->Get();
			v[0].pos.x=pos.x-sx.x-sy.x; v[0].pos.y=pos.y-sx.y-sy.y; v[0].pos.z=pos.z-sx.z-sy.z; 
			v[0].diffuse.b=color.b;	v[0].diffuse.g=color.g; v[0].diffuse.r=color.r; v[0].diffuse.a=color.a; 
			v[0].GetTexel().x = rt.min.x; v[0].GetTexel().y = rt.min.y;	//	(0,0);

			v[1].pos.x=pos.x-sx.x+sy.x;	v[1].pos.y=pos.y-sx.y+sy.y;	v[1].pos.z=pos.z-sx.z+sy.z;
			v[1].diffuse.b=color.b; v[1].diffuse.g=color.g;	v[1].diffuse.r=color.r;	v[1].diffuse.a=color.a;
			v[1].GetTexel().x = rt.min.x; v[1].GetTexel().y = rt.max.y;//	(0,1);

			v[2].pos.x=pos.x+sx.x-sy.x; v[2].pos.y=pos.y+sx.y-sy.y; v[2].pos.z=pos.z+sx.z-sy.z; 
			v[2].diffuse.b=color.b; v[2].diffuse.g=color.g; v[2].diffuse.r=color.r; v[2].diffuse.a=color.a; 
			v[2].GetTexel().x = rt.max.x; v[2].GetTexel().y = rt.min.y;	//  (1,0);

			v[3].pos.x=pos.x+sx.x+sy.x;	v[3].pos.y=pos.y+sx.y+sy.y;	v[3].pos.z=pos.z+sx.z+sy.z;
			v[3].diffuse.b=color.b; v[3].diffuse.g=color.g; v[3].diffuse.r=color.r; v[3].diffuse.a=color.a; 
			v[3].GetTexel().x = rt.max.x; v[3].GetTexel().y = rt.max.y;//  (1,1);
			#ifdef  NEED_TREANGLE_COUNT
				parent->AddCountTriangle(2);
			#endif
		}
		ProcessTime(p,dtime,i);
	}

	pBuf->EndDraw();
	Particle.Compress();
	old_time=time;
}
/*
void cEmitterSpl::SetTarget(vector<Vect3f>& pos_end)
{
	Vect3f center = parent->GetPosition().trans();
	Vect3f pos;
	int n = hkeys.size();
	HeritKey& k=hkeys[n-1];
	float ts=k.inv_dtime;
	k.Get(pos,ts);
	float l = pos.norm();
	float nl = pos_end.front().distance(center);
	float kf = nl/l;
	for(int i=0;i<n;i++)
	{
		HeritKey& k=hkeys[i];
		k.H0*= kf;
		k.H1*= kf;		
		k.H2*= kf;
		k.H3*= kf;
	}
}
*/
void cEmitterSpl::SetEmitterKey(EmitterKeySpl& k,c3dx* models)
{
	cEmitterBase::SetEmitterKey(k,models);
}

void cEmitterSpl::EmitInstantly(float tmin,float tmax)
{
	EMITTER_TYPE_POSITION & particle_position_type = GetEmitterKeySpl()->particle_position.type;
	CKey & num_particle = GetEmitterKeySpl()->num_particle;
	CKey::iterator it;
	FOR_EACH(num_particle,it)
	{
		KeyFloat pos=*it;
		if(pos.time>=tmin && pos.time<tmax)
		{
			int num=(ignoreParticleRate&&parent->GetParticleRateReal()>FLT_EPS)?round(pos.f):parent->GetParticleRateRealInstantly(pos.f);
			int prevsize=Particle.size();
			Particle.resize(prevsize+num);
			if(particle_position_type == EMP_3DMODEL)
				parent->RecalcBeginPos(num);
			for(int i=0;i<num;i++)
			{
				EmitOne(i+prevsize, 0);
			}
		}
	}
}

void cEmitterSpl::EmitProlonged(float dt)
{
	int n=CalcProlongedNum(dt);
	if(n<=0)return;

	float finterpolate=1.0f/n;
	float delta=dt*finterpolate;
	BeginInterpolateGlobalMatrix();
	if(GetEmitterKeySpl()->particle_position.type == EMP_3DMODEL)
		parent->RecalcBeginPos(n);
	for(int i=0;i<n;i++)
	{
		float nt=n-1-i;
		InterpolateGlobalMatrix(finterpolate*nt);
		EmitOne(Particle.GetIndexFree(),delta*nt);
	}
	EndInterpolateGlobalMatrix();
}

bool cEmitterSpl::GetRndPos(Vect3f& pos, Vect3f* norm)
{
	if (Particle.is_empty())
	{
		cur_one_pos = -1;
		pos.set(0,0,0);
		if (norm)
			norm->set(0,0,0);
		return false;
	}
	else
	{
		int size;
		int i = graphRnd(size = Particle.size());
		while(Particle.IsFree(i)) 
			i = graphRnd(size);
		cur_one_pos = i;
		nParticle& p = Particle[i];
		vector<HeritKey> & hkeys = GetEmitterKeySpl()->GetHeritKey();
		HeritKey& k = hkeys[p.hkey];
		k.Get(pos,p.htime*k.inv_dtime);
		p.pos.xformPoint(pos);
		if (norm)
			norm->set(0,0,0);
		return true;
	}
}
Vect3f cEmitterSpl::GetVdir(int i)
{
	if (i==-1)
		return Vect3f::ZERO;
	xassert((UINT)i<Particle.size());
	nParticle& p = Particle[i];
	vector<HeritKey> & hkeys = GetEmitterKeySpl()->GetHeritKey();
	HeritKey& k = hkeys[p.hkey];
	Vect3f pos;
	k.Get(pos,p.htime*k.inv_dtime);
	pos = p.pos.rot().xform(pos); 
	FastNormalize(pos);
	return pos;
}

void cEmitterSpl::EmitOne(int ix_cur/*nParticle& cur*/,float begin_time)
{
	xassert((UINT)ix_cur<Particle.size());
	nParticle& cur = Particle[ix_cur];
	float t=time*inv_emitter_life_time;
	float inv_life = GetEmitterKeySpl()->inv_life_time.Get(t);
	float dlife = GetEmitterKeySpl()->life_time_delta.Get(t);
	cur.key=0;
	cur.hkey=0;
	cur.inv_life_time=inv_particle_life_time*inv_life*(1+dlife*(graphRnd.frand()-0.5f));
	cur.time_summary=cur.htime=cur.time=cur.inv_life_time*begin_time;
	
	if(GetEmitterKeySpl()->randomFrame && GetTexture() && GetTexture()->IsComplexTexture())
		cur.nframe = graphRnd(((cTextureAviScale*)GetTexture())->GetFramesCount());

	float dsz = GetEmitterKeySpl()->begin_size_delta.Get(t);
	cur.begin_size = GetEmitterKeySpl()->begin_size.Get(t) * (1 + dsz * (graphRnd.frand() - 0.5f));
	cur.angle0=0;
	switch(GetEmitterKeySpl()->rotation_direction)
	{
	case ETRD_CW:
		cur.angle_dir=1;
		break;
	case ETRD_CCW:
		cur.angle_dir=-1;
		break;
	default:
		cur.angle_dir=(graphRnd()&1)?-1:1;
		break;
	}

	Vect3f pos;
	bool need_transform;
	switch(GetEmitterKeySpl()->direction)
	{
	case ETDS_ID:
		need_transform = OnePos(ix_cur,pos);
		cur.pos.rot()=Mat3f::ID;
		break;
	case ETDS_ROTATEZ:
		need_transform = OnePos(ix_cur,pos);
		cur.pos.rot().set(Vect3f(0,0,1), graphRnd.frand()*2*M_PI,1);
		break;
	case ETDS_BURST1:
		{
			Vect3f norm;
			need_transform = OnePos(ix_cur,pos, &norm);
			norm.Normalize();
			Vect3f z(0,0,1);
			Vect3f p_norm=norm%z;
			if(p_norm.norm2()<1e-5f)
				p_norm.set(1,0,0);
			p_norm.Normalize();
			Vect3f pp_norm=norm%p_norm;
			cur.pos.rot()= Mat3f(
						p_norm.x, pp_norm.x, norm.x,
						p_norm.y, pp_norm.y, norm.y,
						p_norm.z, pp_norm.z, norm.z	
								);
		}
		break;
	case ETDS_BURST2:
		{
			Vect3f norm;
			need_transform = OnePos(ix_cur,pos, &norm);
			cur.pos.rot().set(Vect3f(0,0,1), norm.psi(),1);
		}
		break;
	default:
		xassert(false);
	}
	cur.pos.trans()=pos;
	if (need_transform) 
		cur.pos = GetEmitterKeySpl()->relative ? cur.pos : GlobalMatrix * cur.pos;
	if(GetEmitterKeySpl()->chPlume)
	{	
		cur.plume.Init(GetEmitterKeySpl()->TraceCount,cur.pos.trans());
	}
}

void cEmitterSpl::ProcessTime(nParticle& p,float delta_time,int i)
{

	p.time_summary+=delta_time;
	{
		float& t=p.time;
		vector<KeyParticleSpl> & keys = GetEmitterKeySpl()->GetKey();
		KeyParticleSpl* k0=&keys[p.key];
		KeyParticleSpl* k1=&keys[p.key+1];

		float dt=delta_time;
		while(t+dt>k0->dtime)
		{
			//t==1
			float tprev=t;
			t=k0->dtime;
			float ts=t*k0->inv_dtime;
			p.angle0 = p.angle0+ p.angle_dir*(k0->angle_vel*t+t*ts*0.5f*(k1->angle_vel-k0->angle_vel));

			dt-=k0->dtime-tprev;
			t=0;
			p.key++;

			if(p.key>=keys.size()-1)
			{
				Particle.SetFree(i);
				p.key = -1;
				return;
			}
			
			k0=k1;
			k1=&keys[p.key+1];
		}

		t+=dt;
	}

	{
		float& t=p.htime;
		vector<HeritKey> & hkeys = GetEmitterKeySpl()->GetHeritKey();
		HeritKey* k0=&hkeys[p.hkey];

		float dt=delta_time;
		while(t+dt>k0->dtime)
		{
			//t==1
			dt-=k0->dtime-t;
			t=0;
			p.hkey++;

			if(p.hkey>=hkeys.size())
			{
				Particle.SetFree(i);
				p.key = -1;
				break;
			}
			
			k0=&hkeys[p.hkey];
		}
		t+=dt;
	}
}

void cEmitterSpl::DummyQuant()
{
	float dtime_global=(time-old_time);
	int size=Particle.size();
	for(int i=0;i<size;i++)
	{
		if(Particle.IsFree(i))
			continue;
		nParticle& p=Particle[i];
		Vect3f pos;

		float dtime=dtime_global*p.inv_life_time;

		{
			vector<HeritKey> & hkeys = GetEmitterKeySpl()->GetHeritKey();
			HeritKey& k=hkeys[p.hkey];
			float ts=p.htime*k.inv_dtime;
			k.Get(pos,ts);
			p.pos.xformPoint(pos);
			Bound.AddBound(pos);
		}

		ProcessTime(p,dtime,i);
	}
	Particle.Compress();
	old_time = time;
}
////////////////////////cLightingEmitter//////////////////////////

cLightingEmitter::cLightingEmitter() : lighting(NULL), attached(false)
{
	no_show_obj_editor = false;
}
cLightingEmitter::~cLightingEmitter()
{
	if (lighting)
		lighting->Release();
}
void cLightingEmitter::SetCycled(bool cycled_)
{
	cycled = cycled_;
}

void cLightingEmitter::SetPosition(const MatXf &mat)
{
	if (lighting)
	{
		Vect3f beg = mat*pos_beg;
		vector<Vect3f> end;
		end.resize(pos_end.size());
		for(int i=pos_end.size()-1; i>=0;i--)
			end[i] = mat*pos_end[i];
		lighting->Update(beg, end);
	}
}
void cLightingEmitter::SetEmitterKey(EmitterLightingKey& key, cEffect *parent_)
{
	xassert(!lighting);
	lighting = new cLighting();
	key.param.texture_name = key.texture_name;
	lighting->SetParameters(key.param);
	lighting->Update(key.pos_begin, key.pos_end);
	pos_beg = key.pos_begin;
	pos_end = key.pos_end;
	emitter_life_time = key.emitter_life_time;
	emitter_create_time = key.emitter_create_time;
	parent = parent_;
	cycled = key.cycled;
	time = 0;
	unical_id=&key;
}
void cLightingEmitter::Animate(float dt)
{
	time+=dt*1e-3f;
	if(no_show_obj_editor)
	{
		if (lighting)
			RELEASE(lighting);
		return;
	}
	if (!attached)
	{
		if(time>emitter_create_time)
		{
			//SetPosition(parent->GetGlobalMatrix());
			parent->GetScene()->AttachObj(lighting);
			attached = true;
		}
	}else 
	{
		if (lighting)
		{
			bool hide=parent->GetParticleRateReal()<=0;
			lighting->StopGenerate(hide);
		}
		if (!cycled && time>emitter_create_time+emitter_life_time)
		{
			RELEASE(lighting);
		}
	}
}
bool cLightingEmitter::IsLive()
{
	return lighting != NULL;
}
void cLightingEmitter::SetTarget(const Vect3f* pos_end,int pos_end_size)
{
	if (lighting)
	{
		this->pos_end.assign(pos_end,pos_end+pos_end_size);
		lighting->Update(parent->GetPosition().trans(), this->pos_end);
	}
}
////////////////////////cEffect//////////////////////////
cEffect::cEffect()
:cIUnkObj(KIND_EFFECT)
{
	link3dx.SetParent(this);
	time=0;
	auto_delete_after_life=false;
	particle_rate=1;
	func_getz=NULL;
	isTarget = false;
	distance_rate = 1.0f;
	model = NULL;
	no_fog_of_war=false;

#ifdef NEED_TREANGLE_COUNT
 
	psOverdraw = NULL;
	psOverdrawColor = NULL;
	psOverdrawCalc = NULL;
	drawOverDraw = false;
	pRenderTexture = NULL;
	pRenderTextureCalc = NULL;
	pSysSurface = NULL;
	enableOverDraw = false;
	useOverDraw = false;
#endif
}

cEffect::~cEffect()
{
	Clear();
	RELEASE(func_getz);
#ifdef NEED_TREANGLE_COUNT
	delete psOverdraw;
	delete psOverdrawColor;
	delete psOverdrawCalc;
	RELEASE(pRenderTexture);
	RELEASE(pRenderTextureCalc);
	RELEASE(pSysSurface);
#endif
}
int cEffect::GetParticleCount()
{
	int particleCount = 0;
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
		particleCount += (*it)->GetParticleCount();
	return particleCount;
}

void cEffect::SetFunctorGetZ(FunctorGetZ* func)
{
	RELEASE(func_getz);
	func_getz=func;
	func_getz->AddRef();

	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
		(*it)->SetFunctorGetZ(func_getz);

}

void cEffect::SetAttr(int attribute)
{
	__super::SetAttr(attribute);
	if(attribute&ATTRUNKOBJ_IGNORE)
	{
		vector<cEmitterInterface*>::iterator it;
		FOR_EACH(emitters,it)
			(*it)->Show(false);
	}
}

void cEffect::ClearAttr(int attribute)
{
	__super::ClearAttr(attribute);
	if(attribute&ATTRUNKOBJ_IGNORE)
	{
		vector<cEmitterInterface*>::iterator it;
		FOR_EACH(emitters,it)
			(*it)->Show(true);
	}
}


void cEffect::SetAutoDeleteAfterLife(bool auto_delete_after_life_)
{
	auto_delete_after_life=auto_delete_after_life_;
	if(GetAttr(ATTRUNKOBJ_IGNORE))
	{
		Release();
		return;
	}
}

bool cEffect::IsLive()
{
	bool live=false;
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
	{
		live=live || (*it)->IsLive();
	}
	vector<cLightingEmitter*>::iterator lt;
	FOR_EACH(lightings, lt)
		live|=(*lt)->IsLive();
	return live;
}
int cEffect::Release()
{
	RELEASE(model);
	return __super::Release();
}

void cEffect::Add(cEmitterInterface* p)
{
	emitters.push_back(p);
	p->SetPause(p->GetStartTime()>0);
}

void cEffect::Clear()
{
	time=0;
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
		delete *it;
	emitters.clear();
	vector<cLightingEmitter*>::iterator lit;
	FOR_EACH(lightings,lit)
		delete (*lit);
	lightings.clear();
}

void cEffect::CalcDistanceRate(float dist)
{
	if (dist < near_distance){
		distance_rate = 1;
		return;
	}
	else {
		distance_rate = 1-min(1.0f,(dist-near_distance)/(far_distance-near_distance));	
	}
}

bool cEffect::test_far_visible = false;
float cEffect::near_distance = 0.0f;
float cEffect::far_distance = 1e6f;
void cEffect::PreDraw(cCamera *pCamera)
{
	bool visible=false;
	bool is_noz_after_grass=false;
	bool is_noz_before_grass=false;
	bool mirage = false;
	vector<cEmitterInterface*>::iterator it;
	if (test_far_visible)
	{
		float d = GetPosition().trans().distance2(pCamera->GetPos());
		CalcDistanceRate(d);
		if (d>far_distance)
			return;
	}

	FOR_EACH(emitters,it)
	{
		if((*it)->IsVisible(pCamera))
		{
			visible=true;
		}

		if((*it)->IsDrawNoZBuffer()==1)
			is_noz_after_grass=true;
		if((*it)->IsDrawNoZBuffer()==2)
			is_noz_before_grass=true;

		if((*it)->IsMirage())
			mirage=true;
	}

	if(visible)
	{
		if (mirage)
			SetAttr(ATTRUNKOBJ_MIRAGE);
		else
			ClearAttr(ATTRUNKOBJ_MIRAGE);
		if(is_noz_after_grass)
			pCamera->Attach(SCENENODE_OBJECT_NOZ_AFTER_GRASS,this);
		if(is_noz_before_grass)
			pCamera->Attach(SCENENODE_OBJECT_NOZ_BEFORE_GRASS,this);
		pCamera->Attach(SCENENODE_OBJECTSORT,this);
		cCamera* pReflection=pCamera->FindChildCamera(ATTRCAMERA_REFLECTION);
		if(pReflection)
		{
			if(is_noz_after_grass)
				pReflection->Attach(SCENENODE_OBJECT_NOZ_AFTER_GRASS,this);
			if(is_noz_before_grass)
				pReflection->Attach(SCENENODE_OBJECT_NOZ_BEFORE_GRASS,this);
			pReflection->Attach(SCENENODE_OBJECTSORT,this);
		}
	}
}

void cEffect::Draw(cCamera *pCamera)
{
	bool old_fog_of_war=gb_RenderDevice3D->GetFogOfWar();
	if(no_fog_of_war)
		gb_RenderDevice3D->SetFogOfWar(false);

	gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_linear);
	vector<cEmitterInterface*>::iterator it;
	if (pCamera->GetAttribute(ATTRCAMERA_MIRAGE))
	{
		FOR_EACH(emitters,it)
			if((*it)->IsMirage())
			{
				(*it)->Draw(pCamera);
				enableMirage = true;
			}
	}else
	if(pCamera->GetCameraPass()==SCENENODE_OBJECT_NOZ_AFTER_GRASS)
	{
		FOR_EACH(emitters,it)
		if((*it)->IsDrawNoZBuffer()==1 && !(*it)->IsMirage())
			(*it)->Draw(pCamera);
	}else
	if(pCamera->GetCameraPass()==SCENENODE_OBJECT_NOZ_BEFORE_GRASS)
	{
		FOR_EACH(emitters,it)
		if((*it)->IsDrawNoZBuffer()==2 && !(*it)->IsMirage())
			(*it)->Draw(pCamera);
	}else
	{
		FOR_EACH(emitters,it)
		if(!(*it)->IsDrawNoZBuffer()&&!(*it)->IsMirage())
			(*it)->Draw(pCamera);
	}

	gb_RenderDevice3D->SetFogOfWar(old_fog_of_war);
#ifdef NEED_TREANGLE_COUNT
	int sum = 0;
	int sumOverDraw = 0;
	if (enableOverDraw && useOverDraw)
	{
		D3DSURFACE_DESC desc;
		gb_RenderDevice3D->SetRenderTarget(pRenderTexture,NULL);
		gb_RenderDevice3D->lpD3DDevice->Clear(0,NULL,D3DCLEAR_TARGET,0,0,0);
		drawOverDraw=true;
		FOR_EACH(emitters,it)
				(*it)->Draw(pCamera);
		drawOverDraw=false;
		gb_RenderDevice3D->SetRenderTarget(pRenderTextureCalc,NULL);
		gb_RenderDevice3D->lpD3DDevice->Clear(0,NULL,D3DCLEAR_TARGET,0,0,0);
		gb_RenderDevice3D->SetTexture(0,pRenderTexture);
		gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
		//gb_RenderDevice3D->SetPixelShader(NULL);
		psOverdrawCalc->Select(pRenderTexture->GetWidth(),pRenderTexture->GetHeight());

		gb_RenderDevice3D->lpBackBuffer->GetDesc(&desc);
		{
			cVertexBuffer<sVertexXYZWDT1>* pBuf=gb_RenderDevice3D->GetBufferXYZWDT1();
			sVertexXYZWDT1* v=	pBuf->Lock(4);

			v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
			v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
			v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=sColor4c(255,255,255,255);
			v[0].x=v[1].x=-0.5f; v[0].y=v[2].y=-0.5f; 
			v[3].x=v[2].x=-0.5f+pRenderTextureCalc->GetWidth(); v[1].y=v[3].y=-0.5f+pRenderTextureCalc->GetHeight(); 
			v[0].u1()=0;    v[0].v1()=0;
			v[1].u1()=0;    v[1].v1()=1;
			v[2].u1()=1; v[2].v1()=0;
			v[3].u1()=1; v[3].v1()=1;

			pBuf->Unlock(4);
			pBuf->DrawPrimitive(PT_TRIANGLESTRIP,2);
		}
		gb_RenderDevice3D->RestoreRenderTarget();

		gb_RenderDevice3D->SetTexture(0,pRenderTexture);
		gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
		//gb_RenderDevice3D->SetPixelShader(NULL);
		psOverdrawColor->Select();

		gb_RenderDevice3D->lpBackBuffer->GetDesc(&desc);
		cVertexBuffer<sVertexXYZWDT1>* pBuf=gb_RenderDevice3D->GetBufferXYZWDT1();
		sVertexXYZWDT1* v=	pBuf->Lock(4);

		v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
		v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
		v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=sColor4c(255,255,255,255);
		v[0].x=v[1].x=-0.5f; v[0].y=v[2].y=-0.5f; 
		v[3].x=v[2].x=-0.5f+desc.Width; v[1].y=v[3].y=-0.5f+desc.Height; 
		v[0].u1()=0;    v[0].v1()=0;
		v[1].u1()=0;    v[1].v1()=1;
		v[2].u1()=1; v[2].v1()=0;
		v[3].u1()=1; v[3].v1()=1;

		pBuf->Unlock(4);
		pBuf->DrawPrimitive(PT_TRIANGLESTRIP,2);
		IDirect3DSurface9 *pDestSurface=NULL;
		RDCALL(pRenderTextureCalc->GetDDSurface(0)->GetSurfaceLevel(0,&pDestSurface));
		RDCALL(gb_RenderDevice3D->lpD3DDevice->GetRenderTargetData(pDestSurface,pSysSurface));
		pSysSurface->GetDesc(&desc);
		D3DLOCKED_RECT LockedRect;
		RDCALL(pSysSurface->LockRect(&LockedRect,
			NULL,
			D3DLOCK_READONLY
			));

		for (int y=0; y<desc.Height; y++)
		for (int x=0; x<desc.Width; x++)
		{
			sColor4c* pix = (sColor4c*)(LockedRect.pBits)+x+y*LockedRect.Pitch/4;
			if (pix->r != 0)
			{
				sum++;
				sumOverDraw += pix->r;
			}
		}

		pSysSurface->UnlockRect();
		pDestSurface->Release();
		square_triangle = float(sumOverDraw)/float(sum);
	}
#endif
	
	
	
//	cInterfaceRenderDevice* rd=pCamera->GetRenderDevice();
//	Vect3f p = GetPosition().trans();
//	Mat3f r = GetPosition().rot();
//	rd->DrawLine(p, p+r.xrow()*100, sColor4c(255, 0, 0));
//	rd->DrawLine(p, p+r.yrow()*100, sColor4c(0, 255, 0));
//	rd->DrawLine(p, p+r.zrow()*100, sColor4c(0, 0, 255));
}

void cEffect::SetCycled(bool cycled)
{
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
		(*it)->SetCycled(cycled);

	vector<cLightingEmitter*>::iterator lt;
	FOR_EACH(lightings, lt)
		(*lt)->SetCycled(cycled);
}

bool cEffect::IsCycled()const
{
	vector<cEmitterInterface*>::const_iterator it;
	FOR_EACH(emitters,it)
	if((*it)->IsCycled())
		return true;

	vector<cLightingEmitter*>::const_iterator lt;
	FOR_EACH(lightings, lt)
	if((*lt)->IsCycled())
		return true;

	return false;
}

void cEffect::Animate(float dt)
{
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
	{
		cEmitterInterface* p=*it;
		bool b=time<p->GetStartTime();
		p->SetPause(b);
		p->Animate(dt);
	}
	vector<cLightingEmitter*>::iterator lt;
	FOR_EACH(lightings, lt)
		(*lt)->Animate(dt);
	time+=dt*1e-3f;
	if(auto_delete_after_life)
	{
		if(!IsLive())
			Release();
	}
}

string cEffect::GetName()
{
	return effect_name;
}

void cEffect::Init(EffectKey& effect_key_,c3dx* models)
{
	start_timer_auto();

	EffectKey* el=&effect_key_;
	effect_name=el->name;
#ifdef C_CHECK_DELETE
	check_file_name=el->filename;
	check_effect_name=el->name;
#endif

#ifdef _DEBUG
	debug_effect_key=el;
#endif _DEBUG

	Clear();
	model = models;
	link3dx.Link(model,0);
	vector<EmitterKeyInterface*>::iterator it;
	if(models)
	{
		models->AddRef();
		bool need_pos=false;
		bool need_normal=false;
		FOR_EACH(el->key,it)
		{
			EmitterKeyInterface* p=*it;
			EmitterKeyBase* pi=dynamic_cast<EmitterKeyBase*>(p);
			if(pi)
			{
				need_pos |= pi->particle_position.type==EMP_3DMODEL;
			}
			if (!need_normal&&need_pos)
			if(p->GetType()==EMC_INTEGRAL)
			{
				EmitterKeyInt* pi=(EmitterKeyInt*)p;
				if(pi->particle_position.type==EMP_3DMODEL)  
				{
					need_normal |= pi->use_light;
					for(int i=pi->begin_speed.size()-1;i>=0; --i)
						need_normal |= pi->begin_speed[i].velocity == EMV_NORMAL_3D_MODEL;
					
				}
			}else if (p->GetType()==EMC_SPLINE)
			{
				EmitterKeySpl* pi=(EmitterKeySpl*)p;
				switch(((EmitterKeySpl*)p)->direction)
				{
				case ETDS_BURST1: case ETDS_BURST2:
					need_normal = true;
					break;
				}
			}
		}
	}

	FOR_EACH(el->key,it)
	{
		EmitterKeyInterface* p=*it;
		switch(p->GetType())
		{
			case EMC_INTEGRAL:
			{
				cEmitterInt* pEmitter=new cEmitterInt;

				EmitterKeyInt* pi=(EmitterKeyInt*)p;
				pEmitter->SetEmitterKey(*pi,models);
				pEmitter->SetParent(this);
				Add(pEmitter);
				break;
			}
			case EMC_INTEGRAL_Z:
			{
				cEmitterZ* pEmitter=new cEmitterZ;

				EmitterKeyZ* pi=(EmitterKeyZ*)p;
				pEmitter->SetParent(this);
				if(func_getz)pEmitter->SetFunctorGetZ(func_getz);
				pEmitter->SetEmitterKey(*pi,models);
				Add(pEmitter);
				break;
			}
			case EMC_SPLINE:
			{
				cEmitterSpl* pEmitter=new cEmitterSpl;

				EmitterKeySpl* pi=(EmitterKeySpl*)p;
				pEmitter->SetEmitterKey(*pi,models);
				pEmitter->SetParent(this);
				Add(pEmitter);
				break;
			}
			case EMC_LIGHT:
			{
				cEmitterLight* pEmitter=new cEmitterLight;

				EmitterKeyLight* pi=(EmitterKeyLight*)p;
				pEmitter->SetEmitterKey(*pi);
				pEmitter->SetParent(this);
				Add(pEmitter);
				break;
			}
			case EMC_COLUMN_LIGHT:
			{
				cEmitterColumnLight* pEmitter=new cEmitterColumnLight;
				pEmitter->SetEmitterKey(*(EmitterKeyColumnLight*)p);
				pEmitter->SetParent(this);
				Add(pEmitter);
				break;
			}
			case EMC_LIGHTING:
			{
				cLightingEmitter* pLighting = new cLightingEmitter;
				pLighting->SetEmitterKey(*(EmitterLightingKey*)p,this);
				lightings.push_back(pLighting);
				break;
			}
		}
	}
	for(int i=0;i<emitters.size();++i)
		switch(el->key[i]->GetType())
		{
		case EMC_INTEGRAL:
		case EMC_INTEGRAL_Z:
		case EMC_SPLINE:
			if (((EmitterKeyBase*)el->key[i])->other!="")
			{
				int ix=0;
				FOR_EACH(el->key,it)
				{
					if (i!=ix && ((EmitterKeyBase*)el->key[i])->other == (*it)->name)
					{	
						xassert(dynamic_cast<cEmitterBase*>(emitters[ix]));
						((cEmitterBase*)emitters[i])->other = (cEmitterBase*)emitters[ix];
						((cEmitterBase*)emitters[ix])->isOther = true;
						break;
					}
					++ix;
				}
			}
			if (((EmitterKeyBase*)el->key[i])->velNoiseOther!="")
			{
				int ix=0;
				FOR_EACH(el->key,it)
				{
					if (i!=ix && ((EmitterKeyBase*)el->key[i])->velNoiseOther == (*it)->name)
					{	
						xassert(dynamic_cast<cEmitterBase*>(emitters[ix]));
						((cEmitterBase*)emitters[i])->velNoiseOther = (cEmitterBase*)emitters[ix];	
						break;
					}
					++ix;
				}
			}
			if (((EmitterKeyBase*)el->key[i])->dirNoiseOther!="")
			{
				int ix=0;
				FOR_EACH(el->key,it)
				{
					if (i!=ix && ((EmitterKeyBase*)el->key[i])->dirNoiseOther == (*it)->name)
					{	
						xassert(dynamic_cast<cEmitterBase*>(emitters[ix]));
						((cEmitterBase*)emitters[i])->dirNoiseOther = (cEmitterBase*)emitters[ix];	
						break;
					}
					++ix;
				}
			}
			break;
		}
}
#ifdef NEED_TREANGLE_COUNT
void cEffect::initOverdraw()
{
	psOverdraw = new PSOverdraw();
	psOverdraw->Restore();
	psOverdrawColor = new PSOverdrawColor();
	psOverdrawColor->Restore();
	psOverdrawCalc = new PSOverdrawCalc();
	psOverdrawCalc->Restore();
	if (useOverDraw)
	{
		D3DSURFACE_DESC desc;
		gb_RenderDevice3D->lpBackBuffer->GetDesc(&desc);
		pRenderTexture = GetTexLibrary()->CreateRenderTexture(desc.Width,desc.Height,TEXTURE_RENDER32);
		pRenderTextureCalc = GetTexLibrary()->CreateRenderTexture(desc.Width/4,desc.Height/4,TEXTURE_RENDER32);
		RDCALL(gb_RenderDevice3D->lpD3DDevice->CreateOffscreenPlainSurface(desc.Width/4,desc.Height/4,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&pSysSurface,NULL));
	}
}
#endif

float cEffect::GetSummaryTime()
{
	float time=0;
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
	{
		float t=(*it)->GetStartTime()+(*it)->GetLiveTime();
		time=max(time,t);
	}
	return time;
}

void cEffect::ShowEmitter(EmitterKeyInterface* emitter_id,bool show)
{
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
	{
		cEmitterInterface* p=*it;
		if(p->GetUnicalID()==emitter_id)
		{
			p->SetShowObjEditor(show);
			return;
		}
	}
	vector<cLightingEmitter*>::iterator itl;
	FOR_EACH(lightings,itl)
	{
		cLightingEmitter* p=*itl;
		if(p->GetUnicalID()==emitter_id)
		{
			p->SetShowObjEditor(show);
			return;
		}
	}
}

void cEffect::ShowAllEmitter()
{
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
	{
		(*it)->SetShowObjEditor(true);
	}
}

void cEffect::HideAllEmitter()
{
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
	{
		(*it)->SetShowObjEditor(false);
	}
}

void cEffect::SetPosition(const MatXf& Matrix)
{
	MatXf pos=Matrix;
	if(isTarget)
		pos.rot()=GlobalMatrix.rot();
	else {
		vector<cLightingEmitter*>::iterator it;
		FOR_EACH(lightings, it)
			(*it)->SetPosition(pos);
	}

	__super::SetPosition(pos);
}

void cEffect::SetTime(float t)
{
	time = t;
}
void cEffect::MoveToTime(float t)
{
	const float dt=1.0f;//милисекунд
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
		(*it)->SetDummyTime(dt);
	while(time<t)
	{
		Animate(dt);
		FOR_EACH(emitters,it)
		{
			(*it)->Animate(dt);
		}
	}
	FOR_EACH(emitters,it)
		(*it)->SetDummyTime(0.2f);
}

void cEffect::LinkToNode(class c3dx* object,int inode)
{
	link3dx.Link(object,inode);
	if(object)
		link3dx.Update();
}

const MatXf& cEffect::GetCenter3DModel()
{
	if(link3dx.IsInitialized())
		return link3dx.GetRootMatrix();
	return GetGlobalMatrix();
}

void cEffect::EffectObserverLink3dx::Link(class c3dx* object_,int inode)
{
	node=inode;

	if(object==object_)
		return;

	if(observer)
	{
		observer->BreakLink(this);
		observer=NULL;
	}
	object=object_;

	if(object)
		object->AddLink(this);
}

const MatXf& cEffect::EffectObserverLink3dx::GetRootMatrix()
{
	return object->GetPosition();
}

void cEffect::EffectObserverLink3dx::Update()
{
	xassert(IsLink());
	MatXf mat;
	mat=object->GetNodePositionMat(node);
	effect->SetPosition(mat);
}

void cEffect::StopAndReleaseAfterEnd()
{
	SetAutoDeleteAfterLife(true);
	vector<cEmitterInterface*>::iterator it;
	FOR_EACH(emitters,it)
	{
		cEmitterInterface* p=*it;
		p->SetCycled(false);
		p->DisableEmitProlonged();
	}
	link3dx.DeInitialization();

	vector<cLightingEmitter*>::iterator lt;
	FOR_EACH(lightings, lt)
	{
		(*lt)->SetCycled(false);
	}
}
void cEffect::ClearTriangleCount()
{
#ifdef  NEED_TREANGLE_COUNT
	count_triangle = 0;
	square_triangle = 0;
#endif
}
inline void cEffect::AddCountTriangle(int count)
{
#ifdef  NEED_TREANGLE_COUNT
	count_triangle+=count;
#endif
}
int cEffect::GetTriangleCount()
{
#ifdef  NEED_TREANGLE_COUNT
	return count_triangle;
#else
	return 0;
#endif
}
double cEffect::GetSquareTriangle()
{
#ifdef  NEED_TREANGLE_COUNT
	return square_triangle;
#else 
	return 0;
#endif
}


///////////////////////////////cEmitterZ///////////////////////////
/*
class FuncrorFieldZ:public FunctorMapZ
{
public:
	float GetZ(float pos_x,float pos_y)
	{
		float out_z=FunctorMapZ::GetZ(pos_x,pos_y);
		if(field_dispatcher)
		{
			float z=field_dispatcher->getGraphZ(pos_x,pos_y);
			if(out_z<z)
				out_z=z;
		}
		return out_z;
	}
};
*/
cEmitterZ::cEmitterZ()
{
	add_z=20;
	real_angle = 0;
	func_getz=NULL;
}

cEmitterZ::~cEmitterZ()
{
	RELEASE(func_getz);
}


void cEmitterZ::SetParent(cEffect* parent_)
{
	cEmitterInt::SetParent(parent_);
}

void cEmitterZ::ProcessTime(nParticle& p,float dtime_global,int i,Vect3f& cur_pos)
{
	float& t=p.time;

	vector<KeyParticleInt> & keys = GetEmitterKeyZ()->GetKey();
	KeyParticleInt* k0=&keys[p.key];
	KeyParticleInt* k1=&keys[p.key+1];

	if(p.key_begin_time>=0)
	{
		EffectBeginSpeedMatrix* s=&begin_speed[p.key_begin_time];
		while(s->time_begin<=p.time_summary)
		{
			Vect3f v=CalcVelocity(*s,p,1);
			
			float& t=p.time;
			float ts=t*k0->inv_dtime;
			p.pos0.x -= v.x*(t*(k0->vel+ts*0.5f*(k1->vel-k0->vel)));
			p.pos0.y -= v.y*(t*(k0->vel+ts*0.5f*(k1->vel-k0->vel)));
			p.vdir += v;
			xassert(p.vdir.z>=-100000 && p.vdir.z<=100000);
			
			p.key_begin_time++;
			if(p.key_begin_time>=begin_speed.size())
			{
				p.key_begin_time=-1;
				break;
			}else
				s=&begin_speed[p.key_begin_time];
		}

	}

//	p.pos0.z=CalcZ(cur_pos.x,cur_pos.y);
	int keys_size2 = keys.size()-2;
	while(t+dtime_global>k0->dtime)
	{
		//t==1
		float tprev=t;
		t=k0->dtime;
		float ts=t*k0->inv_dtime;
		const Vect3f & g = GetEmitterKeyZ()->g;
		p.pos0.x = p.pos0.x+p.vdir.x*(t*(k0->vel+ts*0.5f*(k1->vel-k0->vel)))+g.x*((p.gvel0+t*k0->gravity*0.5f)*t);
		p.pos0.y = p.pos0.y+p.vdir.y*(t*(k0->vel+ts*0.5f*(k1->vel-k0->vel)))+g.y*((p.gvel0+t*k0->gravity*0.5f)*t);
		p.angle0 = p.angle0 + p.angle_dir*(k0->angle_vel*t+t*ts*0.5f*(k1->angle_vel-k0->angle_vel));
		p.gvel0+=t*k0->gravity;

		float dtime=dtime_global*p.inv_life_time;

		//if(p.key==keys_size2 && t+dtime>k0->dtime)
		//{
		//	Particle.SetFree(i);
		//	p.key = -1;
		//	break;
		//}

		dtime_global-=k0->dtime-tprev;
		t=0;
		p.key++;
		
		if(p.key>=keys.size()-1)
		{
			SetFreeOrCycle(i);
			break;
		}

		k0=k1;
		k1=&keys[p.key+1];
		xassert(p.key<=keys_size2);
	}

	t+=dtime_global;
	p.time_summary+=dtime_global;
}

void cEmitterZ::Draw(cCamera *pCamera)
{
	if(no_show_obj_editor)
		return;
	if(is_intantly_infinity_particle && parent->GetParticleRateReal()<=0 )
		return;
//	dprintf("Draw\n");
	bool not_draw=true;
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	float dtime_global=(time-old_time);
	MatXf mat=pCamera->GetMatrix();
	//Bound.SetInvalidBox();
	if(GetEmitterKeyZ()->relative)
	{
		Bound.max.set(0,0,0);
		Bound.min.set(0,0,0);
	}else
	{
		Bound.max = GlobalMatrix.trans();
		Bound.min = GlobalMatrix.trans();
	}

	cTextureAviScale* texture = NULL;
	cTextureComplex* textureComplex = NULL;
	//	cTextureAviScale* plume_texture = NULL;
	if (GetTexture(0) && GetTexture(0)->IsAviScaleTexture())
		texture = (cTextureAviScale*)GetTexture(0);
	if (GetTexture(0) && GetTexture(0)->IsComplexTexture())
		textureComplex = (cTextureComplex*)GetTexture(0);

	TerraInterface* terra = NULL;
	if (pCamera->GetScene()->GetTileMap())
		terra = pCamera->GetScene()->GetTileMap()->GetTerra();
	cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();

	Vect3f CameraPos;
	UCHAR mode;
	if(GetEmitterKeyZ()->chPlume)
	{
		CameraPos = pCamera->GetPos();
		CameraPos = GetEmitterKeyZ()->relative ? GlobalMatrix.invXformPoint(CameraPos) : pCamera->GetPos();
		mode = (UCHAR)GetEmitterKeyZ()->planar + (GetEmitterKeyZ()->smooth ? 0 : 2);
	}
	gb_RenderDevice->SetSamplerDataVirtual(0,sampler_wrap_linear);
	bool reflectionz=pCamera->GetAttr(ATTRCAMERA_REFLECTION);
	{//Ќемного не к месту, зато быстро по скорости, дл€ отражений.
		gb_RenderDevice3D->SetTexture(5,pCamera->GetZTexture());
		gb_RenderDevice3D->SetSamplerData(5,sampler_clamp_linear);
	}
	gb_RenderDevice3D->SetWorldMaterial(blend_mode, GetEmitterKeyZ()->relative ? GlobalMatrix:MatXf::ID, 0, GetTexture(0),0,COLOR_MOD,softSmoke,reflectionz);
#ifdef NEED_TREANGLE_COUNT
	if (parent->drawOverDraw)
	{
		gb_RenderDevice3D->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);
		gb_RenderDevice3D->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_ONE);
		gb_RenderDevice3D->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
		parent->psOverdraw->Select();
	}
#endif
	if(GetEmitterKeyZ()->relative)
	{
		pBuf->BeginDraw(GlobalMatrix);
	}
	else pBuf->BeginDraw();
	int size=Particle.size();
	vector<KeyParticleInt> & keys = GetEmitterKeyZ()->GetKey();
	int keys_size2 = keys.size()-2;
	for(int i=size-1;i>=0;i--)
	{
		if(Particle.IsFree(i))
			continue;
		nParticle& p=Particle[i];

		Vect3f pos;
		float angle;
		sColor4f fcolor;
		float psize;

		float dtime=dtime_global*p.inv_life_time;
		
		KeyParticleInt& k0=keys[p.key];
		KeyParticleInt& k1=keys[p.key+1];
		float& t=p.time;

		if (p.key==keys_size2&&t+dtime>k0.dtime)//–исовать частицу в последний квант, потому как при коротких промежутках времени может вообще не нарисоватьс€.
		{
//			dprintf("Free time=%f, t=%f, dtime=%f\n",time,t,dtime);
			if(SetFreeOrCycle(i))
				continue;
		}

		float ts=t*k0.inv_dtime;
		if(GetEmitterKeyZ()->need_wind && terra)
		{
			const Vect2f& wvel = terra->GetWindVelocity(p.pos0.x, p.pos0.y);
			float k_wind = graphRnd.fabsRnd(GetEmitterKeyZ()->k_wind_min,GetEmitterKeyZ()->k_wind_max);
			//const float & k_wind = GetEmitterKeyZ()->k_wind;
			p.pos0.x += k_wind*wvel.x*dtime_global;
			p.pos0.y += k_wind*wvel.y*dtime_global;
		}
		const Vect3f & g = GetEmitterKeyZ()->g;
		pos.x = p.pos0.x+p.vdir.x*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.x*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.y = p.pos0.y+p.vdir.y*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.y*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.z = CalcZ(pos.x,pos.y); 
		Bound.AddBound(pos);
		angle = p.angle0 + p.angle_dir*(k0.angle_vel*t+t*ts*0.5f*(k1.angle_vel-k0.angle_vel));
		real_angle = angle;
		angle += -p.baseAngle;
		fcolor = k0.color+(k1.color-k0.color)*ts;
		psize = k0.size+(k1.size-k0.size)*ts;

		//ƒобавить в массив
		Vect3f sx,sy;
		Vect2f rot=rotate_angle[round(angle*rotate_angle_size)&rotate_angle_mask];
		rot*=psize*=p.begin_size;
		if(GetEmitterKeyZ()->planar)
		{
			sx.x=-rot.y;
			sx.y=-rot.x;
			sx.z=0;
			sy.x=-rot.x;
			sy.y=+rot.y;
			sy.z=0;
		}else
		{
			mat.invXformVect(Vect3f(+rot.x,-rot.y,0),sx);
			mat.invXformVect(Vect3f(+rot.y,+rot.x,0),sy);
		}

		sColor4c color(round(fcolor.r),round(fcolor.g),round(fcolor.b),round(fcolor.a));
		sRectangle4f rt;
		if(GetEmitterKeyZ()->randomFrame && textureComplex)
			rt = ((cTextureComplex*)textureComplex)->GetFramePos(p.nframe);
		else
			rt = texture?((cTextureAviScale*)texture)->GetFramePos(cycle(p.time_summary,1.f)) :
																									sRectangle4f::ID;	

		if(GetEmitterKeyZ()->chPlume)
		{
			#ifdef  NEED_TREANGLE_COUNT
				parent->AddCountTriangle(p.plume.plumes.size()*2);
			#endif
			if(mode&2) 
			{
				xassert(dynamic_cast<cEmitterZ*>(this));
				pos.z = ((cEmitterZ*)this)->CalcZ(pos.x,pos.y); 
			}
			if (p.plume.PutToBuf(pos,dtime,pBuf,color,CameraPos,psize, rt, mode,GetEmitterKeyZ()->PlumeInterval,p.time_summary))
			{
				continue;
			}
		}
		else 
		{
			if (sizeByTexture)
			{
				if(textureComplex)
				{
					scaleX = scaleY = 1.0f;
					float w = textureComplex->sizes[p.nframe].x;
					float h = textureComplex->sizes[p.nframe].y;
					if (h>w)
					{
						scaleY = float(h)/float(w);
					}else if(w>h)
					{
						scaleX = float(w)/float(h);
					}
				}

				sx*=scaleX;
				sy*=scaleY;
			}
			sVertexXYZDT1 *v=pBuf->Get();

			v[0].pos.x=pos.x-sx.x-sy.x; v[0].pos.y=pos.y-sx.y-sy.y; v[0].pos.z=pos.z-sx.z-sy.z; 
			v[0].diffuse.b=color.b;	v[0].diffuse.g=color.g; v[0].diffuse.r=color.r; v[0].diffuse.a=color.a; 
			v[0].GetTexel().x = rt.min.x; v[0].GetTexel().y = rt.min.y;	//	(0,0);

			v[1].pos.x=pos.x-sx.x+sy.x;	v[1].pos.y=pos.y-sx.y+sy.y;	v[1].pos.z=pos.z-sx.z+sy.z;
			v[1].diffuse.b=color.b; v[1].diffuse.g=color.g;	v[1].diffuse.r=color.r;	v[1].diffuse.a=color.a;
			v[1].GetTexel().x = rt.min.x; v[1].GetTexel().y = rt.max.y;//	(0,1);

			v[2].pos.x=pos.x+sx.x-sy.x; v[2].pos.y=pos.y+sx.y-sy.y; v[2].pos.z=pos.z+sx.z-sy.z; 
			v[2].diffuse.b=color.b; v[2].diffuse.g=color.g; v[2].diffuse.r=color.r; v[2].diffuse.a=color.a; 
			v[2].GetTexel().x = rt.max.x; v[2].GetTexel().y = rt.min.y;	//  (1,0);

			v[3].pos.x=pos.x+sx.x+sy.x;	v[3].pos.y=pos.y+sx.y+sy.y;	v[3].pos.z=pos.z+sx.z+sy.z;
			v[3].diffuse.b=color.b; v[3].diffuse.g=color.g; v[3].diffuse.r=color.r; v[3].diffuse.a=color.a; 
			v[3].GetTexel().x = rt.max.x;v[3].GetTexel().y = rt.max.y;//  (1,1);
			//v[0].pos=pos-sx-sy; v[0].diffuse=color; v[0].GetTexel().set(rt.left, rt.top);	//set(0,0);
			//v[1].pos=pos-sx+sy; v[1].diffuse=color; v[1].GetTexel().set(rt.left, rt.bottom);//set(0,1);
			//v[2].pos=pos+sx-sy; v[2].diffuse=color; v[2].GetTexel().set(rt.right,rt.top);	//set(1,0);
			//v[3].pos=pos+sx+sy; v[3].diffuse=color; v[3].GetTexel().set(rt.right,rt.bottom);//set(1,1);
			#ifdef  NEED_TREANGLE_COUNT
				parent->AddCountTriangle(2);
			#endif
//			dprintf("visible ");
			not_draw=false;

		}
		ProcessTime(p,dtime,i,pos);

	}

	pBuf->EndDraw();
//	dprintf("\n");

	Particle.Compress();

	old_time=time;
}

bool cEmitterZ::GetRndPos(Vect3f& pos, Vect3f* norm)
{
	if (Particle.is_empty())
	{
		cur_one_pos = -1;
		pos.set(0,0,0);
		if (norm)
			norm->set(0,0,0);
		return false;
	}
	else
	{
		int size;
		int i = graphRnd(size = Particle.size());
		while(Particle.IsFree(i)) 
			i = graphRnd(size);
		cur_one_pos = i;
		nParticle& p = Particle[i];
		vector<KeyParticleInt> & keys = GetEmitterKeyZ()->GetKey();
		KeyParticleInt& k0=keys[p.key];
		KeyParticleInt& k1=keys[p.key+1];
		
		float& t=p.time;
		float ts=t*k0.inv_dtime;
		const Vect3f & g = GetEmitterKeyZ()->g;
		pos.x = p.pos0.x+p.vdir.x*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.x*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.y = p.pos0.y+p.vdir.y*(t*(k0.vel+ts*0.5f*(k1.vel-k0.vel)))+g.y*((p.gvel0+t*k0.gravity*0.5f)*t);
		pos.z = p.pos0.z;
		if (norm)
			*norm = Particle[i].normal;
		return true;
	}
}

void cEmitterZ::EmitOne(int ix_cur/*nParticle& cur*/,float begin_time, int num_pos)
{
	xassert((UINT)ix_cur<Particle.size());
	nParticle& cur = Particle[ix_cur];
	cEmitterInt::EmitOne(ix_cur,begin_time);
	if(GetEmitterKeyZ()->angle_by_center)
	{
		Vect3f& v = GetEmitterKeyZ()->relative ? cur.pos0 : cur.pos0 - GlobalMatrix.trans();
		cur.angle0 = real_angle;
		cur.baseAngle = (atan2(cur.vdir.y, cur.vdir.x) + GetEmitterKeyZ()->base_angle) * INV_2_PI;
	}else
	{
		cur.baseAngle = -GetEmitterKeyZ()->base_angle * INV_2_PI;
		cur.angle0 = real_angle;
	}
	if(GetEmitterKeyZ()->randomFrame && GetTexture() && GetTexture()->IsComplexTexture())
		cur.nframe = graphRnd(((cTextureAviScale*)GetTexture())->GetFramesCount());

//	cur.pos0.z=CalcZ(cur.pos0.x,cur.pos0.y);
	if(GetEmitterKeyZ()->chPlume)
	{
		float z = CalcZ(cur.pos0.x,cur.pos0.y);
		vector<cPlumeOne>::iterator it;
		FOR_EACH(cur.plume.plumes,it)
			it->pos.z = z;
	}
}

float cEmitterZ::CalcZ(float pos_x,float pos_y)
{
	if(GetEmitterKeyZ()->relative)
	{
		const MatXf& GM  = GlobalMatrix;
		Vect3f p; //word coord
		p.x = (GM.rot().xrow().x*pos_x + GM.rot().xrow().y*pos_y)  +  GM.trans().x;
		p.y = (GM.rot().yrow().x*pos_x + GM.rot().yrow().y*pos_y)  +  GM.trans().y;
		p.z = func_getz->GetZ(p.x,p.y)+add_z; 
		p.sub(GM.trans());
		return GM.rot().zrow().x*p.x + GM.rot().zrow().y*p.y + GM.rot().zrow().z*p.z;
	}
	else 
		return func_getz->GetZ(pos_x,pos_y)+add_z;
}

void cEmitterZ::SetEmitterKey(EmitterKeyZ& k,c3dx* models)
{
	cEmitterInt::SetEmitterKey(k,models);
	add_z=k.add_z;

	if(!func_getz)
	{
/*
		if(GetEmitterKeyZ()->use_force_field)
			func_getz=new FuncrorFieldZ;
		else
		func_getz=new FunctorMapZ;
*/
		xassert(!GetEmitterKeyZ()->use_force_field);
		if(GetEmitterKeyZ()->use_water_plane)
		{
			//–аз уж они так св€занны, лучше было бы объединить use_water_plane и draw_first_no_zbuffer
			func_getz=parent->GetScene()->GetWaterFunctor();
			draw_first_no_zbuffer=0;
		}else
			func_getz=parent->GetScene()->GetTerraFunctor();
		func_getz->AddRef();
	}
}

///////////////////////////////////cEmitterLight/////////////////////
cEmitterLight::cEmitterLight()
{
	light=NULL;
}

cEmitterLight::~cEmitterLight()
{
	RELEASE(light);
}

void cEmitterLight::SetEmitterKey(EmitterKeyLight& k)
{
	if(k.texture_name.empty())
		SetTexture(0,GetTexLibrary()->GetSpericalTexture());
	else
		SetTexture(0,GetTexLibrary()->GetElement3D(k.texture_name.c_str()));
	emitter_key = &k;
	cycled=k.cycled;
	emitter_life_time=k.emitter_life_time;

	switch(k.light_blend) 
	{
	case EMITTER_BLEND_ADDING:
		blend_mode = ALPHA_ADDBLEND; break;

	case EMITTER_BLEND_SUBSTRACT:
		blend_mode = ALPHA_SUBBLEND; break;

	default:
		blend_mode = ALPHA_ADDBLEND;
	}
}

void cEmitterLight::Show(bool show)
{
	if(light)
	{
		light->PutAttr(ATTRUNKOBJ_IGNORE,!show);
	}
}

bool cEffect::IsLinkIgnored()
{
	if(!link3dx.IsInitialized())
		return false;
	return link3dx.object->GetAttr(ATTRUNKOBJ_IGNORE);
}

void cEmitterLight::Animate(float dt)
{
	if(b_pause)return;
	if(no_show_obj_editor)
	{
		if (light)
			RELEASE(light);
		return;
	}
	dt*=1e-3f;
	if(dt>0.1f)
		dt=0.1f;

	float t=time/emitter_life_time;
	bool hide=parent->GetParticleRateReal()<=0 || parent->GetAttr(ATTRUNKOBJ_IGNORE) || parent->IsLinkIgnored();
	if(light)
	{
		light->PutAttr(ATTRUNKOBJ_IGNORE,hide);
	}

	if(t<1)
	{
		if(!light && !hide)
		{
			cTexture* pTexture=GetTexture(0);
			if(pTexture)pTexture->AddRef();
			light=parent->GetScene()->CreateLight(0,pTexture);

			light->PutAttr(ATTRLIGHT_SPHERICAL_OBJECT, GetEmitterKeyLight()->toObjects);
			light->PutAttr(ATTRLIGHT_SPHERICAL_TERRAIN, GetEmitterKeyLight()->toTerrain);
		}

		if(light)
		{
			LocalMatrix.trans() = GetEmitterKeyLight()->emitter_position.Get(t);
			LocalMatrix.rot()=Mat3f::ID;
			GlobalMatrix=parent->GetGlobalMatrix()*GetLocalMatrix();
			float size = GetEmitterKeyLight()->emitter_size.Get(t);
			light->SetScale(size);
			sColor4f color = GetEmitterKeyLight()->emitter_color.Get(t);
			light->SetDiffuse(color);
			light->SetPosition(GlobalMatrix);
			light->SetBlending(blend_mode);
		}
	}else
	{
		if(cycled)
			time-=emitter_life_time;
		else
			RELEASE(light);
	}

	time+=dt;
}

bool GetAllTextureNamesEffectLibrary(const char* filename, vector<string>& names)
{
	xassert(gb_VisGeneric);
	char drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME], ext[_MAX_EXT];
	_splitpath(filename,drive,dir,fname,ext);
	string tex_dir=drive;
	tex_dir+=dir;
	tex_dir+="Textures";

	EffectKey* effect_key = gb_EffectLibrary->Get(filename,1.0f,tex_dir.c_str());
	if(effect_key==0)
        return false;

	vector<EmitterKeyInterface*>::iterator emitter;
	FOR_EACH(effect_key->key, emitter)
	{
		(*emitter)->GetTextureNames(names);
	}

	return true;
}

void cEffect::SetTarget(const Vect3f& pos_begin, const vector<Vect3f>& pos_end)
{
	SetTarget(pos_begin,pos_end.empty()?NULL:&pos_end[0],pos_end.size());
}

void cEffect::SetTarget(const Vect3f& pos_begin, const Vect3f& pos_end)
{
	SetTarget(pos_begin,&pos_end,1);
}

void cEffect::SetTarget(const Vect3f& pos_begin, const Vect3f* pos_end,int pos_end_size)
{
	MTAccess();
	xassert(pos_end && pos_end_size>0);
	isTarget = true;
	Mat3f rot;
	Vect3f front = pos_end[0] - pos_begin;
	MatrixGrandSmittNormalizationYZ(rot,front,Vect3f::K);


	__super::SetPosition(MatXf(rot, pos_begin));

	vector<cLightingEmitter*>::iterator lt;
	FOR_EACH(lightings, lt)
		(*lt)->SetTarget(pos_end,pos_end_size);	
	vector<cEmitterInterface*>::iterator em;
	FOR_EACH(emitters, em)
		(*em)->SetTarget(pos_end,pos_end_size);
}

PerlinNoise::PerlinNoise()
{
	countValues_ = 0;
	numOctaves_ = 0;
	frequency_ = 1;
	amplitude_ = 1;
	positive_ = false;
}
PerlinNoise::~PerlinNoise()
{
}

float PerlinNoise::Noise(int x)
{
	x = (x<<13) ^ x;
	return ( 1.0 - ( (x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0);    
}
void PerlinNoise::InitRndValues(int oct,bool first)
{
	vector<float> &vf = rndValues_[oct];
	if (!first)
	{
		vf[0] = vf[vf.size()-3];
		vf[1] = vf[vf.size()-2];
		vf[2] = vf[vf.size()-1];
		for(int i=3; i<vf.size(); i++)
		{
			if(positive_)
				vf[i] = graphRnd.fabsRnd();
			else
				vf[i] = graphRnd.frnd();
		}
	}else
	{
		for(int i=0; i<vf.size(); i++)
		{
			if(positive_)
				vf[i] = graphRnd.fabsRnd();
			else
				vf[i] = graphRnd.frnd();
		}
	}
}

float PerlinNoise::Interpolate(float x, int oct)
{
	int integer_x = int(x);
	float fractional_x = x - integer_x;
	integer_x = integer_x%(rndValues_[oct].size()-3)+1;
	if (oldx[oct] > integer_x)
		InitRndValues(oct);
	oldx[oct] = integer_x;

	vector<float> &vf = rndValues_[oct];

	float &v1 = vf[integer_x];
	float &v2 = vf[integer_x+1];

	//return LinearInterpolate(v1,v2,fractional_x);
	return CosInterpolate(v1,v2,fractional_x);
	//return ArcLengthInterpolate(v1,v2,fractional_x);
}

float PerlinNoise::Get(float x)
{
	float total =0;
	float frequency = frequency_;
	float amplitude = amplitude_;
	for (int i=0; i<numOctaves_; i++)
	{
		total += Interpolate(x*frequency,i)*octavesAmplitude_[i].f*amplitude;
		frequency *=2;
	}
	return total;
}
void PerlinNoise::SetParameters(int countValues,float frequency,float amplitude,CKey octavesAmplitude,bool onlyPositive,bool refresh)
{
	countValues_ = countValues;
	numOctaves_ = octavesAmplitude.size();
	frequency_ = frequency;
	amplitude_ = amplitude;
	octavesAmplitude_ = octavesAmplitude;
	positive_ = onlyPositive;
	rndValues_.resize(numOctaves_);
	if (refresh)
	{
		float fr = frequency_;
		for (int i=0; i<rndValues_.size(); i++)
		{
			int size = (countValues_*fr);
			if (size < 1)
				size = 1;
			rndValues_[i].resize(size+3);
			fr*=2;
			InitRndValues(i,true);
		}
	}
	oldx.clear();
	oldx.resize(numOctaves_);
}

void cEffect::SetParticleRate(float rate)
{
	particle_rate=rate;
}


#define FLOAT_EQUAL(a, b) (fabs((a) - (b)) < 1e-4f)

EffectKey * EffectLibrary2::Get(const char * filename, float scale,const char* texture_path,sColor4c skin_color)
{
	MTAuto mtauto(mtlock);
	string normalized_filename;
	string normalized_texture_path;
	normalize_path(filename,normalized_filename);
	normalize_path(texture_path,normalized_texture_path);

	sColor4c white(255,255,255);

	for(int i = 0; i < list.size(); i++)
	{
		Entry & e = list[i];
		if(e.pEffect->filename == normalized_filename)
		{
			if(e.texture_path==normalized_texture_path)
			{
				if(FLOAT_EQUAL(e.scale, scale))
				{
					if(e.skin_color.RGBA()==skin_color.RGBA())
					{
						//if(Option_DprintfLoad)
						//	dprintf("CACHE %s %f\n",e.pEffect->filename.c_str(),scale);
						return e.pEffect;
					}
				}
			}
		}
	}

	for(int i = 0; i < list.size(); i++)
	{
		Entry & e = list[i];
		if(e.pEffect->filename == normalized_filename)
		{
			if(e.texture_path==normalized_texture_path)
			{
				if(FLOAT_EQUAL(e.scale, 1.0f) && e.skin_color.RGBA()==white.RGBA())
					return Copy(e.pEffect,scale,normalized_texture_path.c_str(),skin_color);
			}
		}
	}
	EffectKey* effect=Load(normalized_filename.c_str(), normalized_texture_path.c_str());
	return Copy(effect,scale,normalized_texture_path.c_str(),skin_color);
}

EffectKey * EffectLibrary2::Load(const char * filename, const char* texture_path)
{
	CLoadDirectoryFileRender s;

	if(!s.Load(filename))
	{
		if(Option_DprintfLoad)
			dprintf("NOT LOAD %s\n",filename);
		return NULL;
	}
	if(Option_DprintfLoad)
		dprintf("LOAD %s\n",filename);

	sColor4c white(255,255,255);
	float scale=1.0f;
	sColor4c skin_color=white;

	while(CLoadData * ld = s.next())
	{
		if(ld->id == IDS_EFFECTKEY)
		{
			EffectKey* ek = new EffectKey;
			Entry entry;
			ek->filename = filename;
			entry.scale = scale;
			entry.pEffect = ek;
			entry.texture_path = texture_path;
			entry.skin_color=skin_color;
			list.push_back(entry);

			ek->Load(ld);
			ek->changeTexturePath(texture_path);
			ek->preloadTexture();

			if(!FLOAT_EQUAL(scale, 1.0f))
				ek->RelativeScale(scale);

			if(skin_color.RGBA()!=white.RGBA())
				ek->MulToColor(skin_color);

			return ek;
		}
	}

	return NULL;
}

EffectKey* EffectLibrary2::Copy(EffectKey* ref,float scale,const char* texture_path,sColor4c skin_color)
{
	if(ref==NULL)
		return NULL;
	sColor4c white(255,255,255);
	if(FLOAT_EQUAL(scale, 1.0f) && skin_color.RGBA()==white.RGBA())
	{
		//if(Option_DprintfLoad)
		//	dprintf("CACHE %s %f\n",ref->filename.c_str(),scale);
		return ref;
	}
	if(Option_DprintfLoad)
		dprintf("COPY %s %f\n",ref->filename.c_str(),scale);
	
	EffectKey* ek = new EffectKey;
	Entry entry;
	entry.scale = scale;
	entry.pEffect = ek;
	entry.texture_path = texture_path;
	entry.skin_color=skin_color;
	list.push_back(entry);

	*ek=*ref;
//	ek->changeTexturePath(texture_path); “ут нельз€ мен€ть, должно совпадать с загруженным.
	ek->preloadTexture();

	if(!FLOAT_EQUAL(scale, 1.0f))
		ek->RelativeScale(scale);

	if(skin_color.RGBA()!=white.RGBA())
		ek->MulToColor(skin_color);

	return ek;
}

void EffectLibrary2::preloadLibrary(const char * filename, const char* texture_path)
{
	Get(filename,1,texture_path);
}

EffectLibrary2::~EffectLibrary2()
{
	vector<Entry>::iterator it;
	FOR_EACH(list,it)
	{
		Entry& e=*it;
		delete e.pEffect;
	}
}

#undef FLOAT_EQUAL

