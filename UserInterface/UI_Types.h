#ifndef __UI_TYPES_H__
#define __UI_TYPES_H__

#include "TypeLibrary.h"
#include "SwapVector.h"
#include "..\Util\LocString.h"

#include "UI_Enums.h"
#include "UI_References.h"
#include "UI_BackgroundScene.h"

#include "GenericFileSelector.h"
#include "Rect.h"
#include "..\Render\inc\UMath.h"

#include "EffectReference.h"
#include "..\Units\EffectController.h"
#include "..\Game\GlobalStatistics.h"

#include "Controls.h"
#include "CallWrapper.h"

class AttributeBase;
class ParameterSet;
struct ProducedParameters;

class cFont;
class cTexture;

class UI_Screen;
class UI_ControlBase;

typedef unsigned short int ActionMode;
typedef unsigned char ActionModeModifer;

template<ActionMode controlMode>
class UI_ActionDataBase;

bool isUnderEditor();

#ifndef _FINAL_VERSION_
#define LOG_UI_STATE(str) logUIState(__LINE__, __FUNCSIG__, this, str)
#define UI_NAME_BREAK(control_name) xassert(strcmp(name(), control_name))
#else
#define LOG_UI_STATE(str)
#define UI_NAME_BREAK(control_name)
#endif


/// Данные к интерфейсной команде, базовый класс.
class UI_ActionData : public PolymorphicBase
{
public:
	UI_ActionData() : actionModeTypes_(0), actionModeModifers_(0) { }
	UI_ActionData(ActionMode mode) : actionModeTypes_(mode), actionModeModifers_(0) { }
	virtual ~UI_ActionData(){ };

	virtual void serialize(Archive& ar);

	bool needUpdate() const { return actionModeTypes_ & UI_ACTION_TYPE_UPDATE; }
	bool needExecute(ActionMode clicks, ActionModeModifer modifer) const {
		if(clicks & UI_ACTION_SELF_EXECUTE)
			return true;
		else if(!(clicks & UI_ACTION_HOTKEY) && modifer != actionModeModifers_)
			return false;
		return actionModeTypes_ & clicks & (UI_USER_ACTION_MASK | UI_ACTION_FOCUS_OFF);
	}
private:
	template<ActionMode controlMode>
	friend class UI_ActionDataBase;
	ActionMode actionModeTypes_;
	ActionModeModifer actionModeModifers_;
};

template<ActionMode controlMode>
class UI_ActionDataBase : public UI_ActionData
{
public:
	UI_ActionDataBase() : UI_ActionData(controlMode) {}
	void serialize(Archive& ar);
};

typedef UI_ActionDataBase<0> UI_ActionDataInstrumentary;
typedef UI_ActionDataBase<UI_ACTION_TYPE_UPDATE> UI_ActionDataUpdate;
typedef UI_ActionDataBase<UI_ACTION_MOUSE_LB> UI_ActionDataAction;
typedef UI_ActionDataBase<UI_ACTION_FOCUS_OFF> UI_ActionDataEdit;
typedef UI_ActionDataBase<UI_ACTION_TYPE_UPDATE | UI_ACTION_MOUSE_LB> UI_ActionDataFull;

class UI_ActionDataControlCommand : public UI_ActionDataInstrumentary
{
public:
	enum ControlCommand {
		NONE, // ничего не делать
		RE_INIT, // вызвать повторную инициализацию
		CLEAR, // очистить значение контрола
		EXECUTE, // выполнить назначение
		GET_CURRENT_SAVE_NAME, // скопировать в заголовок контрола имя текущей миссии
		GET_CURRENT_PROFILE_NAME,  // скопировать в заголовок контрола имя текущего профайла
		GET_CURRENT_CDKEY,  // скопировать в заголовок контрола текущий cd-key
		UPDATE_HOTKEY, // обновить hotkey, разрешает конфликты при назначении одной и той же кнопки на разные действия
		UPDATE_COMPATIBLE_HOTKEYS // обновить hotkey'и, совместимые с последним назначенным
	};
	
	UI_ActionDataControlCommand(UI_ControlActionID target,  ControlCommand command) {
		target_ = target;
		command_ = command; }

	UI_ControlActionID target() const { return target_; }
	ControlCommand command() const { return command_; }

private:
	ControlCommand command_;
	UI_ControlActionID target_;
};


class UI_DataStateMark : public UI_ActionDataInstrumentary
{
	StateMarkType type_;
	int enumValue_;

public:

	UI_DataStateMark() : type_(UI_STATE_MARK_NONE), enumValue_(0) {}
	UI_DataStateMark(StateMarkType _type, int value) : type_(_type), enumValue_(value) {}

	StateMarkType type() const { return type_; }
	int enumValue() const { return enumValue_; }

	void serialize(Archive& ar);

};

/// Фабрика объектов, создающая данные к интерфейсным командам.
class UI_ActionDataFactory : public Factory<UI_ControlActionID, UI_ActionData>
{
public:
	UI_ActionDataFactory();

	static UI_ActionDataFactory& instance(){ return Singleton<UI_ActionDataFactory>::instance(); }
};

typedef EnumToClassSerializer<UI_ControlActionID, UI_ActionData, UI_ACTION_NONE, UI_ActionDataFactory> UI_ControlAction;
typedef std::vector<UI_ControlAction> UI_ControlActionList;

/// Ввод с клавиатуры и мыши.
class UI_InputEvent
{
public:
	UI_InputEvent(UI_InputEventID id = UI_INPUT_MOUSE_MOVE, const Vect2f& pos = Vect2f(0,0), int flags = -1) : ID_(id) ,
		cursorPos_(pos),
		keyCode_(0),
		charInput_(0),
		flags_(flags)
	{
	}

	UI_InputEventID ID() const { return ID_; }
	void setID(UI_InputEventID id){ ID_ = id; }

	const Vect2f& cursorPos() const { return cursorPos_; }
	void setCursorPos(const Vect2f& pos){ cursorPos_ = pos; }

	int keyCode() const { return keyCode_; }
	void setKeyCode(int key){ keyCode_ = key; }

	unsigned char charInput() const { return charInput_; }
	void setCharInput(unsigned char chr){ charInput_ = chr; }

	int flags() const { return flags_; }
	void setFlags(int flags){ flags_ = flags; }

	bool isMouseEvent() const { return (ID_ <= UI_ACTION_LAST_MOUSE_EVENT); }
	bool isMouseClickEvent() const { return (ID_ <= UI_ACTION_LAST_MOUSE_CLICK_EVENT); }

private:

	UI_InputEventID ID_;
	Vect2f cursorPos_;

	int keyCode_;
	unsigned char charInput_;

	int flags_;
};

/// текстура
class UI_Texture : public PolymorphicBase
{
public:
	UI_Texture(const char* fileName = "", bool hasResolutionVersion = true, bool hasRaceVersion = false) : texture_(0)
	{
		localized_ = false;
		setFileName(fileName);
		hasResolutionVersion_ = hasResolutionVersion;
		hasRaceVersion_ = hasRaceVersion;

		textureSize_ = Vect2i(0,0);
		textureSizeCurrent_ = Vect2i(0,0);

		textureSizeInit_ = false;
	}
	
	UI_Texture(const UI_Texture& src) : texture_(0)
	{
		fileName_ = src.fileName_;
		localized_ = src.localized_;
		hasResolutionVersion_ = src.hasResolutionVersion_;
		hasRaceVersion_ = src.hasRaceVersion_;

		textureSize_ = src.textureSize_;
		textureSizeCurrent_ = src.textureSizeCurrent_;

		textureSizeInit_ = src.textureSizeInit_;
	}
	
	~UI_Texture()
	{
		releaseTexture();
	}

	void serialize(Archive& ar);

	bool isEmpty() const { return fileName_.empty(); }

	const char* textureFileName() const;

	bool createTexture() const;
	bool releaseTexture() const;

	/// текстура локализована
	bool localized() const { return localized_; }
	/// менялись ли пропорции текстуры
	bool needRemapCoords() const;
	/// пересчёт координат после изменения пропорций текстуры
	Rectf remapCoords(const Rectf& coords) const;
	void initSize();

	cTexture* texture() const;

private:
	void buildLocPath(const char* in, string& out);
	void updateFileNameProxy();
	void setFileName(const char* name);
	string fileName_;
	string fileNameProxy_;
	bool localized_;

	bool hasResolutionVersion_;
	bool hasRaceVersion_;

	/// запомненный размер текстуры, сериализуется
	Vect2i textureSize_;
	/// вычисленный при загрузке размер текстуры
	Vect2i textureSizeCurrent_;
	bool textureSizeInit_;

	mutable cTexture* texture_;
};

typedef StringTable<StringTableBasePolymorphic<UI_Texture> > UI_TextureLibrary;
typedef StringTablePolymorphicReference<UI_Texture, false> UI_TextureReference;

/// спрайт
class UI_Sprite
{
public:
	UI_Sprite();
	explicit UI_Sprite(const char* filename);
	~UI_Sprite();

	void serialize(Archive& ar);

	bool operator == (const UI_Sprite& obj) const { return texture_ == obj.texture_ && textureCoords_ == obj.textureCoords_; }

	bool isEmpty() const { return !texture(); }
	bool isAnimated() const { return isEmpty() ? false : texture()->GetNumberFrame() > 1; }
	// передается текущее время в секундах с начала проигрывания - вычисляется фаза
	float phase(float time, bool cycled) const;

	cTexture* texture() const;
	const Rectf& textureCoords() const { return textureCoords_; }

	Vect2f size() const;

	void setTextureReference(const UI_TextureReference& ref);
	void setTextureCoords(const Rectf& coords) { textureCoords_ = coords; }
	const UI_TextureReference& textureReference() const { return texture_; }

	void setSaturation(float saturation) { saturation_ = saturation; }
	float saturation() const{ return saturation_; }
	sColor4c diffuseColor() const{ return diffuseColor_; }
	void setDiffuseColor(sColor4c color){ diffuseColor_ = color; }

	static const UI_Sprite ZERO;

	void release();

private:

	UI_TextureReference texture_;
	sColor4c diffuseColor_;
	float saturation_;
	Rectf textureCoords_;
	mutable cTexture* runtimeTexture_;

	bool coordsRemapped_;
};

class UI_LibSprite : public UI_Sprite, public StringTableBase {
public:
	UI_LibSprite(const char* name = "") : StringTableBase(name) {}
	void serialize(Archive& ar){
		StringTableBase::serialize(ar);
		ar.serialize(static_cast<UI_Sprite&>(*this), "sprite", "Спрайт");
	}
};

typedef StringTable<UI_LibSprite> UI_SpriteLibrary;
typedef StringTableReference<UI_LibSprite, false> UI_SpriteReference;

class UI_UnitSprite : public UI_Sprite
{
public:
	UI_UnitSprite();
	~UI_UnitSprite();

	void serialize(Archive& ar);

	Vect2f size() const;
	void draw(const Vect3f& pos, float zShift = 0.f, const Vect2f& slot = Vect2f(0.f, 0.f)) const;

	const Vect2f& offset() const { return spriteOffset_; }
	float scale() const { return spriteScale_; }

private:

	Vect2f spriteOffset_;
	float spriteScale_;
};

class UI_Mask
{
public:
	
	UI_Mask();
	~UI_Mask();

	void serialize(Archive& ar);

	/// Возвращает true, если точка с координатами \a pos попадает внутрь маски.
	bool hitTest(const Vect2f& pos) const;

	typedef std::vector<Vect2f> Polygon;	
	const Polygon& polygon() const { return polygon_; }
	Polygon& polygon() { return polygon_; }
	void setPolygon(const Polygon& polygon){ polygon_ = polygon; }

	bool isEmpty() const { return polygon_.empty(); }

	void draw(const Vect2f& base_pos, const sColor4f& color) const;

private:

	/// Полигон, точки в относительных экранных координатах.
	Polygon polygon_;
};

class UI_ControlShowMode;
class UI_TextFormat
{
public:
	UI_TextFormat();
	explicit UI_TextFormat(const sColor4c& color, const sColor4c& shadow = sColor4c(0, 0, 0, 0), UI_BlendMode blending = UI_BLEND_NORMAL);

	UI_BlendMode blendMode() const { return blendmode_; }

	const sColor4c& textColor() const { return textColor_; }
	const sColor4c& shadowColor() const { return shadowColor_; }

	void serialize(Archive& ar);

	static UI_TextFormat WHITE;

private:
	sColor4c textColor_;
	sColor4c shadowColor_;
	UI_BlendMode blendmode_;
};

class UI_ControlShowMode : public PolymorphicBase
{
	struct ActivateSound
	{
		ActivateSound() : sourceMode_(UI_SHOW_NORMAL) {}
		/// звук, звучащий при включении этого режима из указанного
		SoundReference soundReference_;
		UI_ControlShowModeID sourceMode_;
		
		bool operator== (UI_ControlShowModeID srcID) const { return sourceMode_ == srcID; }
		void serialize(Archive& ar);
	};
	typedef vector<ActivateSound> ActivateSounds;

public:
	UI_ControlShowMode();

	void preLoad();
	void release();

	void serialize(Archive& ar);

	const UI_Sprite& sprite() const { return sprite_; }
	bool cycledAnimation() const { return cycledBgAnimation_; }
	void setSprite(const UI_Sprite& _sprite) { MTG(); sprite_ = _sprite; }

	const UI_TextFormat& textFormat() const { return textFormat_; }

	const SoundAttribute* sound(UI_ControlShowModeID srcID) const;

private:
	UI_Sprite sprite_;
	bool cycledBgAnimation_;
	UI_TextFormat textFormat_;
	ActivateSounds activateSounds_;
};

//=======================================================
struct UI_ShowModeSprite : StringTableBase
{
	struct StateSprite{
		StateSprite() : mode_(UI_SHOW_NORMAL) {}
		UI_ControlShowModeID mode_;
		UI_Sprite sprite_;
		void serialize(Archive& ar);
	};
	typedef vector<StateSprite> StateSprites;

	UI_ShowModeSprite(const char* name = "") : StringTableBase(name) {}

	void serialize(Archive& ar);

	const UI_Sprite& sprite(UI_ControlShowModeID id) const
	{
		if(sprites_.empty())
			return UI_Sprite::ZERO;
		
		StateSprites::const_iterator it;
		FOR_EACH(sprites_, it)
			if(it->mode_ == id)
				return it->sprite_;
		
		return sprites_.front().sprite_;
	}

	const StateSprites& sprites() const { return sprites_; }
	
private:
	StateSprites sprites_;
};

typedef StringTable<UI_ShowModeSprite> UI_ShowModeSpriteTable;
typedef StringTableReference<UI_ShowModeSprite, true> UI_ShowModeSpriteReference;


/// шрифт
class UI_Font : public PolymorphicBase
{
public:
	UI_Font() : font_(0), fontSize_(20) { }

	bool isEmpty() const { return (fontName_.empty() || !font_); }

	void serialize(Archive& ar);

	cFont* font() const { return font_; }

	bool createFont(const char* path = 0);
	bool releaseFont();

	int size() const { return fontSize_; }

	void setSize(int size){ fontSize_ = size; }
	void setFontName(const char* name){ fontName_ = name; }

	static void setFontDirectory(const char* path);

private:
	int fontSize_;

	std::string fontName_;

	mutable cFont* font_;

	static std::string fontDirectory_;
};

typedef StringTable<StringTableBasePolymorphic<UI_Font> > UI_FontLibrary;
typedef StringTablePolymorphicReference<UI_Font, false> UI_FontReference;

/// курсор
class UI_Cursor : public PolymorphicBase
{
public:
	UI_Cursor();
	~UI_Cursor();

	void serialize(Archive& ar);

	const HCURSOR cursor() const { return cursor_; }
	bool createCursor(const char* fname = NULL);
	void releaseCursor();

	const EffectReference& effectRef() const { return effectRef_; }
private:
	string fileName_;
	HCURSOR cursor_;
	EffectReference effectRef_;
};

typedef StringTable<StringTableBasePolymorphic<UI_Cursor> > UI_CursorLibrary;
typedef StringTablePolymorphicReference<UI_Cursor, false> UI_CursorReference;

/// Состояние элемента интерфейса.
class UI_ControlState
{
public:
	UI_ControlState(const char* name = "", bool withDefaultShowMode = false);

	bool operator == (const char* name_str) const { return !strcmp(name(), name_str); }

	bool operator == (const UI_DataStateMark& mark) const;

	void preLoad();
	void release();

	void serialize(Archive& ar);

	const char* name() const { return name_.c_str(); }
	void setName (const char* name) { name_ = name; }

	const UI_ControlShowMode* showMode(UI_ControlShowModeID mode_id) const
	{
        if(showModes_[mode_id])
            return showModes_[mode_id];
        else 
            return showModes_[UI_SHOW_NORMAL];
	}

	UI_ControlShowMode* showMode(UI_ControlShowModeID mode_id)
	{
		if(showModes_[mode_id])
			return showModes_[mode_id];
		else 
			return showModes_[UI_SHOW_NORMAL];
	}

	int actionCount() const { return actions_.size(); }
	UI_ControlActionID actionID(int index) const {
		xassert(index >= 0 && index < actions_.size());
		return actions_[index].key();
	}

	const UI_ActionData* actionData(int index) const {
		xassert(index >= 0 && index < actions_.size());
		return actions_[index].type();
	}

	const char* hint() const { return hoveredText_.c_str();	}

private:

	string name_;
	LocString hoveredText_;

	UI_ControlActionList actions_;

	typedef std::vector<ShareHandle<UI_ControlShowMode> > ShowModes;
	ShowModes showModes_;
};

/// Хранилище параметров трансформации
class UI_Transform
{
public:
	UI_Transform(float time = 0.f, bool reversed = false);

	/// возможные трансформации кнопки
	enum Type
	{
		TRANSFORM_COORDS	= 1,	///< перемещение
		TRANSFORM_SCALE_X	= 2,	///< масштабирование по горизонтали
		TRANSFORM_SCALE_Y	= 4,	///< масштабирование по вертикали
		TRANSFORM_ALPHA		= 8,	///< изменение прозрачности

		TRANSFORM_SCALE		= TRANSFORM_SCALE_X | TRANSFORM_SCALE_Y ///< масштабирование по двум осям
	};

	/// режимы масштабирования
	enum ScaleMode
	{
		SCALE_CENTER,		///< от центра
		SCALE_LEFT,			///< от левого края
		SCALE_TOP,			///< от верхнего края
		SCALE_RIGHT,		///< от правого края
		SCALE_BOTTOM		///< от нижнего края
	};

	void start(){ phase_ = (workTime_ < FLT_EPS) ? 1.f : 0.f; }
	bool isFinished() const { return (workTime_ < FLT_EPS || fabs(phase_ - 1.f) < FLT_EPS); }
	void clear();

	bool checkType(Type tp) const { return (type_ & tp) != 0; }

	bool isReversed() const { return reversed_; }

	const Vect2f& trans() const { return trans_; }
	void setTrans(const Vect2f& trans){ type_ |= TRANSFORM_COORDS; trans_ = trans; }
	void clearTrans(){ type_ &= ~TRANSFORM_COORDS; }

	const Vect2f& scale() const { return scale_; }
	void setScale(const Vect2f& scale){ type_ |= TRANSFORM_SCALE; scale_ = scale; }
	void setScaleX(float scale){ type_ |= TRANSFORM_SCALE_X; scale_.x = scale; }
	void setScaleY(float scale){ type_ |= TRANSFORM_SCALE_Y; scale_.y = scale; }
	void clearScale(){ type_ &= ~TRANSFORM_SCALE; }

	void setScaleMode(ScaleMode mode){ scaleMode_ = mode; }
	ScaleMode scaleMode() const { return scaleMode_; }

	float alpha() const { return alpha_; }
	void setAlpha(float alpha){ type_ |= TRANSFORM_ALPHA; alpha_ = alpha; }
	void clearAlpha(){ type_ &= ~TRANSFORM_ALPHA; }

	float phase() const { return (reversed_) ? (1.f - phase_) : phase_; }
	void setPhase(float phase){ phase_ = phase; }

	void quant(float dt);

	void apply(UI_ControlBase* control, bool append = false);

	void getDebugString(XBuffer& out) const;

	static const UI_Transform ID;

private:

	BitVector<Type> type_;

	/// Время действия трансформации
	float workTime_;

	/// Текущая фаза трансформации
	float phase_; 

	bool reversed_;

	/// Изменение координат
	Vect2f trans_;
	/// Изменение масштаба
	Vect2f scale_;
	/// Режим масштабирования
	ScaleMode scaleMode_;
	/// Изменение прозрачности
	float alpha_;
};

class UI_ControlContainer
{
public:
	UI_ControlContainer() : owner_(0) { controlsSorted_ = false; };

	virtual ~UI_ControlContainer(){ };

	virtual void serialize(Archive& ar);
	virtual void init();

	const char* name() const { return name_.c_str(); }
	void setName (const char* name) { name_ = name; }
	bool hasName() const { return !name_.empty(); }

	void referenceString(string& out) const;

	virtual bool visibleInEditor () const { return true; }

	virtual bool addControl(UI_ControlBase* p){ addControlToList(p); return true; }
	virtual bool removeControl(UI_ControlBase* p);

	virtual void preLoad();
	virtual void release();
	
	virtual bool redraw() const;
	virtual void drawDebug2D() const;

	typedef std::list<ShareHandle<UI_ControlBase> > ControlList;
	const ControlList& controlList() const { return controls_; }

	const UI_ControlContainer* owner() const { return owner_; }

	UI_ControlBase* getControlByName(const char* control_name) const;

	void sortControls();

	static unsigned int changeIndex(){ return changeIndex_; }
	static void toggleChange(){ changeIndex_++; }

	void setOwner(UI_ControlContainer* owner){ owner_ = owner; }
	void updateControlOwners();

protected:

	ControlList controls_;
	bool controlsSorted_;

	void clearControlList(){ controls_.clear(); }
	bool addControlToList(UI_ControlBase* p);
	bool removeControlFromList(UI_ControlBase* p);

	UI_ControlBase* operator[](int idx) const {
		if(controls_.empty()) return 0;

		ControlList::const_iterator it = controls_.begin();
		while(idx--){
			if(++it == controls_.end())
				return 0;
		}

		return *it;
	}

private:

	string name_;
	UI_ControlContainer* owner_;

	static unsigned int changeIndex_;
};

/// Фильтр для построения выпадающего списка при редактировании ссылок на контролы.
typedef bool (*UI_ControlFilterFunc)(const UI_ControlBase* p);

/// сообщение контролу
struct ControlMessage
{
	ControlMessage(UI_ControlActionID id, const UI_ActionData* data): id_(id), data_(data){}
	UI_ControlActionID id_;
	const UI_ActionData* data_;
};

struct ControlState
{
	bool show_control;
	bool hide_control;
	bool enable_control;
	bool disable_control;

	ControlState(){
		show_control = false;
		hide_control = false;
		enable_control = false;
		disable_control = false;
	}

	void hide() { hide_control = true; }
	void show() { show_control = true; }
	void enable() { enable_control = true; }
	void disable() { disable_control = true; }

	void apply(Accessibility var){
		if(var){
			show();
			if(var == CAN_START)
				enable();
			else
				disable();
		}
		else
			hide();
	}

	void setPriority(bool prior){
		if(prior){
			if(hide_control)
				show_control = false;
			if(enable_control)
				disable_control = false;
		}
	}
};

struct UI_ControlHotKey
{
	UI_ControlReference ref;
	sKey hotKey;
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
};

/// Базовый класс для элементов интерфейса.
class UI_ControlBase : public UI_ControlContainer, public PolymorphicBase
{
public:
	/// сдвиг контрола по горизонтали при подстройке размера
	enum ResizeShiftHorizontal {
		RESIZE_SHIFT_LEFT,
		RESIZE_SHIFT_CENTER_H,
		RESIZE_SHIFT_RIGHT
	};
	/// сдвиг контрола по вертикали при подстройке размера
	enum ResizeShiftVertical {
		RESIZE_SHIFT_UP,
		RESIZE_SHIFT_CENTER_V,
		RESIZE_SHIFT_DOWN
	};

	UI_ControlBase();
	~UI_ControlBase();

	bool operator < (const UI_ControlBase& control) const { return screenZ_ < control.screenZ_; }

	virtual void serialize(Archive& ar);
	/// вызывается при входе на экран
	virtual void init();
	/// прососать ресурсы до активации экрана
	void preLoad();
	/// вызывается при уходе с экрана
	void release();
	
	const UI_Screen* screen() const;

	sKey hotKey() const { return hotKey_; }
	void setHotKey(sKey hotKey); 

	void setVisibleInEditor (bool visible);
	bool visibleInEditor () const {	return visibleInEditor_; }

	UI_ControlBase* findControl(const Vect2f& pos) const;

	/// прячет саму кнопку и все дочерние кнопки
	void hide(bool immediately = false);
	/// прячет дочерние кнопки
	virtual void hideChildControls(bool immediately = false);
	/// показывает саму кнопку и все дочерние кнопки
	void show();
	/// показывает дочерние кнопки
	virtual void showChildControls();

	/// управление анимацией фона
	void startAnimation(PlayControlAction action, bool recursive = true);

	/// запрещает отображение из тригера
	void hideByTrigger();
	/// разрешает отображение из тригера
	void showByTrigger();

	virtual void onFocus(bool focus_state){ }

	bool isVisible() const { return isVisible_ && isVisibleByTrigger_ && !waitingUpdate_; }
	bool canHovered() const { return isVisible() && (canHovered_ || isUnderEditor()); }

	/// запрещает саму кнопку и все дочерние кнопки
	virtual void disable();
	/// разрешает саму кнопку и все дочерние кнопки
	virtual void enable();
	bool isEnabled() const { return isEnabled_; }
	bool isActivated() const { return isActive_; }
	bool waitingExecution() const { return isActive_ && needDeactivation_; }

	float activationTime() const;
	float deactivationTime() const;

	bool isChangeProcess() const;

	enum DrawMode{
		UI_DRAW_SPRITE = 1 << 0,
		UI_DRAW_BORDER = 1 << 1,
		UI_DRAW_TEXT = 1 << 2,
		UI_DRAW_ALL = UI_DRAW_SPRITE | UI_DRAW_BORDER | UI_DRAW_TEXT
	};
	/// отрисовка кнопки и всех её дочерних кнопок
	bool redraw() const;
	
	void getDebugString(XBuffer& buf) const;
	void drawDebug2D() const;
	
	virtual void quant(float dt);

	void logicInit();
	void logicQuant();

	bool setPosition(const Vect2f& pos);
	bool setPosition(const Rectf& pos);
	Rectf position() const { return position_; }
	
	const Rectf& transfPosition() const { return transfPosition_; }

	float screenZ() const { return float(screenZ_); }

	const char* text() const {
		if(MT_IS_GRAPH())
			return text_.c_str();
		else if(textChanged_)
			return newText_.c_str();
		else
			return text_.c_str();
	}

	int textAlign() const { return textAlign_ | textVAlign_; }
	void setText(const char* p);
	
	virtual void textChanged() {}

	/// координаты текста, пересчитанные относительно экрана
	Rectf textPosition() const;
	
	virtual int getSubRectCount() const;
    virtual Rectf getSubRect(int index) const;
	virtual void setSubRect(int index, const Rectf& rect);
	UI_Mask& mask(){ return mask_; }
	const UI_Mask& mask() const{ return mask_; }

	const UI_Font* font() const { return font_; }
	const cFont* cfont() const;
	const UI_TextFormat* textFormat(UI_ControlShowModeID mode) const;
	const UI_TextFormat* textFormat() const { return textFormat(showModeID_); }

	virtual bool hitTest(const Vect2f& pos) const;

	bool hotKeyHandler();
	virtual bool activate();

	void toggleBorder(bool state){ borderEnabled_ = state; }
	void setBorderColor(const sColor4f& color){ borderColor_ = color; }

	UI_ControlState* state(int state_index);
	const UI_ControlState* state(int state_index) const;

	UI_ControlState* state(const char* state_name);
	const UI_ControlState* state(const char* state_name) const;
	int getStateByMark(StateMarkType type, int value) const;

	int currentStateIndex() const { return currentStateIndex_; }

	bool setState(int state_index);
	bool setState(const char* state_name);
	bool setSprite(const UI_Sprite& sprite, int shMode = -1);
	
	UI_ControlBase* childControl(const char* name) const;

	bool handleInput(const UI_InputEvent& event);
	bool checkInputActionFlag(ActionMode event) const { return actionFlags_ & event; }
	void setInputActionFlag(ActionMode event){ actionFlags_ |= event; }
	ActionMode actionFlags() const { return actionFlags_; }
	void clearInputEventFlags();
	void updateIndex();

	bool hoverUpdate(const Vect2f& cursor_pos);
	void hoverToggle(bool val){ mouseHover_ = val; }
	bool mouseHover() const { return mouseHover_; }

	bool autoFormatText() const { return autoFormatText_; }

	// юнит на которого есть ссылки из назначений
	const AttributeBase* actionUnit(const AttributeBase* selected) const;
	// ParameterSet для конкретного юнита из назначений
	bool actionParameters(const AttributeBase* unit, ParameterSet& params) const;
	// Производимый параметр из назначений
	const ProducedParameters* actionBuildParameter(const AttributeBase* unit) const;

	InterfaceGameControlID controlID() const;

	void setShowMode(UI_ControlShowModeID mode);
	UI_ControlShowModeID showModeID() const { return showModeID_; }

	typedef std::vector<UI_ControlState> StateContainer;
	StateContainer& states(){ return states_; }

	void controlComboListBuild(std::string& list_string, UI_ControlFilterFunc filter) const;

	void setTransform(const UI_Transform& transf);
	void setPermanentTransform(const UI_Transform& transf) { userTransform_ = transf; }

	bool hasActivationTransform() const { return activationType_ != 0; }
	void setActivationTransform(float dt, bool reverse);

	const UI_Cursor* hoveredCursor() const { return hoveredCursor_; }
	const char* hint() const;

	int actionCount() const { return actions_.size(); }
	UI_ControlActionID actionID(int index) const {
		xassert(index >= 0 && index < actions_.size());
		return actions_[index].key();
	}
	const UI_ActionData* actionData(int index) const {
		xassert(index >= 0 && index < actions_.size());
#ifndef _FINAL_VERSION_
		if(const UI_ActionData* data = actions_[index].type())
			return data;
		string nm;
		referenceString(nm);
		xxassert(0, XBuffer() < "Пустое назначение в " < nm.c_str());
		static UI_ActionData emptyData;
		return &emptyData;
#else
		return actions_[index].type();
#endif

	}

	virtual void actionInit(UI_ControlActionID action_id, const UI_ActionData* action_data);
	virtual void actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState);
	virtual void actionExecute(UI_ControlActionID action_id, const UI_ActionData* action_data);
	virtual void controlInit() {}
	virtual void controlUpdate(ControlState& controlState);
	void handleAction(const ControlMessage& msg);

	void toggleDeactivation(bool state){ needDeactivation_ = state; }

	void startBackgroundAnimation(UI_BackgroundAnimation::PlayMode mode, bool recursive = false, bool reverse = false);
	void stopBackgroundAnimation(UI_BackgroundAnimation::PlayMode mode, bool recursive = false) const;
	bool isBackgroundAnimationActive(UI_BackgroundAnimation::PlayMode mode) const;
	void startActivationEffects(float dt = 0.f, bool recursive = true, bool reverse = false);
	void updateActivationOffsets(float offsets[4]);

	void getHotKeyList(vector<UI_ControlHotKey>& out) const;
	void getHotKeyList(vector<sKey>& out) const;

#ifndef _FINAL_VERSION_
	mutable DurationNonStopTimer showInfoTimer_;
	bool needShowInfo() const { return showInfoTimer_() && !(mouseHover_ || mouseHoverPrev_); }
	void startShowInfo() const { showInfoTimer_.start(2000); }
#else
	bool needShowInfo() const { return false; }
	void startShowInfo() const {}
#endif

protected:
	/// инициализация кнопки, необходимо вызывать после serialize()
	virtual void postSerialize();

	void actionRequest() { actionActive_ = true; }
	void action();

	const UI_ActionData* findAction(UI_ControlActionID id) const;

	enum TransformMode {
		TRANSFORM_NONE = 0,
		TRANSFORM_ACTIVATION,
		TRANSFORM_DEACTIVATION
	};
	/// Непосредственная обработка события ввода с клавиатуры/мыши.
	/// Если событие обработано, возвращает true.
	/// Вызывается из \a handleInput().
	virtual bool inputEventHandler(const UI_InputEvent& event);

	const UI_ControlShowMode* showMode(UI_ControlShowModeID id) const {
		if(currentStateIndex_ >= 0){
			xassert(currentStateIndex_ < states_.size()); 
			return states_[currentStateIndex_].showMode(id);
		}
		return 0;
	}

	bool isScaled() const { return isScaled_; }
	float alpha() const { return alpha_; }

	const Rectf& relativeTextPosition() const { return textPosition_; }

	void setDrawMode(int mode) const { drawMode_ = mode; }

	void badActivationMessage();

	/// координаты и размеры кнопки относительно размеров экрана
	Rectf position_;       // Неизменная при трансформациях позиция

private:
	void applyNewText(const char* newText);

	bool isVisible_;
	bool isVisibleByTrigger_;
	bool redrawLock_;
	bool waitingUpdate_;
	bool canHovered_;

	/// true когда кнопка активирована
	bool isActive_;
	bool needDeactivation_;

	bool textChanged_;
	bool isDeactivating_;

	bool mouseHover_;
	bool mouseHoverPrev_;

	/// true когда кнопка разрешена
	bool isEnabled_;
	bool actionActive_;

	bool linkToAnchor_;

	/// true, если контрол смасштабирован
	bool isScaled_;

	/// информация о воздействии мышью на кнопку (клик, курсор над кнопкой и т.д.)
	ActionMode actionFlags_;

	UI_ControlShowModeID showModeID_;
	int currentStateIndex_;

	TransformMode transformMode_;

	/// текущее время анимации текстуры
	float bgTextureAnimationTime_;
	/// проигрывать анимацию
	bool bgTextureAnimationIsPlaying_;

	/// маска прозрачности кнопки
	/// если пустая, то вся кнопка считается непрозрачной для мыши
	UI_Mask mask_;

	/// состояния кнопки
	StateContainer states_;

	UI_BackgroundAnimation::PlayMode animationPlayMode_;

	/// привязка кнопки к якорю на мире
	string anchorName_;
	Rectf transfPosition_; // Трансформируемая и используемая при выводе позиция
	/// глубина, по ней контролы сортируются при отрисовке
	/// чем больше значение, тем раньше контрол отрисовывается
	int screenZ_;

	sKey hotKey_;
	CallWrapper<UI_ControlBase> handler_;

	/// рамка, координаты относительно общего размера контрола
	Rectf borderPosition_;
	/// режим отрисовки
	mutable int drawMode_;
	/// отрисовывать внутренность рамки или нет
	bool borderFill_;
	/// отрисовывать обводку рамки или нет
	bool borderOutline_;
	/// отрисовывать рамку или нет
	bool borderEnabled_;
	/// цвет рамки
	sColor4f borderColor_;
	/// цвет обводки рамки
	sColor4f borderOutlineColor_;

	/// прозрачность контрола, менятеся через \a transform_
	/// [0, 1], 0 - полная прозрачность
	float alpha_;

	LocString locText_;
	std::string text_;
	std::string newText_;
	UI_TextAlign textAlign_;
	UI_TextVAlign textVAlign_;
	/// коордитаты и размеры области для текста относительно координат и размеров кнопки
	Rectf textPosition_;

	bool autoFormatText_;

	UI_FontReference font_;

	friend class UI_Transform;
	/// Трансформирует параметры объекта
	UI_Transform transform_;
	UI_Transform userTransform_;

	BitVector<UI_Transform::Type> activationType_;
	UI_Transform::ScaleMode activationScaleMode_;
	/// В какую сторону выезжать при активации
	ActivationMove activationMove_;

	/// true если заданы отдельные настройки для деактивации
	bool hasDeactivationSettings_;
	BitVector<UI_Transform::Type> deactivationType_;
	UI_Transform::ScaleMode deactivationScaleMode_;
	/// В какую сторону уезжать при деактивации
	ActivationMove deactivationMove_;

	float activationTime_;
	float deactivationTime_;

	UI_CursorReference hoveredCursor_;
	LocString hoveredText_;

	UI_ControlActionList actions_;

	/// 3D анимация, соответствующая контролу
	UI_BackgroundAnimations backgroundAnimations_;

	bool visibleInEditor_;

	/// отрисовка рамки
	void drawBorder() const;
	/// координаты рамки относительно экрана
	Rectf borderPosition() const;

	const UI_ControlState* currentState() const { if(currentStateIndex_ != -1) return &states_[currentStateIndex_]; return 0; }

	void transformQuant(float dt);
	void resetTransform(){ transfPosition_ = position_; }

	void animationQuant(float dt);

	friend void fCommandControlApplyShow(void* data);
	void applyShow();
	friend void fCommandControlApplyHide(void* data);
	void applyHide(bool immediately);

	friend void logUIState(int line, const char* func, const UI_ControlBase* control, const char* str);
};

/// Интерфейсный экран.
class UI_Screen : public UI_ControlContainer
{
public:
	UI_Screen();
	~UI_Screen();

	enum ScreenType{
		ORDINARY,
		GAME_INTERFACE,
		LOADING,
		MESSAGE,
		EXIT_AD
	};

	void serialize(Archive& ar);

	/// вызывается когда экран становится активным
	void activate();
	void deactivate();
	bool isActive() const { return active_; }

	void quant(float dt);
	bool redraw() const;

	void init();
	void logicInit();
	void logicQuant();

	void preloadBgScene(cScene* scene, const Player* player) const;

	/// обработка ввода с клавиатуры или мыши
	bool handleInput(const UI_InputEvent& event, UI_ControlBase* focused_control = 0);

	bool hoverUpdate(const Vect2f& cursor_pos, UI_ControlBase* focused_control);

	/// посылка Action всем контролам на экране
	void handleMessage(const ControlMessage& msg);

	void updateControlOwners();
	void updateIndex();
	void controlComboListBuild(string& list_string, UI_ControlFilterFunc filter = 0) const;

	float activationTime() const { return max(activationTime_, controlsActivationTime()); }
	float deactivationTime() const { return max(deactivationTime_, controlsDeactivationTime()); }

	bool isActivating() const;
	bool isDeactivating() const;

	void initActivationActions(bool activation);
	float activationOffset(ActivationMove direction) const;

	ScreenType type() const { return type_; }

	void drawDebug2D() const;
	void saveHotKeys() const;
	void getHotKeyList(vector<sKey>& list) const;

private:
	/// тип экрана для автоматического выбора и переключения
	ScreenType type_;
	/// сейчас активный
	bool active_;
	/// Время за которое должен происходить выезд объектов из-за экрана (для которых задано)
	float activationTime_;
	float deactivationTime_;

	/// Смещения, с которых должны вылетать кнопки при активации, по четырём сторонам экрана.
	/// Рассчитываются при записи в редакторе.
	float activationOffsets_[4];

	/// Время, оставшееся до конца изменения состояния
	float activationDelay_;
	float deactivationDelay_;
	/// Не учитывать ввод при изменении состояния
	bool disableInputAtChangeState_;

	std::string backgroundModelName_;

	float controlsActivationTime() const;
	float controlsDeactivationTime() const;

	void updateActivationOffsets();
};

class UI_Text : public PolymorphicBase
{
public:
	UI_Text();

	void serialize(Archive& ar);

	bool hasText() const { return !text_.empty(); }
	const char* text() const { return text_.c_str(); }

	bool hasVoice() const { return !voice_.isEmpty(); }
	const VoiceAttribute& voice() const { return voice_; }

private:
	LocString text_;
	VoiceAttribute voice_;
};

typedef StringTable<StringTableBasePolymorphic<UI_Text> > UI_TextLibrary;
typedef StringTablePolymorphicReference<UI_Text, false> UI_TextReference;

struct UI_MessageType : public StringTableBaseSimple
{
	UI_MessageType(const char* name = "") : StringTableBaseSimple(name) {}
};
typedef StringTable<UI_MessageType> UI_MessageTypeLibrary;
typedef StringTableReference<UI_MessageType, true> UI_MessageTypeReference;
typedef vector<UI_MessageTypeReference> UI_MessageTypeReferences;

class UI_MessageSetup
{
public:
	UI_MessageSetup();

	bool operator == (const UI_MessageSetup& setup) const { return text() == setup.text() && type() == setup.type(); }
	bool operator != (const UI_MessageSetup& setup) const { return !(setup == *this); }

	const UI_MessageTypeReference& type() const { return messageType_; }

	const UI_Text* text() const { return textReference_; }
	bool isEmpty() const { return !textReference_; }

	const char* textData() const;

	bool hasText() const { if(textReference_) return textReference_->hasText(); else return false; }
	bool hasVoice() const { if(textReference_) return textReference_->hasVoice(); else return false; }

	const VoiceAttribute* voice() const { if(textReference_) return &textReference_->voice(); else return 0; }
	bool playVoice(bool interrupt) const;
	void stopVoice() const;
	float voiceDuration() const;
	bool isCanInterruptVoice() const { return isCanInterruptVoice_; }
	bool isVoiceInterruptable() const { return isVoiceInterruptable_; }
	bool isInterruptOnDestroy() const { return isInterruptOnDestroy_; }
	bool isCanPaused() const { return isCanPaused_; }
	bool playVoiceAlways() const { return isPlayVoiceAlways_; }

	float displayTime() const;

	void serialize(Archive& ar);

private:

	/// тип сообщения для фильтрации
	UI_MessageTypeReference messageType_;
	/// данные сообщения
	UI_TextReference textReference_;
	/// время показа сообщения, секунды
	float displayTime_;
	/// показывать сообщение по времени проигрывания звука
	bool syncroBySound_;
	/// звуковое сообщение можно прервать
	bool isVoiceInterruptable_;
	/// проигрывать звук
	bool enableVoice_;
	/// прерывать при разрушении мира
	bool isInterruptOnDestroy_;
	/// прерывать играющий звук
	bool isCanInterruptVoice_;
	/// ставить на паузу во время паузы
	bool isCanPaused_;
	/// всегда проигрывать звук
	bool isPlayVoiceAlways_;
};

class UI_Message
{
public:
	UI_Message();
	UI_Message(const UI_MessageSetup& message_setup, const char* custom_text);

	bool operator == (const UI_MessageSetup& setup) const { return setup_ == setup; }

	const char* text() const;
	void setCustomText(const char* text);

	bool isEmpty() const { return customText_.empty() && !setup_.hasText(); }
	const UI_MessageTypeReference& type() const { return setup_.type(); }

	void start();
	bool timerEnd() const { return !aliveTime_(); }

	void serialize(Archive& ar);

	const UI_MessageSetup& messageSetup() const { return setup_; }
private:

	DurationTimer aliveTime_;
	/// если установлен, то выводится он, а не текст из \a setup_
	std::string customText_;
	UI_MessageSetup setup_;
};

struct UI_TaskColor
{
	UI_TaskColor() : color(1.f, 1.f, 1.f) {}
	sColor4f color;
	string tag;
	void serialize(Archive& ar);
};

/// Игровая задача
class UI_Task
{
public:
	UI_Task(){ state_ = UI_TASK_ASSIGNED; isSecondary_ = false; }

	bool operator == (const UI_MessageSetup& message_setup) const { return setup_ == message_setup; }

	void set(UI_TaskStateID state, const UI_MessageSetup& message_setup, bool is_secondary = false)
	{
		state_ = state;
		setup_ = message_setup;
		isSecondary_ = is_secondary;
	}

	UI_TaskStateID state() const { return state_; }
	void setState(UI_TaskStateID state){ state_ = state; }
	bool isSecondary() const { return isSecondary_; }

	bool getText(string& out, bool message_mode = false) const;

	void serialize(Archive& ar);
	static void serializeColors(Archive& ar);

private:
	static UI_TaskColor taskColors_[UI_TASK_STATE_COUNT * 4];

	UI_TaskStateID state_;
	UI_MessageSetup setup_;

	bool isSecondary_;
};

/// символ для обозначения какого-либо объекта на миникарте
class UI_MinimapSymbol
{
public:
	UI_MinimapSymbol();

	void serialize(Archive& ar);

	enum SymbolType {
		/// прямоугольник
		SYMBOL_RECTANGLE,
		/// спрайт
		SYMBOL_SPRITE
	};

	bool operator == (const UI_MinimapSymbol& obj) const { return type_ == obj.type_ &&	scale_ == obj.scale_
		&& useColor_ == obj.useColor_ && (!useColor_ || color_ == obj.color_)
		&& (type_ == SYMBOL_RECTANGLE || sprite_ == obj.sprite_);
	}

	/// самоотрисовка символа
	bool redraw(const Rectf& object_pos, const sColor4f& legionColor, float time) const;
	
	/// получить время жизни символа на миникарте
	int lifeTime() const;
	/// Символ нужно отмаштабировать по юниту
	bool scaleByEvent() const { return scaleByEvent_; }

private:

	/// тип изображения
	SymbolType type_;
	/// собственный масштаб
	float scale_;
	/// маштабировать по объекту/событию
	bool scaleByEvent_;
	/// использовать собственный цвет для отрисовки символа
	bool useColor_;
	/// собственный цвет символа
	sColor4f color_;
	/// спрайт для отрисовки на миникарте
	UI_Sprite sprite_;
};

class AtomAction
{
public:

	enum AtomActionType{
		SHOW,
		HIDE,
		ENABLE_SHOW,
		DISABLE_SHOW,
		ENABLE,
		DISABLE,
		VIDEO_PLAY,
		VIDEO_PAUSE,
		SET_FOCUS
	};

	AtomAction() : type_(SHOW) {}

	void apply() const;
	bool workedOut() const;

	void serialize(Archive& ar);

private:

	AtomActionType type_;
	UI_ControlReference control_;
};

typedef vector<AtomAction> AtomActions;


struct ShowStatisticType
{
	enum Type {
		STAT_VALUE,
		SPACE,
		POSITION,
		NAME,
		RATING
	};

	Type type;
	StatisticType statValueType;

	ShowStatisticType() : type(STAT_VALUE), statValueType(TOTAL_WINS) {}
	void serialize(Archive& ar);

	void getValue(XBuffer& out, const StatisticsEntry& val) const;
};

typedef vector<ShowStatisticType> ShowStatisticTypes;

#endif /* __UI_TYPES_H__ */
