#ifndef __C_CHAOS_H_INCLUDED__
#define __C_CHAOS_H_INCLUDED__

#include "Render\inc\IVisGenericInternal.h"
#include "Render\inc\IRenderDevice.h"

class RENDER_API cChaos : public BaseGraphObject
{
	cTexture    *pTex0;
	cTexture    *pTex0_0,*pTex0_1;
	Vect2f stime_tex0,stime_tex1;

	cTexture    *pTexRender;
	cTexture    *pTexBump;
	float time;

	int size;

	enum BUMP_TYPE
	{
		BUMP_NONE=0,
		BUMP_RENDERTARGET,
		BUMP_PS14
	};

	BUMP_TYPE enablebump;
	float uvmul;

	enum
	{
		sub_div=5,
	};

	sPtrIndexBuffer ib;

	typedef sVertexXYZDT2 VTYPE;
	sPtrVertexBuffer vb;
	Vect2f plane_size;

	class VSChaos* pVS;
	class PSChaos* pPS;
public:

	cChaos(Vect2f size,const char* str_tex0,const char* str_tex1,const char* str_bump,int tile,bool enable_bump);
	void SetTextures(const char* str_tex0,const char* str_tex1,const char* str_bump);
	~cChaos();
	
	void serialize(Archive& ar);
	virtual void Animate(float dt);
	virtual void PreDraw(Camera* camera);
	virtual void Draw(Camera* camera);

	void RenderAllTexture();
	static bool CurrentChaos(){return count_chaos_struct != 0;}
	int sortIndex()const{return -3;}
protected:
	void CreateIB();
	void CreateVB();

	void RenderTexture();
	void RenderTex0();

	float sfmod(float a,float b)
	{
		if(a>0)
			return fmodFast(a,b);
		return b-fmodFast(-a,b);
	}
	static int count_chaos_struct;
};

#endif
