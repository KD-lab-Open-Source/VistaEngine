#include "StdAfxRD.h"
#include "FT_Font.h"
#include "ftrender.h"
#include "TexLibrary.h"
#include "Texture.h"
#include <math.h>

void dprintfW(wchar_t *format, ...);

// расстояние между буквами в текстуре
#define GLYPH_PAD_SPACE 2

namespace FT {

Font::Font()
{
	lineHeight_ = size_ = 0;

	texture_ = 0;
	charMap_ = fontManager().charMap();
}

int Font::textWidth(const wchar_t* str) const
{
	int width = 0;
	int widthMax = 0;

	wchar_t cur;
	for(const wchar_t* ch = str; cur = *str; ++str){
		if(cur < 32)
			continue;

		if(cur == L'\n'){
			widthMax = max(width, widthMax);
			width = 0;
		}

		width += getChar(cur).advance;
	}

	return max(width, widthMax);
}

int Font::textHeight(const wchar_t *str) const
{
	float height = size_;

	wchar_t cur;
	for(const wchar_t* ch = str; cur = *str; ++str)
		if(cur == L'\n')
			height += lineHeight_;

	return height;
}

int Font::lineWidth(const wchar_t *str) const
{
	int width = 0;

	wchar_t cur;
	for(const wchar_t* ch = str; cur = *str; ++str){
		if(cur == L'\n')
			break;
		if(cur < 32)
			continue;

		width += getChar(cur).advance;
	}

	return width;
}

const FontParam FontManager::defParam;

FontManager::FontManager()
{
	// изначально все ссылаются на нулевой индекс
	ZeroMemory(index_, sizeof(index_));

	render_ = new FT_Render();

	//dprintfW(L"Создан FontManager\n");

	chars_.push_back(L' '); // в 0 - Символ по умолчанию
	addCharPage(1252, true);
	addCharPage(1251, false);

	dprintfW(L"Добавлено %d символов\n", chars_.size());
}

FontManager::~FontManager()
{
	delete render_;
}

void FontManager::addChar(uint16 utf)
{
	if(index_[utf] == 0){
		index_[utf] = static_cast<uint16>(chars_.size());
		chars_.push_back(utf);
	}
}

void FontManager::addCharPage(uint16 page, bool all_span)
{
	//dprintfW(L"Добавляется страница: %d %s\n", page, all_span ? L" полностью" : L"");
	WCHAR wch[2];
	char ch[2];
	int code = (all_span ? 0x21 : 0x80);
	for(; code != 0x100; ++code){
		*ch = static_cast<unsigned char>(code);
		MultiByteToWideChar(page, MB_PRECOMPOSED, ch, 1, wch, 2);
		uint16 utf = wch[0];
		//dprintfW(L"0x%02X -> 0x%04X %c -> %d%s\n", code, utf, utf, index_[utf] == 0 ? chars_.size() : index_[utf], index_[utf] != 0 ? L" дубль" : L"");
		addChar(utf);
	}
}

FontManager::ShortSize FontManager::calcTextureSize(bool nonPow2)
{
	FontManager::ShortSize size, max_size;
	size.x = size.y = 128;
	max_size.x = max_size.y = 2048;

	while(size.y <= max_size.y && size.x <= max_size.x){
		//dprintfW(L"<------------------ рассчитывается размер, попытка на %d*%d\n", size.x, size.y);
		Chars::const_iterator ch_it = chars_.begin();
		uint16 x = GLYPH_PAD_SPACE;
		uint16 y = GLYPH_PAD_SPACE;
		uint16 max_line_bottom = GLYPH_PAD_SPACE;
		for(; ch_it != chars_.end(); ++ch_it){
			FT_GlyphSlot glyph = render_->getGlyph(*ch_it);
			if(!glyph)
				continue;

			uint8 glyph_w = min(glyph->bitmap.width, 255 - GLYPH_PAD_SPACE);
			uint8 glyph_h = min(glyph->bitmap.rows, 255);

			//dprintfW(L"%d (%c) -> %d; %d\n", *ch_it, *ch_it, glyph_w, glyph_h);

			if(x + GLYPH_PAD_SPACE + glyph_w > size.x){
				//dprintfW(L"Перенос строки: x = %d\n", x);
				x = GLYPH_PAD_SPACE;
				y = max_line_bottom + GLYPH_PAD_SPACE;
			}

			x += glyph_w + GLYPH_PAD_SPACE;

			uint16 y_bottom = y + glyph_h;
			if(y_bottom > size.y){
				if(size.y < size.x)
					size.y *= 2;
				else {
					size.x *= 2;
					if(!nonPow2)
						size.y /= 2;
				}
				break;
			}

			if(y_bottom > max_line_bottom)
				max_line_bottom = y_bottom;

		}
		if(ch_it == chars_.end()){
			if(nonPow2)
				size.y = max_line_bottom + max_line_bottom % 2;
			break;
		}
	}

	return size;
}

Font* FontManager::createFont(const char* ttf, uint8 font_pixel_size, const FontParam* prm)
{
	if(!ttf || !*ttf)
		return 0;
	
	if(!prm)
		prm = &defParam;
	
	//dprintfW(L"Создается шрифт: %S размер: %d пикселов\n", ttf, font_pixel_size);
	if(!render_->loadFont(ttf)){
		//dprintfW(L"Создать шрифт не удалось\n");
		return 0;
	}
	render_->setMonochrome(!prm->antialiasing);

	if(prm->hinting == FontParam::DEFAULT)
		render_->setHinting(true, true);
	else
		render_->setHinting(prm->hinting == FontParam::BYTE_CODE_ONLY, prm->hinting == FontParam::AUTO_HINT_ONLY);

	if(!render_->setSize(font_pixel_size)){
		render_->releaseFont();
		return 0;
	}

	FontManager::ShortSize size = calcTextureSize(prm->nonPow2);
	//dprintfW(L"Размер текстуры: %d*%d\n", size.x, size.y);

	Font* font = new Font();
	font->param_ = *prm;
	font->size_ = font_pixel_size;
	
	if(!(font->texture_ = GetTexLibrary()->CreateAlphaTexture(size.x, size.y))){
		delete font;
		return 0;
	}

	int pitch;
	BYTE* buf = font->texture_->LockTexture(pitch);

	ZeroMemory(buf, size.y * pitch);

	Chars::const_iterator ch_it = chars_.begin();

	int16 font_top = font_pixel_size;
	int16 font_bottom = 0;

	uint16 x = GLYPH_PAD_SPACE;
	uint16 y = GLYPH_PAD_SPACE;
	uint16 max_line_bottom = GLYPH_PAD_SPACE;

	for(; ch_it != chars_.end(); ++ch_it){
		font->charTable_.push_back(OneChar());
	
		FT_GlyphSlot glyph = render_->getGlyph(*ch_it);
		if(!glyph)
			continue;

		uint8 glyph_w = min(glyph->bitmap.width, 255 - GLYPH_PAD_SPACE);
		uint8 glyph_h = min(glyph->bitmap.rows, 255);

		//dprintfW(L"%d (%c) -> %d; %d\n", *ch_it, *ch_it, glyph_w, glyph_h);

		if(x + GLYPH_PAD_SPACE + glyph_w > size.x){
			//dprintfW(L"Перенос строки: x = %d\n", x);
			x = GLYPH_PAD_SPACE;
			y = max_line_bottom + GLYPH_PAD_SPACE;
		}

		uint16 y_bottom = y + glyph_h;
		if(y_bottom > max_line_bottom){
			max_line_bottom = y_bottom;
			
			if(y_bottom > size.y){
				font->texture_->UnlockTexture();
				render_->releaseFont();
				releaseFont(font);
				dprintfW(L"Не влезли в рассчитанную текстуру: %d*%d\nШрифт:%S, размер:%d\n", size.x, size.y, ttf, font_pixel_size);
				return 0;
			}
		}

		render_->draw(glyph, buf + y * pitch + x, pitch);

		OneChar& one = font->charTable_.back();
		one.u = x;
		one.v = y;
		one.du = glyph_w;
		one.dv = glyph_h;
		one.su = glyph->bitmap_left;
		one.sv = (int)font_pixel_size - glyph->bitmap_top;
		one.rh = (glyph->rsb_delta > 127 ? 127 : (glyph->rsb_delta < -127 ? -128 : uint8(glyph->rsb_delta)));
		one.lh = (glyph->lsb_delta > 127 ? 127 : (glyph->lsb_delta < -127 ? -128 : uint8(glyph->lsb_delta)));
		one.advance = uint8(glyph->metrics.horiAdvance / 64.f + 0.5f);

		if(one.sv < font_top)
			font_top = one.sv;

		if(one.sv + one.dv > font_bottom)
			font_bottom = one.sv + one.dv;

		if(prm->inBox){
			for(int ix = one.u; ix < x + one.du; ++ix){
				buf[one.v * pitch + ix] = 0x8F;
				buf[(one.v + one.dv - 1) * pitch + ix] = 0x8F;
			}
			for(int iy = one.v; iy < one.v + one.dv; ++iy){
				buf[iy * pitch + one.u] = 0x8F;
				buf[iy * pitch + (one.u + one.du - 1)] = 0x8F;
			}
		}
		
		x += glyph_w + GLYPH_PAD_SPACE;
	}

	if(font_top != 0){
		Font::CharTable::iterator it = font->charTable_.begin();
		for(;it != font->charTable_.end(); ++it)
			it->sv -= font_top;
	}

	font->lineHeight_ = font_bottom - font_top;
	
	font->texture_->UnlockTexture();
	render_->releaseFont();
	return font;
}

void FontManager::releaseFont(Font*& font)
{
	if(!font)
		return;
	if(font->texture_)
		font->texture_->Release();
	delete font;
	font = 0;
}

FontManager& fontManager()
{
	static FontManager manager;
	return manager;
}

} // namespace FT