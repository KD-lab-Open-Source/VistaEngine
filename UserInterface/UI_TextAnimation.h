#ifndef __UI_TEXT_ANIMATION_H__
#define __UI_TEXT_ANIMATION_H__

#include "UI_TextParser.h"
#include "UI_Sprite.h"

class Archive;

namespace FT { class Font; }

class UI_TextAnimation
{
public:
	UI_TextAnimation(const FT::Font* fnt = 0);

	void preLoad();
	void release();

	void serialize(Archive& ar);

	void start();
	void quant(const UI_TextParser& data, float dt);

	const UI_TextParser& parser() const { return parser_; }

private:
	float time_;
	UI_TextParser parser_;

	UI_Sprite cursor_;
	bool blinkDuringOut_;
	float blinkPeriodDuringOut_;
	bool blinkAfterOut_;
	float blinkPeriodAfterOut_;

	void update();
	void reparseSize();
	bool addBlink();
	int nextTime(OutNodes::iterator& it) const;
};

#endif
