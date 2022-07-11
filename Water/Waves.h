#ifndef __WAVES_H_INCLUDED__
#define __WAVES_H_INCLUDED__
#include "Handle.h"
#include "Serialization\Serialization.h"
#include "Render\src\NParticle.h"
 
class cEffect;
class cWater;
class cTemperature;
enum
{
	W_water = 255,
	W_earth = 254,
	W_unk = 250,
};
struct ContourPt
{
	Vect2i pos;
	Vect2f dir;
};

class cWaves : public BaseGraphObject
{
public:
	EffectKey* pena;
	cWater* pWater;
	cTexture* Texture;
	float wide;
	struct WaveLine
	{
		Vect3f rect[4];
	};
	struct BORDER
	{
		Vect2i z0, z1;
		bool nz1;
		bool passed;
		BORDER():nz1(true){}
	};
	typedef list<BORDER>::iterator VBRT;
	struct GRID_DATA
	{
		UCHAR mode;
		BORDER* bord;
		operator UCHAR(){return mode;}
		void operator =(UCHAR m){ mode = m;}
		GRID_DATA():mode(W_earth), bord(0){}
	};

	list<BORDER> bord;
//	vector< list<WAVE> > points;
	float dt;
	void SetPoint(int x,int y, Vect2i& dir);
	bool FindPoint(Vect2i p);
	bool IsBorder(int x, int y);
	void AddIsland(Vect2i p);
	bool FindBord(Vect2i v);
	GRID_DATA* gran;
	Vect2i grid;
	Vect2i ix[8];
	void CalcBorder();
	vector<BORDER*> crotch;
	cWaves::BORDER* cWaves::NextPoint(cWaves::BORDER* last, bool& again);
	void cWaves::CalcContour(vector<ContourPt>& contour, vector<WaveLine>& front);
	int bord_count;
	float F_ebb;
	struct FRONT
	{
		vector<WaveLine> waves;
		float F;
		float k1, k2;
		float max_k2;
	};
	vector<FRONT> fronts;
	float T;
	float Fmax;
	bool active;
	cScene* pScene;

public:	   
	void CreateFrontWave(vector<WaveLine>& front);
	void CreateWave();
	cWaves();
	~cWaves();
	void SetTexture(const char* name);
	void cWaves::Init(cWater* pWater, cScene* scene);
	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
	void Animate(float dt);
	void serialize (Archive& ar);
};

class cFixedWavesContainer;
// Класс источник волн
class cFixedWaves
{
	struct OneWave
	{
		Vect3f rect[4];
		Vect3f dir;
		float phase;
		bool coast;
		float speed;
		Vect3f pos;
		float distance;
		Vect2f size;
		Vect2f scale;
		float texturePhase;
	};

	struct OneSegment
	{
		float lenght;
		float lenght2;
		Vect3f pDir;
		Vect3f dest;
		Vect3f begin;
		Vect3f end;
		Vect3f p1,p2;
		float generationCount;
		Vect3f T1,T2;
	};
	Vect2f waveSize_;
	vector<Vect3f> points_;
	vector<OneSegment> segments_;
	int selectedPoint_;
	float dt;
	BackVector<OneWave> waves_;
	cFixedWavesContainer* parent_;
	string name_;
	string textureName_;
	cTexture* texture_;
	float generationTime_;
	float distance_;
	float speed_;
	int invert_;
	float time_;
	float scaleMin_;
	float scaleMax_;
	float sizeMin_;
	float sizeMax_;
	float textureAnimationTime_;

	KeysFloat speedKeys_;
	KeysFloat sizeXKeys_;
	KeysFloat sizeYKeys_;
	KeysFloat sizeYKeysCoast_;
	KeysFloat alphaKeys_;



public:
	cFixedWaves();
	~cFixedWaves();
	
	void ShowInfo(Color4c& color = Color4c(128,128,255));
	int SelectPoint(const Vect3f &point);
	void SelectPoint(int pnt);

	void SetTexture(string textureName);
	string& GetTextureName() {return textureName_;}

	void CreateSegments();
	void AddPoint(const Vect3f &point);
	void SetPoint(int num, const Vect3f& point);
	void SetPoint(const Vect3f& point);
	void DeletePoint(int num);
	void DeletePoint();
	void Rotate(Mat3f &mat);
	void SetParent(cFixedWavesContainer* parent);

	float& distance() {return distance_;}
	float& speed() {return speed_;}
	int& invertation() {return invert_;}
	float& generationTime() {return generationTime_;}
	float& scaleMin() {return scaleMin_;}
	float& scaleMax() {return scaleMax_;}
	float& sizeMin() {return sizeMin_;}
	float& sizeMax() {return sizeMax_;}

	string& name() {return name_;}
	void serialize(Archive& ar);
	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
	void Animate(float dt);
protected:
	void CalcWaveLine();

};

class cFixedWavesContainer : public BaseGraphObject
{
	typedef vector<PointerWrapper<cFixedWaves> > ListWaves;
	ListWaves listWaves_;
	int selected_;
	cWater* water_;
	cTemperature* temperature_;
	friend cFixedWaves;
public:
	cFixedWavesContainer(cWater* water, cTemperature* temperature);
	~cFixedWavesContainer();

	cFixedWaves* AddWaves();
	bool DeleteWaves(cFixedWaves* waves);

	void ShowInfo();
	bool IsIce(int x,int y);
	bool IsWater(Vect3f& vec);

	cFixedWaves* Select(int num);
	cWater* GetWater() {return water_;}
	int GetCount() {return listWaves_.size();}
	cFixedWaves* GetWave(int num);

	void serialize(Archive& ar);

	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
	void Animate(float dt);
};

#endif
