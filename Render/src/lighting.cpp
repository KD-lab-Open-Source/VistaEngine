#include "StdAfxRD.h"
#include "lighting.h"
#include "Texture.h"
#include "TexLibrary.h"
#include "D3DRender.h"
#include "cCamera.h"
#include "Serialization\Serialization.h"

LightingParameters::LightingParameters()
{
	generate_time=0.1f;
	texture_name="Scripts\\Resource\\balmer\\freeze.tga";
	strip_width_begin=5;
	strip_width_time=15;
	strip_length=40;
	fade_time=0.5f;
	lighting_amplitude=40;
}

void LightingParameters::serialize(Archive& ar)
{
	ar.serialize(generate_time, "generate_time", "generate_time");
	ar.serialize(texture_name, "texture_name", "texture_name");
	ar.serialize(strip_width_begin, "strip_width_begin", "strip_width_begin");
	ar.serialize(strip_width_time, "strip_width_time", "strip_width_time");
	ar.serialize(strip_length, "strip_length", "strip_length");
	ar.serialize(fade_time, "fade_time", "fade_time");
	ar.serialize(lighting_amplitude, "lighting_amplitude", "lighting_amplitude");
}

Lighting::Lighting()
:BaseGraphObject(0)
{
	time=0;
	pTexture=0;//GetTexLibrary()->GetElement(param.texture_name.c_str());
	stopGenerate = false;
	pos_begin=pos_delta=Vect3f::ZERO;
	global_pos=MatXf::ID;

	color_ = Color4f(1.f, 1.f, 1.f, 1.f);
}

Lighting::~Lighting()
{
	list<OneLight*>::iterator it;
	FOR_EACH(lights,it)
		delete *it;
	lights.clear();

	RELEASE(pTexture);
}

void Lighting::SetParameters(LightingParameters& param_)
{
	param=param_;
	//xassert(param.generate_time>0.05f);
	RELEASE(pTexture);
	//pTexture=GetTexLibrary()->GetElement(param.texture_name.c_str());
	pTexture=GetTexLibrary()->GetElement2D(param.texture_name.c_str());
	param.fade_time=max(param.fade_time,0.01f);
}

void Lighting::PreDraw(Camera* camera)
{
//	camera->Attach(SCENENODE_OBJECTSORT, this);

	vector<PreGenerate>::iterator it;
	FOR_EACH(pre_generate,it)
	{
		Generate(it->pos_begin,it->pos_end,camera);
	}
	pre_generate.clear();
}

void Lighting::Draw(Camera* camera)
{
	list<OneLight*>::iterator it;
	FOR_EACH(lights,it)
		(*it)->Draw(camera,this);
}

void Lighting::OneLight::Draw(Camera* camera,Lighting* parent)
{
/*
	Color4c color(255,255,255);
	for(int i=1;i<position.size();i++)
	{
		Vect3f& n0=position[i-1];
		Vect3f& n1=position[i];
		gb_RenderDevice->DrawLine(n0,n1,color);
	}
/**/
/*
	float a=1-time;
	Color4c diffuse(255,255*a,255,a*255);
	gb_RenderDevice->SetNoMaterial(ALPHA_ADDBLENDALPHA,0,pTexture);
	DrawStrip strip;
	strip.Begin();
	float size=5+time*15;
	sVertexXYZDT1 v1,v2;
	v1.diffuse=diffuse;
	v2.diffuse=diffuse;
	for(int i=0;i<position.size();i++)
	{
		Vect3f& p=position[i];
		Vect3f n;
		if(i==0)
			n=position[i+1]-position[i];
		else
		if(i==position.size()-1)
			n=position[i]-position[i-1];
		else
			n=position[i+1]-position[i-1];
		n.normalize();
		n=n%Vect3f(0,0,1);
			
		float t=5*i/(float)position.size();
		float sz=size;
//		if(i==0)
//			sz=0;
		v1.pos=p;
		v2.pos=p;
//		v1.pos.y-=sz;
//		v2.pos.y+=sz;
		v1.pos-=sz*n;
		v2.pos+=sz*n;

		v1.u1()=v2.u1()=t;
		v1.v1()=0;v2.v1()=1;
		strip.Set(v1,v2);
	}
	strip.End();
/**/
	float a=1-time;
	Color4f parent_color = parent->color();
	parent_color.a *= a;
	Color4c diffuse = parent_color;
	gb_RenderDevice->SetWorldMaterial(ALPHA_ADDBLENDALPHA,MatXf::ID,0,parent->pTexture);
	gb_RenderDevice->SetSamplerDataVirtual(0,sampler_clamp_anisotropic);
	//gb_RenderDevice3D->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	DrawStrip strip;
	strip.Begin();
	float size=parent->param.strip_width_begin+time*parent->param.strip_width_time;
	sVertexXYZDT1 v1,v2;
	v1.diffuse=diffuse;
	v2.diffuse=diffuse;
	for(int i=0;i<strip_list.size();i++)
	{
		OneStrip& p=strip_list[i];
		v1.pos=p.pos;
		v2.pos=p.pos;
		v1.pos-=size*p.n;
		v2.pos+=size*p.n;

		v1.u1()=v2.u1()=p.u;
		v1.v1()=0;v2.v1()=1;
		strip.Set(v1,v2);
	}
	strip.End();
}

void Lighting::Animate(float dt)
{
	dt*=1e-3f;
	if(!stopGenerate)
	{
		time+=dt;
		while(time>=param.generate_time)
		{
			time-=param.generate_time;
			PreGenerate g;
			g.pos_begin=pos_begin;
			g.pos_end=pos_end[graphRnd()%pos_end.size()];
			pre_generate.push_back(g);
		}
	}else
		time=0;

	float fade_dt=dt/param.fade_time;
	for(list<OneLight*>::iterator it=lights.begin();it!=lights.end();)
	{
		OneLight* p=*it;
		p->Animate(fade_dt);
		if(p->time>=1){
			delete *it;
			it=lights.erase(it);
		}
		else
			++it;
	}
}

void Lighting::Update(const Vect3f& pos_begin_, const vector<Vect3f>& pos_end_)
{
	global_pos.set(Mat3f::ID,pos_begin_);
	pos_delta = pos_begin_ - pos_begin;
	pos_begin = pos_begin_;
	pos_end = pos_end_;
	list<OneLight*>::iterator it;
	FOR_EACH(lights,it)
	{
		OneLight* p=*it;
		p->Update(pos_delta);
	}
	xassert(!pos_end.empty());
}

bool Lighting::IsLive()
{
	if (lights.size() > 0)
		return true;
	else
		return false;
}

void Lighting::Generate(Vect3f pos_begin,Vect3f pos_end,Camera* camera)
{
	OneLight* p=new OneLight;
	p->Generate(pos_begin,pos_end,camera,this);
	lights.push_back(p);
}

Lighting::OneLight::OneLight()
{
	time=0;
}

Lighting::OneLight::~OneLight()
{
}

void Lighting::OneLight::Animate(float dt)
{
	time+=dt;
	if(time>1)
		time=1;
}

void Lighting::OneLight::Generate(Vect3f pos_begin_,Vect3f pos_end_,Camera* camera,Lighting* parent)
{
	pos_begin=pos_begin_;
	pos_end=pos_end_;

	int size=32;
	position.resize(size+2);

	vector<float> pos(size+2);
	pos[0]=pos[pos.size()-1]=0;

	float amplitude=parent->param.lighting_amplitude;
	int i;
	for(i=2;i<=size;i*=2)
	{
		GenerateInterpolate(pos,i,amplitude);
		amplitude*=0.5f;
	}

	Vect3f tangent=pos_end-pos_begin;
	tangent.normalize();
	Vect3f vCameraToObject=camera->GetPos()-(pos_end+pos_begin)*0.5f;
	vCameraToObject.normalize();
	Vect3f orientation;
	orientation.cross(vCameraToObject,tangent);

	for(i=0;i<position.size();i++)
	{
		float t=i/(float)(position.size()-1);
		Vect3f p;
		p.x=LinearInterpolate(pos_begin.x,pos_end.x,t);
		p.y=LinearInterpolate(pos_begin.y,pos_end.y,t);
		p.z=LinearInterpolate(pos_begin.z,pos_end.z,t);
		p+=pos[i]*orientation;
		position[i]=p;
	}

	BuildStrip(camera,parent);
}

void Lighting::OneLight::GenerateInterpolate(vector<float>& pos,int size,float amplitude)
{
	vector<float> p(size);
	int i;
	for(i=0;i<size;i++)
		p[i]=graphRnd.frnd(amplitude);

	for(i=1;i<pos.size()-1;i++)
	{
		float t=i/(float)pos.size();
		float dy=get(p,t);
		pos[i]+=dy;
	}

}

float Lighting::OneLight::get(vector<float>& p,float t)//линейна€ интерпол€ци€
{

	int size=p.size()+2;
	int i=int(t*size);
	float dt=t*size-i;
	i-=1;
	float p0=0,p1=0;
	if(i>=0 && i<p.size())
		p0=p[i];
	if(i+1>=0 && i+1<p.size())
		p1=p[i+1];

	return LinearInterpolate(p0,p1,dt);
//	return CosInterpolate(p0,p1,dt);
}
void Lighting::OneLight::Update(Vect3f pos_delta)
{
	for(int i=0;i<strip_list.size();i++)
	{
		OneStrip& p=strip_list[i];
		p.pos += pos_delta;
	}
}

void Lighting::OneLight::BuildStrip(Camera* camera,Lighting* parent)
{
	float tn=float(round(pos_end.distance(pos_begin)/parent->param.strip_length));
	strip_list.resize(position.size());
	for(int i=0;i<position.size();i++)
	{
		Vect3f& p=position[i];
		Vect3f vCameraToObject=camera->GetPos()-p;
		vCameraToObject.normalize();
		OneStrip& s=strip_list[i];
		Vect3f n;
		if(i==0)
			n=position[i+1]-position[i];
		else
		if(i==position.size()-1)
			n=position[i]-position[i-1];
		else
			n=position[i+1]-position[i-1];
		n.normalize();
		n=n%vCameraToObject;
			
		float t=tn*i/(float)(position.size()-1);
		s.pos=p;
		s.n=n;
		s.u=t;
	}
}
