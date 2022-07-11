#pragma once

#include "Render\Util\Runtime3D.h"
#include "Water\Water.h"
#include "Water\SkyObject.h"
#include "Water\ice.h"
#include "Water\CircleManager.h"
#include "VistaRender\FieldOfView.h"
#include "VistaRender\Field.h"
#include "Environment\Environment.h"
#include "Terra\TerrainType.h"
#include "Render\src\FogOfWar.h"
#include "UserInterface\UI_MinimapSymbol.h"

class EasyMap : public Runtime3D
{
public:
	EasyMap();
	~EasyMap();

	bool init(int xScr,int yScr,bool bwindow);
	void finit();

	void quant();

	void CameraQuant(float dt);
	void DrawMousePos();
	void onWheel(bool wheelUp);
	void KeyDown(int key);

	Vect3f MousePos3D();

	void graphQuant(float dt);
	void logicQuant();

	void Load(const char* worldName);
	void createObject(const char* name, const Vect2f& position, bool cobject3dx, float scale = 1.f);

	void serialize(Archive& ar);

	const Camera* camera() const { return camera_; }

protected:
	double prevtime;
	Vect3f	cameraPosition_;
	Vect2f	cameraAngles_;

	bool wireframe;

	cTileMap* tileMap_;
	Environment* environment_;

	float dayTime_;
	float dayTimeSpeed_;

	FOW_HANDLE fog_cursor;

	struct Sector 
	{
		Vect2f position;
		float psi;
		float radius;
		float sector;
		int colorIndex;
		Sector();
		void serialize(Archive& ar);
		void debugDraw(Camera* camera);
	};
	typedef vector<Sector> Sectors;
	Sectors sectors_;

	struct Object
	{
		Vect2f position;
		float scale;
		string name;
		UnknownHandle<c3dx> model;
		Object();
		void serialize(Archive& ar);
	};
	typedef vector<Object> Objects;
	Objects objects_;

	CircleManager circleManager_;
	CircleManagerParam circleManagerParam_;
	struct Circle
	{
		Vect2f position;
		float radius;
		Circle();
		void serialize(Archive& ar);
	};
	typedef vector<Circle> Circles;
	Circles circles_;

	typedef vector<FieldSource> FieldSources;
	FieldSources fieldSources_;
	FieldDispatcher* fieldDispatcher_;

	BitVector<TerrainType> terrainType_;

	enum 
	{
		sound_size=4,
	};
	
	enum
	{
		CLIMATE_HOT=0,
		CLIMATE_NORMAL=1,
	} climate;

	int enviroment_water;
	vector<Vect2i> water_source;
	bool mouse_lbutton;
	bool mouse_rbutton;
	float logic_quant;
	int logic_speed;
	float logic_time;

	vector<c3dx*> object_array;

	FogOfWarMap* pFogMap;
	FOW_HANDLE fog_handle;

	bool updateRay_;
	Vect3f rayStart_;
	Vect3f rayDir_;

	void OnLButtonDown(){mouse_lbutton=true;};
	void OnLButtonUp(){mouse_lbutton=false;};
	void OnRButtonDown(){mouse_rbutton=true;};
	void OnRButtonUp(){mouse_rbutton=false;};

	bool textTest_;
	float textScale_;
	void textTest();

	int minimapTest_;
	UI_MinimapSymbol minimapSymbol_;
	cTexture* minimapBorder_;
	void minimapTest(float dt);
};

extern EasyMap* easyMap;