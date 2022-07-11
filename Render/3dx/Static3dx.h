#ifndef __STATIC3DX_H_INCLUDED__
#define __STATIC3DX_H_INCLUDED__

#include "Static3dxBase.h"
#include "VoxelBox.h"
#include "Render\Inc\IRenderDevice.h"

//  усок объекта, состо€щий из нескольких нодов, но одного материала
struct StaticBunch
{
	int	offset_polygon;
	int	num_polygon;

	int	offset_vertex;
	int	num_vertex;

	int	imaterial;

	enum
	{
		max_index=20,//ћаксимальное количество матриц
	};
	
	vector<int> nodeIndices; // с какими матрицами св€занны текущие полигоны, не более max_index
	TempVisibleGroups visibleGroups;

	void serialize(Archive& ar);
};
typedef vector<StaticBunch> StaticBunches;

struct cSkinVertexSysMem
{
	Vect3f pos;
	Color4c index;
	BYTE weight[4];
};

class cSkinVertexSysMemI
{
public:
	cSkinVertexSysMem* begin;
	cSkinVertexSysMem* cur;
public:
	cSkinVertexSysMemI():cur(0),begin(0){};
	void SetVB(cSkinVertexSysMem* p_){begin=p_;}
	void Select(int n){cur=begin+n;}
	Vect3f& GetPos(){return cur->pos;}
	BYTE& GetWeight(int idx){return cur->weight[idx];};
	Color4c& GetIndex(){return cur->index;}
};

class RENDER_API cStatic3dx : public UnknownClass, public Static3dxBase
{
public:
	cStatic3dx(bool is_logic_, const char* fname);
	~cStatic3dx();
	int Release();

	bool load(const char* fileName);
	void serialize(Archive& ar);
	void constructInPlace(const char* fileName);
	void saveInPlace(const char* fileName);

	class cVisError& errlog();

	cTexture* LoadTexture(const char* name,char* mode=0);//“о же что и GetTexLibrary()->GetElement, но с более развернутым сообщением об ошибке.
	void GetTextureNames(TextureNames& names) const;
	string fixTextureName(const char* name) const;

	struct LodCache
	{
		int polygonNumber;
		int vertexNumber;
		int vertexSize;

		MemoryBlock ibBlock;
		MemoryBlock vbBlock;

		LodCache();
		void serialize(Archive& ar);
	};

	struct LodsCache
	{
		vector<LodCache> lods;
		LodCache debris;
		void serialize(Archive& ar);
	};

	struct StaticLod
	{
		sPtrIndexBuffer		ib;
		sPtrVertexBuffer	vb;
		int					blend_indices;//количество костей в vb
		StaticBunches bunches;

		cSkinVertexSysMem* sys_vb;
		sPolygon* sys_ib;

		StaticLod();
		void serialize(Archive& ar);
		void prepareToSerialize(LodCache& chache);
		void initBuffersInPlace(const LodCache& cache,cStatic3dx* object);
		int GetBlendWeight() { return blend_indices == 1 ? 0 : blend_indices; }
	};

	typedef vector<StaticLod> Lods;
	Lods lods;//1 или 3 лода.

	StaticLod	debris;
	typedef vector<cStaticSimply3dx*> Debrises;
	Debrises debrises;

	VoxelBox voxelBox;
	
	void CacheBuffersToSystemMem(int ilod);
	void GetVBSize(int& vertex_count,int& vertex_size);

	cSkinVertex GetSkinVertex(int num_weight){return cSkinVertex(num_weight,bump,isUV2,enableFur);}

private:
	bool inPlace_;

	void BuildMeshes();
	void BuildMeshesLod(const TempMeshes& tempMeshesIn, int ilod,bool debris);

	void BuildBuffers(TempMeshes& temp_mesh,StaticLod& lod,int max_bones);
	void DummyVisibilitySet();
	void prepareMesh();
	void PrepareIndices();
	void ParseEffect();
	void loadFur();
	void createTextures();
	void releaseTextures();

	void MergeMaterialMesh(TempMeshes& temp_mesh,TempMeshes& material_mesh,bool merge_mesh);

	void GetVisibleMesh(const string& mesh_name,int& visible_set,DWORD& visible, bool& is_debris,bool& is_no_debris);

	void BuildSkinGroupSortedNormal(TempMeshes& temp_mesh,TempMeshes& out_mesh);
	void ExtractMesh(TempMesh mesh,TempMesh& extracted_mesh,
					  vector<char>& selected_polygon,vector<int>& node_index);

	void CalcBumpSTNorm(StaticLod& lod);//Ќормаль равна SxT вектору.

	void CreateDebrises();
};

struct Shader3dx
{
	class VSSkin* vsSkinSceneShadow;
	class PSSkin* psSkinSceneShadow;
	class VSSkin* vsSkinBumpSceneShadow;
	class PSSkin* psSkinBumpSceneShadow;

	class VSSkin* vsSkin;
	class PSSkinNoShadow* psSkin;
	
	class VSSkin* vsSkinBump;
	class PSSkin* psSkinBump;
	class VSSkinNoLight* vsSkinNoLight;

	class VSSkinShadow* vsSkinShadow;
	class PSSkinShadow* psSkinShadow;
	class PSSkinShadowAlpha* psSkinShadowAlpha;

	class VSSkin* vsSkinReflectionSceneShadow;
	class PSSkin* psSkinReflectionSceneShadow;
	class VSSkin* vsSkinReflection;
	class PSSkinNoShadow* psSkinReflection;

	class VSSkinSecondOpacity* vsSkinSecondOpacity;
	class PSSkinSecondOpacity* psSkinSecondOpacity;

	class VSSkinZBuffer* vsSkinZBuffer;
	class PSSkinZBuffer* psSkinZBuffer;
	class PSSkinZBufferAlpha* psSkinZBufferAlpha;
	
	class VSSkinFur* vsSkinFur;

	Shader3dx();
	~Shader3dx();
};

extern Shader3dx* pShader3dx;

#endif
