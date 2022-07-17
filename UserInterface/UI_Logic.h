#ifndef __UI_LOGIC_H__
#define __UI_LOGIC_H__

#include "Handle.h"
#include "NetPlayer.h"
#include "SwapVector.h"
#include "ProfileManager.h"
#include "Controls.h"
#include "UI_Enums.h"
#include "UI_MarkObject.h"
#include "UI_Inventory.h"
#include "Installer.h"
#include "..\Units\CircleManagerParam.h"
#include "..\Units\WeaponTarget.h"
#include "SystemUtil.h"

class WeaponPrm;

class cEffect;
class cTexture;

class UnitInterface;
class AttributeBase;

class Player;

class UI_InputEvent;
class UI_ActionData;

class UI_ControlCustom;
class UI_ControlUnitList;
class UI_ControlHotKeyInput;

class UI_ActionDataSaveGameList;
class UI_ActionDataPlayer;

class UI_NetCenter;

enum eNetMessageCode;

enum DirectControlMode;

/// Этап загрузки игры.
class UI_LoadProgressSection
{
public:
	UI_LoadProgressSection(UI_LoadProgressSectionID id = UI_LOADING_INITIAL) : ID_(id)
	{
		size_ = 0.5f;
		stepCount_ = 1;
	}

	int ID() const { return ID_; }

	float size() const { return size_; }
	void setSize(float size){ size_ = size; }

	int stepCount() const { return stepCount_; }
	void setStepCount(int count){ stepCount_ = count; }

private:

	UI_LoadProgressSectionID ID_;

	/// длительность этапа относительно длительности всей загрузки
	float size_;
	/// количество шагов загрузки этапа
	int stepCount_;
};

class UI_HoverInfo
{
public:
	UI_HoverInfo(const UI_ControlBase* owner_control = 0, const UI_ControlBase* control = 0, const AttributeBase* unit = 0) : ownerControl_(owner_control),
	  control_(control), unit_(unit) { }

	bool operator == (const UI_ControlBase* control) const { return ownerControl_ == control; }

	void set(const UI_HoverInfo& inf){
		control_ = inf.control_;
		unit_ = inf.unit_;
	}

	const UI_ControlBase* ownerControl() const { return ownerControl_; }
	const UI_ControlBase* control() const { return control_; }
	const AttributeBase* unit() const { return unit_; }

private:

	const UI_ControlBase* ownerControl_;
	const UI_ControlBase* control_;
	const AttributeBase* unit_;
};

class UI_LogicDispatcher
{
public:
	UI_LogicDispatcher();
	~UI_LogicDispatcher();

	bool init();
	void destroyLinkQuant();
	void logicQuant(float dt);
	void directShootQuant();

	void reset();

	void graphQuant(float dt);
	void redraw();
	
	void showDebugInfo() const;
	void drawDebugInfo() const;
	void drawDebug2D() const;

	void sendBuildMessages(const AttributeBase* buildingAttr) const;

	void setSelectedUnit(UnitInterface* _unit) { selectedUnit_ = _unit; }
	UnitInterface* selectedUnit() const { return selectedUnit_; }

	/// обработка ввода с клавиатуры или мыши
	/// возвращает true, если событие обработано
	bool handleInput(const UI_InputEvent& event);

	void handleMessage(const ControlMessage& msg);
	void handleMessageReInitGameOptions();

	void updateInput(const UI_InputEvent& event);

	void updateAimPosition();
	
	/// вызывается когда событие \a event обработано контролом \a control
	bool inputEventProcessed(const UI_InputEvent& event, UI_ControlBase* control);
	void focusControlProcess(const UI_ControlBase* lastHovered);

	void setHoverControl(const UI_ControlBase* control) { hoverControl_ = control; if(hoverControl_) hoverControl_->startShowInfo(); }
	void setHoverPosition(const Vect3f& hoverPosition) { hoverPosition_ = hoverPosition; hoverTerrainPosition_ = hoverPosition;}
	void setHoverPositionTerrain(const Vect3f& hoverPosition) { hoverTerrainPosition_ = hoverPosition;}
	
	bool checkInputEventFlag(UI_InputEventID id) const { return ((inputFlags_ & (1 << id)) ? true : false); }
	void setInputEventFlag(UI_InputEventID id){ inputFlags_ |= (1 << id); }
	void clearInputEventFlags(){ inputFlags_ = 0; }

	/// добавление пометки на мире
	void addMark(const UI_MarkObjectInfo& inf, float time = 0.f);
	void addMovementMark();

	/// работа с мышью
	const Vect2f& mousePosition() const { return mousePosition_; }
	bool isMouseFlagSet(int flags) const { return ((mouseFlags_ & flags) != 0); }
	int mouseFlags() const { return mouseFlags_; }
	ActionModeModifer modifiers() const { return (isMouseFlagSet(MK_SHIFT) ? UI_MOUSE_MODIFER_SHIFT : 0) |( isMouseFlagSet(MK_MENU) ? UI_MOUSE_MODIFER_ALT : 0) | ( isMouseFlagSet(MK_CONTROL) ? UI_MOUSE_MODIFER_CTRL : 0); }

	/// Курсорный интерфейс
	const UI_Cursor* activeCursor() const { return activeCursor_; }
	bool cursorVisible() const { return cursorVisible_; }
	void selectCursor(const UI_Cursor* cameraCursor); // подбор курсора для текущего кадра
	void setCursor(const UI_Cursor* cursor);
	void showCursor();
	void hideCursor();
	void updateCursor();
	void setDefaultCursor();

	void toggleBuildingInstaller(const AttributeBase* attr);
	void disableDirectControl();

	void toggleShowAllParametersMode(bool flag) { showAllUnitParametersMode_ = flag; }
	bool showAllUnitParametersMode() const { return showAllUnitParametersMode_; }

	bool isGameActive() const;
	
	void setGamePause(bool isPause) { gamePause_ = isPause; }
	bool gamePause() const { return gamePause_; }

	void enableInput(bool state){ enableInput_ = state; }
	bool isInputEnabled() const { return enableInput_; }

	static UI_LogicDispatcher& instance(){ return Singleton<UI_LogicDispatcher>::instance(); }

	bool releaseResources();

	/// добавление источника света в 3D модель
	/// position - мировые координаты, внутри пересчитываются в координаты сцены 3D модели
	bool addLight(int light_index, const Vect3f& position) const;

	void selectClickMode(UI_ClickModeID mode_id, const WeaponPrm* selected_weapon = 0);

	UI_ClickModeID clickMode() const { return clickMode_; }
	int selectedWeaponID() const;
	const WeaponPrm* selectedWeapon() const { return selectedWeapon_; }

	void toggleTracking(bool state);

	bool drawSelection(const Vect2f& v0, const Vect2f& v1) const;

	/// Эффект под курсором
	void createCursorEffect();
	void moveCursorEffect();
	/// Работает ли еще триггер, установивший курсор
	bool cursorTriggered() const { return cursorTriggered_; }
	void toggleCursorTriggered(bool val){ cursorTriggered_ = val; }

	bool isAttackCursorEnabled() const { return isAttackEnabled_; }

	ProfileManager& profileSystem() { return profiles_; }
	Profile& currentProfile() { return profiles_.profile(); }

	bool missionIsSelected() const { return currentMission() != 0; }
	const MissionDescription* getMissionByName(const char* name) const;
	const MissionDescription* getMissionByID(const GUID& id) const;
	const MissionDescription* getMissionByID(GameType type, const GUID& id);

	const GUIDcontainer& quickStartFilter();
	void validateMissionFilter(Profile::MissionFilter& filter);
	
	void setCurrentMission(const MissionDescription& descr, bool inherit = false);
	void resetCurrentMission();
	MissionDescription* currentMission() const;
	bool useMapSetings() const { return currentMission() ? currentMission()->useMapSettings() : false; }
	/// старт выбранной миссии
	void missionStart();
	void missionReStart();

	const char* currentPlayerDisplayName() const;

	UI_NetCenter& getNetCenter() const;

	void handleChatString(const class ChatMessage& _chatMsg);
	void handleNetwork(eNetMessageCode msg);
    
	void saveGame(const string& gameName = string(""));
	void deleteSave();
	void saveReplay();

	UnitInterface* hoverUnit(){ return hoverUnit_; }
	const UnitInterface* hoverUnit() const { return hoverUnit_; }
	bool cursorInWorld() const { return cursorInWorld_; }
	const Vect3f& hoverPositionTerrain() const { return hoverTerrainPosition_; }
	const Vect3f& hoverPosition() const { return hoverPosition_; }
	const Vect3f& aimPosition() const { return aimPosition_; }
	float aimDistance() const { return aimDistance_; }
	const UI_ControlBase* hoverControl() const { return hoverControl_; }
	bool hoverWater() const { return hoverWater_; }
	bool hoverPassable() const { return hoverPassable_; }
	Player* player() const;
	int playerID() const;

	void SetSelectPeram(const struct SelectionBorderColor& param);

	void createSignSprites();
	void releaseSignSprites();

	void controlInit(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data);
	void controlUpdate(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data, ControlState& controlState);
	void controlAction(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data);

	void showLoadProgress(float val);
	void setLoadProgress(float val){ loadProgress_ = val; }

	void startLoadProgressSection(UI_LoadProgressSectionID id);
	void loadProgressUpdate();

	void setLastHotKeyInput(const UI_ControlHotKeyInput* control){ lastHotKeyInput_ = control; hotKeyUpdated_ = false; }
	bool hotKeyUpdated() const { return hotKeyUpdated_; }

	bool parseGameVersion(const char* updates);
	bool checkNeedUpdate() const;
	void openUpdateUrl() const;
	/// вызывается при необходммости заменить входящие в строку
	/// шаблонные последовательности: {name} на значения
	void expandTextTemplate(string& text, UnitInterface* unit = 0, const AttributeBase* attr = 0);
	const char* getParam(const char* name, UnitInterface* unit, const AttributeBase* attr);

	bool tipsEnabled() const { return showTips_; }
	void setShowTips(bool showTips) { showTips_ = showTips; }
	bool messagesEnabled() const { return showMessages_; }
	void setShowMessages(bool showMessages) { showMessages_ = showMessages; }

	void setTakenItem(const UI_InventoryItem* item)
	{
		if(item){
			takenItem_ = *item;
			takenItemSet_ = true;
		}
		else
			takenItemSet_ = false;
	}

	void cancelActions();

	void setDiskOp(UI_DiskOpID id, const char* path);
	void confirmDiskOp(bool state){ diskOpConfirmed_ = state; if(!state) setDiskOp(UI_DISK_OP_NONE, ""); }
	bool checkDiskOp(UI_DiskOpID id, const char* path);
	UI_DiskOpID diskOpID() const { return diskOpID_; }
	bool diskOpConfirmed() const { return diskOpConfirmed_; }

	void networkDisconnect(bool hard);
	void profileReseted();
	void clearGameChat();

private:
	/// относительные координаты мышиного курсора
	/**
	отсчитываются от верхнего левого угла экрана (0, 0),
	(1, 1) соответствует правому нижнему углу экрана
	*/
	Vect2f mousePosition_;
	int mouseFlags_;
	/// Курсор интерфейса
	const UI_Cursor* activeCursor_;
	/// Видим ли курсор
	bool cursorVisible_;

	int inputFlags_;

	/// разрешена ли обработка ввода
	bool enableInput_;
	bool gamePause_;

	cEffect* cursorEffect_;
	bool cursorTriggered_;

	BuildingInstaller buildingInstaller_;

	/// список профилей и текущий профиль
	ProfileManager profiles_;
	
	TeamGameType currentTeemGametype_;
	MissionDescription selectedMission_;

	/// поддержка загрузки и отображения списка миссий с диска
	MissionDescriptions missions_;
	GUIDcontainer quickStartMissions_;
	GameType getGameType(const UI_ActionDataSaveGameList& data);
	const MissionDescriptions& getMissionsList(const UI_ActionDataSaveGameList& data);
	const MissionDescription* getMissionByName(const char* name, const UI_ActionDataSaveGameList& data);

	const MissionDescription* getControlStateByGameType(GameTuneOptionType type, ControlState& state, const UI_ActionDataPlayer* data = 0) const;

	typedef std::vector<UI_Sprite> UI_Sprites;
	UI_Sprites signSprites;
	const UI_Sprite& signSprite(int index) const { xassert(index >= 0 && index < signSprites.size()); return signSprites[index]; }

	string saveGameName_;
	string saveReplayName_;

	string profileName_;

	void buildRaceList(UI_ControlBase* control, int selected, bool withAny = false);

	/// режим отдачи команды после клика по миру - идти, атаковать и т.п.
	UI_ClickModeID clickMode_;
	/// оружие, для которого указывается цель
	const WeaponPrm* selectedWeapon_;
	/// координаты прошлого нажатия левой кнопки мыши
	Vect2f mousePressPos_;
	/// true при селекте прямоугольником
	bool trackMode_;

	const UI_ControlHotKeyInput* lastHotKeyInput_;
	bool hotKeyUpdated_;

	bool showTips_;
	bool showMessages_;

	//UnitLink<UnitInterface> selectedUnit_;
	UnitInterface* selectedUnit_;

	bool isAttackEnabled_;

	bool cursorInWorld_;

	Vect3f aimPosition_;
	float aimDistance_;
	//UnitLink<UnitInterface> hoverUnit_;
	UnitInterface* hoverUnit_;
	//UnitLink<UnitInterface> startTrakingUnit_;
	UnitInterface* startTrakingUnit_;
	Vect3f hoverPosition_;
	Vect3f hoverTerrainPosition_;
	bool hoverWater_;
	bool hoverPassable_;
	bool showAllUnitParametersMode_;

	const UI_ControlBase* hoverControl_;

	typedef std::vector<UI_HoverInfo> HoverInfos;
	HoverInfos hoverInfos_;

	ComboStrings chatMessages_;
	typedef vector<DelayTimer> ChatMessageTimes;
	ChatMessageTimes chatMessageTimes_;

	float tipsControlDelay_;
	const UI_ControlBase* tipsControl_;
	float tipsUnitDelay_;
	const AttributeBase* tipsUnit_;

	SelectionBorderColor selectionPrm_;
	cTexture* selectionTexture_;
	cTexture* selectionCornerTexture_;
	cTexture* selectionCenterTexture_;

	typedef SwapVector<UI_MarkObject> MarkObjects;
	MarkObjects markObjects_;
	UI_MarkObject cursorMark_;
	
	UI_InventoryItem takenItem_;
	bool takenItemSet_;

	bool clickAction(bool shiftPressed);

	/// текущее состояние кнопки атаки
	bool currentClickAttackState_;
	/// количество квантов со времени отсылки координат
	unsigned short attackCoordTime_;

	/// ожидающая подтверждения операция - перезапись сэйва или реплея и т.п.
	UI_DiskOpID diskOpID_;
	/// true, если игрок разрешил операцию
	bool diskOpConfirmed_;
	/// имя файла для операции
	std::string diskOpPath_;
	/// тип игры для операции
	GameType diskOpGameType_;

	float loadProgress_;
	UI_LoadProgressSectionID loadSection_;
	int loadStep_;

	typedef std::vector<UI_LoadProgressSection> LoadProgressSections;
	LoadProgressSections loadProgressSections_;

	const Vect3f& cursorPosition() const;
	WeaponTarget attackTarget() const;

	void setHoverInfo(const UI_HoverInfo& info);

	const char* enableColorString() const { return "&00FF00"; }
	const char* disableColorString() const { return "&FF0000"; }

	bool makeDiskOp(UI_DiskOpID id, const char* path, GameType game_type);

	string debugCursorReason_;

	int lastCurrentHiVer_;
	int lastCurrentLoVer_;

	typedef pair<int, string> UpdateUrl;
	typedef vector<UpdateUrl> UpdateUrls;
	UpdateUrls updateUrls_;
};

#endif /* __UI_LOGIC_H__ */
