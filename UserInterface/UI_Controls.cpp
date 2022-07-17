#include "StdAfx.h"

#include "SafeCast.h"
#include "Serialization.h"
#include "RangedWrapper.h"
#include "ResourceSelector.h"
#include "Console.h"

#include "UI_Render.h"
#include "UI_Controls.h"
#include "UserInterface.h"

#include "UI_References.h"
#include "UI_Logic.h"
#include "Controls.h"

#include "StreamCommand.h"

#include "UI_StreamVideo.h"
extern Singleton<UI_StreamVideo> streamVideo;

#pragma warning(disable: 4355)

LogStream outLog;

void logUIState(int line, const char* func, const UI_ControlBase* control, const char* str)
{
#define OB(var) #var < ((control->var)?"=true ":"=false ")
	if(debugShowEnabled && showDebugInterface.writeLog && control && control->owner()){
		if(UI_Dispatcher::instance().debugControl_ 
			? UI_Dispatcher::instance().debugControl_ != control
			: (showDebugInterface.writeLog & 0x8) == 0)
			return;
		
		XBuffer out;
		out <= xclock();

		out < " LQ" < (MT_IS_LOGIC() ? "(!):" : ":") <= UI_Dispatcher::instance().debugLogQuant_;
		out < " GQ" < (MT_IS_GRAPH() ? "(!):" : ":") <= UI_Dispatcher::instance().debugGraphQuant_;

		//UI_AUTOLOCK();
		out.SetDigits(2);

		if(!outLog.isOpen() && showDebugInterface.writeLog & 0x02)
			outLog.open("ui_log.log", XS_OUT);

		string name;
		control->referenceString(name);
		out < " " < name.c_str();

		string fn = func;
		string::size_type idx =	fn.find("::");
		fn.erase(0, fn.find("::")+2);
		fn.erase(fn.find("("));
		out < "::" < fn.c_str() < ", line " <= line;

		if(str && *str)
			out < " : " < str;

		out < "\n ";

		const UI_Screen* scr = control->screen();
		if(scr->isDeactivating())
			out < " Screen DEACTIVATING;";
		if(scr->isActivating())
			out < " Screen ACTIVATING;";
		if(!scr->isActive())
			out < " Screen NOT active";

		out < " " < OB(isVisible_) < OB(isVisibleByTrigger_) < OB(isEnabled_) < OB(redrawLock_) < OB(waitingUpdate_)
			< "\n  " < OB(actionActive_) < OB(textChanged_) < OB(isActive_) < OB(needDeactivation_) < OB(isScaled_) < OB(isDeactivating_)
			< OB(mouseHover_) < "flags=" <= control->actionFlags_ < " state=" <= control->currentStateIndex_;

		out < "\n  Text:" < control->text_.c_str();
		if(control->textChanged_)
			out < "\n  NewText:" < control->newText_.c_str();

		out < "\n  ";
		control->getDebugString(out);

		if(showDebugInterface.writeLog & 0x01)
			dprintf("%s\n", out.c_str());
		if(showDebugInterface.writeLog & 0x02)
			outLog < out.c_str() < "\n";
		if(showDebugInterface.writeLog & 0x04)
			Console::instance().write(Console::LEVEL_INFO, "UI", out.c_str(), Console::Location(func, "UI_Controls", line));
	}
#undef OB
}

// ------------------- UI_ControlButton

UI_ControlButton::UI_ControlButton() 
{
	data_ = -1.f;

	positionOriginal_ = position_;

	autoResize_ = false;
	resizeShiftHorizontal_ = RESIZE_SHIFT_LEFT;
	resizeShiftVertical_ = RESIZE_SHIFT_UP;
}

void UI_ControlButton::serialize(Archive& ar)
{
	__super::serialize(ar);
	
	ar.serialize(autoResize_, "autoResize", "подгонять размер по тексту");
	if(autoResize_){
		ar.serialize(resizeShiftHorizontal_, "resizeShiftHorizontal", "сдвиг по горизонтали");
		ar.serialize(resizeShiftVertical_, "resizeShiftVertical", "сдвиг по вертикали");
	}

	if(ar.isInput())
		positionOriginal_ = position_;

	postSerialize();
}

bool UI_ControlButton::adjustSize()
{
	if(!autoResize_ || isUnderEditor())
		return false;

	Vect2f text_sz = UI_Render::instance().textSize(text(), font(), 
		autoFormatText() ? positionOriginal_.width() * relativeTextPosition().width() : 0.f);

	text_sz.x *= 1.f + (1.f - relativeTextPosition().width()); 
	text_sz.y *= 1.f + (1.f - relativeTextPosition().height()); 

	if(text_sz.x > positionOriginal_.width())
		text_sz.x = positionOriginal_.width();
	if(text_sz.y > positionOriginal_.height())
		text_sz.y = positionOriginal_.height();

	switch(resizeShiftHorizontal_){
	case RESIZE_SHIFT_LEFT:
		position_.left(positionOriginal_.left());
		break;
	case RESIZE_SHIFT_CENTER_H:
		position_.left(positionOriginal_.left() + (positionOriginal_.width() - text_sz.x)/2);
		break;
	case RESIZE_SHIFT_RIGHT:
		position_.left(positionOriginal_.right() - text_sz.x);
		break;
	}

	switch(resizeShiftVertical_){
	case RESIZE_SHIFT_UP:
		position_.top(positionOriginal_.top());
		break;
	case RESIZE_SHIFT_CENTER_V:
		position_.top(positionOriginal_.top() + (positionOriginal_.height() - text_sz.y)/2);
		break;
	case RESIZE_SHIFT_DOWN:
		position_.top(positionOriginal_.bottom() - text_sz.y);
		break;
	}

	position_.size(text_sz);

	return true;
}

// ------------------- UI_ControlTextList

UI_ControlTextList::UI_ControlTextList()
: parser(cfont())
{
	autoHideScroll_ = true;
	textChanged_ = false;
	scrollType_ = ORDINARY;
	scrollSpeed_ = 10.f;
	firstVisibleLine_ = 0;
	viewSize_ = 1.f;
	scrollShift_ = 0.f;
	scrolledLines_ = 0;
	stringHeight_ = 20.f / 768.f;
	stringCount_ = 0;
	setDrawMode(UI_DRAW_SPRITE | UI_DRAW_BORDER);
}

void UI_ControlTextList::serialize(Archive& ar)
{
	__super::serialize(ar);
	
	 /// CONVERSION 2006-11-29
	if(!ar.serialize(scrollType_, "scrollType", "Тип скроллинга")){
		bool enableAutoScroll = false;
		ar.serialize(enableAutoScroll, "enableAutoScroll", 0);
		if(enableAutoScroll)
			scrollType_ = AUTO_SMOOTH;
	}
	/// ^^^
	
	if(scrollType_ == AUTO_SMOOTH)
		ar.serialize(RangedWrapperf(scrollSpeed_, 0.1f, 100.f), "scrollSpeed", "Скорость автоскролла (пикс/сек)");
	else
		ar.serialize(autoHideScroll_, "autoHideScroll", "Показывать скроллер только при необходимости");
	
	postSerialize();
}

void UI_ControlTextList::postSerialize()
{
	__super::postSerialize();
	reparse();
}

void UI_ControlTextList::init()
{
	__super::init();
	reparse();
	scrollShift_ = 0;
	scrolledLines_ = 0;
}

void UI_ControlTextList::textChanged()
{
	reparse();
}

void UI_ControlTextList::reparse()
{
	screenSize_ = UI_Render::instance().windowPosition().size();
	int lastSize_ = parser.lineCount();
	parser.setFont(*cfont());
	parser.parseString(text(), textFormat()->textColor(), UI_Render::instance().screenSize(textPosition().size()).x);
	textChanged_ = (textChanged_ || lastSize_ != parser.lineCount());
}

void UI_ControlTextList::quant(float dt)
{
	__super::quant(dt);

	if(UI_Render::instance().windowPosition().size() != screenSize_)
		reparse();

	stringCount_ = parser.lineCount();
	stringHeight_ = float(parser.fontHeight()) / UI_Render::instance().windowPosition().height();
	viewSize_ = floor(textPosition().height() / stringHeight_);

	UI_ControlSlider* cur = slider();

	if(scrollType_ == AUTO_SMOOTH){
		scrollShift_ += dt * scrollSpeed_ / UI_Render::instance().windowPosition().height();
		int lines = scrollShift_ / stringHeight_;
		scrolledLines_ += lines;
		scrollShift_ -= lines * stringHeight_;

		if(scrolledLines_ >= viewSize_ + stringCount_)
			scrolledLines_ = 0;

		if(cur)
			cur->hide(true);
	}
	else if(cur){
		int size = scrollSize();

		switch(scrollType_){
		case ORDINARY:
			if(!*text())
				cur->setValue(0.f);
			break;
		
		case AT_END_IF_SIZE_CHANGE:
			if(textChanged_ && !cur->dragMode()){
				cur->setValue(1.f);
				textChanged_ = false;
			}
			break;

		case AUTO_SET_AT_END:
			if(!cur->dragMode())
				cur->setValue(1.f);
			break;
		}

		cur->setDiscretStep(size > 0 ? 1.f / size : 0.5f);
		firstVisibleLine_ = min(sliderPhase2Index(cur->value()), size);

		if(size <= 0){
			if(autoHideScroll_)
				cur->hide(true);
		}
		else{
			if(autoHideScroll_)
				cur->show();
			if(UI_ControlBase* arrowUp = cur->arrowDec())
				arrowUp->startAnimation(firstVisibleLine_ > 0 ? PLAY_ACTION_PLAY : PLAY_ACTION_STOP, false);
			if(UI_ControlBase* arrowDown = cur->arrowInc())
				arrowDown->startAnimation(size > 0 && firstVisibleLine_ + 1 < stringCount_ ? PLAY_ACTION_PLAY : PLAY_ACTION_STOP, false);
		}
	}
}

bool UI_ControlTextList::redraw() const
{
	if(!UI_ControlBase::redraw())
		return false;

	if(isScaled())
		return true;

	Rectf text_pos = textPosition();

	int firstLine = max(0, firstVisibleLine_);

	if(scrollType_ == AUTO_SMOOTH){
		float shift = -scrollShift_;
		if(scrolledLines_ < viewSize_){
			firstLine = 0;
			shift += (viewSize_ - scrolledLines_) * stringHeight_;
		}
		else
			firstLine = scrolledLines_ - viewSize_;
		
		text_pos.top(text_pos.top() + shift);
		text_pos.height(text_pos.height() - shift + stringHeight_);
	}

	int maxLines = text_pos.height() / stringHeight_;

	OutNodes::const_iterator begin = parser.getLineBegin(firstLine);
	OutNodes::const_iterator end = parser.getLineBegin(min(stringCount_, maxLines + firstLine));

	if(begin != end){
		const UI_Font* fnt = font();
		if(!fnt)
			fnt = UI_Render::instance().defaultFont();
		UI_Render::instance().outText(text_pos, parser, begin, end, textFormat(), textAlign(), fnt, alpha());
	}

	return true;
}

float UI_ControlTextList::index2sliderPhase(int index) const
{
	if(!slider())
		return 0;
	return slider()->index2sliderPhase(index, scrollSize());
}

int UI_ControlTextList::sliderPhase2Index(float value) const
{
	if(!slider())
		return 0;
	return slider()->sliderPhase2Index(value, scrollSize());
}

// ------------------- UI_ControlSlider

UI_ControlSlider::UI_ControlSlider()
{
	orientation_ = UI_SLIDER_HORIZONTAL;
	isDiscrete_ = false;

	value_ = 0.0f;
	valueDelta_ = 0.1f;
	step_ = 0.1f;
	dragMode_ = false;
}

UI_ControlSlider::~UI_ControlSlider()
{
}

void UI_ControlSlider::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(orientation_, "orientation_", "ориентация");

	ar.serialize(valueDelta_, "valueDelta_", "шаг изменения значения");
	ar.serialize(isDiscrete_, "isDiscrete_", "дискретные значения");
	
	postSerialize();
}

void UI_ControlSlider::postSerialize()
{
	__super::postSerialize();
	step_ = valueDelta_;
}

float UI_ControlSlider::index2sliderPhase(int index, int scrolling_size) const
{
	if(index >= scrolling_size)
		return 1.f;
	else if(scrolling_size > 0)
		return float(index) / float(scrolling_size);

	return 0.f;
}

int UI_ControlSlider::sliderPhase2Index(float value, int scrolling_size) const
{
	if(scrolling_size > 0)
		return round(value * float(scrolling_size));

	return 0;
}

void UI_ControlSlider::quant(float dt)
{

	float newValue = value_;
	UI_ControlBase* slider = (*this)[CONTROL_SLIDER];
	if(slider){
		if(dragMode_){
			switch(orientation()){
			case UI_SLIDER_HORIZONTAL:
				newValue = (UI_LogicDispatcher::instance().mousePosition().x - position().left()) / position().width();
				break;
			case UI_SLIDER_VERTICAL:
				newValue = (UI_LogicDispatcher::instance().mousePosition().y - position().top()) / position().height();
				break;
			}
		}
		else
			if(slider->checkInputActionFlag(UI_ACTION_MOUSE_LB))
				dragMode_ = true;

		if(!UI_LogicDispatcher::instance().isMouseFlagSet(MK_LBUTTON))
			dragMode_ = false;
	}

	if(UI_ControlBase* arrow = arrowDec())
		if(arrow->isActivated())
			newValue -= step_;
			
	if(UI_ControlBase* arrow = arrowInc())
		if(arrow->isActivated())
			newValue += step_;

	if(newValue < 0.0f) newValue = 0.0f;
	if(newValue > 1.0f) newValue = 1.0f;

	if(isDiscrete_)
		newValue = round(newValue / step_) * step_;

	if(abs(newValue - value_) > 0.01f){
		setInputActionFlag(UI_ACTION_SELF_EXECUTE);
		actionRequest();
	}

	value_ = newValue;

	if(slider){
		Vect2f pos = position().center();

		switch(orientation()){
		case UI_SLIDER_HORIZONTAL:
			pos.x = position().left() + slider->position().width()/2.0f + (position().width() - slider->position().width()) * value_;
			break;
		case UI_SLIDER_VERTICAL:
			pos.y = position().top() + slider->position().height()/2.0f + (position().height() - slider->position().height()) * value_;
			break;
		}

		pos.x -= slider->position().width()/2.0f;
		pos.y -= slider->position().height()/2.0f;

		slider->setPosition(pos);
	}

	__super::quant(dt);
}

// ------------------- UI_ControlHotKeyInput

UI_ControlHotKeyInput::UI_ControlHotKeyInput()
{
	waitingInput_ = false;
	compatible_ = false;
}

void UI_ControlHotKeyInput::serialize(Archive& ar)
{
	__super::serialize(ar);
	ar.serialize(compatible_, "compatible", "Может совпадать с хоткеями на кнопках");
	postSerialize();
}

bool UI_ControlHotKeyInput::activate()
{
	UI_ControlBase::activate();
	
	waitingInput_ = true;
	toggleDeactivation(false);

	saveKey_ = key_;
	clearInputEventFlags();

	UI_Dispatcher::instance().setFocus(this);
	return false;
}

void UI_ControlHotKeyInput::done(const sKey& button, bool force)
{
	if((force || (button.key && button.key != VK_SHIFT && button.key != VK_CONTROL && button.key != VK_MENU))
		&& (compatible_ || (ControlManager::instance().checkHotKey(button.fullkey) && !UI_Dispatcher::instance().isIngameHotKey(button))))
	{
		sKey last_key = key_;

		key_ = button;

		if(last_key != button){
			UI_LogicDispatcher::instance().setLastHotKeyInput(this);
			UI_LogicDispatcher::instance().handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_SET_KEYS, UI_ActionDataControlCommand::UPDATE_HOTKEY)));
			UI_LogicDispatcher::instance().handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_SET_KEYS, UI_ActionDataControlCommand::UPDATE_COMPATIBLE_HOTKEYS)));
		}

		waitingInput_ = false;
		toggleDeactivation(true);
		UI_Dispatcher::instance().setFocus(0);
	}
}

void UI_ControlHotKeyInput::onFocus(bool focus_state)
{
	if(!focus_state && waitingInput_){
		key_ = saveKey_;
		waitingInput_ = false;
		toggleDeactivation(true);
	}
}

// ------------------- UI_ControlEdit

UI_ControlEdit::UI_ControlEdit()
{
	isEditing_ = false;
	textLengthMax_ = 0;
	caretPose_ = 0;

	password_ = false;
	saveFocus_ = false;

	caretColor_.set(255, 255, 255, 200);
	caretVisible_ = true;
	caretTimer_ = 0.0f;

	charType_ = LATIN;

	setDrawMode(UI_DRAW_BORDER | UI_DRAW_SPRITE);
}

UI_ControlEdit::~UI_ControlEdit()
{
}

bool UI_ControlEdit::checkSymbol(unsigned char chr)
{
	if(chr < 32)
		return false;

	switch(charType_){
		case LATIN:
			return chr < 128;
		case ALNUM:
			return isalnum(chr) || chr == '_' || chr == '-';
		default:
			return true;
	}
	return false;
}

void UI_ControlEdit::serialize(Archive& ar)
{
	__super::serialize(ar);
	
	ar.serialize(next_, "next", "Следующий контрол");
	ar.serialize(password_, "password", "Ввод пароля");
	ar.serialize(saveFocus_, "saveFocus", "Оставлять фокус после редактирования");
	ar.serialize(textLengthMax_, "textLengthMax_", "максимальная длина строки");
	ar.serialize(charType_, "charType", "тип символов");
	
	postSerialize();
}

void UI_ControlEdit::postSerialize()
{
	__super::postSerialize();

	setEditText(text());

	caretPose_ = textEdit_.size();
}

bool UI_ControlEdit::activate()
{
	UI_ControlBase::activate();

	if(!isEditing_){
		isEditing_ = true;

		setEditText(text());

		caretPose_ = textEdit_.size();

		toggleDeactivation(false);

		caretVisible_ = true;
		caretTimer_ = 0.0f;

		UI_Dispatcher::instance().setFocus(this);
	}
	return false;
}

bool UI_ControlEdit::editDone(bool done)
{
	if(isEditing_){
		applyText(done ? textEdit_.c_str() : textBackup_.c_str());
		isEditing_ = false;
		toggleDeactivation(true);
		return true;
	}

	return false;
}

bool UI_ControlEdit::editInput(int vkey)
{
	if(isEditing_){
		switch(vkey){
		case VK_ESCAPE:
			if(!editDone(false))
				return false;
			UI_Dispatcher::instance().setFocus(0);
			return true;
		
		case VK_RETURN:
			if(!editDone(true))
				return false;
			setInputActionFlag(UI_ACTION_SELF_EXECUTE);
			action();
			if(saveFocus_)
				activate();
			else
				UI_Dispatcher::instance().setFocus(0);
			return true;

		case VK_TAB:
			if(UI_ControlBase* next = next_.control())
				if(next->screen()->isActive()){
					UI_Dispatcher::instance().setFocus(0);
					next->activate();
				}
			return true;

		case VK_LEFT:
			if(caretPose_ > 0)
				--caretPose_;
			return true;

		case VK_HOME:
			caretPose_ = 0;
			return true;

		case VK_END:
			caretPose_ = textEdit_.size();
			return true;

		case VK_RIGHT:
			if(caretPose_ < textEdit_.size())
				++caretPose_;
			return true;

		case VK_BACK:
			if(caretPose_ > 0 && caretPose_ <= textEdit_.size())
				textEdit_.erase(--caretPose_, 1);

			applyText(textEdit_);
			return true;

		case VK_DELETE:
			if(caretPose_ >= 0 && caretPose_ < textEdit_.size())
				textEdit_.erase(caretPose_, 1);

			applyText(textEdit_);
			return true;

		}

		return true;
	}

	return false;
}

bool UI_ControlEdit::editCharInput(int chr)
{
	
	if(isEditing_){
		if(checkSymbol((unsigned char)chr) && (!textLengthMax_ || textEdit_.length() < textLengthMax_))
			textEdit_.insert(caretPose_++, 1, chr);

		applyText(textEdit_);
		return true;
	}

	return false;
}

bool UI_ControlEdit::redraw() const
{
	if(!UI_ControlBase::redraw())
		return false;

	if(!isScaled()){
		string outString = text();
		string caretLeft;
		if(password_){
			outString.assign(outString.size(), '*');
			if(isEditing_)
				caretLeft = outString.substr(caretPose_, string::npos);
		}
		else {
			if(isEditing_)
				UI_Render::reparseWildChars(true, textEdit_.substr(caretPose_, string::npos).c_str(), caretLeft);
		}

		UI_TextParser parser(cfont());

		float shiftFronEnd = 0;
		if(isEditing_){
			parser.parseString(caretLeft.c_str(), BLACK);
			shiftFronEnd = UI_Render::instance().relativeSize(parser.size()).x;
		}

		parser.parseString(outString.c_str(), textFormat()->textColor());
		float fullSize = UI_Render::instance().relativeSize(parser.size()).x;

		if(isEditing_){
			Rectf pos = textPosition();
			if(pos.width() >= fullSize){ // влазит
				Vect2f end = UI_Render::instance().outText(pos, parser, parser.outNodes().begin(), parser.outNodes().end(), textFormat(), textAlign(), font(), alpha(), true);
				if(caretVisible_){
					end.x -= shiftFronEnd;
					UI_Render::instance().outText(Rectf(end, Vect2f::ZERO), "|", &UI_TextFormat(caretColor_), UI_TEXT_ALIGN_CENTER, font(), alpha());
				}
			}
			else if(pos.width() >= shiftFronEnd){
				int align = textAlign() & UI_TEXT_VALIGN | UI_TEXT_ALIGN_RIGHT;
				Vect2f end = UI_Render::instance().outText(pos, parser, parser.outNodes().begin(), parser.outNodes().end(), textFormat(), align, font(), alpha(), true);
				if(caretVisible_){
					end.x -= shiftFronEnd;
					UI_Render::instance().outText(Rectf(end, Vect2f::ZERO), "|", &UI_TextFormat(caretColor_), UI_TEXT_ALIGN_CENTER, font(), alpha());
				}
			}
			else if(pos.width() >= fullSize - shiftFronEnd){
				int align = textAlign() & UI_TEXT_VALIGN | UI_TEXT_ALIGN_LEFT;
				UI_Render::instance().outText(pos, parser, parser.outNodes().begin(), parser.outNodes().end(), textFormat(), align, font(), alpha(), true);
				if(caretVisible_){
					Vect2f begin = pos.left_top();
					begin.x += fullSize - shiftFronEnd;
					UI_Render::instance().outText(Rectf(begin, Vect2f::ZERO), "|", &UI_TextFormat(caretColor_), UI_TEXT_ALIGN_CENTER, font(), alpha());
				}
			}
			else {
				outString = outString.substr(0, caretPose_);
				int align = textAlign() & UI_TEXT_VALIGN | UI_TEXT_ALIGN_RIGHT;
				Vect2f end = UI_Render::instance().outText(pos, parser, parser.outNodes().begin(), parser.outNodes().end(), textFormat(), align, font(), alpha(), true);
				if(caretVisible_)
					UI_Render::instance().outText(Rectf(end, Vect2f::ZERO), "|", &UI_TextFormat(caretColor_), UI_TEXT_ALIGN_CENTER, font(), alpha());
			}
		}
		else
			UI_Render::instance().outText(textPosition(), parser, parser.outNodes().begin(), parser.outNodes().end(), textFormat(), textAlign(), font(), alpha(), true);
	}

	return true;
}

void UI_ControlEdit::onFocus(bool focus_state)
{
	if(!focus_state && isEditing_){
		editDone(true);
		setInputActionFlag(UI_ACTION_FOCUS_OFF);
		action();
	}
}

void UI_ControlEdit::applyText(const string& txt)
{
	string out;
	if(password_)
		out = txt;
	else
		UI_Render::reparseWildChars(true, txt.c_str(), out);
	setText(out.c_str());
}

void UI_ControlEdit::setEditText(const char* p) 
{
	if(password_)
		textBackup_ = p;
	else
		UI_Render::reparseWildChars(false, p, textBackup_);
    textEdit_ = textBackup_;
	caretPose_ = textEdit_.size();
}
// ------------------- UI_ControlTab

UI_ControlTab::UI_ControlTab()
{
	currentSheetIndex_ = -1;
}

UI_ControlTab::~UI_ControlTab()
{
}

void UI_ControlTab::serialize(Archive& ar)
{
	__super::serialize(ar);
	postSerialize();
}

void UI_ControlTab::init()
{
	UI_ControlBase::init();

	selectSheet(0);
}

void UI_ControlTab::showChildControls()
{
	int index = 0;
	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it, ++index){
		(*it)->show();
		if(index != currentSheetIndex_)
			(*it)->hideChildControls(true);
	}
}

void UI_ControlTab::quant(float dt)
{
	__super::quant(dt);

	if(!isVisible())
		return;

	int index = 0;
	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it, index++){
		if(index != currentSheetIndex_ && (*it)->isActivated()){
			selectSheet(index);
			return;
		}
	}
}

void UI_ControlTab::selectSheet(int index)
{
	int ind = 0;
	for(ControlList::iterator it = controls_.begin(); it != controls_.end(); ++it, ind++){
		UI_ControlBase* control = *it;
		if(ind != index){
			control->toggleDeactivation(true);
			control->hideChildControls(true);
		}
		else{
			control->activate();
			control->toggleDeactivation(false);
			control->showChildControls();
		}
	}
	
	currentSheetIndex_ = index;
}

// ------------------- UI_ControlStringList

UI_ControlStringList::UI_ControlStringList()
{
	firstVisibleString_ = 0;
	selectedString_ = -1;
	autoResizeList_ = false;
	listChanged_ = false;
	autoHideSlider_ = true;
	stringMax_ = -1;

	stringHeight_ = 20.0f / 768.0f;
	stringHeightFactor_ = 1.0f;
}

UI_ControlStringList::~UI_ControlStringList()
{
}

void UI_ControlStringList::Column::serialize(Archive& ar)
{
	ar.serialize(RangedWrapperi(width, 0, 100), "width", "Ширина (%)");
	ar.serialize(align, "align", "Выравнивание");
}

void UI_ControlStringList::serialize(Archive& ar)
{
	__super::serialize(ar);

	float stringHeightFactorOld = stringHeightFactor_;
	ar.serialize(RangedWrapperf(stringHeightFactor_, 0.5f, 5.f), "stringHeightFactor", "высота строки относительно размера шрифта");
	stringHeight_ = stringHeight_ / stringHeightFactorOld * stringHeightFactor_;

	ar.serialize(strings_, "strings_", "строки");
	ar.serialize(columns_, "columns", "Столбцы");

	ar.serialize(autoResizeList_, "autoResizeList", "подстройка размеров списка");
	if(autoResizeList_)
		ar.serialize(stringMax_, "stringMax", "Максимальное количество строк в списке");

	ar.serialize(autoHideSlider_, "autoHideSlider", "Показывать слайдер только при необходимости");

	UI_ControlStringList::postSerialize();
}

void UI_ControlStringList::init()
{
	__super::init();

	firstVisibleString_ = 0;
	selectedString_ = -1;

	adjustListSize();

	if(slider())
		slider()->setValue(index2sliderPhase(0));

	setDrawMode(UI_DRAW_SPRITE | UI_DRAW_BORDER);
}

void UI_ControlStringList::adjustListSize()
{
	if(autoResizeList_ && !isUnderEditor()){
		int size = strings_.size();
		float height = (stringMax_ > 0 && (size == 0 || size > stringMax_) ? stringMax_ : size) * stringHeight_;
		height *= 1.f + (1.f - relativeTextPosition().height());

		Rectf pos = transfPosition();
		pos.height(height + 0.001f);

		setPosition(pos);
	}
}

void UI_ControlStringList::toggleString(int idx)
{
	selectedString_ = idx;
}

void UI_ControlStringList::applyNewList(const ComboStrings& newList)
{
	strings_ = newList;
	adjustListSize();
}

void UI_ControlStringList::setList(const ComboStrings& strings)
{
	if(MT_IS_GRAPH()){
		applyNewList(strings);
	}
	else if(!listChanged_){ // второй раз за квант поменять нельзя, HT possible трабл
		newStrings_ = strings;
		listChanged_ = true;
	}
}

void UI_ControlStringList::flushNewStrings()
{
	if(listChanged_){
		applyNewList(newStrings_);
		listChanged_ = false;
	}
}

void UI_ControlStringList::quant(float dt)
{
	__super::quant(dt);

	stringHeight_ = cfont()->size() * stringHeightFactor_ / UI_Render::instance().windowPosition().height();
	flushNewStrings();

	if(UI_ControlSlider* cur = slider()){
		if(!listSize())
			cur->setValue(0.f);
		int size = scrollSize();
		cur->setDiscretStep(size > 0 ? 1.f / size : 0.5f);
		firstVisibleString_ = sliderPhase2Index(cur->value());
		if(autoHideSlider_)
			if(size <= 0)
				cur->hide(true);
			else
				cur->show();
	}
}

const char* UI_ControlStringList::selectedString() const
{
	const ComboStrings& lst = getList();
	xassert(selectedString_ < (int)lst.size());
	if(selectedString_ >= 0 && selectedString_ < lst.size())
		return lst[selectedString_].c_str();
	return 0;
}

bool UI_ControlStringList::redraw() const
{
	if(!UI_ControlBase::redraw())
		return false;

	if(isScaled())
		return true;

	const cFont* fnt = cfont();

	const UI_TextFormat* frm0 = textFormat(UI_SHOW_NORMAL);
	const UI_TextFormat* frm1 = textFormat(UI_SHOW_HIGHLITED);
	const UI_TextFormat* frm2 = showMode(UI_SHOW_ACTIVATED) ? textFormat(UI_SHOW_ACTIVATED) : frm1;

	Rectf text_pos = textPosition();
	Rectf pos = text_pos;
	pos.height(stringHeight_);

	for(int i = firstVisibleString_; i < (int)strings_.size(); i++){
		if(pos.top() -(text_pos.top() + text_pos.height() - stringHeight_) > 0.0005f)
			break;

		const UI_TextFormat* format = frm0;
		if(i == selectedString_)
			format = frm2;
		else if(pos.point_inside(UI_LogicDispatcher::instance().mousePosition()))
			format = frm1;

		ComboStrings cs;
		if(columns_.size() > 0)
			splitComboList(cs, strings_[i].c_str(), '\t');
		if(cs.size() > 1){
			Rectf col = pos;
			for(int idx = 0; idx < columns_.size(); ++idx){
				if(idx < cs.size()){
					if(columns_[idx].width){
						col.width(pos.width() / 100.f * columns_[idx].width);
						UI_TextFormat frm(format->textColor(), format->shadowColor(), format->blendMode());
						UI_Render::instance().outText(col, cs[idx].c_str(), &frm, columns_[idx].align, font(), alpha());
						col.left(col.left() + col.width());
					}
				}
				else
					break;
			}
		}
		else
			UI_Render::instance().outText(pos, strings_[i].c_str(), format, textAlign(), font(), alpha());

		pos.top(pos.top() + pos.height());
	}

	return true;
}

void UI_ControlStringList::drawDebug2D() const
{
	__super::drawDebug2D();

	if(!showDebugInterface.hoveredControlExtInfo ||	UI_LogicDispatcher::instance().hoverControl() != this)
		return;

	if(!columns_.empty()){
		Rectf col =  textPosition();
		UI_Render::instance().drawRectangle(col, sColor4f(1.f, 1.f, 1.f), true);
		float pw = col.width() / 100.f;
		for(int idx = 0; idx < columns_.size() - 1; ++idx)
			if(columns_[idx].width){
				col.width(pw * columns_[idx].width);
				XBuffer buf;
				buf <= columns_[idx].width;
				Rectf txtpos = col;
				txtpos.top(col.top() - stringHeight_);
				txtpos.height(stringHeight_);
				UI_Render::instance().outText(txtpos, buf.c_str(), &UI_TextFormat(WHITE, BLACK), UI_TEXT_ALIGN_CENTER);
				col.left(col.left() + col.width());
				Rectf line = col;
				line.width(0.f);
				UI_Render::instance().drawRectangle(line, sColor4f(.5f, 1.f, .5f));
			}
	}
}

int UI_ControlStringList::scrollSize() const
{
	return getList().size() - floor(textPosition().height() / stringHeight_);
}

float UI_ControlStringList::index2sliderPhase(int index) const
{
	if(!slider())
		return 0;
	return slider()->index2sliderPhase(index, scrollSize());
}

int UI_ControlStringList::sliderPhase2Index(float value) const
{
	if(!slider())
		return 0;
	return slider()->sliderPhase2Index(value, scrollSize());
}

void UI_ControlStringList::setSelect(const ComboStrings& selectList)
{
	if(selectList.empty())
		setSelectedString(-1);
	else {
		const ComboStrings& lst = getList();
		ComboStrings::const_iterator it = find(lst.begin(), lst.end(), selectList.front());
		if(it != lst.end())
			setSelectedString(distance(lst.begin(), it));
	}
}

void UI_ControlStringList::getSelect(ComboStrings& lst)
{
	lst.clear();
	if(const char* str = selectedString())
		lst.push_back(str);
}

// ------------------- UI_ControlStringCheckedList

UI_ControlStringCheckedList::UI_ControlStringCheckedList()
{

}

UI_ControlStringCheckedList::~UI_ControlStringCheckedList()
{

}

void UI_ControlStringCheckedList::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(checkOn_, "checkOn", "Пометка выбранной");
	ar.serialize(checkOff_, "checkOff", "Пометка НЕ выбранной");
	
	postSerialize();
}

void UI_ControlStringCheckedList::init()
{
	__super::init();
	
	selected_.assign(strings_.size(), 0);
}

void UI_ControlStringCheckedList::toggleString(int idx)
{
	xassert(strings_.size() == selected_.size());
	__super::toggleString(idx);
	if(idx >= 0)
		selected_[idx] = !selected_[idx];
}

void UI_ControlStringCheckedList::applyNewList(const ComboStrings& newList)
{
	if(strings_.size() != newList.size())
		selected_.assign(newList.size(), 0);

	__super::applyNewList(newList);
}

void UI_ControlStringCheckedList::setSelect(const ComboStrings& selectList)
{
	UI_AUTOLOCK();
	
	flushNewStrings();
	xassert(strings_.size() == selected_.size());
	
	selected_.assign(selected_.size(), 0);
	
	ComboStrings::const_iterator sel;
	FOR_EACH(selectList, sel){
		ComboStrings::iterator it = find(strings_.begin(), strings_.end(), *sel);
		if(it != strings_.end())
			selected_[distance(strings_.begin(), it)] = 1;
	}
}

void UI_ControlStringCheckedList::getSelect(ComboStrings& out)
{
	MTG();
	flushNewStrings();
	xassert(strings_.size() == selected_.size());
	
	out.clear();
	for(int i = 0; i < strings_.size(); ++i)
		if(selected_[i])
			out.push_back(strings_[i]);
}

bool UI_ControlStringCheckedList::redraw() const
{
	if(!UI_ControlBase::redraw())
		return false;

	xassert(strings_.size() == selected_.size());

	if(isScaled())
		return true;

	const cFont* fnt = cfont();

	stringHeight_ = fnt->size() * stringHeightFactor_ / UI_Render::instance().windowPosition().height();

	const UI_TextFormat* frm0 = textFormat(UI_SHOW_NORMAL);
	const UI_TextFormat* frm1 = textFormat(UI_SHOW_HIGHLITED);
	const UI_TextFormat* frm2 = showMode(UI_SHOW_ACTIVATED) ? textFormat(UI_SHOW_ACTIVATED) : frm1;

	Rectf text_pos = textPosition();
	Rectf pos = text_pos;
	pos.height(stringHeight_);

	for(int i = firstVisibleString_; i < strings_.size(); i++){
		if(pos.top() >= text_pos.top() + text_pos.height() - stringHeight_)
			break;

		const UI_TextFormat* format = frm0;
		const UI_SpriteReference* check = &checkOff_;
		if(selected_[i]){
			format = frm2;
			check = &checkOn_;
		}
		else if(pos.point_inside(UI_LogicDispatcher::instance().mousePosition()))
			format = frm1;

		string out = "<img=";
		out += check->c_str();
		out += ";style=2>";
		out += strings_[i];
		UI_Render::instance().outText(pos, out.c_str(), format, textAlign(), font(), alpha());

		pos.top(pos.top() + stringHeight_);
	}

	return true;

}

// ------------------- UI_ControlComboList

UI_ControlStringList* UI_ControlComboList::getList(UI_ControlBase* control)
{
	UI_ControlComboList* comboList = dynamic_cast<UI_ControlComboList*>(control);

	UI_ControlStringList* lst = 0;
	if(comboList)
		lst = comboList->dropList();
	else
		lst = dynamic_cast<UI_ControlStringList*>(control);

	return lst;
}

void UI_ControlComboList::setList(UI_ControlBase* control, const ComboStrings& strings, int selected)
{
	xassert(selected <= (int)strings.size());
	if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
		lst->setList(strings);
		lst->setSelectedString(selected);
	}
	if(selected >= 0)
		control->setText(strings[selected].c_str());
}

UI_ControlComboList::UI_ControlComboList()
{
	dropListVisible_ = false;
}

UI_ControlComboList::~UI_ControlComboList()
{
}

void UI_ControlComboList::serialize(Archive& ar)
{
	__super::serialize(ar);
	postSerialize();
}

void UI_ControlComboList::init()
{
	__super::init();

	if(UI_ControlStringList* drop_list = dropList())
		drop_list->hide(true);
}

void UI_ControlComboList::quant(float dt)
{
	if(UI_ControlStringList* drop_list = dropList()){
		drop_list->setPosition(Vect2f(position().left(), position().top() + position().height()));

		if(dropListVisible_){
			bool hide_list = false;

			if(drop_list->isActivated()){
				hide_list = true;

				if(const char* str = drop_list->selectedString()){
					setText(str);
					setInputActionFlag(UI_ACTION_SELF_EXECUTE);
					actionRequest();
				}
			}

			if(isActivated())
				hide_list = true;

			if(!drop_list->isVisible())
				drop_list->show();

			if(hide_list){
				drop_list->setShowMode(UI_SHOW_NORMAL);
				drop_list->hide(true);
				dropListVisible_ = false;
				UI_Dispatcher::instance().setFocus(0);
			}
			else {
				if(!drop_list->isVisible())
					drop_list->show();
			}
		}
		else {
			if(isActivated()){
				drop_list->show();
				dropListVisible_ = true;
				UI_Dispatcher::instance().setFocus(this);
			}
			else {
				if(drop_list->isVisible())
					drop_list->hide(true);
			}
		}
	}

	__super::quant(dt);
}

void UI_ControlComboList::onFocus(bool focus_state)
{
	if(!focus_state){
		if(dropListVisible_){
			dropListVisible_ = false;
			if(UI_ControlStringList* drop_list = dropList()){
				if(drop_list->isVisible())
					drop_list->hide();
			}
		}
	}
}

// ------------------- UI_ControlProgressBar

UI_ControlProgressBar::UI_ControlProgressBar()
{
	show_only_not_empty_ = false;
	full_is_transparent_ = false;
	vertical_ = false;
	progress_ = 0;

	colorDone_ = sColor4f(1,1,1,1);
	colorLeft_ = sColor4f(0,0,0,0);
}

UI_ControlProgressBar::~UI_ControlProgressBar()
{
}

void UI_ControlProgressBar::preLoad()
{
	__super::preLoad();
	progressSprite_.texture();
}

void UI_ControlProgressBar::release()
{
	__super::release();
	progressSprite_.release();
}

void UI_ControlProgressBar::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(colorDone_, "colorDone_", "цвет пройденной области");
	ar.serialize(colorLeft_, "colorLeft_", "цвет оставшейся области");
	ar.serialize(show_only_not_empty_, "show_only_not_empty_", "показывать только не пустой");
	ar.serialize(full_is_transparent_, "full_is_transparent", "полный становится прозрачным");
	ar.serialize(vertical_, "vertical", "вертикальный");
	ar.serialize(progressSprite_, "progressSprite", "Спрайт для прогресса");

	postSerialize();
}

bool UI_ControlProgressBar::redraw() const
{
	setDrawMode(UI_DRAW_ALL);
	
	const UI_ControlShowMode* mode = showMode(showModeID());
	
	if(mode && mode->sprite().isAnimated()){
		setDrawMode(UI_DRAW_BORDER);
		if(!UI_ControlBase::redraw())
			return false;
		
		UI_Render::instance().drawSprite(transfPosition(), mode->sprite(), alpha(), clamp(progress_, 0.f, 1.f));
		return true;
	}

	if(!UI_ControlBase::redraw())
		return false;

	if(full_is_transparent_ && progress_ >= 1.f - FLT_EPS)
		return true;
	
	if(progressSprite_.isAnimated()){
		sColor4f sprite_color = colorDone_;
		sprite_color.a *= alpha();
		UI_Render::instance().drawSprite(transfPosition(), progressSprite_, sprite_color, UI_BLEND_NORMAL, clamp(progress_, 0.f, 1.f));
	}
	else if(!progressSprite_.isEmpty()){
		if(progress_ > FLT_EPS){
			
			Rectf rect = transfPosition();
			Recti scr_pos_full = UI_Render::instance().screenCoords(rect);
			Rectf texCoords = progressSprite_.textureCoords();
			
			if(vertical_){
				float h = progress_ * rect.height();
				rect.top(rect.top() + rect.height() - h);
				rect.height(h);

				// боремся с паразитными округлениями
				Recti scr_pos = UI_Render::instance().screenCoords(rect);
				float realProgress = (float)scr_pos.height() / (float)scr_pos_full.height();

				h = realProgress * texCoords.height();
				texCoords.top(texCoords.top() + texCoords.height() - h);
				texCoords.height(h);
			}
			else{
				rect.width(progress_ * rect.width());

				// боремся с паразитными округлениями
				Recti scr_pos = UI_Render::instance().screenCoords(rect);
				float realProgress = (float)scr_pos.width() / (float)scr_pos_full.width();

				texCoords.width(realProgress * texCoords.width());
			}

			sColor4f diffuse = colorDone_;
			diffuse.a *= alpha();

			UI_Render::instance().drawSprite(rect, progressSprite_.texture(), texCoords, diffuse, UI_BLEND_NORMAL);
		}
	}
	else {
		Rectf rect = transfPosition();

		if(progress_ > FLT_EPS)
		{
			sColor4f col = colorDone_;
			col.a *= alpha();

			if(vertical_){
				float h = progress_ * transfPosition().height();
				rect.top(rect.top() + transfPosition().height() - h);
				rect.height(h);
			}
			else
				rect.width(progress_ * transfPosition().width());
			UI_Render::instance().drawRectangle(rect, col);
		}

		if(progress_ < 1.f - FLT_EPS)
		{
			sColor4f col = colorLeft_;
			col.a *= alpha();

			if(vertical_){
				rect.height((1.f - progress_) * transfPosition().height());
				rect.top(transfPosition().top());
			}
			else{
				rect.left(transfPosition().left() + progress_ * transfPosition().width());
				rect.width((1.0f - progress_) * transfPosition().width());
			}
			UI_Render::instance().drawRectangle(rect, col);
		}
	}

	return true;
}

// ------------------- UI_ControlVideo

UI_ControlVideo::UI_ControlVideo()
{
	cycled_ = false;
	mute_ = false;
	needNextFrame_ = false;
	wasStarted_ = false;
	diffuseColor_.set(1.f, 1.f, 1.f, 1.f);
}

UI_ControlVideo::~UI_ControlVideo()
{
}

void UI_ControlVideo::serialize(Archive& ar)
{
	__super::serialize(ar);
	
	ar.serialize(ResourceSelector(videoFile_, ResourceSelector::BINK_OPTIONS), "videoFileName", "Видео файл");
	ar.serialize(cycled_, "cycled", "Зациклить проигрывание");
	ar.serialize(mute_, "mute", "Без звука");
	ar.serialize(diffuseColor_, "diffuseColor", "Диффузный цвет");
	
	postSerialize();
}

void UI_ControlVideo::init()
{
	UI_ControlBase::init();

	setDrawMode(0);
}

void UI_ControlVideo::controlInit()
{
	UI_ControlBase::controlInit();

	wasStarted_ = false;
}

void UI_ControlVideo::release()
{
	__super::release();
	streamVideo().release();
}

void UI_ControlVideo::quant(float dt)
{
	UI_ControlBase::quant(dt);
	
	if(needNextFrame_){
		if(!streamVideo().inited(videoFile_.c_str())){
			streamVideo().init(videoFile_.c_str(), cycled_);
			streamVideo().mute(isUnderEditor() || mute_);
			wasStarted_ = false;
		}
		if(!wasStarted_){
			streamVideo().play();
			wasStarted_ = true;
		}

		streamVideo().setUpdated();
	}
	else
		wasStarted_ = false;

	needNextFrame_ = false;
}

bool UI_ControlVideo::redraw() const
{
	if(!(needNextFrame_ = UI_ControlBase::redraw()))
		return false;

	if(cTexture* tx = streamVideo().texture()){
		sColor4f color(diffuseColor_);
		color.a *= alpha();
		UI_Render::instance().drawSprite(transfPosition(), tx, Rectf(Vect2f::ZERO, streamVideo().size())
			, color
			, streamVideo().alphaPlan() ? UI_BLEND_NORMAL : UI_BLEND_FORCE);
	}

	return true;
}

void UI_ControlVideo::play()
{
	if(streamVideo().pause())
		streamVideo().pause(false);
	else
		streamVideo().play();
}

void UI_ControlVideo::pause()
{
	if(streamVideo().pause())
		streamVideo().pause(false);
	else
		streamVideo().pause(true);
}

// ------------------- UI_ControlCustom

UI_ControlCustom::UI_ControlCustom():
viewZoneColor_(WHITE)
{
	type_ = 0;
	clickAction_ = true;
	drawViewZone_ = true;
	useSelectedMap_ = false;
	mapAlpha_ = 1.f;
	drawFogOfWar_ = true;
	rotateByCamera_ = false;
	getAngleFromWorld_ = false;
	minimapAngle_ = 0.f;
	viewStartLocations_ = false;
	miniMapBorderColor_ = sColor4f(1,1,1,0);
	mapAlign_ = UI_ALIGN_CENTER;
}

UI_ControlCustom::~UI_ControlCustom()
{
}

void UI_ControlCustom::serialize(Archive& ar)
{
	__super::serialize(ar);

	ar.serialize(type_, "type", "тип");
	ar.serialize(clickAction_, "clickAction", "Реагировать на мышь");
	if(checkType(UI_CUSTOM_CONTROL_MINIMAP)){
		ar.serialize(mapAlign_, "mapAlign_", "Выравнивание карты");
		ar.serialize(mapAlpha_, "mapAlpha", "Прозрачность карты");
		ar.serialize(drawViewZone_, "drawViewZone", "Рисовать видимую область на миникарте");
		if(drawViewZone_)
			ar.serialize(viewZoneColor_, "viewZoneColor", "Цвет рамки видимой области");
		ar.serialize(useSelectedMap_, "useSelectedMap", "Брать миникарту из выделенного мира");
		if(!useSelectedMap_){
			ar.serialize(drawFogOfWar_, "drawFogOfWar", "Накладывать туман войны");
			ar.serialize(rotateByCamera_, "rotateByCamera", "Может вращаться за камерой");
			ar.serialize(getAngleFromWorld_, "getAngleFromWorld", "Брать угол поворота из мира");
			ar.serialize(miniMapBorderColor_, "miniMapBorderColor", "Цвет рамки миникарты");
		}
		else{
			drawFogOfWar_ = false;
			rotateByCamera_ = false;
		}
		if(!rotateByCamera_ && !getAngleFromWorld_)
			ar.serialize(minimapAngle_, "minimapAngle", "Угол поворота миникарты");
		ar.serialize(viewStartLocations_, "viewStartLocations", "Показывать стартовые локации");
		if(viewStartLocations_)
			ar.serialize(font_, "useFont", "Шрифт");
	}

	postSerialize();
}

// ------------------- UI_ControlUnitList

UI_ControlUnitList::UI_ControlUnitList()
{
	type_ = UI_UNITLIST_SELECTED;
	activeTransform_.setScale(Vect2f(1.25f, 1.25f));
}

UI_ControlUnitList::~UI_ControlUnitList()
{
}

void UI_ControlUnitList::preLoad()
{
	__super::preLoad();

	UI_UnitSpritePrms::iterator it;
	FOR_EACH(unitSpriteParams_, it)
		it->sprite_.texture();
}

void UI_ControlUnitList::release()
{
	__super::release();

	UI_UnitSpritePrms::iterator it;
	FOR_EACH(unitSpriteParams_, it)
		it->sprite_.release();
}

void UI_ControlUnitList::serialize(Archive& ar)
{
	UI_ControlBase::serialize(ar);

	ar.serialize(type_, "type_", "тип списка");
	ar.serialize(unitSpriteParams_, "unitSpriteParams_", "картинки юнитов");
	
	float scaleFactor = (activeTransform_.scale().x - 1.f) * 100;
	ar.serialize(scaleFactor, "scaleFactor", "% изменения активной ячейки");
	scaleFactor = 1.f + scaleFactor / 100.f;
	activeTransform_.setScale(Vect2f(scaleFactor, scaleFactor));
	
	postSerialize();
}

void UI_ControlUnitList::UI_UnitSpritePrm::serialize(Archive &ar){
	ar.serialize(unitAttributeReferenre_, "unitAttributeReferenre_", "&юнит");
	ar.serialize(ID_, "ID_", "&состояние");
	ar.serialize(sprite_, "sprite_", "текстура");
}

const UI_Sprite& UI_ControlUnitList::getSprite(const AttributeBase* unit, UI_ControlShowModeID id) const{
	if(!unitSpriteParams_.empty()){
		UI_UnitSpritePrms::const_iterator retSprite = unitSpriteParams_.begin(); // по умолчанию первый из списка
		AttributeUnitReference unitRef(unit);
		bool type_find = false;
		for(UI_UnitSpritePrms::const_iterator it = unitSpriteParams_.begin(); it != unitSpriteParams_.end(); ++it){
			if(it->unitAttributeReferenre_ == unitRef){ // по крайней мере этот юнит вообще есть
				if(!type_find){
					type_find = true;
					retSprite = it; 
				}
				if(it->ID_ == id){ // полное совпадение
					retSprite = it;
					break;
				}
			}
		}
		return retSprite->sprite_;
	}
	xxassert(false, "хотя бы одна текстура в списке юнитов должна быть задана");
	const_cast<UI_ControlUnitList*>(this)->unitSpriteParams_.resize(1);
	return unitSpriteParams_.front().sprite_;
}

void UI_ControlUnitList::setSprites(UI_ControlBase* control, const AttributeBase* unit){
	xassert(control);
	if(!unit){
		control->hide(true);
		const UI_Sprite& sprite = UI_Sprite::ZERO;
		control->setSprite(sprite, UI_SHOW_NORMAL);
		control->setSprite(sprite, UI_SHOW_DISABLED);
		control->setSprite(sprite, UI_SHOW_HIGHLITED);
		control->setSprite(sprite, UI_SHOW_ACTIVATED);
	} else {
		control->setSprite(getSprite(unit, UI_SHOW_NORMAL), UI_SHOW_NORMAL);
		control->setSprite(getSprite(unit, UI_SHOW_DISABLED), UI_SHOW_DISABLED);
		control->setSprite(getSprite(unit, UI_SHOW_HIGHLITED), UI_SHOW_HIGHLITED);
		control->setSprite(getSprite(unit, UI_SHOW_ACTIVATED), UI_SHOW_ACTIVATED);
		control->show();
	}
}

void UI_ControlUnitList::setSprites(UI_ControlBase* control, const UI_ShowModeSpriteReference& sprites)
{
	xassert(control);
	control->setSprite(sprites->sprite(UI_SHOW_NORMAL));
	control->setSprite(sprites->sprite(UI_SHOW_DISABLED));
	control->setSprite(sprites->sprite(UI_SHOW_HIGHLITED));
	control->setSprite(sprites->sprite(UI_SHOW_ACTIVATED));
	control->show();
}

// ------------------- UI_ControlContainer

class UI_ControlSortFunctor
{
public:
	// оператор "меньше". сортируются по убыванию Z для отрисовки, т.е. Z контрола больше - тем он ближе к началу списка
	// и тем раньше отрисуется, сответственно закроется кнопками с меньшей глубиной.
	// для обработки кликов кнопки выбираются в обратном порядке,
	// т.е. сначала событие получают кнопки с меньшим Z (глубиной)
	bool operator()(const UI_ControlBase* lh, const UI_ControlBase* rh) const { return lh->screenZ() > rh->screenZ(); }
};

unsigned int UI_ControlContainer::changeIndex_ = 1;

struct ControlCompare {
	bool operator() (ShareHandle<UI_ControlBase> lhs, ShareHandle<UI_ControlBase> rhs) {
		return stricmp(lhs->name(), rhs->name()) < 0;
	}
};

void UI_ControlContainer::sortControls()
{
	std::vector<ShareHandle <UI_ControlBase> > controlsVector(controls_.begin(), controls_.end());
	std::sort(controlsVector.begin(), controlsVector.end(), ControlCompare());
	std::copy(controlsVector.begin(), controlsVector.end(), controls_.begin());
}

void UI_ControlContainer::serialize(Archive& ar)
{
	ar.serialize(name_, "name_", "&имя");
	if(!ar.isEdit())
		ar.serialize(controls_, "controls_", "&кнопки");

	if(!isUnderEditor() && ar.isInput()){
		controls_.sort(UI_ControlSortFunctor());
		controlsSorted_ = true;
	}
}

void UI_ControlContainer::referenceString(string& out) const
{
	if(owner_){
		owner_->referenceString(out);
		out.push_back('.');
		out += name_;
	}
	else
		out = name_;
}

void UI_ControlContainer::init()
{
	std::for_each(controls_.begin(), controls_.end(), std::mem_fun(&UI_ControlBase::init));
}

void UI_ControlContainer::preLoad()
{
	for(ControlList::iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->preLoad();
}

void UI_ControlContainer::release()
{
	for(ControlList::iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->release();
}

bool UI_ControlContainer::addControlToList(UI_ControlBase* p)
{
	p->setOwner(this);
	controls_.push_back(p);
	toggleChange();
	return true;
}


bool UI_ControlContainer::removeControl(UI_ControlBase* control)
{
	return removeControlFromList(control);
}

bool UI_ControlContainer::removeControlFromList(UI_ControlBase* p)
{
	ControlList::iterator it = std::find(controls_.begin(), controls_.end(), p);
	if(it != controls_.end()){
		controls_.erase(it);
		toggleChange();
		return true;
	}
	
	FOR_EACH(controls_, it){
		UI_ControlBase* control = *it;
		if(control->removeControlFromList(p))
			return true;
	}

	return false;
}

bool UI_ControlContainer::redraw() const
{
	if(!controlsSorted_){
		ControlList lst;
		lst.insert(lst.end(), controls_.begin(), controls_.end());
		lst.sort(UI_ControlSortFunctor());
		for(ControlList::const_iterator it = lst.begin(); it != lst.end(); ++it)
			(*it)->redraw();
	}
	else {
		for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
			(*it)->redraw();
	}

	return true;
}

void UI_ControlContainer::drawDebug2D() const
{
	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->drawDebug2D();
}

UI_ControlBase* UI_ControlContainer::getControlByName(const char* name) const
{
	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it){
		if((*it)->hasName() && !strcmp((*it)->name(), name))
			return *it;
	}

	return 0;
}

UI_ControlBase* UI_ControlBase::findControl(const Vect2f& pos) const
{
	UI_ControlBase* p = 0;
	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it){
		if((*it)->hitTest(pos)){
			if(!p || p->screenZ() >= (*it)->screenZ())
				p = *it;
		}
	}

	return p;
}

void UI_ControlContainer::updateControlOwners()
{
	for(ControlList::iterator it = controls_.begin(); it != controls_.end(); ++it){
		(*it)->setOwner(this);
		(*it)->updateControlOwners();
	}
}

// ------------------- UI_ControlBase

UI_ControlBase::UI_ControlBase() : 
	handler_(this, &UI_ControlBase::hotKeyHandler)
{
	isDeactivating_ = false;
	waitingUpdate_ = false;
	redrawLock_ = false;

	transformMode_ = TRANSFORM_NONE;

	currentStateIndex_ = -1;
	textPosition_ = Rectf(0,0,1,1);
	textAlign_ = UI_TEXT_ALIGN_LEFT;
	textVAlign_ = UI_TEXT_VALIGN_TOP;

	borderPosition_ = Rectf(0,0,1,1);
	drawMode_ = UI_DRAW_ALL;
	borderFill_ = false;
	borderOutline_ = false;
	borderEnabled_ = false;
	borderColor_ = sColor4f(1,1,1,1);
	borderOutlineColor_ = sColor4f(1,1,1,1);

	setPosition (Rectf (0.4f, 0.45f, 0.2f, 0.1f));
	autoFormatText_ = false;
	screenZ_ = 0;

	actionFlags_ = 0;
	mouseHover_ = false;
	mouseHoverPrev_ = false;
	actionActive_ = false;

	isActive_ = false;
	isEnabled_ = true;
	needDeactivation_ = false;

	isScaled_ = false;
	alpha_ = 1.f;

	animationPlayMode_ = UI_BackgroundAnimation::PLAY_STARTUP;
    
	isVisible_ = true;
	visibleInEditor_ = true;
	isVisibleByTrigger_ = true;

	compatibleHotKey_ = false;
	canHovered_ = true;

	showModeID_ = UI_SHOW_NORMAL;
	 
	activationType_ = 0;
	activationScaleMode_ = UI_Transform::SCALE_CENTER;
	activationMove_ = ACTIVATION_MOVE_LEFT;

	hasDeactivationSettings_ = false;
	deactivationType_ = 0;
	deactivationScaleMode_ = UI_Transform::SCALE_CENTER;
	deactivationMove_ = ACTIVATION_MOVE_LEFT;

	activationTime_ = deactivationTime_ = 0.f;
	bgTextureAnimationTime_ = 0.f;
	bgTextureAnimationIsPlaying_ = true;
	linkToAnchor_ = false;
	textChanged_ = false;
}

UI_ControlBase::~UI_ControlBase()
{
}

void UI_ControlBase::serialize(Archive& ar)
{
	__super::serialize(ar);

	if (!ar.isEdit()) {
		ar.serialize(isVisible_, "isVisible_", "кнопка видима");
	} else {
		bool isHidden = !isVisible_;
		ar.serialize(isHidden, "isHidden", "спрятана");
		isVisible_ = !isHidden;
	}

	if (!ar.isEdit())
		ar.serialize(visibleInEditor_, "visibleInEditor_", 0);

	ar.serialize(isVisibleByTrigger_, "isVisibleByTrigger", "Разрешена триггером");

	ar.serialize(isEnabled_, "is_enabled", "доступна");
	showModeID_ = (isEnabled_) ? UI_SHOW_NORMAL : UI_SHOW_DISABLED;

	ar.serialize(hotKey_, "hotKey", "Горячая клавиша");
	ar.serialize(compatibleHotKey_, "compatibleHotKey", "Горячая клавиша может совпадать с кнопками управления");

	ar.serialize(linkToAnchor_, "linkToAnchor", "Привязать к миру");
	ar.serialize(bgTextureAnimationIsPlaying_, "bgTextureAnimationIsPlaying", "Проигрывать анимацию фона");

	if(linkToAnchor_)
		ar.serialize(anchorName_, "anchorName", "Имя якоря для привязки");

	ar.openBlock ("text", "текст");
	ar.serialize(locText_, "locText", "заголовок (лок)");
	if(locText_.empty() || ar.isEdit() && ar.isOutput())
		ar.serialize(text_, "text_", "заголовок");
	ar.serialize(textAlign_, "textAlign_", "выравнивание по горизонтали");
	ar.serialize(textVAlign_, "textVAlign", "выравнивание по вертикали");
	ar.serialize(font_, "font_", "шрифт");
	ar.serialize(autoFormatText_, "autoFormatText", "форматировать текст");
	ar.closeBlock ();


	ar.openBlock ("border", "рамка");
	ar.serialize(borderEnabled_, "borderEnabled", "рисовать");
	ar.serialize(borderFill_, "borderFill", "заливать");
	ar.serialize(borderColor_, "borderColor", "цвет");
	ar.serialize(borderOutline_, "borderOutline", "обводка");
	ar.serialize(borderOutlineColor_, "borderOutlineColor", "цвет обводки");

	ar.closeBlock ();

	ar.serialize(screenZ_, "screenZ_", "Z");

	if(!ar.isEdit()){
		ar.serialize(states_, "states_", "состояния");
	}

	ar.openBlock("", "координаты");
	ar.serialize(position_, "position_", "кнопка");
	ar.serialize(textPosition_, "textPosition_", "текст");
	ar.serialize(borderPosition_, "borderPosition", "рамка");
	ar.closeBlock();
	
	ar.serialize(canHovered_, "canHovered", "Реагирует на мышь");
	if(canHovered_)
		ar.serialize(mask_, "mask", "маска прозрачности");
	
	if(!ar.isEdit() && !mask_.isEmpty() && mask_.polygon().size() < 3)
		canHovered_ = false;

	ar.openBlock("activation", "Активация");

	ar.serialize(activationType_, "activationType", "Тип");
	if(activationType_ & UI_Transform::TRANSFORM_COORDS)
		ar.serialize(activationMove_, "activationMove", "Направление вылета");
	if(activationType_ & UI_Transform::TRANSFORM_SCALE)
		ar.serialize(activationScaleMode_, "activationScaleMode", "Режим масштабирования");

	ar.serialize(hasDeactivationSettings_, "hasDeactivationSettings", "Свои настройки для деактивации");
	if(hasDeactivationSettings_){
		ar.openBlock("activation", "Деактивация");

		ar.serialize(deactivationType_, "deactivationType", "Тип");
		if(deactivationType_ & UI_Transform::TRANSFORM_COORDS)
			ar.serialize(deactivationMove_, "deactivationMove", "Направление отлёта");
		if(deactivationType_ & UI_Transform::TRANSFORM_SCALE)
			ar.serialize(deactivationScaleMode_, "deactivationScaleMode", "Режим масштабирования");
		ar.closeBlock();
	}

	ar.serialize(activationTime_, "activationTime", "Время активации");
	ar.serialize(deactivationTime_, "deactivationTime", "Время деактивации");

	ar.closeBlock();
	
	ar.openBlock("hover", "при наведении");
	ar.serialize(hoveredCursor_, "hoveredCursor", "курсор");
	ar.serialize(hoveredText_, "hoveredTextLoc", "текст подсказки (Loc)");
	ar.closeBlock();

	ar.serialize(actions_, "actions", "назначения");
	ar.serialize(backgroundAnimations_, "backgroundAnimations", "анимационные цепочки");
}

void UI_ControlBase::init()
{
	__super::init();

	if(hotKey_.fullkey)
		ControlManager::instance().registerHotKey(hotKey_.fullkey, &handler_);

	StateContainer::iterator it;
	FOR_EACH(states_, it)
		it->preLoad();

	bgTextureAnimationTime_ = 0.f;
}

void UI_ControlBase::postSerialize()
{
	if(!states_.empty())
		setState(0);

	transfPosition_ = position_;

	const char* lt = locText_.c_str();
	if(lt && *lt)
		setText(lt);

	textChanged();
}

void UI_ControlBase::preLoad()
{
	__super::preLoad();

	StateContainer::iterator it;
	FOR_EACH(states_, it)
		it->preLoad();
}

void UI_ControlBase::release()
{
	__super::release();
	
	if(hotKey_.fullkey)
		ControlManager::instance().unRegisterHotKey(&handler_);

	StateContainer::iterator it;
	FOR_EACH(states_, it)
		it->release();
}

void UI_ControlBase::setVisibleInEditor(bool visible)
{
	if(visibleInEditor_ != visible){
		transformMode_ = TRANSFORM_NONE;
		visibleInEditor_ = visible;
		if(!visible){
			if(hasActivationTransform()){
				setActivationTransform(0.f, false);
				transformMode_ = TRANSFORM_DEACTIVATION;
			}
			else
				startActivationEffects(0.f, true, true);
		}
		else
			startActivationEffects(0.f, true, false);
	}
}

void fCommandControlApplyHide(void* data)
{
	UI_ControlBase** control = (UI_ControlBase**)data;
	bool flag = *(bool*)(control + 1);
	(*control)->applyHide(flag);
}

void fCommandControlApplyShow(void* data)
{
	(*(UI_ControlBase**)(data))->applyShow();
}

void UI_ControlBase::applyHide(bool immediately)
{
	MTG();
	
	LOG_UI_STATE(immediately ? "immediately" : 0);

	if(screen()->isDeactivating()){
		transformMode_ = TRANSFORM_DEACTIVATION;
		return;
	}

	transformMode_ = TRANSFORM_NONE;

	stopBackgroundAnimation(UI_BackgroundAnimation::PLAY_HOVER_PERMANENT);

	if(immediately)
		startBackgroundAnimation(UI_BackgroundAnimation::PLAY_STARTUP, true, true);
	else if(hasActivationTransform()){
		setActivationTransform(0.f, false);
		if(!transform_.isFinished())
			transformMode_ = TRANSFORM_DEACTIVATION;
	}
	else
		startActivationEffects(0.f, true, true);
}

void UI_ControlBase::applyShow()
{
	MTG();
	transformMode_ = TRANSFORM_NONE;
	startActivationEffects(0.f, true, false);
	LOG_UI_STATE(0);
	redrawLock_ = false;
}

void UI_ControlBase::badActivationMessage()
{
#ifndef _FINAL_VERSION_
	const UI_Screen* scr = screen();
	if(scr->isDeactivating() || scr->isActivating()){

		static bool first=true;
		if(!first)
			return;
		first=false;

		kdWarning("UI", "Попытка изменить состояние кнопки во время активации/деактивации экрана");
	}
#endif
}

void UI_ControlBase::hide(bool immediately)
{
	badActivationMessage();

	hideChildControls(immediately);

	if(isVisible_){
		LOG_UI_STATE(0);
		isVisible_ = false;

		if(!immediately)
			transformMode_ = TRANSFORM_DEACTIVATION;

		if(MT_IS_GRAPH())
			applyHide(immediately);
		else
			uiStreamCommand.set(fCommandControlApplyHide) << this << immediately;
	}
}

void UI_ControlBase::hideChildControls(bool immediately)
{
	std::for_each(controls_.begin(), controls_.end(), bind2nd(std::mem_fun(&UI_ControlBase::hide), immediately));
}

void UI_ControlBase::show()
{
	badActivationMessage();

	showChildControls();

	if(!isVisible_){
		LOG_UI_STATE(0);
		waitingUpdate_ = true;
		isVisible_ = true;

		redrawLock_ = true;
		if(MT_IS_GRAPH())
			applyShow();
		else
			uiStreamCommand.set(fCommandControlApplyShow) << this;
	}
}

void UI_ControlBase::showChildControls()
{
	std::for_each(controls_.begin(), controls_.end(), std::mem_fun(&UI_ControlBase::show));
}

void UI_ControlBase::hideByTrigger()
{
	badActivationMessage();

	std::for_each(controls_.begin(), controls_.end(), std::mem_fun(&UI_ControlBase::hideByTrigger));

	if(isVisibleByTrigger_){
		LOG_UI_STATE(0);
		isVisibleByTrigger_ = false;
		waitingUpdate_ = true;

		if(isVisible_){
			transformMode_ = TRANSFORM_DEACTIVATION;
			if(MT_IS_GRAPH())
				applyHide(false);
			else
				uiStreamCommand.set(fCommandControlApplyHide) << this << false;
		}
	}
}

void UI_ControlBase::showByTrigger()
{
	badActivationMessage();

	std::for_each(controls_.begin(), controls_.end(), std::mem_fun(&UI_ControlBase::showByTrigger));

	if(!isVisibleByTrigger_){
		LOG_UI_STATE(0);
		waitingUpdate_ = true;
		isVisibleByTrigger_ = true;
		
		if(isVisible_) {
			redrawLock_ = true;
			if(MT_IS_GRAPH())
				applyShow();
			else
				uiStreamCommand.set(fCommandControlApplyShow) << this;
		}
	}
}

void UI_ControlBase::disable()
{
	isEnabled_ = false;
	std::for_each(controls_.begin(), controls_.end(), std::mem_fun(&UI_ControlBase::disable));
}

void UI_ControlBase::enable()
{
	isEnabled_ = true;
	std::for_each(controls_.begin(), controls_.end(), std::mem_fun(&UI_ControlBase::enable));
}

void UI_ControlBase::startAnimation(PlayControlAction action, bool recursive)
{
	switch(action){
		case PLAY_ACTION_STOP:
			bgTextureAnimationTime_ = 0.f;
			bgTextureAnimationIsPlaying_ = false;
			break;

		case PLAY_ACTION_PAUSE:
			bgTextureAnimationIsPlaying_ = !bgTextureAnimationIsPlaying_;
			break;

		case PLAY_ACTION_RESTART:
			bgTextureAnimationTime_ = 0.f;
		case PLAY_ACTION_PLAY:
			bgTextureAnimationIsPlaying_ = true;
			break;
	}

	if(recursive)
		for(ControlList::iterator ci = controls_.begin(); ci != controls_.end(); ++ci)
			(*ci)->startAnimation(action, true);
}

float UI_ControlBase::activationTime() const
{
	float time = activationTime_;

	for(UI_BackgroundAnimations::const_iterator it = backgroundAnimations_.begin(); it != backgroundAnimations_.end(); ++it){
		if(it->playMode() == UI_BackgroundAnimation::PLAY_STARTUP)
			time = max(time, it->duration());
	}

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		time = max(time, (*it)->activationTime());

	return time;
}

float UI_ControlBase::deactivationTime() const
{
	float time = deactivationTime_;

	for(UI_BackgroundAnimations::const_iterator it = backgroundAnimations_.begin(); it != backgroundAnimations_.end(); ++it){
		if(it->playMode() == UI_BackgroundAnimation::PLAY_STARTUP)
			time = max(time, it->duration());
	}

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		time = max(time, (*it)->activationTime());

	return time;
}

bool UI_ControlBase::isChangeProcess() const
{
	return !transform_.isFinished();
}

bool UI_ControlBase::setPosition(const Vect2f& pos)
{
	position_.left(pos.x);
	position_.top(pos.y);

	transfPosition_ = position_;

	return setPosition(position_);
}

bool UI_ControlBase::setPosition(const Rectf& pos)
{
	position_ = pos;
	transfPosition_ = pos;
	return true;
}

Rectf UI_ControlBase::textPosition() const
{
	return Rectf(transfPosition_.left() + textPosition_.left() * transfPosition_.width(),
		transfPosition_.top() + textPosition_.top() * transfPosition_.height(),
		textPosition_.width() * transfPosition_.width(),
		textPosition_.height() * transfPosition_.height());
}

bool UI_ControlBase::hitTest(const Vect2f& pos) const
{
	if(!canHovered()) return false;

	if(!mask_.isEmpty()){
		if(mask_.hitTest(pos - transfPosition_.left_top()))
			return true;
	}
	else if(transfPosition_.point_inside(pos))
		return true;

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		if((*it)->hitTest(pos))
			return true;

	return false;
}

bool UI_ControlBase::activate()
{
	isActive_ = true;
	needDeactivation_ = true;
	return true;
}

const UI_Screen* UI_ControlBase::screen() const
{
	const UI_ControlContainer* parent = owner();
	while(parent->owner())
		parent = parent->owner();
	return safe_cast<const UI_Screen*>(parent);
}

void UI_ControlBase::setHotKey(sKey hotKey)
{
	hotKey_ = hotKey;
	if(hotKey_.fullkey)
		ControlManager::instance().registerHotKey(hotKey_.fullkey, &handler_);
}

bool UI_ControlBase::hotKeyHandler()
{
	if(isEnabled() && isVisible() && !redrawLock_ && !isDeactivating_)
	{
		if(!screen()->isActive())
			return false;

		LOG_UI_STATE(hotKey_.toString().c_str());

		UI_InputEvent event(UI_INPUT_HOTKEY, position_.center());
		inputEventHandler(event);

		UI_LogicDispatcher::instance().inputEventProcessed(event, this);

		return true;
	}
	LOG_UI_STATE(XBuffer() < hotKey_.toString().c_str() < " SKIPED");
	return false;
}

UI_ControlState* UI_ControlBase::state(int state_index)
{
	xassert(state_index >= 0 && state_index < states_.size());
	return &states_[state_index];
}

const UI_ControlState* UI_ControlBase::state(int state_index) const
{
	xassert(state_index >= 0 && state_index < states_.size());
	return &states_[state_index];
}

UI_ControlState* UI_ControlBase::state(const char* state_name)
{
	StateContainer::iterator it = std::find(states_.begin(),states_.end(),state_name);
	if(it != states_.end())
		return &*it;

	return 0;
}

const UI_ControlState* UI_ControlBase::state(const char* state_name) const
{
	StateContainer::const_iterator it = std::find(states_.begin(),states_.end(),state_name);
	if(it != states_.end())
		return &*it;

	return 0;
}

int UI_ControlBase::getStateByMark(StateMarkType type, int value) const
{
	UI_DataStateMark mark(type, value);
	StateContainer::const_iterator it = std::find(states_.begin(), states_.end(), mark);
	if(it != states_.end())
		return distance(states_.begin(), it);
	return 0;
}

bool UI_ControlBase::setState(int state_index)
{
	if (!(state_index >= 0 && state_index < states_.size())) {
		currentStateIndex_ = -1;
	} else if(currentStateIndex_ != state_index){
		currentStateIndex_ = state_index;
		startAnimation(PLAY_ACTION_RESTART, false);
	}
	return true;
}

bool UI_ControlBase::setState(const char* state_name)
{
	StateContainer::iterator it = std::find(states_.begin(),states_.end(),state_name);
	if(it == states_.end())
		return false;

	
	return setState(std::distance(states_.begin(), it));
}

void fCommandSetUISprite(void* data){
	UI_ControlShowMode** show_mode = (UI_ControlShowMode**)data;
	UI_Sprite** sprite = (UI_Sprite**)(show_mode + 1);
	(*show_mode)->setSprite(**sprite);
}

bool UI_ControlBase::setSprite(const UI_Sprite& sprite, int showMode)
{
	if(currentStateIndex_ >= 0){
		xassert(currentStateIndex_ < states_.size()); 
		UI_ControlShowModeID modeID = showMode >= 0 ? UI_ControlShowModeID(showMode) : showModeID_;
		if(UI_ControlShowMode* mp = states_[currentStateIndex_].showMode(modeID)){
			if(MT_IS_GRAPH())
				mp->setSprite(sprite);
			else{
				uiStreamCommand.set(fCommandSetUISprite);
				uiStreamCommand << (void*)mp;
				uiStreamCommand << (void*)&sprite;
			}
			return true;
		}
	}
	return false;
}

void UI_ControlBase::applyNewText(const char* newText)
{
	MTG();
	if(newText && *newText)
		text_ = newText;
	else if(text_.empty())
		return;
	else
		text_.clear();

	textChanged();
}

void UI_ControlBase::setText(const char* p)
{
	LOG_UI_STATE(p);
	if(MT_IS_GRAPH()){
		applyNewText(p);
		textChanged_ = false;
	}
	else if(waitingExecution())
		return;
	else if(!textChanged_){ // второй раз за квант поменять нельзя, HT possible трабл
		if(p)
			newText_ = p;
		else
			newText_.clear();
		textChanged_ = true;
	}
}

void UI_ControlBase::clearInputEventFlags()
{
	actionFlags_ = 0;
	std::for_each(controls_.begin(), controls_.end(), std::mem_fun(&UI_ControlBase::clearInputEventFlags));
}

const UI_ActionData* UI_ControlBase::findAction(UI_ControlActionID id) const
{
	UI_ControlActionList::const_iterator it;
	FOR_EACH(actions_, it)
		if(it->key() == id)
			return it->type();

	if(const UI_ControlState* state = currentState())
		for(int i = 0; i < state->actionCount(); i++)
			if(state->actionID(i) == id)
				return state->actionData(i);

	return 0;
}

void UI_ControlBase::updateIndex()
{
	if(findAction(UI_ACTION_CLICK_FOR_TRIGGER)){
		UI_ControlReference th(this);
		ui_ControlMapReference[ui_ControlMapBackReference[th.reference()] = UI_Dispatcher::instance().getNextControlID()] = th.reference();
	}
	std::for_each(controls_.begin(), controls_.end(), std::mem_fun(&UI_ControlBase::updateIndex));
}

bool UI_ControlBase::hoverUpdate(const Vect2f& cursor_pos)
{
	if(!isVisible())
		return false;

	for(ControlList::reverse_iterator it = controls_.rbegin(); it != controls_.rend(); ++it)
		if((*it)->hoverUpdate(cursor_pos))
			return true;

	if(hitTest(cursor_pos)){
		if(isEnabled())
			hoverToggle(true);
		UI_LogicDispatcher::instance().setHoverControl(this);
		return true;
	}

	return false;
}

void UI_ControlBase::setShowMode(UI_ControlShowModeID mode)
{
	if(showModeID_ != mode){
		if(const UI_ControlShowMode* mp = showMode(mode))
			if(const SoundAttribute* soundAttr = mp->sound(showModeID_))
				soundAttr->play();

		showModeID_ = mode;
	}
}

void UI_ControlBase::controlComboListBuild(std::string& list_string, UI_ControlFilterFunc filter) const
{
	if(hasName()){
		if(!filter || (*filter)(this)){
			UI_ControlReference ref(this);
			list_string += "|";
			list_string += ref.referenceString();
		}

		for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
			(*it)->controlComboListBuild(list_string, filter);
	}
}

void UI_ControlBase::setTransform(const UI_Transform& transf)
{
	transform_ = transf;
	transform_.start();
}

void UI_ControlBase::drawBorder() const
{
	if(borderEnabled_){
		if(borderFill_){
			sColor4f col = borderColor_;
			col.a *= alpha_;
			UI_Render::instance().drawRectangle(borderPosition(), col);
		}
		if(borderOutline_){
			sColor4f col = borderOutlineColor_;
			col.a *= alpha_;
			UI_Render::instance().drawRectangle(borderPosition(), col, true);
		}
	}
}

Rectf UI_ControlBase::borderPosition() const
{
	return Rectf(transfPosition_.left() + borderPosition_.left() * transfPosition_.width(),
		transfPosition_.top() + borderPosition_.top() * transfPosition_.height(),
		borderPosition_.width() * transfPosition_.width(),
		borderPosition_.height() * transfPosition_.height());
}

int UI_ControlBase::getSubRectCount () const
{
    return 2;
}

void UI_ControlBase::setSubRect(int index, const Rectf& rect)
{
    if(index == 0) {
        textPosition_ = rect;
    } else if (index == 1) {
        borderPosition_ = rect;
    } else {
        xassert (0 && "Bad sub rect index!");
    }
}

const cFont* UI_ControlBase::cfont() const
{
	const UI_Font* fnt = font_;
	if(!fnt)
		fnt = UI_Render::instance().defaultFont();
	
	xassert(fnt);

	const cFont* font = fnt->font();
	xassert(font);
	return font;
}

const UI_TextFormat* UI_ControlBase::textFormat(UI_ControlShowModeID id) const
{
	if(const UI_ControlShowMode* mode = showMode(id))
		return &mode->textFormat();

	return &UI_TextFormat::WHITE;
}

Rectf UI_ControlBase::getSubRect(int index) const
{
    if(index == 0) {
        return textPosition_;
    } else if (index == 1) {
        return borderPosition_;
    } else {
        xassert (0 && "Bad sub rect index!");
        return Rectf();
    }
}

void UI_ControlBase::setActivationTransform(float dt, bool reverse)
{
	if(activationType_){
		float time = reverse ? activationTime_ : deactivationTime_;
		if(time > FLT_EPS)
			dt = time;

		UI_Transform transform(dt, reverse);

		if(!reverse && hasDeactivationSettings_){
			if(deactivationType_ & UI_Transform::TRANSFORM_COORDS){
				Vect2f shift(0,0);

				if(const UI_Screen* scr = screen()){
					switch (deactivationMove_)
					{
					case ACTIVATION_MOVE_LEFT:
					case ACTIVATION_MOVE_RIGHT:
						shift.x = scr->activationOffset(deactivationMove_);
						break;
					case ACTIVATION_MOVE_TOP:
					case ACTIVATION_MOVE_BOTTOM:
						shift.y = scr->activationOffset(deactivationMove_);
						break;
					case ACTIVATION_MOVE_CENTER:
						shift = Vect2f(0.5f, 0.5f) - position_.center();
						break;
					}
				}
				transform.setTrans(shift);
			}

			if(deactivationType_ & UI_Transform::TRANSFORM_SCALE_X)
				transform.setScaleX(1.f);
			if(deactivationType_ & UI_Transform::TRANSFORM_SCALE_Y)
				transform.setScaleY(1.f);

			transform.setScaleMode(deactivationScaleMode_);

			if(deactivationType_ & UI_Transform::TRANSFORM_ALPHA)
				transform.setAlpha(1.f);
		}
		else {
			if(activationType_ & UI_Transform::TRANSFORM_COORDS){
				Vect2f shift(0,0);

				if(const UI_Screen* scr = screen()){
					switch (activationMove_)
					{
					case ACTIVATION_MOVE_LEFT:
					case ACTIVATION_MOVE_RIGHT:
						shift.x = scr->activationOffset(activationMove_);
						break;
					case ACTIVATION_MOVE_TOP:
					case ACTIVATION_MOVE_BOTTOM:
						shift.y = scr->activationOffset(activationMove_);
						break;
					case ACTIVATION_MOVE_CENTER:
						shift = Vect2f(0.5f, 0.5f) - position_.center();
						break;
					}
				}
				transform.setTrans(shift);
			}

			if(activationType_ & UI_Transform::TRANSFORM_SCALE_X)
				transform.setScaleX(1.f);
			if(activationType_ & UI_Transform::TRANSFORM_SCALE_Y)
				transform.setScaleY(1.f);

			transform.setScaleMode(activationScaleMode_);

			if(activationType_ & UI_Transform::TRANSFORM_ALPHA)
				transform.setAlpha(1.f);
		}

		setTransform(transform);
		transform.apply(this);
	}
	else
		resetTransform();

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->setActivationTransform(dt, reverse);
}

bool UI_ControlBase::redraw() const
{
	MTG();

	if( transformMode_ != TRANSFORM_DEACTIVATION &&
		(!isUnderEditor() && !isVisible() || isUnderEditor() && !visibleInEditor()) ||
		redrawLock_ || isDeactivating_ || isBackgroundAnimationActive(UI_BackgroundAnimation::PLAY_STARTUP))
	{
		LOG_UI_STATE("redraw skiped");	
		return false;
	}
	LOG_UI_STATE("ok");
	
	if(drawMode_ & UI_DRAW_BORDER)
		drawBorder();

	if(drawMode_ & UI_DRAW_SPRITE)
		if(const UI_ControlShowMode* mode = showMode(showModeID_))
			UI_Render::instance().drawSprite(transfPosition_, mode->sprite(), alpha_, mode->sprite().phase(bgTextureAnimationTime_, mode->cycledAnimation()));

	if(drawMode_ & UI_DRAW_TEXT)
		if(!isScaled_ && !text_.empty())
			UI_Render::instance().outText(textPosition(), text(), textFormat(), textAlign(), font_, alpha_, autoFormatText_);

	UI_ControlContainer::redraw();

	return true;
}

void UI_ControlBase::getDebugString(XBuffer& buf) const
{
	buf < "Act:" <= activationTime_ < ", ";
	buf < "Deact:" <= deactivationTime_;
	buf < " Transform: ";
	transform_.getDebugString(buf);
	buf < ", alpha: " <= alpha();
}

void UI_ControlBase::drawDebug2D() const
{
	__super::drawDebug2D();

	if(	transformMode_ != TRANSFORM_DEACTIVATION &&
		(!isUnderEditor() && !isVisible() || isUnderEditor() && !visibleInEditor()))
	{
		return;
	}

	if(showDebugInterface.showTransformInfo && needShowInfo()){
		XBuffer buf;
		buf.SetDigits(2);
		getDebugString(buf);
		UI_Render::instance().outDebugText(transfPosition_.left_top(), buf.c_str(), &YELLOW);
	}

	if(showDebugInterface.controlBorder || showDebugInterface.hoveredControlBorder && needShowInfo()){
		UI_Render::instance().drawRectangle(transfPosition_, sColor4f(1,1,1,0.7f), true);
		if(!mask_.isEmpty())
			mask_.draw(transfPosition_.left_top(), sColor4f(1,1,1,0.5f));
	}
}

const char* UI_ControlBase::hint() const
{
	if(*hoveredText_.c_str())
		return hoveredText_.c_str();
	
	if(const UI_ControlState* state = currentState())
		return state->hint();
	
	return "";
}

void UI_ControlBase::startBackgroundAnimation(UI_BackgroundAnimation::PlayMode mode, bool recursive, bool reverse)
{
	MTG();
	if(mode == UI_BackgroundAnimation::PLAY_STARTUP)
		isDeactivating_ = false;

	for(UI_BackgroundAnimations::const_iterator it = backgroundAnimations_.begin(); it != backgroundAnimations_.end(); ++it){
		switch(mode){
			case UI_BackgroundAnimation::PLAY_STARTUP:
				if(it->playMode() == UI_BackgroundAnimation::PLAY_PERMANENT)
						UI_BackgroundScene::instance().stop(*it);
				break;
			case UI_BackgroundAnimation::PLAY_HOVER_STARTUP:
				if(it->playMode() == UI_BackgroundAnimation::PLAY_HOVER_PERMANENT)
						UI_BackgroundScene::instance().stop(*it);
				break;
		}
	}

	for(UI_BackgroundAnimations::const_iterator it1 = backgroundAnimations_.begin(); it1 != backgroundAnimations_.end(); ++it1){
		if(it1->playMode() == mode){
			isDeactivating_ = (mode == UI_BackgroundAnimation::PLAY_STARTUP && reverse);
			UI_BackgroundScene::instance().play(*it1, reverse);
		}
	}

	animationPlayMode_ = mode;

	if(recursive){
		for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it){
			if((*it)->isVisible())
				(*it)->startBackgroundAnimation(mode, true, reverse);
		}
	}
}

void UI_ControlBase::stopBackgroundAnimation(UI_BackgroundAnimation::PlayMode mode, bool recursive) const
{
	MTG();
	for(UI_BackgroundAnimations::const_iterator it = backgroundAnimations_.begin(); it != backgroundAnimations_.end(); ++it){
		if(it->playMode() == mode)
			UI_BackgroundScene::instance().stop(*it);
	}

	if(recursive){
		for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it){
			(*it)->stopBackgroundAnimation(mode, true);
		}
	}
}

bool UI_ControlBase::isBackgroundAnimationActive(UI_BackgroundAnimation::PlayMode mode) const
{
	MTG();
	for(UI_BackgroundAnimations::const_iterator it = backgroundAnimations_.begin(); it != backgroundAnimations_.end(); ++it){
		if(it->playMode() == mode && UI_BackgroundScene::instance().isPlaying(*it))
			return true;
	}

	return false;
}

void UI_ControlBase::startActivationEffects(float dt, bool recursive, bool reverse)
{
	MTG();
	LOG_UI_STATE(XBuffer() < "recurs=" <= recursive < " revers=" <= reverse < " dt=" <= dt);

	setActivationTransform(dt, !reverse);
	startBackgroundAnimation(UI_BackgroundAnimation::PLAY_STARTUP, recursive, reverse);

	if(!reverse)
		startAnimation(PLAY_ACTION_RESTART, recursive);
}

void UI_ControlBase::updateActivationOffsets(float offsets[4])
{
	if(activationType_ & UI_Transform::TRANSFORM_COORDS){
		float offs = 0.f;
		switch(activationMove_){
		case ACTIVATION_MOVE_LEFT:
			offs = -position_.right();
			if(offsets[ACTIVATION_MOVE_LEFT] > offs)
				offsets[ACTIVATION_MOVE_LEFT] = offs;
			break;
		case ACTIVATION_MOVE_RIGHT:
			offs = 1.0f - position_.left();
			if(offsets[ACTIVATION_MOVE_RIGHT] < offs)
				offsets[ACTIVATION_MOVE_RIGHT] = offs;
			break;
		case ACTIVATION_MOVE_TOP:
			offs = -position_.bottom();
			if(offsets[ACTIVATION_MOVE_TOP] > offs)
				offsets[ACTIVATION_MOVE_TOP] = offs;
			break;
		case ACTIVATION_MOVE_BOTTOM:
			offs = 1.0f - position_.top();
			if(offsets[ACTIVATION_MOVE_BOTTOM] < offs)
				offsets[ACTIVATION_MOVE_BOTTOM] = offs;
			break;
		}
	}

	if(hasDeactivationSettings_ && (deactivationType_ & UI_Transform::TRANSFORM_COORDS)){
		float offs = 0.f;
		switch(deactivationMove_){
		case ACTIVATION_MOVE_LEFT:
			offs = -position_.right();
			if(offsets[ACTIVATION_MOVE_LEFT] > offs)
				offsets[ACTIVATION_MOVE_LEFT] = offs;
			break;
		case ACTIVATION_MOVE_RIGHT:
			offs = 1.0f - position_.left();
			if(offsets[ACTIVATION_MOVE_RIGHT] < offs)
				offsets[ACTIVATION_MOVE_RIGHT] = offs;
			break;
		case ACTIVATION_MOVE_TOP:
			offs = -position_.bottom();
			if(offsets[ACTIVATION_MOVE_TOP] > offs)
				offsets[ACTIVATION_MOVE_TOP] = offs;
			break;
		case ACTIVATION_MOVE_BOTTOM:
			offs = 1.0f - position_.top();
			if(offsets[ACTIVATION_MOVE_BOTTOM] < offs)
				offsets[ACTIVATION_MOVE_BOTTOM] = offs;
			break;
		}
	}

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->updateActivationOffsets(offsets);
}

void UI_ControlBase::transformQuant(float dt)
{
	if(!transform_.isFinished()){
		if(transformMode_ == TRANSFORM_DEACTIVATION || isVisible() || (visibleInEditor() && isUnderEditor()) &&
			!isDeactivating_ && !isBackgroundAnimationActive(UI_BackgroundAnimation::PLAY_STARTUP)){
				transform_.quant(dt);
		}

		if(transform_.isFinished() && transformMode_ == TRANSFORM_DEACTIVATION){
			startBackgroundAnimation(UI_BackgroundAnimation::PLAY_STARTUP, true, true);
			transformMode_ = TRANSFORM_NONE;
		}
	}

	transform_.apply(this);
	userTransform_.apply(this, true);
}

void UI_ControlBase::animationQuant(float dt)
{
	if(isVisible() || transformMode_){
		if(bgTextureAnimationIsPlaying_)
			bgTextureAnimationTime_ += dt;
	}
	else
		bgTextureAnimationTime_ = 0.f;
}

void UI_ControlBase::getHotKeyList(vector<UI_ControlHotKey>& out) const
{
	if(hotKey_.fullkey){
		out.push_back(UI_ControlHotKey());
		out.back().ref = UI_ControlReference(this);
		out.back().hotKey = hotKey_;
	}

	for(ControlList::const_iterator it = controlList().begin(); it != controlList().end(); ++it)
		(*it)->getHotKeyList(out);
}

void UI_ControlBase::getHotKeyList(vector<sKey>& out) const
{
	if(hotKey_.fullkey && !compatibleHotKey_){
		vector<sKey>::const_iterator it = std::find(out.begin(), out.end(), hotKey_);
		if(it == out.end())
			out.push_back(hotKey_);
	}

	for(ControlList::const_iterator it = controlList().begin(); it != controlList().end(); ++it)
		(*it)->getHotKeyList(out);
}
