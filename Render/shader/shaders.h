#ifndef __SHADERS_H_INCLUDED__
#define __SHADERS_H_INCLUDED__
#include "ShaderStorage.h"

struct sDataRenderMaterial
{
	sColor4f	Ambient;
	sColor4f	Diffuse;
	sColor4f	Specular;
	//Specular.a=Power
	sColor4f    lerp_texture;

	cTexture	*Tex[2];
	vector<class cUnkLight*>* point_light;
};

enum SHADOW_SHADER
{
	SHADOW_SHADER_NONE,
	SHADOW_SHADER_9700,
	SHADOW_SHADER_FX
};

struct ConstShaderInfo
{
	char* name;
	int begin_register;
	int num_register;
};

class cShader
{
public:
	static vector<cShader*> all_shader;

	cShader();
	virtual ~cShader();
	virtual void Restore()=0;
	virtual void Delete()=0;
};

class cVertexShader:public cShader
{
public:
	cVertexShader();
	~cVertexShader();

	void Delete();
	virtual void Restore();

	inline void SetMatrix(const SHADER_HANDLE& h,const D3DXMATRIX* mat);
	inline void SetVector(const SHADER_HANDLE& h,const D3DXVECTOR4* mat);
	inline void SetFloat(const SHADER_HANDLE& h,const float f);

	inline void SetMatrix4x4(const SHADER_HANDLE& h,int index,const D3DXMATRIX* mat);
	inline void SetMatrix4x3(const SHADER_HANDLE& h,int index,const D3DXMATRIX* mat);
	inline void SetMatrix4x3(const SHADER_HANDLE& h,int index,const MatXf& mat);
protected:
	ConstShaderInfo* pShaderInfo;
	int ShaderInfoSize;
	cShaderStorage*  shader;

	void Select();
	virtual void RestoreShader()=0;
	void GetVariableByName(SHADER_HANDLE& h,const char* name);
	virtual void GetHandle();

	void LoadShader(const char* name);
};

class cPixelShader:public cShader
{
protected:
	cShaderStorage*  shader;
public:
	cPixelShader();
	~cPixelShader();

	virtual void Select();
	void Delete();
	inline void SetVector(const SHADER_HANDLE& h,const D3DXVECTOR4* mat);
	inline void SetVector(const SHADER_HANDLE& h,const D3DXVECTOR4* mat,const int count);

	void GetVariableByName(SHADER_HANDLE& sh,const char* name);
	virtual void Restore();
protected:
	void LoadShader(const char* name);
	virtual void RestoreShader()=0;
	virtual void GetHandle();
};

class vsSceneShader:public cVertexShader
{
protected:
	SHADER_HANDLE vFog;
	SHADER_HANDLE mView;

	SHADER_HANDLE fPlanarNode;
	INDEX_HANDLE FOG_OF_WAR;
public:
	virtual void GetHandle();
	void SetFog();
};

class psSceneShader:public cPixelShader
{
protected:
	SHADER_HANDLE vFogOfWar;
	SHADER_HANDLE vShade;
	INDEX_HANDLE FOG_OF_WAR;
public:
	psSceneShader();
	virtual void GetHandle();
	virtual void RestoreShader()=0;
	void SetFog();

	void SetShadowIntensity(const sColor4f& f);
};

class PSShowMap:public cPixelShader
{
public:
	virtual void RestoreShader();
};

class PSShowAlpha:public cPixelShader
{
public:
	virtual void RestoreShader();
};

class VSShadow:public cVertexShader
{
protected:
	SHADER_HANDLE mWVP;
public:
	virtual void Select(const MatXf& world=MatXf::ID);
	virtual void RestoreShader();
	virtual void GetHandle();
};

class PSShadow:public cPixelShader
{
public:
	virtual void RestoreShader();
};

class VSScene:public cVertexShader
{
public:
	virtual void Select(const MatXf* world)=0;
	virtual void SetMaterial(sDataRenderMaterial *Data){};
	void SetWorldSize(Vect2f sz){}//Потом стереть.
};

class VSTileMapScene:public vsSceneShader
{
protected:
	SHADER_HANDLE mWVP;
	SHADER_HANDLE vLightDirection;
	SHADER_HANDLE UV;
	SHADER_HANDLE UVbump;
	SHADER_HANDLE vColor;
	SHADER_HANDLE mulMiniTexture;
	SHADER_HANDLE vReflectionMul;
	INDEX_HANDLE ZREFLECTION;
	
	virtual void GetHandle();
public:
	VSTileMapScene(){}
	virtual void Select();
	virtual void SetUV(Vect2f& uv_base,Vect2f& uv_step,Vect2f& uv_base_bump,Vect2f& uv_step_bump);
	virtual void SetWorldSize(Vect2f sz);

	virtual void SetColor(const sColor4f& color);
	virtual void SetShadowType(SHADOW_SHADER s){}
	virtual void SetReflectionZ(bool reflection);

	void SetMiniTextureSize(int dx,int dy,float res);
};
class PSTileMapScene:public psSceneShader
{
	INDEX_HANDLE ZREFLECTION;
public:
	PSTileMapScene();
	~PSTileMapScene();
	virtual void GetHandle();
	virtual void Select(int matarial_num,VSTileMapScene* vs,float res);
	virtual void SetColor(const sColor4f& color)=0;
	virtual void SetShadowType(SHADOW_SHADER s){}
	virtual void SetReflectionZ(bool reflection);
};

////////////////////////////9700/////////////////////////////////////

class VS9700TileMapScene:public VSTileMapScene
{
protected:
	SHADER_HANDLE mShadow;
public:
	virtual void Select();
	virtual void RestoreShader();
	virtual void GetHandle();
};

class PS9700TileMapScene4x4:public PSTileMapScene
{
	SHADER_HANDLE light_color;
	SHADER_HANDLE inv_light_dir;
public:
	virtual void SetColor(const sColor4f& color);
	virtual void Select(int i,VSTileMapScene* vs,float res);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

////////////////////////////GeforceFX///////////////////////////////////
class VSGeforceFXTileMapScene:public VSTileMapScene
{
	INDEX_HANDLE VERTEX_LIGHT;
protected:
	SHADER_HANDLE mShadow;
	virtual void GetHandle();
	virtual void RestoreShader();
public:
	void Select();
};

class PSGeforceFXTileMapScene:public PSTileMapScene
{
	SHADER_HANDLE light_color;
	SHADER_HANDLE inv_light_dir;
	SHADER_HANDLE fx_offset;
public:
	PSGeforceFXTileMapScene();
	virtual void Select(int i,VSTileMapScene* vs, float res);
	virtual void SetColor(const sColor4f& color);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
};

/////////////////////Chaos///////////////////////////
class VSChaos:public cVertexShader
{
	SHADER_HANDLE mWVP;
	SHADER_HANDLE mUV;
	SHADER_HANDLE mUVBump;
	SHADER_HANDLE mView;
	SHADER_HANDLE vFog;
	void SetFog();
public:
	virtual void Select(float umin,float vmin,float umin2,float vmin2,
		float umin_b0,float vmin_b0,float umin_b1,float vmin_b1);
	void RestoreShader();
protected:
	void GetHandle();
};

class PSChaos:public cPixelShader
{
public:
	virtual void RestoreShader();
};

//////////////////////Skin/////////////////////////////

enum SECOND_UV
{
	SECOND_UV_NONE,
	SECOND_UV_T0,
	SECOND_UV_T1,
};

class VSSkinBase:public vsSceneShader
{
public:
	virtual void Select(MatXf* world,int world_num,int blend_num)=0;
	virtual void SetMaterial(sDataRenderMaterial *Data)=0;
	virtual void SetUVTrans(float mat[6])=0;
	virtual void SetSecondOpacityUVTrans(float mat[6],SECOND_UV use_t1){xassert(0);};
};

class VSSkin:public VSSkinBase
{
protected:
	SHADER_HANDLE	mVP;
	SHADER_HANDLE	mWorldM;
	//SHADER_HANDLE	mWorldProj;

	SHADER_HANDLE	vAmbient;
	SHADER_HANDLE	vDiffuse;
	SHADER_HANDLE	vSpecular;
	SHADER_HANDLE	vCameraPos;
	SHADER_HANDLE	vLightDirection;

	SHADER_HANDLE	vUtrans;
	SHADER_HANDLE	vVtrans;
	SHADER_HANDLE	vReflectionMul;

	SHADER_HANDLE	vPointPos0;
	SHADER_HANDLE	vPointPos1;
	SHADER_HANDLE	vPointColor0;
	SHADER_HANDLE	vPointColor1;
	SHADER_HANDLE	vLightAttenuation;

	INDEX_HANDLE WEIGHT;
	INDEX_HANDLE NOLIGHT;
	INDEX_HANDLE LIGHTMAP;
	INDEX_HANDLE REFLECTION;
	INDEX_HANDLE UVTRANS;
	INDEX_HANDLE ZREFLECTION;
	INDEX_HANDLE POINT_LIGHT;
public:
	VSSkin(){}
	void Select(MatXf* world,int world_num,int blend_num);
	void SetMaterial(sDataRenderMaterial *Data);
	void SetReflection(int n);

	void SetReflectionZ(bool reflection)
	{
		if(ZREFLECTION.is())
			shader->Select(ZREFLECTION,reflection?1:0);
	}

	void SetUVTrans(float mat[6]);
	virtual void SetAlphaColor(sColor4f* color);

	void SetWorldMatrix(MatXf* world,int world_offset,int world_num);//Для cSimply3dx
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	void CalcLightParameters(cUnkLight* light,SHADER_HANDLE& spos,SHADER_HANDLE& scolor,Vect2f& attenuation);
};

class VSSkinReflection:public VSSkin
{
protected:
	void RestoreShader();
};

class VSSkinSecondOpacity:public VSSkin
{
public:
	void SetSecondOpacityUVTrans(float mat[6],SECOND_UV use_t1);
protected:
	void GetHandle();
	void RestoreShader();

	INDEX_HANDLE SECOND_UVTRANS;
	INDEX_HANDLE SECOND_OPACITY_TEXTURE;

	SHADER_HANDLE	vSecondUtrans;
	SHADER_HANDLE	vSecondVtrans;
};

class VSSkinSceneShadow:public VSSkin
{
protected:
	SHADER_HANDLE mShadow;
public:
	void Select(MatXf* world,int world_num,int blend_num);
protected:
	void GetHandle();
	void RestoreShader();
};

class VSSkinNoLight:public VSSkin
{
protected:
	virtual void RestoreShader();
};

class PSSkin:public psSceneShader
{
protected:
	SHADER_HANDLE bumpAmbient;
	SHADER_HANDLE bumpDiffuse;
	SHADER_HANDLE bumpSpecular;
	SHADER_HANDLE reflectionAmount;
	SHADER_HANDLE tLerpPre;

	INDEX_HANDLE REFLECTION;
	INDEX_HANDLE LIGHTMAP;
	INDEX_HANDLE SELF_ILLUMINATION;
	INDEX_HANDLE SPECULARMAP;//Только для bump
	INDEX_HANDLE c2x2;
	INDEX_HANDLE ZREFLECTION;
	INDEX_HANDLE LERP_TEXTURE_COLOR;
public:
	PSSkin(){}
	virtual void SetMaterial(sDataRenderMaterial *Data,bool is_big_ambient);
	void SetReflection(int n,sColor4f& amount);
	void SetSelfIllumination(bool on);
	void SetAlphaColor(sColor4f* color);

	void SelectSpecularMap(cTexture* pSpecularmap,float phase);//Только для bump, обязательно вызывать в bump шейдерах, потому как выставляет SPECULARMAP
	void SetReflectionZ(bool reflection)
	{
		if(ZREFLECTION.is())
			shader->Select(ZREFLECTION,reflection?1:0);
	}
protected:
	void SelectShadowQuality();
	virtual void GetHandle();
	virtual void RestoreShader()=0;
};

class PSSkinNoShadow:public PSSkin
{
protected:
	INDEX_HANDLE NOTEXTURE;
	INDEX_HANDLE NOLIGHT;
public:
	PSSkinNoShadow();
	virtual void SelectLT(bool light,bool texture);
	virtual void Select();
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class PSSkinReflection:public PSSkinNoShadow
{
protected:
	virtual void RestoreShader();
};

class PSSkinSecondOpacity:public PSSkinNoShadow
{
public:
	PSSkinSecondOpacity()
	: PSSkinNoShadow()
	{
	}
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	void Select();
	//void SetOpacityTexture(cTexture tet)
};

class PSSkinSceneShadow:public PSSkin
{
protected:
	SHADER_HANDLE fx_offset;
public:
	virtual void Select();
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class PSSkinBumpSceneShadow:public PSSkinSceneShadow
{
protected:
	virtual void RestoreShader();
};

class PSSkinSceneShadowFX:public PSSkinSceneShadow
{
protected:
	virtual void RestoreShader();
};

class PSSkinBumpSceneShadowFX:public PSSkinSceneShadowFX
{
protected:
	virtual void RestoreShader();
};

class PSSkinReflectionSceneShadowFX:public PSSkinSceneShadowFX
{
protected:
	virtual void RestoreShader();
};

class PSSkinReflectionSceneShadow:public PSSkinSceneShadow
{
protected:
	virtual void RestoreShader();
};

class VSSkinReflectionSceneShadow:public VSSkinSceneShadow
{
protected:
	virtual void RestoreShader();
};

class PSSkinBump:public PSSkin
{
protected:
public:
	virtual void Select();
protected:
	virtual void RestoreShader();
};


class PSSkinNoLight:public PSSkin
{
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class VSSkinBump:public VSSkin
{
protected:
	virtual void RestoreShader();
};

class VSSkinBumpSceneShadow:public VSSkinSceneShadow
{
protected:
	virtual void RestoreShader();
};

class VSSkinShadow:public VSSkinBase
{
protected:
	SHADER_HANDLE	mVP;
	SHADER_HANDLE	mWorldM;
	INDEX_HANDLE WEIGHT;

	SHADER_HANDLE vUtrans;
	SHADER_HANDLE vVtrans;
	SHADER_HANDLE vSecondUtrans;
	SHADER_HANDLE vSecondVtrans;

	INDEX_HANDLE UVTRANS;
	INDEX_HANDLE SECOND_UVTRANS;
	INDEX_HANDLE SECOND_OPACITY_TEXTURE;
public:
	VSSkinShadow(){}

	virtual void Select(MatXf* world,int world_num,int blend_num);
	virtual void GetHandle();
	virtual void RestoreShader();
	void SetMaterial(sDataRenderMaterial *Data){ };
	virtual void SetUVTrans(float mat[6]);
	virtual void SetSecondOpacityUVTrans(float mat[6],SECOND_UV use_t1);
};

class PSSkinShadow:public cPixelShader
{
protected:
	virtual void RestoreShader();
};

class PSSkinShadowAlpha:public cPixelShader
{
protected:
	INDEX_HANDLE SECOND_OPACITY_TEXTURE;
public:
	PSSkinShadowAlpha(){};
	virtual void RestoreShader();
	virtual void GetHandle();

	void SetSecondOpacity(cTexture* pTexture);
};

////////////////////////Water//////////////////////////
class VSWater:public vsSceneShader
{
protected:
	int technique;
	INDEX_HANDLE FLOAT_ZBUFFER;
	bool enableZBuffer;
	SHADER_HANDLE	mVP;
	SHADER_HANDLE	uvScaleOffset;
	SHADER_HANDLE	uvScaleOffset1;
	SHADER_HANDLE	uvScaleOffsetSky;
	SHADER_HANDLE	vCameraPos;
	SHADER_HANDLE	vMirrorVP;
	SHADER_HANDLE	mZBuffer;
public:
	VSWater();
	void SetSpeed(Vect2f scale,Vect2f offset);
	void SetSpeed1(Vect2f scale,Vect2f offset);
	void SetSpeedSky(Vect2f scale,Vect2f offset);
	void Select();
	void SetTechnique(int t);
	int GetTechnique(){return technique;}
	void SetMirrorMatrix(cCamera* mirror);
	void EnableZBuffer(bool enable);

protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class PSWater:public psSceneShader
{
	int technique;
	INDEX_HANDLE FLOAT_ZBUFFER;
	bool enableZBuffer;
	SHADER_HANDLE	vCameraPos;
	SHADER_HANDLE	vLightDirection;
	SHADER_HANDLE	vLightColor;
	SHADER_HANDLE	vReflectionColor;
	SHADER_HANDLE	vPS11Color;
	SHADER_HANDLE	fBrightnes;
public:
	PSWater();
	void SetTechnique(int t);
	void SetWaterColor(sColor4f color0,sColor4f color1);
	void EnableZBuffer(bool enable);
	virtual void Select();
	virtual void GetHandle();
	virtual void RestoreShader();
	void SetReflectionColor(const sColor4f& color);
	void SetReflectionBrightnes(const float brightnes);
	void SetPS11Color(const sColor4f& color);
};

////////////////////Minimal////////////////
class VSMinimalTileMapScene:public VSTileMapScene
{
public:
	virtual void RestoreShader();
	virtual void GetHandle();
};

class PSMinimalTileMapScene:public PSTileMapScene
{
	INDEX_HANDLE VERTEX_LIGHT;
	SHADER_HANDLE light_color;
	SHADER_HANDLE inv_light_dir;
public:
	virtual void Select(int i,VSTileMapScene* vs, float res);
	virtual void SetColor(const sColor4f& color);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
};

////////////////////////////
class VSWaterLava:public vsSceneShader
{
protected:
	SHADER_HANDLE	mVP;
	SHADER_HANDLE	fTime;
	double time;
	cTexture* pGround;
public:
	VSWaterLava();
	~VSWaterLava();
	void SetTime(double time);
	void Select();

	static void SetLavaTexture(const char* file_name);
protected:
	static string lava_name;
	virtual void GetHandle();
	virtual void RestoreShader();
};

class PSWaterLava:public psSceneShader
{
protected:
	IDirect3DBaseTexture9* pRandomVolume;
	SHADER_HANDLE	vLavaColor;
	SHADER_HANDLE	vLavaColorAmbient;
public:
	PSWaterLava();
	~PSWaterLava();
	void SetColors(const sColor4f& lava,const sColor4f& ambient);
	virtual void Select();
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

//////////////////////////////standart
class VSStandart:public vsSceneShader
{
protected:
	SHADER_HANDLE	mWVP;
	SHADER_HANDLE	mWorld;
	SHADER_HANDLE   mZBuffer;
	SHADER_HANDLE	vReflectionMul;
	INDEX_HANDLE FIX_FOG_ADD_BLEND;
	INDEX_HANDLE COLOR_OPERATION;
	INDEX_HANDLE FLOAT_ZBUFFER;
	INDEX_HANDLE ZREFLECTION;
	bool useFloatZBuffer;
public:
	VSStandart(){useFloatZBuffer=false;};
	virtual void Select(const MatXf& world=MatXf::ID);
	void SetFixFogAddBlend(bool fix);
	void SetColorOperation(int op);
	void EnableFloatZBuffer(bool enable);
	void SetReflectionZ(bool reflection)
	{
		if(ZREFLECTION.is())
			shader->Select(ZREFLECTION,reflection?1:0);
	}

protected:
	virtual void GetHandle();
	virtual void RestoreShader();

};

class PSStandart:public psSceneShader
{
	INDEX_HANDLE COLOR_OPERATION;
	INDEX_HANDLE FLOAT_ZBUFFER;
	INDEX_HANDLE ZREFLECTION;
public:
	PSStandart(){};
	virtual void Select();
	void SetColorOperation(int op);
	void EnableFloatZBuffer(bool enable);
	void SetReflectionZ(bool reflection)
	{
		if(ZREFLECTION.is())
			shader->Select(ZREFLECTION,reflection?1:0);
	}
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

//////////////////////////
class VSWaterIce:public vsSceneShader
{
	SHADER_HANDLE mVP;
	SHADER_HANDLE uvScaleOffset;
	SHADER_HANDLE vCameraPos;
	SHADER_HANDLE vMirrorVP;
	SHADER_HANDLE fScaleBumpSnow;
	INDEX_HANDLE MIRROR_LINEAR;
	INDEX_HANDLE PS11;
	bool linear;
public:
	VSWaterIce();
	virtual void Select();
	void SetMirrorMatrix(cCamera* mirror);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
};

class PSWaterIce:public psSceneShader
{
	SHADER_HANDLE vSnowColor;
	INDEX_HANDLE MIRROR_LINEAR;
	INDEX_HANDLE PS11;
	bool linear;
public:
	PSWaterIce();
	virtual void Select();
	void SetSnowColor(sColor4f color);
	void SetMirrorMatrix(cCamera* mirror);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};
class PSSolidColor : public cPixelShader
{
public:
	PSSolidColor() {};
	~PSSolidColor(){};
	void Select();
protected:
	virtual void RestoreShader();
};
class PSMonochrome : public cPixelShader
{
public:
	PSMonochrome() {};
	~PSMonochrome(){};
	void Select(float time_);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE	fTime;
	SHADER_HANDLE	fTimeInv;
};
class PSColorDodge : public PSMonochrome
{
protected:
	virtual void RestoreShader();
};
class PSColorBright : public cPixelShader
{
public:
	PSColorBright(){};
	~PSColorBright(){};
	void Select(float luminance);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE Luminance;
};
class PSCombine : public cPixelShader
{
public:
	PSCombine(){};
	~PSCombine(){};
	void Select(sColor4f _color);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE addColor;
};
class PSBloomHorizontal : public cPixelShader
{
public:
	PSBloomHorizontal(){};
	~PSBloomHorizontal(){};
	void Select(float texWidth, float texHeight, float scale_);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE BloomScale;
	SHADER_HANDLE PixelKernel;
};
class PSBloomVertical : public cPixelShader
{
public:
	PSBloomVertical(){};
	~PSBloomVertical(){};
	void Select(float texWidth, float texHeight, float scale_);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE BloomScale;
	SHADER_HANDLE PixelKernel;
};
class PSUnderWater : public cPixelShader
{
public:
	PSUnderWater(){};
	~PSUnderWater(){};
	void Select(float shift_,float scale_,sColor4f& color_);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE shift;
	SHADER_HANDLE scale;
	SHADER_HANDLE color;
};
class PSMirage : public cPixelShader
{
public:
	void Select();
protected:
	void RestoreShader();
};
class PSBlurMap : public cPixelShader
{
public:
	void Select(Vect2f& params);
protected:
	void GetHandle();
	void RestoreShader();
	SHADER_HANDLE dofParams;
};
class PSDOFCombine : public cPixelShader
{
public:
	void Select();
	void SetDofParams(Vect2f& params, float power);
protected:
	void RestoreShader();
	void GetHandle();
	SHADER_HANDLE dofParams;
	SHADER_HANDLE filterTaps;
	SHADER_HANDLE maxCoC;
};
//class PSFiledDistort : public cPixelShader
//{
//public:
//	PSFiledDistort(){};
//	~PSFiledDistort(){};
//	void Select(float shift_);
//protected:
//	virtual void RestoreShader();
//	virtual void GetHandle();
//	SHADER_HANDLE shift;
//};

class PSFont:public cPixelShader
{
public:
	virtual void RestoreShader();
};

class PSDown4 : public cPixelShader
{
public:
	virtual void Select(float texWidth, float texHeight);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE TexelCoordsDownFilter;

};

class PSOverdraw : public cPixelShader
{
public:
	virtual void Select();
protected:
	virtual void RestoreShader();
};
class PSOverdrawColor : public cPixelShader
{
public:
	virtual void Select();
protected:
	virtual void RestoreShader();
};
class PSOverdrawCalc : public cPixelShader
{
public:
	virtual void Select(float texWidth, float texHeight);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE PixelSum;
};

class VSGrass : public vsSceneShader
{
public:
	VSGrass()
	{
		oldLighting = true;
	}
	void Select(float time_, float hideDistance_, sColor4f sunDiffuse,bool toZBuffer = false);
	void SetOldLighting(bool enble);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	SHADER_HANDLE time;
	SHADER_HANDLE hideDistance;
	SHADER_HANDLE SunDiffuse;
	SHADER_HANDLE vCameraPos;
	SHADER_HANDLE vLightDirection;
	SHADER_HANDLE mShadow;
	SHADER_HANDLE mVP;
	SHADER_HANDLE mWorld;
	SHADER_HANDLE vDofParams;
	INDEX_HANDLE LIGHTMAP;
	INDEX_HANDLE OLD_LIGHTING;
	INDEX_HANDLE ZBUFFER;
	bool oldLighting;
};
class PSGrass : public psSceneShader
{
public:
	virtual void Select();
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	INDEX_HANDLE LIGHTMAP;
};
class PSGrassShadow : public PSGrass
{
public:
	virtual void Select();
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	void SelectShadowQuality();
	SHADER_HANDLE fx_offset;
	INDEX_HANDLE c2x2;
};

class PSGrassShadowFX : public PSGrassShadow
{
protected:
	void RestoreShader();
};
class PSMiniMap : public cPixelShader
{
public:
	PSMiniMap()
	{
		fogAlpha_ = 1.f;
	}
	virtual void Select(const sColor4f& waterColor_,const sColor4f& terraColor_);
	void SetUseWater(bool useWater)
	{
		if(USE_WATER.is())
			shader->Select(USE_WATER,useWater?1:0);
	}
	void SetUseFogOfWar(bool useFogOfWar)
	{
		if(USE_FOGOFWAR.is())
			shader->Select(USE_FOGOFWAR,useFogOfWar?1:0);
	}
	void SetUseTerraColor(bool useTerraColor)
	{
		if(USE_TERRA_COLOR.is())
			shader->Select(USE_TERRA_COLOR,useTerraColor?1:0);
	}
	void SetFogAlpha(float alpha){fogAlpha_ = alpha;}
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	SHADER_HANDLE waterColor;
	SHADER_HANDLE terraColor;
	SHADER_HANDLE fogAlpha;
	INDEX_HANDLE USE_WATER;
	INDEX_HANDLE USE_FOGOFWAR;
	INDEX_HANDLE USE_TERRA_COLOR;
	float fogAlpha_;
};
class PSZBuffer : public psSceneShader
{
public:
	virtual void Select(bool floatZBuffer);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	INDEX_HANDLE FLOATZBUFFER;
};
class PSSkinZBuffer:public cPixelShader
{
protected:
	virtual void RestoreShader();
};

class PSSkinZBufferAlpha:public PSSkinShadowAlpha
{
protected:
	virtual void RestoreShader();
};

class VSSkinZBuffer : public VSSkinShadow
{
protected:
	virtual void RestoreShader();
};

class VSTileZBuffer : public vsSceneShader
{
public:
	void Select();
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE mVP;
};

class VSZBuffer : public vsSceneShader
{
public:
	VSZBuffer();
	void Select(MatXf* world=NULL,int world_num=0,int blend_num=0,bool object=false);
	void SetWorldMatrix(MatXf* world,int world_offset,int world_num);
	void SetDofParams(Vect2f params);
	Vect2f &GetDofParams() {return DofParams;}
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	SHADER_HANDLE mVP;
	SHADER_HANDLE mWorldM;
	SHADER_HANDLE vDofParams;
	INDEX_HANDLE WEIGHT;
	INDEX_HANDLE OBJECT;
	Vect2f DofParams;
};
class PSFillColor : public cPixelShader
{
public:
	virtual void Select(sColor4f color);
	virtual void Select(float r, float g, float b, float a);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE vColor;
};


//////////////////////////////GloudShadow
class VSCloudShadow:public cVertexShader
{
protected:
	SHADER_HANDLE	mWVP;
public:
	virtual void Select(const MatXf& world=MatXf::ID);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();

};

class PSCloudShadow:public cPixelShader
{
	SHADER_HANDLE tfactor;
	SHADER_HANDLE tfactorm05;
public:
	virtual void Select(const sColor4f& ctfactor);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class PSBlobsShader:public cPixelShader
{
	SHADER_HANDLE pixel_size;
	SHADER_HANDLE blobs_color;
	SHADER_HANDLE specular_color;
	SHADER_HANDLE fade_phase;

public:
	virtual void Select(cTexture* pTexture, const D3DXVECTOR4& def_color, const D3DXVECTOR4& spec_color, float phase);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

#endif
