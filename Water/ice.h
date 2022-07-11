#ifndef __ICE_H_INCLUDED__
#define __ICE_H_INCLUDED__
#include "water.h"

/* Позволяет инкрементально сортировать индексные тайлы так, чтобы тайлы одного типа были в одной непрерывной области. 
   Вначале все тайлы в нулевом кластере.
*/
class cClusterIndexBuffer
{
public:
	cClusterIndexBuffer();
	~cClusterIndexBuffer();

	bool Init(int num_tiles);

	void LockSetTile();
	void UnlockSetTile();
	void SetTile(int idx_tile,bool idx_cluster);

	sPtrIndexBuffer& GetIB(){return ib;};
	int GetNumTiles(){return num_tiles;}
	int GetNumPolygon(){return num_tiles*2;}

	int GetPolygon0(){return end_cluster*2;};
	int GetPolygon1(){return (num_tiles-end_cluster)*2;};
protected:
	sPtrIndexBuffer ib;
	struct sTile
	{
		sPolygon p[2];
	};
	int num_tiles;
	sTile* data;
	int end_cluster;

	struct Realloc
	{
		int logic_to_physic;
		int physic_to_logic;
		bool cluster;//fot logic index
	};
	vector<Realloc> realloc_table;

	struct DeferredSetTile
	{
		int idx_tile;
		bool idx_cluster;
	};
	vector<DeferredSetTile> deffered_set;

	void CheckLink(int logic_idx);
};

typedef void (*cIceChangeTile)(int tilex,int tiley);

class cWater;
class cTemperature:public cBaseGraphObject
{
public:
	cTemperature();
	~cTemperature();
	void Init(Vect2i realsize,int shift,cWater* pWater);
	void InitGrid();
	void SetTexture(const char* snow,const char* bump,const char* cleft);
	void SetOutIce(bool enable);
	float Get(int x,int y) const;
	Vect2f GetGradient(int x,int y);
	void LogicQuant();
	void SetT(int x,int y,float t,int radius);

	void DebugShow();
	void DebugShow3(cCamera* camera);

	virtual const MatXf& GetPosition() const {return MatXf::ID;};
	void PreDraw(cCamera *pCamera);
	void Draw(cCamera *pCamera);
	void Animate(float dt);

	bool checkTile(int x,int y) const
	{
		xassert(x>=0 && x<grid_size.x);
		xassert(y>=0 && y<grid_size.y);
		return data[x+y*grid_size.x] >= border_real;
	}

	bool checkTilePrev(int x,int y) const
	{
		xassert(x>=0 && x<grid_size.x);
		xassert(y>=0 && y<grid_size.y);
		return data_new[x+y*grid_size.x] >= border_real;
	}

	bool checkTileWorld(int x,int y) const
	{//!!!Тормозная очень функция!!!! Додуматься же делить на float.
		x-=cell_size/2;y-=cell_size/2;
		x=clamp(x,0,real_size.x-1);
		y=clamp(y,0,real_size.y-1);
		float cx=(x&grid_and)/float(cell_size);
		float cy=(y&grid_and)/float(cell_size);
		int xi=x >> grid_shift;
		int yi=y >> grid_shift;
		if(xi<0)
		{
			xi=0;
			cx=0;
		}
		if(xi>=grid_size.x-1)
		{
			xi=grid_size.x-2;
			cx=1;
		}

		if(yi<0)
		{
			yi=0;
			cy=0;
		}
		if(yi>=grid_size.y-1)
		{
			yi=grid_size.y-2;
			cy=1;
		}

		const float* z00=&data[xi+yi*grid_size.x];
		const float* z01=z00+1;
		const float* z10=z00+grid_size.x;
		const float* z11=z10+1;

		return bilinear(*z00,*z01,
						*z10,*z11,
						cx,cy) >= border_real;
	}

	bool isOnIce(const Vect3f& pos, float radius = 0.f) const;

	int gridShift() const { return grid_shift; }
	void SetChangeTile(cIceChangeTile change_function_){change_function=change_function_;};

	void serialize(Archive& ar);
protected:
	SAMPLER_DATA sampler_border;
	
	cIceChangeTile change_function;
	
	class VSWaterIce* vsShader;
	class PSWaterIce* psShader;
	float *data,*data_new;
	int grid_shift;
	int grid_and;
	int cell_size;
	Vect2i real_size;
	Vect2i grid_size;
	float diffusion;
	float out_temperature;
	float border;
	BYTE alpha_ref;
	float border_real;

	cTexture* pTexture;
	cTexture* pBump;
	cTexture* pAlpha;
	cTexture* pCleft;
	cWater* pWater;

	float dget(int x,int y)
	{
		//xassert(x>=0 && x<grid_size.x);
		//xassert(y>=0 && y<grid_size.y);
		return data[x+y*grid_size.x];
	}

	float dgetb(int x,int y)
	{
		if(x>=0 && x<grid_size.x &&
			y>=0 && y<grid_size.y)
			return data[x+y*grid_size.x];
		return out_temperature;
	}

	void CalcOne(int x,int y);
	void CalcOneBorder(int x,int y);

	void TestChange(int x,int y);
	void Change(int x,int y,bool ice);
	void UpdateColor();
};

#endif
