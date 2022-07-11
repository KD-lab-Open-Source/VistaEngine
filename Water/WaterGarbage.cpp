#include "StdAfx.h"
#include "..\Util\Timers.h"
#include "Serialization.h"
#include "WaterGarbage.h"
#include "Water.h"
#include "RangedWrapper.h"
#include "ResourceSelector.h"
#include "RenderObjects.h"

class FunctorWater:public FunctorGetZ
{
	cWater* pWater;
public:
	FunctorWater(cWater* pWater_)
		:pWater(pWater_)
	{
	}

	void UpdateInterface(cEffect* parent)
	{
	}

	float GetZ(float pos_x,float pos_y)
	{
		return pWater->GetZ(pos_x,pos_y);
	}
};
/*
cWaterGarbage::cWaterGarbage(cWater* pWater_, cTileMap* terrain_)
:pWater(pWater_), terrain(terrain_)
{
	pFunctorWater=new FunctorWater(pWater);
	EffectLibrary* lib=gb_VisGeneric->GetEffectLibrary("balmerz");
	if (lib)
		effect_key=lib->Get("Z");
	else 
		effect_key = NULL;
}

cWaterGarbage::~cWaterGarbage()
{
	RELEASE(pFunctorWater);
	vector<OneObject>::iterator it;
	FOR_EACH(objects,it)
	{
		it->object->Release();
		it->effect->Release();
	}

	vector<SpeedWater>::iterator it_speed;
	FOR_EACH(speed_water,it_speed)
	{
		SpeedWater& s=*it_speed;
		if(s.effect)
		{
			s.effect->StopAndReleaseAfterEnd();
			s.effect=NULL;
		}
	}
}

void cWaterGarbage::Animate(float dt)
{
	float delta_time=dt*1e-3f;
	vector<OneObject>::iterator it;
	FOR_EACH(objects,it)
	{
		OneObject& obj=*it;
		if(obj.life_time<=0)
		{
			obj.object->Release();
			obj.effect->StopAndReleaseAfterEnd();
			it=objects.erase(it);
			it--;
			continue;
		}
		obj.life_time-=delta_time;
		MatXf pos=obj.object->GetPosition();
		int x=round(pos.trans().x),y=round(pos.trans().y);
		Vect2f vel=pWater->GetVelocity(x,y);
		float angle=atan2(vel.x,vel.y);

		pos.trans().x+=vel.x*delta_time;
		pos.trans().y+=vel.y*delta_time;
		pos.trans().z=pWater->GetZ(x,y);
		obj.object->SetPosition(pos);
		Mat3f rot(M_PI-angle,Z_AXIS);
		pos.rot()=rot;
		obj.effect->SetPosition(pos);
	}

	vector<SpeedWater>::iterator it_speed;
	FOR_EACH(speed_water,it_speed)
	{
		SpeedWater& s=*it_speed;
		int x=s.global_pos.x;
		int y=s.global_pos.y;
		Vect2f vel=pWater->GetVelocity(x,y);
		float angle=atan2(vel.x,vel.y);
		MatXf pos=s.effect->GetPosition();
		Mat3f rot(-angle,Z_AXIS);
		pos.rot()=rot;
		s.effect->SetPosition(pos);
	}
}

void cWaterGarbage::Drop(int x,int y,cObject3dx* object,float life_time)
{
	OneObject obj;
	obj.object=object;
	obj.life_time=life_time;
	MatXf matr=MatXf::ID;
	matr.trans().x=x;
	matr.trans().y=y;
	matr.trans().z=pWater->GetZ(x,y);
	object->SetPosition(matr);
	obj.effect=terrain->GetScene()->CreateEffectDetached(*effect_key,NULL,false);
	obj.effect->SetFunctorGetZ(pFunctorWater);
	obj.effect->SetPosition(matr);
	attachSmart(obj.effect);
	objects.push_back(obj);
}

int cWaterGarbage::GetTriggerSpeed()
{
	return 220000;//250000;
}

void cWaterGarbage::AddSpeedWater(int x,int y,int speed_x,int speed_y)
{
	int sz=4;
	if(!(x>sz && y>sz && x<pWater->GetGridSizeX()-sz && y<pWater->GetGridSizeY()-sz))
		return;

	cWater::OnePoint* cur;
	//cur=&pWater->Get(x,y);
	//if(cur->type!=cWater::type_filled)
	//	return;
	//cur=&pWater->Get(x-1,y);
	//if(cur->type!=cWater::type_filled)
	//	return;
	//cur=&pWater->Get(x,y-1);
	//if(cur->type!=cWater::type_filled)
	//	return;
	cur=&pWater->Get(x-1,y-1);
	if(cur->type!=cWater::type_filled)
		return;

	SpeedOne s;
	s.pos.set(x,y);
	s.global_pos.set(x<<cWater::grid_shift,y<<cWater::grid_shift);

	if(speed_x>speed_y)
	{
		s.global_pos.x-=1<<(cWater::grid_shift-1);
	}else
	{
		s.global_pos.y-=1<<(cWater::grid_shift-1);
	}


	temp_speed_water.push_back(s);
}

void cWaterGarbage::ProcessSpeedWater()
{
	vector<SpeedWater> new_speed_water;
	vector<SpeedWater>::iterator it_speed=speed_water.begin();
	vector<SpeedOne>::iterator it;

	if(speed_water.empty())
	{
		int size=temp_speed_water.size();
		new_speed_water.resize(size);
		for(int i=0;i<size;i++)
		{
			new_speed_water[i].pos=temp_speed_water[i].pos;
			new_speed_water[i].global_pos=temp_speed_water[i].global_pos;
			AddSpeed(new_speed_water[i]);
		}
	}else
	{
		it=temp_speed_water.begin();
		while(it!=temp_speed_water.end())
		{
			if(it_speed==speed_water.end())
				break;
			if(it->pos==it_speed->pos)
			{
				new_speed_water.push_back(*it_speed);
				it_speed++;
				it++;
				continue;
			}

			if(*it_speed<*it)
			{
				DeleteSpeed(*it_speed);
				it_speed++;
			}else
			{
				SpeedWater s;
				s.pos=it->pos;
				s.global_pos=it->global_pos;
				new_speed_water.push_back(s);
				AddSpeed(new_speed_water.back());
				it++;
			}
		}

		while(it!=temp_speed_water.end())
		{
			SpeedWater s;
			s.pos=it->pos;
			s.global_pos=it->global_pos;
			new_speed_water.push_back(s);
			AddSpeed(new_speed_water.back());
			it++;
		}

		while(it_speed!=speed_water.end())
		{
			DeleteSpeed(*it_speed);
			it_speed++;
		}
	}

	temp_speed_water.clear();
	speed_water.swap(new_speed_water);
}

void cWaterGarbage::AddSpeed(SpeedWater& s)
{
	MatXf matr=MatXf::ID;
	matr.trans().x=s.global_pos.x;
	matr.trans().y=s.global_pos.y;
	matr.trans().z=0;
	s.effect=terrain->GetScene()->CreateEffectDetached(*effect_key,NULL,false);
	s.effect->SetFunctorGetZ(pFunctorWater);
	s.effect->SetPosition(matr);

	attachSmart(s.effect);
}

void cWaterGarbage::DeleteSpeed(SpeedWater& s)
{
	if(s.effect)
	{
		s.effect->StopAndReleaseAfterEnd();
		s.effect=NULL;
	}
}
*/

/////////////////////////////cWaterBubble//////////////////////////////
cWaterBubble::cWaterBubble(class cWater* pWater_, cTileMap* terrain_)
:cBaseGraphObject(0),pWater(pWater_), terrain(terrain_)
{
}

cWaterBubble::~cWaterBubble()
{
	centers.clear();
	vector<ObjectsTextured*>::iterator itt;
	FOR_EACH(textured,itt)
	{
		RELEASE((*itt)->pTexture);
		delete *itt;
	}

	xassert(centers.empty());
}

cWaterBubbleCenter* cWaterBubble::AddCenter(int x,int y,int radius,float speed,const char* textureName)
{
	cWaterBubbleCenter* c=new cWaterBubbleCenter();
	c->SetPos(Vect2f(float(x),float(y)));
	c->SetRadius(float(radius));
	//c->setPosition(Vect3f(float(x), float(y), 0.0f));
	//c->setRadius(float(radius));
	c->generate_interval=1.0f/speed;
	c->generate_sum=0;
	c->SetTextureName(textureName);
	AddCenter(c);
	return c;
}

void cWaterBubble::AddCenter(cWaterBubbleCenter* c)
{
	//cTexture* pTexture = GetTexLibrary()->GetElement(c->GetTextureName());
	cTexture* pTexture = GetTexLibrary()->GetElement3D(c->GetTextureName());
	c->parent=this;

	vector<ObjectsTextured*>::iterator itt;
	ObjectsTextured* found=NULL;
	FOR_EACH(textured,itt)
	{
		if((*itt)->pTexture==pTexture)
		{
			found=*itt;
			break;
		}
	}

	if(found==NULL)
	{
		found=new ObjectsTextured;
		found->pTexture=pTexture;
		if(pTexture)
			found->pTexture->AddRef();
		textured.push_back(found);
	}

	c->slot=found;
	xassert(find(centers.begin(), centers.end(), c) == centers.end());
	centers.push_back(c);
    RELEASE(pTexture);
}

void cWaterBubble::DeleteCenter(cWaterBubbleCenter* center)
{
	xassert(center->parent==this);
	Centers::iterator it;
	FOR_EACH(centers,it)
	if(*it==center)
	{
		center->parent=NULL;
		centers.erase(it);
		return;
	}

	xassert(0);
}

void cWaterBubble::DeleteNearCenter(int x,int y)
{
	int ifound=-1;
	int distance=INT_MAX;
	for(int i=0;i<centers.size();i++){
		cWaterBubbleCenter& c=*centers[i];
		int d=sqr(c.GetPos().x-x)+(c.GetPos().y-y);
		if(d<distance){
			ifound=i;
			distance=d;
		}
	}

	if(ifound>=0){
		Centers::iterator it=centers.begin()+ifound;
		(*it)->parent=NULL;
		delete *it;
		centers.erase(it);
	}
}

void cWaterBubble::PreDraw(cCamera *pCamera)
{
	pCamera->Attach(SCENENODE_OBJECTSPECIAL,this);
	//pCamera->Attach(SCENENODE_OBJECTSORT,this);
	
}

void cWaterBubble::Animate(float dt)
{
	dt*=1e-3f;
	AnimateCenter(dt);
	AnimateParticle(dt);
}

void cWaterBubble::AnimateCenter(float dt)
{
	TerraInterface* terra=terrain->GetTerra();
	Centers::iterator it;
	FOR_EACH(centers,it)
		if ((*it)->active())
		{
			cWaterBubbleCenter& c=**it;

			c.generate_sum+=dt;
			while(c.generate_sum>c.generate_interval)
			{
				c.generate_sum-=c.generate_interval;

				OneObject& obj=GetSlot(c)->objects.GetFree();
				obj.pos.x=c.GetPos().x+graphRnd.frnd()*c.GetRadius();
				obj.pos.y=c.GetPos().y+graphRnd.frnd()*c.GetRadius();
				obj.pos.x=clamp(obj.pos.x,0.0f,terra->SizeX());
				obj.pos.y=clamp(obj.pos.y,0.0f,terra->SizeY());
				obj.pos.z=terra->GetZf(obj.pos.x,obj.pos.y);
				obj.life_time=c.GetMinLifeTime()+pow(graphRnd.frand(),4)*(c.GetMaxLifeTime()-c.GetMinLifeTime());
				obj.size=graphRnd.frand()*3+1;
				obj.delete_on_up=c.delete_on_up;
			}
		}
}

void cWaterBubble::AnimateParticle(float dt)
{
	float dz_speed=dt;

	vector<ObjectsTextured*>::iterator itt;
	FOR_EACH(textured,itt)
	{
		BackVector<OneObject>& objects=(*itt)->objects;
		int size=objects.size();
		for(int i=0;i<size;i++)
		{
			if(objects.IsFree(i))
				continue;
			OneObject& obj=objects[i];

			obj.life_time-=dt;
			if(obj.life_time<0)
			{
				objects.SetFree(i);
				continue;
			}

			int x=round(obj.pos.x),y=round(obj.pos.y);
			Vect2f vel=pWater->GetVelocity(x,y);

			obj.pos.x+=vel.x*dt;
			obj.pos.y+=vel.y*dt;

			float water_z=pWater->GetZ(x,y);
			if(obj.delete_on_up)
			{
				if(obj.pos.z+1>water_z)
				{
					objects.SetFree(i);
					continue;
				}
			}

			if(obj.pos.z>water_z)
				obj.pos.z=water_z;
			else
				obj.pos.z=(water_z-obj.pos.z)*dz_speed+obj.pos.z;
		}

		objects.Compress();
	}
}

void cWaterBubble::Draw(cCamera *pCamera)
{
//	dist.Draw(pCamera);
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	int old_zwrite=rd->GetRenderState(RS_ZWRITEENABLE);
	rd->SetRenderState(RS_ZWRITEENABLE,FALSE);
	MatXf mat=pCamera->GetMatrix();
	vector<ObjectsTextured*>::iterator itt;
	FOR_EACH(textured,itt)
	{
		BackVector<OneObject>& objects=(*itt)->objects;
		int size=objects.size();
		if(size==0)
			continue;
		rd->SetNoMaterial(ALPHA_BLEND, MatXf::ID, 0, (*itt)->pTexture);

		cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();
		pBuf->BeginDraw();
		for(int i=size-1;i>=0;i--)
		{
			if(objects.IsFree(i))
				continue;
			OneObject& p=objects[i];

			//Добавить в массив
			Vect3f sx,sy;
			Vect2f rot(1,0);
			rot*=p.size;
			mat.invXformVect(Vect3f(+rot.x,-rot.y,0),sx);
			mat.invXformVect(Vect3f(+rot.y,+rot.x,0),sy);

			sColor4c color(255,255,255,255);
			sVertexXYZDT1 *v=pBuf->Get();
			v[0].pos=p.pos-sx-sy; v[0].diffuse=color; v[0].GetTexel().set(0,1);
			v[1].pos=p.pos-sx+sy; v[1].diffuse=color; v[1].GetTexel().set(0,0);
			v[2].pos=p.pos+sx-sy; v[2].diffuse=color; v[2].GetTexel().set(1,1);
			v[3].pos=p.pos+sx+sy; v[3].diffuse=color; v[3].GetTexel().set(1,0);
		}
		pBuf->EndDraw();
	}
	rd->SetRenderState(RS_ZWRITEENABLE,old_zwrite);
}

cWaterBubbleCenter::~cWaterBubbleCenter()
{
}
void cWaterBubbleCenter::SetSpeed(float s)
{
	s=max(s,1e-6f);
	generate_interval=1.0f/s;
	generate_sum=min(generate_sum,generate_interval);
}

cWaterBubbleCenter::cWaterBubbleCenter(const cWaterBubbleCenter& original)
//: SourceBase(original)
: parent(0)
, texture_name(original.texture_name)
, slot(0) 
, generate_interval(original.generate_interval)
, generate_sum(original.generate_sum)
, min_life_time(original.min_life_time)
, max_life_time(original.max_life_time)
, delete_on_up(original.delete_on_up)
, position(original.position)
, radius(original.radius)
{
}

cWaterBubbleCenter::cWaterBubbleCenter()
: parent(NULL)
{
	slot=NULL;
	generate_interval=1.0f;
	generate_sum=0;
	min_life_time=1;
	max_life_time=21;
	delete_on_up=true;
	//setActivity(true);
	active_ = true;
}

cWaterBubbleCenter::cWaterBubbleCenter(int x, int y, int radius, float speed, const char* textureName)
//: SourceBase()
//, parent(NULL)
//, texture_name("")
{
	slot=NULL;
	min_life_time=1;
	max_life_time=21;
	delete_on_up=true;
	//setActivity(true);
	//setPosition(Vect3f(float(x), float(y), 0.0f));
	//setRadius(float(radius));
	SetPos(Vect2f(float(x),float(y)));
	SetRadius(float(radius));
	generate_interval=1.0f/speed;
	generate_sum=0;
	SetTextureName(textureName);
	active_ = true;
}

void cWaterBubbleCenter::SetTextureName(const char* texture_name_)
{
	xassert(parent==NULL);
	texture_name=texture_name_;
}

void cWaterBubbleCenter::serialize(Archive& ar) 
{
	//__super::serialize(ar);
	float speed=GetSpeed();
	ar.serialize(RangedWrapperf (speed, 0.0f, 1000.0f, 5.0f), "speed", "Скорость генерации пузырьков");

	ar.serialize(RangedWrapperf (min_life_time, 0.1f, 1000.0f, 1.0f), "min_life_time", "Минимальное время жизни");
	ar.serialize(RangedWrapperf (max_life_time, 0.1f, 1000.0f, 1.0f), "max_life_time", "Максимальное время жизни");

	ar.serialize(generate_interval, "generate_interval", 0);
	ar.serialize(generate_sum, "generate_sum", 0);
	ar.serialize(ResourceSelector(texture_name, ResourceSelector::TEXTURE_OPTIONS), "texture_name", "Текстура");

	//if(ar.isInput() && ar.isEdit() &&  parent)
	//{	//Для редактора
	//	cWaterBubble* p=parent;
	//	p->DeleteCenter(this);
	//	p->AddCenter(this);
	//}

	ar.serialize(delete_on_up, "delete_on_up", "Пузырьки исчезают на поверхности");

	if(ar.isInput())
		SetSpeed(speed);
}	

void cWaterBubble::serialize(Archive& ar)
{
	ar.serialize(centers, "centers", 0);

	if(ar.isInput()){
		Centers::iterator it;
		FOR_EACH(centers,it)
			AddCenter(*it);
	}
}

