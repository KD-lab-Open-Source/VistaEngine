#include "StdAfx.h"

#include "FileUtils.h"
#include "Factory.h"
#include "Serialization.h"
#include "XPrmArchive.h"
#include "EditArchive.h"
#include "ResourceSelector.h"
#include "TypeLibraryImpl.h"

#include "Sound.h"
#include "PlayOgg.h"
#include "..\Game\SoundApp.h"
#include "..\Units\UnitAttribute.h"

#include "UI_Render.h"
#include "UI_Types.h"
#include "UserInterface.h"
#include "UI_Controls.h"

#include "UI_Actions.h"
#include "UI_Inventory.h"

BEGIN_ENUM_DESCRIPTOR(ActivationMove, "ActivationMove")
REGISTER_ENUM(ACTIVATION_MOVE_LEFT, "Слева");
REGISTER_ENUM(ACTIVATION_MOVE_BOTTOM, "Снизу");
REGISTER_ENUM(ACTIVATION_MOVE_RIGHT, "Справа");
REGISTER_ENUM(ACTIVATION_MOVE_TOP, "Сверху");
REGISTER_ENUM(ACTIVATION_MOVE_CENTER, "Из центра");
END_ENUM_DESCRIPTOR(ActivationMove)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlBase, ResizeShiftHorizontal, "UI_ControlBase::ResizeShiftHorizontal")
REGISTER_ENUM_ENCLOSED(UI_ControlBase, RESIZE_SHIFT_LEFT, "влево");
REGISTER_ENUM_ENCLOSED(UI_ControlBase, RESIZE_SHIFT_CENTER_H, "в центр");
REGISTER_ENUM_ENCLOSED(UI_ControlBase, RESIZE_SHIFT_RIGHT, "вправо");
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlBase, ResizeShiftHorizontal)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlBase, ResizeShiftVertical, "UI_ControlBase::ResizeShiftVertical")
REGISTER_ENUM_ENCLOSED(UI_ControlBase, RESIZE_SHIFT_UP, "вверх");
REGISTER_ENUM_ENCLOSED(UI_ControlBase, RESIZE_SHIFT_CENTER_V, "в центр");
REGISTER_ENUM_ENCLOSED(UI_ControlBase, RESIZE_SHIFT_DOWN, "вниз");
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlBase, ResizeShiftVertical)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_Transform, Type, "UI_Transform::Type")
REGISTER_ENUM_ENCLOSED(UI_Transform, TRANSFORM_COORDS, "Перемещение")
REGISTER_ENUM_ENCLOSED(UI_Transform, TRANSFORM_SCALE_X, "Масштабирование по горизонтали")
REGISTER_ENUM_ENCLOSED(UI_Transform, TRANSFORM_SCALE_Y, "Масштабирование по вертикали")
REGISTER_ENUM_ENCLOSED(UI_Transform, TRANSFORM_ALPHA, "Прозрачность")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_Transform, Type)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_Transform, ScaleMode, "UI_Transform::ScaleMode")
REGISTER_ENUM_ENCLOSED(UI_Transform, SCALE_CENTER, "от центра")
REGISTER_ENUM_ENCLOSED(UI_Transform, SCALE_LEFT, "от левого края")
REGISTER_ENUM_ENCLOSED(UI_Transform, SCALE_TOP, "от верхнего края")
REGISTER_ENUM_ENCLOSED(UI_Transform, SCALE_RIGHT, "от правого края")
REGISTER_ENUM_ENCLOSED(UI_Transform, SCALE_BOTTOM, "от нижнего края")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_Transform, ScaleMode)

REGISTER_CLASS(UI_Text, UI_Text, "текст");
WRAP_LIBRARY(UI_TextLibrary, "UI_TextLibrary", "Сообщения", "Scripts\\Content\\UI_TextLibrary", 0, true);

REGISTER_CLASS(UI_Texture, UI_Texture, "текстура");
WRAP_LIBRARY(UI_TextureLibrary, "UI_TextureLibrary", "UI_TextureLibrary", "Scripts\\Content\\UI_TextureLibrary", 0, false);

REGISTER_CLASS(UI_Font, UI_Font, "шрифт");
WRAP_LIBRARY(UI_FontLibrary, "UI_FontLibrary", "Шрифты", "Scripts\\Content\\UI_FontLibrary", 0, true);

REGISTER_CLASS(UI_Cursor, UI_Cursor, "курсор");
WRAP_LIBRARY(UI_CursorLibrary, "UI_CursorLibrary", "Курсоры", "Scripts\\Content\\UI_CursorLibrary", 0, true);

WRAP_LIBRARY(UI_ShowModeSpriteTable, "UI_ShowModeSpriteTable", "Спрайты для кнопки", "Scripts\\Content\\UI_ShowModeSpriteTable", 0, true);

REGISTER_CLASS(UI_SpriteLibrary, UI_SpriteLibrary, "Спрайт");
WRAP_LIBRARY(UI_SpriteLibrary, "UI_SpriteLibrary", "Картинки для вставки в текст", "Scripts\\Content\\UI_TextSprite", 0, true);

REGISTER_CLASS(UI_MessageTypeLibrary, UI_MessageTypeLibrary, "Тип сообщения");
WRAP_LIBRARY(UI_MessageTypeLibrary, "UI_MessageTypeLibrary", "Типы выводимых сообщений", "Scripts\\Content\\UI_MessageTypes", 0, true);

REGISTER_CLASS(UI_ControlShowMode, UI_ControlShowMode, "режим отрисовки");

REGISTER_CLASS(UI_ControlBase, UI_ControlButton, "обычная кнопка");
REGISTER_CLASS(UI_ControlBase, UI_ControlSlider, "регулятор");
REGISTER_CLASS(UI_ControlBase, UI_ControlTab, "набор вкладок");
REGISTER_CLASS(UI_ControlBase, UI_ControlStringList, "список");
REGISTER_CLASS(UI_ControlBase, UI_ControlStringCheckedList, "список с пометками");
REGISTER_CLASS(UI_ControlBase, UI_ControlComboList, "выпадающий список");
REGISTER_CLASS(UI_ControlBase, UI_ControlEdit, "поле ввода");
REGISTER_CLASS(UI_ControlBase, UI_ControlHotKeyInput, "ввод горячей клавиши");
REGISTER_CLASS(UI_ControlBase, UI_ControlProgressBar, "индикатор прогресса");
REGISTER_CLASS(UI_ControlBase, UI_ControlCustom, "специальная кнопка");
REGISTER_CLASS(UI_ControlBase, UI_ControlTextList, "список текстов");
REGISTER_CLASS(UI_ControlBase, UI_ControlUnitList, "список юнитов");
REGISTER_CLASS(UI_ControlBase, UI_ControlInventory, "инвентарь");
REGISTER_CLASS(UI_ControlBase, UI_ControlVideo, "видео");

// ------------------- UI_Text

UI_Text::UI_Text()
{
}

void UI_Text::serialize(Archive& ar)
{
	ar.serialize(text_, "text", "&Текст (Лок)");
	ar.serialize(voice_, "voiceAttribute", "Голосовое сообщение");
}

// ------------------- UI_Texture

void UI_Texture::buildLocPath(const char* in, string& out)
{
	out = GameOptions::instance().getLocDataPath();
	out += "UI_Textures\\";
	out += extractFileName(in);
}

void UI_Texture::updateFileNameProxy()
{
	if(fileName_.empty())
		fileNameProxy_.clear();
	else if(localized_)
		buildLocPath(fileName_.c_str(), fileNameProxy_);
	else
		fileNameProxy_ = fileName_;
}

// если файл лежит в папке с локкитом, то
// путь нужно обрезать до имени и выставить галку, что текстура локализованная
void UI_Texture::setFileName(const char* name)
{
	xassert(name);
	string locPath;
	buildLocPath(name, locPath);
	if(localized_ = isFileExists(locPath.c_str()))
		fileName_ = extractFileName(name);
	else
		fileName_ = name;
	updateFileNameProxy();
}

const char* UI_Texture::textureFileName() const
{
	return fileNameProxy_.c_str();
}

void UI_Texture::serialize(Archive& ar)
{
	ar.serialize(localized_, "localized", 0);
	ar.serialize(fileName_, "fileName_", "&имя файла");
	if(ar.isInput())
		updateFileNameProxy();
	
	ar.serialize(hasResolutionVersion_, "hasResolutionVersion_", "поддержка разных разрешений");
	ar.serialize(hasRaceVersion_, "hasRaceVersion_", "поддержка разных рас");

	if(isUnderEditor()){
		if(ar.isOutput() && !ar.isEdit() && textureSizeInit_){
			if(textureSizeCurrent_ != Vect2i::ZERO)
				textureSize_ = textureSizeCurrent_;
		}
		else {
			if(ar.isInput())
				initSize();
		}
	}

	ar.serialize(textureSize_, "textureSize", 0);
}

bool UI_Texture::createTexture() const
{
	releaseTexture();

	if(fileName_.empty())
		return false;

	if(!showDebugInterface.disableTextures)
		texture_ = UI_Render::instance().createTexture(textureFileName());
	return true;
}

bool UI_Texture::releaseTexture() const
{
	return UI_Render::instance().releaseTexture(texture_);
}

bool UI_Texture::needRemapCoords() const
{
	if(!textureSizeInit_ || textureSize_ == textureSizeCurrent_ ||textureSize_ == Vect2i::ZERO || textureSizeCurrent_ == Vect2i::ZERO)
		return false;

	float x_ratio = float(textureSizeCurrent_.x) / float(textureSize_.x);
	float y_ratio = float(textureSizeCurrent_.y) / float(textureSize_.y);

	return fabs(x_ratio - y_ratio) > FLT_EPS;
}

Rectf UI_Texture::remapCoords(const Rectf& coords) const
{
	if(!textureSizeInit_ || textureSize_ == textureSizeCurrent_ ||textureSize_ == Vect2i::ZERO || textureSizeCurrent_ == Vect2i::ZERO)
		return coords;

	float x_ratio = float(textureSize_.x) / float(textureSizeCurrent_.x);
	float y_ratio = float(textureSize_.y) / float(textureSizeCurrent_.y);

	return coords * Vect2f(x_ratio, y_ratio);
}

cTexture* UI_Texture::texture() const
{
	if(!texture_)
		createTexture();

	return texture_;
}

void UI_Texture::initSize()
{
	if(textureSizeInit_)
		return;

	textureSizeInit_ = true;

	bool need_release = false;
	if(!texture_){
		createTexture();
		need_release = true;
	}

	if(texture_)
		textureSizeCurrent_ = Vect2i(texture_->GetWidth(), texture_->GetHeight());

	if(need_release)
		releaseTexture();
}

// ------------------- UI_Sprite

const  UI_Sprite UI_Sprite::ZERO;

UI_Sprite::UI_Sprite()
: textureCoords_(0,0,1,1)
, diffuseColor_(255, 255, 255, 255)
, saturation_(1.0f)
{
	runtimeTexture_ = 0;
	coordsRemapped_ = false;
}

UI_Sprite::UI_Sprite(const char* filename)
: textureCoords_(0, 0, 1, 1)
, diffuseColor_(255, 255, 255, 255)
, saturation_(1.0f)
{
	coordsRemapped_ = false;
	runtimeTexture_ = UI_Render::instance().createTexture(filename);
}

UI_Sprite::~UI_Sprite()
{
}

void UI_Sprite::release()
{
	if(texture_)
		texture_->releaseTexture();
	else
		UI_Render::instance().releaseTexture(runtimeTexture_);
}

void UI_Sprite::serialize(Archive& ar)
{
	ar.serialize(texture_, "texture_", "&текстура");
	ar.serialize(textureCoords_, "textureCoords_", "текстурные координаты");
	ar.serialize(diffuseColor_, "diffuseColor", "цвет");
	ar.serialize(saturation_, "saturation", "насыщенность");

	if(isUnderEditor()){
		if(ar.isInput() && !coordsRemapped_ && texture_){
			coordsRemapped_ = true;
			if(texture_->needRemapCoords())
				textureCoords_ = texture_->remapCoords(textureCoords_);
		}
	}
}

void UI_Sprite::setTextureReference(const UI_TextureReference& ref)
{
	release();
	texture_ = ref;
}

cTexture* UI_Sprite::texture() const
{
	if(runtimeTexture_)
		return runtimeTexture_;

	if(texture_)
		return texture_->texture();

	return 0;
}

Vect2f UI_Sprite::size() const
{
	return texture() ?
		Vect2f(texture()->GetWidth() * textureCoords().width(),  texture()->GetHeight() * textureCoords().height()) :
		Vect2f::ZERO;
}

float UI_Sprite::phase(float time, bool cycled) const
{
	return isAnimated() ?
			((time*=1000.f) > texture()->GetTotalTime() ? 
				(cycled ? cycle(time, (float)texture()->GetTotalTime()) / texture()->GetTotalTime() : 1.f) :
				time / texture()->GetTotalTime()) :
			0.f;
}

// ------------------- UI_UnitSprite

UI_UnitSprite::UI_UnitSprite() : UI_Sprite(),
spriteOffset_(Vect2f::ZERO)
{
	spriteScale_ = 1.f;
}

UI_UnitSprite::~UI_UnitSprite()
{
}

void UI_UnitSprite::serialize(Archive& ar)
{
	ar.serialize(static_cast<UI_Sprite&>(*this), "sprite", "Спрайт");
	ar.serialize(spriteOffset_, "spriteOffset", "Смещение значка");
	ar.serialize(spriteScale_, "spriteScale", "Масштаб");
}

Vect2f UI_UnitSprite::size() const
{
	Vect2f sz(UI_Sprite::size());
	sz *= spriteScale_;
	return sz;
}

// ------------------- UI_Mask

UI_Mask::UI_Mask()
{
}

UI_Mask::~UI_Mask()
{
}

void UI_Mask::serialize(Archive& ar)
{
	ar.serialize(polygon_, "polygon", "контур");
}

bool UI_Mask::hitTest(const Vect2f& p) const
{
	int intersections_lt = 0;
	int intersections_gt = 0;

	for(int i = 0; i < polygon_.size(); i ++){
		Vect2f p0 = polygon_[i];
		Vect2f p1 = (i < polygon_.size() - 1) ? polygon_[i + 1] : polygon_[0];

		if(p0.y != p1.y){
			if((p0.y < p.y && p1.y >= p.y) || (p0.y >= p.y && p1.y < p.y)){
				if(p0.x < p.x && p1.x < p.x)
					intersections_lt++;
				else if(p0.x > p.x && p1.x > p.x)
					intersections_gt++;
				else {
					float x = (p.y - p0.y) * (p1.x - p0.x) / (p1.y - p0.y) + p0.x;

					if(x > p.x)
						intersections_gt++;
					else
						intersections_lt++;
				}
			}
		}
	}

	return ((intersections_lt & 1) && intersections_gt != 0);
}

void UI_Mask::draw(const Vect2f& base_pos, const sColor4f& color) const
{
	for(int i = 0; i < polygon_.size() - 1; i++)
		UI_Render::instance().drawLine(base_pos + polygon_[i], base_pos + polygon_[i + 1], color);

	UI_Render::instance().drawLine(base_pos + polygon_[polygon_.size() - 1], base_pos + polygon_[0], color);
}

// ------------------- UI_Font

void UI_Font::serialize(Archive& ar)
{
	ar.serialize(fontName_, "fontName_", "&имя");
	ar.serialize(fontSize_, "fontSize_", "&размер");
}

bool UI_Font::createFont(const char* path)
{
	releaseFont();

	if(fontName_.empty())
		return false;

	font_ = UI_Render::instance().createFont(fontName_.c_str(), fontSize_, path);

	return true;
}

bool UI_Font::releaseFont()
{
	if(font_){
		UI_Render::instance().releaseFont(font_);
		font_ = 0;
	}

	return true;
}

// ---------------------- UI_Cursor

UI_Cursor::UI_Cursor() :
	cursor_(NULL),
	fileName_("")
{
}

UI_Cursor::~UI_Cursor()
{
	releaseCursor();
}

void UI_Cursor::serialize(Archive& ar)
{
	static GenericFileSelector::Options options("*.cur", ".\\Resource\\Cursors", "Cursors", true);
	ar.serialize(GenericFileSelector(fileName_, options), "fileName", "Имя файла");
	ar.serialize(effectRef_, "effect", "Эффект");
}

bool UI_Cursor::createCursor(const char* fname/*=*/)
{
	releaseCursor();
	// Обновляем путь к файлу только если он не NULL
	if (NULL != fname) fileName_ = fname;
	if (fileName_.empty()) return true; // Путь пустой => курсор NULL - все как надо

	releaseCursor();
	cursor_ = (HCURSOR)LoadImage(0, fileName_.c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE);

	return (NULL != cursor_);
}

void UI_Cursor::releaseCursor()
{
	if (NULL != cursor_) 
	{
		DestroyCursor(cursor_);
		cursor_ = NULL;
	}
}

// ------------------- UI_TextFormat

UI_TextFormat UI_TextFormat::WHITE(sColor4c(255, 255, 255, 255));

UI_TextFormat::UI_TextFormat()
{
	textColor_ = sColor4c(255, 255, 255, 255);
	blendmode_ = UI_BLEND_NORMAL;
	shadowColor_ = sColor4c(0, 0, 0, 0);
}

UI_TextFormat::UI_TextFormat(const sColor4c& color, const sColor4c& shadow, UI_BlendMode blending)
: textColor_(color)
, shadowColor_(shadow)
, blendmode_(blending)
{}

void UI_TextFormat::serialize(Archive& ar)
{
	ar.serialize(textColor_, "textColor", "Цвет текста");
	ar.serialize(blendmode_, "blendmode", "Режим вывода");
	ar.serialize(shadowColor_, "shadowColor", "Тень");
}

// ------------------- UI_ControlShowMode

void UI_ControlShowMode::ActivateSound::serialize(Archive& ar)
{
	ar.serialize(sourceMode_, "sourceModes", "Из какого переключилось");
	ar.serialize(soundReference_, "soundReference", "Звук активации");
}

UI_ControlShowMode::UI_ControlShowMode()
{
	cycledBgAnimation_ = true;
}

void UI_ControlShowMode::preLoad()
{
	sprite_.texture();
}

void UI_ControlShowMode::release()
{
	sprite_.release();
}

void UI_ControlShowMode::serialize(Archive& ar)
{
	ar.serialize(sprite_, "sprite_", "текстура");
	if(!ar.isEdit() || sprite_.isAnimated())
		ar.serialize(cycledBgAnimation_, "cycledBgAnimation", "Зациклить анимацию");
	ar.serialize(textFormat_, "textFormat", "Параметры текста");
	ar.serialize(activateSounds_, "activateSounds", "Звуки активации");
}

const SoundAttribute* UI_ControlShowMode::sound(UI_ControlShowModeID srcID) const
{
	ActivateSounds::const_iterator it = find(activateSounds_.begin(), activateSounds_.end(), srcID);
	if(it != activateSounds_.end())
		return it->soundReference_;
	return 0;
}

// ------------------- UI_ControlState


UI_ControlState::UI_ControlState(const char* name, bool withDefaultShowMode)
: name_(name)
{
	showModes_.resize(getEnumDescriptor(UI_SHOW_NORMAL).comboStrings().size());
	if(withDefaultShowMode)
		showModes_[UI_SHOW_NORMAL] = new UI_ControlShowMode();
}

void UI_ControlState::preLoad()
{
	ShowModes::iterator it;
	FOR_EACH(showModes_, it)
		if(UI_ControlShowMode* sm = *it)
			sm->preLoad();
}

void UI_ControlState::release()
{
	ShowModes::iterator it;
	FOR_EACH(showModes_, it)
		if(UI_ControlShowMode* sm = *it)
			sm->release();
}

void UI_ControlState::serialize(Archive& ar)
{
	ar.serialize(name_, "name_", "&имя");
	ar.serialize(hoveredText_, "hoveredTextLoc", "Подсказка (Лок)");

	ar.openBlock("", "Режимы отрисовки");
	const EnumDescriptor& descriptor = getEnumDescriptor(UI_SHOW_NORMAL);
	int count = descriptor.comboStrings().size();
	xassert(showModes_.size() == count);
	for(int i = 0; i < count; ++i)
		ar.serialize(showModes_[i], descriptor.name(i), descriptor.nameAlt(i));
	ar.closeBlock();
	ar.serialize(actions_, "actions", "назначения");
}


bool UI_ControlState::operator == (const UI_DataStateMark& mark) const
{
	UI_ControlActionList::const_iterator it;
	FOR_EACH(actions_, it)
		if(it->key() == UI_ACTION_STATE_MARK 
			&& safe_cast<const UI_DataStateMark*>(it->type())->type() == mark.type()
			&& safe_cast<const UI_DataStateMark*>(it->type())->enumValue() == mark.enumValue())
			return true;
	return false;
}

// ------------------- UI_ShowModeSprite

void UI_ShowModeSprite::StateSprite::serialize(Archive& ar)
{
	ar.serialize(mode_, "mode", "&Режим кнопки");
	ar.serialize(sprite_, "sprite", "Спрайт");
}

void UI_ShowModeSprite::serialize(Archive& ar)
{
	StringTableBase::serialize(ar);
	ar.serialize(sprites_, "sprites", "Спрайты");
}

// ------------------- UI_Transform

const UI_Transform UI_Transform::ID;

UI_Transform::UI_Transform(float time, bool reversed) :
	type_(0),
	workTime_(time),
	trans_(0,0),
	scale_(1,1),
	scaleMode_(SCALE_CENTER),
	alpha_(1.f),
	phase_(0),
	reversed_(reversed)
{
	if(workTime_ < FLT_EPS)
		phase_ = 1.f;
}

void UI_Transform::clear()
{
	*this = ID;
}

void UI_Transform::quant(float dt)
{
	if(workTime_ > FLT_EPS)
		phase_ = clamp(phase_ + dt/workTime_, 0.f, 1.f);
	else
		phase_ = 1.f;
}

void UI_Transform::apply(UI_ControlBase* control, bool append)
{
	if(checkType(UI_Transform::TRANSFORM_COORDS))
		control->transfPosition_ = (append ? control->transfPosition_ : control->position_) + trans() * phase();
	else if(!append)
		control->transfPosition_ = control->position_;

	if(checkType(UI_Transform::TRANSFORM_SCALE)){
		Vect2f center = control->transfPosition_.center();
		Vect2f scal = append ? Vect2f::ID + (scale() - Vect2f::ID) * phase() : scale() * (1.f - phase());

		if(!checkType(UI_Transform::TRANSFORM_SCALE_X))
			scal.x = 1.f;
		if(!checkType(UI_Transform::TRANSFORM_SCALE_Y))
			scal.y = 1.f;

		control->transfPosition_ = control->transfPosition_.scaled(scal, center);

		if(!append){
			switch(scaleMode_){
				case SCALE_LEFT:
					control->transfPosition_.left(control->transfPosition_.left() - (control->position_.width() - control->transfPosition_.width()) / 2.f);
					break;
				case SCALE_TOP:
					control->transfPosition_.top(control->transfPosition_.top() - (control->position_.height() - control->transfPosition_.height()) / 2.f);
					break;
				case SCALE_RIGHT:
					control->transfPosition_.left(control->transfPosition_.left() + (control->position_.width() - control->transfPosition_.width()) / 2.f);
					break;
				case SCALE_BOTTOM:
					control->transfPosition_.top(control->transfPosition_.top() + (control->position_.height() - control->transfPosition_.height()) / 2.f);
					break;
			}
		}

		if(append)
			control->isScaled_ |= !isFinished();
		else
			control->isScaled_  = !isFinished();
	}

	if(checkType(UI_Transform::TRANSFORM_ALPHA)){
		if(append)
			control->alpha_ *= alpha() * phase();
		else
			control->alpha_  = 1.f - alpha() * phase();
	}
	else {
		if(!append)
			control->alpha_ = 1.f;
	}
}

void UI_Transform::getDebugString(XBuffer& out) const
{
	out < "type: " < getEnumDescriptor(Type()).nameCombination(type_).c_str()
		< (reversed_ ? "  Reversed, " : "  ")
		< ", worktime:" <= workTime_ < ", phase: " <= phase()
		< (isFinished() ? "" : ", (in work)");
}

// ------------------- UI_MessageSetup

UI_MessageSetup::UI_MessageSetup()
{
	syncroBySound_ = false;
	displayTime_ = 0.f;
	isVoiceInterruptable_ = false;
	enableVoice_ = true;
	isInterruptOnDestroy_ = true;
	isCanInterruptVoice_ = true;
	isCanPaused_ = true;
	isPlayVoiceAlways_ = false;
}

float UI_MessageSetup::displayTime() const
{
	if(syncroBySound_){
		if(const UI_Text* txt = textReference_)
			return txt->voice().duration();
	}

	return displayTime_;
}

const char* UI_MessageSetup::textData() const
{
	if(const UI_Text* txt = textReference_)
		return txt->text();

	return 0;
}

bool UI_MessageSetup::playVoice(bool interrupt) const
{
	if((!voiceManager.enabled() && !isPlayVoiceAlways_) || (!interrupt && voiceManager.isPlaying()))
		return false;

	if(const UI_Text* txt = textReference_){
		const VoiceAttribute& v = txt->voice();
		if(!v.isEmpty() && enableVoice_){
			v.getNextVoiceFile();
			return voiceManager.Play(v.voiceFile(), false, isCanPaused_, isPlayVoiceAlways_);
		}
	}

	return false;
}

void UI_MessageSetup::stopVoice() const
{
	if(!voiceManager.enabled() && !isPlayVoiceAlways_)
		return;

	voiceManager.Stop();
}

float UI_MessageSetup::voiceDuration() const
{
	if(const UI_Text* txt = textReference_){
		const VoiceAttribute& v = txt->voice();
		if(!v.isEmpty())
			return v.duration();
	}

	return 0.f;
}

void UI_MessageSetup::serialize(Archive& ar)
{
	ar.serialize(messageType_, "messageType", "Тип сообщения");
	ar.serialize(textReference_, "textReference", "Сообщение");
	ar.serialize(syncroBySound_, "syncroBySound", "Синхронизировать со звуком");
	ar.serialize(isVoiceInterruptable_, "isVoiceInterruptable", "Можно прервать голос");
	ar.serialize(isInterruptOnDestroy_, "isInterruptOnDestroy", "Прерывать при разрушении мира");
	ar.serialize(isCanInterruptVoice_, "isCanInterruptVoice", "НЕ Ждать окончания играемого сообщения");
	ar.serialize(isCanPaused_, "isCanPaused", "Ставить на паузу во время паузы");
	ar.serialize(isPlayVoiceAlways_, "isPlayVoiceAlways", "Проигрывать звук всегда");
    if(!syncroBySound_){
		ar.serialize(displayTime_, "displayTime", "Время показа, секунды");
		ar.serialize(enableVoice_, "enableVoice", "Проигрывать звук");
	}		
}

// ------------------- UI_Message

UI_Message::UI_Message()
{
}

UI_Message::UI_Message(const UI_MessageSetup& message_setup, const char* custom_text) : setup_(message_setup)
{
	if(custom_text && *custom_text)
		customText_ = custom_text;

	start();
}

const char* UI_Message::text() const
{
	if(!customText_.empty())
		return customText_.c_str();

	return setup_.textData();
}

void UI_Message::setCustomText(const char* text)
{
	if(text && *text)
		customText_ = text;
	else
		customText_.clear();
}

void UI_Message::start()
{
	if(!setup_.isEmpty()){
		float time = setup_.displayTime();
		if(time > FLT_EPS)
			aliveTime_.start(time * 1000);
		else
			aliveTime_.start(10000000.f);
	}
}

void UI_Message::serialize(Archive& ar)
{
	ar.serialize(aliveTime_, "aliveTimer", 0);
	ar.serialize(customText_, "customText", 0);
	ar.serialize(setup_, "setup", 0);
}

// -------------------

void UI_TaskColor::serialize(Archive& ar)
{
	ar.serialize(color, "color", "Цвет задачи");
	ar.serialize(tag, "tag", "Пометка задачи");
}

// ------------------- UI_Task

UI_TaskColor UI_Task::taskColors_[UI_TASK_STATE_COUNT * 4];

void UI_Task::serializeColors(Archive& ar)
{
	if(ar.isEdit()){
		ar.openBlock("taskColors", "Цвет текста задач");
		int i;
		for(i = 0; i < UI_TASK_STATE_COUNT; i++)
			ar.serialize(taskColors_[i], getEnumName(UI_TaskStateID(i)), getEnumNameAlt(UI_TaskStateID(i)));
		ar.closeBlock();

		ar.openBlock("secondaryTaskColors", "Цвет текста второстепенных задач");
		for(i = 0; i < UI_TASK_STATE_COUNT; i++)
			ar.serialize(taskColors_[i + UI_TASK_STATE_COUNT], getEnumName(UI_TaskStateID(i)), getEnumNameAlt(UI_TaskStateID(i)));
		ar.closeBlock();

		ar.openBlock("taskColorsOnScreen", "Цвет текста задач в списке сообщений");
		for(i = 0; i < UI_TASK_STATE_COUNT; i++)
			ar.serialize(taskColors_[i + UI_TASK_STATE_COUNT * 2], getEnumName(UI_TaskStateID(i)), getEnumNameAlt(UI_TaskStateID(i)));
		ar.closeBlock();

		ar.openBlock("secondaryTaskColorsOnScreen", "Цвет текста второстепенных задач в списке сообщений");
		for(i = 0; i < UI_TASK_STATE_COUNT; i++)
			ar.serialize(taskColors_[i + UI_TASK_STATE_COUNT * 3], getEnumName(UI_TaskStateID(i)), getEnumNameAlt(UI_TaskStateID(i)));
		ar.closeBlock();
	}
	else
		ar.serializeArray(taskColors_, "taskColors", 0);
}

void UI_Task::serialize(Archive& ar)
{
	ar.serialize(state_, "state", 0);
	ar.serialize(setup_, "setup", 0);
	ar.serialize(isSecondary_, "isSecondary", 0);
}

const char* getColorString(const sColor4f& color);

bool UI_Task::getText(string& out, bool message_mode) const
{
	if(state_ == UI_TASK_DELETED)
		return false;

	if(const char* message = setup_.textData()){
		out.clear();

		int col_idx = int(state_);
		if(isSecondary_)
			col_idx += UI_TASK_STATE_COUNT;

		if(message_mode)
			col_idx += UI_TASK_STATE_COUNT * 2;

		out += getColorString(taskColors_[col_idx].color);
		
		if(!taskColors_[col_idx].tag.empty())
			out += taskColors_[col_idx].tag.c_str();

		out += message;
		
		return true;
	}
	return false;
}

// --------------------- UI_ControlHotKey

bool UI_ControlHotKey::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	if(ar.isEdit())
		return ar.serialize(hotKey, ref.referenceString(), ref.referenceString());
	return true;
}

// --------------------- AtomAction

void AtomAction::serialize(Archive& ar)
{
	ar.serialize(type_, "type", "Что сделать");
	ar.serialize(control_, "control", "С чем сделать");
}

void AtomAction::apply() const
{
	if(UI_ControlBase* ctrl = control_.control()){
		switch(type_){
		case ENABLE_SHOW:
			ctrl->showByTrigger();
			break;
		case DISABLE_SHOW:
			ctrl->hideByTrigger();
			break;
		case SHOW:
			if(!ctrl->isVisible())
				ctrl->show();
			break;
		case HIDE:
			if(ctrl->isVisible())
				ctrl->hide();
			break;
		case ENABLE:
			if(!ctrl->isEnabled())
				ctrl->enable();
			break;
		case DISABLE:
			if(ctrl->isEnabled())
				ctrl->disable();
			break;
		case VIDEO_PLAY:
			if(UI_ControlVideo* vid = dynamic_cast<UI_ControlVideo*>(ctrl))
				vid->play();
			break;
		case VIDEO_PAUSE:
			if(UI_ControlVideo* vid = dynamic_cast<UI_ControlVideo*>(ctrl))
				vid->pause();
			break;
		case SET_FOCUS:
			ctrl->activate();
			break;
		default:
			xassert(false && "забыли добавить case AtomAction::apply()");
		}
	}
}

bool AtomAction::workedOut() const
{
	if(const UI_ControlBase* ctrl = control_.control())
		switch(type_){
		case ENABLE_SHOW:
		case SHOW:
		case DISABLE_SHOW:
		case HIDE:
			if(!UI_Dispatcher::instance().isActive(ctrl->screen()))
				return true;
			return !ctrl->isChangeProcess();
		}
	return true;
}

// --------------------- ShowStatisticType

void ShowStatisticType::serialize(Archive& ar)
{
	ar.serialize(type, "type", "Тип значения");
	if(type == STAT_VALUE)
		ar.serialize(statValueType, "statValueType", "Параметр");
}

void ShowStatisticType::getValue(XBuffer& buf, const StatisticsEntry& val) const
{
	switch(type){
	case POSITION:
		buf <= val.position;
		break;
	case NAME:
		buf < val.name.c_str();
		break;
	case RATING:
		buf <= round(double(val.rating));
		break;
	case STAT_VALUE:
		buf <= val[statValueType];
		break;
	default:
		break;
	}
}