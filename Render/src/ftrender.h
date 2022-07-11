#ifndef __FT_VISTA_RENDER_
#define __FT_VISTA_RENDER_

#include <ft2build.h>
#include FT_FREETYPE_H

typedef unsigned char BYTE;

namespace FT {

class FT_Render
{
public:
	FT_Render(const char* ttf = 0);
	~FT_Render();

	bool loadFont(const char* ttf);
	void releaseFont();

	bool setSize(unsigned short pixels);

	void setMonochrome(bool monochrome) { monochrome_ = monochrome; }
	bool monochrome() const { return monochrome_; }

	void setHinting(bool native, bool freetype) { native_hinting_ = native; auto_hinting_ = freetype; }
	bool nativeHinting() const { return native_hinting_; }
	bool autoHinting() const { return auto_hinting_; }

	const FT_Face& face() const { return face_; }

	unsigned short lineHeight() const;

	FT_GlyphSlot getGlyph(unsigned long ch);

	bool draw(FT_GlyphSlot slot, unsigned char* buffer, unsigned short pitch);
	bool draw(unsigned long ch, unsigned char* buffer, unsigned short pitch);

private:
	FT_Library library_;
	FT_Face face_;
	
	bool monochrome_;
	bool native_hinting_;
	bool auto_hinting_;
};

}

#define _LIB_NAME "FreeType"
#include "AutomaticLink.h"

#endif //__FT_VISTA_RENDER_