#ifndef __I_VIS_GENERIC_INTERNAL_H_INCLUDED__
#define __I_VIS_GENERIC_INTERNAL_H_INCLUDED__

#include "RenderMT.h"
#include "Unknown.h"
#include "XMath/mats.h"
#include "XMath/Box6f.h"
#include "XMath/Colors.h"
#include "Render/3dx/Umath.h"

extern RENDER_API RandomGenerator graphRnd;//������������������� rnd, ������� ���������� � ����������� ������.

class cInterfaceRenderDevice;
class cScene;
class cVisGeneric;
class cTexture;
class Camera;

enum ObjectShadowType : int {
    OBJECT_SHADOW_NONE = 0,
    OBJECT_SHADOW_CIRCLE,
    OBJECT_SHADOW_STENCIL
};

class RENDER_API  sAttribute
{
public:
	sAttribute()													{ attribute_=0; }
	
	int getAttribute(int attribute = 0xFFFFFFFF)	const		{ return attribute_ & attribute; }
	virtual void clearAttribute(int attribute)						{ attribute_ &= ~attribute; }
	virtual void setAttribute(int attribute)							{ attribute_ |= attribute; }

	void putAttribute(int attribute, bool on) { if(on) setAttribute(attribute); else clearAttribute(attribute); }

protected:
	int		attribute_;	
};

enum eKindUnknownClass
{
	KIND_NULL				=		0,

	KIND_LIGHT				=		4,			// cUnkLight - ��������� �����
	KIND_OBJ_3DX			=		6,			// cObject3dx - ���������� ������� �� ���������.
	KIND_SIMPLY3DX			=		7,
	KIND_STATICSIMPLY3DX	=		8,
	KIND_EFFECT				=		9,				// 
	KIND_LEAVES				=		10,
};

class RENDER_API BaseGraphObject : public UnknownClass, public sAttribute
{
public:
	// ����������������� ����� 
	BaseGraphObject(int kind);
	virtual ~BaseGraphObject(){}
	virtual	int Release();
	virtual void Attach();//������ ��� ������� ������� �� ��������� � �����, ��� ����� ��� HT, ����� �� ���� ����� ������������������������ �������.

	int GetKind() const	{ return Kind; }

	// ����� ������� ��� ������ ��������� cBaseGraphObject
	virtual void PreDraw(Camera* camera)=0;
	virtual void Draw(Camera* camera)=0;
	virtual void Animate(float dt)									{ }

	virtual void SetPosition(const MatXf& Matrix);
	virtual void SetPosition(const Se3f& pos);
	virtual const MatXf& GetPosition() const;

	cScene* scene() const { return scene_; }
	void setScene(cScene* pScene) { scene_=pScene; }

	//� ��������� �����������, ��� ��������� Box � ���������� �����������,
	//����� �������� �� GetGlobalMatrix()
	virtual float GetBoundRadius() const;
	virtual void GetBoundBox(sBox6f& Box) const;

	// sortIndex - �� ����� ������� ����������� "������� �������"
	virtual int sortIndex()const { return 0; }

	inline void MTAccess();

protected:
	cScene			*scene_;		// ��������� ���������� ������ �����
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

class c3dx : public BaseGraphObject
{
public:
	c3dx(int kind):BaseGraphObject(kind){}
	virtual void SetScale(float scale)=0;
	virtual float GetScale()const=0;

	virtual void GetTriangleInfo(TriangleInfo& all,DWORD tif_flags,int selected_node=-1)=0;
	virtual void GetEmitterMaterial(struct cObjMaterial& material)=0;
	virtual void GetVisibilityVertex(vector<Vect3f> &pos, vector<Vect3f> &norm) = 0;

	virtual void SetNodePosition(int nodeindex,const Se3f& pos) = 0;
	virtual void SetNodePositionMats(int nodeindex,const Mats& pos)=0;

	//radius ����� ���������� �� scale �������!
	virtual void SetShadowType(ObjectShadowType type)=0;//��� ���� �������� ������ ���� ����������� ������������
	virtual void SetCircleShadowParam(float radius,float height=-1)=0;
	virtual ObjectShadowType getShadowType()=0;
	virtual void getCircleShadowParam(float& radius, float& height)=0;

	//� ������ cObject3dx ������� GetNodePositionMats
	//� ������ cSimply3dx ������� GetNodePositionMat
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

	virtual cTexture* GetDiffuseTexture(int num_mat)const{return 0;}
	virtual void SetLodDistance(float lod12,float lod23)=0;
	virtual void SetHideDistance(float distance) = 0;
	
	virtual void Update() = 0;
};


#endif
