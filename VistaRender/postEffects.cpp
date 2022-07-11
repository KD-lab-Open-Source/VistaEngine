#include "StdAfx.h"

#include "Serialization\Factory.h"

#include "Render\shader\shaders.h"
#include "Render\D3d\D3DRender.h"
#include "Water\Water.h"
#include "Render\Src\VisGeneric.h"

#include "postEffects.h"

extern bool enableMirage;

#define PE_RENDER_STATE_BACKUP \
	DWORD z_enable_rs_backup, alpha_blend_enable_rs_backup, alpha_test_enable_rs_backup; \
	z_enable_rs_backup = gb_RenderDevice3D->GetRenderState(D3DRS_ZENABLE); \
	alpha_blend_enable_rs_backup = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHABLENDENABLE); \
	alpha_test_enable_rs_backup = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHATESTENABLE); \
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE,D3DZB_FALSE ); \
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE,false); \
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE,false);

#define PE_RENDER_STATE_RESTORE \
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE, z_enable_rs_backup); \
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE, alpha_blend_enable_rs_backup); \
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE, alpha_test_enable_rs_backup); \
	gb_RenderDevice3D->SetPixelShader(0);

typedef Factory<PostEffectType, PostEffect, FactoryArg1<PostEffect, PostEffectManager*> > PostEffectFactory;

REGISTER_CLASS_IN_FACTORY(PostEffectFactory, PE_MIRAGE, PostEffectMirage);
REGISTER_CLASS_IN_FACTORY(PostEffectFactory, PE_DOF, PostEffectDOF);
REGISTER_CLASS_IN_FACTORY(PostEffectFactory, PE_BLOOM, PostEffectBloom);
REGISTER_CLASS_IN_FACTORY(PostEffectFactory, PE_UNDER_WATER, PostEffectUnderWater);
REGISTER_CLASS_IN_FACTORY(PostEffectFactory, PE_COLOR_DODGE, PostEffectColorDodge);
REGISTER_CLASS_IN_FACTORY(PostEffectFactory, PE_MONOCHROME, PostEffectMonochrome);

PostEffectManager::PostEffectManager()
{
	isPS20_ = gb_RenderDevice3D->IsPS20();
	isEnabled_ = false;
	width_ = 0;
	height_ = 0;

	enabledEffects_ = 0;

	for(int i = 0; i < PE_EFFECT_NUM; i++){
		effects_[i] = 0;
		enabledEffects_ |= 1 << i;
	}

	enabledEffects_ &= ~(1 << PE_BLOOM);

	for(int i = 0; i < PE_TEXTURE_NUM; i++)
		textures_[i] = 0;
}

PostEffectManager::~PostEffectManager()
{
	for(int i = 0; i < PE_TEXTURE_NUM; i++)
		RELEASE(textures_[i]);

	for(int i = 0; i < PE_EFFECT_NUM; i++)
		delete effects_[i];
}

void PostEffectManager::init(PostEffectType type)
{
	if(type == PE_EFFECT_NUM){
		for(int i = 0; i < PE_EFFECT_NUM; i++){
			if(isEnabled(PostEffectType(i)))
				createEffect(PostEffectType(i));
		}
	}
	else
		createEffect(type);

	for(int i = 0; i < PE_EFFECT_NUM; i++){
		if(effects_[i])
			effects_[i]->init();
	}

	updateSize();
}

void PostEffectManager::draw(float dt)
{
	updateSize();
	if(!isEnabled_)
		return;

	for(int i = 0; i < PE_EFFECT_NUM; i++){
		if(effects_[i] && effects_[i]->isEnabled())
			effects_[i]->redraw(dt);
	}
}

PostEffect* PostEffectManager::getEffect(PostEffectType type)
{
	if(type < PE_EFFECT_NUM)
		return effects_[type];

	return 0;
}

int PostEffectManager::texturesSize(PostEffectType type) const
{
	if(type < PE_EFFECT_NUM)
		return effects_[type]->texturesSize();

	return 0;
}

int PostEffectManager::baseTexturesSize() const
{
	int size = textureSize(PE_TEXTURE_BACKBUFFER);
	size += textureSize(PE_TEXTURE_FULL_SCREEN);

	return size;
}

cTexture* PostEffectManager::createTexture(PostEffectTextureID id, const char* texture_name, bool no_release)
{
	if(no_release && textures_[id])
		return textures_[id];

	RELEASE(textures_[id]);
	textures_[id] = GetTexLibrary()->GetElement2D(texture_name);

	return textures_[id];
}

cTexture* PostEffectManager::createTexture(PostEffectTextureID id, int width, int height, bool no_release)
{
	if(no_release && textures_[id])
		return textures_[id];

	RELEASE(textures_[id]);
	textures_[id] = GetTexLibrary()->CreateRenderTexture(width, height, TEXTURE_RENDER32);

	return textures_[id];
}

bool PostEffectManager::isActive(PostEffectType type) const
{
	if(type < PE_EFFECT_NUM && effects_[type])
		return effects_[type]->isActive();

	return false;
}

void PostEffectManager::setActive(PostEffectType type, bool active)
{
	if(type < PE_EFFECT_NUM)
		if(effects_[type])
			effects_[type]->setActive(active);
}

void PostEffectManager::createTextures()
{
	if(!createTexture(PE_TEXTURE_BACKBUFFER, width_, height_)){
		isEnabled_ = false;
		return;
	}

	for(int i = 0; i < PE_EFFECT_NUM; i++){
		if(effects_[i]){
			effects_[i]->setSize(width_, height_);
			effects_[i]->createTextures();
		}
	}
	isEnabled_ = true;
}

bool PostEffectManager::createEffect(PostEffectType type)
{
	if(isPS20_ || type == PE_MONOCHROME){
		PostEffectFactory::instance().setArgument(this);
		effects_[type] = PostEffectFactory::instance().create(type);
		return true;
	}

	return false;
}

cTexture* PostEffectManager::backBufferTexture()
{
	cTexture* backbuffer_texture = getTexture(PE_TEXTURE_BACKBUFFER);

	if(!backbuffer_texture)
		return 0;

	IDirect3DSurface9 *pDestSurface=0;
	RDCALL(backbuffer_texture->GetDDSurface(0)->GetSurfaceLevel(0,&pDestSurface));
	RDCALL(gb_RenderDevice3D->D3DDevice_->StretchRect(
		gb_RenderDevice3D->backBuffer_,0,pDestSurface,0,D3DTEXF_LINEAR));
	RELEASE(pDestSurface);

	return backbuffer_texture;
}

bool PostEffectManager::updateSize()
{
	xassert(gb_RenderDevice3D);

	D3DSURFACE_DESC desc;
	gb_RenderDevice3D->backBuffer_->GetDesc(&desc);
	if(desc.Width != width_ || desc.Height != height_){
		width_ = desc.Width;
		height_ = desc.Height;
		createTextures();
		return true;
	}

	return false;
}

PostEffectColorDodge::PostEffectColorDodge(PostEffectManager* manager) : PostEffect(manager)
{
	shader_ = 0;
	speed_ = 1;
	fadeSpeed_ = 1;
	phase_ = 0.0f;
	process_ = false;
	fadeIn_ = false;
	textureName_ = "Scripts\\Resource\\Textures\\twirl.tga";
}

PostEffectColorDodge::~PostEffectColorDodge()
{
	delete shader_;
}

void PostEffectColorDodge::setTexture(const char* name)
{
	textureName_ = name;
	createTextures();
}

int PostEffectColorDodge::texturesSize() const
{
	return manager_->textureSize(PE_COLOR_DODGE_TEXTURE2);
}

void PostEffectColorDodge::createTextures()
{
	isEnabled_ = true;
	if(!manager_->createTexture(PE_TEXTURE_FULL_SCREEN, width_, height_, true)) isEnabled_ = false;
	if(!manager_->createTexture(PE_COLOR_DODGE_TEXTURE2, textureName_.c_str())) isEnabled_ = false;

	center_.set(width_ * 0.5f, height_ * 0.5f);
	size_ = width_ * 1.414213562373f * 0.5f;
}
void PostEffectColorDodge::init()
{
	shader_ = new PSColorDodge();
	shader_->Restore();
}

void PostEffectColorDodge::redraw(float dtime)
{
	if(!isActive_ && !process_)
		return;

	cTexture* texture1 = manager_->getTexture(PE_TEXTURE_FULL_SCREEN);
	cTexture* texture2 = manager_->getTexture(PE_COLOR_DODGE_TEXTURE2);

	static float angle;
	angle+=dtime*speed_;
	float dt = dtime; //*1e-3f;
	if (process_)
	{
		if (fadeIn_)
		{
			phase_+=fadeSpeed_*dt;
			if (phase_>1)
			{
				phase_ = 1.0f;
				isActive_ = true;
				process_ = false;
			}
		}else
		{
			phase_-=fadeSpeed_*dt;
			if (phase_ < 0)
			{
				phase_ = 0.0f;
				isActive_ = false;
				process_ = false;
			}
		}
	}
	Vect2f c1(-size_,-size_);
	Vect2f c2(-size_,+size_);
	Vect2f c3(+size_,-size_);
	Vect2f c4(+size_,+size_);

	Mat2f mat(angle);
	c1 *= mat;
	c2 *= mat;
	c3 *= mat;
	c4 *= mat;

	setSamplerState();
//	SetRenderState();

	PE_RENDER_STATE_BACKUP;

	gb_RenderDevice3D->SetRenderTarget(texture1,0);
	gb_RenderDevice3D->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,texture2);
	cVertexBuffer<sVertexXYZWDT3>* Buf=gb_RenderDevice3D->GetBufferXYZWDT3();
	sVertexXYZWDT3* v=Buf->Lock(4);
	v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
	v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=Color4c(255,255,255);
	v[0].x=-0.5f+(float)c1.x+center_.x; v[0].y = -0.5f+(float)c1.y+center_.y;
	v[1].x=-0.5f+(float)c2.x+center_.x; v[1].y = -0.5f+(float)c2.y+center_.y;
	v[2].x=-0.5f+(float)c3.x+center_.x; v[2].y = -0.5f+(float)c3.y+center_.y;
	v[3].x=-0.5f+(float)c4.x+center_.x; v[3].y = -0.5f+(float)c4.y+center_.y;

	v[0].u1()=v[0].u2()=v[0].u3() = 0; v[0].v1()=v[0].v2()=v[0].v3()=0;
	v[1].u1()=v[1].u2()=v[1].u3() = 0; v[1].v1()=v[1].v2()=v[1].v3()=1;
	v[2].u1()=v[2].u2()=v[2].u3() = 1; v[2].v1()=v[2].v2()=v[2].v3()=0;
	v[3].u1()=v[3].u2()=v[3].u3() = 1; v[3].v1()=v[3].v2()=v[3].v3()=1;
	Buf->Unlock(4);

	Buf->DrawPrimitive(PT_TRIANGLESTRIP,2);

	cTexture* backBuffer = manager_->backBufferTexture();
	gb_RenderDevice3D->RestoreRenderTarget();
	gb_RenderDevice3D->SetTexture(0,backBuffer);
	gb_RenderDevice3D->SetTexture(1,texture1);
	shader_->Select(phase_);
	gb_RenderDevice3D->DrawQuad(0.0f, 0.0f, float(width_), float(height_), 0.0f, 0.0f, 1.0f, 1.0f);

//	RestoreRenderState();
	PE_RENDER_STATE_RESTORE;
}
void PostEffectColorDodge::setSamplerState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_clamp_linear);
}

PostEffectMonochrome::PostEffectMonochrome(PostEffectManager* manager) : PostEffect(manager)
{
	shader_ = 0;
	phase_ = 0.0f;
	process_ = false;
	speed_ = 1.0f;
	fadeIn_ = false;
	timePrev_ = 0;
}

PostEffectMonochrome::~PostEffectMonochrome()
{
	delete shader_;
}

void PostEffectMonochrome::init()
{
	isEnabled_ = true;
	shader_ = new PSMonochrome();
	shader_->Restore();
	timePrev_ = xclock();
}

void PostEffectMonochrome::redraw(float timeUnused)
{
	if ((!isActive_&&!process_))
		return;
	int time = xclock();
	float dt = float(time - timePrev_)*1e-3f;
	if (process_)
	{
		if (fadeIn_)
		{
			phase_+=speed_*dt;
			if (phase_>1)
			{
				phase_ = 1.0f;
				isActive_ = true;
				process_ = false;
			}
		}else
		{
			phase_-=speed_*dt;
			if (phase_ < 0)
			{
				phase_ = 0.0f;
				isActive_ = false;
				process_ = false;
			}
		}
	}
	setSamplerState();
	PE_RENDER_STATE_BACKUP;

	cTexture* backBuffer = manager_->backBufferTexture();
	gb_RenderDevice3D->SetTexture(0,backBuffer);
	shader_->Select(phase_);
	gb_RenderDevice3D->DrawQuad(0, 0, width_, height_, 0, 0, 1, 1);

	PE_RENDER_STATE_RESTORE;
}

void PostEffectMonochrome::setSamplerState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_point);
}

PostEffectBloom::PostEffectBloom(PostEffectManager* manager) : PostEffect(manager)
{
	psColorBright = 0;
	psCombine = 0;
	psBloomH = 0;
	psBloomV = 0;
	luminance_ = 0.08f;
	bloomScale_ = 1.5f;
	addColor = Color4f(0,0,0,0);
	exlpodeEnable_ = false;
	defaultLuminance_ = 0.08f;
}

PostEffectBloom::~PostEffectBloom()
{
	delete psColorBright;
	delete psCombine;
	delete psBloomH;
	delete psBloomV;
	delete psMonochrome_;
	delete psDown4;
}

void PostEffectBloom::createTextures()
{
	isEnabled_ = true;
	if(!manager_->createTexture(PE_BLOOM_TEXTURE2, width_/16, height_/16))
		isEnabled_ = false;
	if(!manager_->createTexture(PE_BLOOM_TEXTURE3, width_/16, height_/16))
		isEnabled_ = false;
	if(!manager_->createTexture(PE_BLOOM_TEXTURE_TEMP, width_/4, height_/4))
		isEnabled_ = false;
}

void PostEffectBloom::init()
{
	psColorBright = new PSColorBright();
	psColorBright->Restore();
	psCombine = new PSCombine();
	psCombine->Restore();
	psBloomH = new PSBloomHorizontal();
	psBloomH->Restore();
	psBloomV = new PSBloomVertical();
	psBloomV->Restore();
	psMonochrome_ = new PSMonochrome();
	psMonochrome_->Restore();
	psDown4 = new PSDown4();
	psDown4->Restore();
}

int PostEffectBloom::texturesSize() const
{
	int size = manager_->textureSize(PE_BLOOM_TEXTURE2);

	size += manager_->textureSize(PE_BLOOM_TEXTURE3);
	size += manager_->textureSize(PE_BLOOM_TEXTURE_TEMP);

	return size;
}

void PostEffectBloom::RestoreDefaults()
{
	luminance_ = defaultLuminance_;
	addColor = Color4f(0,0,0,0);
}

void PostEffectBloom::SetExplode(float enable)
{
	exlpodeEnable_ = enable;
}

void PostEffectBloom::setSamplerState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_clamp_linear);
}

void PostEffectBloom::redraw(float dt)
{
	if(!isActive_ && !exlpodeEnable_)
		return;

	setSamplerState();
	PE_RENDER_STATE_BACKUP;

	cTexture* backBuffer = manager_->backBufferTexture();

	cTexture* temp_texture = manager_->getTexture(PE_BLOOM_TEXTURE_TEMP);
	cTexture* texture2 = manager_->getTexture(PE_BLOOM_TEXTURE2);
	cTexture* texture3 = manager_->getTexture(PE_BLOOM_TEXTURE3);

	gb_RenderDevice3D->SetRenderTarget(temp_texture,0);
	gb_RenderDevice3D->SetTexture(0,backBuffer);
	psDown4->Select(float(width_), float(height_));
	gb_RenderDevice3D->DrawQuad(0,0,float(temp_texture->GetWidth()),float(temp_texture->GetHeight()),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture3,0);
	gb_RenderDevice3D->SetTexture(0,temp_texture);
	psDown4->Select(temp_texture->GetWidth(),temp_texture->GetWidth());
	gb_RenderDevice3D->DrawQuad(0,0,float(texture3->GetWidth()),float(texture3->GetHeight()),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture2,0);
	gb_RenderDevice3D->SetTexture(0,texture3);
	psColorBright->Select(luminance_);
	gb_RenderDevice3D->DrawQuad(0,0,float(texture2->GetWidth()),float(texture2->GetHeight()),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture3,0);
	gb_RenderDevice3D->SetTexture(0,texture2);
	psBloomV->Select(texture3->GetWidth(),texture3->GetHeight(),bloomScale_);
	gb_RenderDevice3D->DrawQuad(0,0,float(texture3->GetWidth()),float(texture3->GetHeight()),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture2,0);
	gb_RenderDevice3D->SetTexture(0,texture3);
	psBloomH->Select(texture2->GetWidth(),texture2->GetHeight(),bloomScale_);
	gb_RenderDevice3D->DrawQuad(0,0,float(texture2->GetWidth()),float(texture2->GetHeight()),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture3,0);
	gb_RenderDevice3D->SetTexture(0,texture2);
	psBloomV->Select(texture3->GetWidth(),texture3->GetHeight(),bloomScale_);
	gb_RenderDevice3D->DrawQuad(0,0,float(texture3->GetWidth()),float(texture3->GetHeight()),0,0,1,1);

	gb_RenderDevice3D->SetRenderTarget(texture2,0);
	gb_RenderDevice3D->SetTexture(0,texture3);
	psBloomH->Select(texture2->GetWidth(),texture2->GetHeight(),bloomScale_);
	gb_RenderDevice3D->DrawQuad(0,0,float(texture2->GetWidth()),float(texture2->GetHeight()),0,0,1,1);

	gb_RenderDevice3D->RestoreRenderTarget();

	gb_RenderDevice3D->SetTexture(0,backBuffer);
	gb_RenderDevice3D->SetTexture(1,texture2);
	psCombine->Select(addColor);
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);

	PE_RENDER_STATE_RESTORE;
}

PostEffectUnderWater::PostEffectUnderWater(PostEffectManager* manager) : PostEffect(manager)
{
	psUnderWater_ = 0;
	shift_ = 0.0f;
	scale_ = 0.0f;
	delta_ = 10;
	underWater_ = false;
	waveSpeed_ = 0.01f;
	color_ = Color4f(0.0f,0.0f,0.2f);
	fogPlanes_ = Vect2f(100,1000);
	activeAlways_ = false;
	activateUnderWater_ = false;
	isActive_ = true;
}

PostEffectUnderWater::~PostEffectUnderWater()
{
	delete psUnderWater_;
}

void PostEffectUnderWater::setSamplerState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_wrap_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_wrap_linear);
}

void PostEffectUnderWater::createTextures()
{
	isEnabled_ = true;
}

void PostEffectUnderWater::init()
{
	psUnderWater_ = new PSUnderWater();
	psUnderWater_->Restore();
}

int PostEffectUnderWater::texturesSize() const
{
	return manager_->textureSize(PE_UNDER_WATER_TEXTURE_WAVE);
}

void PostEffectUnderWater::setUnderWater(bool under_water)
{
	activateUnderWater_ = under_water;
}

void PostEffectUnderWater::redraw(float dtime)
{
	cTexture* wave_texture = manager_->getTexture(PE_UNDER_WATER_TEXTURE_WAVE);

	if(!isActive_ || !wave_texture)
		return;
	float dt = dtime; //*1e-3f;
	shift_ += waveSpeed_*dt;
	if(!activeAlways_){
		if(!activateUnderWater_){
			delta_ = -10;
		}
		else {
			delta_ = 10;
			underWater_ = true;
		}

		scale_ += delta_*dt;
		if(scale_>1.0f)
			scale_ = 1.0f;
		if(scale_ < 0.0f){
			scale_ = 0.0;
			underWater_ = false;
		}
		if(!underWater_)
			return;
	}
	else
		scale_ = 1.0f;

	PE_RENDER_STATE_BACKUP;

	cTexture* backBuffer = manager_->backBufferTexture();

	gb_RenderDevice3D->SetTexture(0,backBuffer);
	gb_RenderDevice3D->SetTexture(1,wave_texture);
	setSamplerState();
	psUnderWater_->Select(shift_,scale_,color_);
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);

	PE_RENDER_STATE_RESTORE;
}
void PostEffectUnderWater::setFog(Color4f& clr)
{
	Vect2f fog;
	if(scale_ < 1){
		fog.x = LinearInterpolate(environmentFog_.x,fogPlanes_.x,scale_);
		fog.y = LinearInterpolate(environmentFog_.y,fogPlanes_.y,scale_);
		gb_RenderDevice3D->SetGlobalFog(clr,fog);
	}
	else {
		gb_RenderDevice3D->SetGlobalFog(clr,fogPlanes_);
	}
}

void PostEffectUnderWater::setTexture(const char* name)
{
	manager_->createTexture(PE_UNDER_WATER_TEXTURE_WAVE, name);
}

PostEffectDOF::PostEffectDOF(PostEffectManager* manager) : PostEffect(manager)
{
	psDOFCombine_ = 0;
	psBlur_ = 0;
	dofParams_.set(200.f,1.f/200);
	dofPower_ = 4.f;
}

PostEffectDOF::~PostEffectDOF()
{
	delete psDOFCombine_;
	delete psBlur_;
}

void PostEffectDOF::createTextures()
{
	if(manager_->createTexture(PE_TEXTURE_FULL_SCREEN, width_, height_, true))
		isEnabled_ = true;
	else
		isEnabled_ = false;
}

void PostEffectDOF::init()
{
	psDOFCombine_ = new PSDOFCombine;
	psDOFCombine_->Restore();
	psBlur_ = new PSBlurMap;
	psBlur_->Restore();
}

int PostEffectDOF::texturesSize() const
{
	return 0;
}

void PostEffectDOF::setDofPower(float power)
{
	dofPower_ = power;
}

void PostEffectDOF::setDofParams(Vect2f &params)
{
	dofParams_.x = params.x;
	if((params.y-params.x)>FLT_EPS)
		//dofParams_.y = 2.f/params.y; //формула для размывания вблизи (правильная)
		dofParams_.y = 1.f/(params.y-params.x); 
	else
		dofParams_.y=1;
}

void PostEffectDOF::redraw(float dt)
{
	if(!Option_UseDOF)
		return;
	setSamplerState();
	PE_RENDER_STATE_BACKUP;

	IDirect3DSurface9 *pDestSurface=0;
	cTexture* backBuffer = manager_->backBufferTexture();
	cTexture* tmp = manager_->getTexture(PE_TEXTURE_FULL_SCREEN);

	gb_RenderDevice3D->SetRenderTarget(tmp,0);
	gb_RenderDevice3D->SetTexture(0,backBuffer);
	gb_RenderDevice3D->SetTexture(1,gb_RenderDevice3D->GetFloatMap());
	psBlur_->Select(dofParams_);
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);

	gb_RenderDevice3D->RestoreRenderTarget();

	gb_RenderDevice3D->SetTexture(0,tmp);
	gb_RenderDevice3D->SetTexture(1,gb_RenderDevice3D->GetFloatMap());
	psDOFCombine_->SetDofParams(dofParams_,dofPower_);
	psDOFCombine_->Select();
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);

	PE_RENDER_STATE_RESTORE;
}

PostEffectMirage::PostEffectMirage(PostEffectManager* manager) : PostEffect(manager)
{
	psMirage_ = 0;
	isActive_=true;
}
PostEffectMirage::~PostEffectMirage()
{
	delete psMirage_; 
}
void PostEffectMirage::init()
{
	psMirage_ = new PSMirage();
	psMirage_->Restore();
}
void PostEffectMirage::redraw(float dt)
{
	cTexture* wave_texture = manager_->getTexture(PE_TEXTURE_FULL_SCREEN);

	if(!isActive_)
		return;
	if (!enableMirage || !wave_texture)
		return;

	PE_RENDER_STATE_BACKUP;

	cTexture* backBuffer = manager_->backBufferTexture();
	
	IDirect3DSurface9 *pDestSurface=0;
	RDCALL(wave_texture->GetDDSurface(0)->GetSurfaceLevel(0,&pDestSurface));
	RDCALL(gb_RenderDevice3D->D3DDevice_->StretchRect(
		gb_RenderDevice3D->GetMirageMap(),0,pDestSurface,0,D3DTEXF_LINEAR));
	pDestSurface->Release();

	gb_RenderDevice3D->SetTexture(0,backBuffer);
	gb_RenderDevice3D->SetTexture(1,wave_texture);
	setSamplerState();
	psMirage_->Select();
	gb_RenderDevice3D->DrawQuad(0,0,width_,height_,0,0,1,1);

	PE_RENDER_STATE_RESTORE;
	enableMirage = false;
}
void PostEffectMirage::createTextures()
{
	if(!manager_->createTexture(PE_TEXTURE_FULL_SCREEN, width_, height_, true))
		isEnabled_ = true;
	else
		isEnabled_ = false;
}
int PostEffectMirage::texturesSize() const
{
	return 0;
}

void PostEffectMirage::setSamplerState()
{
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_clamp_linear);
}

