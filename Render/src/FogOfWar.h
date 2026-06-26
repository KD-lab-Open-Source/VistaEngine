#ifndef __FOG_OF_WAR_H_INCLUDED__
#define __FOG_OF_WAR_H_INCLUDED__

#include "NParticleKey.h"
#include "Render/inc/IVisGenericInternal.h"

class cCircleDraw;
class FogOfWar;

typedef int FOW_HANDLE;

enum FogOfWarState
{
	FOGST_NONE = 1,
	FOGST_HALF = 2, 
	FOGST_FULL = 4,
};

class FogOfWarMap {
public:
    FogOfWarMap(FogOfWar*const fogOfWar, const Vect2i& size);
    ~FogOfWarMap();
    void quant();

    FOW_HANDLE createVisible();
    void deleteVisible(FOW_HANDLE handle);

    void moveVisible(FOW_HANDLE handle, int x, int y, int radius);//������ �������� ����� ���������� �������

	//moveVisibleOuterRadius ���������� ������� ��� �������
	void moveVisibleOuterRadius(FOW_HANDLE handle, int x, int y, int radius);

	static int maxSightRadius();

	FogOfWarState getFogState(int x, int y) const;
	bool isVisible(const Vect2i& position) const;
	typedef BitVector<FogOfWarState> FogOfWarStates;
	bool checkFogStateInCircle(const Vect2i& centerPosition, int radius) const;

	const char* lockSummarymap() const;
	void unlockSummarymap() const;
	const char* lockTilemap() const;
	void unlockTilemap() const;
	const char* lockScoutmap() const;
	void unlockScoutmap() const;
	void clearMap();

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
	char* tilemap;//������� �� ������� ����
	char* scoutmap;//������� �� ���������� �����
	char* summarymap;//����� ��������� ��� �������
	BackVector<One> objects_;
	FogOfWar* fogOfWar_;

	mutable MTSection lock_;
};

/// ����� ����� - ��������� �������� ������ �����. ������ ��� �������� ����������� � �������� ��� ������� �������.
class FogOfWar : public BaseGraphObject
{
public:
	enum{
		shift = 5,
		step = 1 << shift,
		fade_delta_radius=2<<shift,

		maxSightRadius=32<<shift,
	};

	FogOfWar();
	~FogOfWar();

	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
	void Animate(float dt);
	int sortIndex() const { return 10; }

	void serialize(Archive& ar);

	void AnimateLogic();

	FogOfWarMap* CreateMap();
    void SelectMap(const FogOfWarMap* map);
	const FogOfWarMap* GetSelectedMap() const{ return selected_map; }

	float GetInvFogAlpha(){return invAlpha;}
	Color4c GetFogColor(){ return fogColor_; } // ���� � ������������ ������ � ������������� �������.
	int scoutAreaAlpha() const { return scoutAreaAlpha_; } // ������������� ������������ �������, ������� ����������.

	cTexture* GetTexture(){return texture_;}

	cCircleDraw* GetCircleDraw() {return circleDraw;}

protected:
    const FogOfWarMap* selected_map;

	void UpdateTexture();

	Color4c fogColor_;
	int scoutAreaAlpha_;

	Vect2i size;
	UnknownHandle<cTexture> texture_;
	UnknownHandle<cTexture> texture2_;

	cCircleDraw* circleDraw;
	float invAlpha;
	int fogMinimapAlpha_;

	void createTexture();
};

#endif
