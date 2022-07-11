#ifndef __SHADERS_H_INCLUDED__
#define __SHADERS_H_INCLUDED__

#include "XMath\Colors.h"
#include "XMath\Mat4f.h"
#include "Render\src\Texture.h"
#include "Render\shader\ShaderStorage.h"
#include "Render\D3d\renderstates.h"

struct sDataRenderMaterial
{
	Color4f	Ambient;
	Color4f	Diffuse;
	Color4f	Specular; //Specular.a=Power
	Color4f lerp_texture;

	cTexture* Tex[2];
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
	void Restore();
	void Delete();
	virtual void Select();

	inline void setMatrixVS(const SHADER_HANDLE& h,const Mat4f& mat);

protected:
	cShaderStorage*  shaderVS_;
	cShaderStorage*  shaderPS_;

	virtual void RestoreShader()=0;
	virtual void GetHandle(){}

	void LoadShaderVS(const char* name);
	void LoadShaderPS(const char* name);

	inline void setVectorVS(const SHADER_HANDLE& h,const Vect4f& vect);
	inline void setFloatVS(const SHADER_HANDLE& h,const float f);

	inline void setMatrix4x4VS(const SHADER_HANDLE& h,int index,const Mat4f& mat);
	inline void setMatrix4x3VS(const SHADER_HANDLE& h,int index,const Mat4f& mat);
	inline void setMatrix4x3VS(const SHADER_HANDLE& h,int index,const MatXf& mat);

	inline void setVectorPS(const SHADER_HANDLE& h,const Vect4f& vect);
	inline void setVectorPS(const SHADER_HANDLE& h,const Vect4f* vects,const int count);
};

class RENDER_API cVertexShader : public cShader
{
};

class RENDER_API cPixelShader : public cShader
{
};

class vsSceneShader : public cVertexShader
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

class RENDER_API psSceneShader : public cPixelShader
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

	void SetShadowIntensity(const Color4f& f);
};

class ShaderScene : public cShader
{
public:
	void GetHandle();
	virtual void RestoreShader()=0;
	void SetFog();
	void SetShadowIntensity(const Color4f& f);

protected:
	// VS
	SHADER_HANDLE vFog;
	SHADER_HANDLE mView;

	SHADER_HANDLE fPlanarNode;
	INDEX_HANDLE FOG_OF_WAR;

	// PS
	SHADER_HANDLE vFogOfWar;
	SHADER_HANDLE vShade;
	INDEX_HANDLE FOG_OF_WAR_PS;
};

class PSShowMap : public cPixelShader
{
public:
	virtual void RestoreShader();
};

class PSShowAlpha : public cPixelShader
{
public:
	virtual void RestoreShader();
};

class VSShadow : public cVertexShader
{
protected:
	SHADER_HANDLE mWVP;
public:
	void Select();
	void RestoreShader();
	void GetHandle();
};

class PSShadow : public cPixelShader
{
public:
	virtual void RestoreShader();
	void GetHandle();
};

class ShaderSceneTileMap : public ShaderScene
{
public:
	void Select();
	
	virtual void SetUV(Vect2f& uv_base,Vect2f& uv_step,Vect2f& uv_base_bump,Vect2f& uv_step_bump);
	virtual void SetWorldSize(Vect2f sz);

	void SetMiniTextureSize(int dx,int dy,float res);

	void SetColor(const Color4f& color);
	virtual void SetShadowType(SHADOW_SHADER s){}
	virtual void SetReflectionZ(bool reflection);

protected:
	// VS
	SHADER_HANDLE mWVP;
	SHADER_HANDLE vLightDirection;
	SHADER_HANDLE UV;
	SHADER_HANDLE UVbump;
	SHADER_HANDLE vColor;
	SHADER_HANDLE mulMiniTexture;
	SHADER_HANDLE vReflectionMul;
	INDEX_HANDLE ZREFLECTION;
	INDEX_HANDLE VERTEX_LIGHT;

	// PS
	INDEX_HANDLE ZREFLECTION_PS;
	INDEX_HANDLE FILTER_SHADOW;
	INDEX_HANDLE VERTEX_LIGHT_PS;
	SHADER_HANDLE light_color;

	void GetHandle();
};

////////////////////////////9700/////////////////////////////////////

class ShaderSceneTileMap9700 : public ShaderSceneTileMap
{
public:
	void Select();

protected:
	// VS
	SHADER_HANDLE mShadow;

	// PS
	SHADER_HANDLE inv_light_dir;
	INDEX_HANDLE DETAIL_TEXTURE;

	void RestoreShader();
	void GetHandle();
};

////////////////////////////GeforceFX///////////////////////////////////
class ShaderSceneTileMapGeforceFX : public ShaderSceneTileMap
{
public:
	void Select();

protected:
	// VS
	INDEX_HANDLE VERTEX_LIGHT;
	SHADER_HANDLE mShadow;

	// PS
	SHADER_HANDLE inv_light_dir;
	SHADER_HANDLE fx_offset;
	INDEX_HANDLE DETAIL_TEXTURE;

	void RestoreShader();
	void GetHandle();
};

/////////////////////Chaos///////////////////////////
class VSChaos : public cVertexShader
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

class PSChaos : public cPixelShader
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

class VSSkinBase : public vsSceneShader
{
public:
	virtual void Select(MatXf* world,int world_num,int blend_num)=0;
	virtual void SetMaterial(sDataRenderMaterial *Data)=0;
	virtual void SetUVTrans(float mat[6])=0;
	virtual void SetSecondOpacityUVTrans(float mat[6],SECOND_UV use_t1){xassert(0);};
};

class VSSkin : public VSSkinBase
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
			shaderVS_->Select(ZREFLECTION,reflection?1:0);
	}

	void SetUVTrans(float mat[6]);
	virtual void SetAlphaColor(const Color4f& color);

	void SetWorldMatrix(MatXf* world,int world_offset,int world_num);//Для cSimply3dx
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	void CalcLightParameters(cUnkLight* light,SHADER_HANDLE& spos,SHADER_HANDLE& scolor,Vect2f& attenuation);
};

class VSSkinReflection : public VSSkin
{
protected:
	void RestoreShader();
};

class VSSkinSecondOpacity : public VSSkin
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

class VSSkinSceneShadow : public VSSkin
{
protected:
	SHADER_HANDLE mShadow;
public:
	void Select(MatXf* world,int world_num,int blend_num);
protected:
	void GetHandle();
	void RestoreShader();
};

class VSSkinNoLight : public VSSkin
{
protected:
	virtual void RestoreShader();
};

class PSSkin : public psSceneShader
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
	INDEX_HANDLE FILTER_SHADOW;
	INDEX_HANDLE ZREFLECTION;
	INDEX_HANDLE LERP_TEXTURE_COLOR;
public:
	PSSkin(){}
	void Select();
	virtual void SetMaterial(sDataRenderMaterial *Data,bool is_big_ambient);
	void SetReflection(int n, const Color4f& amount);
	void SetSelfIllumination(bool on);
	void SetAlphaColor(const Color4f& color);

	void SelectSpecularMap(cTexture* pSpecularmap,float phase);//Только для bump, обязательно вызывать в bump шейдерах, потому как выставляет SPECULARMAP
	void SetReflectionZ(bool reflection)
	{
		if(ZREFLECTION.is())
			shaderPS_->Select(ZREFLECTION,reflection?1:0);
	}
protected:
	virtual void GetHandle();
	virtual void RestoreShader()=0;
};

class PSSkinNoShadow : public PSSkin
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

class PSSkinReflection : public PSSkinNoShadow
{
protected:
	virtual void RestoreShader();
};

class PSSkinSecondOpacity : public PSSkinNoShadow
{
protected:
	void GetHandle();
	void RestoreShader();
	void Select();
};

class PSSkinSceneShadow : public PSSkin
{
public:
	void Select();
protected:
	SHADER_HANDLE fx_offset;

	virtual void GetHandle();
	virtual void RestoreShader();
};

class PSSkinBumpSceneShadow : public PSSkinSceneShadow
{
protected:
	virtual void RestoreShader();
};

class PSSkinSceneShadowFX : public PSSkinSceneShadow
{
protected:
	virtual void RestoreShader();
};

class PSSkinBumpSceneShadowFX : public PSSkinSceneShadowFX
{
protected:
	virtual void RestoreShader();
};

class PSSkinReflectionSceneShadowFX : public PSSkinSceneShadowFX
{
protected:
	virtual void RestoreShader();
};

class PSSkinReflectionSceneShadow : public PSSkinSceneShadow
{
protected:
	virtual void RestoreShader();
};

class VSSkinReflectionSceneShadow : public VSSkinSceneShadow
{
protected:
	virtual void RestoreShader();
};

class PSSkinBump : public PSSkin
{
public:
	void Select();
protected:
	void RestoreShader();
};


class PSSkinNoLight : public PSSkin
{
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class VSSkinBump : public VSSkin
{
protected:
	virtual void RestoreShader();
};

class VSSkinBumpSceneShadow : public VSSkinSceneShadow
{
protected:
	virtual void RestoreShader();
};

class VSSkinShadow : public VSSkinBase
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

class PSSkinShadow : public cPixelShader
{
protected:
	virtual void RestoreShader();
};

class PSSkinShadowAlpha : public cPixelShader
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
class RENDER_API VSWater : public vsSceneShader
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
	void SetMirrorMatrix(Camera* mirror);
	void EnableZBuffer(bool enable);

protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class RENDER_API PSWater : public psSceneShader
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
	Color4f flashColor_;
public:
	PSWater();
	void SetTechnique(int t);
	void setFlashColor(const Color4f& flashColor) { flashColor_ = flashColor; }
	void EnableZBuffer(bool enable);
	void Select();
	void GetHandle();
	void RestoreShader();
	void SetReflectionColor(const Color4f& color);
	void SetReflectionBrightnes(const float brightnes);
	void SetPS11Color(const Color4f& color);
};

////////////////////Minimal////////////////
class ShaderSceneTileMapMinimal : public ShaderSceneTileMap
{
public:
	void Select();

protected:
	SHADER_HANDLE inv_light_dir;

	void RestoreShader();
	void GetHandle();
};

////////////////////////////
class RENDER_API ShaderSceneWaterLava : public ShaderScene
{
public:
	ShaderSceneWaterLava(bool convertZ = false);
	~ShaderSceneWaterLava();

	void setTextureScale(float ground, float volume) { textureScaleValue_.set(ground, volume, 0, 0); }
	void SetColors(const Color4f& lava,const Color4f& ambient);
	void SetTime(double time);
	void Select();

protected:
	// VS
	SHADER_HANDLE	mVP;
	SHADER_HANDLE	fTime;
	SHADER_HANDLE	textureScale;
	INDEX_HANDLE CONVERT_Z;
	double time;
	bool convertZ_;
	Vect4f textureScaleValue_;

	// PS
	IDirect3DBaseTexture9* pRandomVolume;
	SHADER_HANDLE	vLavaColor;
	SHADER_HANDLE	vLavaColorAmbient;

	void GetHandle();
	void RestoreShader();
};

//////////////////////////////standart
class RENDER_API VSStandart : public vsSceneShader
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
			shaderVS_->Select(ZREFLECTION,reflection?1:0);
	}

protected:
	virtual void GetHandle();
	virtual void RestoreShader();

};

class RENDER_API PSStandart : public psSceneShader
{
public:
	void Select();
	void SetColorOperation(int op);
	void EnableFloatZBuffer(bool enable);
	void SetReflectionZ(bool reflection)
	{
		if(ZREFLECTION.is())
			shaderPS_->Select(ZREFLECTION,reflection?1:0);
	}

protected:
	INDEX_HANDLE COLOR_OPERATION;
	INDEX_HANDLE FLOAT_ZBUFFER;
	INDEX_HANDLE ZREFLECTION;

	virtual void GetHandle();
	virtual void RestoreShader();
};

//////////////////////////
class RENDER_API ShaderSceneWaterIce : public ShaderScene
{
public:
	ShaderSceneWaterIce();
	void Select();
	void SetSnowColor(const Color4f& color);
	void SetMirrorMatrix(Camera* mirror);

	void beginDraw(cTexture* pTexture, cTexture* pBump, cTexture* pAlpha, cTexture* pCleft, int alphaRef, int borderColor);
	void endDraw();

protected:
	// VS
	SHADER_HANDLE mVP;
	SHADER_HANDLE uvScaleOffset;
	SHADER_HANDLE vCameraPos;
	SHADER_HANDLE vMirrorVP;
	SHADER_HANDLE fScaleBumpSnow;
	INDEX_HANDLE MIRROR_LINEAR;
	INDEX_HANDLE PS11;
	INDEX_HANDLE USE_ALPHA;

	SHADER_HANDLE vSnowColor;
	INDEX_HANDLE MIRROR_LINEAR_PS;
	INDEX_HANDLE PS11_PS;
	INDEX_HANDLE USE_ALPHA_PS;

	bool linear;

	int alpharef_;
	int zwrite_;
	SAMPLER_DATA sampler_border;

	void GetHandle();
	void RestoreShader();
};

class PSSolidColor : public cPixelShader
{
protected:
	void RestoreShader();
};

class RENDER_API PSMonochrome : public cPixelShader
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
	void Select(Color4f _color);
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
	void Select(float shift_,float scale_,Color4f& color_);
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

class PSFont : public cPixelShader
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
protected:
	virtual void RestoreShader();
};
class PSOverdrawColor : public cPixelShader
{
protected:
	virtual void RestoreShader();
};
class PSOverdrawCalc : public cPixelShader
{
public:
	void Select(float texWidth, float texHeight);
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
	void Select(float time_, float hideDistance_, Color4f sunDiffuse,bool toZBuffer = false);
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
	void Select();
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	INDEX_HANDLE LIGHTMAP;
};

class PSGrassShadow : public PSGrass
{
public:
	void Select();
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	SHADER_HANDLE fx_offset;
	INDEX_HANDLE FILTER_SHADOW;
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
		additionAlpha_ = 1.f;
	}
	void Select(const Color4f& waterColor_,const Color4f& terraColor_);
	void SetUseWater(bool useWater)
	{
		if(USE_WATER.is())
			shaderPS_->Select(USE_WATER,useWater?1:0);
	}
	void SetUseAdditionTexture(bool useFogOfWar, bool useInstallZones)
	{
		if(ADDITION_TEXTURE.is())
			shaderPS_->Select(ADDITION_TEXTURE, useFogOfWar ? 1 : (useInstallZones ? 2 : 0));
	}
	void SetUseTerraColor(bool useTerraColor)
	{
		if(USE_TERRA_COLOR.is())
			shaderPS_->Select(USE_TERRA_COLOR,useTerraColor?1:0);
	}
	void SetUseBorder(bool use_border)
	{
		if(USE_BORDER.is())
			shaderPS_->Select(USE_BORDER,use_border?1:0);
	}
	void SetAdditionAlpha(float alpha) { additionAlpha_ = alpha; }
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
	SHADER_HANDLE waterColor;
	SHADER_HANDLE terraColor;
	SHADER_HANDLE additionAlpha;
	INDEX_HANDLE USE_WATER;
	INDEX_HANDLE ADDITION_TEXTURE; // 1 - туман войны, 2 - занятая зона
	INDEX_HANDLE USE_TERRA_COLOR;
	INDEX_HANDLE USE_BORDER;
	float additionAlpha_;
};

class PSMiniMapBorder : public cPixelShader
{
public:

	void SetUseTexture(bool use_texture)
	{
		if(USE_TEXTURE.is())
			shaderPS_->Select(USE_TEXTURE,use_texture ? 1 : 0);
	}

	void SetUseBorder(bool use_border)
	{
		if(USE_BORDER.is())
			shaderPS_->Select(USE_BORDER,use_border ? 1 : 0);
	}

protected:

	INDEX_HANDLE USE_TEXTURE;
	INDEX_HANDLE USE_BORDER;

	virtual void GetHandle();
	virtual void RestoreShader();
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
class PSSkinZBuffer : public cPixelShader
{
protected:
	virtual void RestoreShader();
};

class PSSkinZBufferAlpha : public PSSkinShadowAlpha
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
	void Select(MatXf* world=0,int world_num=0,int blend_num=0,bool object=false);
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
	virtual void Select(Color4f color);
	virtual void Select(float r, float g, float b, float a);
protected:
	virtual void RestoreShader();
	virtual void GetHandle();
	SHADER_HANDLE vColor;
};

//////////////////////////////GloudShadow
class RENDER_API VSCloudShadow : public cVertexShader
{
protected:
	SHADER_HANDLE	mWVP;
public:
	virtual void Select(const MatXf& world=MatXf::ID);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();

};

class RENDER_API PSCloudShadow : public cPixelShader
{
	SHADER_HANDLE tfactor;
	SHADER_HANDLE tfactorm05;
public:
	virtual void Select(const Color4f& ctfactor);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class RENDER_API PSBlobsShader : public cPixelShader
{
	SHADER_HANDLE pixel_size;
	SHADER_HANDLE blobs_color;
	SHADER_HANDLE specular_color;
	SHADER_HANDLE fade_phase;

public:
	virtual void Select(cTexture* pTexture, const Vect4f& def_color, const Vect4f& spec_color, float phase);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class VSSkinFur : public VSSkin
{
protected:
	SHADER_HANDLE	vFurDistance;
public:
	void SetFurDistance(float dist);
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class VSDipCost : public cVertexShader
{
public:
	SHADER_HANDLE	pos;

protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

class PSDipCost : public cPixelShader
{
public:
protected:
	virtual void GetHandle();
	virtual void RestoreShader();
};

#endif
