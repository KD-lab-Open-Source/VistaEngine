#ifndef __I_VIS_GENERIC_INTERNAL_H_INCLUDED__
#define __I_VIS_GENERIC_INTERNAL_H_INCLUDED__
#include "Unknown.h"
#include "..\src\mats.h"

enum UNLOAD_ERROR
{
	UNLOAD_E_SUCCES=0,
	UNLOAD_E_USED=1,
	UNLOAD_E_NOTFOUND=2,
};

extern RandomGenerator graphRnd;//Недетерменированный rnd, который вызывается в графическом потоке.

#include <xutil.h>
#define VISASSERT(a)											xassert(a)
#include "RenderMT.h"
#include "umath.h"

class cInterfaceRenderDevice;
class cScene;
class cVisGeneric;
class cTexture;
class cCamera;

struct sAttribute
{
protected:
	int		Attribute;	
public:
	sAttribute()													{ Attribute=0; }
	
	inline int GetAttribute(int attribute=0xFFFFFFFF)	const		{ return Attribute&attribute; }
	inline void ClearAttribute(int attribute)						{ Attribute&=~attribute; }
	inline void SetAttribute(int attribute)							{ Attribute|=attribute; }

	inline void PutAttribute(int attribute,bool on){if(on)SetAttribute(attribute);else ClearAttribute(attribute);}
};

enum eKindUnknownClass
{
	KIND_NULL				=		0,

	KIND_LIGHT				=		4,			// cUnkLight - источники света
	KIND_OBJ_3DX			=		6,			// cObject3dx - трехмерные объекты из полигонов.
	KIND_SIMPLY3DX			=		7,
	KIND_STATICSIMPLY3DX	=		8,
	KIND_EFFECT				=		9,				// 
};

class cBaseGraphObject: public cUnknownClass, protected sAttribute
{
public:
	// инициализационная часть 
	cBaseGraphObject(int kind);
	virtual ~cBaseGraphObject(){}
	virtual	int Release();
	virtual void Attach();//Теперь все объекты сначала не добавлены в сцену, это нужно для HT, чтобы не было видно непроинициализированного объекта.

	inline int GetKind(int kind) const								{ return Kind==kind; }
	inline int GetKind() const										{ return Kind; }

	// общие функции для работы объектами cBaseGraphObject
	virtual void PreDraw(cCamera *pCamera)=0;
	virtual void Draw(cCamera *pCamera)=0;
	virtual void Animate(float dt)									{ }

	virtual void SetAttr(int attribute)								{ sAttribute::SetAttribute(attribute); }
	virtual void ClearAttr(int attribute=0xFFFFFFFF)				{ sAttribute::ClearAttribute(attribute); }
	inline int GetAttr(int attribute=0xFFFFFFFF)					{ return Attribute&attribute; }
	inline void PutAttr(int attribute,bool on)						{if(on)SetAttr(attribute);else ClearAttr(attribute);}

	virtual void SetPosition(const MatXf& Matrix);
	virtual void SetPosition(const Se3f& pos);
	virtual const MatXf& GetPosition() const =0;

	/// Нужно для сортировки объектов по расстоянию.
	virtual Vect3f GetCenterObject() {return GetPosition().trans();}

	inline class cScene* GetScene()									{ return IParent; }
	inline void SetScene(cScene* pScene)							{ IParent=pScene; }
	//В локальных координатах, для получения Box в глобальных координатах,
	//нужно умножить на GetGlobalMatrix()
	virtual float GetBoundRadius() const;
	virtual void GetBoundBox(sBox6f& Box) const;

	virtual void SetAnimKeys(struct sParticleKey *AnimKeys,int size);

	// GetSpecialSortIndex - по этому индексу сортируются "штучные объекты"
	virtual int GetSpecialSortIndex()const{return 0;}

	inline void MTAccess();
protected:
	cScene			*IParent;		// интерфейс породивший данный класс
	eKindUnknownClass	Kind;
};

struct TriangleInfo
{
	vector<sPolygon> triangles;
	vector<bool>   visible_points;
	vector<Vect3f> positions;
	vector<Vect3f> normals;
	vector<Vect2f> uv;
	vector<bool>   selected_node;
};

enum TRIANGLE_INFO_FILL
{
	TIF_TRIANGLES			=1<<0,
	TIF_VISIBLE_POINTS		=1<<1,
	TIF_POSITIONS			=1<<2,
	TIF_NORMALS				=1<<3,
	TIF_UV					=1<<4,
	TIF_ZERO_POS			=1<<5,
	TIF_ONE_SCALE			=1<<6,
	TIF_ONLY_VISIBLE		=1<<7,
};

class c3dx:public cBaseGraphObject
{
public:
	c3dx(int kind):cBaseGraphObject(kind){}
	virtual void SetScale(float scale)=0;
	virtual float GetScale()const=0;

	virtual void GetTriangleInfo(TriangleInfo& all,DWORD tif_flags,int selected_node=-1)=0;
	virtual void GetEmitterMaterial(struct cObjMaterial& material)=0;
	virtual void GetVisibilityVertex(vector<Vect3f> &pos, vector<Vect3f> &norm) = 0;

	virtual void SetNodePosition(int nodeindex,const Se3f& pos) = 0;
	virtual void SetNodePositionMats(int nodeindex,const Mats& pos)=0;

	//radius потом умножается на scale объекта!
	virtual void SetCircleShadowParam(float radius,float height=-1)=0;
	enum OBJECT_SHADOW_TYPE
	{
		OST_SHADOW_NONE=0,
		OST_SHADOW_CIRCLE=1,
		OST_SHADOW_REAL=2,
	};
	virtual void SetShadowType(OBJECT_SHADOW_TYPE type)=0;//Для всех объектов такого типа применяется одновременно

	//В случае cObject3dx быстрая GetNodePositionMats
	//В случае cSimply3dx быстрая GetNodePositionMat
	virtual const MatXf& GetNodePositionMat(int nodeindex) const=0;
	virtual const Mats& GetNodePositionMats(int nodeindex) const=0;

	virtual void AddLink(class ObserverLink* link)=0;
	virtual void BreakLink(ObserverLink* link)=0;

	virtual int GetNodeNumber() const=0;
	virtual const char* GetNodeName(int node_index) const=0;

	virtual void SetOpacity(float opacity)=0;
	virtual float GetOpacity()const=0;

	virtual const char* GetFileName() const=0;
	virtual const Mats& GetPositionMats() const=0;

	virtual int GetNumOutputPolygons()const{return 0;}

	virtual cTexture* GetDiffuseTexture(int num_mat)const{return NULL;}
	virtual void SetLodDistance(float lod12,float lod23)=0;
	virtual void SetHideDistance(float distance) = 0;
};

#include "..\src\visgeneric.h"

/*
	Нельзя создать два объекта cVisGeneric или cLogicGeneric.
	При повторном вызове этих функций возвращается тот-же объект.
*/

struct sParticleKey
{
	sColor4c	diffuse;	// diffuse of particle
	Vect2f		rotate;		// sin & cos угла поворота * size of particle
	Vect2f		TexPos;		// texture position
	inline void SetRotate(float angle,float size)		{ rotate.x=size*cosf(angle); rotate.y=size*sinf(angle); }
};


void dprintf(char *format, ...);
typedef void (*ExternalObjFunction)();

#endif
