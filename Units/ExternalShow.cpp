#include "StdAfx.h"
//#include "Region.h"
#include "RenderObjects.h"
#include "Universe.h"
#include "terra.h"
#include "CameraManager.h"
#include "ExternalShow.h"

#define ZFIX 2.0f

#define GAME_SHELL_SHOW_REGION_UP_DELTA		32.0f
#define GAME_SHELL_SHOW_REGION_DOWN_DELTA	-16.0f

#define GAME_SHELL_SHOW_REGION_V_STEP		0.1f
#define GAME_SHELL_SHOW_REGION_U_STEP		0.01f
#define GAME_SHELL_SHOW_REGION_U_SPEED		0.5f

const char* sRegionTextureCircle = "Scripts\\Resource\\Textures\\Region.tga";
const char* sRegionTextureCircleDotted = "Scripts\\Resource\\Textures\\dotted_line.tga";
float RegionUSpeedDotted=1;
const char* sRegionTextureTriangle = "Scripts\\Resource\\Textures\\targetTriangle.tga";

float maxAlphaCircleColorHeight = 500.0f;

int region_show_spline = 1;
float region_show_spline_space = 10;
float region_show_spline_animate_dlen = 0;

int region_show_spline_cp = 0;
int region_show_spline_cp_number = 0;
int region_show_including_level = 0;

int region_show_spline_y_min_max = 0;

int region_show_spline_tangents = 0;
int region_show_spline_normals = 0;
float region_show_tangent_len = 0.2f;

cCircleShow* gbCircleShow=NULL;

void terCircleShowNormal()
{
	gbCircleShow->Show(0);
}

void terCircleShowDotted()
{
	gbCircleShow->Show(1);
}

void terCircleShowTriangle()
{
	gbCircleShow->Show(2);
}

void terCircleShowCross()
{
	gbCircleShow->Show(3);
}

cCircleShow::cCircleShow()
{
	gbCircleShow=this;
	no_interpolation=false;
	types.resize(4);
	types[0].external_show=terScene->CreateExternalObj(terCircleShowNormal,sRegionTextureCircle);
	types[1].external_show=terScene->CreateExternalObj(terCircleShowDotted,sRegionTextureCircleDotted);
	types[2].external_show=terScene->CreateExternalObj(terCircleShowTriangle,sRegionTextureTriangle);
	types[3].external_show=terScene->CreateExternalObj(terCircleShowCross,sRegionTextureCircle);
	u_begin=0;
	energyTextureStart_ = 0;
	energyTextureEnd_ = GAME_SHELL_SHOW_REGION_U_STEP;
}

cCircleShow::~cCircleShow()
{
	vector<sCircleType>::iterator it;
	FOR_EACH(types,it)
	{
		(*it).external_show->Release();
	}
	gbCircleShow=NULL;
}

void cCircleShow::Lock()
{
	lock.lock();
}

void cCircleShow::Unlock()
{
	lock.unlock();
}

void terCircleShowGraph(const Vect3f& pos,float r, const CircleColor& circleColor)
{
	gbCircleShow->Circle(pos,r,circleColor);
}

void terCircleShow(const Vect3f& pos0,const Vect3f& pos1,float r, const struct CircleColor& circleColor)
{
	gbCircleShow->Circle(pos0,pos1,r,r,circleColor);
}

void terCircleShow(const Vect3f& pos0,const Vect3f& pos1,float r0, float r1, const struct CircleColor& circleColor)
{
	gbCircleShow->Circle(pos0,pos1,r0,r1,circleColor);
}

void cCircleShow::Circle(const Vect3f& pos0,const Vect3f& pos1,float r0,float r1, const CircleColor& attr)
{
	xassert(pos0.x>-1e4f && pos0.x<1e4f);
	xassert(pos0.y>-1e4f && pos0.y<1e4f);
	xassert(pos0.z>-1e4f && pos0.z<1e4f);
	xassert(r0>=0 && r0<1e4f);

	xassert(pos1.x>-1e4f && pos1.x<1e4f);
	xassert(pos1.y>-1e4f && pos1.y<1e4f);
	xassert(pos1.z>-1e4f && pos1.z<1e4f);
	xassert(r1>=0 && r1<1e4f);

	if(no_interpolation)
	{
		Circle(pos0,r0,attr);
		return;
	}

	xassert(lock.locked());
	sCircle c;
	c.pos[0]=pos0;
	c.pos[1]=pos1;
	c.r[0]=r0;
	c.r[1]=r1;
	c.attr=attr;
	types[attr.dotted].circles.push_back(c);
}

void cCircleShow::Circle(const Vect3f& pos,float r,const CircleColor& attr)
{
	xassert(pos.x>-1e4f && pos.x<1e4f);
	xassert(pos.y>-1e4f && pos.y<1e4f);
	xassert(pos.z>-1e4f && pos.z<1e4f);
	xassert(r>=0 && r<1e4f);

	MTG();
	if(!attr.color.a)
		return;

	sCircleGraph c;
	c.pos=pos;
	c.r=r;
	c.attr=attr;
	types[attr.dotted].circles_graph.push_back(c);
}


void cCircleShow::Show(int dotted)
{
	Lock();
	xassert(dotted>=0 && dotted<types.size());
	sCircleType& tp=types[dotted];

	SAMPLER_DATA data=sampler_wrap_anisotropic;
	data.addressv=DX_TADDRESS_CLAMP;
	gb_RenderDevice->SetSamplerDataVirtual(0,data);

	{
		float t = gb_VisGeneric->GetInterpolationFactor();
		float t_=1-t;

		vector<sCircle>::iterator it;
		FOR_EACH(tp.circles,it)
		{
			sCircle& c=*it;
			CircleShow(c.pos[0]*t_+c.pos[1]*t,c.r[0]*t_+c.r[1]*t,c.attr);
		}
	}

	{
		vector<sCircleGraph>::iterator it;
		FOR_EACH(tp.circles_graph,it)
		{
			sCircleGraph& c=*it;
			CircleShow(c.pos,c.r,c.attr);
		}
		tp.circles_graph.clear();
	}
	Unlock();
}

void cCircleShow::Clear()
{
	vector<sCircleType>::iterator it;
	FOR_EACH(types,it)
	{
		it->circles.clear();
	}
}


void cCircleShow::Quant(float dt)
{
	u_begin=fmod(u_begin + dt*RegionUSpeedDotted + 1.0f,1.0f);

	energyTextureStart_ = fmod(energyTextureStart_ + dt*GAME_SHELL_SHOW_REGION_U_SPEED + 1.0f,1.0f);
	energyTextureEnd_ = energyTextureStart_ + GAME_SHELL_SHOW_REGION_U_STEP;
}

void cCircleShow::CircleShow(const Vect3f& pos,float r, const CircleColor& circleColor)
{
	xassert(pos.x>-1e4f && pos.x<1e4f);
	xassert(pos.y>-1e4f && pos.y<1e4f);
	xassert(pos.z>-1e4f && pos.z<1e4f);
	xassert(r>=0 && r<1e4f);
	sColor4c diffuse(circleColor.color);
	float width = circleColor.width;
	if (circleColor.dotted != 3) {

		Vect2f tp,dn;

		int num_du = round((2.0f * r * M_PI) / circleColor.length);
		if(num_du<2)
			num_du=2;
		int num_da = round((2.0f * r * M_PI) / region_show_spline_space);
		// Кружок рисуется не меньше, чем восьмигранником
		if(num_da<8)
			num_da=8;


		//Кривовато, т.к. только для определённой сцены
		if(width<0)
		{
			float dist = To3D (pos).distance(cameraManager->GetCamera()->GetPos());
			width =  -dist * width;
			float alpha = dist * float(diffuse.a) / maxAlphaCircleColorHeight;
			diffuse.a = (alpha > 255) ? 255.0f : alpha;
		}

		float da = M_PI * 2.0f / (float)(num_da);
		float du=num_du/ (float)(num_da);
		float a=0, u=0;

	//	if(circleColor.dotted == 1 || circleColor.dotted == 2)
		if(circleColor.dotted == 1)
			u=u_begin;

		bool selection = (circleColor.dotted == 4);

		DrawStrip strip;
		strip.Begin();

		float v_pos = 0;
		sVertexXYZDT1 p1,p2;
		p1.diffuse = diffuse;
		p1.v1() = selection ? energyTextureEnd_ : 1.0f;
		p2.diffuse = diffuse;
		p2.v1() = selection ? energyTextureStart_ : 0.05f;

		for(int i = 0;i <= num_da;i++)
		{
			dn.x = cosf(a);
			dn.y = sinf(a);

			tp.x = pos.x + dn.x*r;
			tp.y = pos.y + dn.y*r;

			dn.x *= width;
			dn.y *= width;

			float z0 = ZFIX+(float)(vMap.GetAlt(vMap.XCYCL(round(tp.x)),vMap.YCYCL(round(tp.y))) >> VX_FRACTION);

			p1.pos.set(tp.x-dn.x,tp.y-dn.y,z0);
			p1.u1() = selection ? v_pos : u;
			v_pos += GAME_SHELL_SHOW_REGION_V_STEP;
		
			p2.pos.set(tp.x+dn.x,tp.y+dn.y,z0);
			p2.u1() = selection ? v_pos : p1.u1();
			v_pos += GAME_SHELL_SHOW_REGION_V_STEP;

			strip.Set(p1,p2);
			a += da;
			u+=du;
		}
		strip.End();

	} else {
		int num = round(2.0f * r / region_show_spline_space);
		if (num < 2) {
			return;
		}

		float v_pos = 0;

		sVertexXYZDT1 p1,p2;

		p1.diffuse = diffuse;
		p1.v1() = 1.0f;
		p2.diffuse = diffuse;
		p2.v1() = 0.05f;

		DrawStrip vertStrip;
		vertStrip.Begin();

		float x1 = pos.x - width / 2.0f;
		float x2 = x1 + width;
		float y = pos.y - r;
		int i;
		for (i = 0; i <= num; i++) {
			float z0 = ZFIX+(float)(vMap.GetAlt(vMap.XCYCL(round(pos.x)),vMap.YCYCL(round(y))) >> VX_FRACTION);
			p1.pos.set(x1, y, z0);
			p1.u1() = v_pos;
			v_pos += GAME_SHELL_SHOW_REGION_V_STEP;
		
			p2.pos.set(x2, y, z0);
			p2.u1() = v_pos;
			v_pos += GAME_SHELL_SHOW_REGION_V_STEP;

			vertStrip.Set(p1,p2);
			y += region_show_spline_space;
		}

		vertStrip.End();

		DrawStrip horizStrip;
		horizStrip.Begin();

		float y1 = pos.y - width / 2.0f;
		float y2 = y1 + width;
		float x = pos.x - r;
		for (i = 0; i <= num; i++) {
			float z0 = ZFIX+(float)(vMap.GetAlt(vMap.XCYCL(round(x)),vMap.YCYCL(round(pos.y))) >> VX_FRACTION);
			p1.pos.set(x, y1, z0);
			p1.u1() = v_pos;
			v_pos += GAME_SHELL_SHOW_REGION_V_STEP;
		
			p2.pos.set(x, y2, z0);
			p2.u1() = v_pos;
			v_pos += GAME_SHELL_SHOW_REGION_V_STEP;

			horizStrip.Set(p1,p2);
			x += region_show_spline_space;
		}

		horizStrip.End();
	}
}



inline void AddTriangle(cVertexBuffer<sVertexXYZD>& buf,sVertexXYZD*& v,sVertexXYZD p[3],int& primitive)
{
	v[0]=p[0];
	v[1]=p[1];
	v[2]=p[2];
	v+=3;
	primitive++;

	if((primitive+3)*3>=buf.GetSize())
	{
		buf.Unlock(primitive*3);
		buf.DrawPrimitive(PT_TRIANGLELIST,primitive);
		primitive=0;
		v=buf.Lock();
	}
}
