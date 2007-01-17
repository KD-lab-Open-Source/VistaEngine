#include "stdafx.h"
#include <math.h>

#include "Fallout.h"
#include "RangedWrapper.h"
#include "ResourceSelector.h"
#include "Serialization.h"
#include "..\..\Environment\Environment.h"
#include "..\Physics\WindMap.h"
#include "..\Terra\vmap.h"

BEGIN_ENUM_DESCRIPTOR(ModeFall,"ModeFall");
REGISTER_ENUM(FALLOUT_RAIN, "Дождь");
REGISTER_ENUM(FALLOUT_SNOW, "Снег");
END_ENUM_DESCRIPTOR(ModeFall);


static float max_size_rain = 2; 
static float max_size_snow = 10;

void cFallout::cDrop::Init(float mr,int anim_size)
{
	pos0.set(0,0,0);
	vel.set(0,0,0);
	float dm = (mr+mr*0.2f)/ndub;
	for(int i=ndub-1;i>=0;i--)
	{
		dub[i].pos.set(graphRnd.frand()-0.5f, graphRnd.frand()-0.5f, graphRnd.frand()-0.5f);
		dub[i].pos.Normalize(mr);
		dub[i].animate_key = graphRnd(anim_size);
		mr -= dm;
	}
	animate_key = graphRnd(anim_size);
	visible=false;
	rand = graphRnd(50)-25;
}

cFallout::cFallout():cBaseGraphObject(0)
{
	enable_ = true;
	current_intensity=0;
	mode=FALLOUT_NONE;
	ending = false;
	N = max_N = 0;
//	md = FALLOUT_RAIN;;
//	intensity = 1;
	center.set(0,0,0);
	rain_color.set(180, 180, 190, 57);
	rain_sw = snow_sw = Vect2f(0,100);
	snow_texture = "Scripts\\resource\\Textures\\snowflake.avi";
	rain_texture = "Scripts\\resource\\Textures\\rain.tga";
	Texture=NULL;
	rainTexture = NULL;
	camera_pos.set(0,0,0);
	for(int i=0;i<8;i++)
		near_point8[i].set(10,10,10);
	minimal_water_height=2;
}
void cFallout::Enable(bool enable)
{
	if(!enable)
		Set(FALLOUT_CURRENT,0,0);
	enable_ = enable;
}

cFallout::~cFallout()
{
	RELEASE(Texture);
	RELEASE(rainTexture);
}


float cFallout::GetIntensity()
{
	MTAuto mlock(lock);
	int nd;
	switch(mode)
	{
	case FALLOUT_RAIN:	nd = drops.size();break;
	case FALLOUT_SNOW:	nd = drops.size();	break;
	default: return 0;
	}
	return float(N)/nd;
}
bool cFallout::SetIntensity(float intensity, float time)
{
	MTAuto mlock(lock);
	if (ending) return false;
	intensity=clamp(intensity,0.0f,1.0f);
	current_intensity=intensity;
	switch(mode)
	{
	case FALLOUT_RAIN:	
		break;
	case FALLOUT_SNOW:	
		break;
	default: return false;
	}
	max_N = round(intensity*drops.size());

	if (time < 1e-5)
		dN = max_N-N;
	else dN = round((max_N-N)/time);
	
	return true;
}

void cFallout::Init(float quality, cWater* pWater, cTemperature* pTemperature)
{
	MTAuto mlock(lock);
	mode = FALLOUT_NONE;
	xassert(quality<=1 && quality>=0);
	if (quality>1) quality = 1;
	if (quality<0) quality = 0;
	vel_all.set(0,0,1);
	r=1000;
	int n = 75000;
//	all_N = n;
	if (round(n*quality)<100)
		quality = 100.0f/n;
	max_N = round(n*quality);
	drops.resize(max_N);
	N = 0;
	dN =n/10;
	LoadSnowTexture();
	int count =0; //temp
	int anim_size = 1;
	if (Texture)
		anim_size = Texture->GetFramesCount();
	float fdup = float(n - max_N)/max_N;
	int ndup_min = fdup;
	int ndup_max = ndup_min+1;
	float Pndupd_max = fdup - ndup_min;
	vector<cDrop>::iterator dr;
	FOR_EACH(drops, dr)
	{
		int ndup = (Pndupd_max>graphRnd.frand())? ndup_max : ndup_min;
		dr->Init(100, anim_size);
		count+=1+ndup;
	}

	SetSize(0.3f,FALLOUT_RAIN);
	SetSize(0.3f,FALLOUT_SNOW);
	mode = FALLOUT_NONE;
	ending = false;
	snow_kc = 0.9f;
	rain_kc = 1.2f;
	max_N=0;

	circles.Init(pWater, this, pTemperature);
}

void cFallout::PreDraw(cCamera *pCamera)
{
	pCamera->Attach(SCENENODE_OBJECTSORT,this);
}

void cFallout::SetSidewind(Vect2f v)
{
	MTAuto mlock(lock);
	float mx=1000.0f;
	xassert(v.x>=-mx && v.x<mx);
	xassert(v.y>=-mx && v.y<mx);
	vel_all.x = v.x;
	vel_all.y = v.y;
}

float cFallout::GetSize(ModeFall mode)
{
	MTAuto mlock(lock);
	if(mode == FALLOUT_CURRENT)
		mode = this->mode;
	switch(mode)
	{
	case FALLOUT_RAIN:	return size_rain/max_size_rain; 
	case FALLOUT_SNOW:	return size_snow/max_size_snow; 
	}
	return 0;
}

void cFallout::SetSize(float sc, ModeFall mode)
{
	MTAuto mlock(lock);
	if(mode == FALLOUT_CURRENT)
		mode = this->mode;
	if (sc<0) sc = 0;
	if (sc>1) sc = 1;
	switch(mode)
	{
	case FALLOUT_RAIN:	
		{
			static float k = -3000;
			size_rain = sc*max_size_rain; 
			vel_all.z = k*0.3f;
			break;
		}
	case FALLOUT_SNOW:	
		{
			static float k = -300;
			size_snow = sc*max_size_snow; 
			vel_all.z = k*sc;
			break;
		}
	}
}

void cFallout::Set(ModeFall m,float intensity, float time)
{
	MTAuto mlock(lock);
	if(!enable_)
		return;
//	m=FALLOUT_SNOW;
	if(m == FALLOUT_CURRENT)
		m = this->mode;
	float kc = 0;
	switch(m){
	case FALLOUT_RAIN:	
		mode = FALLOUT_RAIN; 
		SetIntensity(intensity, time);
		kc = rain_kc;
		break;
	case FALLOUT_SNOW:	
		mode = FALLOUT_SNOW; 
		SetIntensity(intensity, time);
		kc = snow_kc;
		break;
	}
	if(intensity < FLT_EPS)
		ending = true;
	else
		ending = false;
	if(N==0&&!ending)
	{
		circles.clear();
		vector<cDrop>::iterator dr;
		FOR_EACH(drops, dr){
			dr->pos0.x = graphRnd.frand()-0.5f;
			dr->pos0.y = graphRnd.frand()-0.5f;
			dr->pos0.z = graphRnd.frand()*0.5f;
			dr->pos0.Normalize(r);
			dr->vel = vel_all;
		}
		Vect3f near_point = (near_point8[0]+near_point8[1]+near_point8[2]+near_point8[3])/4;
		Vect3f focus = near_point - camera_pos;
		Vect3f cur_center = focus;
		cur_center.normalize(r*kc);
		cur_center+=near_point;
		center = cur_center;
	}
}
void cFallout::End(float time)
{
	MTAuto mlock(lock);
	SetIntensity(0, time);
	ending = true;
}
void cFallout::DrawSnow(cCamera *pCamera)
{
	MTAuto mlock(lock);
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	Vect3f cp = pCamera->GetPos();
	rd->SetNoMaterial(ALPHA_BLEND, MatXf::ID, 0, Texture);
	cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();
	vector<cDrop>::iterator dr = drops.begin();
	vector<cDrop>::iterator lim = drops.begin()+N;
	Vect3f near_point = (near_point8[0]+near_point8[1]+near_point8[2]+near_point8[3])/4;
	Vect3f focus = near_point - cp;
	Vect3f cur_center = focus;
	cur_center.normalize(r/*snow_kc*/);
	cur_center+=near_point;
	Vect3f dcenter = cur_center - center;
	static float dd = 4;
	if (dcenter.norm()>r/dd)
	{
		Mat3f mt(M_PI/2,Z_AXIS);
		for(; dr<lim; ++dr)
			dr->pos0 = (dr->pos0 - center)*mt + cur_center; //can opt
		dr = drops.begin();
	}
	center = cur_center;
	static float kd = 0.6f;
	float max_dist =  focus.norm2();
	float min_dist = cp.distance((near_point8[0]+near_point8[1]+near_point8[2]+near_point8[3])/4);
	float kd_dist = max_dist*kd;
	FastNormalize(focus);
	float max_an = (near_point8[7] - cp).Normalize().dot(focus);
	float r2 = sqr(r);
	float r2_1 = sqr(r)-1;
	Vect3f sx = (near_point8[1] - near_point8[0]).Normalize(size_snow/2);
	Vect3f sy = (near_point8[2] - near_point8[0]).Normalize(size_snow/2);//*ksize);
	static float rand_time = 0;
	rand_time+=dt;
	if (rand_time>1)
	{
		vector<cDrop>::iterator dr = drops.begin();
		for(; dr<lim; ++dr)
		{
			dr->rand += graphRnd(10)-5;
			if (dr->rand>25.0) dr->rand = 25;
			else if(dr->rand<-25.0)dr->rand = -25;
		}
		rand_time = 0;
	}

	sColor4c color(255, 255, 255, 255);
	pBuf->BeginDraw();
	for(; dr<lim; ++dr)
	{
		float dc = dr->pos0.distance2(center);
		if (dc>r2)
		{
			dr->pos0.x = (graphRnd.frand()-0.5f);
			dr->pos0.y = (graphRnd.frand()-0.5f);
			dr->pos0.z = (graphRnd.frand()-0.5f);
			dr->pos0.Normalize(r);
			dr->pos0+=center;
			dr->vel = vel_all;
		}
		{
			Vect3f& vel = dr->vel;

			{
				vel.x = vel_all.x + dr->rand;
				vel.y = vel_all.y - dr->rand;
				vel.z = vel_all.z + dr->rand;
			}
			dr->pos0 += dt*vel;
		}
		if (dr->pos0.z<0 || dc>r2_1)
			continue;
		Vect3f ctopos = cp - dr->pos0;
		FastNormalize(ctopos);
		if (-ctopos.dot(focus)<max_an)
			continue;
		Vect3f pos = dr->pos0;

		sRectangle4f rt(0,0,1,1);
		if (Texture)
			rt = Texture->GetFramePos(dr->animate_key);

		sVertexXYZDT1 *v=pBuf->Get();
		v[0].pos=pos-sx-sy; v[0].diffuse=color; v[0].GetTexel().set(rt.min.x,rt.min.y);
		v[1].pos=pos-sx+sy; v[1].diffuse=color; v[1].GetTexel().set(rt.min.x,rt.max.y);
		v[2].pos=pos+sx-sy; v[2].diffuse=color; v[2].GetTexel().set(rt.max.x,rt.min.y);	
		v[3].pos=pos+sx+sy; v[3].diffuse=color; v[3].GetTexel().set(rt.max.x,rt.max.y);

		for(int idub=0;idub<cDrop::ndub;idub++)
		{
			pos = dr->pos0+dr->dub[idub].pos;
			v=pBuf->Get();
			v[0].pos=pos-sx-sy; v[0].diffuse=color; v[0].GetTexel().set(rt.min.x,rt.min.y);
			v[1].pos=pos-sx+sy; v[1].diffuse=color; v[1].GetTexel().set(rt.min.x,rt.max.y);
			v[2].pos=pos+sx-sy; v[2].diffuse=color; v[2].GetTexel().set(rt.max.x,rt.min.y);
			v[3].pos=pos+sx+sy; v[3].diffuse=color; v[3].GetTexel().set(rt.max.x,rt.max.y);
		}

	}
	pBuf->EndDraw();
}
void cFallout::Draw(cCamera *pCamera)
{
	MTAuto mlock(lock);
	start_timer_auto();
	pCamera->GetFrustumPoint(near_point8[0],near_point8[1],near_point8[2],near_point8[3],near_point8[4],
							near_point8[5],near_point8[6],near_point8[7]);
	camera_pos = pCamera->GetPos();
	switch(mode)
	{
	case FALLOUT_NONE:		break;
	case FALLOUT_RAIN:		
		DrawRain(pCamera); 
		if (circles.Visible())
			circles.Draw(pCamera,dt);
		break;
	case FALLOUT_SNOW:		
		DrawSnow(pCamera); 
		break;
	}
	dt=0;
}
void cFallout::GetSphere(Vect3f& pos, float& radius)
{
	MTAuto mlock(lock);
	pos = center;
	radius = r;
}

void cFallout::DrawRain(cCamera *pCamera)
{ 
	MTAuto mlock(lock);
	if(N==0) return;
	xassert(N<=drops.size());
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	Vect3f cp = pCamera->GetPos();
	rd->SetNoMaterial(ALPHA_BLEND, MatXf::ID, 0,rainTexture);
	cQuadBuffer<sVertexXYZDT1>* pBuf = rd->GetQuadBufferXYZDT1();
	Vect3f near_point = (near_point8[0]+near_point8[1]+near_point8[2]+near_point8[3])/4;
	Vect3f focus = near_point - cp;
	Vect3f cur_center = focus;
	cur_center.normalize(r);
	cur_center=near_point;
	//cur_center = cp;
	Vect3f dcenter = cur_center - center;
	if (dcenter.norm()>r)
	{
		Mat3f mt(M_PI/2,Z_AXIS);
		vector<cDrop>::iterator dr = drops.begin(),drend=drops.begin()+N;
		for(; dr<drend; ++dr)
			dr->pos0 = (dr->pos0 - center)*mt + cur_center;
	}
	center = cur_center;
	//float min_vis = (center-cp).norm2() - sqr(r);
	//float max_vis = min_vis + sqr(r/4);
	FastNormalize(focus);
	float max_an = (near_point8[7] - cp).Normalize().dot(focus);

	int nVertex = 0;
	float r2 = sqr(r);
	float r2_1 = sqr(r)-1;
	int visible_num=0;
	pBuf->BeginDraw();
	for(vector<cDrop>::iterator dr = drops.begin(),drend=drops.begin()+N; dr!=drend; ++dr)
	{
		float dc = dr->pos0.distance2(center);
		if(dc>r2)
		{
			dr->pos0.x = (graphRnd.frand()-0.5f);
			dr->pos0.y = (graphRnd.frand()-0.5f);
			dr->pos0.z = (graphRnd.frand()*0.5f);
			dr->pos0.Normalize(r);
			dr->pos0+=center;
		}

		{
			Vect3f& vel = dr->vel;

			vel = vel_all;
			Vect2f wind= windMap->getBilinear(dr->pos0);
			vel.x += wind.x*10;
			vel.y += wind.y*10;

			dr->pos0 += dt*vel;
		}

		if (dr->pos0.z<0 ||(environment->water() && environment->water()->isUnderWater(dr->pos0)))
		{
			if(dr->visible)
			{
				dr->visible = false;
				if(environment->water() && environment->water()->GetDeepWaterFast(dr->pos0.x,dr->pos0.y)>minimal_water_height)
					circles.SetCircle(dr->pos0);
			}
			continue;
		}
		if (dc>r2_1)
			continue;
		Vect3f ctopos = cp - dr->pos0;
		//float dtoc =  ctopos.norm2();
		FastNormalize(ctopos);
		float an = -ctopos.dot(focus);
		if (an<max_an)
		{
			dr->visible = false;
			continue;
		}
		dr->visible = true;
		Vect3f pos = dr->pos0;
		Vect3f sy;
		static float scz = -0.05f;
		static float sc = -0.03f;
		sy.x = sc*dr->vel.x;
		sy.y = sc*dr->vel.y;
		sy.z = scz*dr->vel.z;
		Vect3f sx;
		sx.cross(sy,ctopos);
		FastNormalize(sx);
		sx.x*=size_rain;
		sx.y*=size_rain;
		sx.z*=size_rain;

		visible_num++;
		sVertexXYZDT1* v = pBuf->Get();
		v[0].pos.x=pos.x-sx.x-sy.x; v[0].pos.y=pos.y-sx.y-sy.y; v[0].pos.z=pos.z-sx.z-sy.z; v[0].diffuse=rain_color; v[0].GetTexel().set(0,1);
		v[1].pos.x=pos.x-sx.x+sy.x; v[1].pos.y=pos.y-sx.y+sy.y; v[1].pos.z=pos.z-sx.z+sy.z; v[1].diffuse=rain_color; v[1].GetTexel().set(0,0);
		v[2].pos.x=pos.x+sx.x-sy.x; v[2].pos.y=pos.y+sx.y-sy.y; v[2].pos.z=pos.z+sx.z-sy.z; v[2].diffuse=rain_color; v[2].GetTexel().set(1,1);
		v[3].pos.x=pos.x+sx.x+sy.x; v[3].pos.y=pos.y+sx.y+sy.y; v[3].pos.z=pos.z+sx.z+sy.z; v[3].diffuse=rain_color; v[3].GetTexel().set(1,0);
		for(int idub=0;idub<cDrop::ndub;idub++)
		{
			pos = dr->pos0+dr->dub[idub].pos;
			v=pBuf->Get();
			v[0].pos.x=pos.x-sx.x-sy.x; v[0].pos.y=pos.y-sx.y-sy.y; v[0].pos.z=pos.z-sx.z-sy.z; v[0].diffuse=rain_color; v[0].GetTexel().set(0,1);
			v[1].pos.x=pos.x-sx.x+sy.x; v[1].pos.y=pos.y-sx.y+sy.y; v[1].pos.z=pos.z-sx.z+sy.z; v[1].diffuse=rain_color; v[1].GetTexel().set(0,0);
			v[2].pos.x=pos.x+sx.x-sy.x; v[2].pos.y=pos.y+sx.y-sy.y; v[2].pos.z=pos.z+sx.z-sy.z; v[2].diffuse=rain_color; v[2].GetTexel().set(1,1);
			v[3].pos.x=pos.x+sx.x+sy.x; v[3].pos.y=pos.y+sx.y+sy.y; v[3].pos.z=pos.z+sx.z+sy.z; v[3].diffuse=rain_color; v[3].GetTexel().set(1,0);
		}
	}
	pBuf->EndDraw();
	//dprintf("visible_num=%i\n",visible_num);
}

void cFallout::Animate(float dtime)
{
	MTAuto mlock(lock);
	dt = dtime/1000;
	if (N!=max_N)
	{
		if(dN==0)
			dN=max_N/5;

		N+=dN*dt;
		if (dN>0)
		{
			if (N>max_N)
				N = max_N;
		}
		if(dN<0)
		{
			if(N<max_N)
				N = max_N;
		}
		xassert((UINT)N<=drops.size());
	}else if (ending)
	{
		mode = FALLOUT_NONE;
		ending = false;
	}
}

void cFallout::LoadSnowTexture()
{
	MTAuto mlock(lock);
	RELEASE(Texture);
	RELEASE(rainTexture);
	Texture=(cTextureAviScale *)GetTexLibrary()->GetElement3DAviScale(snow_texture.c_str());	
	rainTexture = GetTexLibrary()->GetElement3D(rain_texture.c_str());

//	xassert(Texture&& "snow texture not found");
}

FallaoutSnowAttributes::FallaoutSnowAttributes()
{
	enable_ = false;
	size_	= 0.3f;
	textureName_ = "Scripts\\resource\\Textures\\snowflake.avi";
}
void FallaoutSnowAttributes::serialize(Archive& ar)
{
	ar.serialize(enable_, "enable", "Вкл.");
	if (ar.isEdit())
	{
		float sz = size_*100.f;
		ar.serialize(RangedWrapperf(sz, 0, 100, 0.01f), "size", "Размер");
		size_ = sz/100.f;
	}else
	{
		ar.serialize(RangedWrapperf(size_, 0, 1, 0.01f), "size", "Размер");
	}
	ar.serialize(ResourceSelector (textureName_, ResourceSelector::TEXTURE_OPTIONS),
		"textureName", "Текстура");
}

FalloutRainRipplesAttributes::FalloutRainRipplesAttributes()
{
	size_ = 5.f;
	time_ = 0.5f;
	minWaterHeight_ = 2;
	textureName_ = "RESOURCE\\TERRAINDATA\\TEXTURES\\G_TEX_WATERRINGS_001.AVI";
}
void FalloutRainRipplesAttributes::serialize(Archive& ar)
{
	ar.serialize(size_, "size", "Размер");
	ar.serialize(time_, "time", "Время цикла");
	ar.serialize(minWaterHeight_, "minWaterHeight", "Минимальная глубина");
	ar.serialize(ResourceSelector (textureName_, ResourceSelector::AVI_OPTIONS),
		"textureName", "Текстура");
}
FalloutRainAttributes::FalloutRainAttributes()
{
	enable_ = false;
	size_	= 0.3f;
	textureName_ = ".\\Resource\\TerrainData\\Textures\\Rain.tga";
	color_ = sColor4c(153,171,217,57);
}
void FalloutRainAttributes::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(color_, "color", "Цвет дождя");
	ar.serialize(ripples_,"ripples","Круги на воде");
}

FalloutAttributes::FalloutAttributes()
{
	intensity_ = 0;
}

void FalloutAttributes::serialize(Archive& ar)
{
	ar.serialize(rain_,"rain","Дождь");
	ar.serialize(snow_,"snow","Снег");
	ar.serialize(intensity_,"intensity","Интенсивность осадков");
}

void cFallout::SetupAttributes(FalloutAttributes& attributes)
{
	MTAuto mlock(lock);

	current_intensity = attributes.intensity_;
	snow_texture = attributes.snow_.textureName_;
	rain_texture = attributes.rain_.textureName_;
	rain_color = attributes.rain_.color_;

	if (circles.Visible())
	{
		minimal_water_height = attributes.rain_.ripples_.minWaterHeight_;
		circles.SetTexture(attributes.rain_.ripples_.textureName_.c_str());
		circles.SetSize(attributes.rain_.ripples_.size_);
		circles.SetTime(attributes.rain_.ripples_.time_);
	}
	SetSize(attributes.snow_.size_, FALLOUT_SNOW);
	SetSize(attributes.rain_.size_, FALLOUT_RAIN);
	if(attributes.rain_.enable_)
	{
		if (GetMode()!=FALLOUT_RAIN)
			Set(FALLOUT_RAIN,current_intensity,2);
		else SetIntensity(current_intensity,2);
	}else
	if (attributes.snow_.enable_)
	{
		if (GetMode()!=FALLOUT_SNOW)
			Set(FALLOUT_SNOW,current_intensity,2);
		else SetIntensity(current_intensity,2);
	}
	LoadSnowTexture();


}
/*
void cFallout::serializeParameters (Archive& ar)
{
	ar.openBlock("Rain", "Дождь");
	float rain_sz = GetSize(FALLOUT_RAIN);
	bool rain_on = GetMode()==FALLOUT_RAIN;
	ar.serialize(ResourceSelector (rain_texture, ResourceSelector::TEXTURE_OPTIONS),
		"rain_texture", "Текстура Дождя");
	if (ar.isEdit())
	{
		float sz = rain_sz*100.f;
		ar.serialize(RangedWrapperf(sz, 0, 100, 0.01f), "Rain_Size", "Размер");
		rain_sz = sz/100.f;
	}else
	{
		ar.serialize(RangedWrapperf(rain_sz, 0, 1, 0.01f), "Rain_Size", "Размер");
	}

	
	//ar.serialize(rain_sw, "rain_sw", "Боковой ветер");
	ar.serialize(rain_on, "rain_on", "Вкл.");
	if (circles.Visible())
	{
		string texture = circles.GetTextureName();
		float time = circles.GetTime();
		float size = circles.GetSize();
		ar.openBlock("ripples on the water", "Круги на воде");
			ar.serialize(ResourceSelector (texture, ResourceSelector::AVI_OPTIONS),
								"water_cirle_texture", "Текстура");
			ar.serialize(size, "size_water_cirle", "Размер");
			ar.serialize(time, "time_water_cirle", "Время цикла");
			ar.serialize(minimal_water_height, "minimal_water_height", "Минимальная глубина");
		ar.closeBlock();
		if (ar.isInput())
		{
			circles.SetTexture(texture.c_str());
			circles.SetSize(size);
			circles.SetTime(time);
		}
	}

	//if(!ar.isEdit())
	//{
	ar.serialize(current_intensity, "current_intensity", "Интенсивность");
	//}
	ar.closeBlock();
	ar.openBlock("Snow", "Снег");
	float snow_sz = GetSize(FALLOUT_SNOW);
	bool snow_on = GetMode()==FALLOUT_SNOW;
	ar.serialize(ResourceSelector (snow_texture, ResourceSelector::AVI_OPTIONS),
						"snow_texture", "Текстура снега");
	if (ar.isEdit())
	{
		float s = snow_sz*100.f;
		ar.serialize(RangedWrapperf(s, 0, 100, 0.01f), "Snow_Size", "Размер");
		snow_sz = s/100.f;
	}else
	{
		ar.serialize(RangedWrapperf(snow_sz, 0, 1, 0.01f), "Snow_Size", "Размер");
	}
	//ar.serialize(snow_sw, "snow_sw", "Боковой ветер");
	ar.serialize(snow_on, "snow_on", "Вкл.");
	ar.closeBlock();
	if (ar.isInput())
	{
		SetSize(snow_sz, FALLOUT_SNOW);
		SetSize(rain_sz, FALLOUT_RAIN);
		if (rain_on) snow_on = false;
		if(rain_on)
		{
			if (GetMode()!=FALLOUT_RAIN)
				Set(FALLOUT_RAIN,current_intensity,2);
			else SetIntensity(current_intensity,2);
			SetSize(rain_sz, FALLOUT_RAIN);
			SetSidewind(rain_sw);
		}else if (GetMode()==FALLOUT_RAIN)
			Set(FALLOUT_RAIN,0,5);
		if (snow_on)
		{
			if (GetMode()!=FALLOUT_SNOW)
				Set(FALLOUT_SNOW,current_intensity,2);
			else SetIntensity(current_intensity,2);
			SetSize(snow_sz, FALLOUT_SNOW);
			SetSidewind(snow_sw);
		}else if (GetMode()==FALLOUT_SNOW)
			Set(FALLOUT_SNOW,0,5);
		LoadSnowTexture();
	}
}

void cFallout::serializeColors (Archive& ar)
{
	ar.serialize(rain_color, "rain_color", "Цвет дождя");
}

void cFallout::serialize(Archive& ar)
{
	MTAuto mlock(lock);
	serializeColors (ar);
	serializeParameters (ar);
}
*/