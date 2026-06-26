#ifndef __FT_FONT_H_
#define __FT_FONT_H_

#include <vector>

class cTexture;

namespace FT {

typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef unsigned char uint8;
typedef signed char int8;
	
// --------------------------------------------------------------------------

struct OneChar
{
	OneChar()
		: u(0), v(0)
		, du(0), dv(0) 
		, su(0), sv(0)
		, lh(0), rh(0)
		, advance(0) {}
		uint16 u; //left
		uint16 v; //top
		uint8 du; // width
		uint8 dv; // height
		int8 su; // �������� �� ������ �������� ����
		int8 sv;
		int8 lh; //left hinding
		int8 rh; //right hinding
		uint8 advance;
};

// --------------------------------------------------------------------------

struct FontParam
{
	enum HintMode {
		DEFAULT, //���� ����, �� ttf native hinting, ���� ����, �� FreeType auto hinting
		BYTE_CODE_ONLY, //ttf native hinting
		AUTO_HINT_ONLY, //FreeType auto hinting
		NO_HINTING
	};

	FontParam() : nonPow2(false), inBox(false), antialiasing(false), hinting(DEFAULT) {}
	
	bool nonPow2; // �� ������ �������� ����� ����������� �� ��������� �������
	bool inBox; // ���������� ��������� ������
	bool antialiasing; // 256 �������� ������, ����� 1 ������
	HintMode hinting;
};

// --------------------------------------------------------------------------

class Font
{
	Font();
	friend class FontManager;
public:

	// �������� ������ ������ � ��������
	uint16 size() const { return size_; }
	// ������������ ���������� ����� ������� � ������ ������ ������
	uint16 lineHeight() const { return lineHeight_; }

	const FontParam& param() const { return param_; }

	const cTexture* texture() const { return texture_; }
	const OneChar& getChar(uint16 utf16) const { return charTable_[charMap_[utf16]]; }

	int textWidth(const wchar_t* str) const;
	int textHeight(const wchar_t* str) const;
	int lineWidth(const wchar_t *str) const;

private:
	cTexture* texture_;
	const uint16* charMap_;

	uint16 size_;
	uint16 lineHeight_;

	FontParam param_;

	typedef std::vector<OneChar> CharTable;
	CharTable charTable_;
};

// --------------------------------------------------------------------------

class FontManager
{
	FontManager();
	friend FontManager& fontManager();
public:
	~FontManager();

	class Font* createFont(const char* ttf, uint8 size, const FontParam* prm = 0);
	void releaseFont(class Font*& font);

	const uint16* charMap() const { return index_; }

	static const FontParam defParam;
private:
	struct ShortSize{
		uint16 x, y;
	};

	class FT_Render* render_;
	
	// ������� �������� utf16 ���� ������� � ������ (Font::charTable_) ���������� ������� ������������� � ��������.
	// ���, ��� ���� ��� ������������ ������� � ��������, ��������� �� ������ � ������� ��������
	uint16 index_[0xFFFF + 1];

	typedef std::vector<uint16> Chars;
	// ������������ �������. ������ ��� ������� ���������� � �������� ��� �������� ������ � ������� � Font::charTable_
	Chars chars_;

	ShortSize calcTextureSize(bool nonPow2);

	void addChar(uint16 charCode);
	void addCharPage(uint16 page, bool all_span);
};

// Namespace-scope declaration: a friend declaration alone is only found via ADL
// (clang follows the standard; MSVC's friend-injection made it visible directly).
FontManager& fontManager();

}
#endif //__FT_FONT_H_
