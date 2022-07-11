#ifndef __CIRCLES_H_INCLUDED__
#define __CIRCLES_H_INCLUDED__
#include <math.h>

#include "RangedWrapper.h"
#include "..\Water\ice.h"

class cFallout;
class cCircles
{
	struct cWaterCircle
	{
		Vect3f pos;
		float phasa;
		float time;
		bool visible;
		cWaterCircle();
	};
	float z;
	list<cWaterCircle> drops;
	cWater* pWater;
	cTemperature* pTemperature;
	Vect3f center;
	Vect2i size;
	float max_size;
	float time;
	vector<cWaterCircle> cirles;
	sColor4c color;
	cTextureAviScale* Texture;
	int r2_rain;
	bool env_water_circle;
	Vect2i rain_center;
	void SetCirclePos(cWaterCircle& circle);
	Vect2f GetPtOnZ(Vect3f& p1,Vect3f& p2, sPlane4f& back);
	struct Triangle
	{
		Vect2f v[3];
		Vect2f& operator[](int i){xassert((UINT)i<3);return v[i];}
	};
	void SetCirclePos(cWaterCircle& circle, Vect2f& p);

	Vect2f trapeze_beg;
	float trapeze_beg_r;
	float trapeze_h;
	float trapeze_dr;
	Vect2f trapeze_dir_h;
	Vect2f trapeze_dir_n;
	cFallout* fallout;
	inline void RndPosInTrapeze(Vect2f& p);
	string texture_name;
public:
	void clear();
	void SetTexture(const char* texture);
	void SetSize(float size);
	void SetTime(float time);
	const char* GetTextureName();
	float GetSize();
	float GetTime();

	bool Visible(){return pWater!=NULL;}
	bool pause;
	cCircles();
	~cCircles();
	void Init(cWater* pWater, cFallout* fallout, cTemperature* pTemperature);
	void SetIntensity(float sc);
	void SetCircle(const Vect3f& dr_pos0);
	void PreDraw(cCamera *pCamera);
	void Draw(cCamera *pCamera, float dt);
	void Animate(float dt);
	virtual const MatXf& GetPosition() const {return MatXf::ID;};
};




#endif
