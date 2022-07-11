#include "StdAfx.h"

#include <vector>
#include "IVisGeneric.h"
#include "IVisD3D.h"
#include "IRenderDevice.h"

#include "..\Render\src\FontInternal.h"
#include "..\Render\d3d\D3DRender.h"

#include "..\Game\RenderObjects.h"

#include "UI_TextParser.h"
#include "UI_Types.h"
#include "UI_Render.h"
#include "UI_BackgroundScene.h"
#include "..\Environment\Environment.h"
#include "..\Water\Water.h"
#include "CameraManager.h"

static eBlendMode convertBlendMode(UI_BlendMode mode)
{
	switch (mode) {
		case UI_BLEND_ADD:
			return ALPHA_ADDBLENDALPHA;
		case UI_BLEND_NORMAL:
			return ALPHA_BLEND;
		case UI_BLEND_APPEND:
			return ALPHA_ADDBLEND;
		case UI_BLEND_FORCE:
			return ALPHA_BLEND_NOTUSETEXTUREALPHA;
		default:
			return ALPHA_NONE;
	}
}

static void drawSprite(int left, int top, int width, int height, float u, float v, float du, float dv, cTexture* texture, sColor4c color, float saturation, eBlendMode blendMode, float phase = 0.0f)
{
	int right = left + width;
	int bottom = top + height;

	int screenWidth = UI_Render::instance().renderSize().x;
	int screenHeight = UI_Render::instance().renderSize().y;

	if(width >= 0){
		if(right < 0 || left > screenWidth)
			return;
	}
	else if(left < 0 || right > screenWidth)
		return;
	
	if(height >= 0){
		if(bottom < 0 || top > screenHeight)
			return;
	}
	else if(top < 0 || bottom > screenHeight)
		return;

	bool alpha = color.a < 255 || (texture && texture->IsAlpha());

	if(blendMode <= ALPHA_TEST && alpha)
		blendMode = ALPHA_BLEND;
	gb_RenderDevice3D->SetNoMaterial(blendMode, MatXf::ID, phase, texture);
	if(fabsf(saturation - 1.f) > FLT_COMPARE_TOLERANCE)
		gb_RenderDevice3D->psMonochrome->Select(1.f - saturation);

	gb_RenderDevice3D->DrawQuad(left, top, width, height, u, v, du, dv, color);
}

static void drawSpriteSolid(int left, int top, int width, int height, float u, float v, float du, float dv, cTexture* texture, sColor4c color, eBlendMode blendMode, float phase = 0.0f)
{
	int right = left + width;
	int bottom = top + height;

	int screenWidth = UI_Render::instance().renderSize().x;
	int screenHeight = UI_Render::instance().renderSize().y;

	if(width >= 0){
		if(right < 0 || left > screenWidth)
			return;
	}
	else if(left < 0 || right > screenWidth)
		return;
	
	if(height >= 0){
		if(bottom < 0 || top > screenHeight)
			return;
	}
	else if(top < 0 || bottom > screenHeight)
		return;

	bool alpha = color.a < 255 || (texture && texture->IsAlpha());

	if(blendMode <= ALPHA_TEST && alpha)
		blendMode = ALPHA_BLEND;
	gb_RenderDevice3D->SetNoMaterial(blendMode, MatXf::ID, phase, texture);
	gb_RenderDevice3D->psSolidColor->Select();

	gb_RenderDevice3D->DrawQuad(left, top, width, height, u, v, du, dv, color);
}

UI_Render::UI_Render()
: visGeneric_(0)
, renderDevice_(0)
, windowPosition_(0, 0, 1024, 768)
, defaultResolution_(1024, 768)
, renderSize_(0, 0)
, defaultFont_(0)
, screenTexture_(0)
, camera_(0)
{
}

UI_Render::~UI_Render()
{
	setInterfaces(0, 0);
}

cTexture* UI_Render::createTexture(const char* file_name) const
{
	if(renderDevice_)
		return GetTexLibrary()->GetElement2D(file_name);

	return 0;
}

bool UI_Render::releaseTexture(cTexture* &texture) const
{
	bool ret = true;
	if(texture)
		if(texture->Release()) // еще не удалена
			ret = !GetTexLibrary()->Unload(texture->GetUniqName().c_str());
	texture = 0;
	return ret;
}

cFont* UI_Render::createFont(const char* font_name, int font_size, const char* path) const
{
	int real_size = max(1, round(float(windowPosition_.width()) / float(defaultResolution_.x) * float(font_size)));

	std::string full_path = (path ? path : fontDirectory_);

	if(*full_path.rbegin() != '\\')
		full_path += "\\";

	full_path += font_name;
	full_path += ".font";

	return visGeneric_->CreateFont(full_path.c_str(), -real_size);
}

bool UI_Render::releaseFont(cFont* font) const
{
	font->Release();
	return true;
}

void UI_Render::drawSprite(const Rectf& pos, const UI_Sprite& sprite, sColor4f color, UI_BlendMode blend_mode, float phase) const
{
	if(!sprite.isEmpty()){
		Recti scr_pos = screenCoords(pos);
		Rectf txt_pos = sprite.textureCoords();
		
		sColor4c diffuseColor(round(color.r * sprite.diffuseColor().r),
                              round(color.g * sprite.diffuseColor().g),
                              round(color.b * sprite.diffuseColor().b),
                              round(color.a * sprite.diffuseColor().a));

		gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_linear);
		::drawSprite(scr_pos.left(), scr_pos.top(), scr_pos.width(), scr_pos.height(), 
					 txt_pos.left(), txt_pos.top(), txt_pos.width(), txt_pos.height(),
					 sprite.texture(), diffuseColor, sprite.saturation(), convertBlendMode(blend_mode), phase);

	}
}

void UI_Render::drawSprite(const Rectf& pos, cTexture* texture, sColor4f color, UI_BlendMode blend_mode, float phase) const
{
	Recti scr_pos = screenCoords(pos);

	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_linear);
	::drawSprite(scr_pos.left(), scr_pos.top(), scr_pos.width(), scr_pos.height(), 0.0f, 0.0f, 1.0f, 1.0f,
				 texture, color, 1.f, convertBlendMode(blend_mode), phase);

}


void UI_Render::drawSprite(const Rectf& pos, cTexture* texture, const Rectf& txt_pos, sColor4f color, UI_BlendMode blend_mode) const
{
	Recti scr_pos = screenCoords(pos);

	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_linear);
	::drawSprite(scr_pos.left(), scr_pos.top(), scr_pos.width(), scr_pos.height(),
				 txt_pos.left(), txt_pos.top(), txt_pos.width(), txt_pos.height(),
				 texture, color, 1.f, convertBlendMode(blend_mode));

}

void UI_Render::drawMiniMap(const Vect2f& v1,const Vect2f& v2,const Vect2f& v3,const Vect2f& v4,
							cTexture* baseTexture,
							bool drawFog, bool drawWater,
							const sColor4f& borderColor, float Alpha,const sColor4f& terraColor)
{
	sColor4f water(0.f, 0.f, 1.f, 1.f);
	sColor4f fog(0.f, 0.f, 0.f, Alpha);

	//Recti scr_pos = screenCoords(pos);
	/*
	v1------v3
	|		|
	|		|
	v2------v4
	*/
	Vect2i vi1 = screenCoords(v1);
	Vect2i vi2 = screenCoords(v2);
	Vect2i vi3 = screenCoords(v3);
	Vect2i vi4 = screenCoords(v4);
	gb_RenderDevice3D->SetBlendStateAlphaRef(ALPHA_BLEND);
	gb_RenderDevice3D->SetSamplerData(0,sampler_clamp_linear);
	gb_RenderDevice3D->SetSamplerData(1,sampler_clamp_linear);
	gb_RenderDevice3D->SetSamplerData(3,sampler_clamp_linear);
	if(!baseTexture)
		gb_RenderDevice3D->psMiniMap->SetUseTerraColor(true);
	else
		gb_RenderDevice3D->SetTexture(0,baseTexture);

	if (environment)
	{
		if (drawWater && environment->water())
		{
			gb_RenderDevice3D->SetTexture(1,environment->water()->GetTextureMiniMap());
			gb_RenderDevice3D->psMiniMap->SetUseWater(true);
		}else
		{
			gb_RenderDevice3D->psMiniMap->SetUseWater(false);
		}
		if (drawFog && environment->fogOfWar())
		{
			gb_RenderDevice3D->SetTexture(2,environment->fogOfWar()->GetTexture());
			gb_RenderDevice3D->psMiniMap->SetUseFogOfWar(true);
			gb_RenderDevice3D->psMiniMap->SetFogAlpha(environment->fogOfWar()->GetInvFogAlpha());
		}
		else
		{
			gb_RenderDevice3D->psMiniMap->SetUseFogOfWar(false);
		}

	}else
	{
		gb_RenderDevice3D->psMiniMap->SetUseWater(false);
		gb_RenderDevice3D->psMiniMap->SetUseFogOfWar(false);
	}
	gb_RenderDevice3D->psMiniMap->Select(water,terraColor);

	cVertexBuffer<sVertexXYZWDT3>* Buf=gb_RenderDevice3D->GetBufferXYZWDT3();
	sVertexXYZWDT3* v=Buf->Lock(4);
	v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
	v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=fog;
	v[0].x=-0.5f+(float)vi1.x; v[0].y = -0.5f+(float)vi1.y;
	v[1].x=-0.5f+(float)vi2.x; v[1].y = -0.5f+(float)vi2.y;
	v[2].x=-0.5f+(float)vi3.x; v[2].y = -0.5f+(float)vi3.y;
	v[3].x=-0.5f+(float)vi4.x; v[3].y = -0.5f+(float)vi4.y;

	v[0].u1()=v[0].u2()=v[0].u3() = 0; v[0].v1()=v[0].v2()=v[0].v3()=0;
	v[1].u1()=v[1].u2()=v[1].u3() = 0; v[1].v1()=v[1].v2()=v[1].v3()=1;
	v[2].u1()=v[2].u2()=v[2].u3() = 1; v[2].v1()=v[2].v2()=v[2].v3()=0;
	v[3].u1()=v[3].u2()=v[3].u3() = 1; v[3].v1()=v[3].v2()=v[3].v3()=1;
	Buf->Unlock(4);

	Buf->DrawPrimitive(PT_TRIANGLESTRIP,2);

	sColor4f brd(borderColor.r, borderColor.g, borderColor.b, borderColor.a * Alpha);
	if(brd.a > FLT_EPS){
		renderDevice_->DrawLine(vi1.x, vi1.y, vi2.x, vi2.y, brd);
		renderDevice_->DrawLine(vi1.x, vi1.y, vi3.x, vi3.y, brd);
		renderDevice_->DrawLine(vi4.x, vi4.y, vi2.x, vi2.y, brd);
		renderDevice_->DrawLine(vi4.x, vi4.y, vi3.x, vi3.y, brd);
	}
}


void UI_Render::drawLine(const Vect2f& p0, const Vect2f& p1, const sColor4f& color) const
{
	Vect2i pos0 = screenCoords(p0);
	Vect2i pos1 = screenCoords(p1);

	renderDevice_->DrawLine(pos0.x, pos0.y, pos1.x, pos1.y, color);

}

void UI_Render::drawRectangle(const Rectf& rect, const sColor4f& color, bool outlined/*, UI_BlendMode blend_mode*/) const
{
	Recti r = screenCoords(rect);

	renderDevice_->DrawRectangle(r.left(), r.top(), r.width(), r.height(), sColor4c(color), outlined);
	renderDevice_->FlushPrimitive2D ();
}

namespace {
	float textStart(int align, float width)
	{
		switch(align & UI_TEXT_ALIGN){
		case UI_TEXT_ALIGN_CENTER:
			return width / 2.f;
		case UI_TEXT_ALIGN_RIGHT:
			return width;
		}
		return 0;
	}
};

void UI_Render::outDebugText(const Vect2f& pos, const char* text, const sColor4c* color) const
{
	Vect2f text_size = UI_Render::instance().textSize(text, 0);
	Vect2f text_pos(clamp(pos.x + text_size.x, text_size.x, 1.f), clamp(pos.y + text_size.y, text_size.y, 1.f));
	text_pos -= text_size;
	
	sColor4c clr(WHITE);
	if(!color)
		color = &clr;

	unsigned char bright = max(color->r, color->g, color->b) > 128 ? 0 : 255;
	sColor4c shadow(bright, bright, bright);

	outText(Rectf(text_pos, Vect2f::ZERO), text, &UI_TextFormat(*color, shadow));
}

Vect2f UI_Render::outText(const Rectf& pos, const char* text, const UI_TextFormat* format, int textAlign, const UI_Font* font, float alpha, bool formatText) const
{
	xassert(text);

	if(!*text)
		return Vect2f(pos.left() + textStart(textAlign, pos.width()), pos.top());

	if(!font)
		font = defaultFont();

	UI_TextParser parser(font->font());
	parser.parseString(text,
		format ? format->textColor() : BLACK,
		formatText ? screenSize(pos.size()).x : -1);
	
	return outText(pos, parser, parser.outNodes().begin(), parser.outNodes().end(), format, textAlign, font, alpha);
}

Vect2f UI_Render::outText(const Rectf& pos, const UI_TextParser& parser, OutNodes::const_iterator begin, OutNodes::const_iterator end, const UI_TextFormat* format, int textAlign, const UI_Font* font, float alpha, bool clampInRange) const
{
	if(!font)
		font = defaultFont();

	if(!format)
		format = &UI_TextFormat::WHITE;

	if(!setFont(font))
		return Vect2f(pos.left() + textStart(textAlign, pos.width()), pos.top());

	Recti scr_pos = screenCoords(pos);

	if(clampInRange && scr_pos.width() < 3)
		return Vect2f(pos.left() + textStart(textAlign, pos.width()), pos.top());

	int x = scr_pos.left();
	int y = scr_pos.top();

	int hSize = parser.fontHeight();
	y -= hSize;

	if((textAlign & UI_TEXT_VALIGN) != UI_TEXT_VALIGN_TOP){
		switch(textAlign & UI_TEXT_VALIGN){
		case UI_TEXT_VALIGN_CENTER:
			y += (scr_pos.height() - parser.size().y) / 2;
			break;
		case UI_TEXT_VALIGN_BOTTOM:
			y += scr_pos.height() - parser.size().y;
			break;
		}
	}

	eBlendMode alphaBlend = convertBlendMode(format->blendMode());
	int rightRange = scr_pos.right();
	
	if(rightRange - x < 2)
		rightRange += 10;

	sColor4c diffuse = format->textColor();
	diffuse.a *= alpha;
	sColor4c shadow = format->shadowColor();
	shadow.a *= 1.f / 255.f * diffuse.a;

	bool skipToNextLine = false;
	int cur_x = x;
	for(OutNodes::const_iterator it = begin; it != end; ++it){
		switch(it->type){
			case OutNode::NEW_LINE:
				skipToNextLine = false;
				y += hSize;
				switch(textAlign & UI_TEXT_ALIGN){
					case UI_TEXT_ALIGN_LEFT:
						cur_x = x;
						break;
					case UI_TEXT_ALIGN_CENTER:
						cur_x = x + (scr_pos.width() - it->width - 1) / 2;
						break;
					case UI_TEXT_ALIGN_RIGHT:
						cur_x = x + scr_pos.width() - it->width - 1;
						break;
					default:
						xxassert(false, ":\\");
				}
				break;

			case OutNode::TEXT:
				if(skipToNextLine)
					break;
				if(clampInRange){
					if(shadow.a)
						renderDevice_->OutTextLine(cur_x+1, y+1, it->begin, it->end, shadow, ALPHA_BLEND, x, rightRange + 1);
					int posx = renderDevice_->OutTextLine(cur_x, y, it->begin, it->end, diffuse, alphaBlend, x, rightRange);
					cur_x += it->width;
					if(cur_x >= rightRange){
						cur_x = posx;
						skipToNextLine = true;
					}
				}
				else {
					if(shadow.a)
						renderDevice_->OutTextLine(cur_x+1, y+1, it->begin, it->end, shadow, ALPHA_BLEND);
					renderDevice_->OutTextLine(cur_x, y, it->begin, it->end, diffuse, alphaBlend);
					cur_x += it->width;
				}
				break;

			case OutNode::COLOR:
				diffuse.RGBA() = it->color;
				diffuse.a *= alpha;
				shadow.a = round(1.f / 255.f * format->shadowColor().a * diffuse.a);
				break;

			case OutNode::SPRITE:{
				if(skipToNextLine)
					break;
				const UI_Sprite* sprite =  it->sprite;
				Vect2f size = sprite->size();
				if(size.xi() != it->width){
					size.y *= float(it->width) / size.x;
					size.x = it->width;
					gb_RenderDevice3D->SetSamplerData(0, sampler_clamp_linear);
				}
				else
					gb_RenderDevice3D->SetSamplerData(0, sampler_clamp_point);

				int yPos;
				if((it->style & 0x03)== 1)
					yPos = y - (size.yi() - hSize + 1) / 2;
				else
					yPos = y - size.yi() + hSize - 1;

				if(!clampInRange || (cur_x >= x && cur_x + it->width <= rightRange)){
					sColor4c diffuseColor;
					if(it->style & 0x04)
						diffuseColor.set(
						diffuse.r,
						diffuse.g,
						diffuse.b,
						round(1.f / 255.f * diffuse.a * sprite->diffuseColor().a));
					else
						diffuseColor.set(
						sprite->diffuseColor().r,
						sprite->diffuseColor().g,
						sprite->diffuseColor().b,
						round(1.f / 255.f * diffuse.a * sprite->diffuseColor().a));

					if(shadow.a)
						::drawSpriteSolid(cur_x+1, yPos+1, size.xi(), size.yi(), 
						sprite->textureCoords().left(), sprite->textureCoords().top(),
						sprite->textureCoords().width(), sprite->textureCoords().height(),
						sprite->texture(), shadow, ALPHA_BLEND);


					::drawSprite(cur_x, yPos, size.xi(), size.yi(), 
						sprite->textureCoords().left(), sprite->textureCoords().top(),
						sprite->textureCoords().width(), sprite->textureCoords().height(),
						sprite->texture(), diffuseColor, sprite->saturation(), alphaBlend);
				}

				cur_x += it->width;
				break;
									}
		}
	}

	setFont();
	return relativeCoords(Vect2f(cur_x, y));
}

void UI_Render::reparseWildChars(bool dir, const char* in, string& out)
{
	static char wch[] = "&<";
	out.clear();
	if(dir)
		while(char ch = *in){
			if(strchr(wch, ch))
				out.push_back(ch);
			out.push_back(ch);
			++in;
		}
	else
		while(char ch = *in){
			if(strchr(wch, ch) && in[1] == ch)
				++in;
			out.push_back(ch);
			++in;
		}
}

std::string UI_Render::extractFirstLineText(const char* taggedText) const
{
	UI_TextParser parser(defaultFont()->font());
	parser.parseString(taggedText);

	std::string result;
	OutNodes::const_iterator it = parser.outNodes().begin();
	while(it != parser.outNodes().end() && (it == parser.outNodes().begin() || it->type != OutNode::NEW_LINE)){
		if(it->type == OutNode::TEXT){
			string str(it->begin, it->end);
			result += str;
		}
		++it;
	}
	return result;
}


bool UI_Render::setFont(const UI_Font* font) const
{
	if(font && font->font()){
		renderDevice_->SetFont(font->font());
		return true;
	}

	return false;
}

void UI_Render::setFont() const
{
	renderDevice_->SetFont(0);
}

const UI_Font* UI_Render::defaultFont() const
{
	if(!UI_FontLibrary::instance().strings().empty())
		return UI_FontLibrary::instance().strings().begin()->get();

	return defaultFont_;

}

void UI_Render::updateRenderSize()
{
	if(camera_ && camera_->GetRenderTarget()){
        renderSize_.x = camera_->GetRenderTarget()->GetWidth();		
        renderSize_.y = camera_->GetRenderTarget()->GetHeight();		
	}
	else{
		if(gb_RenderDevice && gb_RenderDevice->GetCurRenderWindow()){
			renderSize_.x = gb_RenderDevice->GetSizeX();
			renderSize_.y = gb_RenderDevice->GetSizeY();
		}
		else{
			renderSize_.x = 1;
			renderSize_.y = 1;
		}
	}
}

void UI_Render::setCamera(cCamera* camera)
{
	camera_ = camera;
	updateRenderSize();
}

bool UI_Render::createDefaultFont()
{
	if(defaultFont_)
		releaseDefaultFont();

	defaultFont_ = new UI_Font;
	defaultFont_->setFontName("Arial");
	defaultFont_->setSize(20);
	defaultFont_->createFont("Scripts\\Resource\\Fonts");

	return true;
}

bool UI_Render::releaseDefaultFont()
{
	if(defaultFont_){
		defaultFont_->releaseFont();
		delete defaultFont_;

		defaultFont_ = 0;
		return true;
	}

	return false;
}

Vect2f UI_Render::textSize(const char* text, const UI_Font* font, float fitIn) const
{
	Vect2f size(0.f, 0.f);

	if(!*text)
		return size;

	if(!font)
		font = defaultFont();
	
	UI_TextParser parser(font->font());
	parser.parseString(text, BLACK, fitIn > FLT_EPS ? round(fitIn * windowPosition_.width()) : -1);

	size.x = float(parser.size().x) / float(windowPosition_.width());
	size.y = float(parser.size().y) / float(windowPosition_.height());

	return size;
}

void UI_Render::setInterfaces(cVisGeneric* vis_generic, cInterfaceRenderDevice* render_device)
{
	visGeneric_ = vis_generic;
	renderDevice_ = render_device;
	updateRenderSize();
}

Rectf UI_Render::entireScreenCoords() const
{
	return relativeCoords(Recti(0, 0, renderSize_.x, renderSize_.y));
}

float UI_Render::screenBorderLeft() const
{
	// return -entireScreenCoords().left();
	return float(windowPosition_.left()) / float(windowPosition_.width());
}

float UI_Render::screenBorderTop() const
{
	float aspect = float(renderSize_.x) / float(renderSize_.y);
	if(aspect + FLT_COMPARE_TOLERANCE < 4.0f / 3.0f)
		return 0.0f;
	else
		return float(windowPosition_.top()) / float(windowPosition_.height());
}

float UI_Render::screenBorderRight() const
{
	return float(renderSize_.x - windowPosition_.left()) / float(windowPosition_.width()) - 1.0f;
}

float UI_Render::screenBorderBottom() const
{
	float aspect = float(renderSize_.x) / float(renderSize_.y);
	if(aspect + FLT_COMPARE_TOLERANCE < 4.0f / 3.0f)
		return 0.0f;
	else
		return float(renderSize_.y - windowPosition_.top()) / float(windowPosition_.height()) - 1.0f;
}

Vect2i UI_Render::screenCoords(const Vect2f& rel_coords) const
{
    Vect2f vec = rel_coords * Vect2f(windowPosition_.size());

    return Vect2i(vec.xi(), vec.yi()) + windowPosition_.left_top();
}

Recti UI_Render::screenCoords(const Rectf& rel_coords) const
{
	Rectf rect = rel_coords * Vect2f(windowPosition_.size());

	return Recti(round(rect.left()) + windowPosition_.left(), 
		round(rect.top()) + windowPosition_.top(),
		max(1, round(rect.width())), max(1, round(rect.height())));

	/*
	Vect2i left_top = screenCoords(rel_coords.left_top());
	Vect2i right_bottom = screenCoords(rel_coords.right_bottom());
    
	return Recti(left_top, right_bottom - left_top);
		*/
}

Recti UI_Render::device2screenCoords(const Rectf& device_coords) const
{
	if(!renderDevice_)
		return Recti(0,0,1,1);

	return Recti((device_coords.left_top() + Vect2f(0.5f, 0.5f)) * Vect2f(renderSize_.x, renderSize_.y),
		device_coords.size() * Vect2f(renderSize_.x, renderSize_.y));
}

Vect2f UI_Render::relativeSize(const Vect2i& screen_size) const
{
	return Vect2f(
		screen_size.x / float(windowPosition_.width()),
		screen_size.y / float(windowPosition_.height()));
}

Vect2i UI_Render::screenSize(const Vect2f& rel_size) const
{
	return Vect2i(
		round(rel_size.x * float(windowPosition_.width())),
		round(rel_size.y * float(windowPosition_.height())));
}

Vect2f UI_Render::relativeCoords(const Vect2i& screen_coords) const
{
    return Vect2f(screen_coords - Vect2i(windowPosition_.left_top()))
		  * Vect2f(1.0f / windowPosition_.width(), 1.0f / windowPosition_.height());
}

Rectf UI_Render::relativeCoords(const Recti& screen_coords) const
{
    Vect2f top_left = relativeCoords(Vect2i(screen_coords.left_top()));
	Vect2f right_bottom = relativeCoords(Vect2i(screen_coords.right_bottom()));
    return Rectf(top_left, right_bottom - top_left);
}

Vect2f UI_Render::relativeSpriteSize(const UI_Sprite& sprite) const
{
	if(sprite.isEmpty())
		return Vect2f(0, 0);

	return Vect2f(float(sprite.texture()->GetWidth()) * sprite.textureCoords().width() / 1024.f,
		float(sprite.texture()->GetHeight()) * sprite.textureCoords().height() / 768.f);
}

Vect2f UI_Render::deviceCoords(const Vect2i& screen_coords) const
{
	if(!renderDevice_)
		return Vect2f(0,0);

	return Vect2f(float(screen_coords.x) / float(renderSize_.x),
		float(screen_coords.y) / float(renderSize_.y)) - Vect2f(0.5f, 0.5f);
}

Rectf UI_Render::deviceCoords(const Recti& screen_coords) const
{
	if(!renderDevice_)
		return Rectf(0,0,1,1);

	return Rectf(float(screen_coords.left()) / float(renderSize_.x),
		float(screen_coords.top()) / float(renderSize_.y),
		float(screen_coords.width()) / float(renderSize_.x),
		float(screen_coords.height()) / float(renderSize_.y)) - Vect2f(0.5f, 0.5f);
}

Vect2f UI_Render::device2relativeCoords(const Vect2f& device_coords) const
{
	if(!renderDevice_)
		return Vect2f::ZERO;

	return relativeCoords((device_coords + Vect2f(0.5f, 0.5f)) * Vect2f(renderSize_.x, renderSize_.y));
}

Vect2f UI_Render::relative2deviceCoords(const Vect2f& relative_coords) const
{
	return deviceCoords(screenCoords(relative_coords));
}

Rectf UI_Render::relative2deviceCoords(const Rectf& relative_coords) const
{
	return deviceCoords(screenCoords(relative_coords));
}

void UI_Render::setFontDirectory(const char* path)
{
	fontDirectory_ = path;

	if(!fontDirectory_.empty() && *fontDirectory_.rbegin() != '\\')
		fontDirectory_ += "\\";
}


void UI_Render::setWindowPosition(const Recti& windowPosition)
{
	bool windowPositionChanged = (windowPosition_ != windowPosition);
	windowPosition_ = windowPosition;

	if(windowPositionChanged)
		createFonts();
	updateRenderSize();

	UI_BackgroundScene::instance().setCamera();
}

Vect2f UI_Render::mousePosition() const
{
	if(renderDevice_){
		POINT pt;
		GetCursorPos(&pt);
		if(ScreenToClient(renderDevice_->GetWindowHandle(), &pt))
			return relativeCoords(Vect2i(pt.x, pt.y));
	}

	return Vect2f(0,0);
}

void UI_UnitSprite::draw(const Vect3f& worldCoord, float zShift, const Vect2f& slot) const
{
	if(!isEmpty())
	{
		Vect3f pos(worldCoord.x, worldCoord.y, worldCoord.z + zShift);
		Vect3f e;
		Vect3f pv;
		cameraManager->GetCamera()->ConvertorWorldToViewPort(&pos,&pv,&e);

		if(pv.z > 0)
		{
			float radiusFactor = cameraManager->GetCamera()->GetFocusViewPort().x / pv.z;

			Vect2f sz = size();
			sz *= radiusFactor;

			Vect2f off = offset();
			off *= radiusFactor;

			UI_Render::instance().drawSprite(UI_Render::instance().relativeCoords(Recti(round(e.x - sz.x/2.f + off.x + slot.x * sz.x), round(e.y - sz.y/2.f + off.y + slot.y * sz.y),	sz.xi(), sz.yi())), *this);
		}
	}
}

void UI_Render::createFonts()
{
	createDefaultFont();
	for(UI_FontLibrary::Strings::const_iterator itf = UI_FontLibrary::instance().strings().begin(); itf != UI_FontLibrary::instance().strings().end(); ++itf)
		if(itf->get())
			itf->get()->createFont();
}


