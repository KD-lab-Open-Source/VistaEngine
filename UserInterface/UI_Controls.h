#ifndef __UI_CONTROLS_H__
#define __UI_CONTROLS_H__

#include "UI_Types.h"
#include "UI_TextParser.h"
#include "..\Units\UnitLink.h"
#include "..\Units\AttributeReference.h"
#include "SystemUtil.h"
#include "SafeCast.h"

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

	void postSerialize();
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

/// кнопка лога со скроллом
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

	void serialize(Archive& ar);
	void init();

	void quant(float dt);

	bool redraw() const;

	void textChanged();
	void actionUpdate(UI_ControlActionID action_id, const UI_ActionData* action_data, ControlState& controlState);

protected:
	void postSerialize();
	void reparse();

	bool inputEventHandler(const UI_InputEvent& event);

	UI_ControlSlider* slider() const { return safe_cast<UI_ControlSlider*>((*this)[0]); }

	int scrollSize() const { return stringCount_ - viewSize_; }
	float index2sliderPhase(int index) const;
	int sliderPhase2Index(float value) const;

private:

	bool autoHideScroll_;
	bool textChanged_;

	ScrollType scrollType_;

	int firstVisibleLine_;
	int viewSize_;
	
	int scrolledLines_;
	float scrollSpeed_;
	float scrollShift_;

	Vect2i screenSize_;

	float stringHeight_;
	int stringCount_;
	UI_TextParser parser;
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

	const sKey& key() const { return key_; }
	void setKey(const sKey& key){ key_ = key; }

	const sKey& saveKey() const { return saveKey_; }

protected:
	bool inputEventHandler(const UI_InputEvent& event);

private:
	bool waitingInput_;
	/// может пересекаться с хоткеями на кнопках
	bool compatible_;
	
	sKey key_;
	sKey saveKey_;

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
	bool editCharInput(int chr);

	void setEditText(const char* p);
	
	bool redraw() const;

	void onFocus(bool focus_state);

protected:
	void postSerialize();
	void applyText(const string& txt);

	bool inputEventHandler(const UI_InputEvent& event);

	bool checkSymbol(unsigned char chr);

private:
	/// максимальная длина редактируемой строки, 0 - без ограничений
	int textLengthMax_;

	bool isEditing_;
	bool password_;
	bool saveFocus_;

	bool caretVisible_;
	int caretPose_;
	sColor4c caretColor_;
	float caretTimer_;

	CharType charType_;

	string textBackup_;
	string textEdit_;

	UI_ControlReference next_;
};

/// таб
class UI_ControlTab : public UI_ControlBase
{
public:
	UI_ControlTab();
	~UI_ControlTab();

	void serialize(Archive& ar);
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

	void serialize(Archive& ar);
	void init();

	void quant(float dt);
	bool redraw() const;
	void drawDebug2D() const;

	void setList(const ComboStrings& strings);

	int listSize() const { return getList().size(); }
	float stringHeight() const { return stringHeight_; }

	/// возвращает выбранную строку или 0 если ничего не выбрано
	const char* selectedString() const;
	int selectedStringIndex() const { return selectedString_; }
	void setSelectedString(int index){ 
		if(MT_IS_GRAPH() || !waitingExecution())
			selectedString_ = index; }

	virtual void setSelect(const ComboStrings& selectList);
	virtual void getSelect(ComboStrings& out);

protected:

	void flushNewStrings();

	bool inputEventHandler(const UI_InputEvent& event);
	virtual void toggleString(int idx);

	virtual void applyNewList(const ComboStrings& newList);
	const ComboStrings& getList() const {
		if(MT_IS_GRAPH())
			return strings_;
		else if(listChanged_)
			return newStrings_;
		else
			return strings_;
	}

	ComboStrings strings_;
	ComboStrings newStrings_;

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

	void setSelect(const ComboStrings& selectList);
	void getSelect(ComboStrings& out);

protected:

	void toggleString(int idx);
	void applyNewList(const ComboStrings& newList);

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
	static void setList(UI_ControlBase* control, const ComboStrings& strings, int selected);

	void onFocus(bool focus_state);

private:

	bool dropListVisible_;
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

	bool redraw() const;

	void setProgress(float progress){ xassert(progress >= 0.f && progress <= 1.f); progress_ = progress; }

private:

	bool show_only_not_empty_;
	bool full_is_transparent_;
	bool vertical_;
	float progress_;

	UI_Sprite progressSprite_;

	sColor4f colorDone_;
	sColor4f colorLeft_;
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

private:

	/// типы выводимой информации
	BitVector<UI_ControlCustomType> type_;

	/// прозрачность миникарты, [0, 1]
	float mapAlpha_;
	
	bool clickAction_;
	bool drawViewZone_;
	bool drawFogOfWar_;
	// брать миникарту из текущей выделенной в списке игры, а не с мира
	bool useSelectedMap_;

	bool rotateByCamera_;
	bool getAngleFromWorld_;
	float minimapAngle_;
	sColor4f viewZoneColor_;
	sColor4f miniMapBorderColor_;

	bool viewStartLocations_;
	UI_FontReference font_;

	UI_Align mapAlign_;
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

	const UI_Transform& activeTransform() const { return activeTransform_; }
	
	struct UI_UnitSpritePrm{
		UI_UnitSpritePrm() {
			ID_ = UI_SHOW_NORMAL;
		}
		
		AttributeUnitOrBuildingReference unitAttributeReferenre_;
		UI_ControlShowModeID ID_;
		UI_Sprite sprite_;

		void serialize(Archive& ar);
	};

	typedef vector<UI_UnitSpritePrm> UI_UnitSpritePrms;
	typedef SwapVector<UnitLink<UnitInterface> > UnitInterfaceLinkList;

private:

	const UI_Sprite& getSprite(const AttributeBase* unit, UI_ControlShowModeID id) const;
	void setSprites(UI_ControlBase* control, const AttributeBase* unit = 0);
	void setSprites(UI_ControlBase* control, const UI_ShowModeSpriteReference& sprites);

	UI_ControlUnitListType type_;	
	UI_UnitSpritePrms unitSpriteParams_;
	
	UI_Transform activeTransform_;
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
	sColor4f diffuseColor_;
};

#endif /* __UI_CONTROLS_H__ */
