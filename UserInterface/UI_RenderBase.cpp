#include "StdAfx.h"
#include "UI_RenderBase.h"
#include "Render\src\TexLibrary.h"
#include "Render\D3d\D3DRender.h"
#include "UI_Sprite.h"

UI_RenderBase* UI_RenderBase::self_ = 0;

UI_RenderBase::UI_RenderBase()
: windowPosition_(0, 0, 1024, 768)
, defaultResolution_(1024, 768)
, renderSize_(1, 1)
{
	self_ = this;
}

void UI_RenderBase::create()
{
	xassert(!self_);
	static UI_RenderBase obj;
	self_ = &obj;
}

cTexture* UI_RenderBase::createTexture(const char* file_name) const
{
	if(gb_RenderDevice3D)
		return GetTexLibrary()->GetElement2D(file_name);
	return 0;
}

void UI_RenderBase::releaseTexture(cTexture* &texture) const
{
	if(texture && texture->Release() == 1)
		GetTexLibrary()->Unload(texture->GetUniqueName().c_str());
	texture = 0;
}


void UI_RenderBase::drawSprite(int left, int top, int width, int height, float u, float v, float du, float dv, cTexture* texture, Color4c color, float saturation, eBlendMode blendMode, float phase) const
{
	if(width >= 0){
		if(left + width < 0 || left > renderSize_.x)
			return;
	}
	else if(left < 0 || left + width > renderSize_.x)
		return;

	if(height >= 0){
		if(top + height < 0 || top > renderSize_.y)
			return;
	}
	else if(top < 0 || top + height > renderSize_.y)
		return;

	if(blendMode <= ALPHA_TEST && (color.a < 255 || (texture && texture->isAlpha())))
		blendMode = ALPHA_BLEND;

	gb_RenderDevice3D->SetNoMaterial(blendMode, MatXf::ID, phase, texture);

	xassert(saturation <= 1.f);
	if(1.f - saturation > 0.01f)
		gb_RenderDevice3D->psMonochrome->Select(1.f - saturation);

	gb_RenderDevice3D->DrawQuad(left, top, width, height, u, v, du, dv, color);
}

void UI_RenderBase::drawSpriteTiled(int left, int top, int width, int height, float u, float v, float du, float dv, cTexture* texture, Color4c color, float saturation, eBlendMode blendMode, float phase) const
{
	int right = left + width;
	int bottom = top + height;

	if(width >= 0){
		if(right < 0 || left > renderSize_.x)
			return;
	}
	else if(left < 0 || right > renderSize_.x)
		return;

	if(height >= 0){
		if(bottom < 0 || top > renderSize_.y)
			return;
	}
	else if(top < 0 || bottom > renderSize_.y)
		return;

	if(blendMode <= ALPHA_TEST && (color.a < 255 || (texture && texture->isAlpha())))
		blendMode = ALPHA_BLEND;

	gb_RenderDevice3D->SetNoMaterial(blendMode, MatXf::ID, phase, texture);

	xassert(saturation <= 1.f);
	if(1.f - saturation > 0.01f)
		gb_RenderDevice3D->psMonochrome->Select(1.f - saturation);

	int tx_w = round(du * (float)texture->GetWidth());
	int tx_h = round(dv * (float)texture->GetHeight());

	int x = left;
	int y = top;

	for(;;){
		int curR = x + tx_w;
		int curB = y + tx_h;

		if(curR > right)
			curR = right;

		if(curB > bottom)
			curB = bottom;

		int w = curR - x;
		int h = curB - y;
		float curDU = (float)(w) / (float)texture->GetWidth();
		float curDV = (float)(h) / (float)texture->GetHeight();

		gb_RenderDevice3D->DrawQuad(x, y, w, h, u, v, curDU, curDV, color);

		if(curR >= right){
			x = left;
			y = curB;

			if(y >= bottom)
				break;
		}
		else
			x = curR;
	}
}

void UI_RenderBase::drawSpriteSolid(int left, int top, int width, int height, float u, float v, float du, float dv, cTexture* texture, Color4c color, eBlendMode blendMode, float phase) const
{
	if(width >= 0){
		if(left + width < 0 || left > renderSize_.x)
			return;
	}
	else if(left < 0 || left + width > renderSize_.x)
		return;

	if(height >= 0){
		if(top + height < 0 || top > renderSize_.y)
			return;
	}
	else if(top < 0 || top + height > renderSize_.y)
		return;

	if(blendMode <= ALPHA_TEST && (color.a < 255 || (texture && texture->isAlpha())))
		blendMode = ALPHA_BLEND;

	gb_RenderDevice3D->SetNoMaterial(blendMode, MatXf::ID, phase, texture);
	gb_RenderDevice3D->psSolidColor->Select();

	gb_RenderDevice3D->DrawQuad(left, top, width, height, u, v, du, dv, color);
}


void UI_RenderBase::drawLine(const Vect2f& p0, const Vect2f& p1, const Color4f& color) const
{
	Vect2i pos0 = screenCoords(p0);
	Vect2i pos1 = screenCoords(p1);

	gb_RenderDevice3D->DrawLine(pos0.x, pos0.y, pos1.x, pos1.y, color);
}

void UI_RenderBase::drawRectangle(const Rectf& rect, const Color4f& color, bool outlined/*, UI_BlendMode blend_mode*/) const
{
	Recti r = screenCoords(rect);

	gb_RenderDevice3D->DrawRectangle(r.left(), r.top(), r.width(), r.height(), Color4c(color), outlined);
	gb_RenderDevice3D->FlushPrimitive2D();
}

Vect2f UI_RenderBase::deviceCoords(const Vect2i& screen_coords) const
{
	if(!gb_RenderDevice3D)
		return Vect2f(0,0);

	return Vect2f(
		float(screen_coords.x) / float(renderSize_.x),
		float(screen_coords.y) / float(renderSize_.y)) - Vect2f(0.5f, 0.5f);
}

Rectf UI_RenderBase::deviceCoords(const Recti& screen_coords) const
{
	if(!gb_RenderDevice3D)
		return Rectf(0,0,1,1);

	return Rectf(float(screen_coords.left()) / float(renderSize_.x),
		float(screen_coords.top()) / float(renderSize_.y),
		float(screen_coords.width()) / float(renderSize_.x),
		float(screen_coords.height()) / float(renderSize_.y)) - Vect2f(0.5f, 0.5f);
}

Recti UI_RenderBase::device2screenCoords(const Rectf& device_coords) const
{
	if(!gb_RenderDevice3D)
		return Recti(0,0,1,1);

	return Recti(
		(device_coords.left_top() + Vect2f(0.5f, 0.5f)) * Vect2f(renderSize_.x, renderSize_.y),
		device_coords.size() * Vect2f(renderSize_.x, renderSize_.y));
}

Vect2f UI_RenderBase::device2relativeCoords(const Vect2f& device_coords) const
{
	if(!gb_RenderDevice3D)
		return Vect2f::ZERO;

	return relativeCoords((device_coords + Vect2f(0.5f, 0.5f)) * Vect2f(renderSize_.x, renderSize_.y));
}