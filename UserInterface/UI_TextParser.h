#ifndef __UI_TEXT_PARSER_H__
#define __UI_TEXT_PARSER_H__

#include "XMath/Colors.h"
#include "Render/src/FT_Font.h"

class UI_Sprite;

struct OutNode{
	enum{
		NEW_LINE,
		TEXT,
		SPRITE,
		COLOR,
		ALPHA,
		TIME,
		SPACE
	} type;
	int width;
	union{
		struct {
			const wchar_t* begin;
			const wchar_t* end;
		};
		struct {
			const UI_Sprite* sprite;
			int style;
		};
		DWORD color;
		struct {
			float time;
			int style;
		};
	};
	OutNode() : type(NEW_LINE), width(0) {}
	OutNode(const wchar_t* b, const wchar_t* e, int wd) : type(TEXT), width(wd), begin(b), end(e) {}
	OutNode(const Color4c& clr) : type(COLOR), width(0), color(clr.RGBA()) {}
};

typedef vector<OutNode> OutNodes;


class UI_TextParser
{
	friend class UI_TextAnimation;
public:
	UI_TextParser(const FT::Font* font = 0);
	
	void operator= (const UI_TextParser& src);
	
	void setFont(const FT::Font* font);
	const FT::Font* font() const { return font_; }

	void parseString(const wchar_t* text, const Color4c& color = Color4c(0, 0, 0), int fitIn = -1);

	const OutNodes& outNodes() const { return outNodes_; }
	
	const Vect2i& size() const { return size_; }

	int lineCount() const { return lineCount_; }
	OutNodes::const_iterator getLineBegin(int lineNum) const;

	void getCopy(UI_TextParser& copy) const;

private:
	UI_TextParser(const UI_TextParser& src);

	void init();

	__forceinline int fromHex(char a)
	{
		if(a >= L'0' && a <= '9')
			return a-'0';
		if(a >= L'A' && a <= 'F')
			return a-'A'+10;
		if(a >= L'a' && a <= L'f')
			return a - L'a' + 10;
		return -1;
	}

	__forceinline void addChar(wchar_t cc)
	{
		const FT::OneChar& ch = font_->getChar(cc);
		int advance = ch.advance;
		if(prev_rh_ - ch.lh >= 32)
			--advance;
		else if(prev_rh_ - ch.lh < -32)
			++advance;
		prev_rh_ = ch.rh;
		bool inLine = testWidth(advance);
		if(cc != L' ' || inLine)
			tagWidth_ += advance;
		++pstr_;
	}

	__forceinline void skipNode()
	{
		lineBegin_ = pstr_;
		lastSpace_ = lineBegin_;
		lastTagWidth_ = 0;
		tagWidth_ = 0;
	}

	__forceinline void putNode(OutNode& node)
	{
		outNodes_.push_back(node);
		skipNode();
	}

	void putText()
	{
		prev_rh_ = 0;
		if(pstr_ == lineBegin_)
			return;
		lineWidth_ += tagWidth_;
		putNode(OutNode(lineBegin_, pstr_, tagWidth_));
	}

	void endLine()
	{
		size_.x = max(size_.x, lineWidth_);

		outNodes_[prevLineIndex_].width = lineWidth_;
		lineWidth_ = 0;
		prev_rh_ = 0;

		outNodes_.push_back(OutNode());
		prevLineIndex_ = outNodes_.size() - 1;

		++lineCount_;
	}

	void getColor(const Color4c& defColor);
	int getStyle(const wchar_t* styleptr, const wchar_t* end);
	int getToken();
	bool testWidth(int width);

	OutNodes outNodes_;

	int prevLineIndex_;
	int prev_rh_;
	const wchar_t* lastSpace_;
	int lastTagWidth_;

	const wchar_t* lineBegin_;
	const wchar_t* pstr_;
	int tagWidth_;
	int lineWidth_;

	int fitIn_;

	const FT::Font* font_;
	Vect2i size_;
	int lineCount_;
};


class UI_TextParserAnsiWrapper : public UI_TextParser
{
public:
	UI_TextParserAnsiWrapper(const FT::Font* font = 0) : UI_TextParser(font) {}
	void parseString(const char* text, const Color4c& color = Color4c(0, 0, 0), int fitIn = -1);

private:
	wstring wideBuf_;
};

#endif //__UI_TEXT_PARSER_H__
