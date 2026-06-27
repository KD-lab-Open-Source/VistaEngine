// FreeType stub for the cross-platform build.
//
// The font rasterizer (Render/src/FT_Font.cpp, ftrender.cpp) links against
// FreeType, which is vendored under XLibs.Net/FreeType but not yet built here.
// These stubs let the engine link with font rasterization disabled (FT_Init
// fails, so the font path is skipped); building the vendored FreeType library is
// a later step. The freetype headers are included for the exact C signatures and
// extern "C" linkage.
#include <ft2build.h>
#include FT_FREETYPE_H

extern "C" {

FT_Error FT_Init_FreeType(FT_Library* alibrary)
{
	if(alibrary) *alibrary = 0;
	return 1; // non-zero: initialisation failed, font path is skipped
}

FT_Error FT_Done_FreeType(FT_Library) { return 0; }

FT_Error FT_New_Face(FT_Library, const char*, FT_Long, FT_Face* aface)
{
	if(aface) *aface = 0;
	return 1;
}

FT_Error FT_Done_Face(FT_Face) { return 0; }

FT_Error FT_Set_Pixel_Sizes(FT_Face, FT_UInt, FT_UInt) { return 1; }

FT_Error FT_Load_Glyph(FT_Face, FT_UInt, FT_Int32) { return 1; }

FT_Error FT_Render_Glyph(FT_GlyphSlot, FT_Render_Mode) { return 1; }

FT_UInt FT_Get_Char_Index(FT_Face, FT_ULong) { return 0; }

} // extern "C"
