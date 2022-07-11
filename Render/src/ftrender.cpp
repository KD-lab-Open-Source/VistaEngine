#include "StdAfxRD.h"
#include "ftrender.h"

namespace FT {

FT_Render::FT_Render(const char* fontName)
: face_(0)
{
	FT_Error error = FT_Init_FreeType(&library_);
	
	monochrome_ = true;
	native_hinting_ = true;
	auto_hinting_ = true;

	loadFont(fontName);
}

FT_Render::~FT_Render()
{
	releaseFont();
	FT_Done_FreeType(library_);
}

void FT_Render::releaseFont()
{
	if(face_){
		FT_Done_Face(face_);
		face_ = 0;
	}
}

bool FT_Render::loadFont(const char* ttf)
{
	if(!ttf || !*ttf)
		return false;

	FT_Face newFace;
	if(FT_New_Face(library_, ttf, 0, &newFace) == 0){
		if(FT_IS_SCALABLE(newFace) == 0){
			FT_Done_Face(newFace);
			return false;
		}
		releaseFont();
		face_ = newFace;
		return true;
	}

	return false;
}

bool FT_Render::setSize(unsigned short pixels)
{
	if(pixels <= 1)
		return false;

	if(!face_)
		return false;

	FT_Set_Pixel_Sizes(face_, 0, pixels);
	return true;
}

unsigned short FT_Render::lineHeight() const
{
	return face_ ? (face_->ascender - face_->descender + 1) >> 6 : 0;	
}

FT_GlyphSlot FT_Render::getGlyph(unsigned long ch)
{
	/* hinting - уточнение положения растеризованного пикселя. есть 2 способа:
	1 - патентованный Apple, с помошью встроенного в шрифт алгоритма (bytecode_interpreter)
	2 - собственный FreeType, срабатывает хуже первого, но лучше чем вообще никакого
	По умолчанию, сначала пытается примениться 1й способ, если не достуен, то 2й. */

	FT_Int32 loadFlags = 0;
	FT_Render_Mode renderMode = FT_RENDER_MODE_NORMAL;

	if(!auto_hinting_ || !native_hinting_)
		if(native_hinting_)
			loadFlags |= FT_LOAD_NO_AUTOHINT;
		else if(auto_hinting_)
			loadFlags |= FT_LOAD_FORCE_AUTOHINT;
		else
			loadFlags |= FT_LOAD_NO_HINTING;

	if(monochrome_){
		loadFlags |= FT_LOAD_TARGET_MONO;
		renderMode = FT_RENDER_MODE_MONO;
	}
	else if(!auto_hinting_ || !native_hinting_){
		if(native_hinting_){
			loadFlags |= FT_LOAD_TARGET_NORMAL;
		}
		else if(auto_hinting_){
			loadFlags |= FT_LOAD_TARGET_LIGHT;
			renderMode = FT_RENDER_MODE_LIGHT;
		}
	}
	else
		loadFlags |= FT_LOAD_DEFAULT;
	
    FT_Load_Glyph(face_, FT_Get_Char_Index(face_, ch), loadFlags);
	FT_Render_Glyph(face_->glyph, renderMode);

	return face_->glyph;
}

bool FT_Render::draw(unsigned long ch, unsigned char* buffer, unsigned short pitch)
{
	return draw(getGlyph(ch), buffer, pitch);
}

bool FT_Render::draw(FT_GlyphSlot slot, unsigned char* buffer, unsigned short pitch)
{
	if(!slot)
		return false;

	const BYTE* bitmap = slot->bitmap.buffer;

	switch (slot->bitmap.pixel_mode){
	case FT_PIXEL_MODE_GRAY:{
		for(int y = 0; y < slot->bitmap.rows; ++y){
			memcpy(buffer, bitmap, slot->bitmap.width);
			buffer += pitch;
			bitmap += slot->bitmap.pitch;
		}
		break;
							}
	case FT_PIXEL_MODE_MONO:{
		for(int y = 0; y < slot->bitmap.rows; ++y){
			for(int x = 0; x < slot->bitmap.width; ++x)
				buffer[x] = ((bitmap[(y * slot->bitmap.pitch) + x / 8] << (x % 8)) & 0x80) ? 0xFF : 0x00;
			buffer += pitch;
		}
		break;
							}
	default:
		return false;
	}

	return true;
}

}