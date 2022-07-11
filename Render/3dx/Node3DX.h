#ifndef __NODE_3DX_H_INCLUDED__
#define __NODE_3DX_H_INCLUDED__
#include "Static3dx.h"
#include "..\src\observer.h"
#include "..\src\nparticle.h"
#include "..\src\mats.h"
/*
	Ограничения cObject3dx. 
	65536 вертексов суммарно может быть в модели. 
	максимум 256 нод. (Это то что передаётся например в GetNodePosition)
	Максимум 32 группы видимости в одном set. (SetVisibilityGroup)
*/

class cObject3dx;
class cSimply3dx;

struct cNode3dx
{
	Mats pos;
	float phase;
	BYTE chain;
	BYTE index_scale;
	BYTE index_position;
	BYTE index_rotation;
	BYTE additional_transform;//255 - нет добавки
	bool IsAdditionalTransform(){return additional_transform!=255;}

	inline void CalculatePos(cStaticNodeChain& chain,Mats& pos);
	cNode3dx();
};

struct c3dxAdditionalTransformation
{
	int nodeindex;
	Mats mat;
};

class cObject3dxAnimation
{
public:
	cObject3dxAnimation(cStatic3dx* pStatic);
	cObject3dxAnimation(cObject3dxAnimation* obj);
	cStatic3dx* GetStatic(){return pStatic;}//только для чтения

	void SetPhase(float phase);
	void SetAnimationGroupPhase(int ianimationgroup,float phase);

	float GetAnimationGroupPhase(int ianimationgroup);
	void SetAnimationGroupChain(int ianimationgroup,const char* chain_name);
	void SetAnimationGroupChain(int ianimationgroup,int chain_index);
	int GetAnimationGroupChain(int ianimationgroup);

	int GetChainNumber();
	cAnimationChain* GetChain(int i);
	bool SetChain(const char* chain);
	void SetChain(int chain_index);
	int GetChainIndex(const char* chain_name);//Возвращает индекс, который используется как chain_index

	//nodeindex - индекс из массива cStatic3dx::nodes
	int FindNode(const char* node_name) const;//индекс в cStatic3dx::nodes (-1=не нашли)
	const Se3f& GetNodePosition(int nodeindex) const;//Валидные данные только после первого Update, возвращает положение относительно глобальной системы координат
	const Mats& GetNodePositionMats(int nodeindex) const;
	const MatXf& GetNodePositionMat(int nodeindex) const;
	int GetNodeNumber() const;
	int GetParentNumber(int node_index) const;
	bool CheckDependenceOfNodes(int dependent_node_index, int node_index) const;
	const char* GetNodeName(int node_index) const;

	void UpdateMatrix(Mats& position,vector<c3dxAdditionalTransformation>& additional_transformations);

	void GetNodeOffset(int nodeindex,Mats& offset,int& parent_node);
protected:
	int FindChain(const char* chain_name);
	void CalcOpacity(int imaterial);

	cStatic3dx* pStatic;
	bool updated;
	//порядок хранения cNode3dx таков, что 
	//child всегда находится после parent
	vector<cNode3dx> nodes;

	struct  MaterialAnim
	{
		float phase;
		int chain;
		float opacity;
	};
	vector<MaterialAnim> materials;

	friend class cObject3dxAnimationSecond;
	friend class cObject3dx;
};

class cObject3dxAnimationSecond:public cObject3dxAnimation
{
public:
	cObject3dxAnimationSecond(cStatic3dx* pStatic);
	cObject3dxAnimationSecond(cObject3dxAnimationSecond* pObj);
	float GetAnimationGroupInterpolation(int ianimationgroup);
	void SetAnimationGroupInterpolation(int ianimationgroup,float k);

	bool IsInterpolation();
protected:
	void lerp(cObject3dxAnimation* to);
	friend class cObject3dx;
	vector<float> interpolation;
};

class cObject3dx:public c3dx,public cObject3dxAnimation
{//root объект через который происходит всё взаимодействие
public:
	cObject3dx(cStatic3dx* pStatic,bool interpolate);
	cObject3dx(cObject3dx* pObj);
	~cObject3dx();
	void SetPosition(const Se3f& pos);
	void SetPosition(const MatXf& Matrix);
	const MatXf& GetPosition() const;
	const Se3f& GetPositionSe() const;
	const Mats& GetPositionMats() const;
	virtual Vect3f GetCenterObject() {return position.trans();}

	void SetScale(float scale);
	float GetScale()const{return position.s;}

	float GetBoundRadius() const;
	void GetBoundBox(sBox6f& box) const;
	void GetBoundBoxUnscaled(sBox6f& box);

	void PreDraw(cCamera *UCamera);
	void Draw(cCamera *UCamera);
	void Animate(float dt);

	void DrawLine(cCamera* pCamera);

	int GetAnimationGroupNumber();
	int GetAnimationGroup(const char* name);//Возвращает индекс, который используется как ianimationgroup
	const char* GetAnimationGroupName(int ianimationgroup);

	int GetVisibilityGroupNumber(C3dxVisibilitySet iset = C3dxVisibilitySet::ZERO){return GetVisibilitySet(iset)->GetVisibilityGroupNumber();};
	const char* GetVisibilityGroupName(C3dxVisibilityGroup group,C3dxVisibilitySet iset = C3dxVisibilitySet::ZERO){return GetVisibilitySet(iset)->GetVisibilityGroupName(group);};
	C3dxVisibilityGroup GetVisibilityGroupIndex(const char* group_name,C3dxVisibilitySet iset = C3dxVisibilitySet::ZERO){return GetVisibilitySet(iset)->GetVisibilityGroupIndex(group_name);};
	void SetVisibilityGroup(C3dxVisibilityGroup group,C3dxVisibilitySet iset=C3dxVisibilitySet::ZERO);
	bool SetVisibilityGroup(const char* name, bool silently = false,C3dxVisibilitySet iset = C3dxVisibilitySet::ZERO);

	cStaticVisibilityChainGroup* GetVisibilityGroup(C3dxVisibilitySet iset=C3dxVisibilitySet::ZERO);
	C3dxVisibilityGroup GetVisibilityGroupIndex(C3dxVisibilitySet iset);
	
	int GetVisibilitySetNumber(){return pStatic->visible_sets.size();};
	const char* GetVisibilitySetName(C3dxVisibilitySet iset);
	C3dxVisibilitySet GetVisibilitySetIndex(const char* set_name);
	void SetVisibilitySetAlpha(float alpha,C3dxVisibilitySet iset=C3dxVisibilitySet::ZERO);

	bool IntersectSphere(const Vect3f& p0, const Vect3f& p1) const;
	bool IntersectBound(const Vect3f& p0, const Vect3f& p1) const;
	//Intersect - Стала в текущей версии существенно медленнее за счёт скиннинга
	//не абсолютно точная функция, если объект выйдет за пределы своего bound даст ошибку.
	bool Intersect(const Vect3f& p0,const Vect3f& p1) const;
	const char* GetFileName() const;

	//SetNodePosition можно применять только при флаге ATTR3DX_NOUPDATEMATRIX
	//Эта функция неприятна в том смысле, что любая функция, требующая для своей работы UpdateMatrix изменит положение объекта
	void SetNodePosition(int nodeindex,const Se3f& pos);

	void SetNodePositionMats(int nodeindex,const Mats& pos);

	void SetUserTransform(int nodeindex,const Se3f& pos);//Не оптимально написанна, если трансформация будет для многих node
	void SetUserTransform(int nodeindex,const Mats& pos);

	//Возвращает значение видимости, заданное художником в треке видимости объекта в 3DS Max
	//Не влияет ни на что, кроме эффектов, автоматически привязанных к объекту.
	bool GetVisibilityTrack(int nodeindex) const;

	//Возвращает true если на этом интервале есть переход от невидимого к видимому.
	bool GetVisibilityTrackInterval(int nodeindex,float begin_phase,float end_phase) const;

	//RestoreNodeMatrix отменяет действие SetUserTransform
	void RestoreUserTransform(int nodeindex);

	void SetSilhouetteIndex(int index);
	int GetSilhouetteIndex() const { return silhouette_index; }

	void SetSkinColor(sColor4c skin_color, const char* emblem_name_ = "");//Дорогая функция, грузит текстуры (если только не вызвали PreloadElement c нужными параметрами)
	sColor4c GetSkinColor(){return skin_color;};

	//ambient.a,specular.a  коэффициэнты интерполяции с цветами из материала заданного в редакторе.
	//!diffuse.a - прозрачность объекта
	void SetColorOld(const sColor4f *ambient,const sColor4f *diffuse,const sColor4f *specular=0);
	void GetColorOld(sColor4f *ambient,sColor4f *diffuse,sColor4f *specular=0) const;

	//ambient.a,specular.a,diffuse.a  коэффициэнты интерполяции с цветами из материала заданного в редакторе.
	void SetColorMaterial(const sColor4f *ambient,const sColor4f *diffuse,const sColor4f *specular=0);
	void GetColorMaterial(sColor4f *ambient,sColor4f *diffuse,sColor4f *specular=0) const;

	//Прозрачность объекта.
	virtual void SetOpacity(float opacity);
	virtual float GetOpacity()const;

	//Цвет текстуры в зависимости от параметра lerp_color.а меняется. 0 - цвет текстуры, 1 - цвет lerp_color.
	void SetTextureLerpColor(const sColor4f& lerp_color);
	sColor4f GetTextureLerpColor() const;

	void GetLocalBorder(int *nVertex,Vect3f **Vertex,int *nIndex,short **Index);

	void DrawLogic(cCamera* pCamera,int selected=-1);
	void DrawLogicBound();
	void DrawBound() const;

	//Пересчитывает местоположение объекта.
	//Нужно вызывать, если хочется узнать положение одного из узлов
	void Update();

	void AddLink(ObserverLink* link){observer.AddLink(link);}
	void BreakLink(ObserverLink* link){observer.BreakLink(link);}

	//Прилинковать один объект к другому.
	//Если object=NULL - отлинковать.
	void LinkTo(cObject3dx* object,int inode,bool set_scale=true);

	//Выдают для соответствующей группы видимости треугольники и точки.
	void GetEmitterMaterial(struct cObjMaterial& material);

	//Выдает все точки, определить какие видимы из них можно только по triangles либо по visible_points
	void GetTriangleInfo(TriangleInfo& all,DWORD tif_flags,int selected_node=-1);
	void GetVisibilityVertex(vector<Vect3f> &pos, vector<Vect3f> &norm);

	//debug statistics
	int GetMaterialNum();
	int GetNodeNum();
	/////////////

	void SetLodDistance(float lod12,float lod23);
	void SetHideDistance(float distance);
	static void SetUseLod(bool enable);
	static bool GetUseLod();

	//SetCircleShadow,EnableSelfShadow() - Параметры общие для всех объектов с таким названием (GetFileName())
	//height -Высота, когда исчезает тень
	void SetCircleShadowParam(float radius,float height=-1);
	void SetShadowType(OBJECT_SHADOW_TYPE type);

	cObject3dxAnimationSecond* GetInterpolation(){return pAnimSecond;};

	void SetLod(int ilod);
	void AutomaticSetLod(){ClearAttribute(ATTRUNKOBJ_NO_USELOD);};

	void EnableSelfIllumination(bool enable);
	virtual void SetAttr(int attribute);
	virtual void ClearAttr(int attribute);

	const MatXf& GetNodePositionMat(int nodeindex) const{return cObject3dxAnimation::GetNodePositionMat(nodeindex);};
	const Mats& GetNodePositionMats(int nodeindex) const{return cObject3dxAnimation::GetNodePositionMats(nodeindex);};

	int GetNodeNumber() const{return cObject3dxAnimation::GetNodeNumber();};
	const char* GetNodeName(int node_index) const{return cObject3dxAnimation::GetNodeName(node_index);}

	void DrawAll(cCamera *UCamera);
	void AddLight(cUnkLight* light);//Для SceneLightProc

	bool QueryVisibleIsVisible();
	void QueryVisible(cCamera* pCamera);
	void DisableDetailLevel();//Его нельзя выключить, потому как противоречие тогда будет.

	void SetSilouetteCenter(Vect3f center){silouette_center=center;}
	const Vect3f& GetSilouetteCenter()const{return silouette_center;}
	void GetTextureNames(vector<string>& texture_names);

	sBox6f CalcDynamicBoundBox(MatXf world_view);//Только для силуэтов! В глобальных координатах.
	cStaticVisibilitySet* GetVisibilitySet(C3dxVisibilitySet iset);

	bool IsLogicBound(int nodeindex);
	sBox6f GetLogicBoundScaledUntransformed(int nodeindex);
	int GetNumPolygons();
#ifdef POLYGON_LIMITATION
	int GetNumOutputPolygons()const{return num_out_polygons;}
#endif
	cTexture* GetDiffuseTexture(int num_mat)const;

	//Нужно не забывать, что SetSkinColor выставляет тоже текстуру
	//Делает внутри AddRef
	void SetDiffuseTexture(int num_mat,cTexture* texture);
	float GetFov(){return pStatic->camera_params.fov;}
protected:

	struct EffectData
	{
		cEffect* pEffect;
		BYTE index_visibility;
		float prev_phase;
		EffectData():pEffect(0),index_visibility(255){}//255 - неинициализированн.
	};
	float distance_alpha;
	int iLOD;
	bool isSkinColorSet_;
	bool isHaveSkinColorMaterial_;
	struct sGroup
	{
		C3dxVisibilityGroup i;
		cStaticVisibilityChainGroup* p;
		float alpha;
	};
	vector<sGroup> iGroups;
	Mats position;

	struct MaterialTexture
	{
		cTexture* diffuse_texture;
		float texture_phase;
		bool is_opacity_vg;
	};
	vector<MaterialTexture> material_textures;

	sColor4c skin_color;
	sColor4f ambient,diffuse,specular;
	sColor4f lerp_color;
	float object_opacity;

	vector<c3dxAdditionalTransformation> additional_transformations;
	vector<EffectData> effects;
	Observer observer;

	class EffectObserverLink3dx:protected ObserverLink
	{
		cObject3dx* parent_object;
		int parent_node;
		cObject3dx* this_object;
		bool set_scale;
	public:
		EffectObserverLink3dx():parent_object(0),parent_node(-1),this_object(0),set_scale(true){}
		void SetParent(cObject3dx* this_object_){this_object=this_object_;}

		void Link(class cObject3dx* object,int inode,bool set_scale);
		virtual void Update();
		bool IsInitialized(){return observer!=0;}
	} link3dx;

	vector<class cUnkLight*> point_light;//Источники, которые светят на объект
	vector<class cUnkLight*> lights;//Источники внутри объекта.

	cObject3dxAnimationSecond* pAnimSecond;//Для интерполяции анимационных цепочек.
	unsigned char silhouette_index;

	void CalcBoundingBox();
	void CalcBoundSphere();

	void LoadTexture(bool only_skinned,bool only_self_illumination=false, const char* texture_name = "");
	void ProcessEffect(cCamera *pCamera);

	bool IntersectTriangle(const Vect3f& p0,const Vect3f& p1) const;
	bool IntersectTriangleSSE(const Vect3f& p0,const Vect3f& p1) const;
	void CalcOffsetMatrix(MatXf& offset,const Vect3f& p0,const Vect3f& p1) const;

	void GetWorldPos(cStaticIndex& s,MatXf* world,int& world_num) const;
	void GetWorldPos(cStaticIndex& s,MatXf& world, int& idx);
	void AddCircleShadow();
	void UpdateVisibilityGroups();
	inline void UpdateVisibilityGroup(C3dxVisibilitySet iset);

	void DrawMaterialGroup(cStaticIndex& s);
	void DrawMaterialGroupSelectively(cStaticIndex& s,const sColor4f& color,bool draw_opacity,
							class VSSkin* vs,class PSSkin* ps);

	bool IsVisibleMaterialGroup(cStaticIndex& s) const;
	void DrawShadowAndZbuffer(cCamera* pCamera,bool ZBuffer=false);

	void CalcIsOpacity(	bool& is_opacity,bool& is_noopacity);

	float border_lod12_2,border_lod23_2;
	float hideDistance;
	static bool enable_use_lod;

	class cOcclusionSilouette* pOcclusionQuery;

	c3dxAdditionalTransformation& GetUserTransformIndex(int nodeindex);
	friend int GetLodByDistance2(float distance2);

#ifdef POLYGON_LIMITATION
	int polygon_limitation_num_polygon;
	int num_out_polygons;
#endif

	Vect3f silouette_center;
	void SetUVTrans(class VSSkinBase* vs,cStaticMaterial& mat,MaterialAnim& mat_anim);
	void SetSecondUVTrans(bool use,class VSSkinBase* vs,cStaticMaterial& mat,MaterialAnim& mat_anim);
	void UpdateAndLerp(Mats& position,vector<c3dxAdditionalTransformation>& additional_transformations);
	
	bool IsHaveSkinColorMaterial();

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
};

#endif
