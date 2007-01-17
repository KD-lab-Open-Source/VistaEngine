#include "stdafx.h"
#include "circles.h"
#include "FallOut.h"
static RandomGenerator rnd;

void DrawRect(Vect3f p, sColor4c c)
{
	static float d = 1000;
	gb_RenderDevice->DrawLine(Vect3f(p.x-d,p.y-d,p.z), Vect3f(p.x+d,p.y-d,p.z), c);
	gb_RenderDevice->DrawLine(Vect3f(p.x+d,p.y-d,p.z), Vect3f(p.x+d,p.y+d,p.z), c);
	gb_RenderDevice->DrawLine(Vect3f(p.x+d,p.y+d,p.z), Vect3f(p.x-d,p.y+d,p.z), c);
	gb_RenderDevice->DrawLine(Vect3f(p.x-d,p.y+d,p.z), Vect3f(p.x-d,p.y-d,p.z), c);
}

cCircles::cWaterCircle::cWaterCircle()
{
	pos.set(0,0,0);
	phasa = 10;
	time = 1;
	visible = false;
}
cCircles::cCircles()//:cBaseGraphObject(0)
{
	pause = false;
	Texture = NULL;
	pWater = NULL;
	fallout = NULL;
	texture_name = "Scripts\\Resource\\Textures\\Circle.avi";
}
cCircles::~cCircles()
{
	RELEASE(Texture);
}

void cCircles::Init(cWater* pWater, cFallout* fallout, cTemperature* pTemperature_)
{
 	if(!pWater)
		return;
	xassert(fallout);
	this->pWater = pWater;
	this->fallout = fallout;
	pTemperature = pTemperature_;
	size.set(pWater->GetGridSizeX()-1, pWater->GetGridSizeY()-1);
	size.x<<=pWater->GetCoordShift();
	size.y<<=pWater->GetCoordShift();

	SetTexture(texture_name.c_str());
	max_size = 10;
	time = 0.5f;
	color.set(255,255,255);
}
void cCircles::clear()
{
	drops.clear();
}

void cCircles::SetCircle(const Vect3f& dr_pos0)
{
	if (!drops.empty()&&drops.back().phasa>1) //opt
		drops.pop_back();
	cWaterCircle t;
	SetCirclePos(t,(Vect2f)dr_pos0);
	drops.push_front(t);
	return;
/*	int r = round(sqr(dr->pos0.x-center.x) + sqr(dr->pos0.y-center.y));
	if (r>r2_rain - r2_rain/3)
		if (rnd(5))
			return;
	drops.push_back(dr);
*/
}

const float cir_delta_time = 0.3f;
void cCircles::SetCirclePos(cWaterCircle& circle, Vect2f& p)
{
	
	circle.time = 1.0f/(time + rnd.frand()*cir_delta_time);
	circle.phasa = /*rnd.frand()*0.9f +*/ 0.1f;
	circle.visible = false;
	if (pWater->IsShowEnvironmentWater()&&(p.x<0||p.y<0||p.x>size.x||p.y>size.y))
	{
		circle.pos.set(p.x,p.y,z);
		circle.visible = true;
	}else if (p.x>max_size&&p.x<size.x-max_size&&p.y>max_size&&p.y<size.y-max_size)
	{
		int size = round(max_size)>>pWater->GetCoordShift();
		int xs = round(p.x)>>pWater->GetCoordShift();
		int ys = round(p.y)>>pWater->GetCoordShift();
		if (pWater->Get(xs-size,ys).z>0 && pWater->Get(xs+size,ys).z>0
			&& pWater->Get(xs,ys-size).z>0&& pWater->Get(xs,ys+size).z>0)
		{
			if (pTemperature) 
			{
				if (pTemperature->checkTile(round(p.x)>>pTemperature->gridShift(),round(p.y)>>pTemperature->gridShift()))
					return;
			}
			circle.pos.set(p.x,p.y,pWater->GetZ(round(p.x),round(p.y)));
			circle.visible = true;
		}
	}
}

inline void cCircles::RndPosInTrapeze(Vect2f& p)
{
	float hd = trapeze_h*pow(rnd.frand(), sqrtf(1-trapeze_dr));
	float r = (rnd.frand()*2.0f - 1.0f)* (trapeze_dr*hd + trapeze_beg_r);
	p = trapeze_beg + trapeze_dir_h*hd + trapeze_dir_n*r;
}
void cCircles::SetCirclePos(cWaterCircle& circle)
{
	circle.time = 1.0f/(time+rnd.frand()*cir_delta_time);
	circle.phasa = 0.1f;
	circle.visible = false;
	Vect2f p;
	RndPosInTrapeze(p);
/*	if (!drops.empty())
	{
		p = drops.back()->pos0;
		drops.pop_back();
	}
	else if(r2_rain<0)
		RndPosInTrapeze(p);
	else if (env_water_circle)
	{
		int count =0;
		do
			RndPosInTrapeze(p);
		while(p.distance2(rain_center)<r2_rain && count++<5);
	}
*/
	if (p.distance2(center)<r2_rain)
		return;
	if (pWater->IsShowEnvironmentWater()&&(p.x<0||p.y<0||p.x>size.x||p.y>size.y))
	{
		circle.pos.set(p.x,p.y,pWater->GetEnvironmentWater());
		circle.visible = true;
	}else if (p.x>max_size&&p.x<size.x-max_size&&p.y>max_size&&p.y<size.y-max_size)
	{
		int size = round(max_size)>>pWater->GetCoordShift();
		int xs = round(p.x)>>pWater->GetCoordShift();
		int ys = round(p.y)>>pWater->GetCoordShift();
		if (pWater->Get(xs-size,ys).z>0 && pWater->Get(xs+size,ys).z>0
			&& pWater->Get(xs,ys-size).z>0&& pWater->Get(xs,ys+size).z>0)
		{
			circle.pos.set(p.x,p.y,pWater->GetZ(round(p.x),round(p.y)));
			circle.visible = true;
		}
	}
}
Vect2f cCircles::GetPtOnZ(Vect3f& p1,Vect3f& p2, sPlane4f& back)
{
	float z = pWater->GetEnvironmentWater();
	Vect3f v = p1-p2;
	sPlane4f zero(Vect3f(1,0,z), Vect3f(0,1,z), Vect3f(1,1,z));
	float t = zero.GetCross(p2,p1);
	if (t==0) // прямая v || плоскости
	{
		if (round(p1.z)<=round(z)) 
			return Vect2f(p1);
	}
	if (t>1||t<=0)		// точка пересечения не лежит в зоне видимости
		t = back.GetCross(p2, p1);
	Vect2f cross(p2.x+t*v.x, p2.y+t*v.y);
	return cross;
}
void cCircles::PreDraw(cCamera *pCamera)
{
//	if(pWater)
//		pCamera->Attach(SCENENODE_OBJECTSORT,this);
}

void cCircles::Draw(cCamera *pCamera, float dt)
{
	float intensity = 0.5f*(fallout->GetN())/(2*M_PI*sqr(fallout->GetR()));
	z = pWater->GetEnvironmentWater();
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	rd->SetNoMaterial(ALPHA_BLEND, MatXf::ID, 0, Texture);
	cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();
	pBuf->BeginDraw();
	{
		list<cWaterCircle>::iterator it;
		for(it = drops.begin(); it != drops.end();){
			it->phasa+=(dt*it->time);
			if (it->phasa<=1){
				if(pCamera->GetScene()->GetFogOfWar())
					if(!pCamera->GetScene()->GetFogOfWar()->GetSelectedMap()->isVisible(Vect2f(it->pos.x,it->pos.y))){
						++it;
						continue;
					}
				sVertexXYZDT1 *v=pBuf->Get();
				float size = (it->phasa)*max_size;
				Vect3f sx(size,0,0);
				Vect3f sy(0,size,0);
				static int alpha = 255;
				color.a = alpha*(1 - it->phasa);
				if (Texture){
					const sRectangle4f& rt = Texture->GetFramePos(1-it->phasa);
					v[0].pos=it->pos-sx-sy; v[0].diffuse=color; v[0].GetTexel().set(rt.min.x,rt.min.y);	
					v[1].pos=it->pos-sx+sy; v[1].diffuse=color; v[1].GetTexel().set(rt.min.x,rt.max.y);
					v[2].pos=it->pos+sx-sy; v[2].diffuse=color; v[2].GetTexel().set(rt.max.x,rt.min.y);
					v[3].pos=it->pos+sx+sy; v[3].diffuse=color; v[3].GetTexel().set(rt.max.x,rt.max.y);
				}
				else{
					v[0].pos=it->pos-sx-sy; v[0].diffuse=color; v[0].GetTexel().set(0,1);
					v[1].pos=it->pos-sx+sy; v[1].diffuse=color; v[1].GetTexel().set(0,0);
					v[2].pos=it->pos+sx-sy; v[2].diffuse=color; v[2].GetTexel().set(1,1);
					v[3].pos=it->pos+sx+sy; v[3].diffuse=color; v[3].GetTexel().set(1,0);
				}

				++it;
			}
			else
				it = drops.erase(it);
		}
	}

	pBuf->EndDraw();
}

void cCircles::Animate(float dt)
{
//	this->dt = dt*1e-3f;
}

const char* cCircles::GetTextureName()
{
	if (Texture)
		return Texture->GetName();
	else return texture_name.c_str();
}
void cCircles::SetTexture(const char* name)
{
	if (Texture)
		RELEASE(Texture);
	Texture = (cTextureAviScale*)GetTexLibrary()->GetElement3DAviScale(name);	
}

float cCircles::GetSize()
{
	return max_size;
}
void cCircles::SetSize(float size)
{
	max_size = size;
}

float cCircles::GetTime()
{
	return time;
}
void cCircles::SetTime(float time)
{
	this->time = max(time, 0.01f);
}
