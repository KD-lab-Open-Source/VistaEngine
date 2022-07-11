#ifndef __WATER_H_INCLUDED__
#define __WATER_H_INCLUDED__

#include "..\Render\inc\IVisGeneric.h"

class cWaterSpeedInterface
{
public:
	virtual int GetTriggerSpeed()=0;
	virtual void AddSpeedWater(int x,int y,int speed_x,int speed_y)=0;//¬ызываетс€ дл€ всех узлов, в которых быстро течЄт вода
	virtual void ProcessSpeedWater()=0;//ќпредел€ет, что вода ускорилась выше порога срабатывани€ либо замедлилась.
};

typedef void (*cWaterChangeTile)(int tilex,int tiley);
class cWater:public cBaseGraphObject
{
public: 
	cWater();
	~cWater();
	void PreDraw(cCamera *pCamera);
	void Draw(cCamera *pCamera);
	void Animate(float dt);

	void showDebug() const;

	void AnimateLogic();
	virtual const MatXf& GetPosition() const {return MatXf::ID;};

	void Init(cScene* pScene);
	Vect2f GetVelocity(int x,int y);
	float GetZ(int x,int y) const;//¬ысота воды над уровнем 0
	float GetZFast(int x,int y) const;
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

	void UpdateMap(const Vect2i& pos1, const Vect2i& pos2);

	//SetRainConstant - Ѕольше нул€ - идЄт дождь, меньше нул€ - вода высыхает.
	//–еально дождь нельз€ чтобы шел, артефакты получаютс€.
	void SetRainConstant(float dz);
	float GetRainConstant() const;
	void SetEnvironmentWater(float z);//¬ысота воды за пределами мира
	float GetEnvironmentWater() const;
	void ShowEnvironmentWater(bool sh);
	bool IsShowEnvironmentWater(){return border.isInit();}

	const sColor4f& GetReflectionColor(){return reflection_color;}

	void SetReflectionColor(sColor4f color){reflection_color=color;};
	void SetReflectionBrightnes(float brightnes){reflection_brightnes = brightnes;}
	float GetReflectionBrightnes(){return reflection_brightnes;}

	float GetChangeStat(){return change_type_statistic_mid;}
	void SetSpeedInterface(cWaterSpeedInterface* p){pSpeedInterface=p;}

	enum WATER_TYPE
	{
		WATER_BAD=0,
		WATER_EMPTY=1,
		WATER_REFLECTION=2,
		WATER_LAVA=3,
		WATER_LINEAR_REFLECTION=4,
	};

	void SetTechnique(WATER_TYPE set);

	inline float GetMinPermittedOpacity(){return 1.0f/8;}
	void ClampOpacity(CKeyColor& gradient);
	void SetOpacity(const CKeyColor& gradient);
	//const CKeyColor& GetOpacity() const { return opacity_gradient; }
	const CKeyColor& GetOpacity() const { return z_level; }

	int GetSpecialSortIndex()const{return -2;}//ѕотом на enum быть может переделать
	void SetChangeTile(cWaterChangeTile change_function_){change_function=change_function_;};

	float analyzeArea(const Vect2i& center, int radius, Vect3f& normalNonNormalized); // возвращаема€ средн€€ высота абсолютна€
	void findMinMaxInArea(const Vect2i& center, int radius, int& zMin, int& zMax); // высоты zMin, zMax относительные

	void serialize(Archive& ar);

	bool Trace(const Vect3f& begin, const Vect3f& end, Vect3f& intersecttion);

	int GetDampfK(){return -minus_scale_p_div_dampf_k; };
	void SetDampfK(int k);

	void UpdateTexture();
	cTexture* GetTextureMiniMap(){return pTextureMiniMap;}

	sColor4c GetEarchColor()const{return earth_color;};
	void SetEarthColor(sColor4c color){earth_color=color;};
	void SetCurReflectSkyColor(sColor4c color){cur_reflect_sky_color=sColor4f(color);}

	const sColor4f& GetLavaColor()const {return lava_color_;}
	void SetLavaColor(sColor4f color){lava_color_=color;}
	const sColor4f& GetLavaColorAmbient()const{return lava_color_ambient_;}
	void SetLavaColorAmbient(sColor4f color){lava_color_ambient_=color;}
public:
	FunctorGetZ* GetFunctorZ() {return pFunctorZ;}
	enum {
		grid_shift=4,
	};

	void setRelativeWaterLevel( float relativeWaterLevel_ ) { relativeWaterLevel = relativeWaterLevel_; relativeWaterLeveInt_ = round(relativeWaterLevel / z_int_to_float); }
	float getRelativeWaterLevel() const { return relativeWaterLevel; } 

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

	void DrawPolygons(cCamera* camera);//Ѕез вс€ких изначальных сетапов
	typedef sVertexXYZD VType;
protected:
	
	FunctorGetZ* pFunctorZ;

	float relativeWaterLevel;
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
	void CalcVisibleLine(cCamera* pCamera);
	struct VISIVLE_LINE
	{
		int begin_tile_x;
		int begin_tile_y;
		int num_tile;
	};
	vector<VISIVLE_LINE> visible_tile;

	Vect2i delta;
	Vect2f inv_delta;
	int dt_global;
	static float z_int_to_float;

	int z_epsilon;
	int ignoreWaterLevel_;
	int rain;
	int environment_water;

	int change_type_statistic;
	float change_type_statistic_mid;
public:
	enum
	{
		z_shift=16,
		z_shift_one=24,
		grid_and=(1<<grid_shift)-1,
		alpha_delta=1,
		max_z=255<<z_shift
	};
protected:
	OnePoint* zbuffer;
	Vect2i* speed_buffer;
	cWaterSpeedInterface* pSpeedInterface;
	cWaterChangeTile change_function;

	class VSWater* vsShader;
	class PSWater* psShader;
	class VSWaterLava* vsShaderLava;
	class PSWaterLava* psShaderLava;

	cTexture* pBump;
	cTexture* pBump1;
	float animate_time;

	void InitZBuffer();

	void OutsideWater(OnePoint& cur,Vect2i& cur_speed,bool isx);
	void UpdateVB(cCamera *pCamera);

	void ChangeType(int x,int y,bool include);
	void CheckType(int x,int y);

	void BorderCheck(int x,int y);

	CKeyColor opacity_gradient;
	sColor4c opacity_buffer[256];

	CKeyColor z_level;
	BYTE alpha_min;
	BYTE alpha_max;
	int zend_zero;
	int zmin;
	int zmax;
	float inv_zmin_zend_zero;//1/float(zmin-zend_zero);
	float inv_zmax_zmin;//1/float(zmax-zmin);


	unsigned char opacity_shallow, opacity_river, opacity_sea;
	float opacity_k1, opacity_k2;

	void UpdateUndergroundZ(int x,int y);

	cTexture* pWaterZ;
	void CalcWaterTextures();

	cTexture* pTextureMiniMap;

	sColor4f reflection_color;
	float reflection_brightnes;

	sColor4f lava_color_;
	sColor4f lava_color_ambient_;
protected: 
	int max_z_color;

	float zreflection_interpolated;
	float zreflection;

	enum {
		border_con = 32,
		border_pr_size = 256,
	};

	sColor4c earth_color;
	class cEnvironmentEarth* pEnvironmentEarth;

	void InitBorder();
	void ChangeColor();
	void ChangeBorderZ();
	void SetVertexBorder(VType* v,int x, int y);

	cTileMap		*terMapPoint;

	vector<Vect2i> check_array;

	void DrawToZBuffer(cCamera *pCamera);

	__forceinline void CalcColor(sColor4c& color,int z,unsigned char opacity_shallow);

	sColor4f cur_reflect_sky_color;

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
		void Draw(cCamera* camera);
		BorderTile tiles[8];
	};
	
	Border border;
};

enum Outside_Environment
{
	ENVIRONMENT_NO,
	ENVIRONMENT_CHAOS,
	ENVIRONMENT_WATER,
	ENVIRONMENT_EARTH,
};

class cEnvironmentEarth: public cBaseGraphObject
{
	sPtrVertexBuffer earth_vb;
	sPtrIndexBuffer earth_ib;
	cTexture* Texture;
	int size_vb;
	int size_ib;
	float sur_z;
	class PSEnvironmentEarth* psEnvironmentEarth;

	sColor4f color;
public:
	typedef sVertexXYZDT1 VType;
	cEnvironmentEarth(const char* texture);
	~cEnvironmentEarth();
	void SetColor(sColor4f color_){color=color_;};
	sColor4f GetColor(){return color;}
	void SetTexture(const char* texture);
	void PreDraw(cCamera *pCamera);
	void Draw(cCamera *pCamera);
	void Animate(float dt){};
	virtual const MatXf& GetPosition() const {return MatXf::ID;}
};

#endif
