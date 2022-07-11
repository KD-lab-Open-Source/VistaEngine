#ifndef __FALLOUT_H_INCLUDED__
#define __FALLOUT_H_INCLUDED__

//class cCamera;
#include <xmath.h>
#include "Circles.h"
#include "..\Environment\EnvironmentColors.h"

enum ModeFall
{ 
	FALLOUT_CURRENT,
	FALLOUT_NONE,
	FALLOUT_RAIN, 
	FALLOUT_SNOW, 
};

class cFallout : public cBaseGraphObject
{
	struct cDrop
	{
		enum { ndub =1 };
		struct DropDuple
		{
			Vect3f pos;
			int animate_key;
		};
		Vect3f pos0;
		Vect3f vel;
		DropDuple dub[ndub];
		int animate_key;
		bool visible;
		float rand;

		void Init(float mr,int anim_size);
	};

	Vect3f near_point8[8];
	Vect3f camera_pos;
	vector<cDrop> drops;
	int dN;
	cTextureAviScale* Texture;
	cTexture* rainTexture;
	int N;
	int max_N;
	void DrawSnow(cCamera *pCamera);
	void DrawRain(cCamera *pCamera);
	ModeFall mode;
	float size_rain;
	float size_snow;
	float dt;
	float r;
	Vect3f center;

	bool ending;
	Vect3f vel_all;
	string snow_texture;
	string rain_texture;
	void LoadSnowTexture();
	float snow_kc, rain_kc;//Относительное расстояние до ближайшей плоскости отсечения, при которой начинает дождь/снег рисоваться.
	cCircles circles;
	int minimal_water_height;

	MTSection lock;
public:
	int GetN(){return N;}
	float GetR(){return r;}
	void GetSphere(Vect3f& pos, float& radius);
	cFallout();
	~cFallout();
	void Init(float quality, cWater* pWater, cTemperature* pTemperature);
	ModeFall GetMode(){return mode;}
	Vect2f GetSidewind(){return vel_all;}
	void SetSidewind(Vect2f v);
	float GetSize(ModeFall m = FALLOUT_CURRENT);
	void SetSize(float sz, ModeFall m = FALLOUT_CURRENT);							//0..1
	float GetIntensity();
	bool SetIntensity(float intensity/*0..1*/, float time/*sec*/);
	void Set(ModeFall m,float intensity, float time);	//time in second 
	void End(float time);
	void PreDraw(cCamera *pCamera);
	void Draw(cCamera *pCamera);
	void Animate(float dt);
	void Enable(bool enable);
	virtual const MatXf& GetPosition() const {return MatXf::ID;};

	//void serialize (Archive& ar);
	//void serializeParameters (Archive& ar);
	//void serializeColors (Archive& ar);
	void SetupAttributes(FalloutAttributes& attributes);
protected:
	sColor4c rain_color;
	Vect2f rain_sw;
	Vect2f snow_sw;
	bool enable_;

	float current_intensity;
};


#endif
