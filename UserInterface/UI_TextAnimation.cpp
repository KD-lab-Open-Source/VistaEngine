#include "stdafx.h"
#include "UI_TextAnimation.h"
#include "UnicodeConverter.h"
#include "Serialization\Serialization.h"
#include "Serialization\RangedWrapper.h"

UI_TextAnimation::UI_TextAnimation(const FT::Font* fnt)
: parser_(fnt)
{
	time_ = 0.f;
	blinkDuringOut_ = false;;
	blinkPeriodDuringOut_ = 0.2f;
	blinkAfterOut_ = true;
	blinkPeriodAfterOut_ = 0.2f;
}

void UI_TextAnimation::preLoad()
{
	cursor_.addRef();
}

void UI_TextAnimation::release()
{
	cursor_.decRef();
	cursor_.release();
}

void UI_TextAnimation::serialize(Archive& ar)
{
	ar.serialize(cursor_, "cursor", "Курсор в конце");
	ar.serialize(blinkDuringOut_, "blinkDuringOut", "Мерцать при выводе");
	ar.serialize(RangedWrapperf(blinkPeriodDuringOut_, 0.05f, 2.f), "blinkPeriodDuringOut", "Период мерцания при выводе");
	ar.serialize(blinkAfterOut_, "blinkAfterOut", "Мерцать после вывода");
	ar.serialize(RangedWrapperf(blinkPeriodAfterOut_, 0.05f, 2.f), "blinkPeriodAfterOut", "Период мерцания после вывода");
}

void UI_TextAnimation::start()
{
	time_ = 0.f;
}

void UI_TextAnimation::quant(const UI_TextParser& data, float dt)
{
	time_ += dt;
	data.getCopy(parser_);
	update();
}

void UI_TextAnimation::update()
{
	if(parser_.outNodes_.empty())
		return;

	OutNodes::iterator it = parser_.outNodes_.begin();
	nextTime(it);

	if(it == parser_.outNodes_.end())
		return;

	float partTime = it->time;
	float time = 0.f;
	++it;
	while(it != parser_.outNodes_.end()){

		if(time + partTime <= time_){
			time += partTime;
			nextTime(it);
			if(it == parser_.outNodes_.end()){
				if(blinkAfterOut_ && round(time_ / blinkPeriodAfterOut_) % 2){
					if(addBlink())
						reparseSize();
				}
				break;
			}
			partTime = max(0.f, it->time);
			++it;
			continue;
		}

		OutNodes::iterator it2 = it;
		int charCount = nextTime(it2);
		
		float phase = partTime > 0.f ? (time_ - time) / partTime : 1.f;
		int skipCount = round(phase * charCount);

		const wchar_t* end = 0;

		for(; it != parser_.outNodes_.end(); ++it){
			if(it->type == OutNode::SPRITE){
				if(!skipCount || !--skipCount)
					break;
			}
			else if(it->type == OutNode::TEXT){
				if(it->end - it->begin < skipCount)
					skipCount -= it->end - it->begin;
				else {
					end = it->begin + skipCount;
					break;
				}
			}
		}

		if(it == parser_.outNodes_.end())
			break;

		if(end && end != it->end){
			OutNode new_end = *it;
			new_end.end = end;

			UI_TextParser parser(parser_.font());
			parser.parseString(wstring(new_end.begin, new_end.end).c_str(), Color4c::WHITE, -1);
			new_end.width = parser.size().x;

			parser_.outNodes_.erase(it, parser_.outNodes_.end());
			parser_.outNodes_.push_back(new_end);
		}
		else
			parser_.outNodes_.erase(it+1, parser_.outNodes_.end());

		if(!blinkDuringOut_ || round(time_ / blinkPeriodDuringOut_) % 2)
			addBlink();

		reparseSize();

		break;
	}
}

void UI_TextAnimation::reparseSize()
{

	parser_.lineCount_ = 0;
	parser_.size_.x = 0;

	int width = 0;
	OutNodes::iterator it = parser_.outNodes_.end();
	while(it != parser_.outNodes_.begin()){
		--it;
		switch(it->type){
		case OutNode::NEW_LINE:
			if(parser_.lineCount_ == 0)
				it->width = width;
			parser_.size_.x = max(it->width, parser_.size_.x);
			++parser_.lineCount_;
			break;
		case OutNode::TEXT:
		case OutNode::SPRITE:
			width += it->width;
			break;
		}
	}

	parser_.size_.y = parser_.font_->lineHeight() * parser_.lineCount_;
}

bool UI_TextAnimation::addBlink()
{
	if(!cursor_.isEmpty()){
		OutNode node;
		node.type = OutNode::SPRITE;
		node.sprite = &cursor_;
		node.style = 2;
		Vect2f size(cursor_.size());
		node.width = round(size.x / size.y * (float)parser_.font_->size());
		parser_.outNodes_.push_back(node);
		return true;
	}
	return false;
}

int UI_TextAnimation::nextTime(OutNodes::iterator& it) const
{
	xassert(it != parser_.outNodes_.end());
	int char_count = 0;
	
	for(;it != parser_.outNodes_.end(); ++it){
		switch(it->type){
		case OutNode::SPRITE:
			++char_count;
			break;
		case OutNode::TEXT:
			char_count += it->end - it->begin;
			break;
		case OutNode::TIME:
			return char_count;
		}
	}
	
	return char_count;
}