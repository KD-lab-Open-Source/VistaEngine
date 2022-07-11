#ifndef __WATER_GARBAGE_H_INCLUDED__
#define __WATER_GARBAGE_H_INCLUDED__

#include "Water.h"
#include "Serialization\Serialization.h"

class cWaterBubbleCenter
{
protected:
	float generate_interval;
	float generate_sum;
	Vect2f position;
	float radius;
	bool active_;
	friend class cWaterBubble;
	void* slot;

	cWaterBubble* parent;

	float min_life_time,max_life_time;
	string texture_name;
	bool delete_on_up;
public:
	cWaterBubbleCenter(const cWaterBubbleCenter& original);
	cWaterBubbleCenter(int x,int y,int radius,float speed,const char* textureName);

	cWaterBubbleCenter();
	~cWaterBubbleCenter();

	void	SetPos(const Vect2f& pos) {position = pos;}
	Vect2f	GetPos() {return position;}

	float	GetSpeed() {return 1.0f/generate_interval;}
	void	SetSpeed(float s);

	float	GetRadius() {return radius;}
	void	SetRadius(float r) {radius = r;}

	float	GetMinLifeTime(){return min_life_time;};
	float	GetMaxLifeTime(){return max_life_time;};
	void	SetLifeTime(float mint,float maxt){min_life_time=mint;max_life_time=maxt;}
	
	void	SetTextureName(const char* texture_name);
	const char* GetTextureName(){return texture_name.c_str();}

	bool&	DeleteOnUp() {return delete_on_up;}

	virtual void serialize(Archive& ar);

	bool& active() {return active_;}
	cWaterBubble* GetParent() const{ return parent; }
};

class cWaterBubble : public BaseGraphObject
{
public:
	typedef vector<PointerWrapper<cWaterBubbleCenter> >Centers;

	cWaterBubble(class cWater* water);
	~cWaterBubble();

	void PreDraw(Camera* camera);
	void Animate(float dt);
	void Draw(Camera* camera);

	//speed particle/sec
	//return - возвращает указатель объекта
	cWaterBubbleCenter* AddCenter(int x,int y,int radius,float speed,cTexture* pTexture);//Удалять функцией delete
	cWaterBubbleCenter* AddCenter(int x,int y,int radius,float speed,const char* textureName);//Удалять функцией delete
	void AddCenter(cWaterBubbleCenter* center);
	void DeleteCenter(cWaterBubbleCenter* center);

	void DeleteNearCenter(int x,int y);//Для отладки функция

	Centers& GetAllCenters(){return centers;}
	void serialize(Archive& ar);

protected:
	void DrawCoastSpriteAsBubble(Camera* camera);
	void DrawCoastSpriteAsWave(Camera* camera);
	cWater* pWater;

	struct OneObject
	{
		Vect3f pos;
		float life_time;
		float size;
		bool delete_on_up;
	};

	struct ObjectsTextured
	{
		cTexture* pTexture;
		BackVector<OneObject> objects;
	};

	vector<ObjectsTextured*> textured;
	Centers centers;
	
	inline ObjectsTextured* GetSlot(cWaterBubbleCenter& c){return (ObjectsTextured*)c.slot;}

	void AnimateCenter(float dt);
	void AnimateParticle(float dt);
	
	friend class cWaterBubbleCenter;
};


#endif
