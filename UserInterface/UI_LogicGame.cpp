#include "StdAfx.h"

#include "Render\inc\fps.h"
#include "runtime.h"
#include "GameShell.h"
#include "Squad.h"
#include "Universe.h"
#include "Inventory.h"
#include "SelectManager.h"
#include "RenderObjects.h"
#include "vmap.h"
#include "Triggers.h"
#include "IronBuilding.h"
#include "Units\UnitItemInventory.h"
#include "Units\UnitItemResource.h"
#include "Units\UnitPad.h"
#include "Network\P2P_interface.h"
#include "Terra\QSWorldsMgr.h"
#include "AI\PFTrap.h"
#include "Weapon.h"
#include "WeaponPrms.h"
#include "Timers.h"
#include "Water\Water.h"
#include "Environment\Environment.h"
#include "Environment\SourceManager.h"
#include "VistaRender\postEffects.h"
#include "TextDB.h"
#include "FileUtils\FileUtils.h"
#include "Serialization\StringTable.h"
#include "Serialization\SerializationFactory.h"
#include "Render\Src\cCamera.h"
#include "Render\Src\TexLibrary.h"
#include "WBuffer.h"
#include "Controls.h"
#include "GameLoadManager.h"
#include "UI_UnitView.h"
#include "UI_Render.h"
#include "UI_Actions.h"
#include "UI_Logic.h"
#include "UI_Minimap.h"
#include "ShowHead.h"
#include "UserInterface.h"
#include "CommonLocText.h"
#include "UI_Controls.h"
#include "UI_NetCenter.h"
#include "UnicodeConverter.h"
#include "Render\src\Scene.h"

#include "CameraManager.h"
#include "UniverseX.h"

#include "Joystick.h"

#include "UI_StreamVideo.h"
extern Singleton<UI_StreamVideo> streamVideo;

#include "StreamCommand.h"
#include <shellapi.h>

int indexInComboListStringW(const wchar_t* combo_string, const wchar_t* value);
void splitComboListW(ComboWStrings& combo_array, const wchar_t* ptr, wchar_t delimeter);
void joinComboListW(std::wstring& out, const ComboWStrings& strings, wchar_t delimeter);

Singleton<UI_NetCenter> uiNetCenter;

class IdleUnitsManager
{
	typedef UnitInterface UnitType;
	typedef UnitFormationTypes KeyLib;

	typedef vector<UnitType*> UnitList;
	struct IdleTypeNode{
		IdleTypeNode() : count(0), current(0) {}
		int count;
		mutable int current;
		UnitList unitList;
	};

public:
	typedef UnitFormationTypeReference KeyType;
	
	IdleUnitsManager(){}
	void init(){
		idleUnits_.clear();
		KeyLib::Strings::const_iterator it;
		FOR_EACH(KeyLib::instance().strings(), it)
			idleUnits_[KeyType(it->c_str())];
	}

	int idleUnitsCount(const KeyType& type) const
	{
		return idleUnits_[type].count;
	}

	UnitType* getUdleUnit(const KeyType& type) const
	{
		MTG();
		const IdleTypeNode& node = idleUnits_[type];
		if(int size = node.unitList.size()){
			if(node.current >= size)
				node.current = 0;
			return node.unitList[node.current++];
		}
		return 0;
	}

private:
	typedef StaticMap<KeyType, IdleTypeNode> IdleUnits;
	IdleUnits idleUnits_;

	friend void fCommandResetIdleUnits(XBuffer& stream);
	void resetIdleUnits()
	{
		MTG();
		IdleUnits::iterator it;
		FOR_EACH(idleUnits_, it){
			IdleTypeNode& node = it->second;
			node.count = node.unitList.size();
			if(!node.count)
				node.current = 0;
			node.unitList.clear();
		}
	}

	friend void fCommandAddIdleUnits(XBuffer& stream);
	void addIdleUnit(UnitType* unit)
	{
		MTG();
		IdleTypeNode& node = idleUnits_[unit->attr().formationType];
		
		if(UnitType* squad = unit->getSquadPoint()){
			if(std::find(node.unitList.begin(), node.unitList.end(), squad) == node.unitList.end())
				node.unitList.push_back(squad);
		}
		else
			node.unitList.push_back(unit);
	}

};

IdleUnitsManager idleUnitsManager;

class UI_HoverInfo
{
public:
	UI_HoverInfo(const UI_ControlBase* owner_control)
		: ownerControl_(owner_control)
	{	control_ = 0; attr_ = 0; unit_ = 0; }

	bool operator == (const UI_ControlBase* control) const { return ownerControl_ == control; }

	void stop() { aliveTimer_.stop(); }
	bool dead() const { return !aliveTimer_.busy(); }

	void update(const UI_ActionDataHint& action, const UI_ControlBase* hoverControl, const AttributeBase* attr, const UnitInterface* unit) {
		aliveTimer_.start(250);
		if(action.showDelay < FLT_EPS)
			delayTimer_.stop();
		else if(control_ != hoverControl || attr_ != attr || unit_ != unit){
			control_ = hoverControl;
			attr_ = attr;
			unit_ = unit;
			delayTimer_.start(action.showDelay * 1000);
		}
	}

	bool ready() const { return !delayTimer_.busy(); }

private:
	const UI_ControlBase* ownerControl_;
	const UI_ControlBase* control_;
	const AttributeBase* attr_;
	const UnitInterface* unit_;
	GraphicsTimer aliveTimer_;
	GraphicsTimer delayTimer_;
};

typedef vector<UI_HoverInfo> HoverInfos;

class HoverInfoManager
{
	HoverInfos hoverInfos_;
public:
	HoverInfoManager() {}

	UI_HoverInfo& update(const UI_ActionDataHint& action, const UI_ControlBase* control, const UI_ControlBase* hoverControl, const AttributeBase* attr, const UnitInterface* unit)
	{
		MTL();
		HoverInfos::iterator it = std::find(hoverInfos_.begin(), hoverInfos_.end(), control);
		if(it == hoverInfos_.end()){
			hoverInfos_.push_back(UI_HoverInfo(control));
			it = hoverInfos_.end() - 1;
		}
		it->update(action, hoverControl, attr, unit);
		return *it;
	}

	void quant()
	{
		MTL();
		hoverInfos_.erase(remove_if(hoverInfos_.begin(), hoverInfos_.end(), mem_fun_ref(&UI_HoverInfo::dead)), hoverInfos_.end());
	}
};

HoverInfoManager hoverInfoManager;

void fCommandResetIdleUnits(XBuffer& stream)
{
	idleUnitsManager.resetIdleUnits();
}

void fCommandAddIdleUnits(XBuffer& stream)
{
	UnitActing* unit;
	stream.read(unit);
	idleUnitsManager.addIdleUnit(unit);
}

void fCommandSetTakenItem(void* data)
{
	bool* flag_ptr = (bool*)data;

	if(*flag_ptr){
		UI_InventoryItem* item_ptr = (UI_InventoryItem*)(flag_ptr + 1);
		UI_LogicDispatcher::instance().setTakenItem(item_ptr);
	}
	else
		UI_LogicDispatcher::instance().setTakenItem(0);
}

void fCommandSetSourceOnMouse(void* data)
{
	SourceBase** source_ref_ptr = (SourceBase**)data;
	sourceManager->setSourceOnMouse(*source_ref_ptr);
}

void fSetTriggerVariable(void* data)
{
	const char** name = (const char**)(data);
	Singleton<IntVariables>::instance()[*name] = *(int*)(name+1);
}

enum UITextTag {
	UI_TAG_PLAYER_DISPLAY_NAME,
	UI_TAG_PLAYERS_COUNT,
	UI_TAG_CURRENT_GAME_NAME,
	UI_TAG_CURRENT_SERVER_ADDRESS,
	UI_TAG_SELECTED_GAME_NAME,
	UI_TAG_SELECTED_MULTIPLAYER_GAME_NAME,
	UI_TAG_SELECTED_GAME_SIZE,
	UI_TAG_SELECTED_GAME_TYPE,
	UI_TAG_SELECTED_GAME_PLAYERS,
	UI_TAG_CURRENT_CHAT_CHANNEL,
	UI_TAG_TIME_H12,
	UI_TAG_TIME_AMPM,
	UI_TAG_TIME_H24,
	UI_TAG_TIME_M,
	UI_TAG_HOTKEY,
	UI_TAG_ACCESSIBLE, // что надо для доступности юнита
	UI_TAG_ACCESSIBLE_PARAM,  // что надо для доступности параметра
	UI_TAG_LEVEL, // уровень юнита
	UI_TAG_ACCOUNTSIZE, // сколько слотов занимает юнит
	UI_TAG_SUPPLYSLOTS, // сколько слотов занимает юнит (с раскраской)
	UI_TAG_SQUAD_ACCOUNTSIZE, // сколько слотов занимает сквад юнита
	UI_TAG_SQUAD_SUPPLYSLOTS // сколько слотов занимает сквад юнита (с раскраской)
};

struct CharStringLess {
	bool operator() (const wchar_t* left, const wchar_t* right) const {
		return wcscmp(left, right) < 0;
	}
};

typedef StaticMap<const wchar_t*, enum UITextTag, CharStringLess> UI_TextTags;
UI_TextTags uiTextTags;

UI_LogicDispatcher::UI_LogicDispatcher() :
	mousePosition_(Vect2f::ZERO),
	mouseFlags_(0),
	activeCursor_(NULL),
	cursorVisible_(true),
	inputFlags_(0),
	cursorEffect_(NULL),
	cursorTriggered_(false),
	hoverUnit_(0),
	startTrakingUnit_(0),
	selectedUnit_(0),
	selectedUnitIfOne_(0),
	hoverControl_(0),
	hoverControlInfo_(0),
	lastHotKeyInput_(0),
	buildingInstaller_()
{ 
	enableInput_ = true;
	gamePause_ = false;

	weaponUpKeyPressed_ = false;
	weaponDownKeyPressed_ = false;

	selectedWeapon_ = 0;
	currentWeaponCursor_ = 0;
	
	showAllUnitParametersMode_ = false;
	showItemHintsMode_ = false;

	clickMode_ = UI_CLICK_MODE_NONE;
	cachedClickMode_ = UI_CLICK_MODE_NONE;
	clearClickMode_ = false;

	cachedWeaponID_ = 0;
	mousePressPos_ = Vect2f(0,0);
	trackMode_ = false;

	hoverPassable_ = false;
	traficabilityCommonFlag_ = 0;

	aimPosition_ = Vect3f::ZERO;
	aimDistance_ = 0.f;

	selectionTexture_ = 0;
	selectionCornerTexture_ = 0;
	selectionCenterTexture_ = NULL;

	showTips_ = true;
	showMessages_ = true;

	takenItemSet_ = false;
	takenItemOffset_ = Vect2f(0,0);

	currentClickAttackState_ = false;
	attackCoordTime_ = 0;

	currentTeemGametype_ = TEAM_GAME_TYPE_INDIVIDUAL;

	diskOpID_ = UI_DISK_OP_NONE;
	diskOpConfirmed_ = false;

	lastCurrentHiVer_ = UI_Dispatcher::instance().gameMajorVersion();
	lastCurrentLoVer_ = UI_Dispatcher::instance().gameMinorVersion();

	uiTextTags.insert(make_pair(L"display_name", UI_TAG_PLAYER_DISPLAY_NAME)); // в online - логин, иначе имя профиля
	uiTextTags.insert(make_pair(L"players_count", UI_TAG_PLAYERS_COUNT)); // количество оставшихся игроков
	uiTextTags.insert(make_pair(L"game_name", UI_TAG_CURRENT_GAME_NAME)); // название текущей загруженной карты
	uiTextTags.insert(make_pair(L"server_address", UI_TAG_CURRENT_SERVER_ADDRESS)); // адрес текущего сервера для подключения
	uiTextTags.insert(make_pair(L"cm_name", UI_TAG_SELECTED_GAME_NAME)); // название выбранной в списке карты
	uiTextTags.insert(make_pair(L"multi_name", UI_TAG_SELECTED_MULTIPLAYER_GAME_NAME)); // название выбранной сетевой игры
	uiTextTags.insert(make_pair(L"cm_size", UI_TAG_SELECTED_GAME_SIZE)); // размер выбранной карты
	uiTextTags.insert(make_pair(L"cm_type", UI_TAG_SELECTED_GAME_TYPE)); // тип (custom, predefine) выбранной карты
	uiTextTags.insert(make_pair(L"cm_players", UI_TAG_SELECTED_GAME_PLAYERS)); // максимальное количество игроков
	uiTextTags.insert(make_pair(L"channel_name", UI_TAG_CURRENT_CHAT_CHANNEL)); //название текущего канала или статус подключения
	uiTextTags.insert(make_pair(L"hotkey", UI_TAG_HOTKEY)); // горячая клавиша для кнопки
	uiTextTags.insert(make_pair(L"accessible", UI_TAG_ACCESSIBLE)); // что надо для доступности юнита
	uiTextTags.insert(make_pair(L"accessible_param", UI_TAG_ACCESSIBLE_PARAM)); // что надо для доступности параметра
	uiTextTags.insert(make_pair(L"level", UI_TAG_LEVEL)); // уровень юнита
	uiTextTags.insert(make_pair(L"accountsize", UI_TAG_ACCOUNTSIZE)); // сколько слотов занимает юнит
	uiTextTags.insert(make_pair(L"supplyslots", UI_TAG_SUPPLYSLOTS)); // сколько слотов занимает юнит (с раскраской)
	uiTextTags.insert(make_pair(L"squadaccounts", UI_TAG_SQUAD_ACCOUNTSIZE)); // сколько слотов занимает сквад юнита
	uiTextTags.insert(make_pair(L"squadslots", UI_TAG_SQUAD_SUPPLYSLOTS)); // сколько слотов занимает сквад юнита (с раскраской)
	uiTextTags.insert(make_pair(L"time_h12", UI_TAG_TIME_H12)); // время мира в 12 часовом формате
	uiTextTags.insert(make_pair(L"time_ampm", UI_TAG_TIME_AMPM)); // до полудня или после полудня
	uiTextTags.insert(make_pair(L"time_h24", UI_TAG_TIME_H24)); // вреям мира в 24 часовом формате
	uiTextTags.insert(make_pair(L"time_min", UI_TAG_TIME_M)); // минуты времени мира
}

UI_LogicDispatcher::~UI_LogicDispatcher()
{
	dassert(!cursorEffect_);
}

bool UI_LogicDispatcher::init()
{
	for (UI_CursorLibrary::Strings::const_iterator itc = UI_CursorLibrary::instance().strings().begin(); itc != UI_CursorLibrary::instance().strings().end(); ++itc)
		if(itc->get())
			itc->get()->createCursor();

	setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_WAITING));

	UI_Render::instance().setWindowPosition(aspectedWorkArea(Rectf(0, 0, gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY()), 4.0f / 3.0f));

	createSignSprites();

	UI_UnitView::instance().init();

	showHead().init();

	idleUnitsManager.init();

	missions_.readFromDir("RESOURCE\\WORLDS\\", GAME_TYPE_SCENARIO);

	for(MissionDescriptions::const_iterator qsm = missions_.begin(); qsm != missions_.end(); ++qsm)
		if(qsm->isBattle() && qsWorldsMgr.isMissionPresent(qsm->missionGUID()))
			quickStartMissions_.push_back(qsm->missionGUID());

	validateMissionFilter(currentProfile().findMissionFilter);

	GUIDcontainer::iterator it = currentProfile().quickStartMissionFilter.filterList.begin();
	while(it != currentProfile().quickStartMissionFilter.filterList.end())
		if(!qsWorldsMgr.isMissionPresent(*it))
			it = currentProfile().quickStartMissionFilter.filterList.erase(it);
		else
			++it;
	validateMissionFilter(currentProfile().quickStartMissionFilter);

	return true;
}

const GUIDcontainer& UI_LogicDispatcher::quickStartFilter()
{
	return currentProfile().quickStartMissionFilter.filterDisabled
		? quickStartMissions_
		: currentProfile().quickStartMissionFilter.filterList;
}

void UI_LogicDispatcher::validateMissionFilter(Profile::MissionFilter& filter)
{
	if(filter.filterDisabled){
		filter.filterList.clear();
		return;
	}

	GUIDcontainer::iterator it = filter.filterList.begin();
	while(it != filter.filterList.end())
		if(!missions_.find(*it))
			it = filter.filterList.erase(it);
		else
			++it;
	
	if(filter.filterList.empty())
		filter.filterDisabled = true;
}

void UI_LogicDispatcher::createSignSprites()
{
	releaseSignSprites();
	
	for(int idx = 0; idx <= GlobalAttributes::instance().playerAllowedSignSize(); ++idx)
		signSprites.push_back(GlobalAttributes::instance().playerSigns[idx].sprite);
}

void UI_LogicDispatcher::releaseSignSprites()
{
	for(UI_Sprites::iterator its = signSprites.begin(); its != signSprites.end(); ++its)
		its->release();
	signSprites.clear();
}

void UI_LogicDispatcher::redraw()
{
	start_timer_auto();

	if(takenItemSet_)
		takenItem_.redraw(mousePosition() - takenItemOffset_, 1.f);

	if(trackMode_)
		drawSelection(mousePressPos_, mousePosition());
}

Vect3f G2S(const Vect3f &vg);

void UI_LogicDispatcher::drawDebug2D() const
{
	if(showDebugInterface.hoveredControlBorder){
		if(const UI_ControlBase* hovered_control = hoverControl_){
			UI_Render::instance().drawRectangle(hovered_control->transfPosition(), Color4f(0.5f, 1.f, 0.5f, 0.5f), true);
			Rectf pos(hovered_control->textPosition());
			if(!pos.eq(hovered_control->transfPosition(), 0.001f))
				UI_Render::instance().drawRectangle(pos, Color4f(.5f, 1.f, .5f, .9f), true);

			if(!hovered_control->mask().isEmpty())
				hovered_control->mask().draw(hovered_control->transfPosition().left_top(), Color4f(0.5f, 1.f, 0.5f, 0.5f));

			UI_ControlReference ref(hovered_control);
			XBuffer buf;
			buf.SetDigits(2);
			buf < ref.referenceString();
			if(showDebugInterface.hoveredControlExtInfo){
				UI_TextVAlign va = (UI_TextVAlign)(hovered_control->textAlign() & UI_TEXT_VALIGN);
				UI_TextAlign ha = (UI_TextAlign)(hovered_control->textAlign() & UI_TEXT_ALIGN);
				buf < '\n' < typeid(*hovered_control).name() < '\n'
				< (hovered_control->isEnabled() ? "enabled" : "DISABLED")
				< (va != UI_TEXT_VALIGN_TOP ? (va == UI_TEXT_VALIGN_CENTER ? ", Vcenter" : ", Vbottom") : "")
				< (ha != UI_TEXT_ALIGN_LEFT ? (ha == UI_TEXT_ALIGN_CENTER ? ", Hcenter" : ", Hright") : "")
				< (hovered_control->isActivated() ? ", Active\n" : "\n")
				< "Current state: " <= hovered_control->currentStateIndex();
				if(hovered_control->currentStateIndex() >= 0)
					buf < "; Name: " < hovered_control->state(hovered_control->currentStateIndex())->name();
				const UI_ActionDataHotKey* hotKeyAction = safe_cast<const UI_ActionDataHotKey*>(hovered_control->findAction(UI_ACTION_HOT_KEY));
				WBuffer keyName;
				if(hotKeyAction && hotKeyAction->hotKey() != sKey())
					buf < "\nHotkey: " < w2a(hotKeyAction->hotKey().toString(keyName)).c_str();
			}
			if(showDebugInterface.showTransformInfo){
				if(buf.tell()) 
					buf < "\n";
				hovered_control->getDebugString(buf);
			}
			
			UI_Render::instance().outDebugText(mousePosition_ + Vect2f(0.02f, 0.03f), buf.c_str(), &Color4c(120, 255, 120));
		}
	}
/*
	if(showDebugInterface.showAimPosition && gameShell->directControl_){
		const Vect2f sz1(24.f/1024.f, 6.f/768.f);
		const Vect2f sz2(6.f/1024.f, 24.f/768.f);

		UI_Render::instance().drawRectangle(Rectf(0.5f - sz1.x/2.f, 0.5f-sz1.y/2.f, sz1.x, sz1.y));
		UI_Render::instance().drawRectangle(Rectf(0.5f - sz2.x/2.f, 0.5f-sz2.y/2.f, sz2.x, sz2.y));
	}
*/	
	XBuffer out;
	if(showDebugInterface.cursorReason){
		Rectf crs(Vect2f(10.f / 1024.f, 10.f / 768.f));
		crs.center(mousePosition_);
		UI_Render::instance().drawRectangle(crs);
		out < "Cursor reason: " < debugCursorReason_.c_str();
	}

	if(showDebugInterface.logicDispatcher){
		if(out.tell()) 
			out <  "\n";
		out < "nameToSaveGame: " < saveGameName_.c_str()
			< ", nameToSaveReplay: " < saveReplayName_.c_str()
			< ", nameToCreateProfile: " < w2a(profileName_).c_str()
			< "\nCurrentMission: ";
		selectedMission_.getDebugInfo(out);
	}

	if(showDebugInterface.showDebugJoystick){
		if(out.tell()) 
			out <  "\n";
		out < "Joystick buttons pressed:";

		for(int i = 1; i <= JOYSTICK_BUTTONS_MAX; i++){
			if(joystickState.isControlPressed(JoystickControlID(JOY_BUTTON_01 + i)))
				out < " " <= i;
		}

		out < "\nJoystick Axis -> " <= joystickState.controlState(JOY_AXIS_X) < " " <= joystickState.controlState(JOY_AXIS_Y) < " " <= joystickState.controlState(JOY_AXIS_Z);
		out < "\nJoystick Rotation axis -> " <= joystickState.controlState(JOY_AXIS_X_ROT) < " " <= joystickState.controlState(JOY_AXIS_Y_ROT) < " " <= joystickState.controlState(JOY_AXIS_Z_ROT);
	}

	if(out.tell())
		UI_Render::instance().outDebugText(Vect2f(0.f, .25f), out.c_str());

}

void UI_LogicDispatcher::showDebugInfo() const
{
	if(showDebugInterface.marks){
		if(const WeaponPrm* wpn = selectedWeapon_){
			if(!takenItemSet_ && (clickMode_ == UI_CLICK_MODE_ATTACK || clickMode_ == UI_CLICK_MODE_PLAYER_ATTACK) && isAttackCursorEnabled())
				cursorMark_.showDebugInfo();
			linkToCursor_.showDebugInfo();
		}

		for(MarkObjects::const_iterator it = markObjects_.begin(); it != markObjects_.end(); ++it)
			it->showDebugInfo();
	}

	if(showDebugInterface.showAimPosition){
		show_vector(aimPosition_, 5.f, Color4c::GREEN);
	}

	if(showDebugInterface.showInventoryItemInfo)
	if(const UnitInterface* un = selectedUnitIfOne())
		if(const InventorySet* inv = un->inventory()){
			const InventoryItem* item = 0;
			if(inv->takenItem().attribute())
				item = &inv->takenItem();
			else if(const UI_ControlBase* p = hoverControl_)
				if(const UI_ControlInventory* uinv = dynamic_cast<const UI_ControlInventory*>(p))
					item = uinv->hoverItem(inv, mousePosition());
			if(item)
				item->showDebugInfo();
		}

}

void UI_LogicDispatcher::drawDebugInfo() const
{
	if(showDebugInterface.hoverUnitBound){
		if(const UnitInterface* abstract = hoverUnit()){
			const UnitReal* unit = abstract->getUnitReal();
			const cObject3dx* model = unit->modelLogic();
			if(!model)
				model = unit->model();
			xassert(model);
			if(unit->attr().selectBySphere){
				extern void clip_circle_3D(const Vect3f& vc, float radius, Color4c color);

				sBox6f box;
				model->GetBoundBox(box);
				float radius = box.max.distance(box.min) * 0.5f;
				Vect3f center(model->GetPosition() * ((box.max + box.min) * 0.5f));
				
				float t = 0.95f * radius;
				for(float z_add = -t; z_add <= t; z_add += t / 6.f){
					float rad = sqrtf(sqr(radius) - sqr(z_add));
					clip_circle_3D(Vect3f(center.x, center.y, center.z + z_add), rad, Color4c(255, 0, 0));
				}
			}
			else
				model->DrawBound();
		}
	}
}

void UI_LogicDispatcher::sendBuildMessages(const AttributeBase* buildingAttr) const
{
	if(const Player* pl = player()){
		if(!pl->checkUnitNumber(buildingAttr))
			UI_Dispatcher::instance().sendMessage(UI_MESSAGE_UNIT_LIMIT_REACHED);
		if(!pl->requestResource(buildingAttr->installValue, NEED_RESOURCE_TO_INSTALL_BUILDING))
			UI_Dispatcher::instance().sendMessage(UI_MESSAGE_NOT_ENOUGH_RESOURCES_FOR_BUILDING);
	}
}

void UI_LogicDispatcher::destroyLinkQuant()
{
	if(UnitInterface* unit = hoverUnit_)
		if(!unit->alive())
			hoverUnit_ = 0;
	if(UnitInterface* unit = startTrakingUnit_)
		if(!unit->alive())
			startTrakingUnit_ = 0;
	if(UnitInterface* unit = selectedUnit_)
		if(!unit->alive())
			selectedUnit_ = 0;
	if(UnitInterface* unit = selectedUnitIfOne_)
		if(!unit->alive())
			selectedUnitIfOne_ = 0;
}

void UI_LogicDispatcher::logicPreQuant()
{
	start_timer_auto();

	xassert(selectManager && gameShell->GameActive);
	setSelectedUnit(selectManager->selectedUnit());
	setSelectedUnitIfOne(selectManager->getUnitIfOne());
	xassert(pathFinder);

	if(UnitInterface* p = selectedUnit())
	{
		UI_UnitView::instance().setAttribute(&p->getUnitReal()->attr());
		Vect3f hovPos = hoverPositionTerrain();
		hoverPassable_ = !pathFinder->checkImpassability(hovPos.xi(), hovPos.yi(), traficabilityCommonFlag_);
	}
	else {
		UI_UnitView::instance().setAttribute(0);
		hoverPassable_ = true;
	}
}

void UI_LogicDispatcher::logicQuant(float dt)
{
	start_timer_auto();

	if(UnitInterface* unit = selectedUnitIfOne()){
		if(const InventorySet* ip = unit->inventory()){
			ip->updateUI();

			uiStreamCommand.set(fCommandSetTakenItem);
			if(ip->takenItem()())
				uiStreamCommand << true << UI_InventoryItem(ip->takenItem(), UI_INVENTORY);
			else
				uiStreamCommand << false;
		}
		else 
			uiStreamCommand.set(fCommandSetTakenItem) << false;
	}
	else
		uiStreamCommand.set(fCommandSetTakenItem) << false;

	xassert(chatMessages_.size() == chatMessageTimes_.size());
	ChatMessageTimes::iterator tit = chatMessageTimes_.begin();
	ComboWStrings::iterator sit = chatMessages_.begin();
	while(tit != chatMessageTimes_.end())
		if(tit->finished()){
			tit = chatMessageTimes_.erase(tit);
			sit = chatMessages_.erase(sit);
		}
		else {
			++tit;
			++sit;
		}

	hoverInfoManager.quant();

	UI_UnitView::instance().logicQuant(dt);

	UI_BackgroundScene::instance().logicQuant(dt);

	toggleShowAllParametersMode(ControlManager::instance().key(CTRL_SHOW_UNIT_PARAMETERS).pressed());
	toggleShowItemsHintMode(ControlManager::instance().key(CTRL_SHOW_ITEMS_HINT).pressed());
}

void fSendHoverMouseCommand(void* data)
{
	universe()->activePlayer()->sendCommand(UnitCommand(COMMAND_ID_DIRECT_SHOOT_MOUSE,  UI_LogicDispatcher::instance().hoverPosition()));
}

void fSendSetMouseCommand(void* data)
{
	universe()->activePlayer()->sendCommand(UnitCommand(COMMAND_ID_DIRECT_MOUSE_SET,  UI_LogicDispatcher::instance().hoverPosition()));
}

void UI_LogicDispatcher::directShootQuant()
{
	if(player()->updateShootPoint())
		uiStreamCommand.set(fSendSetMouseCommand);
	else if(currentClickAttackState_){
		if(attackCoordTime_++ == universe()->directShootInterpotatePeriod()){
			attackCoordTime_ = 0;
			uiStreamCommand.set(fSendHoverMouseCommand);
		}
	}
	else
		attackCoordTime_ = 0;
}

void UI_LogicDispatcher::reset()
{
	buildingInstaller_.Clear();

	UI_UnitView::instance().setAttribute(0);
	showHead().resetHead();

	hoverUnit_ = 0;
	startTrakingUnit_ = 0;

	currentHoverInfo_ = CursorHoverInfo();
	savedHoverInfo_ = CursorHoverInfo();

	hoverPassable_ = false;
	traficabilityCommonFlag_ = 0;

	aimPosition_ = Vect3f::ZERO;
	aimDistance_ = 0.f;

	cachedClickMode_ = clickMode_ = UI_CLICK_MODE_NONE;
	selectedWeapon_ = 0;
	currentWeaponCursor_ = 0;
	cachedWeaponID_ = 0;
	
	hoverControl_ = 0;
	hoverControlInfo_ = 0;

	RELEASE(cursorEffect_);
	activeCursor_ = 0;

	cursorMark_.clear();
	linkToCursor_.release();
	showCursor();

	selectedUnit_ = 0;
	selectedUnitIfOne_ = 0;

	for(MarkObjects::iterator it = markObjects_.begin(); it != markObjects_.end(); ++it)
		it->clear();

	markObjects_.clear();
	
	clearGameChat();
}

void UI_LogicDispatcher::graphQuant(float dt)
{
	start_timer_auto();

	if(clearClickMode_){
		selectClickMode(UI_CLICK_MODE_NONE);
		clearClickMode_ = false;
	}

	cachedClickMode_ = clickMode();
	cachedWeaponID_ = selectedWeaponID();

	if(JoystickSetup::instance().isEnabled() && gameShell->underFullDirectControl()){
		bool keypress = joystickState.isControlPressed(JoystickSetup::instance().controlSetup(JOY_NEXT_WEAPON));
		if(keypress && !weaponUpKeyPressed_)
			selectManager->makeCommand(UnitCommand(COMMAND_ID_DIRECT_CHANGE_WEAPON, 1), true);
		weaponUpKeyPressed_ = keypress;

		keypress = joystickState.isControlPressed(JoystickSetup::instance().controlSetup(JOY_PREV_WEAPON));
		if(keypress && !weaponDownKeyPressed_)
			selectManager->makeCommand(UnitCommand(COMMAND_ID_DIRECT_CHANGE_WEAPON, -1), true);
		weaponDownKeyPressed_ = keypress;
	}

	if(Player* pl = player()){
		if(gameShell->directControl() && (pl->isWin() || pl->isDefeat()))
			disableDirectControl();
	}

	buildingInstaller_.quant(universe()->activePlayer(), cameraManager->GetCamera());

	Vect2i mouse_screan = UI_Render::instance().screenCoords(mousePosition());
	Vect2f mouse_pos = UI_Render::instance().deviceCoords(mouse_screan);

	UnitInterface* unitNear = 0;
	if(!hoverControl_)
	{
		SquadSelectSigns::const_reverse_iterator it = squadSelectSigns_.rbegin();
		SquadSelectSigns::const_reverse_iterator it_end = squadSelectSigns_.rend();
		for(; it != it_end; ++it)
			if(it->check(mouse_screan)){
				it->unit()->hover();
				unitNear = it->unit();
				break;
			}

		if(!unitNear){
			Vect3f v0, v1;
			cameraManager->calcRayIntersection(mouse_pos, v0, v1);
			float distMin = v1.distance2(v0);

			if(unitNear = gameShell->unitHover(v0, v1, distMin)){
				Vect3f dirPoint = v1 - v0;
				dirPoint.normalize(clamp(sqrtf(distMin) - 15.f, 1.f, 10000.f));
				if(!weapon_helpers::traceGround(v0, v0+dirPoint, dirPoint))
					unitNear = 0; // земля ближе
			}
		}
	}
	hoverUnit_ = unitNear;

	if(!gameShell->cameraMouseTrack){
		Vect3f hoverPos;
		bool inWorld = cameraManager->cursorTrace(mouse_pos, hoverPos);
		setHoverPosition(hoverPos, inWorld);
	}
	else
		setHoverPosition(currentHoverInfo_.hoverPosition_, currentHoverInfo_.cursorInWorld_);

	traficabilityCommonFlag_ = selectManager->getTraficabilityCommonFlag();

	for(MarkObjects::iterator it1 = markObjects_.begin(); it1 != markObjects_.end();)
		if(it1->quant(dt))
			++it1;
		else {
			it1->clear();
			it1 = markObjects_.erase(it1);
		}

	const WeaponPrm* wpn = selectedWeapon_;
	const UI_Cursor* weaponCursor = 0;

	if(takenItemSet_){
		hideCursor();
	}
	else if((clickMode_ == UI_CLICK_MODE_ATTACK || clickMode_ == UI_CLICK_MODE_PLAYER_ATTACK) && wpn && isAttackCursorEnabled()){
		const UnitInterface* owner = selectManager->selectedUnit();
		if(clickMode_ == UI_CLICK_MODE_PLAYER_ATTACK)
			if(Player* pl = player())
				owner = pl->playerUnit();
		
		WeaponBase* weapon = safe_cast<const UnitActing*>(owner->getUnitReal())->findWeapon(wpn->ID());
		const UI_MarkObjectAttribute& targetMark = wpn->targetMark(weapon->analyseMarkObject(hoverUnit_));

		if(wpn->hideCursor())
			hideCursor();
		else if(targetMark.cursor().cursor())
			weaponCursor = &targetMark.cursor();

		cursorMark_.update(UI_MarkObjectInfo(
			UI_MARK_ATTACK_TARGET, 
			Se3f(targetMark.rotateWithCamera() ? QuatF(cameraManager->coordinate().psi(), Vect3f::K) : QuatF::ID, cursorPosition()),
			&targetMark, owner, hoverUnit_));

		cursorMark_.quant(dt);

		if(owner && wpn->weaponClass() == WeaponPrm::WEAPON_PAD && (owner = owner->getUnitReal())->attr().isActing()){
			bool updated = false;
			if(UnitPlayer* up = player()->playerUnit())
				if(UnitPad* pad = up->getPad(wpn->ID())){
					const WeaponPadPrm& padprm = *safe_cast<const WeaponPadPrm*>(wpn);
					if(padprm.linkToCursor().isEmpty() || linkToCursor_.attr() != &padprm.linkToCursor())
						linkToCursor_.release();

					if(!padprm.linkToCursor().isEmpty() && !linkToCursor_.isEnabled()){
						linkToCursor_.setOwner(&cursorMark_);
						linkToCursor_.effectStart(&padprm.linkToCursor());
					}

					linkToCursor_.updateLink(pad->model(), padprm.linkNodeIndex());
					updated = true;
				}
			if(!updated)
				linkToCursor_.release();
		}
		else
			linkToCursor_.release();
	}
	else {
		cursorMark_.clear();
		linkToCursor_.release();
		showCursor();
	}
	currentWeaponCursor_ = weaponCursor;

	if(trackMode_){
		if(selectManager->selectArea(
			UI_Render::instance().relative2deviceCoords(mousePressPos_),
			UI_Render::instance().relative2deviceCoords(mousePosition()),
			isShiftPressed(), startTrakingUnit_)){
				if(hoverUnit_)
					selectManager->selectUnit(hoverUnit_, true, true);
			}

		if(!ControlManager::instance().key(CTRL_SELECT).keyPressed(KBD_SHIFT|KBD_MENU))
			toggleTracking(false);
	}

	if(Player* pl = player()){
		if(pl->shootFailed()){
			if(clickMode_ == UI_CLICK_MODE_ATTACK || clickMode_ == UI_CLICK_MODE_PLAYER_ATTACK)
				selectClickMode(UI_CLICK_MODE_NONE);
			pl->setShootFailed(false);
		}
	}

	if(diskOpID_ != UI_DISK_OP_NONE){
		if(checkDiskOp(diskOpID_, diskOpPath_.c_str()))
			makeDiskOp(diskOpID_, diskOpPath_.c_str(), diskOpGameType_);
	}
}

void UI_LogicDispatcher::updateLinkToCursor(const cObject3dx* model, int nodeIndex, const EffectAttribute& effect)
{
	MTG();
	if(effect.isEmpty() || linkToCursor_.attr() != &effect)
		linkToCursor_.release();

	if(!effect.isEmpty() && !linkToCursor_.isEnabled()){
		linkToCursor_.setOwner(&cursorMark_);
		linkToCursor_.effectStart(&effect);
	}
		
	linkToCursor_.updateLink(model, nodeIndex);
}

bool UI_LogicDispatcher::handleInput(const UI_InputEvent& event)
{
	if(!isGameActive())
		return false;
		
	switch(event.ID()){
		case UI_INPUT_MOUSE_MOVE:
			if(buildingInstaller_.inited())
				if(isMouseFlagSet(MK_CONTROL))
					buildingInstaller_.SetBuildPosition(buildingInstaller_.position(), buildingInstaller_.angle() - gameShell->mousePositionDelta().x*35);
				else
					buildingInstaller_.SetBuildPosition(hoverPosition(), buildingInstaller_.angle());
			return false;

		case UI_INPUT_MOUSE_LBUTTON_DOWN:
			if(buildingInstaller_.inited()){
				buildingInstaller_.ConstructObject(universe()->activePlayer(), selectManager->selectedUnit());
				return false;
			}
		case UI_INPUT_MOUSE_RBUTTON_DOWN:
			if(buildingInstaller_.inited()){
				buildingInstaller_.Clear();
				return false;
			}

			mousePressPos_ = mousePosition();
	}

	// для мышино-кнопочных действий, shift независимо от настроек является флагом постановки в очередь (добавления к селекту), ALT во
	int mask = ~(event.isMouseEvent() ? KBD_SHIFT|KBD_MENU : KBD_SHIFT);
	int withoutShiftCode = event.keyCode() & mask;
	bool shiftPressed = event.keyCode() & KBD_SHIFT;

	if((ControlManager::instance().key(CTRL_CLICK_ACTION).fullkey & mask) == withoutShiftCode){
		if(clickAction(shiftPressed, event.isFlag(UI_InputEvent::BY_MINIMAP)))
			return false;
	}
	else if(event.isMouseClickEvent()){
		const WeaponPrm* prm = selectedWeapon();
		if(prm && prm->ignoreMouseDblClick() && event.isMouseDblClick())
			return false;
		if(!prm || !hoverUnit_)
			if(clickMode_ != UI_CLICK_MODE_NONE){
				if(event.ID() == UI_INPUT_MOUSE_MBUTTON_DOWN && selectedWeapon() && selectedWeapon()->weaponClass() == WeaponPrm::WEAPON_PAD)
					return false;
				selectClickMode(UI_CLICK_MODE_NONE);
				return false;
			}
	}

	switch(event.ID()){
	case UI_INPUT_MOUSE_LBUTTON_DOWN:
		if(!isMouseFlagSet(MK_RBUTTON))
#ifndef _FINAL_VERSION_
			if(DebugPrm::instance().debugClickKillMode){
				if(hoverUnit_)
					hoverUnit_->sendCommand(UnitCommand(COMMAND_ID_KILL_UNIT));
			}
#endif
			if(UnitInterface* p = selectManager->getUnitIfOne())
				if(const InventorySet* ip = p->inventory()){
					if(ip->takenItem()()){
						if(hoverUnit_ && hoverUnit_->player() == universe()->activePlayer()){
							p->sendCommand(UnitCommand(COMMAND_ID_ITEM_TAKEN_TRANSFER, hoverUnit_));
						}
						else
							p->sendCommand(UnitCommand(COMMAND_ID_ITEM_TAKEN_DROP, hoverPosition()));
						return false;
					}
				}
		break;
	
	case UI_INPUT_MOUSE_RBUTTON_DOWN:
		if(UnitInterface* unit = selectManager->getUnitIfOne())
			if(const InventorySet* ip = unit->inventory())
				if(ip->takenItem()()){
					unit->sendCommand(UnitCommand(COMMAND_ID_ITEM_RETURN, -1));
					return false;
				}
		break;
/*
	case UI_INPUT_MOUSE_MBUTTON_DOWN:
		if(gameShell->underFullDirectControl()){
			if(UnitInterface* selected_unit = selectedUnit()){
				UnitReal* selected_unit_real = selected_unit->getUnitReal();
				UnitInterface* ip = hoverUnit_;
				if(ip && ip->attr().isTransport()){
					UnitActing* p = safe_cast<UnitActing*>(ip);
					if(p->canPutInTransport(selected_unit_real) && 
						(p->position2D().distance2(selected_unit_real->position2D()) < 
						sqr(p->radius() + selected_unit_real->radius() + p->attr().transportLoadDirectControlRadius))){
						selectManager->selectUnitForced(p);
						selected_unit_real->sendCommand(UnitCommand(COMMAND_ID_DIRECT_PUT_IN_TRANSPORT, p));
					}
				}else if(selected_unit_real->attr().isTransport()){
					selected_unit_real->sendCommand(UnitCommand(COMMAND_ID_DIRECT_PUT_OUT_TRANSPORT));
				}
			}
		}
		break;
*/
	case UI_INPUT_MOUSE_WHEEL_UP:
		if(gameShell->directControl())
			selectManager->makeCommand(UnitCommand(COMMAND_ID_DIRECT_CHANGE_WEAPON, 1), true);
		break;

	case UI_INPUT_MOUSE_WHEEL_DOWN:
		if(gameShell->directControl())
			selectManager->makeCommand(UnitCommand(COMMAND_ID_DIRECT_CHANGE_WEAPON, -1), true);
		break;
	}

	if(!gameShell->directControl())
	{
		if((ControlManager::instance().key(CTRL_SELECT).fullkey & mask) == withoutShiftCode){
			if(!trackMode_){
				if(hoverUnit_ && hoverUnit_->player() == universe()->activePlayer()){
					startTrakingUnit_ = hoverUnit_;
					selectManager->selectUnit(hoverUnit_, shiftPressed);
				}
				else if(!shiftPressed)
					selectManager->deselectAll();
				toggleTracking(true);
			}
		}
	}
	
	if(!gameShell->underFullDirectControl())
	{
		if(ControlManager::instance().key(CTRL_SELECT_ALL) == event.keyCode()){
			if(hoverUnit_){
				selectManager->selectAllOnScreen(hoverUnit_->selectionAttribute());
			}
		}
		
		if((ControlManager::instance().key(CTRL_ATTACK).fullkey & mask) == withoutShiftCode){
			if(hoverUnit_){
				UnitCommand unitCommand(COMMAND_ID_OBJECT, hoverUnit_->getUnitReal(), hoverPosition(), 0);
				unitCommand.setShiftModifier(shiftPressed);
				unitCommand.setMiniMap(event.isFlag(UI_InputEvent::BY_MINIMAP));
				selectManager->makeCommand(unitCommand);
				if(hoverUnit_->player()->clan() != universe()->activePlayer()->clan() && hoverUnit_->player() != universe()->worldPlayer())
					addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, Se3f(QuatF::ID, cursorPosition()), &universe()->activePlayer()->race()->orderMark(UI_CLICK_MARK_ATTACK_UNIT), selectManager->selectedUnit(), hoverUnit_));
			}
		}
		else {
			if(gameShell->underHalfDirectControl() && (ControlManager::instance().key(CTRL_ATTACK_ALTERNATE).fullkey & mask) == withoutShiftCode){
				if(UnitInterface* p = selectedUnit()){
					if((p = p->getUnitReal())->attr().isActing()){
						UnitActing* unit = safe_cast<UnitActing*>(p);
						int weapon_id = unit->directControlWeaponID();

						WeaponTarget target = attackTarget(weapon_id);
						if(selectManager->canAttack(target)){
							selectManager->makeCommandAttack(target, shiftPressed, event.isFlag(UI_InputEvent::BY_MINIMAP), false);

							if(const Player* pl = player()){
								if(hoverUnit_)
									addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, Se3f(QuatF::ID, cursorPosition()), &pl->race()->orderMark(UI_CLICK_MARK_ATTACK_UNIT), selectManager->selectedUnit(), hoverUnit_));
								else
									addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, Se3f(QuatF::ID, cursorPosition()), &pl->race()->orderMark(UI_CLICK_MARK_ATTACK), selectManager->selectedUnit(), hoverUnit_));
							}
						}
					}
				}
			}
		}
		
		if((ControlManager::instance().key(CTRL_MOVE).fullkey & mask) == withoutShiftCode){
			if(!selectManager->isSelectionEmpty()){
				if(hoverUnit_){
					UnitCommand unitCommand(COMMAND_ID_OBJECT, hoverUnit_->getUnitReal(), hoverPosition(), 0);
					unitCommand.setShiftModifier(shiftPressed);
					unitCommand.setMiniMap(event.isFlag(UI_InputEvent::BY_MINIMAP));
					selectManager->makeCommand(unitCommand);
					if(hoverUnit_->player()->clan() == universe()->activePlayer()->clan() || hoverUnit_->player() == universe()->worldPlayer())
						addMovementMark();
				}
				else {
					selectManager->makeCommandSubtle(COMMAND_ID_POINT, cursorPosition(), shiftPressed, event.isFlag(UI_InputEvent::BY_MINIMAP));
					addMovementMark();
				}
			}
		}
	}

	return false;
}

void UI_LogicDispatcher::handleMessage(const ControlMessage& msg)
{
	UI_Dispatcher::instance().handleMessage(msg);
}

void UI_LogicDispatcher::handleMessageReInitGameOptions()
{
	UI_Dispatcher::instance().handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_OPTION, UI_ActionDataControlCommand::RE_INIT)));
	UI_Dispatcher::instance().handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_OPTION_PRESET_LIST, UI_ActionDataControlCommand::RE_INIT)));
}

void UI_LogicDispatcher::updateInput(const UI_InputEvent& event)
{
	setInputEventFlag(event.ID());

	if(event.isMouseEvent()){
		mousePosition_ = event.cursorPos();
		if(event.hasMouseFlags())
			mouseFlags_ = event.mouseFlags();
	}

	if(isGameActive() && universe()->activePlayer()){
		bool shootPressed = false;

		if(!gameShell->underFullDirectControl())
			shootPressed = !gameShell->isPaused(GameShell::PAUSE_BY_ANY) && ControlManager::instance().key(CTRL_CLICK_ACTION).keyPressed(KBD_SHIFT|KBD_MENU);
		else
			shootPressed = !gameShell->isPaused(GameShell::PAUSE_BY_ANY) && isMouseFlagSet(MK_LBUTTON | MK_RBUTTON);

		if(currentClickAttackState_ != shootPressed){
			currentClickAttackState_ = !currentClickAttackState_;
			UnitCommand cmd(COMMAND_ID_DIRECT_SHOOT, UI_LogicDispatcher::instance().hoverPosition(), (int)currentClickAttackState_);
			cmd.setMiniMap(event.isFlag(UI_InputEvent::BY_MINIMAP));
			universe()->activePlayer()->sendCommand(cmd);
			attackCoordTime_ = 0;
		}
	}
}

void UI_LogicDispatcher::saveCursorPositionInfo()
{
	savedHoverInfo_ = currentHoverInfo_;
}

void UI_LogicDispatcher::restoreCursorPositionInfo()
{
	currentHoverInfo_ = savedHoverInfo_;
}

void UI_LogicDispatcher::setHoverPosition(const Vect3f& hp, bool inWorld)
{
	currentHoverInfo_.cursorInWorld_ = inWorld;
	currentHoverInfo_.hoverPosition_ = hp;

	currentHoverInfo_.hoverTerrainPosition_ = hp;
	currentHoverInfo_.hoverPosition_.z += 0.05f;

	currentHoverInfo_.hoverWater_ = (environment->water() && environment->water()->isFullWater(hp));
	if(currentHoverInfo_.hoverWater_)
		currentHoverInfo_.hoverPosition_.z = environment->water()->GetZ(hp.xi(), hp.yi()) - 0.5f;

	if(Player* pl = player()){
		const WeaponTarget& target = attackTarget();
		if(pl->playerUnit() && pl->playerUnit()->canAttackTarget(target, false))
			currentHoverInfo_.isAttackEnabled_ = true;
		else 			
			currentHoverInfo_.isAttackEnabled_ = selectManager->canAttack(attackTarget(), gameShell->underFullDirectControl(), true); 	
	} 	
	else 		
		currentHoverInfo_.isAttackEnabled_ = false;
}

void UI_LogicDispatcher::updateAimPosition()
{
	Vect2f mouse_pos = UI_Render::instance().relative2deviceCoords(mousePosition());

	Vect3f hoverPos;
	bool inWorld = false;
	inWorld = cameraManager->cursorTrace(mouse_pos, hoverPos);

	setHoverPosition(hoverPos, inWorld);

	Vect3f aimPos = currentHoverInfo_.hoverPosition_;
	if(!currentHoverInfo_.cursorInWorld_){
//		Vect2f mouse_pos = UI_Render::instance().relative2deviceCoords(mousePosition());
		Vect3f pos, dir;
		cameraManager->GetCamera()->GetWorldRay(mouse_pos,pos,dir);
		aimPos = pos + dir * UI_DIRECT_CONTROL_CURSOR_DIST;
	}

	float aimDist = 0.f;
	if(gameShell->underFullDirectControl()){
		if(UnitInterface* p = selectedUnit()){
			const UnitReal* rp = p->getUnitReal();
			Vect3f pos = rp->position();
			pos.z += .7f * rp->height();
/*
			if(const UnitInterface* target = hoverUnit()){
				if(target->rigidBody()){
					Vect3f pos1;
					int bodyPart(target->rigidBody()->computeBoxSectionPenetration(pos1, pos, aimPos));
					if(bodyPart >= 0)
						aimPos = pos1;
				}
			}
*/
			aimDist = pos.distance(aimPos);
		}
	}
	
	aimDistance_ = aimDist;
	aimPosition_ = aimPos;

/*
	aimPosition_ = currentHoverInfo_.hoverPosition_;
	if(gameShell->directControl_){
		if(UnitInterface* p = selectedUnit()){
			const UnitReal* rp = p->getUnitReal();
			Vect3f pos = rp->position();
			pos.z += .7f * rp->height();
			aimPosition_ = pos + 300.f * cameraManager->directControlShootingDirection();
			weapon_helpers::traceGround(pos, aimPosition_, aimPosition_);
		}
	}*/
}

void UI_LogicDispatcher::toggleTracking(bool state)
{
	gameShell->selectMouseTrack = state;
	trackMode_ = state;
	if(!state)
		startTrakingUnit_ = 0;
	enableInput(!state);
}

void UI_LogicDispatcher::setCursor(const UI_Cursor* cursor)
{
	// Если какой-нибудь курсор установлен и это устанавливаемый, то выход
	if (activeCursor() && cursor == activeCursor()) 
		return;
	activeCursor_ = cursor;
	if (!activeCursor_)
		setDefaultCursor();
	else
		updateCursor();
	
	createCursorEffect();
	updateCursor();

}

void UI_LogicDispatcher::showCursor() 
{
	if (!cursorVisible_)
	{
		cursorVisible_ = true;
		HCURSOR cursor = !activeCursor_ ? 0 : activeCursor_->cursor();
		::SetCursor(cursor);
	}
	
	if (cursorEffect_)
		cursorEffect_->ShowAllEmitter();
}

void UI_LogicDispatcher::hideCursor() 
{
	if (cursorVisible_)
	{
		cursorVisible_ = false;
		::SetCursor(NULL);
	}
	
	if (cursorEffect_)
		cursorEffect_->HideAllEmitter();

}

void UI_LogicDispatcher::updateCursor()
{
	if (cursorVisible_)
	{
		HCURSOR cursor = !activeCursor_ ? 0 : activeCursor_->cursor();
		::SetCursor(cursor);
	}
	else
		::SetCursor(NULL);

	if (cursorEffect_)
	{
		if (cursorVisible())
			cursorEffect_->ShowAllEmitter();
		else
			cursorEffect_->HideAllEmitter();
	}

}

void UI_LogicDispatcher::setDefaultCursor()
{
	static UI_Cursor default_cursor;
	if(!default_cursor.cursor())
		default_cursor.createCursor("Scripts\\Resource\\Cursors\\default.cur");
	xassert(default_cursor.cursor());
	activeCursor_ = &default_cursor;
	updateCursor();
}

void UI_LogicDispatcher::SetSelectPeram(const struct SelectionBorderColor& param)
{
	selectionPrm_=param;
	RELEASE(selectionTexture_);
	selectionTexture_ = GetTexLibrary()->GetElement2D(param.selectionTextureName_.c_str());
	RELEASE(selectionCornerTexture_);
	selectionCornerTexture_ = GetTexLibrary()->GetElement2D(param.selectionCornerTextureName_.c_str());
	RELEASE(selectionCenterTexture_);
	selectionCenterTexture_ = GetTexLibrary()->GetElement2D(param.selectionCenterTextureName_.c_str());
}

bool UI_LogicDispatcher::releaseResources()
{
	releaseSignSprites();
	UI_UnitView::instance().release();
	showHead().release();
	RELEASE(selectionTexture_);
	RELEASE(selectionCornerTexture_);
	RELEASE(selectionCenterTexture_);
	markObjects_.clear();
	return true;
}

void UI_LogicDispatcher::clearSquadSelectSigns()
{
	MTG();
	squadSelectSigns_.clear();
}

void UI_LogicDispatcher::addSquadSelectSign(const Recti& rect, UnitInterface* unit)
{
	MTG();
	squadSelectSigns_.push_back(SquadSelectSign(rect, unit));
	squadSelectSigns_.back().distance = cameraManager->eyePosition().distance2(unit->interpolatedPose().trans());
}

void UI_LogicDispatcher::addUnitOffscreenSprite(const UnitObjective* unit)
{
	MTG();
	const UI_Sprite* sprite = universe()->activePlayer()->isEnemy(unit->player()) ? unit->attr().offscreenSpriteForEnemy.get() : unit->attr().offscreenSprite.get();
	const UI_Sprite* msprite = universe()->activePlayer()->isEnemy(unit->player()) ? unit->attr().offscreenMultiSpriteForEnemy.get() : unit->attr().offscreenMultiSprite.get();
	if(!sprite && !msprite)
		return;

	const UI_Render& rd = UI_Render::instance();
	Rectf view(0, rd.windowPosition().top(), rd.renderSize().x, rd.windowPosition().height());

	Vect3f posd;
	posd.sub(unit->interpolatedPose().trans(), cameraManager->viewPosition()).normalize(20.f).add(cameraManager->viewPosition());
	cameraManager->GetCamera()->ConvertorWorldToViewPort(&posd, 0, &posd);

	Vect2f vcenter(view.center());
	Vect2f crd((Vect2f&)posd);
	crd -= vcenter;
	crd.normalize(view.width());
	crd += vcenter;
	view.clipLine(vcenter, crd);

	Vect2f half(sprite->size() / 2.f);
	crd.x = clamp(crd.x, view.left() + half.x, view.right() - half.x);
	crd.y = clamp(crd.y, view.top() + half.y, view.bottom() - half.y);
	half *= 2.f;
	Rectf out(rd.relativeSize(half));
	out.center(rd.relativeCoords(crd));

	if(msprite){
		SideSprites::iterator it;
		FOR_EACH(sideSprites_, it)
			if(it->msprite == msprite && it->sprite && it->pos.rect_overlap(out)){ // это первичный спрайт с таким же мультиспрайтом, пересекающийся с текущим
				it->sprite = 0; // теперь это мультиспрайт
				it->merge(out.center(), rd.relativeCoords(Recti(view.left_top(), view.size())));
				return;
			}
	}

	sideSprites_.push_back(SideSprite(sprite, msprite, out));
}

bool sideSpriteCompare(const SideSprite& lsh, const SideSprite& rsh)
{
	return lsh.msprite < rsh.msprite;
}

void UI_LogicDispatcher::drawUnitSideSprites()
{
	MTG();
	if(sideSprites_.empty())
		return;

	const UI_Render& rd = UI_Render::instance();
	Rectf border(rd.relativeCoords(Recti(0, rd.windowPosition().top(), rd.renderSize().x, rd.windowPosition().height())));
	
	sort(sideSprites_.begin(), sideSprites_.end(), &sideSpriteCompare);

	int size = -1;
	while(size != sideSprites_.size()){
		size = sideSprites_.size();

		SideSprites::iterator it1 = sideSprites_.begin();
		while(it1 != sideSprites_.end()){
			if(it1->msprite == 0){
				++it1;
				continue;
			}
			SideSprites::iterator it2 = it1 + 1;
			while(it2 != sideSprites_.end() && it2->msprite == it1->msprite){
				if(it1->pos.rect_overlap(it2->pos)){
					it1->sprite = 0;
					it1->merge(it2->pos.center(), border);
					it1 = sideSprites_.erase(it2) - 1;
					break;
				}
				++it2;
			}
			++it1;
		}
	}

	SideSprites::const_iterator it;
	FOR_EACH(sideSprites_, it)
		rd.drawSprite(it->pos, *(it->sprite ? it->sprite : it->msprite), 1.0f);

	sideSprites_.clear();
}


bool UI_LogicDispatcher::addLight(int light_index, const Vect3f& position) const
{
	MTL();

	if(Camera* camera = cameraManager->GetCamera()){
		Vect3f screen_position;
		camera->ConvertorWorldToViewPort(&position, 0, &screen_position);

		Vect2f pos = UI_Render::instance().deviceCoords(Vect2i(screen_position));
		return UI_BackgroundScene::instance().addLight(light_index, UI_Render::instance().device2relativeCoords(pos));
	}

	return false;
}

void fCommandAddMark(void* data)
{
	UI_MarkObjectInfo* inf = (UI_MarkObjectInfo*)(data);
	UI_LogicDispatcher::instance().addMark(*inf);
}

void UI_LogicDispatcher::addMark(const UI_MarkObjectInfo& inf)
{
	if(inf.isEmpty())
		return;

	if(MT_IS_GRAPH()){
		MarkObjects::iterator mi = std::find(markObjects_.begin(), markObjects_.end(), inf);
		if(mi == markObjects_.end()){
			markObjects_.push_back(UI_MarkObject());
			mi = markObjects_.end();
			--mi;
		}
		mi->update(inf);
		return;
	}

	uiStreamCommand.set(fCommandAddMark) << inf;
}

void UI_LogicDispatcher::addMovementMark()
{
	if(const Player* pl = player())
		addMark(UI_MarkObjectInfo(UI_MARK_MOVE_POINT, Se3f(QuatF::ID, hoverPosition()), &pl->race()->orderMark(hoverWater() ? UI_CLICK_MARK_MOVEMENT_WATER : UI_CLICK_MARK_MOVEMENT), selectManager->selectedUnit()));
}

void UI_LogicDispatcher::selectClickMode(UI_ClickModeID mode_id, const WeaponPrm* selected_weapon)
{
	MTG();
	
	if(mode_id != UI_CLICK_MODE_NONE && selected_weapon){
		const UnitActing* unit = 0;
		if(mode_id == UI_CLICK_MODE_PLAYER_ATTACK)
			unit = player()->playerUnit();
		else if(const UnitInterface* uif = selectManager->selectedUnit())
			if(uif->getUnitReal()->attr().isActing())
				unit = safe_cast<const UnitActing*>(uif->getUnitReal());
		// тогда выбираем только, если это оружие у юнита есть
		if(!unit || !unit->hasWeapon(selected_weapon->ID()))
			return;
	}

	selectedWeapon_ = selected_weapon;

	if(clickMode_ == UI_CLICK_MODE_PLAYER_ATTACK || mode_id == UI_CLICK_MODE_PLAYER_ATTACK)
		if(!selected_weapon || selected_weapon->weaponClass() == WeaponPrm::WEAPON_PAD)
			if(player()->playerUnit())
				player()->playerUnit()->sendCommand(UnitCommand(COMMAND_ID_WEAPON_ID, selectedWeaponID()));

	clickMode_ = mode_id;
}

void fCommandSendEventButton(void* data)
{
	XBuffer rb(data, 0);
	Event::Type event;
	rb.read(event);
	int playerID;
	rb.read(playerID);
	UI_ControlBase* control;
	rb.read(control);
	if(event != Event::UI_BUTTON_CLICK)
		gameShell->checkEvent(EventButton(event, playerID, control));
	else{
		ActionMode ui_event;
		ActionModeModifer modifiers;
		bool enabled;
		rb.read(ui_event);
		rb.read(modifiers);
		rb.read(enabled);
		EventButtonClick tmp(Event::UI_BUTTON_CLICK, playerID, control, ui_event, modifiers, enabled);
		gameShell->checkEvent(tmp);
	}
}

void UI_LogicDispatcher::inputEventProcessed(const UI_InputEvent& event, UI_ControlBase* control)
{
	MTG();
	
	if(!isGameActive())
		return;

	if(event.isMouseClickEvent()){
		uiStreamGraph2LogicCommand.set(fCommandSendEventButton) << Event::UI_BUTTON_CLICK << playerID() << control << control->actionFlags() << modifiers() << control->isEnabled();
	}

	if(event.isMouseClickEvent())
		selectClickMode(UI_CLICK_MODE_NONE);
}

void UI_LogicDispatcher::focusControlProcess(const UI_ControlBase* lastHovered)
{
	if(!isGameActive())
		return;

	if(hoverControl() != lastHovered){
		if(lastHovered)
			uiStreamGraph2LogicCommand.set(fCommandSendEventButton) << Event::UI_BUTTON_FOCUS_OFF << playerID() << lastHovered;
		uiStreamGraph2LogicCommand.set(fCommandSendEventButton) << Event::UI_BUTTON_FOCUS_ON << playerID() << hoverControl();
	}
}

void UI_LogicDispatcher::updateHoverInfo()
{
	if(hoverControl_){
		hoverControl_->startShowInfo();
		if(const UI_ActionData* action = hoverControl_->findAction(UI_ACTION_HOVER_INFO))
			hoverControlInfo_ = safe_cast<const UI_ActionDataHoverInfo*>(action);
		else
			hoverControlInfo_ = 0;
	}
	else
		hoverControlInfo_ = 0;
}

void UI_LogicDispatcher::setTakenItem(const UI_InventoryItem* item)
{
	if(item){
		takenItem_ = *item;
		takenItemSet_ = true;
	}
	else
		takenItemSet_ = false;
}

void UI_LogicDispatcher::toggleBuildingInstaller(const AttributeBase* attr)
{
	MTG();
	if(attr && UI_Dispatcher::instance().isEnabled()){
		buildingInstaller_.InitObject(safe_cast<const AttributeBuilding*>(attr), true);
		buildingInstaller_.SetBuildPosition(hoverPosition(), buildingInstaller_.angle());
	}
	else
		buildingInstaller_.Clear();
}

void UI_LogicDispatcher::disableDirectControl()
{
	if(gameShell->directControl())
		selectManager->makeCommand(UnitCommand(COMMAND_ID_DIRECT_CONTROL, false));
}

void UI_LogicDispatcher::createCursorEffect()
{
	RELEASE(cursorEffect_);
	
	if (terScene && activeCursor() && activeCursor()->effectRef()){
		cursorEffect_ = terScene->CreateEffectDetached(*activeCursor()->effectRef()->getEffect(1.f), NULL);
		if (cursorEffect_){
			cursorEffect_->setCycled(true);
			cursorEffect_->SetParticleRate(1.0f);
			cursorEffect_->Attach();
		}
	}
}

void UI_LogicDispatcher::moveCursorEffect()
{
	if(cursorEffect_)
		cursorEffect_->SetPosition(MatXf(Mat3f::ID, hoverPosition()));
}

int  UI_LogicDispatcher::selectedWeaponID() const
{
	return (selectedWeapon_) ? selectedWeapon_->ID() : 0;
}

GameType UI_LogicDispatcher::getGameType(const UI_ActionDataSaveGameList& data)
{
	GameType saveType = data.gameType();
	xassert(!data.autoSaveType() || isGameActive());
	if(data.autoSaveType() && isGameActive())
		saveType = gameShell->CurrentMission.gameType();
	return saveType;
}

const MissionDescriptions& UI_LogicDispatcher::getMissionsList(const UI_ActionDataSaveGameList& data)
{
	xassert(!data.shareList() || !data.autoSaveType());
	if(data.shareList())
		return missions_;
	else
		return profileSystem().saves(getGameType(data));
}

const MissionDescription* UI_LogicDispatcher::getMissionByName(const wchar_t* name, const UI_ActionDataSaveGameList& data)
{
	if(!name)
		return 0;
	return getMissionsList(data).find(name);
}

const MissionDescription* UI_LogicDispatcher::getMissionByName(const wchar_t* name) const
{
	if(!name)
		return 0;
	return missions_.find(name);
}

const MissionDescription* UI_LogicDispatcher::getMissionByID(const GUID& id) const
{
	return missions_.find(id);
}

const MissionDescription* UI_LogicDispatcher::getMissionByID(GameType type, const GUID& id)
{
	return profileSystem().saves(type).find(id);
}

void UI_LogicDispatcher::buildRaceList(UI_ControlBase* control, int racekey, bool withAny)
{
	const RaceTable::Strings& map = RaceTable::instance().strings();

	int selected = -1;
	ComboWStrings strings;
	if(withAny)
		strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_ANY_RACE));
	for(int idx = 0; idx < map.size(); ++idx)
		if(!map[idx].instrumentary()){
			if(idx == racekey)
				selected = strings.size();
			strings.push_back(map[idx].name());
		}

	UI_ControlComboList::setList(control, strings, selected);
}

void UI_LogicDispatcher::missionStart()
{
	if(MissionDescription* mission = currentMission())
		gameShell->startMissionSuspended(*mission);
}

void UI_LogicDispatcher::missionReStart()
{
	MissionDescription mission = gameShell->CurrentMission;
	if(mission.isLoaded()){
		mission.restart();
		gameShell->startMissionSuspended(mission);
	}
}

const wchar_t* UI_LogicDispatcher::currentPlayerDisplayName(WBuffer& out) const
{
	if(PNetCenter::isNCConfigured(PNCWM_ONLINE_DW))
		return (out < UI_LogicDispatcher::instance().currentProfile().lastInetName.c_str()).c_str();
	else
		return (out < UI_LogicDispatcher::instance().currentProfile().name.c_str()).c_str();
}

void UI_LogicDispatcher::setCurrentMission(const MissionDescription& descr, bool inherit)
{
	WBuffer buf;
	if(descr.gameType() & GAME_TYPE_REEL){
		selectedMission_ = descr;
	}
	else if(inherit && currentMission()){
		MissionDescription old = selectedMission_;
		selectedMission_ = descr;
		if(MissionDescription* current = currentMission()){
			current->setUseMapSettings(old.useMapSettings());
			if(!current->isMultiPlayer())
				current->setPlayerName(current->activePlayerID(), current->activeCooperativeIndex(), currentPlayerDisplayName(buf));
			else if(old.isMultiPlayer())
				current->setGameType(old.gameType());
		}
	}
	else {
		selectedMission_ = descr;
		if(currentMission() && !currentMission()->isMultiPlayer())
			currentMission()->setPlayerName(currentMission()->activePlayerID(), currentMission()->activeCooperativeIndex(), currentPlayerDisplayName(buf));
	}
}

void UI_LogicDispatcher::resetCurrentMission()
{
	currentTeemGametype_ = TEAM_GAME_TYPE_INDIVIDUAL;
	selectedMission_.clear(); //setCurrentMission(MissionDescription());
}

MissionDescription* UI_LogicDispatcher::currentMission() const
{
	return selectedMission_.isLoaded() ? const_cast<MissionDescription*>(&selectedMission_) : 0;
}

UI_NetCenter& UI_LogicDispatcher::getNetCenter() const
{
	return uiNetCenter();
}

void UI_LogicDispatcher::handleInGameChatString(const wchar_t* txt)
{
	if(uiNetCenter().gameCreated()){
		chatMessages_.push_back(txt);
		chatMessageTimes_.push_back(LogicTimer());
		chatMessageTimes_.back().start(UI_GlobalAttributes::instance().chatDelay() * 1000);
	}
}

void UI_LogicDispatcher::handleChatString(const ChatMessage& chatMsg)
{
	MTL();
	int rawIntData=chatMsg.id;
	xassert(rawIntData < 0 || uiNetCenter().gameCreated());

	if(rawIntData >= 0 && rawIntData != gameShell->CurrentMission.playerData(gameShell->CurrentMission.activePlayerID()).clan)
		return;

	WBuffer message, buf;
	if(rawIntData >= 0)
		message < UI_Dispatcher::instance().privateMessageColor();

	message < fromUTF8(buf, chatMsg.getText());

	handleInGameChatString(message.c_str());

	uiNetCenter().handleChatString(message.c_str());
}

void UI_LogicDispatcher::handleSystemMessage(const wchar_t* str)
{
	WBuffer msg;
	msg < UI_Dispatcher::instance().systemMessageColor() < str < L"&>";
	uiNetCenter().handleChatString(msg.c_str());

	handleInGameChatString(msg.c_str());
}

const MissionDescription* UI_LogicDispatcher::getControlStateByGameType(GameTuneOptionType type, ControlState& controlState, const UI_ActionDataPlayer* data) const
{
	bool useLoaded = type != TUNE_FLAG_VARIABLE && (!data || data->useLoadedMission());
	const MissionDescription* mission = useLoaded ? &gameShell->CurrentMission : currentMission();

	if(!mission || (data && data->playerIndex() >= mission->playersAmountMax())){
		controlState.hide();
		return 0;
	}

	RealPlayerType playerType = data ? mission->playerData(data->playerIndex()).realPlayerType : REAL_PLAYER_TYPE_PLAYER ;
	if(playerType & REAL_PLAYER_TYPE_WORLD){
		controlState.hide();
		return 0;
	}

	bool su = (GlobalAttributes::instance().serverCanChangeClientOptions ? uiNetCenter().isServer() : false);

	ControlState state;

	if(data && data->readOnly())
		state.disable();

	switch(type){
	case TUNE_PLAYER_TYPE:
		if(playerType & REAL_PLAYER_TYPE_PLAYER)
			state.hide();
		else {
			state.show();
			if(data->teamIndex() > 0)
				state.disable();
			else if(uiNetCenter().isNetGame()){
				if(uiNetCenter().isServer())
					state.enable();
				else
					state.disable();
			}
			else if(mission->useMapSettings())
				state.disable();
			else
				state.enable();
		}
		break;

	case TUNE_BUTTON_JOIN:
		if(uiNetCenter().isNetGame()
		&& playerType & (REAL_PLAYER_TYPE_PLAYER | REAL_PLAYER_TYPE_OPEN)
		&& mission->gameType() == GAME_TYPE_MULTIPLAYER_COOPERATIVE
		&& data->playerIndex() != mission->activePlayerID()
		&& mission->playerData(data->playerIndex()).hasFree())
			state.show();
		else
			state.hide();
		break;
	
	default:
		if(useLoaded){
			if(playerType & REAL_PLAYER_TYPE_EMPTY){
				controlState.hide();
				return 0;
			}
		}
		else {
			if(playerType & REAL_PLAYER_TYPE_CLOSE){
				controlState.hide();
				return 0;
			}
		}

		if(mission->activeCooperativeIndex() > 0)
			state.disable();

		su = su || (uiNetCenter().isServer() && playerType == REAL_PLAYER_TYPE_AI);

		switch(type){
		case TUNE_PLAYER_NAME:
			state.disable();
			if(playerType == REAL_PLAYER_TYPE_PLAYER
			&& mission->playerData(data->playerIndex()).usersIdxArr[data->teamIndex()] != USER_IDX_NONE)
				state.show();
			else
				state.hide();
			break;

		case TUNE_PLAYER_STAT_NAME:
			state.disable();
			if(playerType == REAL_PLAYER_TYPE_PLAYER || playerType == REAL_PLAYER_TYPE_AI)
				state.show();
			else
				state.hide();
			break;

		case TUNE_PLAYER_DIFFICULTY:
			if(playerType != REAL_PLAYER_TYPE_AI)
				state.hide();
			else {
				state.show();
				if(!uiNetCenter().isNetGame() || UI_NetCenter().isServer())
					state.enable();
				else
					state.disable();
			}
			break;

		case TUNE_PLAYER_CLAN:
		case TUNE_PLAYER_RACE:
			state.show();
			if(mission->useMapSettings())
				state.disable();
			else if(uiNetCenter().isNetGame()){
				if(su || data->playerIndex() == mission->activePlayerID())
					state.enable();
				else
					state.disable();
			}
			else
				state.enable();
			break;

		case TUNE_PLAYER_COLOR:
		case TUNE_PLAYER_EMBLEM:
			state.show();
			if(uiNetCenter().isNetGame())
				if(su || data->playerIndex() == mission->activePlayerID())
					state.enable();
				else
					state.disable();
			else
				state.enable();
				
			break;

		case TUNE_FLAG_READY:
			state.disable();
			if(uiNetCenter().isNetGame() 
			&& playerType == REAL_PLAYER_TYPE_PLAYER
			&& mission->playerData(data->playerIndex()).usersIdxArr[data->teamIndex()] != USER_IDX_NONE)
				state.show();
			else
				state.hide();
			break;

		case TUNE_BUTTON_KICK:
			state.enable();
			if(uiNetCenter().isServer() 
			&& playerType == REAL_PLAYER_TYPE_PLAYER
			&& data->playerIndex() != mission->activePlayerID()
			&& mission->playerData(data->playerIndex()).usersIdxArr[data->teamIndex()] != USER_IDX_NONE)
				state.show();
			else
				state.hide();
			break;

		case TUNE_FLAG_VARIABLE:
			state.show();
			if(uiNetCenter().isNetGame()){
				if(uiNetCenter().isServer())
					state.enable();
				else
					state.disable();
			}
			else
				state.enable();
			break;

		default:
			dassert(0);
			return 0;
		}
	}

#ifndef _FINAL_VERSION_
	if(showDebugInterface.enableAllNetControls){
		state.disable_control = false;
		state.enable_control = true;
	}
#endif
	
	if(state.disable_control)
		controlState.disable();
	else if(state.enable_control)
		controlState.enable();
	
	if(state.hide_control)
		controlState.hide();
	else if(state.show_control)
		controlState.show();

	if(data && data->onlyOperateControl())
		return 0;

	return mission;
}

void UI_LogicDispatcher::saveGame(const string& gameName, bool autoSave)
{
	if(saveGameName_.empty() && gameName.empty())
		return;

	if(!gameShell->CurrentMission.isLoaded())
		return;

	GameType gameType = gameShell->CurrentMission.gameType();
	
	if(gameType & GAME_TYPE_REEL)
		return;
	
	setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_WAITING));

	profileSystem().fixProfileDir();

	string path = profileSystem().getSavePath(gameType);
	createDirectory(path.c_str());

	string name = gameName.empty() ? saveGameName_ : gameName;
	path += name;

	if(autoSave ||checkDiskOp(UI_DISK_OP_SAVE_GAME, path.c_str()))
		makeDiskOp(UI_DISK_OP_SAVE_GAME, path.c_str(), gameShell->CurrentMission.gameType());
	else
		setDiskOp(UI_DISK_OP_SAVE_GAME, path.c_str());
}

void UI_LogicDispatcher::saveReplay()
{
	if(saveReplayName_.empty())
		return;

	profileSystem().fixProfileDir();

	string path = profileSystem().getSavePath(GAME_TYPE_REEL);
	createDirectory(path.c_str());

	path += saveReplayName_;

	if(checkDiskOp(UI_DISK_OP_SAVE_REPLAY, path.c_str()))
		makeDiskOp(UI_DISK_OP_SAVE_REPLAY, path.c_str(), gameShell->CurrentMission.gameType());
	else
		setDiskOp(UI_DISK_OP_SAVE_REPLAY, path.c_str());
}

void UI_LogicDispatcher::deleteSave()
{
	const MissionDescription* mission = currentMission();
	if(!mission)
		return;

	mission->deleteSave();

	UI_LogicDispatcher::instance().profileSystem().deleteSave(*mission);
	handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND,
		&UI_ActionDataControlCommand(
		mission->gameType() & GAME_TYPE_REEL ? UI_ACTION_REPLAY_NAME_INPUT : UI_ACTION_SAVE_GAME_NAME_INPUT,
		UI_ActionDataControlCommand::CLEAR)
		));
	handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_MISSION_LIST, UI_ActionDataControlCommand::RE_INIT)));
}

bool UI_LogicDispatcher::isGameActive() const
{
	return gameShell->GameActive;
}

void SpriteToBuf(cQuadBuffer<sVertexXYZWDT1>* buf, const Color4c& color, int x1, int y1, int x2, int y2,
					float u1=0.f, float v1=0.f,  
					float u2=0.f, float v2=1.f,
					float u3=1.f, float v3=0.f,
					float u4=1.f, float v4=1.f
				 )
{
	sVertexXYZWDT1* v = buf->Get();
	v[0].z=v[1].z=v[2].z=v[3].z=0.001f;
	v[0].w=v[1].w=v[2].w=v[3].w=0.001f;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=color;

	v[0].x = float(x1)-0.5f;	v[0].y = float(y1)-0.5f;
	v[1].x = float(x1)-0.5f;	v[1].y = float(y2)-0.5f;
	v[2].x = float(x2)-0.5f;	v[2].y = float(y1)-0.5f;
	v[3].x = float(x2)-0.5f;	v[3].y = float(y2)-0.5f;

	v[0].u1()=u1;	v[0].v1()=v1;
	v[1].u1()=u2;	v[1].v1()=v2;
	v[2].u1()=u3;	v[2].v1()=v3;
	v[3].u1()=u4;	v[3].v1()=v4;
}

bool UI_LogicDispatcher::drawSelection(const Vect2f& topLeft, const Vect2f& rightBottom) const
{
	Vect2f vv0 = UI_Render::instance().relative2deviceCoords(topLeft) + Vect2f(0.5f, 0.5f);
	Vect2f vv1 = UI_Render::instance().relative2deviceCoords(rightBottom) + Vect2f(0.5f, 0.5f);

	if(vv0.x == vv1.x || vv0.y == vv1.y)
		return false;

	if(!selectionTexture_ || !selectionCornerTexture_ || !selectionCenterTexture_)
		return false;
	gb_RenderDevice->SetSamplerDataVirtual(0,sampler_wrap_linear);

	Color4c color = selectionPrm_.selectionBorderColor_;
	Camera* cam = cameraManager->GetCamera();
	cInterfaceRenderDevice* rd = gb_RenderDevice;

	Vect2f bp = vv0;
	Vect2f cp = vv1;

	if(bp.x > cp.x) swap(bp.x, cp.x);
	if(bp.y > cp.y) swap(bp.y, cp.y);

	bp.x*=rd->GetSizeX();	bp.y*=rd->GetSizeY();
	cp.x*=rd->GetSizeX();	cp.y*=rd->GetSizeY();
	Vect2i wide(selectionTexture_->GetWidth(), selectionTexture_->GetHeight());
	bp.x-=wide.y/2;
	bp.y-=wide.y/2;
	cp.x-=wide.y/2;
	cp.y-=wide.y/2;

	cQuadBuffer<sVertexXYZWDT1>* buf= rd->GetQuadBufferXYZWDT1();
	rd->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,selectionTexture_);
	buf->BeginDraw();
	float u = 1;
	int real_wide = wide.x;
	int x = bp.x+wide.y;
	while(x<cp.x)
	{
		if (x+wide.x>cp.x)
		{
			real_wide = cp.x-x;
			u = float(real_wide)/wide.x;
		}
		SpriteToBuf(buf, color, x, bp.y, x+real_wide, bp.y+wide.y, 
			0,0,	0,1,	u,0,	u,1);
		SpriteToBuf(buf, color, x, cp.y, x+real_wide, cp.y+wide.y, 
			0,1,	0,0,	u,1,	u,0);
		x+=real_wide;
		
		if(!real_wide)
			break;
	}
	int y = bp.y+wide.y;
	float v1 = 1;
	real_wide = wide.x;
	while(y<cp.y)
	{
		if (cp.y<y+wide.x)
		{
			real_wide = cp.y-y;
			v1 = float(real_wide)/wide.x;
		}
		SpriteToBuf(buf, color, bp.x, y, bp.x+wide.y, y+real_wide, 
			0,-1,	v1,-1,	0,0,	v1,0);
		SpriteToBuf(buf, color, cp.x, y, cp.x+wide.y, y+real_wide, 
			0,1,	v1,1,	0,0,	v1,0);
		y+=wide.x;
	}
	buf->EndDraw();
		
	rd->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,selectionCornerTexture_);
	buf->BeginDraw();
	{
		float xw = (bp.x+wide.y>cp.x)? (cp.x-bp.x +wide.y)/2 : wide.y;
		float yw = (bp.y+wide.y>cp.y)? (cp.y-bp.y +wide.y)/2: wide.y;
		SpriteToBuf(buf, color, bp.x, bp.y, bp.x+xw, bp.y+yw, 
			0,0, 0,yw/wide.y, xw/wide.y,0, xw/wide.y,yw/wide.y);

		float x = cp.x;
		float y = bp.y;
		xw = yw = wide.y;
		if (bp.x+wide.y>cp.x)
		{
			x = ((cp.x+wide.y) + bp.x)/2;
			xw = (cp.x-bp.x+wide.y)/2;
		}
		if (bp.y+wide.y>cp.y)
			yw = (cp.y-bp.y+wide.y)/2;
		SpriteToBuf(buf, color, x, y, x+xw, y+yw, 
			0,xw/wide.y,	yw/wide.y,xw/wide.y,	0,0,	yw/wide.y,0);

		x = bp.x;
		y = cp.y;
		xw = yw = wide.y;	
		if (bp.y+wide.y>cp.y)
		{
			y = (cp.y+wide.y+ bp.y)/2;
			yw = (cp.y-bp.y+wide.y)/2;
		}
		if (bp.x+wide.y>cp.x)
			xw = (cp.x-bp.x+wide.y)/2;
		SpriteToBuf(buf, color, x, y, x+xw, y+yw, 
			yw/wide.y,0,	0,0,	yw/wide.y,xw/wide.y,	0,xw/wide.y);

		x = cp.x;
		y = cp.y;
		xw = yw = wide.y;
		if (bp.y+wide.y>cp.y)
		{
			y = (cp.y+wide.y+ bp.y)/2;
			yw = (cp.y-bp.y+wide.y)/2;
		}
		if (bp.x+wide.y>cp.x)
		{
			x = (cp.x+wide.y + bp.x)/2;
			xw = (cp.x-bp.x+wide.y)/2;
		}
		SpriteToBuf(buf, color, x, y, x+xw, y+yw, 
			xw/wide.y,yw/wide.y, xw/wide.y,0, 0,yw/wide.y, 0,0);
	}
	buf->EndDraw();

	if (selectionPrm_.needSelectionCenter_)
	{
		rd->SetNoMaterial(ALPHA_NONE,MatXf::ID,0,selectionCenterTexture_);
		buf->BeginDraw();
			float v = (cp.y-bp.y-wide.y)/selectionCenterTexture_->GetHeight();
			float u = (cp.x-bp.x-wide.y)/selectionCenterTexture_->GetWidth();
			SpriteToBuf(buf, color, bp.x+wide.y, bp.y+wide.y, cp.x, cp.y,	0,0, 0,v, u,0, u,v);
		buf->EndDraw();
	}
	return true;
}

bool UI_LogicDispatcher::clickAction(UI_ClickModeID clMode, int weaponID, UnitInterface* unit, bool shiftPressed)
{
	if(gameShell->underFullDirectControl())
		return false;

	xassert(unit);

	switch(clMode)
	{
	case UI_CLICK_MODE_ASSEMBLY:
		if(Player* pl = player())
			pl->sendCommand(UnitCommand(COMMAND_ID_ASSEMBLY_POINT, unit->position(), 1));
		return true;

	case UI_CLICK_MODE_MOVE:
		selectManager->makeCommandSubtle(COMMAND_ID_POINT, unit->position(), shiftPressed);
		return true;

	case UI_CLICK_MODE_ATTACK:
		if(!selectManager->isSelectionEmpty()){
			WeaponTarget target(unit, weaponID);
			if(selectManager->canAttack(target))
				selectManager->makeCommandAttack(target, shiftPressed);
		}
		return true;

	case UI_CLICK_MODE_PLAYER_ATTACK:
		if(Player* pl = player()){
			WeaponTarget target(unit, weaponID);
			if(pl->playerUnit() && pl->playerUnit()->canAttackTarget(target, false)){
				if(!shiftPressed)
					if(const WeaponPrm* prm = WeaponPrm::getWeaponPrmByID(weaponID))
						if(prm->alwaysPutInQueue())
							shiftPressed = true;
				
				UnitCommand cmd(COMMAND_ID_OBJECT, unit, weaponID);
				cmd.setShiftModifier(shiftPressed);
				pl->playerUnit()->sendCommand(cmd);
			}
		}
		return true;

	case UI_CLICK_MODE_REPAIR:
		if(Player* pl = player()){
			if(unit->player()->isWorld() || unit->player() == pl){
				UnitCommand cmd(COMMAND_ID_OBJECT, unit);
				cmd.setShiftModifier(shiftPressed);
				selectManager->makeCommand(cmd);
			}
		}
		return true;
	}

	return false;
}

bool UI_LogicDispatcher::clickAction(bool shiftPressed, bool byMinimap)
{
	if(gameShell->underFullDirectControl())
		return true;

	bool ret = false;

	switch(clickMode_)	{
	case UI_CLICK_MODE_ASSEMBLY:
		if(Player* pl = player()){
			pl->sendCommand(UnitCommand(COMMAND_ID_ASSEMBLY_POINT, hoverPosition(), 1));
			selectClickMode(UI_CLICK_MODE_NONE);
		}
		ret = true;
		break;

	case UI_CLICK_MODE_MOVE:
		selectManager->makeCommandSubtle(COMMAND_ID_POINT, hoverPosition(), shiftPressed, byMinimap);
		addMovementMark();
		
		selectClickMode(UI_CLICK_MODE_NONE);
		ret = true;
		break;
	case UI_CLICK_MODE_ATTACK: {
		bool clearClickMode = false;
		if(!selectManager->isSelectionEmpty()){
			WeaponTarget target = attackTarget();
			if(selectManager->canAttack(target)){
				selectManager->makeCommandAttack(target, shiftPressed, byMinimap);

				if(const Player* pl = player()){
					if(hoverUnit_)
						addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, Se3f(QuatF::ID, cursorPosition()), &pl->race()->orderMark(UI_CLICK_MARK_ATTACK_UNIT), selectManager->selectedUnit(), hoverUnit_));
					else
						addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, Se3f(QuatF::ID, cursorPosition()), &pl->race()->orderMark(UI_CLICK_MARK_ATTACK), selectManager->selectedUnit(), 0));
				}
			}
			else if(!gameShell->directControl() && hoverUnit_ && hoverUnit_->player() == universe()->activePlayer()){
				selectManager->selectUnit(hoverUnit_, false);
				clearClickMode = true;
			}
		}
	
		if(!shiftPressed && (!selectedWeapon_ || selectedWeapon_->clearAttackClickMode() || clearClickMode))
			selectClickMode(UI_CLICK_MODE_NONE);

		ret = true;
		break;
							   }

	case UI_CLICK_MODE_PLAYER_ATTACK:
		if(const Player* pl = player()){
			bool clearClickMode = false;
			WeaponTarget target = attackTarget();
			if(pl->playerUnit() && pl->playerUnit()->canAttackTarget(target, false)){
				if(!shiftPressed)
					if(const WeaponPrm* prm = WeaponPrm::getWeaponPrmByID(target.weaponID()))
						if(prm->alwaysPutInQueue())
							shiftPressed = true;

				UnitCommand cmd = target.unit()
					? UnitCommand(COMMAND_ID_OBJECT, target.unit(), target.weaponID())
					: UnitCommand(COMMAND_ID_ATTACK, target.position(), target.weaponID());

				cmd.setShiftModifier(shiftPressed);
				cmd.setMiniMap(byMinimap);

				player()->playerUnit()->sendCommand(cmd);

				if(hoverUnit_)
					addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, Se3f(QuatF::ID, cursorPosition()), &pl->race()->orderMark(UI_CLICK_MARK_ATTACK_UNIT), pl->playerUnit(), hoverUnit_));
				else
					addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, Se3f(QuatF::ID, cursorPosition()), &pl->race()->orderMark(UI_CLICK_MARK_ATTACK), pl->playerUnit(), 0));
			}
			else if(!gameShell->directControl() && hoverUnit_ && hoverUnit_->player() == universe()->activePlayer()){
				selectManager->selectUnit(hoverUnit_, false);
				clearClickMode = true;
			}

			if(!shiftPressed && (!selectedWeapon_ || selectedWeapon_->clearAttackClickMode() || clearClickMode))
				selectClickMode(UI_CLICK_MODE_NONE);
		}
		ret = true;
		break;

	case UI_CLICK_MODE_PATROL:
		if(selectManager->isSelectionEmpty())
			selectClickMode(UI_CLICK_MODE_NONE);
		else {
			selectManager->makeCommandSubtle(COMMAND_ID_PATROL, hoverPosition(), false, byMinimap);
			addMovementMark();

//			selectClickMode(UI_CLICK_MODE_NONE);
			ret = true;
		}
		break;
	case UI_CLICK_MODE_REPAIR:
		if(universe()){
			Player* player = universe()->activePlayer();
			if(player && hoverUnit_ && (hoverUnit_->player()->isWorld() || hoverUnit_->player() == player)){
				UnitCommand cmd(COMMAND_ID_OBJECT, hoverUnit_->getUnitReal());
				cmd.setShiftModifier(shiftPressed);
				cmd.setMiniMap(byMinimap);
				selectManager->makeCommand(cmd);

				addMark(UI_MarkObjectInfo(UI_MARK_REPAIR_TARGET, Se3f(QuatF::ID, hoverPosition()), &player->race()->orderMark(UI_CLICK_MARK_REPAIR), selectManager->selectedUnit()));
			}

			selectClickMode(UI_CLICK_MODE_NONE);
			ret = true;
		}
		break;
	case UI_CLICK_MODE_RESOURCE:
		if(universe()){
			Player* player = universe()->activePlayer();
			if(player && hoverUnit_ && (hoverUnit_->attr().isResourceItem() || hoverUnit_->attr().isInventoryItem())){
				UnitCommand cmd(COMMAND_ID_OBJECT, hoverUnit_);
				cmd.setShiftModifier(shiftPressed);
				cmd.setMiniMap(byMinimap);
				selectManager->makeCommand(cmd);

				addMovementMark();
			}

			selectClickMode(UI_CLICK_MODE_NONE);
			ret = true;
		}
		break;
	}

	return ret;
}

Player* UI_LogicDispatcher::player() const
{
	return universe() ? universe()->activePlayer() : 0;
}

int UI_LogicDispatcher::playerID() const
{
	return universe() ? universe()->activePlayer()->playerID() : -1;
}

void UI_LogicDispatcher::controlInit(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data){
	
	switch(id){	
		case UI_ACTION_LAN_GAME_LIST:
			uiNetCenter().reset();
			uiNetCenter().updateFilter();
			break;

		case UI_ACTION_CHAT_MESSAGE_BOARD:
			uiNetCenter().clearChatBoard();
			break;
		
		case UI_ACTION_GAME_CHAT_BOARD:
			clearGameChat();
			break;
		
		case UI_ACTION_CHAT_EDIT_STRING:
			control->setText(0);
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(L"");
			break;

		case UI_ACTION_INET_STATISTIC_SHOW:
			uiNetCenter().queryGlobalStatistic(true);
			break;

		case UI_ACTION_PROFILES_LIST:{
			ComboWStrings strings;
			for(Profiles::const_iterator it = profileSystem().profilesVector().begin(); it != profileSystem().profilesVector().end(); ++it)
				strings.push_back(it->name);
			UI_ControlComboList::setList(control, strings, profileSystem().currentProfileIndex());
			break;
									 }
		case UI_ACTION_ONLINE_LOGIN_LIST:
			if(UI_ControlStringList* lst = dynamic_cast<UI_ControlStringList*>(control)){
				ComboWStrings logins;
				ComboStrings::const_iterator it;
				FOR_EACH(currentProfile().onlineLogins, it)
					logins.push_back(a2w(*it));
				lst->setList(logins);
			}
			break;

		case UI_ACTION_SAVE_GAME_NAME_INPUT:
			control->setText(a2w(currentProfile().lastSaveGameName).c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(a2w(currentProfile().lastSaveGameName).c_str());
			saveGameName_ = currentProfile().lastSaveGameName;
			break;

		case UI_ACTION_REPLAY_NAME_INPUT:
			control->setText(a2w(currentProfile().lastSaveReplayName).c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(a2w(currentProfile().lastSaveReplayName).c_str());
			saveReplayName_ = currentProfile().lastSaveReplayName;
			break;
		
		case UI_ACTION_LAN_PLAYER_CLAN:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				ComboWStrings strings;
				for(int i = 1; i <= 6; ++i){
					WBuffer buf;
					buf <= i;
					strings.push_back(buf.c_str());
				}
				lst->setList(strings);
			}
			break;

		case UI_ACTION_GET_MODAL_MESSAGE:
			control->setText(UI_Dispatcher::instance().getModalMessage());
			break;

		case UI_ACTION_LAN_GAME_NAME_INPUT:
			if(currentProfile().lastCreateGameName.empty())
				currentProfile().lastCreateGameName = "GameName";
			control->setText(a2w(currentProfile().lastCreateGameName).c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(a2w(currentProfile().lastCreateGameName).c_str());
			break;

		case UI_ACTION_INET_NAME:
			control->setText(a2w(currentProfile().lastInetName).c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(a2w(currentProfile().lastInetName).c_str());
			break;

		case UI_ACTION_INET_PASS:
			uiNetCenter().resetPasswords();
			control->setText(0);
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(L"");
			break;

		case UI_ACTION_INET_PASS2:
			control->setText(0);
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(L"");
			break;

		case UI_ACTION_INET_DIRECT_ADDRESS:
			control->setText(currentProfile().lastInetDirectAddress.c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(currentProfile().lastInetDirectAddress.c_str());
			break;
		
		case UI_ACTION_CDKEY_INPUT:
			control->setText(a2w(currentProfile().cdKey).c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(a2w(currentProfile().cdKey).c_str());
			break;

		case UI_ACTION_LAN_GAME_TYPE:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				ComboWStrings strings;
				strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_INDIVIDUAL_LAN_GAME));
				strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_TEEM_LAN_GAME));
				lst->setList(strings);
			}
			break;

		case UI_ACTION_INET_FILTER_PLAYERS_COUNT:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				ComboWStrings strings;
				strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_ANY_UNITS_SIZE));
				WBuffer buf;
				for(int i = 2; i <= NETWORK_SLOTS_MAX; ++i){
					buf.init();
					buf <= i;
					strings.push_back(buf.c_str());
				}
				xassert(currentProfile().playersSlotFilter >= -1 && currentProfile().playersSlotFilter <= NETWORK_SLOTS_MAX);
				UI_ControlComboList::setList(control, strings, currentProfile().playersSlotFilter < 0 ? 0 : currentProfile().playersSlotFilter - 1);
			}
			break;

		case UI_ACTION_INET_FILTER_GAME_TYPE:{
			ComboWStrings strings;
			strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_ANY_GAME));
			strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_CUSTOM_GAME));
			strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_PREDEFINE_GAME));
			UI_ControlComboList::setList(control, strings, currentProfile().gameTypeFilter < 0 ? 0 : currentProfile().gameTypeFilter + 1);
			break;
											 }
		case UI_ACTION_QUICK_START_FILTER_POPULATION:
		case UI_ACTION_STATISTIC_FILTER_POPULATION:{
			ComboWStrings strings;
			strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_STAT_FREE_FOR_ALL));
			strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_STAT_1_VS_1));
			strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_STAT_2_VS_2));
			strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_STAT_3_VS_3));
			UI_ControlComboList::setList(control, strings, id == UI_ACTION_STATISTIC_FILTER_POPULATION
				? currentProfile().statisticFilterGamePopulation
				: currentProfile().quickStartFilterGamePopulation);
			break;
												   }
		case UI_ACTION_MISSION_SELECT_FILTER:
		case UI_ACTION_MISSION_QUICK_START_FILTER:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){

				const Profile::MissionFilter& filter = id == UI_ACTION_MISSION_SELECT_FILTER 
					? currentProfile().findMissionFilter
					: currentProfile().quickStartMissionFilter;

				ComboWStrings strings;
				lst->setSelectedString(-1);

				if(UI_ControlStringCheckedList* chekedList = dynamic_cast<UI_ControlStringCheckedList*>(lst)){

					for(MissionDescriptions::const_iterator it = missions_.begin(); it != missions_.end(); ++it)
						if(it->isBattle() && (id == UI_ACTION_MISSION_SELECT_FILTER || qsWorldsMgr.isMissionPresent(it->missionGUID())))
							strings.push_back(a2w(it->interfaceName()));

					lst->setList(strings);

					if(filter.filterDisabled)
						chekedList->setSelect(strings);
					else {
						ComboWStrings filterList;
						GUIDcontainer::const_iterator it;
						FOR_EACH(filter.filterList, it)
							if(const MissionDescription* mission = missions_.find(*it))
								filterList.push_back(a2w(mission->interfaceName()));
						chekedList->setSelect(filterList);
					}
				}
				else {
					strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_ALL_GAMES));

					GUID selectedMission = ZERO_GUID;

					if(filter.filterDisabled)
						lst->setSelectedString(0);
					else if(!filter.filterList.empty())
						selectedMission = filter.filterList.front();

					for(MissionDescriptions::const_iterator it = missions_.begin(); it != missions_.end(); ++it)
						if(it->isBattle() && (id == UI_ACTION_MISSION_SELECT_FILTER || qsWorldsMgr.isMissionPresent(it->missionGUID()))){
							if(it->missionGUID() == selectedMission)
								lst->setSelectedString(strings.size());
							strings.push_back(a2w(it->interfaceName()));
						}

						lst->setList(strings);
				}

				if(UI_ControlComboList* comboList = dynamic_cast<UI_ControlComboList*>(control))
					comboList->setText(lst->selectedString());
			}
			break;

		case UI_ACTION_QUICK_START_FILTER_RACE:
			buildRaceList(control, currentProfile().quickStartFilterRace, true);
			break;

		case UI_ACTION_STATISTIC_FILTER_RACE:
			buildRaceList(control, currentProfile().statisticFilterRace);
			break;
		
		case UI_ACTION_OPTION_PRESET_LIST:{
			int preset = GameOptions::instance().getCurrentPreset();
			ComboWStrings strings;
			WBuffer buf;
			splitComboListW(strings, GameOptions::instance().getPresetList(buf), L'|');
			xassert(preset < (int)strings.size());
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				lst->setList(strings);
				lst->setSelectedString(preset);
			}
			if(preset >= 0)
				control->setText(strings[preset].c_str());
			else
				control->setText(GET_LOC_STR(UI_COMMON_TEXT_CUSTOM_PRESET));
			break;
										  }
		case UI_ACTION_OPTION:{
			const UI_ActionOption* action = safe_cast<const UI_ActionOption*>(data);
			int data = GameOptions::instance().getOption(action->Option());
			if(UI_ControlSlider* slider = dynamic_cast<UI_ControlSlider*>(control))
				slider->setValue(0.001f * data);
			else {
				ComboWStrings strings;
				WBuffer buf;
				splitComboListW(strings, GameOptions::instance().getList(buf, action->Option()), L'|');
				if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
					lst->setList(strings);
					lst->setSelectedString(data);
					if(lst != control)
						control->setText(lst->selectedString());
				}
				else if(data >= 0 && !strings.empty()) {
					data = data % strings.size();
					if(control->states().size() > 1 && control->states().size() == strings.size()) // переключаем состояния
						control->setState(data);
					else // иначе надписи
						control->setText(strings[data].c_str());
				}
			}
							  }
			break;

		case UI_ACTION_PROFILE_DIFFICULTY:{
				const DifficultyTable::Strings& map = DifficultyTable::instance().strings();
				ComboWStrings strings;
				const Difficulty& cur = currentProfile().difficulty;
				int current = -1;
				for(int idx = 0; idx < map.size(); ++idx){
					if(Difficulty(map[idx].c_str()) == cur)
						current = strings.size();
					strings.push_back(map[idx].name());
				}
				UI_ControlComboList::setList(control, strings, current);
			break;
										  }

	}
}

void UI_LogicDispatcher::controlUpdate(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data, ControlState& controlState)
{
	switch(id){
		case UI_ACTION_CLICK_MODE: {
			const UI_ActionDataClickMode* action = safe_cast<const UI_ActionDataClickMode*>(data);
			switch(action->modeID()){
			case UI_CLICK_MODE_ATTACK:
			case UI_CLICK_MODE_PLAYER_ATTACK:
				if(action->weaponPrm()){
					const UnitActing* unit = 0;
					if(action->modeID() == UI_CLICK_MODE_PLAYER_ATTACK)
						unit = player()->playerUnit();
					else if(const UnitInterface* tmp = selectedUnit()){
						if((tmp = tmp->getUnitReal())->attr().isActing())
							unit = safe_cast<const UnitActing*>(tmp);
					}
					if(unit){
						if(unit->hasWeapon(action->weaponPrm()->ID())){
							controlState.show();
							if(unit->canFire(action->weaponPrm()->ID()))
								controlState.enable();
							else
								controlState.disable();
						}
						else
							controlState.hide();
					}
				}
				break;
			}
			break;
								   }
		case UI_ACTION_PLAYER_PARAMETER:
			if(const UI_ActionDataPlayerParameter* action = safe_cast<const UI_ActionDataPlayerParameter*>(data)){
				if(Player *pl = player()){
					float current = pl->resource().findByIndex(action->parameterIndex(), -1.f);
					if(current >= 0.f){
						float max = pl->resourceCapacity().findByIndex(action->parameterIndex(), action->parameterMax());
						if(UI_ControlProgressBar* bar = dynamic_cast<UI_ControlProgressBar*>(control)){
							if(max > FLT_EPS)
								bar->setProgress(clamp(current / max, 0.0f, 1.0f));
							else
								bar->setProgress(0.f);
						}
						else{
							WBuffer buf;
							buf <= round(current);
							control->setText(buf);
						}
					}
				}
			}
			break;

		case UI_ACTION_UNIT_PARAMETER:
			if(const UI_ActionDataUnitParameter* action = safe_cast<const UI_ActionDataUnitParameter*>(data)){
				const UnitInterface* unit = 0;
				int child_index = -1;
				switch(action->unitType()){
				case UI_ActionDataUnitParameter::SPECIFIC :
					if(const AttributeBase* attr = action->attribute()){
						if(universe() && universe()->activePlayer()){
							const RealUnits& units = universe()->activePlayer()->realUnits(attr);
							if(!units.empty())
								unit = units.front();
						}
					}
					break;
				case UI_ActionDataUnitParameter::PLAYER:
					unit = player()->playerUnit();
				    break;
				case UI_ActionDataUnitParameter::SELECTED:
					unit = selectedUnitIfOne();
					break;
				case UI_ActionDataUnitParameter::HOVERED:
					unit = hoverUnit();
					break;
				case UI_ActionDataUnitParameter::UNIT_LIST: {
					const UI_ControlBase* current = control;
					const UI_ControlContainer* parent = current->owner();
					while(parent){
						if(const UI_ControlUnitList* ul = dynamic_cast<const UI_ControlUnitList*>(parent)){
							dassert(std::find(ul->controlList().begin(), ul->controlList().end(), current) != ul->controlList().end());
							control->setPermanentTransform(current->permanentTransform()); // такая же трансформация как у активной ячейки
							// вычисляем номер контрола в списке
							child_index = std::distance(ul->controlList().begin(), std::find(ul->controlList().begin(), ul->controlList().end(), current));

							switch(ul->GetType()){
							case UI_UNITLIST_SELECTED:
								unit = selectManager->getUnitIfOne(child_index);
								break;
							case UI_UNITLIST_SQUADS_IN_WORLD:
								if(const Player* pl = player()){
									const SquadList& squadList = pl->squadList(ul->squadAtrribute());
									if(child_index < squadList.size())
										unit = squadList[child_index];
								}
								break;
							case UI_UNITLIST_SQUAD:
								if(UnitInterface* ui = selectManager->getUnitIfOne())
									if(const UnitSquad* squad = ui->getSquadPoint())
										if(child_index < squad->units().size())
											unit = squad->units()[child_index];
								break;
							case UI_UNITLIST_PRODUCTION_SQUAD:
								unit = selectedUnitIfOne();
								break;
							default:
								break;
							}
							break;
						}
						else
							current = safe_cast<const UI_ControlBase*>(parent);
						parent = current->owner();
						xassert(parent && "<<список юнитов>> во владельцах не найден");
					}
					break;
															}
				default:
					xassert(0 && "Необработанный параметр в UI_ActionDataUnitParameter");
					break;
				}
			
				if(unit){
					float current = -1.f, max = 1.f;
					UI_ControlProgressBar* bar = dynamic_cast<UI_ControlProgressBar*>(control);
					if(action->type() == UI_ActionDataUnitParameter::LOGIC){
						unit = unit->getUnitReal();
						current = unit->parameters().findByIndex(action->parameterIndex(), -1.f);
						if(!(current < 0.f))
							max = unit->parametersMax().findByIndex(action->parameterIndex(), action->parameterMax());
					
					}
					else if(action->type() == UI_ActionDataUnitParameter::LEVEL){
						if((unit = unit->getUnitReal())->attr().isLegionary())
							if(bar)
								current = safe_cast<const UnitLegionary*>(unit)->levelProgress();
							else
								current = safe_cast<const UnitLegionary*>(unit)->level() + 1;
					}
					else if(action->type() == UI_ActionDataUnitParameter::GROUND){
						if(bar){
							if(unit->attr().isPlayer())
								current = safe_cast<const UnitPlayer*>(unit)->currentCapacity();
						}
					}
					else if(action->type() == UI_ActionDataUnitParameter::PRODUCTION_PROGRESS){
						if(bar){
							if(child_index < 0){
								if((unit = unit->getUnitReal())->attr().isActing())
									current = safe_cast<const UnitActing*>(unit)->currentProductionProgress();
							}
							else if(const UnitSquad* squad = unit->getSquadPoint()){
								const UnitSquad::RequestedUnits& products = squad->requestedUnits();
								if(child_index < products.size())
									current = products[child_index].productionProgress();
							}
						}
					}

					if(!(current < 0.f)){
						if(action->updateAndCheck((int)unit, current))
							controlState.show();
						else
							controlState.hide();

						if(bar){
							if(max > FLT_EPS)
								bar->setProgress(clamp(current / max, 0.0f, 1.0f));
							else
								bar->setProgress(0.f);
						}
						else{
							WBuffer buf;
							buf <= round(current);
							control->setText(buf);
						}
					}
				}
				else
					if(UI_ControlProgressBar* bar = dynamic_cast<UI_ControlProgressBar*>(control))
						bar->setProgress(0);
					else
						control->setText(L"0");
			}
			break;

		case UI_ACTION_UNIT_HAVE_PARAMS: {
			const UI_ActionDataParams* action = safe_cast<const UI_ActionDataParams*>(data);
			bool flag = false;
			switch(action->type()){
				case UI_ActionDataParams::SELECTED_UNIT_THEN_PLAYER:
					if(const UnitInterface* unit = selectedUnit())
						flag = unit->getUnitReal()->requestResource(action->prms(), NEED_RESOURCE_SILENT_CHECK);
					else if(const Player* plr = player())
						flag = plr->requestResource(action->prms(), NEED_RESOURCE_SILENT_CHECK);
					break;
				case UI_ActionDataParams::PLAYER_UNIT:
					if(const Player* plr = player())
						if(plr->playerUnit())
							flag = plr->playerUnit()->requestResource(action->prms(), NEED_RESOURCE_SILENT_CHECK);
					break;
				case UI_ActionDataParams::PLAYER_ONLY:
						if(const Player* plr = player())
							flag = plr->requestResource(action->prms(), NEED_RESOURCE_SILENT_CHECK);
						break;
				case UI_ActionDataParams::SELECTRD_UNIT_ONLY:
					if(const UnitInterface* unit = selectedUnit())
						flag = unit->getUnitReal()->requestResource(action->prms(), NEED_RESOURCE_SILENT_CHECK);
					break;
			}
			switch(action->thatDoThanExist()){
			case UI_ActionDataParams::HIDE:
				if(flag)
					controlState.hide();
				else
					controlState.show();
				break;
			case UI_ActionDataParams::SHOW:
				if(flag)
					controlState.show();
				else
					controlState.hide();
				break;
			case UI_ActionDataParams::DISABLE:
				if(flag)
					controlState.disable();
				else
					controlState.enable();
				break;
			case UI_ActionDataParams::ENABLE:
				if(flag)
					controlState.enable();
				else
					controlState.disable();
				break;
			}
			break;
										 }
		case UI_ACTION_FIND_UNIT:{
			const UI_ActionDataFindUnit* action = safe_cast<const UI_ActionDataFindUnit*>(data);
			if(action->showCount())
				if(const Player* player = UI_LogicDispatcher::instance().player()){
					int size = 0, cnt = action->attributeCount();
					for(int idx = 0; idx < cnt; ++idx)
						size += player->realUnits(action->attribute(idx)).size();
					if(size > 0){
						controlState.show();
						if(size > 1)
							control->setText((WBuffer() <= size).c_str());
						else
							control->setText(0);
					}
					else
						controlState.hide();
				}
			break;
								 }
		case UI_ACTION_BIND_TO_IDLE_UNITS:
			if(int cnt = idleUnitsManager.idleUnitsCount(safe_cast<const UI_ActionDataIdleUnits*>(data)->type())){
				controlState.show();
				if(cnt > 1)
					control->setText((WBuffer() <= cnt).c_str());
				else
					control->setText(0);
			}
			else
				controlState.hide();
			break;
		
		case UI_ACTION_MINIMAP_ROTATION:
			controlState.setState(minimap().isRotateByCamera() ? 0 : 1);
			break;
		case UI_ACTION_MINIMAP_DRAW_WATER:
			controlState.setState(minimap().isDrawWater() ? 0 : 1);
			break;
		case UI_ACTION_INVENTORY_QUICK_ACCESS_MODE:
			controlState.setState(int(InventorySet::quickAccessMode()));
			break;

		case UI_ACTION_SHOW_TIME:{
			float seconds = global_time()/1000;
			int h = seconds / 3600;
			seconds -= h * 3600;
			int m = seconds / 60;
			int s = seconds - m * 60;
			wchar_t buf[128];
			swprintf(buf, L"%02u:%02u:%02u", h, m, s);
			control->setText(buf);
			break;
								 }
		case UI_ACTION_SHOW_COUNT_DOWN_TIMER:{
			int counter = 0;
			if(universe())
				counter = universe()->countDownTime();
			if(counter){
				controlState.show();
				float seconds = counter/1000;
				int m = seconds / 60;
				int s = seconds - m * 60;
				wchar_t buf[128];
				swprintf(buf, L"%02u:%02u", m, s);
				control->setText(buf);
			}
			else
				controlState.hide();
			break;
											 }
		case UI_ACTION_INET_NAT_TYPE:
			control->setText(uiNetCenter().natType());
			break;
		case UI_ACTION_LAN_GAME_LIST:{
			const UI_ActionDataHostList* dat = safe_cast<const UI_ActionDataHostList*>(data);
			uiNetCenter().updateGameList();
			ComboWStrings  strings;
			int selected = uiNetCenter().getGameList(dat->format(), strings, dat->startedGameColor());
			UI_ControlComboList::setList(control, strings, selected);
			break;
									 }
		case UI_ACTION_INET_STATISTIC_SHOW:{
			ComboWStrings  strings;
			int selected = uiNetCenter().getGlobalStatistic(safe_cast<const UI_ActionDataStatBoard*>(data)->format(), strings);
			UI_ControlComboList::setList(control, strings, selected);
			break;
										   }
		case UI_ACTION_INET_STATISTIC_QUERY:
			switch(safe_cast<const UI_ActionDataGlobalStats*>(data)->task()){
			case UI_ActionDataGlobalStats::GET_PREV:
			case UI_ActionDataGlobalStats::GOTO_BEGIN:
				if(uiNetCenter().canGetPrevGlobalStats())
					controlState.enable();
				else
					controlState.disable();
				break;
			case UI_ActionDataGlobalStats::GET_NEXT:
				if(uiNetCenter().canGetNextGlobalStats())
					controlState.enable();
				else
					controlState.disable();
				break;
			}
			break;
		case UI_ACTION_LAN_CHAT_CHANNEL_LIST:{
			uiNetCenter().updateChatChannels();
			ComboWStrings  strings;
			int selected = uiNetCenter().getChatChannels(strings);
			UI_ControlComboList::setList(control, strings, selected);
			if(uiNetCenter().autoSubscribeMode())
				controlState.disable();
			else
				controlState.enable();
			break;
										  }
		case UI_ACTION_LAN_CHAT_USER_LIST:{
			uiNetCenter().updateChatUsers();
			ComboWStrings  strings;
			int selected = uiNetCenter().getChatUsers(strings);
			UI_ControlComboList::setList(control, strings, selected);
			break;
										  }
		case UI_ACTION_CHAT_MESSAGE_BOARD: {
			ComboWStrings  strings;
			uiNetCenter().getChatBoard(strings);
			if(UI_ControlStringList* sp = dynamic_cast<UI_ControlStringList*>(control))
				sp->setList(strings);
			else {
				wstring txt;
				joinComboListW(txt, strings, L'\n');
				control->setText(txt.c_str());
			}
			break;
										   }
		case UI_ACTION_GAME_CHAT_BOARD:
			if(UI_ControlStringList* sp = dynamic_cast<UI_ControlStringList*>(control))
				sp->setList(chatMessages_);
			else {
				wstring txt;
				joinComboListW(txt, chatMessages_, L'\n');
				control->setText(txt.c_str());
			}
			break;

		case UI_ACTION_BIND_PLAYER:
			if(const MissionDescription* mission = currentMission())
			{
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				if( pdata->playerIndex() < mission->playersAmountMax()
				&& mission->playerData(pdata->playerIndex()).realPlayerType != REAL_PLAYER_TYPE_CLOSE
				&& (!pdata->teamIndex() || mission->playerData(pdata->playerIndex()).usersIdxArr[pdata->teamIndex()] != USER_IDX_NONE))
					controlState.show();
				else
					controlState.hide();
			}	
			else
				controlState.hide();

			break;

		case UI_ACTION_LAN_PLAYER_RACE:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_PLAYER_RACE, controlState, pdata))
				buildRaceList(control, mission->playerData(pdata->playerIndex()).race.key());
			break;
									   }
		case UI_ACTION_LAN_PLAYER_DIFFICULTY:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_PLAYER_DIFFICULTY, controlState, pdata)){
				const DifficultyTable::Strings& map = DifficultyTable::instance().strings();
				ComboWStrings strings;
				for(int idx = 0; idx < map.size(); ++idx)
					strings.push_back(map[idx].name());
				UI_ControlComboList::setList(control, strings, mission->playerData(pdata->playerIndex()).difficulty.key());
			}
			break;
											 }
		case UI_ACTION_LAN_PLAYER_TYPE:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_PLAYER_TYPE, controlState, pdata)){
				RealPlayerType type = mission->playerData(pdata->playerIndex()).realPlayerType;
				if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
					ComboWStrings strings;
					if(mission->isMultiPlayer())
						strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_SLOT_OPEN));
					if(type == REAL_PLAYER_TYPE_OPEN)
						lst->setSelectedString(strings.size() - 1);
					strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_SLOT_CLOSED));
					if(type == REAL_PLAYER_TYPE_CLOSE)
						lst->setSelectedString(strings.size() - 1);
					strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_SLOT_AI));
					if(type == REAL_PLAYER_TYPE_AI)
						lst->setSelectedString(strings.size() - 1);
					lst->setList(strings);
				}
				switch(type){
				case REAL_PLAYER_TYPE_OPEN:
					control->setText(GET_LOC_STR(UI_COMMON_TEXT_SLOT_OPEN));
					break;
				case REAL_PLAYER_TYPE_CLOSE:
					control->setText(GET_LOC_STR(UI_COMMON_TEXT_SLOT_CLOSED));
				    break;
				case REAL_PLAYER_TYPE_AI:
					control->setText(GET_LOC_STR(UI_COMMON_TEXT_SLOT_AI));
				    break;
				default:
				    control->setText(0);
					break;
				}
			}	
									   }
			break;

		case UI_ACTION_LAN_PLAYER_COLOR:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_PLAYER_COLOR, controlState, pdata)){
				int color_index = mission->playerData(pdata->playerIndex()).colorIndex;
				Color4f color = color_index <= GlobalAttributes::instance().playerAllowedColorSize() ? GlobalAttributes::instance().playerColors[color_index] : Color4f(1.f, 1.f, 1.f);
				control->setColor(color);
			}
										}
			break;

		case UI_ACTION_LAN_PLAYER_SIGN:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_PLAYER_EMBLEM, controlState, pdata)){
				int sign_index = mission->playerData(pdata->playerIndex()).signIndex;
				const UI_Sprite* sprite = &UI_Sprite::ZERO;
				if(sign_index >= 0 && sign_index <= GlobalAttributes::instance().playerAllowedSignSize())
					sprite = &UI_LogicDispatcher::instance().signSprite(sign_index);
				control->setSprite(*sprite, UI_SHOW_NORMAL);
				control->setSprite(*sprite, UI_SHOW_DISABLED);
				control->setSprite(*sprite, UI_SHOW_HIGHLITED);
				control->setSprite(*sprite, UI_SHOW_ACTIVATED);
			}
									   }
			break;

		case UI_ACTION_LAN_PLAYER_CLAN:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_PLAYER_CLAN, controlState, pdata)){
				int clan = mission->playerData(pdata->playerIndex()).clan;
				if(clan > 0 && clan <= 6){
					if(UI_ControlStringList* lst = UI_ControlComboList::getList(control))
						lst->setSelectedString(clan-1);
					WBuffer buf;
					buf <= clan;
					control->setText(buf);
				}
				else
					control->setText(0);
			}	
									   }
			break;

		case UI_ACTION_LAN_PLAYER_READY:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_FLAG_READY, controlState, pdata))
				controlState.setState(mission->getUserData(pdata->playerIndex(), pdata->teamIndex()).flag_playerStartReady);
										}
			break;			

		case UI_ACTION_LAN_PLAYER_NAME:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_PLAYER_NAME, controlState, pdata)){
				WBuffer buf;
				control->setText(mission->getPlayerName(buf, pdata->playerIndex(), pdata->teamIndex()));
			}
									   }
			break;

		case UI_ACTION_LAN_PLAYER_STATISTIC_NAME:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_PLAYER_STAT_NAME, controlState, pdata)){
				RealPlayerType type = mission->playerData(pdata->playerIndex()).realPlayerType;
				if(type == REAL_PLAYER_TYPE_AI)
					control->setText(GET_LOC_STR(UI_COMMON_TEXT_SLOT_AI));
				else {
					WBuffer buf;
					control->setText(mission->getPlayerName(buf, pdata->playerIndex(), 0));
				}
			}
												 }
			break;

		case UI_ACTION_LAN_PLAYER_JOIN_TEAM:
			getControlStateByGameType(TUNE_BUTTON_JOIN, controlState, safe_cast<const UI_ActionDataPlayer*>(data));
			break;
		
		case UI_ACTION_LAN_PLAYER_LEAVE_TEAM:
			getControlStateByGameType(TUNE_BUTTON_KICK, controlState, safe_cast<const UI_ActionDataPlayer*>(data));
			break;

		case UI_ACTION_LAN_USE_MAP_SETTINGS:
			if(const MissionDescription* mission = currentMission()){
				controlState.show();
				control->setText(mission->useMapSettings()
					? GET_LOC_STR(UI_COMMON_TEXT_PREDEFINE_GAME)
					: GET_LOC_STR(UI_COMMON_TEXT_CUSTOM_GAME));
			}
			else
				controlState.hide();
				
			break;

		case UI_ACTION_LAN_GAME_TYPE:
			if(const MissionDescription* mission = currentMission()){
				controlState.show();
				if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
					if(mission->gameType() == GAME_TYPE_MULTIPLAYER_COOPERATIVE)
						lst->setSelectedString(1);
					else
						lst->setSelectedString(0);

					if(UI_ControlComboList* comboList = dynamic_cast<UI_ControlComboList*>(control))
						comboList->setText(lst->selectedString());
				}
			}
			else
				controlState.hide();
				
			break;

		case UI_ACTION_PROFILE_CURRENT:
			control->setText(currentProfile().name.c_str());
			break;
		
		case UI_ACTION_MISSION_DESCRIPTION:
			if(const MissionDescription* mission = currentMission()){
				wstring str = mission->missionDescription();
				expandTextTemplate(str, ExpandInfo(ExpandInfo::MESSAGE));
				control->setText(str.c_str());
			}
			else
				control->setText(0);
			break;

		case UI_ACTION_BIND_GAME_PAUSE: {
			int currentState = (uiNetCenter().isOnPause() ? UI_ActionDataPause::NET_PAUSE : 0)
				| (gameShell->isPaused(GameShell::PAUSE_BY_USER) ? UI_ActionDataPause::USER_PAUSE : 0)
				| (gameShell->isPaused(GameShell::PAUSE_BY_MENU) ? UI_ActionDataPause::MENU_PAUSE : 0);

			if((safe_cast<const UI_ActionDataPause*>(data)->type() & currentState) != 0)
				controlState.show();
			else
				controlState.hide();
			break;
										}
		case UI_ACTION_BIND_GAME_TYPE:{
			const UI_ActionDataBindGameType* dat = safe_cast<const UI_ActionDataBindGameType*>(data);
			bool show = false;
			
			if(dat->checkGameType(UI_ActionDataBindGameType::UI_GT_NETGAME))
				if(gameShell->CurrentMission.isMultiPlayer())
					show = true;
			
			if(dat->checkGameType(UI_ActionDataBindGameType::UI_GT_BATTLE))
				if(gameShell->CurrentMission.battle() && !gameShell->CurrentMission.isMultiPlayer())
					show = true;
				
			if(dat->checkGameType(UI_ActionDataBindGameType::UI_GT_SCENARIO))
				if(gameShell->CurrentMission.scenario() && !gameShell->CurrentMission.isMultiPlayer())
					show = true;

			if(dat->replay() != UI_ANY)
				if((dat->replay() == UI_YES) != bool(gameShell->CurrentMission.gameType() & GAME_TYPE_REEL))
					show = false;
			
			if(show)
				controlState.show();
			else
				controlState.hide();

			break;
									  }
		case UI_ACTION_BIND_GAME_LOADED:
			if(gameShell->GameActive)
				controlState.show();
			else
				controlState.hide();
			break;

		case UI_ACTION_BIND_ERROR_TYPE:
			if(safe_cast<const UI_ActionDataBindErrorStatus*>(data)->errorStatus() == uiNetCenter().status())
				controlState.show();
			else
				controlState.hide();
			break;

		case UI_ACTION_LAN_CREATE_GAME:
			if(uiNetCenter().canCreateGame())
				controlState.enable();
			else
				controlState.disable();
			break;

		case UI_ACTION_LAN_JOIN_GAME:
			if(uiNetCenter().canJoinGame())
				controlState.enable();
			else
				controlState.disable();
			break;
		
		case UI_ACTION_INET_DIRECT_JOIN_GAME:
			if(uiNetCenter().canJoinDirectGame())
				controlState.enable();
			else
				controlState.disable();
			break;

		case UI_ACTION_BIND_NET_PAUSE:
			if(uiNetCenter().isOnPause())
				controlState.show();
			else
				controlState.hide();
			break;

		case UI_ACTION_NET_PAUSED_PLAYER_LIST: {
			ComboWStrings names;
			uiNetCenter().getPausePlayerList(names);
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control))
				lst->setList(names);
			else
				control->setText(names.empty() ? 0 : names[0].c_str());
			break;
											   }
		case UI_ACTION_PLAYER_UNITS_COUNT:
			if(universe() && universe()->activePlayer()){
				const UI_ActionDataPlayerDefParameter* action = safe_cast<const UI_ActionDataPlayerDefParameter*>(data);
				WBuffer buf;
				buf <= universe()->activePlayer()->unitNumber(action->par());
				control->setText(buf);
			}
			break;
		
		case UI_ACTION_PLAYER_STATISTICS:
			if(universe() && isGameActive()){
				const UI_ActionPlayerStatistic* action = safe_cast<const UI_ActionPlayerStatistic*>(data);
				if(action->statisticType() == UI_ActionPlayerStatistic::LOCAL){
					int index = gameShell->CurrentMission.findPlayerIndex(action->playerIndex());
					if(index >= 0){
						WBuffer buf;
						buf <= universe()->findPlayer(index)->playerStatistics()[action->type()];
						control->setText(buf);
						controlState.show();
					}
					else
						controlState.hide();
				}
				else {
					WBuffer buf;
					buf <= uiNetCenter().getCurrentGlobalStatisticValue(action->type());
					control->setText(buf);
				}
			}
			break;
		
		case UI_ACTION_UNIT_HINT:{
			const UI_ActionDataUnitHint* action = safe_cast<const UI_ActionDataUnitHint*>(data);
			if(action->unitType() != UI_ActionDataUnitHint::SELECTED && !tipsEnabled()){
				controlState.hide();
				break;
			}
		
			ExpandInfo info(ExpandInfo::UNIT);

			switch(action->unitType()){
			case UI_ActionDataUnitHint::HOVERED:
				if(info.unit = hoverUnit())
					if(showItemsHintMode() && !info.unit->attr().isActing())
						info.unit = 0;
				break;
			case UI_ActionDataUnitHint::SELECTED:
				info.unit = selectedUnit();
				break;
			case UI_ActionDataUnitHint::CONTROL:
				if(const UI_ControlBase* p = hoverControl_){
					const AttributeBase* selected = 0;
					if(const UnitInterface* un = selectedUnit())
						selected = un->selectionAttribute();
					info.attr = p->actionUnit(selected);
					info.control = hoverControl_;
				}
				break;
			case UI_ActionDataUnitHint::INVENTORY:
				if(const UnitInterface* un = selectedUnitIfOne())
					if(const InventorySet* inv = un->inventory()){
						if(inv->takenItem().attribute())
							break;
						if(const UI_ControlBase* p = hoverControl_)
							if(const UI_ControlInventory* uinv = dynamic_cast<const UI_ControlInventory*>(p))
								if(const InventoryItem* item = uinv->hoverItem(inv, mousePosition())){
									info.type = ExpandInfo::INVENTORY;
									info.control = hoverControl_;
									info.item = item;
									info.attr = item->attribute();
								}
					}
				break;
			case UI_ActionDataUnitHint::UNIT_LIST:
				if(!(info.control = hoverControl_))
					break;
				if(const UI_ControlUnitList* own = dynamic_cast<const UI_ControlUnitList*>(info.control->owner())){
					xassert(std::find(own->controlList().begin(), own->controlList().end(), info.control) != own->controlList().end());
					int child_index = std::distance(own->controlList().begin(), std::find(own->controlList().begin(), own->controlList().end(), info.control));
					switch(own->GetType()){
					case UI_UNITLIST_SELECTED:
						info.unit = selectManager->getUnitIfOne(child_index);
						break;
					case UI_UNITLIST_SQUAD:
						if(UnitInterface* ui = selectedUnitIfOne())
							if(UnitSquad* squad = ui->getSquadPoint())
								if(child_index < squad->units().size())
									info.unit = squad->units()[child_index];
						break;
					case UI_UNITLIST_SQUADS_IN_WORLD: {
						const SquadList& squadList = player()->squadList(own->squadAtrribute());
						if(child_index < squadList.size())
							info.unit = squadList[child_index];
						break;
													  }
					}
				}
			}

			if(info.unit && !info.attr)
				info.attr = &info.unit->getUnitReal()->attr();

			if(!info.attr){
				controlState.hide();
				break;
			}

			const wchar_t* hint = 0;
			switch(action->hintType()){
			case UI_ActionDataUnitHint::SHORT:
				hint = info.attr->interfaceName(action->lineNum());
				break;
			case UI_ActionDataUnitHint::FULL:
				hint = info.attr->interfaceDescription(action->lineNum());
				break;
			}

			if(!hint || !*hint){
				controlState.hide();
				break;
			}

			UI_HoverInfo& hi = hoverInfoManager.update(*action, control, 0, info.attr, info.unit);
			if(hi.ready()){
				wstring str = hint;
				expandTextTemplate(str, info);
				if(str.empty()){
					hi.stop();
					controlState.hide();
				}
				else {
					controlState.show();
					control->setText(str.c_str());
				}
			}
			else
				controlState.hide();

			break;
								 }
		case UI_ACTION_UI_HINT: {
			ExpandInfo info(ExpandInfo::CONTROL);
			info.control = hoverControl_;
			const UI_ActionDataHoverInfo* hvInfo = hoverControlInfo_;
			if(!info.control || !hvInfo || !tipsEnabled()){
				controlState.hide();
				break;
			}  
			const UI_ActionDataControlHint* action = safe_cast<const UI_ActionDataControlHint*>(data);
			if(action->type() == hvInfo->type() && !hvInfo->textEmpty()){
				UI_HoverInfo& hi = hoverInfoManager.update(*action, control, info.control, 0, 0);
				if(hi.ready()){
					if(const UnitInterface* unit = selectedUnit())
						info.attr = info.control->actionUnit(unit->selectionAttribute());
					wstring str = hvInfo->text();
					expandTextTemplate(str, info);
					if(str.empty()){
						hi.stop();
						controlState.hide();
					}
					else {
						controlState.show();
						control->setText(str.c_str());
					}
				}
				else
					controlState.hide();
			}
			else
				controlState.hide();
			
			break;
								}
		case UI_ACTION_BIND_NEED_COMMIT_SETTINGS:
			if(GameOptions::instance().needCommit())
				controlState.show();
			else
				controlState.hide();
			break;
		
		case UI_ACTION_NEED_COMMIT_TIME_AMOUNT:
			if(GameOptions::instance().needCommit()){
				controlState.show();
				WBuffer buf;
				buf <= round(GameOptions::instance().commitTimeAmount());
				control->setText(buf);
			}
			else{
				controlState.hide();
				control->setText(0);
			}
			break;

		case UI_ACTION_LOADING_PROGRESS:
			if(UI_ControlProgressBar* p = dynamic_cast<UI_ControlProgressBar*>(control))
				p->setProgress(GameLoadManager::instance().progress());
			break;
		
		case UI_ACTION_BUILDING_CAN_INSTALL:
			controlState.apply(universe()->activePlayer()->canBuildStructure(safe_cast<const UI_ActionDataUnitOrBuildingUpdate*>(data)->attribute()));
			break;
		
		case UI_ACTION_GET_MODAL_MESSAGE:
			if(UI_Dispatcher::instance().isModalMessageMode())
				control->setText(UI_Dispatcher::instance().getModalMessage());
			break;

		case UI_ACTION_MISSION_LIST: {
			const UI_ActionDataSaveGameList* action = safe_cast<const UI_ActionDataSaveGameList*>(data);
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				GUID lastMission;
				if(const MissionDescription* mission = currentMission())
					lastMission = mission->missionGUID();
				lst->setSelectedString(-1);
				ComboWStrings strings;
				GameType saveType = getGameType(*action);
				const MissionDescriptions& missions = getMissionsList(*action);
				for(MissionDescriptions::const_iterator it = missions.begin(); it != missions.end(); ++it){
					if(saveType == GAME_TYPE_BATTLE && !it->isBattle() && !currentProfile().passedMissions.exists(it->missionGUID()))
						continue;
					if(it->missionGUID() == lastMission)
						lst->setSelectedString(strings.size());
					strings.push_back(a2w(it->interfaceName()));
				}
				lst->setList(strings);

				if(UI_ControlComboList* comboList = dynamic_cast<UI_ControlComboList*>(control))
					comboList->setText(lst->selectedString());
			}
			break;
									 }
		case UI_ACTION_BIND_SAVE_LIST:
			if(getMissionsList(*safe_cast<const UI_ActionDataSaveGameList*>(data)).empty())
				controlState.disable();
			else
				controlState.enable();
			break;

		case UI_ACTION_GLOBAL_VARIABLE:{
			int val = 0;
			switch (safe_cast<const UI_ActionTriggerVariable*>(data)->type()){
			case UI_ActionTriggerVariable::GLOBAL:
				val = Singleton<IntVariables>::instance()[safe_cast<const UI_ActionTriggerVariable*>(data)->name()];
				break;
			case UI_ActionTriggerVariable::MISSION_DESCRIPTION:
				if(const MissionDescription* mission = getControlStateByGameType(TUNE_FLAG_VARIABLE, controlState))
					val = (mission->triggerFlags() & (1 << safe_cast<const UI_ActionTriggerVariable*>(data)->number()) ? 1 : 0);
				break;
			}
			if(UI_ControlButton *but = dynamic_cast<UI_ControlButton*>(control))
				controlState.setState(val);
			else if(UI_ControlStringList *lst = dynamic_cast<UI_ControlStringList*>(control))
				lst->setSelectedString(clamp(val, -1, lst->listSize()-1));
									   }
			break;
	}
}

void UI_LogicDispatcher::controlAction(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data)
{
	switch(id){
		case UI_ACTION_CLICK_MODE:
			selectClickMode(safe_cast<const UI_ActionDataClickMode*>(data)->modeID(), safe_cast<const UI_ActionDataClickMode*>(data)->weaponPrm());
			break;

		case UI_ACTION_CONTROL_COMMAND:
			if(const UI_ActionDataControlCommand* action = safe_cast<const UI_ActionDataControlCommand*>(data)){
				switch(action->command()){
					case UI_ActionDataControlCommand::GET_CURRENT_SAVE_NAME:
						if(currentMission())
							control->setText(a2w(currentMission()->interfaceName()).c_str());
						else
							control->setText(0);

						if(UI_ControlEdit* ctrl = dynamic_cast<UI_ControlEdit*>(control))
							ctrl->setEditText(control->text());

						break;
					
					case UI_ActionDataControlCommand::GET_CURRENT_PROFILE_NAME:
						control->setText(profileName_.c_str());
						if(UI_ControlEdit* ctrl = dynamic_cast<UI_ControlEdit*>(control))
							ctrl->setEditText(control->text());
						break;

					case UI_ActionDataControlCommand::GET_CURRENT_CDKEY:
						control->setText(a2w(currentProfile().cdKey.c_str()).c_str());
						if(UI_ControlEdit* ctrl = dynamic_cast<UI_ControlEdit*>(control))
							ctrl->setEditText(control->text());
						break;

					case UI_ActionDataControlCommand::UPDATE_HOTKEY:
						if(UI_ControlHotKeyInput* cp = dynamic_cast<UI_ControlHotKeyInput*>(control)){
							if(const UI_ControlHotKeyInput* last_input = lastHotKeyInput_){
								if(last_input == cp) break;

								if(cp->key() == last_input->key() && !ControlManager::instance().isCompatible(cp->controlID(), last_input->controlID())){
									cp->setKey(last_input->saveKey());
									hotKeyUpdated_ = true;
								}
							}
						}
						break;
					case UI_ActionDataControlCommand::UPDATE_COMPATIBLE_HOTKEYS:
						if(!hotKeyUpdated_)
							break;

						if(UI_ControlHotKeyInput* cp = dynamic_cast<UI_ControlHotKeyInput*>(control)){
							if(const UI_ControlHotKeyInput* last_input = lastHotKeyInput_){
								if(last_input == cp) break;

								if(ControlManager::instance().isCompatible(cp->controlID(), last_input->controlID()) && cp->key() == last_input->saveKey())
									cp->setKey(last_input->key());
							}
						}
						break;
				}
			}
			break;
		
		case UI_ACTION_CANCEL:
			cancelActions();
			selectManager->makeCommand(UnitCommand(COMMAND_ID_STOP, 0));
			break;

		case UI_ACTION_CLICK_FOR_TRIGGER:
			gameShell->checkEvent(*EventHandle(new EventButtonClick(Event::UI_BUTTON_CLICK_LOGIC, playerID(), control, control->actionFlags(), modifiers(), control->isEnabled())));
			break;

		case UI_ACTION_CONTEX_APPLY_CLICK_MODE:{
			Player* pl = 0;
			if(!selectManager || cachedClickMode_ == UI_CLICK_MODE_NONE || !(pl = player()))
				break;

			const AttributeBase* attribute = 0;
			UnitInterface* commandUnit = 0;

			if(const UI_ControlUnitList* own = dynamic_cast<const UI_ControlUnitList*>(control->owner())){
				xassert(std::find(own->controlList().begin(), own->controlList().end(), control) != own->controlList().end());
				int child_index = std::distance(own->controlList().begin(), std::find(own->controlList().begin(), own->controlList().end(), control));
				switch(own->GetType()){
				case UI_UNITLIST_SELECTED:
					commandUnit = selectManager->getUnitIfOne(child_index);
					break;
				case UI_UNITLIST_SQUAD:
					if(UnitInterface* ui = selectManager->getUnitIfOne())
						if(UnitSquad* squad = ui->getSquadPoint())
							if(child_index < squad->units().size())
								commandUnit = squad->units()[child_index];
					break;
				case UI_UNITLIST_SQUADS_IN_WORLD: {
					CUNITS_LOCK(pl);
					const SquadList& squadList = pl->squadList(own->squadAtrribute());
					if(child_index < squadList.size())
						commandUnit = squadList[child_index];
					break;
												  }
				}
			}
			else
				attribute = control->actionUnit(selectManager->selectedAttribute());

			if(!commandUnit && attribute)
				if(Player* pl = player()){
					CUNITS_LOCK(pl);
					const RealUnits& units = pl->realUnits(attribute);
					if(units.size() == 1)
						commandUnit = units.front();
				}
			if(commandUnit)
				clickAction(cachedClickMode_, cachedWeaponID_, commandUnit->getUnitReal(), isMouseFlagSet(MK_SHIFT));
			break;
											   }
		case UI_ACTION_PAUSE_GAME:
			if(safe_cast<const UI_ActionDataPauseGame*>(data)->onlyLocal() && uiNetCenter().isNetGame())
				break;
			
			if(safe_cast<const UI_ActionDataPauseGame*>(data)->enable())
				gameShell->pauseGame(GameShell::PAUSE_BY_MENU);
			else
				gameShell->resumeGame(GameShell::PAUSE_BY_MENU);
			break;

		case UI_ACTION_POST_EFFECT:
			if(environment)
				environment->PEManager()->setActive(
					safe_cast<const UI_ActionDataPostEffect*>(data)->postEffect(),
					safe_cast<const UI_ActionDataPostEffect*>(data)->enable());
			break;

		case UI_ACTION_OPTION_CANCEL:
			GameOptions::instance().revertChanges();
			handleMessageReInitGameOptions();
			break;
		
		case UI_ACTION_OPTION_APPLY:
			GameOptions::instance().userApply();
			handleMessageReInitGameOptions();
			break;

		case UI_ACTION_OPTION_PRESET_LIST:
			if(const UI_ControlStringList* lst = UI_ControlComboList::getList(control))
				if(lst->isActivated()){
					WBuffer buf;
					int preset = indexInComboListStringW(GameOptions::instance().getPresetList(buf), control->text());
					if(preset >= 0){
						GameOptions::instance().loadPresets(lst->selectedStringIndex());
						handleMessageReInitGameOptions();
					}
				}
			break;

		case UI_ACTION_OPTION:{
			const UI_ActionOption* action = safe_cast<const UI_ActionOption*>(data);
			int data = GameOptions::instance().getOption(action->Option());
			if(UI_ControlSlider* slider = dynamic_cast<UI_ControlSlider*>(control))
				GameOptions::instance().setOption(action->Option(), slider->value() * 1000);
			else {
				ComboWStrings strings;
				WBuffer buf;
				splitComboListW(strings, GameOptions::instance().getList(buf, action->Option()), L'|');
				if(const UI_ControlStringList* lst = UI_ControlComboList::getList(control))
					GameOptions::instance().setOption(action->Option(), lst->selectedStringIndex());
				else if(data >= 0 && !strings.empty()){
					int add = isMouseFlagSet(MK_RBUTTON) ? strings.size() - 1 : 1;
					GameOptions::instance().setOption(action->Option(), (data + add) % strings.size());
				}
			}
			if(GameOptions::instance().needInstantApply(action->Option()))
				GameOptions::instance().setPartialOptionsApply();
			handleMessageReInitGameOptions();
			break;
							  }
		case UI_ACTION_MINIMAP_ROTATION:
			minimap().toggleRotateByCamera(!minimap().isRotateByCamera());
			break;
		case UI_ACTION_MINIMAP_DRAW_WATER:
			minimap().water(!minimap().isDrawWater());
			break;
		case UI_ACTION_INVENTORY_QUICK_ACCESS_MODE:
			if(InventorySet::quickAccessMode() == UI_INVENTORY_QUICK_ACCESS_OFF)
				InventorySet::setQuickAccessMode(UI_INVENTORY_QUICK_ACCESS_ON);
			else
				InventorySet::setQuickAccessMode(UI_INVENTORY_QUICK_ACCESS_OFF);
			break;

		case UI_ACTION_BIND_TO_IDLE_UNITS:
			if(UnitInterface* unit = idleUnitsManager.getUdleUnit(safe_cast<const UI_ActionDataIdleUnits*>(data)->type()))
				selectManager->selectUnit(unit, false);
			break;

		case UI_ACTION_LAN_DISCONNECT_SERVER:
			uiNetCenter().reset();
			break;

		case UI_ACTION_LAN_ABORT_OPERATION:
			uiNetCenter().abortCurrentOperation();
			break;

		case UI_ACTION_LAN_CHAT_CHANNEL_LIST:
			if(UI_ControlStringList* sp = dynamic_cast<UI_ControlStringList*>(control)){
				uiNetCenter().selectChatChannel(sp->selectedStringIndex());
			}
			break;
		case UI_ACTION_LAN_CHAT_CHANNEL_ENTER:
			uiNetCenter().enterChatChannel();
			break;

		case UI_ACTION_LAN_CHAT_CLEAR:
			uiNetCenter().clearChatBoard();
			break;

		case UI_ACTION_LAN_GAME_LIST:
			if(UI_ControlStringList* sp = dynamic_cast<UI_ControlStringList*>(control)){
				uiNetCenter().selectGame(sp->selectedStringIndex());
				setCurrentMission(uiNetCenter().selectedGame());
			}
			break;
		
		case UI_ACTION_INET_STATISTIC_SHOW:
			if(UI_ControlStringList* sp = UI_ControlComboList::getList(control))
				uiNetCenter().selectGlobalStaticticEntry(sp->selectedStringIndex());
			break;

		//case UI_ACTION_LAN_CHAT_USER_LIST:
		//	if(UI_ControlStringList* sp = dynamic_cast<UI_ControlStringList*>(control)){
		//	}
		//	break;

		case UI_ACTION_REPLAY_SAVE:
			saveReplay();
			break;

		case UI_ACTION_PROFILES_LIST:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				if(const wchar_t* name = lst->selectedString()){
					profileName_ = name;
					handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_PROFILE_INPUT, UI_ActionDataControlCommand::GET_CURRENT_PROFILE_NAME)));
					handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_CDKEY_INPUT, UI_ActionDataControlCommand::GET_CURRENT_CDKEY)));
					handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_CDKEY_INPUT, UI_ActionDataControlCommand::EXECUTE)));
				}
			}
			break;
		
		case UI_ACTION_ONLINE_LOGIN_LIST:
			if(UI_ControlStringList* lst = dynamic_cast<UI_ControlStringList*>(control)){
				if(const wchar_t* name = lst->selectedString()){
					w2a(currentProfile().lastInetName, name);
					handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_INET_NAME, UI_ActionDataControlCommand::RE_INIT)));
				}
			}
			break;

		case UI_ACTION_MISSION_LIST:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				const UI_ActionDataSaveGameListFixed* action = safe_cast<const UI_ActionDataSaveGameListFixed*>(data);
				if(const MissionDescription* mission = getMissionByName(lst->selectedString(), *action)){
					MissionDescription md(*mission);
					if(PNetCenter::isNCCreated()){
						xassert(!(action->gameType() & GAME_TYPE_REEL));
						md.setGameType(GAME_TYPE_MULTIPLAYER);
					}
					else
						md.setGameType(getGameType(*action));
					
					if(action->inherit())
						setCurrentMission(md, true);
					else {
						md.setUseMapSettings(action->isPredefine());
						setCurrentMission(md, false);
					}
					
					if(action->resetSaveGameName())
						if(action->gameType() & GAME_TYPE_REEL){
							handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_REPLAY_NAME_INPUT, UI_ActionDataControlCommand::GET_CURRENT_SAVE_NAME)));
							handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_REPLAY_NAME_INPUT, UI_ActionDataControlCommand::EXECUTE)));
						}
						else {
							handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_SAVE_GAME_NAME_INPUT, UI_ActionDataControlCommand::GET_CURRENT_SAVE_NAME)));
							handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_SAVE_GAME_NAME_INPUT, UI_ActionDataControlCommand::EXECUTE)));
						}
				}
			}
			break;

		case UI_ACTION_LAN_PLAYER_CLAN:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				if(const wchar_t* text = control->text()){
					int clan = _wtoi(text);
					mission->changePlayerData(pdata->playerIndex()).clan = clan;
					if(PNetCenter::isNCCreated())
						PNetCenter::instance()->changePlayerClan(pdata->playerIndex(), clan);
				}
			}
			break;

		case UI_ACTION_LAN_USE_MAP_SETTINGS:
			if(MissionDescription* mission = currentMission()){
				if(mission->useMapSettings()){
					mission->setUseMapSettings(false);
					mission->shufflePlayers();
				}
				else {
					const MissionDescription* clean = getMissionByID(mission->missionGUID());
					xassert(clean);
					MissionDescription md(*clean);
					if(uiNetCenter().isNetGame())
						md.setGameType(GAME_TYPE_MULTIPLAYER);
					else
						md.setGameType(mission->gameType());
						md.setUseMapSettings(true);
					setCurrentMission(md);
				}
			}
			break;
		
		case UI_ACTION_LAN_GAME_TYPE:
			if(MissionDescription* mission = currentMission()){
				UI_CommonLocText curType = getLocStringId(control->text());
				if(curType == UI_COMMON_TEXT_INDIVIDUAL_LAN_GAME)
					currentTeemGametype_ = TEAM_GAME_TYPE_INDIVIDUAL;
				else if(curType == UI_COMMON_TEXT_TEEM_LAN_GAME)
					currentTeemGametype_ = TEAM_GAME_TYPE_TEEM;
				if(mission->isMultiPlayer())
					mission->setGameType(currentTeemGametype_ == TEAM_GAME_TYPE_TEEM ? GAME_TYPE_MULTIPLAYER_COOPERATIVE : GAME_TYPE_MULTIPLAYER);
			}
			break;

		case UI_ACTION_LAN_PLAYER_TYPE:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				RealPlayerType type = REAL_PLAYER_TYPE_CLOSE;
				switch(getLocStringId(control->text())){
				case UI_COMMON_TEXT_SLOT_OPEN:
					type = REAL_PLAYER_TYPE_OPEN;
					break;
				case UI_COMMON_TEXT_SLOT_AI:
					type = REAL_PLAYER_TYPE_AI;
				    break;
				}
				if(type == REAL_PLAYER_TYPE_CLOSE && pdata->playerIndex() == mission->activePlayerID())
					break; // свой слот закрывать нельзя
				if(mission->playerData(pdata->playerIndex()).realPlayerType != type){
					mission->changePlayerData(pdata->playerIndex()).realPlayerType = type;
					if(PNetCenter::isNCCreated())
						PNetCenter::instance()->changeRealPlayerType(pdata->playerIndex(), type);
				}
			}
			break;

		case UI_ACTION_LAN_PLAYER_RACE:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				const RaceProperty* val = 0;
				const wchar_t* name = control->text();
				const RaceTable::Strings& map = RaceTable::instance().strings();
				RaceTable::Strings::const_iterator it;
				FOR_EACH(map, it)
					if(!wcscmp(it->name(), name))
						val = &*it;
				if(val){
					mission->changePlayerData(pdata->playerIndex()).race = Race(val->c_str());
					if(PNetCenter::isNCCreated())
						PNetCenter::instance()->changePlayerRace(pdata->playerIndex(), Race(val->c_str()));
				}
			}
			break;

		case UI_ACTION_LAN_PLAYER_DIFFICULTY:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				const DifficultyPrm* val = 0;
				const wchar_t* name = control->text();
				const DifficultyTable::Strings& map = DifficultyTable::instance().strings();
				DifficultyTable::Strings::const_iterator it;
				FOR_EACH(map, it)
					if(!wcscmp(it->name(), name))
						val = &*it;
				if(val){
					mission->changePlayerData(pdata->playerIndex()).difficulty = Difficulty(val->c_str());
					if(PNetCenter::isNCCreated())
						PNetCenter::instance()->changePlayerDifficulty(pdata->playerIndex(), Difficulty(val->c_str()));
				}
			}
			break;

		case UI_ACTION_LAN_PLAYER_COLOR:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				int color_index = mission->playerData(pdata->playerIndex()).colorIndex;
				if(++color_index > GlobalAttributes::instance().playerAllowedColorSize())
					color_index = 0;
				mission->changePlayerData(pdata->playerIndex()).colorIndex = color_index;
				if(PNetCenter::isNCCreated())
					PNetCenter::instance()->changePlayerColor(pdata->playerIndex(), color_index);
			}
			break;

		case UI_ACTION_LAN_PLAYER_SIGN:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				int sign_index = mission->playerData(pdata->playerIndex()).signIndex;
				if(++sign_index > GlobalAttributes::instance().playerAllowedSignSize())
					sign_index = 0;
				mission->changePlayerData(pdata->playerIndex()).signIndex = sign_index;
				if(PNetCenter::isNCCreated())
					PNetCenter::instance()->changePlayerSign(pdata->playerIndex(), sign_index);
			}
			break;

		case UI_ACTION_PROFILE_DIFFICULTY:{
				const DifficultyPrm* val = 0;
				const wchar_t* name = control->text();
				const DifficultyTable::Strings& map = DifficultyTable::instance().strings();
				DifficultyTable::Strings::const_iterator it;
				FOR_EACH(map, it)
					if(!wcscmp(it->name(), name))
						val = &*it;
				if(val)
					currentProfile().difficulty = Difficulty(val->c_str());
				break;
										  }
		case UI_ACTION_GAME_SAVE:
			saveGame();
			break;

		case UI_ACTION_GAME_START:
			missionStart();
			break;

		case UI_ACTION_GAME_RESTART:
			missionReStart();
			break;

		case UI_ACTION_DELETE_SAVE:
			deleteSave();
			break;
		
		case UI_ACTION_RESET_MISSION:
			resetCurrentMission();
			break;

		case UI_ACTION_PROFILE_INPUT:
			profileName_ = control->text();
			break;
		
		case UI_ACTION_CDKEY_INPUT:
			w2a(currentProfile().cdKey, control->text());
			break;

		case UI_ACTION_PROFILE_CREATE:
			if(!profileName_.empty()){
				int idx = profileSystem().updateProfile(profileName_);
				if(idx >= 0)
					if(profileSystem().setCurrentProfile(idx)){
						handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_PROFILES_LIST, UI_ActionDataControlCommand::RE_INIT)));
						profileReseted();
					}
			}
			break;
		
		case UI_ACTION_PROFILE_DELETE:{
			int idx = profileSystem().getProfileByName(profileName_);
			if(idx >= 0){
				bool needReset = (idx == profileSystem().currentProfileIndex());
				profileSystem().removeProfile(idx);
				handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_PROFILES_LIST, UI_ActionDataControlCommand::RE_INIT)));
				if(needReset)
					profileReseted();
			}
			break;
									  }
		case UI_ACTION_PROFILE_SELECT:{
			int idx = profileSystem().getProfileByName(profileName_);
			if(idx >= 0){
				if(profileSystem().setCurrentProfile(idx))
					profileReseted();
			}
									  }
		  break;

		case UI_ACTION_DELETE_ONLINE_LOGIN_FROM_LIST:
			profileSystem().deleteOnlineLogin();
			handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_ONLINE_LOGIN_LIST, UI_ActionDataControlCommand::RE_INIT)));
			break;

		case UI_ACTION_SAVE_GAME_NAME_INPUT:
			w2a(saveGameName_, control->text());
			break;

		case UI_ACTION_REPLAY_NAME_INPUT:
			w2a(saveReplayName_, control->text());
			break;

		case UI_ACTION_COMMIT_GAME_SETTINGS:
			GameOptions::instance().commitSettings();
			break;

		case UI_ACTION_ROLLBACK_GAME_SETTINGS:
			GameOptions::instance().restoreSettings();
			handleMessageReInitGameOptions();
			break;

		case UI_ACTION_SOURCE_ON_MOUSE:
			uiStreamGraph2LogicCommand.set(fCommandSetSourceOnMouse) << safe_cast<const UI_ActionDataSourceOnMouse*>(data)->attr().get();
			break;

		case UI_ACTION_OPERATE_MODAL_MESSAGE:
			switch(safe_cast<const UI_ActionDataModalMessage*>(data)->type()){
			case UI_ActionDataModalMessage::CLOSE:
				handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_GET_MODAL_MESSAGE, UI_ActionDataControlCommand::CLEAR)));
				UI_Dispatcher::instance().closeMessageBox();
				break;
			case UI_ActionDataModalMessage::CLEAR:
				UI_Dispatcher::instance().messageBox(0);
				break;
			};
			break;

		case UI_ACTION_LAN_GAME_NAME_INPUT:
			w2a(currentProfile().lastCreateGameName, control->text());
			break;

		case UI_ACTION_INET_NAME:
			w2a(currentProfile().lastInetName, control->text());
			break;
		
		case UI_ACTION_INET_DIRECT_ADDRESS:
			currentProfile().lastInetDirectAddress = control->text();
			break;

		case UI_ACTION_INET_PASS:
			uiNetCenter().setPassword(w2a(control->text()).c_str());
			break;

		case UI_ACTION_INET_PASS2:
			uiNetCenter().setPass2(w2a(control->text()).c_str());
			break;

		case UI_ACTION_CHAT_EDIT_STRING:
			uiNetCenter().setChatString(control->text());
			break;

		case UI_ACTION_CHAT_SEND_MESSAGE:
			if(uiNetCenter().sendChatString(-1))
				handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_CHAT_EDIT_STRING, UI_ActionDataControlCommand::CLEAR)));
			break;

		case UI_ACTION_CHAT_SEND_CLAN_MESSAGE:
			if(uiNetCenter().sendChatString(gameShell->CurrentMission.playerData(gameShell->CurrentMission.activePlayerID()).clan))
				handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_CHAT_EDIT_STRING, UI_ActionDataControlCommand::CLEAR)));
			break;

		case UI_ACTION_INET_LOGIN:
			uiNetCenter().login();
			break;
		
		case UI_ACTION_INET_STATISTIC_QUERY:
			switch(safe_cast<const UI_ActionDataGlobalStats*>(data)->task()){
			case UI_ActionDataGlobalStats::REFRESH:
				uiNetCenter().queryGlobalStatistic(true);
				break;
			case UI_ActionDataGlobalStats::GET_PREV:
				uiNetCenter().getPrevGlobalStats();
				break;
			case UI_ActionDataGlobalStats::GET_NEXT:
				uiNetCenter().getNextGlobalStats();
				break;
			case UI_ActionDataGlobalStats::FIND_ME:
				uiNetCenter().getAroundMeGlobalStats();
				break;
			case UI_ActionDataGlobalStats::GOTO_BEGIN:
				uiNetCenter().getGlobalStatisticFromBegin();
				break;
			}
			break;

		case UI_ACTION_INET_QUICK_START:
			uiNetCenter().quickStart();
			break;

		case UI_ACTION_INET_REFRESH_GAME_LIST:
			uiNetCenter().refreshGameList();
			break;

		case UI_ACTION_LAN_CREATE_GAME:
			uiNetCenter().createGame();
			break;

		case UI_ACTION_LAN_JOIN_GAME:
			uiNetCenter().joinGame();
			break;
		
		case UI_ACTION_INET_DIRECT_JOIN_GAME:
			uiNetCenter().joinDirectGame();
			break;

		case UI_ACTION_INET_CREATE_ACCOUNT:
			uiNetCenter().createAccount();
			break;
		
		case UI_ACTION_INET_CHANGE_PASSWORD:
			uiNetCenter().changePassword();
			break;
		
		case UI_ACTION_INET_DELETE_ACCOUNT:
			uiNetCenter().deleteAccount();
			break;

		case UI_ACTION_LAN_PLAYER_JOIN_TEAM:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				uiNetCenter().teamConnect(pdata->playerIndex());
			}
			break;

		case UI_ACTION_LAN_PLAYER_LEAVE_TEAM:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				uiNetCenter().teamDisconnect(pdata->playerIndex(), pdata->teamIndex());
			}
			break;

		case UI_ACTION_INET_FILTER_PLAYERS_COUNT:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				currentProfile().playersSlotFilter = (lst->selectedStringIndex() <= 0
					? -1
					: clamp(lst->selectedStringIndex() + 1, 2, NETWORK_SLOTS_MAX));
				uiNetCenter().updateFilter();
			}
			break;

		case UI_ACTION_INET_FILTER_GAME_TYPE:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				currentProfile().gameTypeFilter = (lst->selectedStringIndex() <= 0
					? -1
					: clamp(lst->selectedStringIndex() - 1, 0, 1));
				uiNetCenter().updateFilter();
			}
			break;

		case UI_ACTION_MISSION_SELECT_FILTER:
		case UI_ACTION_MISSION_QUICK_START_FILTER:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				Profile::MissionFilter& filter = id == UI_ACTION_MISSION_SELECT_FILTER 
					? currentProfile().findMissionFilter
					: currentProfile().quickStartMissionFilter;

				filter.filterList.clear();

				if(UI_ControlStringCheckedList* chekedList = dynamic_cast<UI_ControlStringCheckedList*>(lst)){
					ComboWStrings strings;
					lst->getSelect(strings);

					ComboWStrings::const_iterator it;
					FOR_EACH(strings, it)
						if(const MissionDescription* mission = missions_.find(it->c_str()))
							filter.filterList.push_back(mission->missionGUID());

					filter.filterDisabled = (filter.filterList.size() == lst->listSize());
				}
				else {
					if(lst->selectedStringIndex() <= 0)
						filter.filterDisabled = true;
					else if(const wchar_t* name = lst->selectedString()){
						const MissionDescription* mission = missions_.find(name);
						xassert(mission);
						filter.filterDisabled = false;
						filter.filterList.push_back(mission->missionGUID());
					}
					else
						filter.filterDisabled = true;
				}

				uiNetCenter().updateFilter();
			}
			break;

		case UI_ACTION_QUICK_START_FILTER_RACE:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control))
				currentProfile().quickStartFilterRace = clamp(lst->selectedStringIndex(), 0, 3) - 1; // FIXME - надо искать по имени
			break;

		case UI_ACTION_QUICK_START_FILTER_POPULATION:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control))
				currentProfile().quickStartFilterGamePopulation = clamp(lst->selectedStringIndex(), 0, 3);
			break;

		case UI_ACTION_STATISTIC_FILTER_POPULATION:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				currentProfile().statisticFilterGamePopulation = clamp(lst->selectedStringIndex(), 0, 3);
				uiNetCenter().queryGlobalStatistic();
			}
			break;

		case UI_ACTION_STATISTIC_FILTER_RACE:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				currentProfile().statisticFilterRace = clamp(lst->selectedStringIndex(), 0, 2); // FIXME - надо искать по имени
				uiNetCenter().queryGlobalStatistic();
			}
			break;

		case UI_ACTION_GLOBAL_VARIABLE: {
			int val = -1;
			if(UI_ControlButton *but = dynamic_cast<UI_ControlButton*>(control))
				val = !but->currentStateIndex();
			else if(UI_ControlStringList *lst = dynamic_cast<UI_ControlStringList*>(control))
				val = lst->selectedStringIndex();
			if(safe_cast<const UI_ActionTriggerVariable*>(data)->type() == UI_ActionTriggerVariable::MISSION_DESCRIPTION){
				if(MissionDescription* mission = currentMission()){
					int flags = mission->triggerFlags() ^ (1 << safe_cast<const UI_ActionTriggerVariable*>(data)->number());
					mission->setTriggerFlags(flags);
					if(PNetCenter::isNCCreated())
						PNetCenter::instance()->changeMissionDescription(MissionDescription::CMDV_ChangeTriggers, flags);
				}
			}
			else
				uiStreamGraph2LogicCommand.set(fSetTriggerVariable) << safe_cast<const UI_ActionTriggerVariable*>(data)->name() << val;
									   }
			break;
		case UI_ACTION_CONFIRM_DISK_OP: {
			const UI_ActionDataConfirmDiskOp* dp = safe_cast<const UI_ActionDataConfirmDiskOp*>(data);
			if(dp->confirmDiskOp())
				diskOpConfirmed_ = true;
			else
				setDiskOp(UI_DISK_OP_NONE, "");
										}
			break;
	}
}

void UI_LogicDispatcher::expandTextTemplate(wstring& text, const ExpandInfo& info){
	const std::string::size_type npos = std::string::npos;
	std::wstring out;
	WBuffer outBuf(1024, true);

	string::size_type begin = 0, end = 0;

	for(;;){
		if(end >= text.size())
			break;

		begin = text.find(L'{', end);
		if(begin == npos){
			out += text.substr(end);
			break;
		}else if(begin && text[begin-1] == L'\\'){
			out += L'{';
			end = begin + 1;
			continue;
		}else
			out += text.substr(end, begin - end);

		end = text.find(L'}', begin);
		if(end == npos){
			out += text.substr(begin);
			break;
		}

		outBuf.init();
		if(text[begin + 1] == L'$'){ // обязательный тег
			const wchar_t* par = getParam(text.substr(begin + 2, end - begin - 2).c_str(), info, outBuf);
			if(par && *par)
				out += par;
			else {
				out.clear();
				break;
			}
		}
		else
			out += getParam(text.substr(begin + 1, end - begin - 1).c_str(), info, outBuf);

		++end;
	}

	text = out;
}

const wchar_t* UI_LogicDispatcher::getParam(const wchar_t* name, const ExpandInfo& info, WBuffer& retBuf)
{
	xassert(name && *name);

	ParameterSet cacheParams;
	const ParameterSet* params = 0;
	
	int plrID = -1;
	if(name[1] == L'#'){
		wchar_t numChar = *name;
		if(numChar >= 0x30 && numChar <= 0x39){
			name += 2;
			plrID = gameShell->CurrentMission.findPlayerIndex(numChar - 0x30);
		}
	}

	if(name[1] == L'!'){ // это из параметров юнита
		switch(*name){
		case L'c': // текущие личные параметры
			if(info.type == ExpandInfo::INVENTORY){
				if(info.item)
					params = &(info.item->parameters());
			}
			else if(info.unit)
				params = &(info.unit->getUnitReal()->parameters());
			break;
		case L'm': // максимальные личные параметры
			if(info.unit)
				params = &(info.unit->getUnitReal()->parametersMax());
			break;
		case L'b': // ресурсы для постройки
			if(info.attr)
				params = &(info.attr->creationValue);
			break;
		case L'i': // ресурсы для заказа
			if(info.attr)
				params = &(info.attr->installValue);
			break;
		case L'u': // ресурсы возвращенные при продаже
			if(info.attr)
				params = &(info.attr->uninstallValue);
			break;
		case L'n': // ресурсы возвращенные при отмене строительства
			if(info.attr)
				params = &(info.attr->cancelConstructionValue);
			break;
		case L'a': // необходимые параметры
			if(info.attr)
				params = &(info.attr->accessValue);
			break;
		case L'w': // параметр из личных параметров оружия
		case L'd': // параметр из урона оружия
		case L's': // из abnormalState
		case L'e': // из accessValue оружия
		case L'o': // стоимость применения оружия
			if(info.unit && info.attr->isActing()){
				wchar_t type = *name;
				//имя параметра из оружия задается в виде: WeaponName/ParameterNameOrModifer
				//WeaponName - ключ локализации для имени оружия
				name += 2;
				if(const wchar_t* delimeter = wcschr(name, L'/')){
					const WeaponBase* weapon = 0;
					const UnitActing* shooter = safe_cast<const UnitActing*>(info.unit->getUnitReal());
					if(name != delimeter){
						weapon = shooter->findWeapon(w2a(wstring(name, delimeter)).c_str());
					}
					else if(int weaponID = shooter->selectedWeaponID())
						weapon = shooter->findWeapon(weaponID);  // текущее выбранное оружие

					++delimeter;
					if(weapon && weapon->isEnabled())
						if(type == L'e')
							params = &weapon->weaponPrm()->accessValue();
						else if(type == L'o'){
							weapon->weaponPrm()->getFireCostReal(cacheParams);
							params = &cacheParams;
						}
						else if(weapon->prmCache().getParametersForUI(delimeter, type, cacheParams))
							params = &cacheParams;
					name = delimeter - 2;
				}
			}
			else if(info.attr->isInventoryItem()){
				wchar_t type = *name;
				if(name[2] == L'/')
					++name; // имя оружия для предмета не требуется - оно там одно
				if(const WeaponPrm* prm = safe_cast<const AttributeItemInventory*>(info.attr)->weaponReference)
					if(prm->ID() != -1){
						if(type == L'e')
							params = &prm->accessValue();
						else if(type == L'o'){
							prm->getFireCostReal(cacheParams);
							params = &cacheParams;
						}
						else {
							name += 2;
							WeaponPrmCache cache = *player()->weaponPrmCache(prm);
							prm->initCache(cache);
							if(cache.getParametersForUI(name, type, cacheParams))
								params = &cacheParams;
							name -= 2;
						}
					}
			}
			break;
		case L'*': // параметры из текущего контрола под мышой, стоимость команды показывается для заселекченного
			if(info.control){
				if(name[2] && name[3] == L'/'){
					name += 2;
					wchar_t type = *name;
					switch(type){
					case L'w': // параметр из личных параметров оружия
					case L'd': // параметр из урона оружия
					case L's': // из abnormalState
					case L'e': // из accessValue оружия
					case L'o': // стоимость применения оружия
						if(const WeaponPrm* prm = info.control->actionWeapon())
							if(prm->ID() != -1){
								if(type == L'e')
									params = &prm->accessValue();
								else if(type == L'o'){
									prm->getFireCostReal(cacheParams);
									params = &cacheParams;
								}
								else {
									name += 2;
									WeaponPrmCache cache = *player()->weaponPrmCache(prm);
									prm->initCache(cache);
									if(cache.getParametersForUI(name, type, cacheParams))
										params = &cacheParams;
									name -= 2;
								}
							}
						break;
					}
				}
				else if(info.control->actionParameters(selectManager->selectedAttribute(), cacheParams))
					params = &cacheParams;
			}
		
			break;
		case L'r': // из арифметики
			name += 2;
			if(info.type == ExpandInfo::INVENTORY && info.item)
				if(*name)
					info.item->arithmetics().getUIData(retBuf, w2a(name).c_str());
				else
					info.item->arithmetics().getUIData(retBuf);
			else if(info.attr)
				if(*name)
					info.attr->parametersArithmetics.getUIData(retBuf, w2a(name).c_str());
				else
					info.attr->parametersArithmetics.getUIData(retBuf);
			return retBuf.c_str();
		}
		name += 2;
	}
	else if(name[1] == L'&'){ // это из параметров игрока
		const Player* plr = plrID < 0 ? universe()->activePlayer() : universe()->findPlayer(plrID);
		xassert(plr);
		switch(*name){
		case L'c': // текущие ресурсы игрока
			params = &(plr->resource());
			break;
		case L'm': // максимальные ресурсы игрока
			params = &(plr->resourceCapacity());
			break;
		}
		name += 2;
	}
	else if(*name == L'@'){
		retBuf < TextDB::instance().getText(w2a(name + 1).c_str());
		return retBuf.c_str();
	}
	else {
		UI_TextTags::const_iterator it = uiTextTags.find(name);
		if(it != uiTextTags.end()){
			switch(it->second){
			case UI_TAG_PLAYER_DISPLAY_NAME:
				if(plrID >= 0){
					const Player* plr = universe()->findPlayer(plrID);
					if(plr->realPlayerType() & REAL_PLAYER_TYPE_PLAYER)
						retBuf < plr->name();
					else
						retBuf < GET_LOC_STR(UI_COMMON_TEXT_AI);
				}
				break;
			case UI_TAG_PLAYERS_COUNT:
				retBuf <= universe()->playersNumber();
				break;
			case UI_TAG_CURRENT_SERVER_ADDRESS: {
				WBuffer name;
				if(uiNetCenter().currentServerAddress(name))
					retBuf < name.c_str();
				break;
												}
			case UI_TAG_CURRENT_GAME_NAME:
				retBuf < gameShell->CurrentMission.interfaceName();
				break;
			case UI_TAG_SELECTED_GAME_NAME:
				if(const MissionDescription* mission = currentMission())
					retBuf < mission->interfaceName();
				break;
			case UI_TAG_SELECTED_MULTIPLAYER_GAME_NAME:{
				WBuffer name;
				if(uiNetCenter().selectedGameName(name))
					retBuf < name.c_str();
				break;
													   }
			case UI_TAG_SELECTED_GAME_SIZE:
				if(const MissionDescription* mission = currentMission())
					retBuf < mission->getMapSizeName();
				break;
			case UI_TAG_SELECTED_GAME_TYPE:
				if(const MissionDescription* mission = currentMission())
					if(mission->useMapSettings())
						retBuf < GET_LOC_STR(UI_COMMON_TEXT_PREDEFINE_GAME);
					else
						retBuf < GET_LOC_STR(UI_COMMON_TEXT_CUSTOM_GAME);
				break;
			case UI_TAG_SELECTED_GAME_PLAYERS:
				if(const MissionDescription* mission = currentMission())
					retBuf <= mission->playersAmountMax();
				break;
			case UI_TAG_CURRENT_CHAT_CHANNEL:{
				wstring get;
				getNetCenter().getCurrentChatChannelName(get);
				retBuf < get.c_str();
				break;
											 }
			case UI_TAG_HOTKEY: // hotkey для кнопки под мышкой
				if(const UI_ControlBase* hov = hoverControl()){
					WBuffer keyName;
					if(const UI_ActionDataHotKey* hotKeyAction = safe_cast<const UI_ActionDataHotKey*>(hov->findAction(UI_ACTION_HOT_KEY)))
						retBuf < hotKeyAction->hotKey().toString(keyName);
				}
				break;
			case UI_TAG_ACCESSIBLE: // что надо для доступности юнита
				if(info.attr)
					player()->printAccessible(retBuf, info.attr->accessBuildingsList, enableColorString(), disableColorString());
				break;
			case UI_TAG_ACCESSIBLE_PARAM: // что надо для доступности параметра
				if(info.control && info.attr)
					if(const ProducedParameters* par = info.control->actionBuildParameter(info.attr))
						player()->printAccessible(retBuf, par->accessBuildingsList, enableColorString(), disableColorString());
				break;
			case UI_TAG_LEVEL: // уровень юнита
				if(info.unit && info.unit->getUnitReal()->attr().isLegionary())
					retBuf <= safe_cast<const UnitLegionary*>(info.unit->getUnitReal())->level() + 1;
				break;
			case UI_TAG_ACCOUNTSIZE: // сколько слотов занимает юнит
				if(info.attr)
					retBuf <= info.attr->accountingNumber;
				break;
			case UI_TAG_SUPPLYSLOTS: // сколько слотов занимает юнит (с раскраской)
				if(info.attr){
					if(universe()->activePlayer()->checkUnitNumber(info.attr))
						retBuf < enableColorString();
					else
						retBuf < disableColorString();
					retBuf <= info.attr->accountingNumber;
				}
				break;
			case UI_TAG_SQUAD_ACCOUNTSIZE: // сколько слотов занимает сквад юнита
				if(info.attr && info.attr->isLegionary())
					retBuf <= safe_cast<const AttributeLegionary*>(info.attr)->squad->accountingNumber;
				break;
			case UI_TAG_SQUAD_SUPPLYSLOTS: // сколько слотов занимает сквад юнита (с раскраской)
				if(info.attr && info.attr->isLegionary()){
					if(universe()->activePlayer()->checkUnitNumber(info.attr))
						retBuf < enableColorString();
					else
						retBuf < disableColorString();
					retBuf <= safe_cast<const AttributeLegionary*>(info.attr)->squad->accountingNumber;
				}
				break;
			case UI_TAG_TIME_H12:{
				xassert(environment);
				int time = environment->getTime();
				if(time < 1)
					time = 12;
				else if(time > 12)
					time -= 12.f;
				if(time < 10)
					retBuf < L"0";
				retBuf <= time;
				break;
								 }
			case UI_TAG_TIME_AMPM:{
				xassert(environment);
				int time = environment->getTime();
				UI_CommonLocText tm = UI_COMMON_TEXT_TIME_AM;
				if(time > 12 || time < 1)
					tm = UI_COMMON_TEXT_TIME_PM;
				retBuf < getLocString(tm, tm == UI_COMMON_TEXT_TIME_AM ? L"a.m." : L"p.m.");
				break;
								  }
			case UI_TAG_TIME_H24:{
				xassert(environment);
				int time = environment->getTime();
				if(time < 10)
					retBuf < L"0";
				retBuf <= time;
				break;
								 }
			case UI_TAG_TIME_M:{
				xassert(environment);
				float time = environment->getTime();
				time -= floor(time);
				time *= 60.f;
				if(time < 10.f)
					retBuf < L"0";
				retBuf <= int(time);
				break;
							   }
			default:
				xxassert(false, "такого быть не должно!");
				retBuf.init();
			}
			return retBuf.c_str();
		}
		retBuf.init();
		return retBuf.c_str();
	}

	if(params){
		switch(*name){
		case L'?':  // чего не хватает у игрока
			player()->resource().printSufficient(retBuf, *params, enableColorString(), disableColorString(), w2a(name+1).c_str());
		case L'&': // чего не хватает у селекченного юнита
			if(const UnitInterface* unit = selectedUnitIfOne())
				unit->getUnitReal()->parameters().printSufficient(retBuf, *params, enableColorString(), disableColorString(), w2a(name+1).c_str());
			break;
		case L'!': // просто напечатать все параметры
			params->toString(retBuf);
		default: // вывести конкретный параметр
			retBuf <= round(params->findByLabel(w2a(name).c_str()));
		}
		return retBuf.c_str();
	}
	retBuf.init();
	return retBuf.c_str();
}

const Vect3f& UI_LogicDispatcher::cursorPosition() const
{
	return (selectedWeapon_ && !selectedWeapon_->targetOnWaterSurface()) ? hoverPositionTerrain() : hoverPosition();
}

WeaponTarget UI_LogicDispatcher::attackTarget(int weapon_id) const
{
	UnitInterface* unit = hoverUnit_;
	if(unit){
		if(unit->attr().isSquad())
			unit = unit->getUnitReal();
		else if(!unit->attr().isObjective())
			unit = 0;
	}
	return WeaponTarget(unit, cursorPosition(), weapon_id ? weapon_id : selectedWeaponID());
}

void UI_LogicDispatcher::selectCursor(const UI_Cursor* cameraCursor)
{
#ifndef _FINAL_VERSION_
#define SET_CURSOR_REASON(reason) debugCursorReason_ = (reason)
#else
#define SET_CURSOR_REASON(reason)
#endif

	const UI_GlobalAttributes& ui = UI_GlobalAttributes::instance();
	UnitReal* hovered_unit = (gameShell->GameActive && hoverUnit() ? hoverUnit()->getUnitReal() : 0);

	if(cursorTriggered())
	{// Ничего не делаем - работает курсор, назначенный триггером
		SET_CURSOR_REASON("by trigger");
	}
	else if(cameraCursor){
		SET_CURSOR_REASON("by camera");
		setCursor(cameraCursor);
	}
	else if(const UI_ControlBase* control = UI_LogicDispatcher::instance().hoverControl())
	{// Курсор на интерфейсе - нужный курсор должен назначить	обработчик мышки для объектов интерфейса
		const UI_ActionDataHoverInfo* info = UI_LogicDispatcher::instance().hoverControlInfo();
		if(info && info->hoveredCursor()){
			SET_CURSOR_REASON("control own hover cursor");
			setCursor(info->hoveredCursor());
		}
		else if(gameShell->GameActive){
			SET_CURSOR_REASON("general interface");
			setCursor(ui.cursor(UI_CURSOR_INTERFACE));
		}
		else {
			SET_CURSOR_REASON("main menu");
			setCursor(ui.cursor(UI_CURSOR_MAIN_MENU));
		}
		showCursor();
	}
	else if(UI_Dispatcher::instance().hasFocusedControl()){
		if(gameShell->GameActive){
			SET_CURSOR_REASON("general interface, has focused control");
			setCursor(ui.cursor(UI_CURSOR_INTERFACE));
		}
		else {
			SET_CURSOR_REASON("main menu, has focused control");
			setCursor(ui.cursor(UI_CURSOR_MAIN_MENU));
		}
		showCursor();
	}
	else if(!gameShell->GameActive){
		SET_CURSOR_REASON("game not started, main menu");
		setCursor(ui.cursor(UI_CURSOR_MAIN_MENU));
	}
	else if(currentWeaponCursor_){
		setCursor(currentWeaponCursor_);
	}
	else if(!player()->controlEnabled()){
		SET_CURSOR_REASON("player control disabled");
		setCursor(ui.cursor(UI_CURSOR_PLAYER_CONTROL_DISABLED));
	}
	else if(gameShell->underFullDirectControl() && !gameShell->isPaused(GameShell::PAUSE_BY_ANY)){
		if(UI_LogicDispatcher::instance().isAttackCursorEnabled()){
			SET_CURSOR_REASON("direct control, attack cursor");
			setCursor(ui.cursor(UI_CURSOR_DIRECT_CONTROL_ATTACK));
		}
		else {
			SET_CURSOR_REASON("direct control, can't attack");
			setCursor(ui.cursor(UI_CURSOR_DIRECT_CONTROL));
		}
	}
	else if (isMouseFlagSet(MK_LBUTTON) &&
		ui.cursor(UI_CURSOR_MOUSE_LBUTTON_DOWN))
	{
		SET_CURSOR_REASON("left button down");
		setCursor(ui.cursor(UI_CURSOR_MOUSE_LBUTTON_DOWN));	
	}
	else if	(isMouseFlagSet(MK_RBUTTON) &&
		ui.cursor(UI_CURSOR_MOUSE_RBUTTON_DOWN))
	{
		SET_CURSOR_REASON("right button down");
		setCursor(ui.cursor(UI_CURSOR_MOUSE_RBUTTON_DOWN));	
	}
	else if(clickMode() != UI_CLICK_MODE_NONE){
		switch(clickMode()){
				case UI_CLICK_MODE_ASSEMBLY:
					SET_CURSOR_REASON("click mode set assembly point");
					setCursor(ui.cursor(UI_CURSOR_ASSEMBLY_POINT));
					break;
				case UI_CLICK_MODE_MOVE:
					if(hoverPassable()){
						SET_CURSOR_REASON("click mode move, can walk");
						setCursor(ui.cursor(UI_CURSOR_WALK));
					}
					else {
						SET_CURSOR_REASON("click mode move, walk disabled");
						setCursor(ui.cursor(UI_CURSOR_WALK_DISABLED));
					}
					break;
				case UI_CLICK_MODE_PATROL:
					if(hoverPassable()){
						SET_CURSOR_REASON("click mode patrol, can walk");
						setCursor(ui.cursor(UI_CURSOR_PATROL));
					}
					else {
						SET_CURSOR_REASON("click mode patrol, walk disabled");
						setCursor(ui.cursor(UI_CURSOR_PATROL_DISABLED));
					}
					break;
				case UI_CLICK_MODE_ATTACK:
				case UI_CLICK_MODE_PLAYER_ATTACK:
					if(isAttackCursorEnabled())
						if(hovered_unit)
							if(hovered_unit->player()->clan() == universe()->activePlayer()->clan()){
								SET_CURSOR_REASON("click mode attack, hover unit, eq clans (friend attack)");
								setCursor(ui.cursor(UI_CURSOR_FRIEND_ATTACK));
							}
							else {
								SET_CURSOR_REASON("click mode attack, hover not friendly unit");
								setCursor(ui.cursor(UI_CURSOR_ATTACK));
							}
						else {
							SET_CURSOR_REASON("click mode attack, attack possible");
							setCursor(ui.cursor(UI_CURSOR_ATTACK));
						}
					else {
						SET_CURSOR_REASON("click mode attack, attack disabled");
						setCursor(ui.cursor(UI_CURSOR_ATTACK_DISABLED));
					}
					break;
				default:
					xxassert(0, "new click mode type?");
		}
	}
	else if (universe()->activePlayer() && hovered_unit){ // Если юниты под мышкой найдены
		const UI_Cursor* unit_cur = hovered_unit->attr().selectionCursorProxy();
		if(unit_cur){ // В приоритете курсор, назначенный объекту - если его нет, то вешаем общие курсоры
			SET_CURSOR_REASON("object hover, object own cursor");
			setCursor(unit_cur);
		}
		else if(hovered_unit->attr().isResourceItem() || hovered_unit->attr().isInventoryItem())
			if(hovered_unit->attr().isInventoryItem() && selectManager->canPickItem(safe_cast<const UnitItemInventory*>(hovered_unit))){
				SET_CURSOR_REASON("object hover, selected unit can pick object");
				setCursor(ui.cursor(UI_CURSOR_ITEM_CAN_PIC));
			}
			else if(hovered_unit->attr().isResourceItem() && selectManager->canExtractResource(safe_cast<const UnitItemResource*>(hovered_unit))){
				SET_CURSOR_REASON("object hover, selected unit can extract from object");
				setCursor(ui.cursor(UI_CURSOR_ITEM_EXTRACT));
			}
			else {
				SET_CURSOR_REASON("object hover, this is resource or inventory item");
				setCursor(ui.cursor(UI_CURSOR_ITEM_OBJECT));
			}
		else if(hovered_unit->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>(hovered_unit->attr()).teleport && 
				selectManager->canTeleportate(safe_cast<const UnitBuilding*>(hovered_unit))){
			SET_CURSOR_REASON("object hover, can teleportate selected unit");
			setCursor(ui.cursor(UI_CURSOR_TELEPORT));
		}
		else if(hovered_unit->attr().isActing() && hovered_unit->attr().isTransport() && selectManager->canPutInTransport(safe_cast<const UnitActing*>(hovered_unit))){
			SET_CURSOR_REASON("this is transport, selected can put in");
			setCursor(ui.cursor(UI_CURSOR_TRANSPORT)); // для транспортов в которые можно сесть отдельный курсор
		}
		else if(universe()->activePlayer()->clan() == hovered_unit->player()->clan())
			if(isAttackCursorEnabled()){
				SET_CURSOR_REASON("object hover, can attack, eq clans (friend attack)");
				setCursor(ui.cursor(UI_CURSOR_FRIEND_ATTACK));
			}
			else
				if(universe()->activePlayer() == hovered_unit->player())
					if(hovered_unit->attr().isBuilding() && !safe_cast<const UnitBuilding*>(hovered_unit)->isConstructed()
						&& selectManager->canBuild(hovered_unit)){
							SET_CURSOR_REASON("own object hover, this is not constructed building, selected build this");
							setCursor(ui.cursor(UI_CURSOR_CAN_BUILD)); // может достроить этот долгострой
					}
					else {
						SET_CURSOR_REASON("own object hover");
						setCursor(ui.cursor(UI_CURSOR_PLAYER_OBJECT));
					}
				else {
					SET_CURSOR_REASON("any object hover, eq clans, can't attack this");
					setCursor(ui.cursor(UI_CURSOR_ALLY_OBJECT));
				}
		else if(hovered_unit->player()->isWorld()){
			SET_CURSOR_REASON("any world object hover");
			setCursor(ui.cursor(UI_CURSOR_WORLD_OBJECT));
		}
		else if(isAttackCursorEnabled()){
			SET_CURSOR_REASON("any enemy object hover, can attack");
			setCursor(ui.cursor(UI_CURSOR_ENEMY_OBJECT));
		}
		else {
			SET_CURSOR_REASON("object hover, can't attack this");
			setCursor(ui.cursor(UI_CURSOR_ATTACK_DISABLED));
		}
	}
	else if(cursorInWorld()) // Если пересеклись с землей
		if(!hoverPassable()) {
			SET_CURSOR_REASON("impassability region in world");
			setCursor(ui.cursor(UI_CURSOR_IMPASSABLE));
		}
		else if(hoverWater()) {
			SET_CURSOR_REASON("water in world");
			setCursor(ui.cursor(UI_CURSOR_WATER));
		}
		else {
			SET_CURSOR_REASON("in world, can walk to this pint");
			setCursor(ui.cursor(UI_CURSOR_PASSABLE));
		}
	else {
		SET_CURSOR_REASON("out of world");
		setCursor(ui.cursor(UI_CURSOR_IMPASSABLE));
	}
#undef SET_CURSOR_REASON
}

void UI_LogicDispatcher::cancelActions()
{
	MTG();
	buildingInstaller_.Clear();

	selectClickMode(UI_CLICK_MODE_NONE);
}

void UI_LogicDispatcher::setDiskOp(UI_DiskOpID id, const char* path)
{
	diskOpID_ = id;
	diskOpPath_ = path;

	diskOpGameType_ = gameShell->CurrentMission.gameType();

	diskOpConfirmed_ = false;
}

bool UI_LogicDispatcher::checkDiskOp(UI_DiskOpID id, const char* path)
{
	if(UI_Dispatcher::instance().autoConfirmDiskOp() ||	diskOpConfirmed_)
		return true;

	switch(id){
	case UI_DISK_OP_SAVE_GAME:
		return !profileSystem().isSaveExist(path, gameShell->CurrentMission.gameType());
	case UI_DISK_OP_SAVE_REPLAY:
		return !profileSystem().isReplayExist(path, GAME_TYPE_REEL);
	}

	return true;
}

void UI_LogicDispatcher::networkDisconnect(bool hard)
{
	MTL();
	gameShell->checkEvent(EventNet(Event::NETWORK_DISCONNECT, hard));
}

void fSendProfileReseted(void* data)
{
	UI_LogicDispatcher::instance().profileReseted();
}

void UI_LogicDispatcher::profileReseted()
{
	if(MT_IS_LOGIC())
		gameShell->checkEvent(Event(Event::RESET_PROFILE));
	else
		uiStreamGraph2LogicCommand.set(fSendProfileReseted);
}

void UI_LogicDispatcher::clearGameChat()
{
	MTL();
	chatMessages_.clear();
	chatMessageTimes_.clear();
}

//void UI_LogicDispatcher::handleNetwork(eNetMessageCode message)
//{
//	UI_CommonLocText locKey = UI_COMMON_TEXT_LAST_ENUM;
//	string str;

	//switch(message){
	//case NetGEC_ConnectionFailed:
	//	str="General error:ConnectionFailed";
	//	PNetCenter::instance()->FinishGame();
	//	locKey = UI_COMMON_TEXT_ERROR_SESSION_TERMINATE;
	//	break;
	//case NetGEC_HostTerminatedSession:
	//	PNetCenter::instance()->FinishGame();
	//	locKey = UI_COMMON_TEXT_ERROR_SESSION_TERMINATE;
	//	str="HostTerminatedSession";
	//	break;
	//case NetGEC_DWLobbyConnectionFailed:
	//	locKey = UI_COMMON_TEXT_ERROR_DISCONNECT;
	//	str="DWLobbyConnectionFailed";
	//	break;
	//case NetGEC_DWLobbyConnectionFailed_MultipleLogons:
	//	locKey = UI_COMMON_TEXT_ERROR_DISCONNECT_MULTIPLE_LOGON;
	//	str="DWLobbyConnectionFailed_MultipleLogons";
	//	break;

	//case NetGEC_GameDesynchronized:
	//	locKey = UI_COMMON_TEXT_ERROR_DESINCH;
	//	str="GameDesynchronized";
	//	break;

		//Init
	//case NetRC_Init_Ok:
	//	break;
	//case NetRC_Init_Err:
	//	PNetCenter::instance()->stopNetCenter();
	//	locKey = UI_COMMON_TEXT_ERROR_CANT_CONNECT;
	//	str="Internet init error";
	//	break;


		//Create Account
	//case NetRC_CreateAccount_Ok:
	//	break;
	//case NetRC_CreateAccount_BadLicensed:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE_BAD_LIC;
	//	str="CreateAccount Err: BadLicensed";
	//	break;
	//case NetRC_CreateAccount_IllegalOrEmptyPassword_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_PASSWORD;
	//	str="CreateAccount Err: NetRC_CreateAccount_IllegalOrEmptyPassword_Err";
	//	break;
	//case NetRC_CreateAccount_IllegalUserName_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_NAME;
	//	str="CreateAccount Err: IllegalUserName";
	//	break;
	//case NetRC_CreateAccount_VulgarUserName_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_VULGAR_NAME;
	//	str="CreateAccount Err: VulgarUserName";
	//	break;
	//case NetRC_CreateAccount_UserNameExist_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_NAME_EXIST;
	//	str="CreateAccount Err: UserNameExist";
	//	break;
	//case NetRC_CreateAccount_MaxAccountExceeded_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE_MAX;
	//	str="CreateAccount Err: MaxAccountExceeded";
	//	break;
	//case NetRC_CreateAccount_Other_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE;
	//	str="CreateAccount Err: Unknown";
	//	break;

	//case NetRC_ChangePassword_Ok:
	//	break;
	//case NetRC_ChangePassword_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_CHANGE_PASSWORD;
	//	str="ChangePassword Err.";
	//	break;

	//case NetRC_DeleteAccount_Ok:
	//	break;
	//case NetRC_DeleteAccount_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_DELETE;
	//	str="DeleteAccount Err.";
	//	break;


		//Configurate
	//case NetRC_Configurate_Ok:
	//	break;
	//case NetRC_Configurate_ServiceConnect_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_CONNECTION;
	//	str="Configurate: ServiceConnect Err";
	//	break;
	//case NetRC_Configurate_UnknownName_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_UNKNOWN_NAME;
	//	str="Configurate: UnknownName";
	//	break;
	//case NetRC_Configurate_IncorrectPassword_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_INCORRECT_PASSWORD;
	//	str="Configurate: IncorrectPassword";
	//	break;
	//case NetRC_Configurate_AccountLocked_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_LOCKED;
	//	str="Configurate: AccountLocked";
	//	break;


	//case NetRC_CreateGame_Ok:
	//	setCurrentMission(PNetCenter::instance()->getCurrentMissionDescription());
	//	break;
	//case NetRC_CreateGame_CreateHost_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_CREATE_GAME;
	//	str="Create Game Error";
	//	break;

	//case NetRC_QuickStart_Ok:
	//	break;
	//case NetRC_QuickStart_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_UNKNOWN;
	//	break;

	//case NetRC_JoinGame_Ok:
	//	//setCurrentMission(PNetCenter::instance()->getCurrentMissionDescription());
	//	break;
	//case NetRC_JoinGame_GameSpyPassword_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_INCORRECT_PASSWORD;
	//	break;
	//case NetRC_JoinGame_GameSpyConnection_Err:
	//case NetRC_JoinGame_Connection_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_CONNECTION_GAME;
	//	str="Join Game: Connection Error";
	//	break;
	//case NetRC_JoinGame_GameIsRun_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_CONNECTION_GAME_IS_RUN;
	//	str="Join Game Connection Error: GameIsRun";
	//	break;
	//case NetRC_JoinGame_GameIsFull_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_CONNECTION_GAME_IS_FULL;
	//	str="Join Game Connection Error: GameIsFull";
	//	break;
	//case NetRC_JoinGame_GameNotEqualVersion_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_INCORRECT_VERSION;
	//	str="Join Game Connection Error: GameNotEqualVersion";
	//	break;

	//case NetRC_ReadStats_Ok:
	//case NetRC_ReadStats_Empty:
	//case NetRC_WriteStats_Ok:
	//case NetRC_ReadGlobalStats_Ok:
	//case NetRC_LoadInfoFile_Ok:
	//	break;
	//case NetRC_ReadStats_Err:
	//case NetRC_WriteStats_Err:
	//case NetRC_ReadGlobalStats_Err:
		//locKey = UI_COMMON_TEXT_ERROR_UNKNOWN;
	//	break;
	
	//case NetRC_LoadInfoFile_Err:
	//	locKey = UI_COMMON_TEXT_ERROR_CONNECTION;
	//	str="Read version info failed";
	//	if(demonware())
	//		demonware()->logout();
	//	break;

	//case NetRC_Subscribe2ChatChanel_Ok:
	//	getNetCenter().chatSubscribeOK();
	//	return;
	//case NetRC_Subscribe2ChatChanel_Err:
	//	getNetCenter().chatSubscribeFailed();
	//	return;

	//case NetMsg_PlayerDisconnected:
	//case NetMsg_PlayerExit:
	//	break;

	//default:
	//	locKey = UI_COMMON_TEXT_ERROR_UNKNOWN;
	//	str="Unknown Message-Error";
	//}

//	switch(locKey){
//	case UI_COMMON_TEXT_LAST_ENUM:
//		getNetCenter().commit(UI_NET_OK);
//		return;
//	case UI_COMMON_TEXT_ERROR_CANT_CONNECT:
//	case UI_COMMON_TEXT_ERROR_DISCONNECT:
//	case UI_COMMON_TEXT_ERROR_DISCONNECT_MULTIPLE_LOGON:
//		getNetCenter().commit(UI_NET_SERVER_DISCONNECT);
//	    break;
//	case UI_COMMON_TEXT_ERROR_SESSION_TERMINATE:
//		getNetCenter().commit(UI_NET_TERMINATE_SESSION);
//		break;
//	default:
//		getNetCenter().commit(UI_NET_ERROR);
//	    break;
//	}
//
//#ifndef _FINAL_VERSION_
//		XBuffer buf;
//		buf < str.c_str() < " (" <= (int)message < ")\n";
//		buf < getLocString(locKey, "NO COMMON LOCTEXT KEY");
//		UI_Dispatcher::instance().messageBox(buf);
//#else
//		UI_Dispatcher::instance().messageBox(getLocString(locKey, "NO MESSAGE"));
//#endif
//}

bool UI_LogicDispatcher::makeDiskOp(UI_DiskOpID id, const char* path, GameType game_type)
{
	switch(id){
	case UI_DISK_OP_SAVE_GAME:
		if(gameShell->universalSave(path, true)){
			profileSystem().newSave(MissionDescription(path, game_type));
			currentProfile().lastSaveGameName = saveGameName_;

			handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_SAVE_GAME_NAME_INPUT, UI_ActionDataControlCommand::CLEAR)));
			handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_MISSION_LIST, UI_ActionDataControlCommand::RE_INIT)));
		}
		else 
			UI_Dispatcher::instance().messageBox(GET_LOC_STR(UI_COMMON_TEXT_ERROR_SAVE));
		break;
	case UI_DISK_OP_SAVE_REPLAY:
		if(universeX()->savePlayReel(path)){
			profileSystem().newSave(MissionDescription(path, GAME_TYPE_REEL));
			currentProfile().lastSaveReplayName = saveReplayName_;

			handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_REPLAY_NAME_INPUT, UI_ActionDataControlCommand::CLEAR)));
			handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_MISSION_LIST, UI_ActionDataControlCommand::RE_INIT)));
		}
		else 
			UI_Dispatcher::instance().messageBox(GET_LOC_STR(UI_COMMON_TEXT_ERROR_SAVE));
		break;
	}

	setDiskOp(UI_DISK_OP_NONE, "");
	return true;
}

bool UI_LogicDispatcher::parseGameVersion(const char* ptr)
{

	int hVer = lastCurrentHiVer_;
	int lVer = lastCurrentLoVer_;

	// пропуск до первой цифры
	while(*ptr && *ptr != '\r' && *ptr != '\n' && !isdigit(*ptr))
		++ptr;

	XBuffer tag;
	while(*ptr && isdigit(*ptr))
		tag < *ptr++;

	if(!*ptr || !tag.tell())
		return false;

	tag < '\0';
	tag.set(0);
	tag >= hVer;

	if(*ptr == '.')
		++ptr;

	tag.init();
	while(*ptr && isdigit(*ptr))
		tag < *ptr++;

	if(!*ptr || !tag.tell())
		return false;

	tag < '\0';
	tag.set(0);
	tag >= lVer;

	while(*ptr && *ptr != '\n')
		++ptr;
	if(!*ptr)
		return false;
	
	++ptr;

	lastCurrentHiVer_ = hVer;
	lastCurrentLoVer_ = lVer;

	updateUrls_.clear();

	ComboStrings urls;
	splitComboList(urls, ptr, '\n');

	if(urls.empty())
		return false;

	ComboStrings::const_iterator it;
	FOR_EACH(urls, it){
		string url;
		string::const_iterator del = find(it->begin(), it->end(), ':');
		if(it->end() - del > 10){
			int langIndex = -1;
			string lang(it->begin(), del);
			if(stricmp(lang.c_str(), "ru") == 0)
				langIndex = UI_COMMON_TEXT_LANG_RUSSIAN;
			else if(stricmp(lang.c_str(), "en") == 0)
				langIndex = UI_COMMON_TEXT_LANG_ENGLISH;
			else if(stricmp(lang.c_str(), "de") == 0)
				langIndex = UI_COMMON_TEXT_LANG_GERMAN;
			else if(stricmp(lang.c_str(), "fr") == 0)
				langIndex = UI_COMMON_TEXT_LANG_FRENCH;
			else if(stricmp(lang.c_str(), "sp") == 0)
				langIndex = UI_COMMON_TEXT_LANG_SPANISH;
			else if(stricmp(lang.c_str(), "it") == 0)
				langIndex = UI_COMMON_TEXT_LANG_ITALIAN;
			
			++del;
			while(del != url.end() && !isgraph(*del))
				++del;
			string url(del, it->end());
			while(!url.empty() && !isgraph(*(url.end()-1)))
				url.pop_back();

			if(url.size() < 10)
				continue;

			updateUrls_.push_back(make_pair(langIndex, url));
		}
	}
	return !updateUrls_.empty();
}

bool UI_LogicDispatcher::checkNeedUpdate() const
{
	return lastCurrentHiVer_ > UI_Dispatcher::instance().gameMajorVersion()
		|| lastCurrentLoVer_ > UI_Dispatcher::instance().gameMinorVersion();
}

void UI_LogicDispatcher::openUpdateUrl() const
{
	if(updateUrls_.empty())
		return;

	int lang = GameOptions::instance().getTranslate();
	string url;

	UpdateUrls::const_iterator it;
	FOR_EACH(updateUrls_, it)
		if(it->first == lang){
			url = it->second;
			break;
		}
		else if(it->first == -1)
			url = it->second;

	if(url.empty())
		return;

	ShellExecute(0, "open", url.c_str(), 0, 0, SW_SHOWNORMAL);
}


UnitObjective* UI_ActionDataFindUnit::getUnit() const
{
	if(unitReferences_.empty())
		return 0;

	if(referenceIdx_ >= unitReferences_.size()){
		referenceIdx_ = 0;
		unitIdx_ = 0;
	}

	int startRefIdx = referenceIdx_;
	int startUnitIdx = unitIdx_;

	if(Player* player = UI_LogicDispatcher::instance().player()){
		CUNITS_LOCK(player);
		int unitsSize = player->realUnits(unitReferences_[referenceIdx_]).size();
		do 
		{
			++unitIdx_;
			if(unitIdx_ >= unitsSize){
				unitIdx_ = 0;
				++referenceIdx_;
				if(referenceIdx_ >= unitReferences_.size())
					referenceIdx_ = 0;
				unitsSize = player->realUnits(unitReferences_[referenceIdx_]).size();
			}
			else
				break;
		}while(unitIdx_ != startUnitIdx && referenceIdx_ != startRefIdx);
		if(unitIdx_ < unitsSize)
			return player->realUnits(unitReferences_[referenceIdx_])[unitIdx_];
	}
	return 0;
}
