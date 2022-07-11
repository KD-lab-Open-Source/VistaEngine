#ifndef __C_CHAOS_H_INCLUDED__
#define __C_CHAOS_H_INCLUDED__

class cChaos:public cIUnkObj
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

	cChaos(Vect2f size,LPCSTR str_tex0,LPCSTR str_tex1,LPCSTR str_bump,int tile,bool enable_bump);
	void SetTextures(LPCSTR str_tex0,LPCSTR str_tex1,LPCSTR str_bump);
	~cChaos();
	
	virtual void Animate(float dt);
	virtual void PreDraw(cCamera *DrawNode);
	virtual void Draw(cCamera *DrawNode);

	void RenderAllTexture();
	static bool CurrentChaos(){return count_chaos_struct;}
	int GetSpecialSortIndex()const{return -3;}
protected:
	void CreateIB();
	void CreateVB();

	void InitBumpTexture1(cTexture* pTex);
	void InitBumpTexture2(cTexture* pTex);
	void ConvertToBump(cTexture*& pOut,cTexture* pIn);

	void RenderTexture();
	void RenderTex0();

	double sfmod(double a,double b)
	{
		if(a>0)
			return fmod(a,b);
		return b-fmod(-a,b);
	}
	static int count_chaos_struct;
};

#endif
