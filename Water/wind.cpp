#include "StdAfx.h"
#include "Wind.h"
#include "Serialization.h"
#include "ResourceSelector.h"
#include "RenderObjects.h"
#include "..\Util\RangedWrapper.h"
#include "CameraManager.h"
//#include "Simply3dx.h"

#include "..\terra\terra.h"
#include "..\Environment\Environment.h"

//==================================================================
BEGIN_ENUM_DESCRIPTOR(ListTypeQuant, "ListTypeQuant");
REGISTER_ENUM(LINEAR_WIND, "Линейный ветер");
REGISTER_ENUM(TWISTING_WIND, "Крутящийся ветер");
REGISTER_ENUM(BLAST_WIND, "Взрывная волна");
END_ENUM_DESCRIPTOR(ListTypeQuant);

//==================================================================
//class cQuantumWind
cQuantumWind::~cQuantumWind()
{
	if(ef)
		ef->StopAndReleaseAfterEnd();
}
void cQuantumWind::Animate(float dt)
{
	if(owner) {
		Vect2f delta = Vect2f(owner->position()) - last_owner_position;
		last_owner_position = owner->position();
	}
	
	float k = k_fading*dt;
	r+=r*k_fading*dt;
	dr+=dr*k_fading*dt;
	maxz-=maxz*k_fading*dt;
}
//------------------------------------------------------------------------------------------
void cQuantumWindL::Animate(float dt)
{
	if(owner) {
		pos += Vect2f(owner->position()) - last_owner_position;
		last_owner_position = owner->position();
	}

	pos.x+=vel.x*dt;
	pos.y+=vel.y*dt;
	r-=r*(dr*dt);
	float k = k_fading*dt;
	vel -= vel*k;
	vel_wind -= vel_wind*k;
	if(ef)
		ef->SetPosition(MatXf(Mat3f(M_PI/2+atan2(vel_wind.y,vel_wind.x),Z_AXIS), Vect3f(pos.x, pos.y, surf_z+add_z) ));
}
//------------------------------------------------------------------------------------------
void cQuantumWindW::Animate(float dt)
{
	if(owner) {
		pos += Vect2f(owner->position()) - last_owner_position;
		last_owner_position = owner->position();
	}
	/*
	pos.x+=vel.x*dt;
	pos.y+=vel.y*dt;
	*/
	r-=r*(dr*dt);
	float k = k_fading*dt;
	vel -= vel*k;
	w -= w*k;
	if(ef)
		ef->SetPosition(MatXf(Mat3f::ID, Vect3f(pos.x, pos.y, surf_z+add_z)));
}

//==================================================================
//struct ObjNodes

void ObjNodes::Set(cObject3dx* pObj)
{
	xassert(pObj);
	obj = pObj;
	vector<cStaticNode>::iterator it;
	int i=0;
	FOR_EACH(pObj->GetStatic()->nodes, it)
	{
		if (it->name.size()>=5&&memcmp(it->name.begin(),"wind",4)==0)
		{
			inds.push_back(i);
		}
		i++;
	}
}

void SimplyObjNodes::Set(cSimply3dx* pObj)
{
	xassert(pObj);
	obj = pObj;
	vector<string>::iterator it;
	int i=0;
	FOR_EACH(pObj->GetStatic()->node_name, it)
	{
		string &str = *it;
		if (str.size()>=5&&memcmp(str.begin(),"wind",4)==0)
		{
			inds.push_back(i);
		}
		i++;
	}
}


//------------------------------------------------------------------------------------------
bool WindQuantInfo::SetEffectName(const string& filename)
{
	EffectLibrary* lib=gb_VisGeneric->GetEffectLibrary(filename.c_str());
	if (lib)
	{
		if (lib->begin()!=lib->end())
			effect = *lib->begin();
		else 
		{
			xassert(0&&"пустая библиотека спецэффектов");
			effect = NULL;
			return false;
		}
	}else return false;
	return true;
}
const string& WindQuantInfo::GetEffectName()
{
	const static string empty;
	if (effect)
		return effect->filename;
	return empty;
}

float SourceWind::windVelocity() const
{
	return windVelocity_;
}

void SourceWind::setWindVelocity(float speed)
{
	windVelocity_ = speed;
	FastNormalize(prototype.vel);
	prototype.vel *= speed;
}

void SourceWind::quant()
{
	float dt = logicPeriodSeconds;
	cMapWind* wind = environment->getWind();
	SourceWind* source = this;

	if (active()) {
		if (source->cur_interval>=source->interval && (source->prototype.type != TWISTING_WIND || source->noQuantumAttached_)) {
			wind->CreateQuant(source->prototype, &*source);
			source->cur_interval = dt;
			source->noQuantumAttached_ = false;
		} else
			source->cur_interval += dt;
	}

	__super::quant();
}

void SourceWind::start()
{
	__super::start();

	if(!effect_filename.empty()) {
		prototype.SetEffectName(effect_filename.c_str());
	}
}

void SourceWind::stop()
{
	__super::stop();
}

void SourceWind::serialize(Archive& ar)
{
	radius_ = prototype.r;
	__super::serialize(ar);
	prototype.r = radius_;
	ar.serialize(interval, "interval", "Интервал");
	prototype.pos = pose_.trans();
	
	ar.serialize(prototype.type, "Type", 0);

	windVelocity_ = prototype.vel.norm();
	ar.serialize(windVelocity_, "WindAbsVelocity", "Скорость ветра");

	ar.serialize(prototype.dr, "dr", "Изменение радиуса со временем");
	ar.serialize(prototype.k_fading, "Fading", "Коэффициент затухания");
	ar.serialize(prototype.maxz, "Max_Z", "Максимальная высота");
	if (prototype.type==TWISTING_WIND)
		ar.serialize(prototype.w, "Angular", "Угловая скорость ветра");

	if(enabled())
		effect_filename = prototype.GetEffectName();
	if (effect_filename.empty())
		switch(prototype.type)
		{
		case LINEAR_WIND: effect_filename = "Scripts\\Resource\\FX\\wind.effect"; break;
		case TWISTING_WIND: effect_filename = "Scripts\\Resource\\FX\\twister.effect"; break;
		}
	ar.TRANSLATE_NAME(ModelSelector (effect_filename, ModelSelector::EFFECT_OPTIONS),
						"Effect_name", "Сопутствующий эффект");
	ar.serialize(prototype.scale , "scale_wind_effect", "Маштаб спецэффекта");
	ar.serialize(prototype.add_z , "fx_height", "Высота спецэффекта над землей");
	if (ar.isInput()){
		Vect3f dir(Vect3f::J);
		orientation().xform(dir);
		prototype.vel = dir * windVelocity_;
		prototype.vel_wind = prototype.vel;
		if (enabled())//enabled())
			prototype.SetEffectName(effect_filename);
		if(ar.isEdit() &&
			enabled() &&
			environment &&
			environment->getWind()) {
			environment->getWind()->updateWindQuantum(this);
		}
	}
	ar.serialize(prototype.toolser, "toolser", "Тулзер");
}

//------------------------------------------------------------------------------------------
/*void SourceWind::Animate(float dt)
{
	if (cur_interval>=interval)
	{
		xassert(parent);
		parent->CreateQuant(prototype);
		cur_interval = dt;
	}
	else cur_interval += dt;
}
*/

//////////////////////////////// realization cMapWind //////////////////////////////////////
cMapWind* cMapWind::wind_this = NULL;
cMapWind::cMapWind()//:cBaseGraphObject(0)
{
	background = NULL;
	nds = NULL;
	trn = NULL;
	wind = NULL;
	cObject3dx::AddRefContainer(this);
}
//------------------------------------------------------------------------------------------
cMapWind::~cMapWind()
{
	wind_this = NULL;
	cObject3dx::DelRefContainer(this);
	delete[] nds;
	delete[] background;
//	for(int i=0;i<static_quants.size();++i)
//		delete static_quants[i];
}
//------------------------------------------------------------------------------------------
void cMapWind::Init(int w, int h, cScene* scene_, int shift_)//,cWater* pWater_)
{
//	pWater = pWater_;
	scene = scene_;
	xassert(scene);
	shift = shift_;
	xassert(shift);
	x_count = w>>shift;
	y_count = h>>shift;
	xassert(y_count);
	xassert(x_count);
	ng = 1<<shift;
	fng = ng;
	if (nds)
		delete[] nds;
	nds = new WindNode[x_count*y_count];
	memset(nds, 0,x_count*y_count*sizeof(WindNode));
	map_width = w;
	map_height = h;
	if (background)
		delete[] background;
	background = new WindNode[x_count*y_count];
	memset(background, 0, x_count*y_count*sizeof(WindNode));
	quant_num = 1;
	wind_this = this;
}

void cMapWind::serialize(Archive& ar)
{
	serializeParameters(ar);
	if (background && !ar.isEdit())
	{
		int size_buf = x_count*y_count;
		WindNodeBase* buf = new WindNodeBase[size_buf];
		int len = x_count*y_count*sizeof(WindNodeBase);
		XBuffer background_map(len, 1);
		if (ar.isOutput())
		{
			WindNodeBase* end = buf + size_buf;
			WindNode* back = background;
			for(WindNodeBase* it = buf;it<end; it++, back++)
			{
				it->maxz = back->maxz;
				it->vel  = back->vel;
			}
			background_map.write(buf, len);
		}
		ar.serialize(background_map, "background_map", 0);
		if (ar.isInput())
		{
			if (background_map.tell()) 
			{
				background_map.set(0);
				background_map.read(buf, len);
				WindNode* end = background + size_buf;
				WindNodeBase* back = buf;
				for(WindNode* it = background;it<end; it++, back++)
				{
					it->maxz = back->maxz;
					it->vel  = back->vel;
				}
			}
		}
		delete[] buf;
	}	
}

void cMapWind::serializeParameters(Archive& ar)
{
/*
	ar.openBlock("Wind", "Ветер");
		ar.TRANSLATE_NAME(ModelSelector (default_wind, ModelSelector::EFFECT_OPTIONS),
							"default_wind", "Эффект ветра по умолчанию");
		ar.TRANSLATE_NAME(ModelSelector (default_tornado, ModelSelector::EFFECT_OPTIONS),
							"default_tornado", "Эффект смерча по умолчанию");
		if (ar.isInput())
			LoadDefaultEffect();
	ar.closeBlock();
*/
}

//------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------

void cMapWind::Clear()
{
//	memcpy(nds, background, x_count*y_count*sizeof(*nds));
}
//------------------------------------------------------------------------------------------
inline float cMapWind::GetZ(int x, int y)
{
	//return pWater->GetZ(x,y);
	return vMap.getVoxelW(x, y);
}

//------------------------------------------------------------------------------------------
void cMapWind::Animate(float dt)
{
	start_timer_auto(cMapWind, STATISTICS_GROUP_GRAPHICS);
	xassert(nds);
	quant_num++;
	dt*=1e-3f;
	Clear();
	/*
	list<SourceWind>::iterator source;
	FOR_EACH(sources, source)
		if (source->active()) {
			if (source->cur_interval>=source->interval && (source->prototype.type != TWISTING_WIND || source->noQuantumAttached_)) {
				CreateQuant(source->prototype, &*source);
				source->cur_interval = dt;
				source->noQuantumAttached_ = false;
			} else
				source->cur_interval += dt;
		}
	*/
	Quants::iterator q;
	FOR_EACH(quants, q) {
		cQuantumWind* qu = *q;
		qu->Animate(dt);
		int xp = round(qu->pos.x);
		int yp = round(qu->pos.y);
		if ((UINT)xp>=map_width || (UINT)yp>=map_height || !qu->IsLive() ||
			(qu->owner && !qu->owner->isAlive())) {
			if(qu->owner)
				qu->owner->noQuantumAttached_ = true;

			delete qu;
			q = quants.erase(q);
			--q;
			continue;
		}
		qu->surf_z = vMap.getVoxelW(xp, yp);
		qu->toolser.setPosition(Se3f(MatXf(Mat3f::ID, Vect3f(qu->pos.x, qu->pos.y, qu->surf_z))));
		switch(qu->TypeId())
		{
		case LINEAR_WIND:
			SetL((cQuantumWindL*)qu);
			break;
		case TWISTING_WIND:
			SetW((cQuantumWindW*)qu);
			break;
		case BLAST_WIND:
			SetRing(qu);
			break;
		}
	}
	vector<ObjNodes>::iterator obj;
	static float wind_force = 0.5;//1.0f;
	static float k = 30.0f;//15.0f;
	static float kr = 0.6f;
	static float phase = 0;
	{
		static bool phase_dir = true;
		static float k_phase = 0.4f;
		static float min_a = -M_PI/8;
		static float max_a = M_PI/8;
		if (phase_dir)
		{
			phase+=dt*k_phase;
			if (phase>1)
			{
				phase = 1;
				phase_dir = false;
			}
		}else
		{
			phase-=dt*k_phase;
			if (phase<0)
			{
				phase = 0;
				phase_dir = true;
			}
		}
	}
	cCamera *pCamera = cameraManager->GetCamera();
	float wind_force_dt = wind_force*dt;
	float k_dt = k*dt;
	float min_kr_dt = min(kr*dt,0.9f);
	float wind_effect = /*graphRnd.frand()**/phase;//Общее колыхание для всех объектов.
	FOR_EACH(objects, obj)
	{
		vector<NodeObj>::iterator i;
		if(pCamera->TestVisible(obj->obj->GetPosition().trans(),obj->obj->GetBoundRadius()*obj->obj->GetScale()))
		{
			sBox6f Bound;
			obj->obj->GetBoundBox(Bound);
			static int default_size = 50;
			float BoundK = (Bound.max.z-Bound.min.z)/default_size*wind_effect;
			FOR_EACH(obj->inds, i)
			{
				MatXf mx;
				mx.set(obj->obj->GetNodePosition(i->ix));
				Vect3f v = mx.trans();
				//Общее колыхание*Колыхание ноды*Отношение размеров*Фазa движения*Вектор ветра
				v = graphRnd.frand()*BoundK*GetVelocity(v);//scale*T*GetVelocity(v)
				i->vel+= v*(wind_force_dt) - i->dis*(k_dt);
				i->vel-=i->vel*min_kr_dt;
				i->dis += i->vel*dt;

				Se3f u;
				u.trans() = i->dis;
				u.rot().set(QuatF::ID);
				obj->obj->SetUserTransform(i->ix,u);
			}
		}
	}
	
	vector<UnitEnvironmentSimple*>::iterator unit_environment;
	FOR_EACH(unit_environment_objects, unit_environment)
	{
		sBox6f Bound;
		cSimply3dx* object = (*unit_environment)->modelSimple();
		object->GetBoundBox(Bound);
		Vect3f vel = object->GetPosition().trans();
		vel.z += Bound.max.z-Bound.min.z;
		vel = GetVelocity(vel);
		//vel.z = -vel.z;
		//vel.z =  - sqrt((Bound.max.z-Bound.min.z)*(Bound.max.z-Bound.min.z) - (vel.x*vel.x +vel.y*vel.y));
		//(*unit_environment)->springDamping3DX()->Accelerate(object,vel,dt);
	}
}
//------------------------------------------------------------------------------------------
void cMapWind::CreateQuant(WindQuantInfo& dat, SourceWind* owner)
{
	cQuantumWind *t = NULL;
	switch(dat.type)
	{
	case LINEAR_WIND:	t = new cQuantumWindL; 	break;
	case TWISTING_WIND:	t = new cQuantumWindW;	break;
	case BLAST_WIND:	t = new cQuantumWind;	break;
	}
	if (t)
	{
		t->pos = dat.pos;
		t->r = dat.r;
		t->dr = dat.dr;
		t->vel = dat.vel;
		t->maxz = dat.maxz;
		t->k_fading = dat.k_fading;
		t->surf_z = GetZ(round(dat.pos.x),round(dat.pos.y));
		t->add_z = dat.add_z;
		t->toolser = dat.toolser;
		t->toolser.start(Se3f(MatXf(Mat3f::ID, Vect3f(dat.pos.x, dat.pos.y, t->surf_z))));
		t->owner = owner;
		t->last_owner_position = owner->position();

		switch(dat.type)
		{
		case LINEAR_WIND:	((cQuantumWindL*)t)->vel_wind = dat.vel_wind;	break;
		case TWISTING_WIND:	((cQuantumWindW*)t)->w = dat.w;					break;
		}
		if (dat.GetEffectKey())
		{
			t->ef = scene->CreateEffect(*dat.GetEffectKey(), NULL, dat.scale, true);		
			if (t->ef)
				t->ef->AddZ(t->add_z);
		}	
		t->Animate(0);
		quants.push_back(t);
	}else xassert(false);
}
//------------------------------------------------------------------------------------------
int cMapWind::CreateStaticQuant(StaticWindQuantInfo& dat)
{
	cQuantumWindL t;
	t.pos = dat.pos;
	t.r = dat.r;
	t.maxz = dat.maxz;
	if (dat.type==LINEAR_WIND)
	{
		t.vel_wind = dat.vel_wind;
		SetStaticL(&t); 	
	}
	return 0;
}
//------------------------------------------------------------------------------------------
inline WindNode& cMapWind::GetValidNode(UINT index)
{
	xassert((UINT)index< x_count*y_count);
	WindNode &ret = nds[index];
	if (ret.quant_num!=quant_num)
	{
		WindNode &back = background[index];
		ret.maxz = back.maxz;
		ret.vel = back.vel;
		ret.quant_num = quant_num;
	}
	return ret;
}

inline const WindNode& cMapWind::GetNode(int index)
{
//	xassert((UINT)index< x_count*y_count);
	WindNode &ret = nds[index];
	if (ret.quant_num!=quant_num)
	{
//		xassert((UINT)round(background[index].vel.x)<1000);
		return background[index];
	}
//	xassert((UINT)round(ret.vel.x)<1000);
	return ret;
}
//inline const WindNode& cMapWind::GetNode(int ix, int iy)
//{
//	return GetNode(ix + iy*x_count);
//}
const WindNode& cMapWind::GetNode(const float fx, const float fy)
{
	int x = round(fx);
	if (x<0) x=0;
	else if (x>=map_width) x = map_width-1;
	int y = round(fy);
	if (y<0) y=0;
	else if (y>=map_height) y = map_height-1;
	return GetNode((x>>shift) + (y>>shift)*x_count);
}
//------------------------------------------------------------------------------------------
Vect3f cMapWind::GetVelocity(const Vect3f& p)
{
	float surz = GetZ(round(p.x),round(p.y));
	if (p.z<surz)
		return Vect3f::ZERO;
	int x =	round(p.x)&(-ng);
	int y =	round(p.y)&(-ng);
	if (x>p.x) x-=ng;
	if (y>p.y) y-=ng;
	float kx = (p.x - x)/ng;
	float ky = (p.y - y)/ng;
	x>>=shift;
	y>>=shift;
	if (x<0)				x=0;
	else if (x>=x_count-1) 	x = x_count - 2;
	if (y<0)				y=0;
	else if (y>=y_count-1) 	y = y_count - 2;
	int ym = y*x_count;
	int y1m = (y+1)*x_count;

	const WindNode &xym = GetNode(x+ym);
	const WindNode &x1ym = GetNode(x+1+ym);
	const WindNode &xy1m = GetNode(x+y1m);
	const WindNode &x1y1m = GetNode(x+1+y1m);
	float v_x =  bilinear(xym.vel.x, x1ym.vel.x, xy1m.vel.x, x1y1m.vel.x, kx, ky);
	float v_y =  bilinear(xym.vel.y, x1ym.vel.y, xy1m.vel.y, x1y1m.vel.y, kx, ky);
	float z = bilinear(xym.maxz,x1ym.maxz, xy1m.maxz, x1y1m.maxz, kx, ky)+surz;

//	z = (z<=p.z)? 0 : (z+surz)-p.z;
	z = ((z+surz)-p.z)*0.5f;
	return Vect3f(v_x,v_y,z);
}
//------------------------------------------------------------------------------------------
void cMapWind::SetL(cQuantumWindL* ql)
{
	int ixb = max(round(ql->pos.x - ql->r)>>shift, 0);
	int iy = max(round(ql->pos.y - ql->r)>>shift, 0);
	int ix_lim = min(round(ql->pos.x + ql->r)>>shift, x_count);
	int iy_lim = min(round(ql->pos.y + ql->r)>>shift, y_count);

	float y = iy<<shift;
	float xb = ixb<<shift;
	UINT index = ixb + iy*x_count;
	UINT nd_add = x_count - (ix_lim - ixb);
	float rp2 = ql->r*ql->r;
	for(;iy<iy_lim; ++iy)
	{
		float x = xb;
		for(int ix = ixb; ix<ix_lim; ++ix)
		{
			float rx = x-ql->pos.x;
			float ry = y-ql->pos.y;
			float r1=rx*rx + ry*ry;
			if (r1 <= rp2 )
			{
				float r1r = r1/rp2;
				float k = 1 - r1r;
				WindNode &nd = GetValidNode(index);
				nd.vel.x += ql->vel_wind.x*k;
				nd.vel.y += ql->vel_wind.y*k;
				nd.maxz  = max(nd.maxz, ql->maxz*r1r);
			}
			x+=fng;
			index++;
//			++nd;
		}			
		y+=fng;
		index+=nd_add;
//		nd += nd_add;
	}
}
//------------------------------------------------------------------------------------------
void cMapWind::SetW(cQuantumWindW* q)//WindNode* nds
{
	int ixb = max(round(q->pos.x - q->r)>>shift, 0);
	int iy = max(round(q->pos.y - q->r)>>shift, 0);
	int ix_lim = min(round(q->pos.x + q->r)>>shift, x_count);
	int iy_lim = min(round(q->pos.y + q->r)>>shift, y_count);

	float y = iy<<shift;
	float xb = ixb<<shift;
	UINT index = ixb + iy*x_count;
	UINT nd_add = x_count - (ix_lim - ixb);
//	WindNode* nd = nds + (iy*x_count + ixb);
	float rp2 = q->r*q->r;
	for(;iy<iy_lim; ++iy)
	{
		float x = xb;
		for(int ix = ixb; ix<ix_lim; ++ix)
		{
			float rx = x-q->pos.x;
			float ry = y-q->pos.y;
			float r1=rx*rx + ry*ry;
			if (r1<=rp2)
			{
				float r1r = r1/rp2;
				float k = 1 - r1r;
				WindNode &nd = GetValidNode(index);
				nd.vel.x += q->w*ry*k;
				nd.vel.y += -q->w*rx*k;
				nd.maxz  = max(nd.maxz, q->maxz*r1r);
			}
			x+=fng;
			index++;
//			++nd;
		}			
		y+=fng;
		index+=nd_add;
//		nd += nd_add;
	}
}
//------------------------------------------------------------------------------------------
void cMapWind::SetRing(cQuantumWind* q)
{
	float r2 = q->r + q->dr;
	int ixb = max(round(q->pos.x - r2)>>shift, 0);
	int iy = max(round(q->pos.y - r2)>>shift, 0);
	int ix_lim = min(round(q->pos.x + r2)>>shift, x_count);
	int iy_lim = min(round(q->pos.y + r2)>>shift, y_count);

	float y = iy<<shift;
	float xb = ixb<<shift;
	UINT index = xb + y*x_count;
	UINT nd_add = x_count - (ix_lim - ixb);
//	WindNode* nd = nds + (iy*x_count + ixb);
	float r1s = sqr(q->r);
	float r2s = sqr(r2);
	float sr = sqr((q->r+r2)*0.5f);
	static float w = .6f;
	for(;iy<iy_lim; ++iy)
	{
		float x = xb;
		for(int ix = ixb; ix<ix_lim; ++ix)
		{
			float rx = x-q->pos.x;
			float ry = y-q->pos.y;
			float r = rx*rx + ry*ry;
			if (r>=r1s && r<=r2s)
			{
				float r1r;
				if (r>=sr) r1r = r/sr;
				else r1r = sr/r;
				float k = 1 - r1r;
				WindNode &nd = GetValidNode(index);
				nd.vel.x -= q->maxz*rx*k;
				nd.vel.y -= q->maxz*ry*k;
		 		nd.maxz  = max(nd.maxz, 500/r1r);
			}
			x += fng;
			index++;
//			++nd;
		}			
		y += fng;
		index+=nd_add;
//		nd += nd_add;
	}
}
//------------------------------------------------------------------------------------------

void cMapWind::ClearStaticWind(const Vect3f& pos, float radius)
{
	int ixb = max(round(pos.x - radius)>>shift, 0);
	int iy = max(round(pos.y - radius)>>shift, 0);
	int ix_lim = min(round(pos.x + radius)>>shift, x_count);
	int iy_lim = min(round(pos.y + radius)>>shift, y_count);

	float y = iy<<shift;
	float xb = ixb<<shift;
	WindNode* nd = background + (ixb + iy*x_count);
	UINT nd_add = x_count - (ix_lim - ixb);
	float rp2 = sqr(radius);
	for(;iy<iy_lim; ++iy)
	{
		float x = xb;
		for(int ix = ixb; ix<ix_lim; ++ix)
		{
			float rx = x-pos.x;
			float ry = y-pos.y;
			float r1;
			if ((r1=rx*rx + ry*ry) <= rp2 )
			{
				nd->vel.x = 0;
				nd->vel.y = 0;
				nd->maxz  = 0;
			}
			x+=fng;
			++nd;
		}			
		y+=fng;
		nd += nd_add;
	}
}
void cMapWind::SetStaticL(cQuantumWindL* ql)
{
	int ixb = max(round(ql->pos.x - ql->r)>>shift, 0);
	int iy = max(round(ql->pos.y - ql->r)>>shift, 0);
	int ix_lim = min(round(ql->pos.x + ql->r)>>shift, x_count);
	int iy_lim = min(round(ql->pos.y + ql->r)>>shift, y_count);

	float y = iy<<shift;
	float xb = ixb<<shift;
	WindNode* nd = background + (ixb + iy*x_count);
	UINT nd_add = x_count - (ix_lim - ixb);
	float rp2 = ql->r*ql->r;
	for(;iy<iy_lim; ++iy)
	{
		float x = xb;
		for(int ix = ixb; ix<ix_lim; ++ix)
		{
			float rx = x-ql->pos.x;
			float ry = y-ql->pos.y;
			float r1;
			if ((r1=rx*rx + ry*ry) <= rp2 )
			{
				float r1r;
				float k;
				nd->vel.x += ql->vel_wind.x*(k=1 - (r1r=r1/rp2));
				nd->vel.y += ql->vel_wind.y*k;
				nd->maxz  = ql->maxz;
			}
			x+=fng;
			++nd;
		}			
		y+=fng;
		nd += nd_add;
	}
}
/*void cMapWind::DeleteStaticQuant(int id)
{
	vector<cQuantumWind*>::iterator it = find(static_quants.begin(), static_quants.end(), (cQuantumWind*)id);
	if (it!=static_quants.end())
	{
		if ((*it)->TypeId() == LINEAR_WIND)
		{
			cQuantumWindL* q = (cQuantumWindL*)*it;
			q->vel_wind = -q->vel_wind;
			SetL(background, q);
		}else
		if ((*it)->TypeId() == TWISTING_WIND)
		{
			cQuantumWindW* q = (cQuantumWindW*)*it;
			q->w = -q->w;
			SetW(background, q);
		}
		delete *it;
		static_quants.erase(it);
	}
}

void cMapWind::DiscardStaticQuant(int id)
{
	vector<cQuantumWind*>::iterator it = find(static_quants.begin(), static_quants.end(), (cQuantumWind*)id);
	if (it!=static_quants.end())
	{
		delete *it;
		static_quants.erase(it);
	}
}
*/
//------------------------------------------------------------------------------------------
void cMapWind::Register(cObject3dx* obj)
{
	xassert(obj);
	vector<cStaticNode>::iterator it;
	bool push = false;
	FOR_EACH(obj->GetStatic()->nodes, it)
		if (it->name.size()>=5&&memcmp(it->name.begin(),"wind",4)==0)
		{
			push = true;
			break;
		}
	if (push)
	{
		objects.push_back(ObjNodes());
		objects.back().Set(obj);
	}
}
//------------------------------------------------------------------------------------------
void cMapWind::UnRegister(cObject3dx* obj)
{
	vector<ObjNodes>::iterator it;
	FOR_EACH(objects, it)
		if (it->obj==obj)
		{
			objects.erase(it);
			break;
		}
}

//------------------------------------------------------------------------------------------
void cMapWind::Register(UnitEnvironmentSimple* unit_environment)
{
	xassert(unit_environment);
	vector<string>::iterator it;
	bool push = false;
	
	FOR_EACH(unit_environment->modelSimple()->GetStatic()->node_name, it)
	{
		if (it->size()>=5&&memcmp(it->begin(),"wind",4)==0)
		{
			push = true;
			break;
		}
	}
	if (push)
	{
		unit_environment_objects.push_back(unit_environment);
	}
}
//------------------------------------------------------------------------------------------
void cMapWind::UnRegister(UnitEnvironmentSimple* unit_environment)
{
	vector<UnitEnvironmentSimple*>::iterator it;
	FOR_EACH(unit_environment_objects, it)
		if ((*it)==unit_environment)
		{
			unit_environment_objects.erase(it);
			break;
		}
}
//------------------------------------------------------------------------------------------
void cMapWind::GetSpeed(BYTE* data,int pitch,float mul)
{
	Vect2i size=GetGridSize();
	BYTE* curdata=data;
	WindNode* nd = nds;
	for(int y=0;y<size.y;y++)
	{
		for(int x=0;x<size.x;x++)
		{
			Vect2f& vel=nd->vel;
			++nd;
			float f=fabs(vel.x)+fabs(vel.y);
			int p=round(f*mul);
			p=clamp(p,0,255);
			curdata[x]=p;
		}
		curdata+=pitch;
	}
}


void cWindArrow::Animate(float){}

cWindArrow::cWindArrow(cMapWind *pWind) : cBaseGraphObject(0)
{
	wind = pWind;
	Texture = GetTexLibrary()->GetElement("Scripts\\Resource\\Textures\\arrow.tga");
}
cWindArrow::~cWindArrow()
{
	RELEASE(Texture);
}

void cWindArrow::PreDraw(cCamera *pCamera)
{
	pCamera->Attach(SCENENODE_OBJECTSORT,this);
}

void cWindArrow::Draw(cCamera *pCamera)
{
	start_timer_auto(cMapWind, STATISTICS_GROUP_GRAPHICS);
	cInterfaceRenderDevice* rd=pCamera->GetRenderDevice();
	rd->SetNoMaterial(ALPHA_BLEND, MatXf::ID, 0, Texture);
	cQuadBuffer<sVertexXYZDT1>* pBuf=rd->GetQuadBufferXYZDT1();
	pBuf->BeginDraw();
	sColor4f color(255,255,255);
	Vect2i size = wind->GetGridSize();
	int x_count = wind->GetGridSize().x;
	const int shift = wind->GetGridShift();
	for(int y=0;y<size.y;y++)
	for(int x=0;x<size.x;x++)
	{
		const WindNode& node = wind->GetNode(x + y*x_count);
		float av = node.vel.norm();
		if (av>1e-5f)
		{
			int wx = x<<shift;
			int wy = y<<shift;
			float z = vMap.getVoxelW(wx, wy) + node.maxz;
			Vect3f pos(wx, wy, z);
			Vect3f sy(node.vel.x, node.vel.y, 0);
			FastNormalize(sy);
			Vect3f sx(-sy.y, sy.x, 0);
			static float ky = -0.1f;
			static float kx = 15;
			sy*=av*ky;
			sx*= kx;

			sVertexXYZDT1 *v=pBuf->Get();
			v[0].pos=pos-sx-sy; v[0].diffuse=color; 
			v[1].pos=pos-sx+sy; v[1].diffuse=color; 
			v[2].pos=pos+sx-sy; v[2].diffuse=color; 
			v[3].pos=pos+sx+sy; v[3].diffuse=color; 

			v[0].GetTexel().set(0, 0);	
			v[1].GetTexel().set(0, 1);
			v[2].GetTexel().set(1, 0);	
			v[3].GetTexel().set(1, 1);
		}
	}
	pBuf->EndDraw();
}

void cMapWind::updateWindQuantum(SourceWind* source)
{
	Quants::iterator q;
	FOR_EACH(quants, q) {
		cQuantumWind* qu = *q;
		if(qu->owner && qu->owner == source) {
			if(qu->owner)
				qu->owner->noQuantumAttached_ = true;

			delete qu;
			q = quants.erase(q);
			--q;
		}
	}
}
