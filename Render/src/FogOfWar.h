#ifndef __FOG_OF_WAR_H_INCLUDED__
#define __FOG_OF_WAR_H_INCLUDED__
#include "NParticleKey.h"
#include "..\..\Util\Serialization\SerializationTypes.h"

enum FogOfWarState
{
	FOGST_NONE = 1,
	FOGST_HALF = 2, 
	FOGST_FULL = 4,
};

typedef int FOW_HANDLE;

class cCircleDraw;
class cFogOfWar;
class FogOfWarMap{
public:
    FogOfWarMap(cFogOfWar*const fogOfWar, const Vect2i& size);
    ~FogOfWarMap();
    void quant();

    FOW_HANDLE createVisible();
    void deleteVisible(FOW_HANDLE handle);

    void moveVisible(FOW_HANDLE handle, int x, int y, int radius);//Радиус включает всебя переходную область

	//moveVisibleOuterRadius Переходная область вне радиуса
	void moveVisibleOuterRadius(FOW_HANDLE handle, int x, int y, int radius);

	static int maxSightRadius();

	FogOfWarState getFogState(int x, int y) const;
	bool isVisible(const Vect2i& position) const;
	typedef BitVector<FogOfWarState> FogOfWarStates;
	bool checkFogStateInCircle(Vect2i& centerPosition, int radius) const;

	const char* lockSummarymap() const;
	void unlockSummarymap() const;
	const char* lockTilemap() const;
	void unlockTilemap() const;
	const char* lockScoutmap() const;
	void unlockScoutmap() const;
	void ClearMap();

	void serialize(Archive& ar);
private:
	struct One{
		int x,y;
		int radius;
	};
	enum{
		max_opacity=70,//-127..+127
	};

	void calcScoutMap();
	void calcSummaryMap();
	void drawCircle(int x, int y, int radius);

	Vect2i size;
	char* raw_data;
	char* tilemap;//Видимое на текущий кадр
	char* scoutmap;//Видимое на предыдущие кадры
	char* summarymap;//Карта видимости для графики
	BackVector<One> objects_;
	cFogOfWar* fogOfWar_;

	mutable MTSection lock_;
};

/// Туман войны - заполняет текстуру тумана войны. Дальше это текстура применяется в шейдерах для каждого объекта.
class cFogOfWar:public cBaseGraphObject
{
public:
	enum{
		shift = 5,
		step = 1 << shift,
		fade_delta_radius=2<<shift,

		maxSightRadius=32<<shift,
	};

	cFogOfWar(TerraInterface* terra);
	~cFogOfWar();

	void PreDraw(cCamera *pCamera);
	void Draw(cCamera *pCamera);
	void Animate(float dt);
	virtual const MatXf& GetPosition() const{return MatXf::ID;}

	void AnimateLogic();

	FogOfWarMap* CreateMap();
    void SelectMap(const FogOfWarMap* map);
	const FogOfWarMap* GetSelectedMap() const{ return selected_map; }

	//fog_color_ - rgb - цвет тумана, a - прозрачность неразведанного тумана.
	void SetFogColor(sColor4c fog_color_)
	{
		fog_color=fog_color_;
		if (fog_color_.a>0)
			invAlpha = (float)fogMinimapAlpha_/fog_color_.a;
		else
			invAlpha = 0.f;
	}
	void SetFogMinimapAplha(int fog_alpha)
	{
		fogMinimapAlpha_ = fog_alpha;
		SetFogColor(fog_color);
	}
	float GetInvFogAlpha(){return invAlpha;}
	
	/// Цвет и прозрачность тумана в неразведанной области.
	sColor4c GetFogColor(){return fog_color;}
	
	/// Относительная прозрачность области, которая разведанна.
	void SetScoutAreaAlpha(BYTE alpha){scout_area_alpha=alpha;};
	BYTE GetScoutAreaAlpha(){return scout_area_alpha;};

	cTexture* GetTexture(){return pTextureTilemap;}

	cCircleDraw* GetCircleDraw() {return circleDraw;}

	void SetDebugShowScoutmap(bool b){show_tilemap=!b;}
	bool GetDebugShowScoutmap(){return !show_tilemap;}

	void DeleteDefaultResource();
	void RestoreDefaultResource();
protected:
	void Init(TerraInterface* terra);
    const FogOfWarMap* selected_map;

	void UpdateTexture();

	TerraInterface* terra;
	sColor4c fog_color;
	BYTE scout_area_alpha;

	Vect2i size;
	cTexture* pTextureTilemap;

	cCircleDraw* circleDraw;
	float invAlpha;
	int fogMinimapAlpha_;
	bool show_tilemap;
};

#endif
