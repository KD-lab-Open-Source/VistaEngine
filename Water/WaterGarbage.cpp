#include "StdAfx.h"
#include "Timers.h"
#include "Serialization\Serialization.h"
#include "WaterGarbage.h"
#include "Water.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\ResourceSelector.h"
#include "Terra\vmap.h"
#include "Serialization\SerializationFactory.h"
#include "Render\Src\cCamera.h"
#include "Render\Src\TexLibrary.h"

/////////////////////////////cWaterBubble//////////////////////////////
cWaterBubble::cWaterBubble(class cWater* pWater_)
:BaseGraphObject(0),pWater(pWater_)
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
	ObjectsTextured* found=0;
	FOR_EACH(textured,itt)
	{
		if((*itt)->pTexture==pTexture)
		{
			found=*itt;
			break;
		}
	}

	if(found==0)
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
		center->parent=0;
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
		(*it)->parent=0;
		delete *it;
		centers.erase(it);
	}
}

void cWaterBubble::PreDraw(Camera* camera)
{
	camera->Attach(SCENENODE_OBJECTSPECIAL,this);
	//camera->Attach(SCENENODE_OBJECTSORT,this);
	
}

void cWaterBubble::Animate(float dt)
{
	dt*=1e-3f;
	AnimateCenter(dt);
	AnimateParticle(dt);
}

void cWaterBubble::AnimateCenter(float dt)
{
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
				obj.pos.x=clamp(obj.pos.x,0.0f,vMap.H_SIZE);
				obj.pos.y=clamp(obj.pos.y,0.0f,vMap.V_SIZE);
				obj.pos.z = vMap.getZf(obj.pos.x,obj.pos.y);
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

void cWaterBubble::Draw(Camera* camera)
{
//	dist.Draw(camera);
	cInterfaceRenderDevice* rd=gb_RenderDevice;
	int old_zwrite=rd->GetRenderState(RS_ZWRITEENABLE);
	rd->SetRenderState(RS_ZWRITEENABLE,FALSE);
	MatXf mat=camera->GetMatrix();
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

			Color4c color(255,255,255,255);
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
: parent(0)
{
	slot=0;
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
//, parent(0)
//, texture_name("")
{
	slot=0;
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
	xassert(parent==0);
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
	static ResourceSelector::Options textureOptions("*.tga", "Resource\\TerrainData\\Textures");
	ar.serialize(ResourceSelector(texture_name, textureOptions), "texture_name", "Текстура");

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

