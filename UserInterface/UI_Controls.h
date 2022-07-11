#ifndef __UI_CONTROLS_H__
#define __UI_CONTROLS_H__

#include "UI_Types.h"
#include "UI_TextParser.h"
#include "UI_TextAnimation.h"
#include "Units\UnitLink.h"
#include "Units\AttributeReference.h"
#include "Units\AttributeSquad.h"
#include "UI_Minimap.h"
#include "SystemUtil.h"
#include "XTL\SafeCast.h"

class UnitInterface;

/// обычная кнопка
class UI_ControlButton : public UI_ControlBase
{
public:
	UI_ControlButton();

	void serialize(Archive& ar);

	void textChanged();

	void actionInit(UI_ControlActionID action_id, const UI_ActionData* action_data);
	void actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState);
	void actionExecute(UI_ControlActionID action_id, const UI_ActionData* action_data);

	const Rectf& originalPosition() const { return positionOriginal_; }

	void setAutoResize(bool val) { autoResize_ = val; }

private:
	/// выставляет размер контрола по тексту
	bool adjustSize();

	/// автоматически подгонять размер контрола под содержимое
	bool autoResize_;

	ResizeShiftHorizontal resizeShiftHorizontal_;
	ResizeShiftVertical resizeShiftVertical_;

	/// резервная копия размеров контрола
	Rectf positionOriginal_;

	float data_;
};

/// кнопка - регулятор
class UI_ControlSlider : public UI_ControlBase
{
public:
	UI_ControlSlider();
	~UI_ControlSlider();

	void serialize(Archive& ar);

	UI_SliderOrientation orientation() const { return orientation_; }

	float value() const { return value_; }
	void setValue(float value){ value_ = value; }
	void setDiscretStep(float step){ step_ = step; }

	void quant(float dt);

	UI_ControlBase* arrowDec() const { return (*this)[CONTROL_ARROW_DEC_VALUE]; }
	UI_ControlBase* arrowInc() const { return (*this)[CONTROL_ARROW_INC_VALUE]; }

	enum ControlID
	{
		CONTROL_SLIDER = 0,
		CONTROL_ARROW_DEC_VALUE,
		CONTROL_ARROW_INC_VALUE,
	};

	bool dragMode() const { return dragMode_; }

	float index2sliderPhase(int index, int scrolling_size) const;
	int sliderPhase2Index(float value, int scrolling_size) const;

protected:

	bool inputEventHandler(const UI_InputEvent& event);

private:

	/// положение регулятора, [0, 1]
	float value_;
	float step_;

	/// шаг изменения значения
	float valueDelta_;

	/// true если принимает только дискретные значения, кратные valueDelta_
	bool isDiscrete_;

	UI_SliderOrientation orientation_;

	bool dragMode_;
};

class UI_ControlTextList : public UI_ControlBase
{
public:
	enum ScrollType {
		ORDINARY,
		AUTO_SMOOTH,
		AT_END_IF_SIZE_CHANGE,
		AUTO_SET_AT_END
	};
	UI_ControlTextList();

	void preLoad();
	void release();

	void serialize(Archive& ar);
	void init();

	void quant(float dt);

	bool redraw() const;

	void textChanged();
	void actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState);

protected:
	void reparse();

	bool inputEventHandler(const UI_InputEvent& event);

	UI_ControlSlider* slider() const { return safe_cast<UI_ControlSlider*>((*this)[0]); }

	int scrollSize() const { return stringCount_ - viewSize_; }
	float index2sliderPhase(int index) const;
	int sliderPhase2Index(float value) const;

private:

	bool autoHideScroll_;
	bool lineCountChanged_;

	bool isAnimation_;

	ScrollType scrollType_;

	int firstVisibleLine_;
	int viewSize_;
	
	int scrolledLines_;
	float scrollSpeed_;
	float scrollShift_;

	Vect2i screenSize_;

	float stringHeight_;
	int stringCount_;
	UI_TextParser parser_;

	UI_TextAnimation animation_;
};

// ввод комбинаций клавиш
class UI_ControlHotKeyInput : public UI_ControlBase
{
public:
	UI_ControlHotKeyInput();
	~UI_ControlHotKeyInput() {}
	
	void serialize(Archive& ar);
	
	bool activate();

	void quant(float dt);

	void actionInit(UI_ControlActionID action_id, const UI_ActionData* action_data);
	void actionExecute(UI_ControlActionID action_id, const UI_ActionData* action_data);

	void onFocus(bool focus_state);

	const UI_Key& key() const { return key_; }
	void setKey(const UI_Key& key){ key_ = key; }

	const UI_Key& saveKey() const { return saveKey_; }

protected:
	bool inputEventHandler(const UI_InputEvent& event);

private:
	bool waitingInput_;
	/// может пересекаться с хоткеями на кнопках
	bool compatible_;
	
	UI_Key key_;
	UI_Key saveKey_;

	void done(const sKey& key, bool force);
};

/// поле ввода
class UI_ControlEdit : public UI_ControlBase
{
public:
	enum CharType {
		ALNUM,
		LATIN,
		ALLCHAR
	};

	UI_ControlEdit();
	~UI_ControlEdit();

	void serialize(Archive& ar);

	void quant(float dt);
	bool activate();
	
	bool editDone(bool done);
	bool editInput(int vkey);
	bool editCharInput(wchar_t chr);

	void setEditText(const wchar_t* p);
	
	bool redraw() const;

	void onFocus(bool focus_state);

protected:
	void applyText(const wstring& txt);

	bool inputEventHandler(const UI_InputEvent& event);

	bool checkSymbol(wchar_t chr);

private:
	/// максимальная длина редактируемой строки, 0 - без ограничений
	int textLengthMax_;

	bool isEditing_;
	bool password_;
	bool saveFocus_;

	bool caretVisible_;
	int caretPose_;
	Color4c caretColor_;
	float caretTimer_;

	CharType charType_;

	wstring textBackup_;
	wstring textEdit_;

	UI_ControlReference next_;
};

/// таб
class UI_ControlTab : public UI_ControlBase
{
public:
	UI_ControlTab();
	~UI_ControlTab();

	void init();

	void quant(float dt);
	
	void showChildControls();

	bool removeControl(UI_ControlBase* p){ if(currentSheet() == p) currentSheetIndex_ = -1; return removeControlFromList(p); }

private:

	/// делает страницу активной
	void selectSheet(int index);

	/// номер активной страницы
	int currentSheetIndex_;

	/// возвращает активную страницу
	UI_ControlBase* currentSheet(){ return (*this)[currentSheetIndex_]; }
};

/// список строк
class UI_ControlStringList : public UI_ControlBase
{
public:
	struct Column {
		Column() : width(100), align(UI_TEXT_ALIGN_LEFT) {}
		int width; // в процентах
		UI_TextAlign align;
		void serialize(Archive& ar);
	};
	typedef vector<Column> Columns;
	
	UI_ControlStringList();
	~UI_ControlStringList();

	void preLoad();
	void release();

	void serialize(Archive& ar);
	void init();

	void quant(float dt);
	bool redraw() const;
	void drawDebug2D() const;

	void setList(const ComboWStrings& strings);

	int listSize() const { return getList().size(); }
	float stringHeight() const { return stringHeight_; }

	/// возвращает выбранную строку или 0 если ничего не выбрано
	const wchar_t* selectedString() const;
	int selectedStringIndex() const { return selectedString_; }
	void setSelectedString(int index){ 
		if(MT_IS_GRAPH() || !waitingExecution())
			selectedString_ = index; }

	virtual void setSelect(const ComboWStrings& selectList);
	virtual void getSelect(ComboWStrings& out);

protected:

	void flushNewStrings();

	bool inputEventHandler(const UI_InputEvent& event);
	virtual void toggleString(int idx);

	virtual void applyNewList(const ComboWStrings& newList);
	const ComboWStrings& getList() const {
		if(MT_IS_GRAPH())
			return strings_;
		else if(listChanged_)
			return newStrings_;
		else
			return strings_;
	}

	ComboWStrings strings_;
	ComboWStrings newStrings_;

	/// высота одной строки, в относительных экранных коодинатах
	mutable float stringHeight_;
	/// высота одной строки, коэффициент от размера шрифта
	float stringHeightFactor_;
	/// номер первой видимой строки
	int firstVisibleString_;
	/// номер выбранной строки
	int selectedString_;
	/// максимальное количество строк, до которого можно автоматически растягивать
	int stringMax_;
	/// подстройка размеров списка
	bool autoResizeList_;
	/// скрывать slider если недего слайдить
	bool autoHideSlider_;

	bool listChanged_;

	UI_Sprite underline_;

	Columns columns_;

	UI_ControlSlider* slider() const { return safe_cast<UI_ControlSlider*>((*this)[0]); }

	void adjustListSize();
	int scrollSize() const;
	float index2sliderPhase(int index) const;
	int sliderPhase2Index(float value) const;
};

class UI_ControlStringCheckedList : public UI_ControlStringList
{
public:
	UI_ControlStringCheckedList();
	~UI_ControlStringCheckedList();

	void serialize(Archive& ar);
	void init();

	bool redraw() const;

	void setSelect(const ComboWStrings& selectList);
	void getSelect(ComboWStrings& out);

protected:

	void toggleString(int idx);
	void applyNewList(const ComboWStrings& newList);

private:
	vector<char> selected_;

	UI_SpriteReference checkOn_;
	UI_SpriteReference checkOff_;
};

class UI_ControlComboList : public UI_ControlBase
{
public:
	UI_ControlComboList();
	~UI_ControlComboList();

	void serialize(Archive& ar);
	void init();

	void quant(float dt);
	
	/// выпадающий список
	UI_ControlStringList* dropList() const { return safe_cast<UI_ControlStringList*>((*this)[0]); }

	static UI_ControlStringList* getList(UI_ControlBase* control);
	static void setList(UI_ControlBase* control, const ComboWStrings& strings, int selected);

	void onFocus(bool focus_state);

private:
	void showChildControls();

	bool dropListVisible_;
	/// автоматически выбирать первое значение из списка
	bool autoSetValue_;
};

class UI_ControlProgressBar : public UI_ControlBase
{
public:
	UI_ControlProgressBar();
	~UI_ControlProgressBar();

	void preLoad();
	void release();

	void actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState);
	void serialize(Archive& ar);

	void quant(float dt);

	bool redraw() const;

	void setProgress(float progress){ xassert(progress >= 0.f && progress <= 1.f); progress_ = progress; }

private:

	bool show_only_not_empty_;
	bool full_is_transparent_;
	bool changeColor_;
	bool vertical_;

	bool showProgressChange_; // менять цвет при изменении значения
	float changePeriod_; // время, за которое измеряется изменение
	float changeMin_; // минимальное показываемое изменение
	float changeTimer_;
	float lastProgress_;

	float progress_;

	UI_Sprite progressSprite_;

	Color4f colorDone_;
	Color4f colorLeft_;

	Color4f colorDoneFull_;

	Color4f colorInc_; // цвет при увеличении значения
	Color4f colorDec_; // цвет при уменьшении значения
	float changeColorPhase_;
	float changeColorPhaseTarget_;

	Color4f colorDone() const {
		Color4f color = colorDone_;
		if(changeColor_)
			color.interpolate(color, colorDoneFull_, progress_);
		if(showProgressChange_)
			color.interpolate(color, changeColorPhase_ < 0.f ? colorDec_ : colorInc_, fabs(changeColorPhase_));

		return color;
	}
};

/// кнопка для вывода разной игровой информации
class UI_ControlCustom : public UI_ControlBase
{
public:
	UI_ControlCustom();
	~UI_ControlCustom();

	bool redraw() const;

	void serialize(Archive& ar);

	void controlInit();
	
	bool checkType(UI_ControlCustomType tp) const { if(type_ & tp) return true; else return false; }

protected:

	bool inputEventHandler(const UI_InputEvent& event);
	void sendToWorld(UI_InputEvent event, const Vect2f& mousePos) const;

private:

	/// типы выводимой информации
	BitVector<UI_ControlCustomType> type_;

	/// прозрачность миникарты, [0, 1]
	float mapAlpha_;
	
	bool clickAction_;
	bool drawViewZone_;
	bool drawFogOfWar_;
	bool drawInstallZones_;
	// брать миникарту из текущей выделенной в списке игры, а не с мира
	bool useSelectedMap_;

	bool drawWindDirection_;

	bool rotateByCamera_;
	bool rotateByCameraInitial_;
	/// масштабировать под размер окна при повороте карты
	bool rotationScale_;
	bool getAngleFromWorld_;
	float minimapAngle_;
	Color4f viewZoneColor_;
	Color4f miniMapBorderColor_;

	/// текстура маски миникарты
	UI_TextureReference maskTexture_;

	/// маштабировать мышкой
	bool scaleMinimap_;
	/// двигать мышкой
	bool dragMinimap_;
	/// сдвигать миникарту, что бы выделенный юнит был в центре
	bool minimapToSelect_;
	/// маштаб вывода миникарты
	float minimapScale_;

	bool viewStartLocations_;
	UI_FontReference font_;

	MinimapAlign mapAlign_;

	mutable LogicTimer headDelayTimer_;
};

/// список юнитов
class UI_ControlUnitList : public UI_ControlBase
{
public:
	UI_ControlUnitList();
	~UI_ControlUnitList();

	void preLoad();
	void release();

	void serialize(Archive& ar);

	void controlUpdate(ControlState& cs);

	const UI_ControlUnitListType GetType() const { return type_; }
	const AttributeSquad* squadAtrribute() const { return squadRef_; }

	const UI_Transform& activeTransform() const { return activeTransform_; }
	
	typedef StaticMap<AttributeUnitOrBuildingReference, UI_ShowModeSprite> UI_UnitSpritePrms;
	typedef SwapVector<UnitLink<UnitInterface> > UnitInterfaceLinkList;

private:

	const UI_ShowModeSprite* getSprite(const AttributeBase* unit) const;
	void setSprites(UI_ControlBase* control, const AttributeBase* unit = 0);
	void setSprites(UI_ControlBase* control, const UnitInterface* unit);

	UI_ControlUnitListType type_;	
	UI_UnitSpritePrms unitSpriteParams_;

	UI_ShowModeSprite defSprite_;
	
	UI_Transform activeTransform_;

	AttributeSquadReference squadRef_;
};

// --------------------------------------------------------------------------

class UI_ControlCustomList : public UI_ControlBase
{
public:
	enum Type {
		TASK_LIST		= 1,
		MESSAGE_LIST	= 1 << 1,
		COMMON_LIST		= TASK_LIST|MESSAGE_LIST
	};

	enum Side {
		TOP,
		BOTTOM,
		LEFT,
		RIGHT
	};
	
	UI_ControlCustomList();
	~UI_ControlCustomList();

	void init();
	void preLoad();
	void release();

	void quant(float dt);

	void serialize(Archive& ar);

	void controlUpdate(ControlState& cs);

private:
	UI_ControlButton* hint() const { return safe_cast<UI_ControlButton*>((*this)[0]); }

	Type type_;
	bool reverse_;
	bool deleteOld_;
	bool autoDelete_;
	bool showOnHover_;
	UI_MessageTypeReferences messageTypes_;

	UI_ShowModeSprite activeTask_;
	UI_ShowModeSprite completedTask_;
	UI_ShowModeSprite activeSecTask_;
	UI_ShowModeSprite completedSecTask_;
	UI_ShowModeSprite activeMessage_;
	UI_ShowModeSprite oldMessage_;

	Side hintSide_;
	Side animationSide_;

	struct Node {
		//union {
			UI_Message message;
			UI_Task task;
		//};
		bool isTask;
		bool cached;
		bool open;
		Node() : isTask(true), cached(true), open(false), task() {}
		Node(const UI_Task& t) : isTask(true), cached(false), open(false) { task = t; }
		Node(const UI_Message& m) : isTask(false), cached(false), open(false) { message = m; }

		bool operator==(const UI_Task& t) const { return isTask ? task == t.messageSetup() : false; }
		bool operator==(const UI_Message& m) const { return isTask ? false : message == m.messageSetup(); }

		bool operator!=(const Node& n) const {
			return isTask != n.isTask || cached != n.cached || open != n.open
				|| !(isTask ? task == n.task.messageSetup() : message == n.message.messageSetup());
		}
	};
	typedef vector<Node> Nodes;
	Nodes nodes_;

	long volatile activated_;
	int lastNode_;
	Rectf hintPosition_;
};

//------------------------------------------------------------------------------------

class UI_ControlVideo : public UI_ControlBase
{
public:
	UI_ControlVideo();
	~UI_ControlVideo();

	void serialize(Archive& ar);

	void init();
	void controlInit();
	void release();
	
	void quant(float dt);
	bool redraw() const;

	void play();
	void pause();

private:
	
	string videoFile_;
	bool cycled_;
	bool mute_;
	mutable bool needNextFrame_;
	bool wasStarted_;
	Color4f diffuseColor_;
};

#endif /* __UI_CONTROLS_H__ */
