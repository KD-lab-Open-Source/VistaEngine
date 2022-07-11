#ifndef __UI_LOGIC_H__
#define __UI_LOGIC_H__

#include "Handle.h"
#include "Network\NetPlayer.h"
#include "XTL\SwapVector.h"
#include "ProfileManager.h"
#include "Controls.h"
#include "UI_Enums.h"
#include "UI_MarkObject.h"
#include "UI_Inventory.h"
#include "Installer.h"
#include "Units\CircleManagerParam.h"
#include "Units\WeaponTarget.h"

class WeaponPrm;

class cEffect;
class cTexture;

class UnitInterface;
class UnitObjective;
class AttributeBase;

class Player;

class UI_InputEvent;
class UI_ActionData;

class UI_ControlBase;
class UI_ControlCustom;
class UI_ControlUnitList;
class UI_ControlHotKeyInput;

class UI_ActionDataSaveGameList;
class UI_ActionDataPlayer;
class UI_ActionDataHoverInfo;

class UI_NetCenter;
class ChatMessage;

enum eNetMessageCode;

enum DirectControlMode;

struct ExpandInfo
{
	enum Type {
		MESSAGE,
		UNIT,
		CONTROL,
		INVENTORY
	};
	ExpandInfo(Type t, const UnitInterface* u = 0, const AttributeBase* a = 0, const InventoryItem* i = 0, const UI_ControlBase* c = 0) {
		type = t;
		unit = u;
		attr = a;
		item = i;
		control = c;
	}
	Type type;
	const UnitInterface* unit;
	const AttributeBase* attr;
	const InventoryItem* item;
	const UI_ControlBase* control;
};

class UI_LogicDispatcher
{
public:
	UI_LogicDispatcher();
	~UI_LogicDispatcher();

	bool init();
	void destroyLinkQuant();
	void logicPreQuant();
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

	void setSelectedUnitIfOne(UnitInterface* unit) { selectedUnitIfOne_ = unit; }
	UnitInterface* selectedUnitIfOne() const { return selectedUnitIfOne_; }

	/// обработка ввода с клавиатуры или мыши
	/// возвращает true, если событие обработано
	bool handleInput(const UI_InputEvent& event);

	void handleMessage(const ControlMessage& msg);
	void handleMessageReInitGameOptions();

	void updateInput(const UI_InputEvent& event);

	void updateAimPosition();
	
	/// вызывается когда событие \a event обработано контролом \a control
	void inputEventProcessed(const UI_InputEvent& event, UI_ControlBase* control);
	void focusControlProcess(const UI_ControlBase* lastHovered);

	void setHoverControl(const UI_ControlBase* control) { hoverControl_ = control; updateHoverInfo(); }
	void saveCursorPositionInfo();
	void setHoverPosition(const Vect3f& hp, bool inWorld);
	void restoreCursorPositionInfo();

	bool checkInputEventFlag(UI_InputEventID id) const { return ((inputFlags_ & (1 << id)) ? true : false); }
	void setInputEventFlag(UI_InputEventID id){ inputFlags_ |= (1 << id); }
	void clearInputEventFlags(){ inputFlags_ = 0; }

	/// добавление пометки на мире
	void addMark(const UI_MarkObjectInfo& inf);
	void addMovementMark();
	void updateLinkToCursor(const cObject3dx* model, int index, const EffectAttribute& effect);

	void clearSquadSelectSigns();
	void addSquadSelectSign(const Recti& rect, UnitInterface* unit);

	void addUnitOffscreenSprite(const UnitObjective* unit);
	void drawUnitSideSprites();

	/// работа с мышью
	const Vect2f& mousePosition() const { return mousePosition_; }
	void setMousePosition(const Vect2f& pos) { mousePosition_ = pos; }
	bool isMouseFlagSet(ActionFlags flags) const { return ((mouseFlags_ & flags) != 0); }
	ActionModeModifer modifiers() const { return (isMouseFlagSet(MK_SHIFT) ? UI_MOUSE_MODIFER_SHIFT : 0) |( isMouseFlagSet(UI_InputEvent::MK_MENU) ? UI_MOUSE_MODIFER_ALT : 0) | ( isMouseFlagSet(MK_CONTROL) ? UI_MOUSE_MODIFER_CTRL : 0); }

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
	void toggleShowItemsHintMode(bool flag) { showItemHintsMode_ = flag; }
	bool showItemsHintMode() const { return showItemHintsMode_; }

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
	void clearClickMode(){ clearClickMode_ = true; }

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

	ProfileManager& profileSystem() { return profiles_; }
	Profile& currentProfile() { return profiles_.profile(); }

	bool missionIsSelected() const { return currentMission() != 0; }
	const MissionDescription* getMissionByName(const wchar_t* name) const;
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

	const wchar_t* currentPlayerDisplayName(WBuffer& out) const;

	UI_NetCenter& getNetCenter() const;

	void handleChatString(const ChatMessage& _chatMsg);
	void handleSystemMessage(const wchar_t* msg);
	//void handleNetwork(eNetMessageCode msg);
    
	void saveGame(const string& gameName = "", bool autoSave = false);
	void deleteSave();
	void saveReplay();

	UnitInterface* hoverUnit(){ return hoverUnit_; }
	const UnitInterface* hoverUnit() const { return hoverUnit_; }

	bool isAttackPressed() const { return currentClickAttackState_; }
	bool isAttackCursorEnabled() const { return currentHoverInfo_.isAttackEnabled_; }
	bool cursorInWorld() const { return currentHoverInfo_.cursorInWorld_; }
	const Vect3f& hoverPositionTerrain() const { return currentHoverInfo_.hoverTerrainPosition_; }
	const Vect3f& hoverPosition() const { return currentHoverInfo_.hoverPosition_; }
	bool hoverWater() const { return currentHoverInfo_.hoverWater_; }
	
	bool hoverPassable() const { return hoverPassable_; }

	const Vect3f& aimPosition() const { return aimPosition_; }
	float aimDistance() const { return aimDistance_; }

	const UI_ControlBase* hoverControl() const { return hoverControl_; }
	const UI_ActionDataHoverInfo* hoverControlInfo() const { return hoverControlInfo_; }

	Player* player() const;
	int playerID() const;

	void SetSelectPeram(const struct SelectionBorderColor& param);

	void createSignSprites();
	void releaseSignSprites();

	void controlInit(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data);
	void controlUpdate(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data, ControlState& controlState);
	void controlAction(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data);

	void showLoadProgress(float val);

	void setLastHotKeyInput(const UI_ControlHotKeyInput* control){ lastHotKeyInput_ = control; hotKeyUpdated_ = false; }
	bool hotKeyUpdated() const { return hotKeyUpdated_; }

	bool parseGameVersion(const char* updates);
	bool checkNeedUpdate() const;
	void openUpdateUrl() const;
	/// вызывается при необходммости заменить входящие в строку
	/// шаблонные последовательности: {name} на значения
	void expandTextTemplate(wstring& text, const ExpandInfo& info);
	const wchar_t* getParam(const wchar_t* name, const ExpandInfo& info, WBuffer& retBuf);

	bool tipsEnabled() const { return showTips_; }
	void setShowTips(bool showTips) { showTips_ = showTips; }
	bool messagesEnabled() const { return showMessages_; }
	void setShowMessages(bool showMessages) { showMessages_ = showMessages; }

	void setTakenItem(const UI_InventoryItem* item);
	const Vect2f& takenItemOffset() const { return takenItemOffset_; }
	void setTakenItemOffset(const Vect2f& offset){ takenItemOffset_ = offset; }

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
	ActionFlags mouseFlags_;
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
	const MissionDescription* getMissionByName(const wchar_t* name, const UI_ActionDataSaveGameList& data);

	const MissionDescription* getControlStateByGameType(GameTuneOptionType type, ControlState& state, const UI_ActionDataPlayer* data = 0) const;

	typedef std::vector<UI_Sprite> UI_Sprites;
	UI_Sprites signSprites;
	const UI_Sprite& signSprite(int index) const { xassert(index >= 0 && index < signSprites.size()); return signSprites[index]; }

	SquadSelectSigns squadSelectSigns_;
	SideSprites sideSprites_;

	string saveGameName_;
	string saveReplayName_;

	wstring profileName_;

	void buildRaceList(UI_ControlBase* control, int selected, bool withAny = false);

	/// режим отдачи команды после клика по миру - идти, атаковать и т.п.
	UI_ClickModeID clickMode_;
	UI_ClickModeID cachedClickMode_;
	bool clearClickMode_;
	/// оружие, для которого указывается цель
	const WeaponPrm* selectedWeapon_;
	int cachedWeaponID_;
	/// координаты прошлого нажатия левой кнопки мыши
	Vect2f mousePressPos_;
	/// true при селекте прямоугольником
	bool trackMode_;

	const UI_ControlHotKeyInput* lastHotKeyInput_;
	bool hotKeyUpdated_;

	bool showTips_;
	bool showMessages_;

	bool hoverPassable_;
	int traficabilityCommonFlag_;

	UnitInterface* selectedUnit_;
	UnitInterface* selectedUnitIfOne_;

	struct CursorHoverInfo
	{
		CursorHoverInfo()
			: isAttackEnabled_(false)
			, cursorInWorld_(false)
			, hoverWater_(false)
			, hoverPosition_(Vect3f::ZERO)
			, hoverTerrainPosition_(Vect3f::ZERO)
		{}

		bool isAttackEnabled_;
		bool cursorInWorld_;
		bool hoverWater_;
		Vect3f hoverPosition_;
		Vect3f hoverTerrainPosition_;
	};

	CursorHoverInfo currentHoverInfo_;
	CursorHoverInfo savedHoverInfo_;

	Vect3f aimPosition_;
	float aimDistance_;
	
	UnitInterface* hoverUnit_;
	
	UnitInterface* startTrakingUnit_;
	bool showAllUnitParametersMode_;
	bool showItemHintsMode_;

	bool weaponUpKeyPressed_;
	bool weaponDownKeyPressed_;

	const UI_ControlBase* hoverControl_;

	const UI_ActionDataHoverInfo* hoverControlInfo_;
	void updateHoverInfo();

	ComboWStrings chatMessages_;
	typedef vector<LogicTimer> ChatMessageTimes;
	ChatMessageTimes chatMessageTimes_;

	SelectionBorderColor selectionPrm_;
	cTexture* selectionTexture_;
	cTexture* selectionCornerTexture_;
	cTexture* selectionCenterTexture_;

	typedef SwapVector<UI_MarkObject> MarkObjects;
	MarkObjects markObjects_;
	UI_MarkObject cursorMark_;
	UI_LinkToMarkObject linkToCursor_;
	
	UI_InventoryItem takenItem_;
	bool takenItemSet_;
	Vect2f takenItemOffset_;

	const UI_Cursor* currentWeaponCursor_;

	/// контекстное применение клика
	bool clickAction(UI_ClickModeID clMode, int weaponID, UnitInterface* unit, bool shiftPressed);
	/// обработка клика по миру
	bool clickAction(bool shiftPressed, bool byMinimap);

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

	const Vect3f& cursorPosition() const;
	WeaponTarget attackTarget(int weapon_id = 0) const;

	const wchar_t* enableColorString() const { return L"&00FF00"; }
	const wchar_t* disableColorString() const { return L"&FF0000"; }

	bool makeDiskOp(UI_DiskOpID id, const char* path, GameType game_type);

	void handleInGameChatString(const wchar_t* txt);
	
	string debugCursorReason_;

	int lastCurrentHiVer_;
	int lastCurrentLoVer_;

	typedef pair<int, string> UpdateUrl;
	typedef vector<UpdateUrl> UpdateUrls;
	UpdateUrls updateUrls_;
};

#endif /* __UI_LOGIC_H__ */
