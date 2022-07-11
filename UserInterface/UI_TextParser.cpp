#include "StdAfx.h"

#include "UI_TextParser.h"
#include "UI_Types.h"
#include "UnicodeConverter.h"

UI_TextParser::UI_TextParser(const FT::Font* font)
{
	outNodes_.reserve(8);
	setFont(font);
}

void UI_TextParser::operator= (const UI_TextParser& src)
{
	outNodes_.reserve(8);
	setFont(src.font_);
}

void UI_TextParser::init()
{
	tagWidth_ = 0;
	lineWidth_ = 0;

	lineBegin_ = 0;
	pstr_ = 0;

	fitIn_ = -1;
	lastSpace_ = 0;
	lastTagWidth_ = 0;
	prev_rh_ = 0;

	outNodes_.clear();
	outNodes_.push_back(OutNode());
	prevLineIndex_ = outNodes_.size() - 1;

	lineCount_ = 1;
	if(font_)
		size_.set(0, font_->lineHeight());
	else
		size_.set(0, 0);
}

void UI_TextParser::getCopy(UI_TextParser& copy) const
{
	copy.init();

	copy.font_ = font_;

	copy.outNodes_ = outNodes_;
	copy.prevLineIndex_ = prevLineIndex_;
	copy.lineCount_ = lineCount_;

	copy.fitIn_ = fitIn_;
	copy.size_ = size_;
}

void UI_TextParser::setFont(const FT::Font* font)
{
	font_ = font;
	init();
}

OutNodes::const_iterator UI_TextParser::getLineBegin(int lineNum) const
{
	dassert(lineNum >= 0);

	if(!lineNum)
		return outNodes_.begin();

	if(lineNum >= lineCount_)
		return outNodes_.end();

	OutNodes::const_iterator it;
	FOR_EACH(outNodes_, it)
		if(it->type == OutNode::NEW_LINE)
			if(lineNum-- == 0)
				return it;

	xassert(lineNum == 0); 
	return outNodes_.end();
}

bool UI_TextParser::testWidth(int width)
{
	if(fitIn_ < 0)
		return true;

	if(lineWidth_ + tagWidth_ + width > fitIn_){
		if(lastSpace_ != lineBegin_){
			outNodes_.push_back(OutNode(lineBegin_, lastSpace_, lastTagWidth_));

			lineWidth_ += lastTagWidth_;
			endLine();

			lineBegin_ = lastSpace_ + 1;
			lastSpace_ = lineBegin_;
			tagWidth_ -= lastTagWidth_;
			lastTagWidth_ = 0;
		}
		else if(lineWidth_ > 0){
			dassert(lastTagWidth_ == 0);
			endLine();
			testWidth(width);
		}
		else if(tagWidth_ > 0){
			putText();
			endLine();
			skipNode();
		}
		return false;
	}
	return true;
}

void UI_TextParser::parseString(const wchar_t* text, const Color4c& color, int fitIn)
{
	xassert(font_);
	init();

	fitIn_ = fitIn > 2 * font_->size() ? fitIn : -1;

	pstr_ = text;

	lineBegin_ = text;
	lastSpace_ = lineBegin_;

	while(wchar_t cc = *pstr_){
		if(cc == L'\n'){
			putText();
			++pstr_;

			endLine();
			skipNode();

			continue;
		}

		if(cc < 32){
			++pstr_;
			continue;
		}

		if(cc == L' '){
			lastTagWidth_ = tagWidth_;
			lastSpace_ = pstr_;
		}

		if(cc == L'&'){
			if(pstr_[1] != L'&'){
				putText();
				++pstr_;
				getColor(color);
				continue;
			}
			else{
				addChar(L'&');
				putText();
				++pstr_;
				skipNode();
				continue;
			}
		}
		else if(cc == L'<'){
			if(pstr_[1] != L'<'){
				putText();
				++pstr_;
				lineWidth_ += getToken();
				continue;
			}
			else{
				addChar(L'<');
				putText();
				++pstr_;
				skipNode();
				continue;
			}
		}
		
		addChar(cc);
	}
	
	putText();
	size_.x = max(size_.x, lineWidth_);
	outNodes_[prevLineIndex_].width = lineWidth_;

	size_.y = font_->lineHeight() * lineCount_;
}

int UI_TextParser::getToken()
{
	const wchar_t* ptr = pstr_;
	const wchar_t* begin_tag = ptr;
	const wchar_t* begin_style = 0;

	char cc;
	while((cc = *pstr_) && cc != L'=' && cc != L'>')
		++pstr_;
	unsigned int tag_len = pstr_ - begin_tag;

	if(cc != L'>'){
		while((cc = *pstr_) && cc != L';' && cc != L'>')
			++pstr_;
		if(cc == ';'){
			begin_style = pstr_;
			while((cc = *pstr_) && cc != L'>')
				++pstr_;
		}
	}

	if(!cc){
		skipNode();
		return 0;
	}

	switch(tag_len){
	case 3:
		if(!wcsncmp(begin_tag, L"img=", 4)){
			string name;
			w2a(name, begin_tag + 4, (begin_style ? begin_style : pstr_) - begin_tag - 4, 1251);
			if(const UI_Sprite* sprite = UI_SpriteReference(name.c_str()))
				if(!sprite->isEmpty()){
					OutNode node;
					node.type = OutNode::SPRITE;
					node.sprite = sprite;
					node.style = getStyle(begin_style, pstr_);
					if((node.style & 0x03) != 2)
						node.width = sprite->size().xi();
					else{
						Vect2f size = sprite->size();
						node.width = round(size.x / size.y * (float)font_->size());
					}
					++pstr_;
					testWidth(node.width);
					putNode(node);
					return node.width;
				}
		}
		break;
	case 4:
		if(!wcsncmp(begin_tag, L"time=", 5)){
			float time = _wtof(wstring(begin_tag + 5, (begin_style ? begin_style : pstr_) - begin_tag - 5).c_str());
			if(time > 0.){
				OutNode node;
				node.type = OutNode::TIME;
				node.time = time;
				node.style = getStyle(begin_style, pstr_);
				++pstr_;
				putNode(node);
				return node.width;
			}
		}
		break;
	case 5:
		if(!wcsncmp(begin_tag, L"alpha=", 6)){
			int alpha = _wtoi(wstring(begin_tag + 6, (begin_style ? begin_style : pstr_) - begin_tag - 6).c_str());
			OutNode node;
			node.type = OutNode::ALPHA;
			node.style = clamp(alpha, 0, 255);
			getStyle(begin_style, pstr_);
			++pstr_;
			putNode(node);
			return node.width;
		}
		if(!wcsncmp(begin_tag, L"space=", 6)){
			int space = _wtoi(wstring(begin_tag + 6, (begin_style ? begin_style : pstr_) - begin_tag - 6).c_str());
			if(space > 0){
				OutNode node;
				node.type = OutNode::SPACE;
				node.width = space * font_->getChar(L' ').advance;
				node.style = getStyle(begin_style, pstr_);
				++pstr_;
				testWidth(node.width);
				putNode(node);
				return node.width;
			}
		}
		break;
	}

	++pstr_;
	skipNode();
	return 0;
}

int UI_TextParser::getStyle(const wchar_t* styleptr, const wchar_t* end)
{
	if(!styleptr || *end != L'>')
		return 0;

	wchar_t cc;
	while((cc = *(++styleptr)) && cc != L'=' && cc != L'>');

	if(cc != L'=')
		return 0;

	int style = 0;
	while((cc = *(++styleptr)) >= L'0' && cc <= L'9')
		style = style * 10 + (int)(cc - L'0');

	return style;
}

void UI_TextParser::getColor(const Color4c& defColor)
{
	Color4c diffuse = defColor;

	if(*pstr_ != L'>'){
		DWORD s = 0;
		int i = 0;
		for(; i < 6; ++i, ++pstr_)
			if(char k = *pstr_){
				int a = fromHex(k);
				if(a < 0)
					break;
				s = (s << 4) + a;
			}
			else
				break;

		if(i > 5){
			diffuse.RGBA() &= 0xFF000000;
			diffuse.RGBA() |= s;
		}
		else {
			skipNode();
			return;
		}
	}
	else
		++pstr_;

	putNode(OutNode(diffuse));
}


void UI_TextParserAnsiWrapper::parseString(const char* text, const Color4c& color, int fitIn)
{
	a2w(wideBuf_, text);
	UI_TextParser::parseString(wideBuf_.c_str(), color, fitIn);
}
