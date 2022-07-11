#ifndef __WATER_H_INCLUDED__
#define __WATER_H_INCLUDED__

#include "Terra\UpdateMapClient.h"
#include "Render\inc\IVisGenericInternal.h"
#include "Render\inc\IRenderDevice.h"
#include "Render\src\NParticle.h"
#include "XMath\KeysBase.h"

class cWaterSpeedInterface
{
public:
	virtual int GetTriggerSpeed()=0;
	virtual void AddSpeedWater(int x,int y,int speed_x,int speed_y)=0;//¬ызываетс€ дл€ всех узлов, в которых быстро течЄт вода
	virtual void ProcessSpeedWater()=0;//ќпредел€ет, что вода ускорилась выше порога срабатывани€ либо замедлилась.
};

typedef void (*cWaterChangeTile)(int tilex,int tiley);

class cWater : public BaseGraphObject, UpdateMapClient
{
public: 
	enum {
		grid_shift = 4,
		z_shift = 16,
		z_shift_one = 24,
		grid_and = (1<<grid_shift) - 1,
		alpha_delta = 1,
		max_z=  255<<z_shift
	};

	cWater();
	~cWater();
	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
	void Animate(float dt);

	void showDebug() const;

	void AnimateLogic();

	void Init();
	Vect2f GetVelocity(int x,int y);
	float GetZ(int x,int y) const;//¬ысота воды над уровнем 0
	float GetZFast(int x,int y) const;
	Vect3f GetNormalFast(int x,int y) const;
	//¬ода мен€етс€ плавно, а земл€ резко, не надо об этом забывать, лучше плользоватьс€ GetZFast
	float GetDeepWaterFast(int x,int y) const;//¬ысота воды над землей (Ќеточна€)

	/// возвращает true если область частично или полностью под водой
	/// уровень воды сравниваетс€ с \a pos.z
	bool isUnderWater(const Vect3f& pos, float radius) const;
	bool isUnderWater(const Vect3f& pos) const;
	/// возвращает true если в области в центе или где либо вообще есть вода,
	/// удобно дл€ уточнени€ карты льда, например
	bool isWater(const Vect3f& pos, float radius = 0.f) const;

	/// "относительную" воду за воду не считаем, должно быть налито "как следует"
	bool isFullWater(const Vect3f& pos) const;

	/// возвращает минимальную глубину,
	float waterDeep(const Vect3f& pos, float radius = 0.f) const;

	void SetWaterRect(int x,int y,float z,int size);
	void AddWaterRect(int x,int y,float dz,int size);

	void updateMap(const Vect2i& pos1, const Vect2i& pos2, UpdateMapType type = UPDATEMAP_ALL);

	//SetRainConstant - Ѕольше нул€ - идЄт дождь, меньше нул€ - вода высыхает.
	//–еально дождь нельз€ чтобы шел, артефакты получаютс€.
	void SetRainConstant(float dz);
	float GetRainConstant() const;
	void SetEnvironmentWater(float z);//¬ысота воды за пределами мира
	float GetEnvironmentWater() const;
	void ShowEnvironmentWater(bool sh);
	bool IsShowEnvironmentWater(){return border.isInit();}

	float GetChangeStat(){return change_type_statistic_mid;}
	void SetSpeedInterface(cWaterSpeedInterface* p){pSpeedInterface=p;}

	enum Technique
	{
		WATER_BAD=0,
		WATER_EMPTY=1,
		WATER_REFLECTION=2,
		WATER_LAVA=3,
		WATER_LINEAR_REFLECTION=4,
	};

	void setTechnique();
	bool isLava() const { return isLava_; }
	bool waterIsIce() const { return anywhereIce; }

	float GetMinPermittedOpacity() { return 1.0f/8; }
	void SetOpacity(const KeysColor& gradient);
	const KeysColor& GetOpacity() const { return opacityGradient_; }

	int sortIndex()const{return -2;}//ѕотом на enum быть может переделать
	void SetChangeTile(cWaterChangeTile change_function_){change_function=change_function_;};

	float analyzeArea(const Vect2i& center, int radius, Vect3f& normalNonNormalized); // возвращаема€ средн€€ высота абсолютна€
	void findMinMaxInArea(const Vect2i& center, int radius, int& zMin, int& zMax); // высоты zMin, zMax относительные

	void serialize(Archive& ar);

	bool Trace(const Vect3f& begin, const Vect3f& end, Vect3f& intersecttion);

	void SetDampfK(int k);
	void UpdateTexture();
	cTexture* GetTextureMiniMap() const {return textureMiniMap_; }
	cTexture* reflectionTexture() const {return pWaterZ; }

	void SetCurReflectSkyColor(Color4c color){cur_reflect_sky_color=Color4f(color);}

	FunctorGetZ* GetFunctorZ() {return pFunctorZ;}

	void setRelativeWaterLevel( float relativeWaterLevel_ ) { relativeWaterLevel_ = relativeWaterLevel_; relativeWaterLeveInt_ = round(relativeWaterLevel_ / z_int_to_float); }
	float relativeWaterLevel() const { return relativeWaterLevel_; } 

	int GetGridSizeX() const {return grid_size.x;}
	int GetGridSizeY() const {return grid_size.y;}
	int GetCoordShift() const {return grid_shift;}
	void SetWater(int x,int y,float z);//¬о внутренних координатах
	void AddWater(int x,int y,float dz);//¬о внутренних координатах
	void GetVelocity(Vect2f& vel,int x,int y);
	Vect3f GetVelocity(const Vect3f& pos) const;

	enum TYPE
	{
		type_empty=0,
		type_filled=1,
	};
	struct OnePoint
	{
		int z;
		int map_z;
		int underground_z;
		///
		char type;//TYPE
//		BYTE alpha;
		BYTE ambient_type;
		int SZ() const { return z+map_z; }
		float realHeight() const { return SZ()*z_int_to_float; }

		void serialize(Archive& ar);
	};
	inline OnePoint& Get(int x,int y)
	{
#ifdef _DEBUG
		xassert(x>=0 && x<grid_size.x);
		xassert(y>=0 && y<grid_size.y);
#endif
		return zbuffer[x+y*grid_size.x];
	}
	inline const OnePoint& Get(int x,int y) const
	{
#ifdef _DEBUG
		xassert(x>=0 && x<grid_size.x);
		xassert(y>=0 && y<grid_size.y);
#endif
		return zbuffer[x+y*grid_size.x];
	}
	inline float GetRelativeZ(int x,int y) const
	{
#ifdef _DEBUG
		xassert(x>=0 && x<grid_size.x);
		xassert(y>=0 && y<grid_size.y);
#endif
		return zbuffer[x+y*grid_size.x].z * z_int_to_float;
	}
	inline Vect2i& GetSpeed(int x,int y) const
	{
#ifdef _DEBUG
		xassert(x>=0 && x<grid_size.x);
		xassert(y>=0 && y<grid_size.y);
#endif
		return speed_buffer[x+y*grid_size.x];
	}
	inline int GetOffset(int x,int y) const
	{
#ifdef _DEBUG
		xassert(x>=0 && x<grid_size.x);
		xassert(y>=0 && y<grid_size.y);
#endif
		return x+y*grid_size.x;
	}

	void DrawPolygons(Camera* camera);//Ѕез вс€ких изначальных сетапов
	typedef sVertexXYZD VType;

protected:
	FunctorGetZ* pFunctorZ;

	float relativeWaterLevel_;
	int relativeWaterLeveInt_;
	int minus_scale_p_div_dampf_k;

	Vect2i size;
	Vect2i grid_size;

	vector<sPtrVertexBuffer> vb;
	sPtrIndexBuffer ib;
	int number_polygon;
	int number_vertex;

	enum
	{
		visible_tile_size=8,// оличесво в ширину и высоту квадов
		visible_tile_width=visible_tile_size<<grid_shift,//Ўирина и высота в пиксел€х
	};
	void CalcVisibleLine(Camera* camera);
	struct VisibleLine
	{
		int begin_tile_x;
		int begin_tile_y;
		int num_tile;
	};
	vector<VisibleLine> visible_tile;

	Vect2i delta;
	Vect2f inv_delta;
	int dt_global;
	static float z_int_to_float;

	int z_epsilon;
	int ignoreWaterLevel_;
	int rain;
	float rainConstant_;
	int water_dampf_k_;
	int environment_water;

	int change_type_statistic;
	float change_type_statistic_mid;

	OnePoint* zbuffer;
	Vect2i* speed_buffer;
	cWaterSpeedInterface* pSpeedInterface;
	cWaterChangeTile change_function;

	class VSWater* vsShader;
	class PSWater* psShader;
	class ShaderSceneWaterLava* lavaShader_;

	string bumpTextureName_;
	string bumpTextureName1_;
	UnknownHandle<cTexture> bumpTexture_;
	UnknownHandle<cTexture> bumpTexture1_;
	float animate_time;

	void InitZBuffer();

	void OutsideWater(OnePoint& cur,Vect2i& cur_speed,bool isx);
	void UpdateVB();

	void ChangeType(int x,int y,bool include);
	void CheckType(int x,int y);

	void BorderCheck(int x,int y);

	KeysColor opacityGradient_;
	enum { opacityZMax = 255 };
	unsigned char opacityBuffer_[opacityZMax + 1];
	unsigned char opacity_sea;

	float flashIntensity_;

	void UpdateUndergroundZ(int x,int y);

	cTexture* pWaterZ;
	void CalcWaterTextures();

	cTexture* textureMiniMap_;
	cTexture* textureMiniMap2_;

	Color4f reflection_color;
	float reflection_brightnes;

	bool anywhereIce;
	bool isLava_;
	Color4f lava_color_;
	Color4f lava_color_ambient_;
	string lavaTextureName_;
	float lavaTextureScale_;
	float lavaVolumeTextureScale_;
	UnknownHandle<cTexture> lavaTexture_;

	enum {
		border_con = 32,
		border_pr_size = 256,
	};

	Color4c earth_color;
	class cEnvironmentEarth* pEnvironmentEarth;

	void InitBorder();
	void ChangeBorderZ();
	void SetVertexBorder(VType* v,int x, int y);

	vector<Vect2i> check_array;

	void DrawToZBuffer(Camera* camera);

	__forceinline void CalcColor(Color4c& color,int z,unsigned char opacity_shallow);

	Color4f cur_reflect_sky_color;

	struct BorderTile
	{
		Vect3f min;
		Vect3f max;
		sPtrVertexBuffer vertexBuffer;
		sPtrIndexBuffer indexBuffer;
	};
	struct Border
	{
		bool isInit()
		{
			for(int i=0; i<8; i++)
				if(!tiles[i].vertexBuffer.IsInit())
					return false;
			return true;
		}
		void destroy();
		void Draw(Camera* camera);
		BorderTile tiles[8];
	};
	
	Border border;
};

extern cWater* water;

class cEnvironmentEarth : public BaseGraphObject
{
	sPtrVertexBuffer earth_vb;
	sPtrIndexBuffer earth_ib;
	cTexture* Texture;
	int size_vb;
	int size_ib;
	float sur_z;
	class PSEnvironmentEarth* psEnvironmentEarth;

	Color4f color;
public:
	typedef sVertexXYZDT1 VType;
	cEnvironmentEarth(const char* texture, float height);
	~cEnvironmentEarth();
	void SetColor(Color4f color_){color=color_;}
	Color4f GetColor(){return color;}
	void SetTexture(const char* texture);
	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
};

#endif
