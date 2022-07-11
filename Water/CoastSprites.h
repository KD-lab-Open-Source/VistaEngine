#ifndef __COAST_SPRITES_H_INCLUDED__
#define __COAST_SPRITES_H_INCLUDED__

#include "..\Environment\EnvironmentColors.h"
enum CoastSpritesMode
{
	CSM_NOTHING = 0,
	CSM_MOVING = 1, 
	CSM_SIMPLE = 2,
};

struct SpriteCenter
{
	float generate_sum_static;
	float generate_sum_moving;
	int x, y;
	float z;
};

struct SpriteCenterContainer
{
	SpriteCenter* simpleCenter;
	SpriteCenter* moveCenter;
};



class cCoastSprites : public cBaseGraphObject
{
	SpriteCenterContainer* grid_center;
	float z_int_to_float;
	int simple_coast_minz;
	float simple_coast_dz;
	float simple_min_interval;
	float simple_sub_interval;
	float speed;

	int move_coast_minz;
	float move_coast_dz;
	float move_min_interval;
	float move_sub_interval;

	float simple_scale_time;
	float move_scale_time;

	CoastSpritesMode mode;
	float dt;
	struct CoastSprite
	{
		Vect3f pos;
		float size;
		int phase;
		int cx;
		int cy;
	};
	struct MovingCoastSprite
	{
		Vect2f dir;
		Vect3f pos;
		int phase;
		float size;
		float speed_x;
		float speed_y;
		int cx;
		int cy;
	};
	cWater* pWater;
	cTemperature* pTemperature;
	cTexture* Texture_stay;
	cTexture* Texture_mov;
	typedef list<SpriteCenter> ContainerCenters;
	ContainerCenters simple_coast_sprite_centers;
	ContainerCenters move_coast_sprite_centers;
	BackVector<CoastSprite> coast_sprites;
	BackVector<MovingCoastSprite> mov_coast_sprites;
	void AnimateSpriteCenters(float dt);
	bool IsIce(int x, int y);
	sColor4c GetDiffuseColor(cCamera *pCamera);

	string buble_spume_texture_name;
	float simple_coast_intensity1;
	float simple_coast_intensity2;
	float move_coast_intensity1;
	float move_coast_intensity2;
	float add_z;
	float simpleMinSize;
	float simpleMaxSize;
	float moveMinSize;
	float moveMaxSize;
	float simpleDeltaPos;
	float moveDeltaPos;
	Vect2i grid_size;
	float moveAnimationTime;
	float simpleAnimationTime;
	bool dieInCoast;
	
	string tex_name_stay;
	string tex_name_mov;
	cCamera *pSaveToAnimateCamera;
public:
	cCoastSprites(cWater* pWater, cTemperature* pTemperature);
	~cCoastSprites();
	//void serialize(Archive& ar);
	void Init(const CoastSpritesAttributes& attributes);
	void Draw(cCamera *pCamera);
	void Animate(float dt);
	void DrawSimpleCoastSprite(cCamera *pCamera);
	void DrawMovingCoastSprite(cCamera *pCamera);

	int GetMovingCoastSpritesCount(){return mov_coast_sprites.size();};
	int GetSimpleCoastSpritesCount(){return coast_sprites.size();};

	CoastSpritesMode GetMode();
	void SetMode(CoastSpritesMode m);
	void SetSpriteParameters(const CoastSpriteSimpleAttributes* attributes, bool simple = true);
	void DeleteSpriteCenter(SpriteCenter* sprite_center,ContainerCenters &container);
	SpriteCenter* AddSpriteCenter(int x, int y,ContainerCenters &container);

	void PreDraw(cCamera *pCamera);
	virtual const MatXf& GetPosition() const {return MatXf::ID;}
	virtual int GetSpecialSortIndex()const{return 0;}
protected:
	void SoftClampGenerate(int num_sprites,float dt,float animation_time,float& soft_clamp);
	float soft_clamp_simple;
	float soft_clamp_move;
	MTSection lock;
};

#endif
