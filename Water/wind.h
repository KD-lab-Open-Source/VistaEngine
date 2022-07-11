#ifndef __WIND_H_INCLUDED__
#define __WIND_H_INCLUDED__
#include "Water.h"
#include "..\Environment\SourceBase.h"
#include "..\terra\terTools.h"
#include "..\Units\EnvironmentSimple.h"

class Archive;
class cMapWind;
class SourceWind;
struct WindNodeBase 
{
	float maxz;
	Vect2f vel;
};

struct WindNode : public WindNodeBase
{
	UINT quant_num;	
};

enum ListTypeQuant
{
	LINEAR_WIND,
	TWISTING_WIND,
	BLAST_WIND,
};	

class cQuantumWind
{
public:
	cEffect* ef;
	Vect2f pos;
	Vect2f vel;
	float r;
	float dr;
	float k_fading;
	float maxz;
	float surf_z;
	float add_z;
	TerToolCtrl toolser;
	SourceWind* owner;
	Vect2f last_owner_position;

	cQuantumWind()
	: ef(0)
	, owner(0)
	, last_owner_position(Vect2f::ZERO)
	{};
	~cQuantumWind();
	virtual ListTypeQuant TypeId(){return BLAST_WIND;}
	virtual void Animate(float dt);
	virtual bool IsLive(){return maxz>1e-5;}
};
class cQuantumWindL:public cQuantumWind
{
public:
	Vect2f vel_wind;
	virtual ListTypeQuant TypeId(){return LINEAR_WIND;}
	virtual void Animate(float dt);
	virtual bool IsLive(){return fabsf(vel_wind.x)+fabsf(vel_wind.y)>10;};
};
class cQuantumWindW:public cQuantumWind
{
public:
	float w;
	virtual ListTypeQuant TypeId(){return TWISTING_WIND;}
	virtual void Animate(float dt);
	virtual bool IsLive(){return w>0.1f;}
};
class cQuantumWindBlast
{
public:
};
/*struct NodeObj
{
	int ix;
	Vect3f vel;
	Vect3f dis;
	bool rotate;
	float alpha;
	Vect3f ppv;
	NodeObj(int i){ix= i; ppv =vel = dis = Vect3f::ZERO;rotate = false;alpha=1e10;}
};*/

struct ObjNodes
{
	cObject3dx* obj;
	vector<NodeObj> inds;
	ObjNodes():obj(NULL){};
	ObjNodes(cObject3dx* pObj){Set(pObj);}
	void Set(cObject3dx* pObj);
};

struct SimplyObjNodes
{
	cSimply3dx* obj;
	vector<NodeObj> inds;
	SimplyObjNodes():obj(NULL){};
	SimplyObjNodes(cSimply3dx* pObj){Set(pObj);}
	void Set(cSimply3dx* pObj);
};
struct StaticWindQuantInfo
{
	ListTypeQuant type;
	Vect2f pos;
	float r;
	float maxz;
	Vect2f vel_wind;
	float w;
};
class WindQuantInfo : public StaticWindQuantInfo
{
	EffectKey* effect;
public:
	float add_z;
	float scale;
	Vect2f vel;
	float dr;
	float k_fading;
	EffectKey* GetEffectKey(){return effect;}
	bool SetEffectName(const string& filename);
	const string& GetEffectName();
	TerToolCtrl toolser;

	WindQuantInfo()
    {
        pos.set (0.0f, 0.0f);
        vel.set (100.0f, 100.0f);
        vel_wind.set (300.0f, 300.0f);
        r = 500.0f;
        dr = 0.0f;
        w = 5.0f;
        k_fading = 0.0f;
        maxz = 50.0f;
		scale = 1;
        type = LINEAR_WIND;
		effect = NULL;
		add_z = 0;
    }
};

class SourceWind : public SourceBase
{
	friend class cMapWind;
public:
//	const QuatF& orientation() const;

	void setPose (const Se3f& pos, bool init) {
		Vect3f v;
		pos.rot().xform (Vect3f::J, v);
		
		prototype.vel.set(v.x * windVelocity_, v.y * windVelocity_);
		prototype.vel_wind = prototype.vel;
        
		prototype.pos.x = pos.trans().x;
		prototype.pos.y = pos.trans().y;
		SourceBase::setPose(pos, init);
	}

	void setRadius (float _radius) {
		prototype.r = _radius;
		SourceBase::setRadius(_radius);
	}

	float windVelocity() const;
	void setWindVelocity(float speed);

	Vect2f& GetPos(){return prototype.pos;}
	SourceWind()
	: SourceBase()
	, cur_interval(0)
	, interval(1)
	, noQuantumAttached_(true)
	, windVelocity_(100.0f)
	{
		setScanEnvironment(false);
	}
	SourceBase* clone () const {
		return new SourceWind(*this);
	}
	SourceWind(const SourceWind& original)
	: SourceBase(original)
	, cur_interval(original.cur_interval)
	, interval(original.interval)
	, effect_filename(original.effect_filename)
	, noQuantumAttached_(true)
	, windVelocity_(original.windVelocity_)
	{
		prototype = original.prototype;
		radius_ = prototype.r;
		setScanEnvironment(false);
	}

	void quant();
	void serialize(Archive& ar);

	SourceType type()const {return SOURCE_WIND;}

protected:
	void start();
	void stop();

private:
	float windVelocity_;
	float cur_interval;
	float interval;
	string effect_filename;
	bool noQuantumAttached_;
	WindQuantInfo prototype;
};
class cMapWind : public cContainerRef3dx
{
protected:
	int map_width,map_height;
	typedef vector<cQuantumWind*> Quants;
	Quants quants;
	vector<ObjNodes> objects;
	vector<SimplyObjNodes> simply_objects;
	vector<UnitEnvironmentSimple*> unit_environment_objects;

	EffectKey* trn;
	EffectKey* wind;
	int x_count;
	int y_count;
	int shift;
	int ng;
	float fng;
	WindNode* nds;
	WindNode* background;
	cScene* scene;
	UINT quant_num;
	WindNode& GetValidNode(UINT index);
	static cMapWind* wind_this;
public:
	static cMapWind* GetWind(){ return wind_this;}
	cMapWind();
	~cMapWind();
	void Init(int w, int h, cScene* scene, int n); 
	void Animate(float dt);

	void UnRegister(cObject3dx* obj);
	void Register(cObject3dx* obj);

	void Register(UnitEnvironmentSimple* unit_environment);
	void UnRegister(UnitEnvironmentSimple* unit_environment);
//	void DeleteStaticQuant(int id);
//	void DiscardStaticQuant(int id);

	SourceWind* CreateSource(const SourceWind& original);
	void CreateQuant(WindQuantInfo& dat, SourceWind* owner);
	int CreateStaticQuant(StaticWindQuantInfo& dat);

	Vect3f GetVelocity(const Vect3f& pos);
	const WindNode& GetNode(int index);
//	const WindNode& GetNode(int ix, int iy); // ix == x>>shift
	const WindNode& GetNode(const float x, const float y);
	Vect2i GetGridSize(){return Vect2i(x_count,y_count);}
	int GetGridShift(){return shift;}
	void GetSpeed(BYTE* data,int pitch,float mul);
	void serialize(Archive& ar);
	void serializeParameters(Archive& ar);
//	void PreDraw(cCamera *pCamera){};
//	void Draw(cCamera *pCamera){};
//	virtual const MatXf& GetPosition() const {return MatXf::ID;};
	void ClearStaticWind(const Vect3f& pos, float radius);

	void updateWindQuantum(SourceWind* source);
protected:
	void SetL(cQuantumWindL* q);
	void SetW(cQuantumWindW* q);
	void SetRing(cQuantumWind* q);
	void SetStaticL(cQuantumWindL* ql);
	void Clear();
	float GetZ(int x, int y);
};

class cWindArrow: public cBaseGraphObject
{
	cMapWind *wind;
	cTexture *Texture;
public:
	cWindArrow(cMapWind *wind);
	~cWindArrow();
	void PreDraw(cCamera *pCamera);
	void Draw(cCamera *pCamera);
	void Animate(float dt);
	virtual const MatXf& GetPosition() const {return MatXf::ID;}
};

#endif
