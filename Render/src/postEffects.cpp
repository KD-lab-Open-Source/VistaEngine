#include "StdAfxRD.h"
#include "postEffects.h"
#include "..\shader\shaders.h"
#include "..\..\Water\Water.h"

bool enableMirage = false;

PostEffectManager::PostEffectManager()
{
	isPS20_ = gb_RenderDevice3D->IsPS20();
	backBufferTexture = NULL;
	enable_ = false;
	width_ = 0;
	height_ = 0;
	for(int i=0; i<PE_END; i++)
		effects[i] = NULL;
}
PostEffectManager::~PostEffectManager()
{
	RELEASE(backBufferTexture);
	for(int i=0; i<PE_END; i++)
		delete effects[i];
}
void PostEffectManager::Init(int type)
{
	
	// Все это как то криво. Сделано для EffectTool, так как там нужен только один постэффект
	if(type >= 0 && type < PE_END)
	{
		if(type == PE_MONOCHROME)
			effects[PE_MONOCHROME] = new MonochromeEffect(this);
		else
		if(isPS20_)
		{
			switch(type){
				case PE_BLOOM:
					effects[PE_BLOOM]		= new BloomEffect(this);
					break;
				case PE_UNDER_WATER:
					effects[PE_UNDER_WATER] = new UnderWaterEffect(this);
					break;
				case PE_COLOR_DODGE:
					effects[PE_COLOR_DODGE] = new ColorDodgeEffect(this);
					break;
				case PE_DOF:
					effects[PE_DOF]			= new DOFEffect(this);
					break;
				case PE_MIRAGE:
					effects[PE_MIRAGE]		= new MirageEffect(this);
					break;
			}
		}
	}else
	{
		// Монохромный эффект создаем и без поддержки пиксельных шейдеров 2.0
		effects[PE_MONOCHROME] = new MonochromeEffect(this);

		if(isPS20_)
		{
			//effects[PE_BLOOM]		= new BloomEffect(this); // в Maelstrom отключен, так как нигде не используется,
			// а ресурсы жрет, для последующих проектов расскоментарить
			effects[PE_UNDER_WATER] = new UnderWaterEffect(this);
			effects[PE_COLOR_DODGE] = new ColorDodgeEffect(this);
			effects[PE_DOF]			= new DOFEffect(this);
			effects[PE_MIRAGE]		= new MirageEffect(this);
		}
	}

	for(int i=0; i<PE_END; i++)
	{
		if(effects[i])
			effects[i]->Init();
	}
	ChangeSize();
}

void PostEffectManager::Draw(float dt)
{
	ChangeSize();
	if(!enable_)
		return;
	for(int i=0; i<PE_END; i++)
	{
		if(effects[i])
			effects[i]->Draw(dt);
	}
}
PostEffect* PostEffectManager::GetEffect(PostEffectType type)
{
	if(type < PE_END)
		return effects[type];
	return NULL;
}
int PostEffectManager::GetTexturesSize(PostEffectType type)
{
	if(type < PE_END)
		return effects[type]->GetTexturesSize();
	return 0;
}
int PostEffectManager::GetBaseTexturesSize()
{
	if(backBufferTexture)
		return backBufferTexture->CalcTextureSize();
	return 0;
}

bool PostEffectManager::IsActive(PostEffectType type)
{
	if(type < PE_END)
		if(effects[type])
			return effects[type]->IsActive();
	return false;
}

void PostEffectManager::SetActive(PostEffectType type, bool active)
{
	if(type < PE_END)
		if(effects[type])
			effects[type]->SetActive(active);
}
void PostEffectManager::CreateTextures()
{
	RELEASE(backBufferTexture);
	backBufferTexture = GetTexLibrary()->CreateRenderTexture(width_,height_,TEXTURE_RENDER32);
	if(!backBufferTexture)
	{
		enable_ = false;
		return;
	}
	for(int i=0; i<PE_END; i++)
	{
		if(effects[i])
		{
			effects[i]->SetSize(width_,height_);
			effects[i]->CreateTextures();
		}
	}
	enable_ = true;
}
cTexture* PostEffectManager::GetBackBufferTexture()
{
	if(!backBufferTexture)
		return NULL;
	IDirect3DSurface9 *pDestSurface=NULL;
	RDCALL(backBufferTexture->GetDDSurface(0)->GetSurfaceLevel(0,&pDestSurface));
	RDCALL(gb_RenderDevice3D->lpD3DDevice->StretchRect(
		gb_RenderDevice3D->lpBackBuffer,NULL,pDestSurface,NULL,D3DTEXF_LINEAR));
	RELEASE(pDestSurface);
	return backBufferTexture;
}

bool PostEffectManager::ChangeSize()
{
	xassert(gb_RenderDevice3D);
	D3DSURFACE_DESC desc;
	gb_RenderDevice3D->lpBackBuffer->GetDesc(&desc);
	if(desc.Width!=width_ || desc.Height != height_)
	{
		width_ = desc.Width;
		height_ = desc.Height;
		CreateTextures();
	}
	return false;
}

void PostEffect::SetRenderState()
{
	oldZ_ = gb_RenderDevice3D->GetRenderState(D3DRS_ZENABLE);
	oldAlpha_ = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHABLENDENABLE);
	oldAlphaTest_ = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHATESTENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE,D3DZB_FALSE );
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE,false);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE,false);
}
void PostEffect::RestoreRenderState()
{
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE,oldZ_);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE,oldAlpha_);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE,oldAlphaTest_);
	gb_RenderDevice3D->SetPixelShader(NULL);
}
void PostEffect::Draw(float dt)
{
	if(!enable_)
		return;
	DrawEffect(dt);
}
void PostEffect::Init()
{
	//isPS20_ = gb_RenderDevice3D->IsPS20();
	//ChangeBackBufferSize();
	//CreateTextures();
}
void PostEffect::SetSize(int width,int height)
{
	width_ = width;
	height_ = height;
}

ColorDodgeEffect::ColorDodgeEffect(PostEffectManager* manager) : PostEffect(manager)
{
	texture1 = NULL;
	texture2 = NULL;
	psShader = NULL;
	speed = 1;
	fadeSpeed_ = 1;
	phase_ = 0.0f;
	process = false;
	fadeIn = false;
	textureName = "Scripts\\Resource\\Textures\\twirl.tga";
}
ColorDodgeEffect::~ColorDodgeEffect()
{
	RELEASE(texture1);
	RELEASE(texture2);
	delete psShader;
}
void ColorDodgeEffect::SetTexture(const char* name)
{
	textureName = name;
	CreateTextures();
}

void ColorDodgeEffect::CreateTextures()
{
	RELEASE(texture1);
	RELEASE(texture2);
	texture1 = GetTexLibrary()->CreateRenderTexture(width_,height_,TEXTURE_RENDER32);
	texture2 = GetTexLibrary()->GetElement2D(textureName.c_str());
	center.set(width_*0.5f,height_*0.5f);
	size = width_*1.414213562373f*0.5f;
	if(texture2&&texture1)
		enable_ = true;
	else
		enable_ = false;
}
void ColorDodgeEffect::Init()
{
	__super::Init();	
	psShader = new PSColorDodge();
	psShader->Restore();
}
void ColorDodgeEffect::DrawEffect(float dtime)
{
	if((!isActive_&&!process)||textureName.empty())
		return;
	static float angle;
	angle+=dtime*speed;
	float dt = dtime; //*1e-3f;
	if (process)
	{
		if (fadeIn)
		{
			phase_+=fadeSpeed_*dt;
			if (phase_>1)
			{
				phase_ = 1.0f;
				isActive_ = true;
				process = false;
			}
		}else
		{
			phase_-=fadeSpeed_*dt;
			if (phase_ < 0)
			{
				phase_ = 0.0f;
				isActive_ = false;
				process = false;
			}
		}
	}
	Vect2f c1(-size,-size);
	Vect2f c2(-size,+size);
	Vect2f c3(+size,-size);
	Vect2f c4(+size,+size);

	Mat2f mat(angle);
	c1 *= mat;
	c2 *= mat;
	c3 *= mat;
	c4 *= mat;

	SetSamplerState();
	SetRenderState();
	gb_RenderDevice3D->SetRenderTarget(texture1,NULL);
	gb_RenderDevice3D->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,texture2);
	cVertexBuffer<sVertexXYZWDT3>* Buf=gb_RenderDevice3D->GetBufferXYZWDT3();
	sVertexXYZWDT3* v=Buf->Lock(4);
	v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
	v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=sColor4c(255,255,255);
	v[0].x=-0.5f+(float)c1.x+center.x; v[0].y = -0.5f+(float)c1.y+center.y;
	v[1].x=-0.5f+(float)c2.x+center.x; v[1].y = -0.5f+(float)c2.y+center.y;
	v[2].x=-0.5f+(float)c3.x+center.x; v[2].y = -0.5f+(float)c3.y+center.y;
	v[3].x=-0.5f+(float)c4.x+center.x; v[3].y = -0.5f+(float)c4.y+center.y;

	v[0].u1()=v[0].u2()=v[0].u3() = 0; v[0].v1()=v[0].v2()=v[0].v3()=0;
	v[1].u1()=v[1].u2()=v[1].u3() = 0; v[1].v1()=v[1].v2()=v[1].v3()=1;
	v[2].u1()=v[2].u2()=v[2].u3() = 1; v[2].v1()=v[2].v2()=v[2].v3()=0;
	v[3].u1()=v[3].u2()=v[3].u3() = 1; v[3].v1()=v[3].v2()=v[3].v3()=1;
	Buf->Unlock(4);

	Buf->DrawPrimitive(PT_TRIANGLESTRIP,2);

	cTexture* backBuffer = manager_->GetBackBufferTexture();
	gb_RenderDevice3D->RestoreRenderTarget();
	gb_RenderDevice3D->SetTexture(0,backBuffer);
	gb_RenderDevice3D->SetTexture(1,texture1);
	psShader->Select(phase_);
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);
	RestoreRenderState();
}
void ColorDodgeEffect::SetSamplerState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_clamp_linear);
}

MonochromeEffect::MonochromeEffect(PostEffectManager* manager) : PostEffect(manager)
{
	psShader = NULL;
	phase_ = 0.0f;
	process = false;
	speed = 1.0f;
	fadeIn = false;
	timePrev_ = 0;
}

MonochromeEffect::~MonochromeEffect()
{
	delete psShader;
}
void MonochromeEffect::CreateTextures()
{
	enable_ = true;
}

void MonochromeEffect::Init()
{
	__super::Init();	
	psShader = new PSMonochrome();
	psShader->Restore();
	timePrev_ = xclock();
}
void MonochromeEffect::DrawEffect(float timeUnused)
{
	if ((!isActive_&&!process))
		return;
	int time = xclock();
	float dt = float(time - timePrev_)*1e-3f;
	if (process)
	{
		if (fadeIn)
		{
			phase_+=speed*dt;
			if (phase_>1)
			{
				phase_ = 1.0f;
				isActive_ = true;
				process = false;
			}
		}else
		{
			phase_-=speed*dt;
			if (phase_ < 0)
			{
				phase_ = 0.0f;
				isActive_ = false;
				process = false;
			}
		}
	}
	SetSamplerState();
	SetRenderState();
	cTexture* backBuffer = manager_->GetBackBufferTexture();
	gb_RenderDevice3D->SetTexture(0,backBuffer);
	psShader->Select(phase_);
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);
	RestoreRenderState();
}
void MonochromeEffect::SetSamplerState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_point);
}
int MonochromeEffect::GetTexturesSize()
{
	return 0;
}
BloomEffect::BloomEffect(PostEffectManager* manager) : PostEffect(manager)
{
	texture2 = NULL;
	texture3 = NULL;
	tempTexture = NULL;
	psColorBright = NULL;
	psCombine = NULL;
	psBloomH = NULL;
	psBloomV = NULL;
	luminance_ = 0.08f;
	bloomScale_ = 1.5f;
	addColor = sColor4f(0,0,0,0);
	exlpodeEnable_ = false;
	defaultLuminance_ = 0.08f;
}
BloomEffect::~BloomEffect()
{
	RELEASE(texture2);
	RELEASE(texture3);
	RELEASE(tempTexture);
	delete psColorBright;
	delete psCombine;
	delete psBloomH;
	delete psBloomV;
	delete psMonochrome;
	delete psDown4;
}
void BloomEffect::CreateTextures()
{
	RELEASE(texture2);
	RELEASE(texture3);
	RELEASE(tempTexture);
	texture2 = GetTexLibrary()->CreateRenderTexture(width_/16,height_/16,TEXTURE_RENDER32);
	texture3 = GetTexLibrary()->CreateRenderTexture(width_/16,height_/16,TEXTURE_RENDER32);
	tempTexture = GetTexLibrary()->CreateRenderTexture(width_/4,height_/4,TEXTURE_RENDER32);
	if(tempTexture&&texture2&&texture3)
		enable_ = true;
	else
		enable_ = false;
}
void BloomEffect::Init()
{
	__super::Init();
	psColorBright = new PSColorBright();
	psColorBright->Restore();
	psCombine = new PSCombine();
	psCombine->Restore();
	psBloomH = new PSBloomHorizontal();
	psBloomH->Restore();
	psBloomV = new PSBloomVertical();
	psBloomV->Restore();
	psMonochrome = new PSMonochrome();
	psMonochrome->Restore();
	psDown4 = new PSDown4();
	psDown4->Restore();
}
int BloomEffect::GetTexturesSize()
{
	int size=0;
	if (texture2&&texture3&&tempTexture)
	{
		size+=texture2->CalcTextureSize();
		size+=texture3->CalcTextureSize();
		size+=tempTexture->CalcTextureSize();
	}
	return size;
}

void BloomEffect::RestoreDefaults()
{
	luminance_ = defaultLuminance_;
	addColor = sColor4f(0,0,0,0);
}
void BloomEffect::SetExplode(float enable)
{
	exlpodeEnable_ = enable;
}

void BloomEffect::SetSamplerState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_clamp_linear);
}
void BloomEffect::DrawEffect(float dt)
{
	if((!isActive_&&!exlpodeEnable_))
		return;
	SetSamplerState();
	SetRenderState();
	cTexture* backBuffer = manager_->GetBackBufferTexture();

	gb_RenderDevice3D->SetRenderTarget(tempTexture,NULL);
	gb_RenderDevice3D->SetTexture(0,backBuffer);
	psDown4->Select(width_,height_);
	gb_RenderDevice3D->DrawQuad(0,0,tempTexture->GetWidth(),tempTexture->GetHeight(),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture3,NULL);
	gb_RenderDevice3D->SetTexture(0,tempTexture);
	psDown4->Select(tempTexture->GetWidth(),tempTexture->GetWidth());
	gb_RenderDevice3D->DrawQuad(0,0,texture3->GetWidth(),texture3->GetHeight(),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture2,NULL);
	gb_RenderDevice3D->SetTexture(0,texture3);
	psColorBright->Select(luminance_);
	gb_RenderDevice3D->DrawQuad(0,0,texture2->GetWidth(),texture2->GetHeight(),0,0,1,1);

	//gb_RenderDevice3D->SetRenderTarget(texture3,NULL);
	//gb_RenderDevice3D->SetTextureBase(texture2,0);
	//psMonochrome->Select(1.0f);
	//DrawQuad(0,0,texture3->GetWidth(),texture3->GetHeight(),0,0,1,1);
	gb_RenderDevice3D->SetRenderTarget(texture3,NULL);
	gb_RenderDevice3D->SetTexture(0,texture2);
	psBloomV->Select(texture3->GetWidth(),texture3->GetHeight(),bloomScale_);
	gb_RenderDevice3D->DrawQuad(0,0,texture3->GetWidth(),texture3->GetHeight(),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture2,NULL);
	gb_RenderDevice3D->SetTexture(0,texture3);
	psBloomH->Select(texture2->GetWidth(),texture2->GetHeight(),bloomScale_);
	gb_RenderDevice3D->DrawQuad(0,0,texture2->GetWidth(),texture2->GetHeight(),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture3,NULL);
	gb_RenderDevice3D->SetTexture(0,texture2);
	psBloomV->Select(texture3->GetWidth(),texture3->GetHeight(),bloomScale_);
	gb_RenderDevice3D->DrawQuad(0,0,texture3->GetWidth(),texture3->GetHeight(),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture2,NULL);
	gb_RenderDevice3D->SetTexture(0,texture3);
	psBloomH->Select(texture2->GetWidth(),texture2->GetHeight(),bloomScale_);
	gb_RenderDevice3D->DrawQuad(0,0,texture2->GetWidth(),texture2->GetHeight(),0,0,1,1);

	gb_RenderDevice3D->RestoreRenderTarget();

	gb_RenderDevice3D->SetTexture(0,backBuffer);
	gb_RenderDevice3D->SetTexture(1,texture2);
	psCombine->Select(addColor);
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);
	RestoreRenderState();
}

UnderWaterEffect::UnderWaterEffect(PostEffectManager* manager) : PostEffect(manager)
{
	psUnderWater = NULL;
	psBlurH = NULL;
	psBlurV = NULL;
	psMonochrome = NULL;
	shift_ = 0.0f;
	scale_ = 0.0f;
	delta_ = 10;
	underWater_ = false;
	waveSpeed_ = 0.01f;
	color_ = sColor4f(0.0f,0.0f,0.2f);
	fogPlanes = Vect2f(100,1000);
	waveTexture = NULL;
	workTexture = NULL;
	activeAlways_ = false;
	activateUnderWater = false;
	isActive_ = true;
}

UnderWaterEffect::~UnderWaterEffect()
{
	RELEASE(waveTexture);
	RELEASE(workTexture);

	delete psBlurH;
	delete psBlurV;
	delete psUnderWater;
	delete psMonochrome;
}
void UnderWaterEffect::SetSamplerState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_wrap_linear);
}
void UnderWaterEffect::CreateTextures()
{
	RELEASE(workTexture);
	workTexture = GetTexLibrary()->CreateRenderTexture(width_,height_,TEXTURE_RENDER32);
	if(workTexture)
		enable_ = true;
	else
		enable_ = false;
}
void UnderWaterEffect::Init()
{
	__super::Init();
	psBlurH = new PSBloomHorizontal();
	psBlurH->Restore();
	psBlurV = new PSBloomVertical();
	psBlurV->Restore();
	psMonochrome = new PSMonochrome();
	psMonochrome->Restore();
	psUnderWater = new PSUnderWater();
	psUnderWater->Restore();
}
int UnderWaterEffect::GetTexturesSize()
{
	int size=0;
	if (workTexture)
	{
		size+=workTexture->CalcTextureSize();
	}
	return size;
}
void UnderWaterEffect::SetIsUnderWater(bool under_water)
{
	activateUnderWater = under_water;
}

void UnderWaterEffect::DrawEffect(float dtime)
{
	if(!isActive_ || !waveTexture)
		return;
	float dt = dtime; //*1e-3f;
	shift_ += waveSpeed_*dt;
	if (!activeAlways_)
	{
		if (!activateUnderWater)
		{
			delta_ = -10;
		}
		else 
		{
			delta_ = 10;
			underWater_ = true;
		}

		scale_ += delta_*dt;
		if (scale_>1.0f)
			scale_ = 1.0f;
		if (scale_ < 0.0f)
		{
			scale_ = 0.0;
			underWater_ = false;
		}
		if (!underWater_)
			return;
	}else
		scale_ = 1.0f;
	SetRenderState();
	cTexture* backBuffer = manager_->GetBackBufferTexture();

	gb_RenderDevice3D->SetTexture(0,backBuffer);
	gb_RenderDevice3D->SetTexture(1,waveTexture);
	SetSamplerState();
	psUnderWater->Select(shift_,scale_,color_);
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);
	RestoreRenderState();
}
void UnderWaterEffect::SetFog(sColor4f& clr)
{
	Vect2f fog;
	if (scale_ < 1)
	{
		fog.x = LinearInterpolate(environmentFog.x,fogPlanes.x,scale_);
		fog.y = LinearInterpolate(environmentFog.y,fogPlanes.y,scale_);
		gb_RenderDevice3D->SetGlobalFog(clr,fog);
	}else
	{
		gb_RenderDevice3D->SetGlobalFog(clr,Vect2f(fogPlanes.x,fogPlanes.y));
	}
}
void UnderWaterEffect::SetTexture(string &name)
{
	RELEASE(waveTexture);
	waveTexture = GetTexLibrary()->GetElement2D(name.c_str());
}

DOFEffect::DOFEffect(PostEffectManager* manager) : PostEffect(manager)
{
	//texture2 = NULL;
	//texture3 = NULL;
	tempTexture = NULL;
	//psBloomH = NULL;
	//psBloomV = NULL;
	//psDown4 = NULL;
	psDOFCombine = NULL;
	psBlur = NULL;
	dofParams.set(200.f,1.f/200);
	dofPower = 4.f;
}
DOFEffect::~DOFEffect()
{
	//RELEASE(texture2);
	//RELEASE(texture3);
	RELEASE(tempTexture);
	//delete psBloomH;
	//delete psBloomV;
	//delete psDown4;
	delete psDOFCombine;
	delete psBlur;

}
void DOFEffect::CreateTextures()
{
	//RELEASE(texture2);
	//RELEASE(texture3);
	RELEASE(tempTexture);
	//texture2 = GetTexLibrary()->CreateRenderTexture(width_/2,height_/2,TEXTURE_RENDER32);
	//texture3 = GetTexLibrary()->CreateRenderTexture(width_/2,height_/2,TEXTURE_RENDER32);
	//tempTexture = GetTexLibrary()->CreateRenderTexture(width_/2,height_/2,TEXTURE_RENDER32);
	tempTexture = GetTexLibrary()->CreateRenderTexture(width_,height_,TEXTURE_RENDER32);
	if(tempTexture)
		enable_ = true;
	else
		enable_ = false;
}
void DOFEffect::Init()
{
	__super::Init();
	//psBloomH = new PSBloomHorizontal();
	//psBloomH->Restore();
	//psBloomV = new PSBloomVertical();
	//psBloomV->Restore();
	//psDown4 = new PSDown4();
	//psDown4->Restore();
	psDOFCombine = new PSDOFCombine;
	psDOFCombine->Restore();
	psBlur = new PSBlurMap;
	psBlur->Restore();

}
int DOFEffect::GetTexturesSize()
{
	int size=0;
	if (/*texture2&&texture3&&*/tempTexture)
	{
		//size+=texture2->CalcTextureSize();
		//size+=texture3->CalcTextureSize();
		size+=tempTexture->CalcTextureSize();
	}
	return size;
}
void DOFEffect::SetDofPower(float power)
{
	dofPower = power;
}

void DOFEffect::SetDofParams(Vect2f &params)
{
	//gb_RenderDevice3D->dtAdvance->pVSZBuffer->SetDofParams(params);
	dofParams.x = params.x;
	if((params.y-params.x)>FLT_EPS)
		//dofParams.y = 2.f/params.y; //формула для размывания вблизи (правильная)
		dofParams.y = 1.f/(params.y-params.x); 
	else
		dofParams.y=1;
}

void DOFEffect::DrawEffect(float dt)
{
	if(!Option_UseDOF)
		return;
	SetSamplerState();
	SetRenderState();
	IDirect3DSurface9 *pDestSurface=NULL;
	cTexture* backBuffer = manager_->GetBackBufferTexture();

	//gb_RenderDevice3D->SetRenderTarget(texture3,NULL);
	//gb_RenderDevice3D->SetTexture(0,backBuffer);
	//psDown4->Select(tempTexture->GetWidth(),tempTexture->GetHeight());
	//gb_RenderDevice3D->DrawQuad(0,0,texture3->GetWidth(),texture3->GetHeight(),0,0,1,1);

	//gb_RenderDevice3D->SetRenderTarget(texture2,NULL);
	//gb_RenderDevice3D->SetTexture(0,texture3);
	//psBloomH->Select(texture2->GetWidth(),texture2->GetHeight(),1);
	//gb_RenderDevice3D->DrawQuad(0,0,texture2->GetWidth(),texture2->GetHeight(),0,0,1,1);

	//gb_RenderDevice3D->SetRenderTarget(texture3,NULL);
	//gb_RenderDevice3D->SetTexture(0,texture2);
	//psBloomV->Select(texture3->GetWidth(),texture3->GetHeight(),1);
	//gb_RenderDevice3D->DrawQuad(0,0,texture3->GetWidth(),texture3->GetHeight(),0,0,1,1);
	gb_RenderDevice3D->SetRenderTarget(tempTexture,NULL);
	gb_RenderDevice3D->SetTexture(0,backBuffer);
	gb_RenderDevice3D->SetTexture(1,gb_RenderDevice3D->dtFixed->GetFloatMap());
	psBlur->Select(dofParams);
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);


	gb_RenderDevice3D->RestoreRenderTarget();

	gb_RenderDevice3D->SetTexture(0,tempTexture);
	//gb_RenderDevice3D->SetTexture(1,tempTexture);
	gb_RenderDevice3D->SetTexture(1,gb_RenderDevice3D->dtFixed->GetFloatMap());
	psDOFCombine->SetDofParams(dofParams,dofPower);
	psDOFCombine->Select();
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);
	RestoreRenderState();

}

MirageEffect::MirageEffect(PostEffectManager* manager) : PostEffect(manager)
{
	waveTexture = NULL;
	psMirage = NULL;
	psFillColor = NULL;
	isActive_=true;
}
MirageEffect::~MirageEffect()
{
	RELEASE(waveTexture);
	delete psMirage; 
	delete psFillColor;
}
void MirageEffect::Init()
{
	__super::Init();
	psMirage = new PSMirage();
	psMirage->Restore();
	psFillColor = new PSFillColor();
	psFillColor->Restore();
}
void MirageEffect::DrawEffect(float dt)
{
	if(!isActive_)
		return;
	if (!enableMirage || !waveTexture)
		return;
	SetRenderState();
	cTexture* backBuffer = manager_->GetBackBufferTexture();
	
	IDirect3DSurface9 *pDestSurface=NULL;
	RDCALL(waveTexture->GetDDSurface(0)->GetSurfaceLevel(0,&pDestSurface));
	RDCALL(gb_RenderDevice3D->lpD3DDevice->StretchRect(
		gb_RenderDevice3D->dtFixed->GetMirageMap(),NULL,pDestSurface,NULL,D3DTEXF_LINEAR));
	pDestSurface->Release();

	gb_RenderDevice3D->SetTexture(0,backBuffer);
	gb_RenderDevice3D->SetTexture(1,waveTexture);
	SetSamplerState();
	psMirage->Select();
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);
	RestoreRenderState();
	enableMirage = false;
}
void MirageEffect::CreateTextures()
{
	RELEASE(waveTexture);
	waveTexture = GetTexLibrary()->CreateRenderTexture(width_,height_,TEXTURE_RENDER32);
	if(waveTexture)
		enable_ = true;
	else
		enable_ = false;
}
int MirageEffect::GetTexturesSize()
{
	if(waveTexture)
		return waveTexture->CalcTextureSize();
	return 0;
}

void MirageEffect::SetSamplerState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_clamp_linear);
}

