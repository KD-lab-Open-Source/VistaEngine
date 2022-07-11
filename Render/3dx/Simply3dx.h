#ifndef __SIMPLY_3DX_H_INCLUDED__
#define __SIMPLY_3DX_H_INCLUDED__

#include "Render\Src\observer.h"
#include "Render\Inc\IVisGenericInternal.h"
#include "Render\Inc\IRenderDevice.h"
#include "Render\Src\Texture.h"

class cStatic3dx;
class cStaticSimply3dx;
struct VisibilityGroupIndex;

class RENDER_API cSimply3dx : public c3dx
{
public:
	/*
	Не лежит в общем списке, поэтому PreDraw и Animate не вызываются.
	Draw - вызывается только для полупрозрачных объектов, для остальных -
	установка матриц и материала, а Draw глобальный.
	*/
	cSimply3dx(cStaticSimply3dx* pStatic);
	~cSimply3dx();

	void PreDraw(Camera* camera);
	void Draw(Camera* camera);

	void SetCircleShadowParam(float radius,float height=-1);
	void SetShadowType(ObjectShadowType type);
	ObjectShadowType getShadowType();
	void getCircleShadowParam(float& radius, float& height);

	//Прозрачность объекта, остальные параметры не меняются.
	void SetOpacity(float opacity);
	float GetOpacity()const{return opacity;};

	void SetPosition(const MatXf& Matrix);
	void SetPosition(const Se3f& pos);
	const MatXf& GetPosition() const { return positionX_; }
	const Mats& GetPositionMats() const { return position; }
	const Se3f& GetPositionSe() const { return position.se(); }

	void SetScale(float scale);//Вызывать обязательнро до SetPosition
	float GetScale()const;

	cStaticSimply3dx* GetStatic(){return pStatic;}
	float GetBoundRadius() const;
	void GetBoundBox(sBox6f& box_) const;
	void GetBoundBoxUnscaled(sBox6f& box_);

	int FindNode(const char* node_name) const;// (-1=не нашли)
	int GetNodeNum() {return node_position.size();};
	const MatXf& GetNodePosition(int nodeindex) const;//Положение ноды в глобальном пространстве.

	//SetNodePosition - Положение ноды выстапляется пользователем, при этом оно не апдейтится в SetPosition
	void SetNodePosition(int nodeindex,const MatXf& pos);
	void SetNodePosition(int nodeindex,const Se3f& pos);
	void SetNodePositionMats(int nodeindex,const Mats& pos);
	void UseDefaultNodePosition(int nodeindex);

	//Возвращает смещение ноды относительно начала координат.
	const MatXf& GetNodeInitialOffset(int nodeindex) const;

	void clearAttribute(int attribute);
	const Vect3f& GetCenterObjectInline() { return position.trans(); }
	__forceinline float GetBoundRadiusInline() const;

	virtual void GetTriangleInfo(TriangleInfo& all,DWORD tif_flags,int selected_node=-1);
	virtual void GetEmitterMaterial(struct cObjMaterial& material);
	virtual void GetVisibilityVertex(vector<Vect3f> &pos, vector<Vect3f> &norm);

	virtual const MatXf& GetNodePositionMat(int nodeindex) const{return GetNodePosition(nodeindex);}
	virtual const Mats& GetNodePositionMats(int nodeindex) const;

	void AddLink(ObserverLink* link){observer.AddLink(link);}
	void BreakLink(ObserverLink* link){observer.BreakLink(link);}

	virtual int GetNodeNumber() const;
	virtual const char* GetNodeName(int node_index) const;

	const char* GetFileName() const;

	// для осколков
	cTexture* GetDiffuseTexture(int num_mat)const;
	void SetDiffuseTexture(cTexture* texture);
	void SetLodDistance(float lod12,float lod23);
	void SetHideDistance(float distance);

protected:
	void Update();
	Mats position;
	MatXf positionX_;
	float opacity;
	float distance_alpha;
	float hideDistance;
	DWORD user_node_positon;
	vector<MatXf> node_position;
	Observer observer;
	cStaticSimply3dx* pStatic;
	char iLOD;

	void SelectMaterial(Camera* camera);
	void SelectShadowMaterial();
	void SelectZBufferMaterial();
	void SelectMatrix(int offset_matrix);
	__forceinline bool CalcDistanceAlpha(Camera* camera, bool alpha=true);//Возвращает - видим ли объект.
	inline void CalcOpacityFlag();

	friend void SortByLod(cSimply3dx** object,int num_visible_object,Camera* camera,cStaticSimply3dx* pStatic);
	inline bool IsDraw2Pass();

	friend cStaticSimply3dx;
};

class cStaticSimply3dx : public BaseGraphObject
{
public:
	cStaticSimply3dx();
	~cStaticSimply3dx();

	bool		bump;
	bool		is_uv2;
	bool		isDebris;
	bool		enable_fur;

	enum {
		num_lod=3,
	};

	struct ONE_LOD
	{
		sPtrIndexBuffer		ib;
		sPtrVertexBuffer	vb;
		int num_repeat_models;//Количество повторений модели, чтобы несколько моделий одним DIP вывести.
		int ib_begin;
		int vb_begin;
		int	ib_polygon_one_models;
		int	vb_vertex_one_models;
		int blend_indices;//количество костей в vb

		ONE_LOD()
		{
			num_repeat_models=0;
			ib_polygon_one_models=0;
			vb_vertex_one_models=0;
			blend_indices=0;
			ib_begin=vb_begin=0;
		}
		int GetBlendWeight()
		{
			if(blend_indices==1)
				return 0;
			return blend_indices;
		}
	};

	vector<ONE_LOD> lods;//1 или 3 лода.

	vector<MatXf>	node_offset;//Статическое смещение для дополнительных нод

	Mats debrisPos; // Специально для осколков

	sBox6f bound_box;
	float radius;

	ObjectShadowType circle_shadow_enable;
	ObjectShadowType circle_shadow_enable_min;
	int circle_shadow_height;
	float circle_shadow_radius;

	vector<string> node_name;
	string file_name;
	string visibleGroupName;

	void AddCircleShadow(Vect3f Simply3dxPos,float circle_shadow_radius);

	void PreDraw(Camera* camera);
	void Draw(Camera* camera);
	cTexture* GetTexture() {return pDiffuse;};

	const MatXf& GetPosition() const {return MatXf::ID;}
	const string& getVisibleGroupName() {return visibleGroupName;}

	bool BuildFromNode(cStatic3dx* pStatic,int num_node, VisibilityGroupIndex igroup);
	bool BuildLods(cStatic3dx* pStatic,const char* visible_group);
	virtual int Release(){return UnknownClass::Release();}
	bool GetLoaded(){return loaded;}
	void SetLoaded(){loaded = true;}
	int CalcTextureSize() {return pDiffuse?pDiffuse->CalcTextureSize():0;}
	int GetNumOutputPolygons()const{return num_out_polygons;}
	int GetNumOutputObjects()const{return num_out_objects;}
	void SetLodDistance(float lod12,float lod23);
	void SetHideDistance(float distance);
	inline int GetLodByDistance2(float distance2)
	{
		int iLOD;
		if(distance2<border_lod12_2)
			iLOD=0;
		else
		if(distance2<border_lod23_2)
			iLOD=1;
		else
			iLOD=2;
		return iLOD;
	}

	void AddZMinZMaxShadowReciver(MatXf& camera_matrix,Vect2f& objectz);
	cSkinVertex GetSkinVertex(int num_weight){return cSkinVertex(num_weight,bump,is_uv2,enable_fur);}

protected:
	Color4f	ambient;
	Color4f	diffuse;
	Color4f	specular;
	cTexture* pDiffuse;
	bool is_opacity_texture;
	bool is_big_ambient;
	bool loaded;

	vector<cSimply3dx*>* pActiveSceneList;
	vector<cSimply3dx*> shadow_visible_list;
	vector<cSimply3dx*> zBufferVisibleList;
	int num_visible_object;
	int num_visible_opacity_object;

	friend class cScene;
	friend class cSimply3dx;

	void DrawModels(int num_models,ONE_LOD& lod);
	void DrawShadow(Camera* camera);
	void DrawZBuffer(Camera* camera);

	void CalcBoundBox();
	struct PsiVisible
	{
		const struct StaticBunch* psi;
		DWORD visibility;
	};
	// для осколков, номер материала (для извлечения текстуры)
	int num_material;

	struct TemporatyData
	{
		vector<int> old_to_new;
		vector<int> new_to_old;
		vector<sPolygon> polygons;
	};
	bool BuildAllBuffers(vector<PsiVisible>& groups,cStatic3dx* pStatic,vector<int>& old_to_new_bone_index);
	void BuildBuffersOneNode(vector<PsiVisible>& groups,cStatic3dx* pStatic,int nNode);

	void DrawObjects(Camera* camera,cSimply3dx** objects,int num_object);
	
	int num_out_polygons;
	int num_out_objects;
	int border_lod12_2,border_lod23_2;
	float hideDistance;
};

#endif
