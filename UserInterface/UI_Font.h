#ifndef __UI_FONT_
#define __UI_FONT_

#include "Render\src\FT_Font.h"
#include "Serialization\StringTableBase.h"
#include "Serialization\StringTableReference.h"

class Archive;

class UI_Font
{
public:
	UI_Font();
	UI_Font(const char* file, int size);

	FT::Font* font() const { return font_; }

	void serialize(Archive& ar);

	bool createFont();
	void releaseFont();

	const char* file() const { return fontFile_.c_str(); }
	int size() const { return font_ ? font_->size() : fontSize_; }

private:
	FT::FontParam::HintMode hinting_;
	string fontFile_;
	int fontSize_;
	int aaMin_;
	int aaMax_;
	FT::Font* font_;

#ifndef _FINAL_VERSION_
	bool inBox;
#endif
};

class UI_LibFont : public UI_Font, public StringTableBase {
public:
	UI_LibFont(const char* name = "") : StringTableBase(name) {}
	void serialize(Archive& ar);
};

typedef StringTable<UI_LibFont> UI_FontLibrary;
typedef StringTableReference<UI_LibFont, false> UI_FontReference;

#endif //__UI_FONT_