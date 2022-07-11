#include "StdAfx.h"

#include "UI_Render.h"

#include "Render\d3d\D3DRender.h"
#include "Render\src\cCamera.h"

#include "UI_GlobalAttributes.h"
#include "UI_TextParser.h"
#include "UI_Types.h"

#include "Game\CameraManager.h"
#include "Serialization\StringTable.h"
#include "UnicodeConverter.h"


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

UI_Render::UI_Render()
: camera_(0)
{
	self_ = this;
	defaultFont_ = new UI_Font();
	UI_GlobalAttributes::instance(); // загрузка параметров UI_Render
}

UI_Render::~UI_Render()
{
	self_ = 0;
}

void UI_Render::create()
{
	xassert(!self_);
	static UI_Render obj;
	self_ = &obj;
}

void UI_Render::init()
{
	updateRenderSize();
	createFonts();
}

void UI_Render::releaseResources()
{
	defaultFont_->releaseFont();
	delete defaultFont_;
	defaultFont_ = 0;

	UI_FontLibrary::Strings::const_iterator itf;
	FOR_EACH(UI_FontLibrary::instance().strings(), itf){
		const UI_Font& fnt = *itf;
		const_cast<UI_Font&>(fnt).releaseFont();
	}
}

void UI_Render::drawSprite(const Rectf& pos, const UI_Sprite& sprite, const Color4f& color, UI_BlendMode blend_mode, bool tiled, float phase) const
{
	if(sprite.isEmpty())
		return;

	Recti scr_pos = screenCoords(pos);
	Rectf txt_pos = sprite.textureCoords();

	Color4c diffuseColor(	round(color.r * sprite.diffuseColor().r),
							round(color.g * sprite.diffuseColor().g),
							round(color.b * sprite.diffuseColor().b),
							round(color.a * sprite.diffuseColor().a));

	gb_RenderDevice3D->SetSamplerData(0, sampler_clamp_linear);

	if(tiled)
		drawSpriteTiled(scr_pos.left(), scr_pos.top(), scr_pos.width(), scr_pos.height(), 
						txt_pos.left(), txt_pos.top(), txt_pos.width(), txt_pos.height(),
						sprite.texture(), diffuseColor, sprite.saturation(), convertBlendMode(blend_mode), phase);
	else
		UI_RenderBase::drawSprite(scr_pos.left(), scr_pos.top(), scr_pos.width(), scr_pos.height(), 
					txt_pos.left(), txt_pos.top(), txt_pos.width(), txt_pos.height(),
					sprite.texture(), diffuseColor, sprite.saturation(), convertBlendMode(blend_mode), phase);

}

void UI_Render::drawSprite(const Rectf& pos, cTexture* texture, const Color4f& color, UI_BlendMode blend_mode, float phase) const
{
	Recti scr_pos = screenCoords(pos);

	gb_RenderDevice3D->SetSamplerData(0, sampler_clamp_linear);

	UI_RenderBase::drawSprite(scr_pos.left(), scr_pos.top(), scr_pos.width(), scr_pos.height(),
				0.0f, 0.0f, 1.0f, 1.0f,
				texture, color, 1.f, convertBlendMode(blend_mode), phase);
}


void UI_Render::drawSprite(const Rectf& pos, cTexture* texture, const Rectf& txt_pos, const Color4f& color, UI_BlendMode blend_mode) const
{
	Recti scr_pos = screenCoords(pos);

	gb_RenderDevice3D->SetSamplerData(0, sampler_clamp_linear);

	UI_RenderBase::drawSprite(scr_pos.left(), scr_pos.top(), scr_pos.width(), scr_pos.height(),
				txt_pos.left(), txt_pos.top(), txt_pos.width(), txt_pos.height(),
				texture, color, 1.f, convertBlendMode(blend_mode));
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

void UI_Render::outDebugText(const Vect2f& pos, const char* text, const Color4c* color) const
{
	if(!text || !*text)
		return;

	Color4c clr(Color4c::WHITE);
	if(!color)
		color = &clr;

	unsigned char bright = max(color->r, color->g, color->b) > 128 ? 0 : 255;
	Color4c shadow(bright, bright, bright);

	UI_TextParserAnsiWrapper parser(defaultFont()->font());
	parser.parseString(text, *color);

	Vect2f text_size =  relativeSize(parser.size());
	Vect2f text_pos(clamp(pos.x + text_size.x, text_size.x, 1.f), clamp(pos.y + text_size.y, text_size.y, 1.f));
	text_pos -= text_size;
	
	drawRectangle(Rectf(text_pos, text_size), Color4f(0, 0, 0, 0.4f));

	outText(Rectf(text_pos, Vect2f::ZERO), parser, parser.outNodes().begin(), parser.outNodes().end(), &UI_TextFormat(*color, shadow), 0, 1.f);
}

Vect2f UI_Render::outText(const Rectf& pos, const wchar_t* text, const UI_TextFormat* format, int textAlign, const UI_Font* font, float alpha, bool formatText) const
{
	xassert(text);

	if(!*text)
		return Vect2f(pos.left() + textStart(textAlign, pos.width()), pos.top());

	if(!font || !font->font())
		font = defaultFont();

	UI_TextParser parser(font->font());
	parser.parseString(text,
		format ? format->textColor() : Color4c::BLACK,
		formatText ? screenSize(pos.size()).x : -1);

	return outText(pos, parser, parser.outNodes().begin(), parser.outNodes().end(), format, textAlign, alpha);
}

Vect2f UI_Render::outText(const Rectf& pos, const UI_TextParser& parser, OutNodes::const_iterator begin, OutNodes::const_iterator end, const UI_TextFormat* format, int textAlign, float alpha, bool clampInRange) const
{
	if(!format)
		format = &UI_TextFormat::WHITE;

	if(!parser.font() || begin == end)
		return Vect2f(pos.left() + textStart(textAlign, pos.width()), pos.top());

	xassert(begin->type == OutNode::NEW_LINE);

	const FT::Font& ftfont = *parser.font();

	Recti scr_pos = screenCoords(pos);

	if(clampInRange && scr_pos.width() < 3)
		return Vect2f(pos.left() + textStart(textAlign, pos.width()), pos.top());

	int x = scr_pos.left();
	int y = scr_pos.top();

	int hSize = parser.font()->lineHeight();
	int fSize = parser.font()->size();
	y -= hSize; // первым тегом идет служебный перенос строки

	if((textAlign & UI_TEXT_VALIGN) != UI_TEXT_VALIGN_TOP){

		Vect2i parseSize = parser.size();
		if(begin != parser.outNodes().begin() || end != parser.outNodes().end()){
			parseSize = Vect2i::ZERO;
			int width = 0;
			for(OutNodes::const_iterator it = begin; it != end; ++it)
				switch(it->type){
					case OutNode::NEW_LINE:
						parseSize.y += hSize;
						parseSize.x = max(parseSize.x, width);
						width = 0;
						break;
					case OutNode::TEXT:
					case OutNode::SPRITE:
						width += it->width;
						break;
				}
			if(parseSize.y == hSize)
				parseSize.y = fSize;
			parseSize.x = max(parseSize.x, width);
		}

		switch(textAlign & UI_TEXT_VALIGN){
		case UI_TEXT_VALIGN_CENTER:
			y += (scr_pos.height() - parseSize.y) / 2;
			break;
		case UI_TEXT_VALIGN_BOTTOM:
			y += scr_pos.height() - parseSize.y;
			break;
		}
	}

	int rightRange = scr_pos.right();
	
	if(rightRange - x < 2)
		rightRange += 10;

	Color4c diffuse = format->textColor();
	diffuse.a *= alpha;
	Color4c shadow = format->shadowColor();
	shadow.a *= 1.f / 255.f * diffuse.a;
	eBlendMode alphaBlend = (ftfont.param().antialiasing || fabs(diffuse.a - 1.f) > FLT_EPS || shadow.a > 0.f ? ALPHA_BLEND : ALPHA_TEST);

	bool skipToNextLine = false;
	int cur_x = x;
	for(OutNodes::const_iterator it = begin; it != end; ++it){
		switch(it->type){
			case OutNode::NEW_LINE:
				y += hSize;
				if(clampInRange){
					if(y < scr_pos.top() || y + hSize > scr_pos.bottom()){
						skipToNextLine = true;
						break;
					}
				}
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
				skipToNextLine = false;
				break;

			case OutNode::TEXT:
				if(skipToNextLine)
					break;
				if(clampInRange){
					if(shadow.a)
						gb_RenderDevice3D->OutTextLine(cur_x+1, y+1, ftfont, it->begin, it->end, shadow, ALPHA_BLEND, x, rightRange + 1);
					int posx = gb_RenderDevice3D->OutTextLine(cur_x, y, ftfont, it->begin, it->end, diffuse, alphaBlend, x, rightRange);
					cur_x += it->width;
					if(cur_x >= rightRange){
						cur_x = posx;
						skipToNextLine = true;
					}
				}
				else {
					if(shadow.a)
						gb_RenderDevice3D->OutTextLine(cur_x+1, y+1, ftfont, it->begin, it->end, shadow, ALPHA_BLEND);
					gb_RenderDevice3D->OutTextLine(cur_x, y, ftfont, it->begin, it->end, diffuse, alphaBlend);
					cur_x += it->width;
				}
				break;
			
			case OutNode::SPACE:
				if(skipToNextLine)
					break;
				cur_x += it->width;
				if(clampInRange && cur_x >= rightRange)
					skipToNextLine = true;
				break;

			case OutNode::ALPHA:
				alpha = it->style / 255.f;
				diffuse.a *= alpha;
				shadow.a = round(1.f / 255.f * format->shadowColor().a * diffuse.a);
				alphaBlend = (ftfont.param().antialiasing || fabs(diffuse.a - 1.f) > FLT_EPS || shadow.a > 0.f ? ALPHA_BLEND : ALPHA_TEST);
				break;

			case OutNode::COLOR:
				diffuse.RGBA() = it->color;
				diffuse.a *= alpha;
				shadow.a = round(1.f / 255.f * format->shadowColor().a * diffuse.a);
				alphaBlend = (ftfont.param().antialiasing || fabs(diffuse.a - 1.f) > FLT_EPS || shadow.a > 0.f ? ALPHA_BLEND : ALPHA_TEST);
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
				if((it->style & 0x03) == 1)
					yPos = y + (hSize - size.yi()) / 2;
				else
					yPos = y + fSize - size.yi();

				if(!clampInRange || (cur_x >= x && cur_x + it->width <= rightRange)){
					Color4c diffuseColor;
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
						drawSpriteSolid(cur_x+1, yPos+1, size.xi(), size.yi(), 
							sprite->textureCoords().left(), sprite->textureCoords().top(),
							sprite->textureCoords().width(), sprite->textureCoords().height(),
							sprite->texture(), shadow, ALPHA_BLEND);


					UI_RenderBase::drawSprite(cur_x, yPos, size.xi(), size.yi(), 
						sprite->textureCoords().left(), sprite->textureCoords().top(),
						sprite->textureCoords().width(), sprite->textureCoords().height(),
						sprite->texture(), diffuseColor, sprite->saturation(), alphaBlend);
				}

				cur_x += it->width;
				break;
									}
		}
		if(skipToNextLine && clampInRange && y + hSize > scr_pos.bottom())
			break;
	}

	return relativeCoords(Vect2f(cur_x, y));
}

Vect2f UI_Render::textSize(const wchar_t* text, const UI_Font* font, float fitIn) const
{
	if(!*text)
		return Vect2f::ZERO;

	if(!font)
		font = defaultFont();

	UI_TextParser parser(font->font());
	parser.parseString(text, Color4c::BLACK, fitIn > FLT_EPS ? round(fitIn * windowPosition_.width()) : -1);

	return relativeSize(parser.size());
}

void UI_Render::reparseWildChars(bool dir, const wchar_t* in, wstring& out)
{
	static wchar_t wch[] = L"&<";
	out.clear();
	if(dir)
		while(wchar_t ch = *in){
			if(wcschr(wch, ch))
				out.push_back(ch);
			out.push_back(ch);
			++in;
		}
	else
		while(wchar_t ch = *in){
			if(wcschr(wch, ch) && in[1] == ch)
				++in;
			out.push_back(ch);
			++in;
		}
}

std::string UI_Render::extractFirstLineText(const wchar_t* taggedText) const
{
	UI_TextParser parser(defaultFont()->font());
	parser.parseString(taggedText);

	std::wstring result;
	OutNodes::const_iterator it = parser.outNodes().begin();
	while(it != parser.outNodes().end() && (it == parser.outNodes().begin() || it->type != OutNode::NEW_LINE)){
		if(it->type == OutNode::TEXT){
			wstring str(it->begin, it->end);
			result += str;
		}
		++it;
	}
	return w2a(result);
}

// --------------------------------------------------------------------------


void UI_Render::updateRenderSize()
{
	if(camera_ && camera_->GetRenderTarget())
        setRenderSize(Vect2i(camera_->GetRenderTarget()->GetWidth(), camera_->GetRenderTarget()->GetHeight()));
	else if(gb_RenderDevice && gb_RenderDevice->currentRenderWindow())
		setRenderSize(Vect2i(gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY()));
	else
		setRenderSize(Vect2i(1, 1));
}

void UI_Render::setWindowPosition(const Recti& windowPosition)
{
	bool windowPositionChanged = (windowPosition_ != windowPosition);

	UI_RenderBase::setWindowPosition(windowPosition);

	if(windowPositionChanged)
		createFonts();
	updateRenderSize();

	UI_BackgroundScene::instance().setCamera();
}

void UI_Render::setCamera(Camera* camera)
{
	camera_ = camera;
	updateRenderSize();
}

// --------------------------------------------------------------------------

Rectf UI_Render::entireScreenCoords() const
{
	return relativeCoords(Recti(0, 0, renderSize_.x, renderSize_.y));
}

float UI_Render::screenBorderLeft() const
{
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

// --------------------------------------------------------------------------

const UI_Font* UI_Render::defaultFont() const
{
	xassert(defaultFont_);

	if(defaultFont_->font())
		return defaultFont_;

	if(!defaultFont_->createFont()){
		(*defaultFont_) = UI_Font("Scripts\\Resource\\fonts\\default.ttf", 16);
		if(!defaultFont_->createFont())
			if(!UI_FontLibrary::instance().strings().empty()){
				(*defaultFont_) = UI_Font(UI_FontLibrary::instance().strings().front().file(), 16);
				defaultFont_->createFont();
			}
	}

	xxassert(defaultFont_->font(), "Не удалось создать фонт по умолчанию");
	return defaultFont_;
}

void UI_Render::createFonts(bool force)
{
	if(force && defaultFont_)
		defaultFont_->releaseFont();

	defaultFont();
	defaultFont_->createFont();

	UI_FontLibrary::Strings::const_iterator itf;
	FOR_EACH(UI_FontLibrary::instance().strings(), itf){
		UI_Font& fnt = const_cast<UI_Font&>(safe_cast_ref<const UI_Font&>(*itf));
		if(force)
			fnt.releaseFont();
		fnt.createFont();
	}
}

void UI_Render::serialize(Archive& ar)
{
	xassert(defaultFont_);
	if(ar.isInput())
		defaultFont_->releaseFont();
	ar.serialize(*defaultFont_, "defaultFont", "Фонт по умолчанию");
}

// --------------------------------------------------------------------------


bool UI_UnitSprite::draw(const Vect3f& worldCoord, float zShift, const Color4f& diffuse, const Vect2f& slot, float scale, Recti* posOut) const
{
	if(!isEmpty())
	{
		Vect3f pos(worldCoord.x, worldCoord.y, worldCoord.z + zShift);
		Vect3f e;
		Vect3f pv;
		cameraManager->GetCamera()->ConvertorWorldToViewPort(&pos,&pv,&e);

		if(pv.z > 1.f)
		{
			const float focus = cameraManager->GetCamera()->GetFocusViewPort().x;

			float radiusFactor = focus / pv.z;

			if(radiusFactor < hideScale_)
				return false;

			radiusFactor = scale * clamp(radiusFactor, minPerspectiveScale_, maxPerspectiveScale_);

			Vect2f sz = size();
			sz *= radiusFactor;

			Vect2f off = spriteOffset_;
			off *= radiusFactor;

			Rectf pos(e.x - sz.x/2.f + off.x + slot.x * sz.x, e.y - sz.y/2.f + off.y + slot.y * sz.y, sz.x, sz.y);
			if(posOut)
				*posOut = Recti(Vect2i(pos.left_top()), Vect2i(pos.size()));


			UI_Render& render = UI_Render::instance();

			Vect2f left_top(render.relativeCoords(pos.left_top()));
			Vect2f right_bottom(render.relativeCoords(pos.right_bottom()));
			render.drawSprite(Rectf(left_top, right_bottom - left_top), *this, useLegionColor_ ? diffuse : Color4f::WHITE);

			if(showDebugInterface.showDebugSpriteScale)
				render.outDebugText(left_top, (XBuffer() <= radiusFactor).c_str());

			return true;
		}
	}
	return false;
}