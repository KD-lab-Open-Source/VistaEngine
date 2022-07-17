#include "StdAfx.h"

#include "fps.h"
#include "runtime.h"
#include "..\ht\ht.h"
#include "GameShell.h"
#include "Squad.h"
#include "Universe.h"
#include "Inventory.h"
#include "SelectManager.h"
#include "RenderObjects.h"
#include "Terra.h"
#include "Triggers.h"
#include "IronBuilding.h"
#include "..\Units\UnitItemInventory.h"
#include "..\Units\UnitItemResource.h"
#include "..\\Network\\P2P_interface.h"
#include "..\Terra\QSWorldsMgr.h"
#include "PFtrap.h"
#include "Weapon.h"
#include "Timers.h"
#include "..\Water\Water.h"
#include "..\Environment\Environment.h"
#include "..\Render\src\postEffects.h"
#include "TextDB.h"

#include "Controls.h"
#include "UI_UnitView.h"
#include "UI_Render.h"
#include "UI_Actions.h"
#include "UI_Logic.h"
#include "UI_CustomControls.h"
#include "ShowHead.h"
#include "UserInterface.h"
#include "CommonLocText.h"
#include "UI_Controls.h"
#include "UI_NetCenter.h"

#include "CameraManager.h"
#include "UniverseX.h"

#include "UI_StreamVideo.h"
extern Singleton<UI_StreamVideo> streamVideo;

#include "StreamCommand.h"
#include <shellapi.h>

Singleton<UI_NetCenter> netCenter;

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
	environment->setSourceOnMouse(*source_ref_ptr);
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
	UI_TAG_SUPPLYSLOTS // сколько слотов занимает юнит (с раскраской)
};

struct CharStringLess {
	bool operator() (const char* left, const char* right) const {
		return strcmp(left, right) < 0;
	}
};

typedef StaticMap<const char*, enum UITextTag, CharStringLess> UI_TextTags;
UI_TextTags uiTextTags;

UI_LogicDispatcher::UI_LogicDispatcher() : mousePosition_(0,0), 
	mouseFlags_(0),
	activeCursor_(NULL),
	cursorVisible_(true),
	inputFlags_(0),
	cursorEffect_(NULL),
	cursorTriggered_(false),
	hoverUnit_(0),
	startTrakingUnit_(0),
	selectedUnit_(0),
	hoverControl_(0),
	tipsControl_(0),
	tipsUnit_(0),
	lastHotKeyInput_(0),
	buildingInstaller_()
{ 
	enableInput_ = true;
	gamePause_ = false;

	aimPosition_ = Vect3f::ZERO;
	aimDistance_ = 0.f;

	loadProgress_ = 0.f;
	loadSection_ = UI_LOADING_INITIAL;
	loadStep_ = 0;

	loadProgressSections_.reserve(2);

	loadProgressSections_.push_back(UI_LoadProgressSection(UI_LOADING_INITIAL));
	loadProgressSections_.back().setSize(0.2f);
	loadProgressSections_.back().setStepCount(2);

	loadProgressSections_.push_back(UI_LoadProgressSection(UI_LOADING_UNIVERSE));
	loadProgressSections_.back().setSize(0.8f);
	loadProgressSections_.back().setStepCount(8);

	selectedWeapon_ = 0;
	isAttackEnabled_ = false;

	cursorInWorld_ = false;
	hoverPosition_ = hoverTerrainPosition_ = Vect3f(0,0,0);
	hoverWater_ = false;
	hoverPassable_ = false;
	
	showAllUnitParametersMode_ = false;

	tipsControlDelay_ = 0.f;
	tipsUnitDelay_ = 0.f;

	clickMode_ = UI_CLICK_MODE_NONE;
	mousePressPos_ = Vect2f(0,0);
	trackMode_ = false;

	selectionTexture_ = 0;
	selectionCornerTexture_ = 0;
	selectionCenterTexture_ = NULL;

	showTips_ = true;
	showMessages_ = true;

	takenItemSet_ = false;

	currentClickAttackState_ = false;
	attackCoordTime_ = 0;

	currentTeemGametype_ = TEAM_GAME_TYPE_INDIVIDUAL;

	diskOpID_ = UI_DISK_OP_NONE;
	diskOpConfirmed_ = false;

	lastCurrentHiVer_ = UI_Dispatcher::instance().gameMajorVersion();
	lastCurrentLoVer_ = UI_Dispatcher::instance().gameMinorVersion();

	uiTextTags.insert(make_pair("display_name", UI_TAG_PLAYER_DISPLAY_NAME)); // в online - DW логин, иначе имя профиля
	uiTextTags.insert(make_pair("players_count", UI_TAG_PLAYERS_COUNT)); // количество оставшихся игроков
	uiTextTags.insert(make_pair("game_name", UI_TAG_CURRENT_GAME_NAME)); // название текущей загруженной карты
	uiTextTags.insert(make_pair("cm_name", UI_TAG_SELECTED_GAME_NAME)); // название выбранной в списке карты
	uiTextTags.insert(make_pair("multi_name", UI_TAG_SELECTED_MULTIPLAYER_GAME_NAME)); // название выбранной сетевой игры
	uiTextTags.insert(make_pair("cm_size", UI_TAG_SELECTED_GAME_SIZE)); // размер выбранной карты
	uiTextTags.insert(make_pair("cm_type", UI_TAG_SELECTED_GAME_TYPE)); // тип (custom, predefine) выбранной карты
	uiTextTags.insert(make_pair("cm_players", UI_TAG_SELECTED_GAME_PLAYERS)); // максимальное количество игроков
	uiTextTags.insert(make_pair("channel_name", UI_TAG_CURRENT_CHAT_CHANNEL)); //название текущего канала или статус подключения
	uiTextTags.insert(make_pair("hotkey", UI_TAG_HOTKEY)); // горячая клавиша для кнопки
	uiTextTags.insert(make_pair("accessible", UI_TAG_ACCESSIBLE)); // что надо для доступности юнита
	uiTextTags.insert(make_pair("accessible_param", UI_TAG_ACCESSIBLE_PARAM)); // что надо для доступности параметра
	uiTextTags.insert(make_pair("level", UI_TAG_LEVEL)); // уровень юнита
	uiTextTags.insert(make_pair("accountsize", UI_TAG_ACCOUNTSIZE)); // сколько слотов занимает юнит
	uiTextTags.insert(make_pair("supplyslots", UI_TAG_SUPPLYSLOTS)); // сколько слотов занимает юнит (с раскраской)
	uiTextTags.insert(make_pair("time_h12", UI_TAG_TIME_H12)); // время мира в 12 часовом формате
	uiTextTags.insert(make_pair("time_ampm", UI_TAG_TIME_AMPM)); // до полудня или после полудня
	uiTextTags.insert(make_pair("time_h24", UI_TAG_TIME_H24)); // вреям мира в 24 часовом формате
	uiTextTags.insert(make_pair("time_min", UI_TAG_TIME_M)); // минуты времени мира
}

UI_LogicDispatcher::~UI_LogicDispatcher()
{
	dassert(!cursorEffect_);
}

Rectf aspectedWorkArea(const Rectf& windowPosition, float aspect);

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
		takenItem_.redraw(Rectf(mousePosition(), Vect2f(0,0)), 1.f);

	if(trackMode_)
		drawSelection(mousePressPos_, mousePosition());
}

Vect3f G2S(const Vect3f &vg);

void UI_LogicDispatcher::drawDebug2D() const
{
	if(showDebugInterface.hoveredControlBorder){
		if(const UI_ControlBase* hovered_control = hoverControl_){
			UI_Render::instance().drawRectangle(hovered_control->transfPosition(), sColor4f(0.5f, 1.f, 0.5f, 0.5f), true);
			if(!hovered_control->mask().isEmpty())
				hovered_control->mask().draw(hovered_control->transfPosition().left_top(), sColor4f(0.5f, 1.f, 0.5f, 0.5f));

			UI_ControlReference ref(hovered_control);
			XBuffer buf;
			buf.SetDigits(2);
			buf < ref.referenceString();
			if(showDebugInterface.hoveredControlExtInfo){
				buf < '\n' < typeid(*hovered_control).name() < '\n'
				< (hovered_control->isEnabled() ? "enabled" : "DISABLED")
				< (hovered_control->isActivated() ? ", Active\n" : "\n")
				< "Current state: " <= hovered_control->currentStateIndex();
				if(hovered_control->currentStateIndex() >= 0)
					buf < "; Name: " < hovered_control->state(hovered_control->currentStateIndex())->name();
			}
			if(showDebugInterface.showTransformInfo){
				buf < "\n";
				hovered_control->getDebugString(buf);
			}
			
			UI_Render::instance().outDebugText(mousePosition_, buf.c_str(), &sColor4c(120, 255, 120));
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
		out < "Cursor reason: " < debugCursorReason_.c_str() < "\n";
	}

	if(showDebugInterface.logicDispatcher){
		out < "nameToSaveGame: " < saveGameName_.c_str()
			< ", nameToSaveReplay: " < saveReplayName_.c_str()
			< ", nameToCreateProfile: " < profileName_.c_str()
			< "\nCurrentMission: ";
		selectedMission_.getDebugInfo(out);
	}

	if(out.tell())
		UI_Render::instance().outDebugText(Vect2f(.02f, .25f), out.c_str());

}

void UI_LogicDispatcher::showDebugInfo() const
{
	if(showDebugInterface.marks){
		if(const WeaponPrm* wpn = selectedWeapon_){
			if(!takenItemSet_ && clickMode_ == UI_CLICK_MODE_ATTACK && isAttackEnabled_)
				cursorMark_.showDebugInfo();
		}

		for(MarkObjects::const_iterator it = markObjects_.begin(); it != markObjects_.end(); ++it)
			it->showDebugInfo();
	}

	if(showDebugInterface.showAimPosition){
		show_vector(aimPosition_, 5.f, GREEN);
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
				extern void clip_circle_3D(const Vect3f& vc, float radius, sColor4c color);

				sBox6f box;
				model->GetBoundBox(box);
				float radius = box.max.distance(box.min) * 0.5f;
				Vect3f center(model->GetPosition() * ((box.max + box.min) * 0.5f));
				
				float t = 0.95f * radius;
				for(float z_add = -t; z_add <= t; z_add += t / 6.f){
					float rad = sqrtf(sqr(radius) - sqr(z_add));
					clip_circle_3D(Vect3f(center.x, center.y, center.z + z_add), rad, sColor4c(255, 0, 0));
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
}

void UI_LogicDispatcher::logicQuant(float dt)
{
	start_timer_auto();

	xassert(selectManager && gameShell->GameActive);
	setSelectedUnit(selectManager->selectedUnit());
	xassert(pathFinder);
	hoverPassable_ = !pathFinder->checkFlag(PathFinder::IMPASSABILITY_FLAG, hoverTerrainPosition_.xi(), hoverTerrainPosition_.yi());

	if(UnitInterface* p = selectedUnit()){
		UI_UnitView::instance().setAttribute(&p->getUnitReal()->attr());

		if(const InventorySet* ip = p->inventory()){
			ip->updateUI();

			uiStreamCommand.set(fCommandSetTakenItem);
			if(ip->takenItem()())
				uiStreamCommand << true << UI_InventoryItem(ip->takenItem());
			else
				uiStreamCommand << false;
		}
		else 
			uiStreamCommand.set(fCommandSetTakenItem) << false;
	}
	else {
		UI_UnitView::instance().setAttribute(0);
		uiStreamCommand.set(fCommandSetTakenItem) << false;
	}

	xassert(chatMessages_.size() == chatMessageTimes_.size());
	ChatMessageTimes::iterator tit = chatMessageTimes_.begin();
	ComboStrings::iterator sit = chatMessages_.begin();
	while(tit != chatMessageTimes_.end())
		if((*tit)()){
			tit = chatMessageTimes_.erase(tit);
			sit = chatMessages_.erase(sit);
		}
		else {
			++tit;
			++sit;
		}

	UI_UnitView::instance().logicQuant(dt);

	UI_BackgroundScene::instance().logicQuant(dt);
}

void fSendHoverMouseCommand(void* data)
{
	universe()->activePlayer()->sendCommand(UnitCommand(COMMAND_ID_DIRECT_SHOOT_MOUSE,  UI_LogicDispatcher::instance().hoverPosition()));
//	universe()->activePlayer()->sendCommand(UnitCommand(COMMAND_ID_DIRECT_SHOOT_MOUSE,  UI_LogicDispatcher::instance().aimPosition()));
}

void UI_LogicDispatcher::directShootQuant()
{
	if(currentClickAttackState_)
		if(attackCoordTime_++ == universe()->directShootInterpotatePeriod())
		{
			attackCoordTime_ = 0;
			uiStreamCommand.set(fSendHoverMouseCommand);
		}
}

void UI_LogicDispatcher::reset()
{
	buildingInstaller_.Clear();

	UI_UnitView::instance().setAttribute(0);
	showHead().resetHead();

	hoverUnit_ = 0;
	hoverPosition_ = hoverTerrainPosition_ = Vect3f(0,0,0);
	startTrakingUnit_ = 0;

	hoverControl_ = 0;

	RELEASE(cursorEffect_);
	activeCursor_ = 0;

	hoverInfos_.clear();

	cursorMark_.clear();
	showCursor();

	selectedWeapon_ = 0;
	selectedUnit_ = 0;

	for(MarkObjects::iterator it = markObjects_.begin(); it != markObjects_.end(); ++it)
		it->clear();

	markObjects_.clear();
	
	clearGameChat();
}

void UI_LogicDispatcher::graphQuant(float dt)
{
	start_timer_auto();

	Vect2f mouse_pos = UI_Render::instance().relative2deviceCoords(mousePosition());

	if(Player* pl = player()){
		if(gameShell->directControl() && (pl->isWin() || pl->isDefeat()))
			disableDirectControl();
	}

	UnitInterface* unitNear = 0;
	if(!hoverControl_)
	{
		Vect3f v0, v1;
		cameraManager->calcRayIntersection(mouse_pos, v0, v1);

		float distMin = v1.distance2(v0);
		unitNear = gameShell->unitHover(v0, v1, distMin);
		
		if(unitNear){
			Vect3f dirPoint = v1 - v0;
			dirPoint.Normalize(clamp(sqrtf(distMin) - 15.f, 1.f, 10000.f));
			if(!weapon_helpers::traceGround(v0, v0+dirPoint, dirPoint))
				unitNear = 0; // земля ближе
		}
	}
	hoverUnit_ = unitNear;

	isAttackEnabled_ = selectManager->canAttack(attackTarget(), gameShell->underFullDirectControl(), true);

	buildingInstaller_.quant(universe()->activePlayer(), cameraManager->GetCamera());
	
	tipsControlDelay_ = max(0.f, tipsControlDelay_ - dt);
	tipsUnitDelay_ = max(0.f, tipsUnitDelay_ - dt);

	if(!gameShell->cameraMouseTrack){
		
		Vect3f hoverPos;
		cursorInWorld_ = cameraManager->cursorTrace(mouse_pos, hoverPos);
		hoverPosition_ = hoverPos;

		hoverTerrainPosition_ = hoverPosition_;
		hoverPosition_.z += 0.05f;

		hoverWater_ = (environment->water() && environment->water()->isFullWater(hoverPosition_));
		if(hoverWater_)
			hoverPosition_.z = environment->water()->GetZ(hoverPosition_.xi(), hoverPosition_.yi()) - 0.5f;
/*
		if(!cursorInWorld_){
			Vect3f pos, dir;
			cameraManager->GetCamera()->GetWorldRay(mouse_pos,pos,dir);
			aimPosition_ = pos + dir * 300.f;
		}
		else
			aimPosition_ = hoverPosition_;
*/
/*		aimPosition_ = hoverPosition_;
		if(gameShell->directControl_){
			if(UnitInterface* p = selectManager->selectedUnit()){
				Vect3f pos = p->position();
				pos.z += .7f * p->height();
				aimPosition_ = pos + 300.f * cameraManager->directControlShootingDirection();
			}
		}*/
	}
	
	for(MarkObjects::iterator it1 = markObjects_.begin(); it1 != markObjects_.end();)
		if(it1->quant(dt))
			++it1;
		else {
			it1->clear();
			it1 = markObjects_.erase(it1);
		}

	const WeaponPrm* wpn = selectedWeapon_;

	if(takenItemSet_){
		hideCursor();
	}
	else if(clickMode_ == UI_CLICK_MODE_ATTACK && wpn && isAttackEnabled_){
		cursorMark_.update(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, 
			Se3f(QuatF::ID, cursorPosition()), &wpn->targetMark(), selectManager->selectedUnit(), hoverUnit_));

		cursorMark_.quant(dt);

		if(wpn->hideCursor())
			hideCursor();
	}
	else {
		cursorMark_.clear();
		showCursor();
	}

	if(trackMode_){
		if(selectManager->selectArea(
			UI_Render::instance().relative2deviceCoords(mousePressPos_),
			UI_Render::instance().relative2deviceCoords(mousePosition()),
			isShiftPressed(), startTrakingUnit_)){
				if(hoverUnit_)
					selectManager->selectUnit(hoverUnit_, true, true);
			}

		if(!ControlManager::instance().key(CTRL_SELECT).keyPressed(KBD_SHIFT))
			toggleTracking(false);
	}

	if(Player* pl = player()){
		if(pl->shootFailed()){
			if(clickMode_ == UI_CLICK_MODE_ATTACK)
				selectClickMode(UI_CLICK_MODE_NONE);
			pl->setShootFailed(false);
		}
	}

	if(diskOpID_ != UI_DISK_OP_NONE){
		if(checkDiskOp(diskOpID_, diskOpPath_.c_str()))
			makeDiskOp(diskOpID_, diskOpPath_.c_str(), diskOpGameType_);
	}
}

bool UI_LogicDispatcher::handleInput(const UI_InputEvent& event)
{
	if(!universe())
		return false;
		
	switch(event.ID()){
		case UI_INPUT_MOUSE_MOVE:
			if(buildingInstaller_.inited())
				if(isMouseFlagSet(MK_CONTROL))
					buildingInstaller_.SetBuildPosition(buildingInstaller_.position(), buildingInstaller_.angle() - gameShell->mousePositionDelta().x*35);
				else
					buildingInstaller_.SetBuildPosition(hoverPosition_, buildingInstaller_.angle());
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

	// для мышино-кнопочных действий, shift независимо от настроек является флагом постановки в очередь (добавления к селекту)
	int withoutShiftCode = event.keyCode() & ~KBD_SHIFT;
	bool shiftPressed = event.keyCode() & KBD_SHIFT;

	if((ControlManager::instance().key(CTRL_CLICK_ACTION).fullkey & ~KBD_SHIFT) == withoutShiftCode){
		if(clickAction(shiftPressed))
			return false;
	}
	else if(event.isMouseClickEvent()){
		if(clickMode_ != UI_CLICK_MODE_NONE){
			selectClickMode(UI_CLICK_MODE_NONE);
			return false;
		}
	}

	switch(event.ID()){
	case UI_INPUT_MOUSE_LBUTTON_DOWN:
		if(!isMouseFlagSet(MK_RBUTTON))
			if(UnitInterface* p = selectManager->getUnitIfOne())
				if(const InventorySet* ip = p->inventory())
					if(ip->takenItem()())
						p->sendCommand(UnitCommand(COMMAND_ID_ITEM_TAKEN_DROP, hoverPosition_));
		break;
	
	case UI_INPUT_MOUSE_RBUTTON_DOWN:
		if(UnitInterface* unit = selectManager->getUnitIfOne())
			if(const InventorySet* ip = unit->inventory())
				if(ip->takenItem()())
					unit->sendCommand(UnitCommand(COMMAND_ID_ITEM_RETURN, -1));
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
		if(gameShell->underFullDirectControl())
			selectManager->makeCommand(UnitCommand(COMMAND_ID_DIRECT_CHANGE_WEAPON, 1), true);
		break;

	case UI_INPUT_MOUSE_WHEEL_DOWN:
		if(gameShell->underFullDirectControl())
			selectManager->makeCommand(UnitCommand(COMMAND_ID_DIRECT_CHANGE_WEAPON, -1), true);
		break;
	}

	if(!gameShell->directControl())
	{
		if((ControlManager::instance().key(CTRL_SELECT).fullkey & ~KBD_SHIFT) == withoutShiftCode){
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
				selectManager->selectAllOnScreen(&hoverUnit_->attr());
			}
		}
		
		if((ControlManager::instance().key(CTRL_ATTACK).fullkey & ~KBD_SHIFT) == withoutShiftCode){
			if(hoverUnit_){
				UnitCommand unitCommand(COMMAND_ID_OBJECT, hoverUnit_, hoverPosition_, 0);
				unitCommand.setShiftModifier(shiftPressed);
				selectManager->makeCommand(unitCommand);
				if(hoverUnit_->player() != universe()->activePlayer() && hoverUnit_->player() != universe()->worldPlayer() && hoverUnit_->player()->clan() != universe()->activePlayer()->clan())
					addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET_UNIT, Se3f(QuatF::ID, cursorPosition()), &universe()->activePlayer()->race()->orderMark(UI_CLICK_MARK_ATTACK_UNIT), selectManager->selectedUnit(), hoverUnit_));
			}
		}
		
		if((ControlManager::instance().key(CTRL_MOVE).fullkey & ~KBD_SHIFT) == withoutShiftCode){
			if(hoverUnit_){
				UnitCommand unitCommand(COMMAND_ID_OBJECT, hoverUnit_, hoverPosition_, 0);
				unitCommand.setShiftModifier(shiftPressed);
				selectManager->makeCommand(unitCommand);
				if(!(hoverUnit_->player() != universe()->activePlayer() && hoverUnit_->player() != universe()->worldPlayer() && hoverUnit_->player()->clan() != universe()->activePlayer()->clan()))
					addMovementMark();
			}
			else if(!selectManager->isSelectionEmpty()){
				selectManager->makeCommandSubtle(COMMAND_ID_POINT, cursorPosition(), shiftPressed);
				addMovementMark();
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
		if(event.flags() != -1)
			mouseFlags_ = event.flags();
	}

	if(universe() && universe()->activePlayer()){
		bool shootPressed = false;

		if(!gameShell->underFullDirectControl())
			shootPressed = !gameShell->isPaused(GameShell::PAUSE_BY_ANY) && ControlManager::instance().key(CTRL_CLICK_ACTION).keyPressed(KBD_SHIFT);
		else
			shootPressed = !gameShell->isPaused(GameShell::PAUSE_BY_ANY) && isMouseFlagSet(MK_LBUTTON | MK_RBUTTON);

		if(currentClickAttackState_ != shootPressed){
			currentClickAttackState_ = !currentClickAttackState_;
			universe()->activePlayer()->sendCommand(UnitCommand(COMMAND_ID_DIRECT_SHOOT, UI_LogicDispatcher::instance().hoverPosition(), (int)currentClickAttackState_));
			attackCoordTime_ = 0;
		}
	}
}

void UI_LogicDispatcher::updateAimPosition()
{
	Vect2f mouse_pos = UI_Render::instance().relative2deviceCoords(mousePosition());

	Vect3f hoverPos;
	if(gameShell->underFullDirectControl()){
		if(UnitInterface* p = selectedUnit()){
			const UnitReal* rp = p->getUnitReal();
			Vect3f pos = rp->position();
			pos.z += .7f * rp->height();
			cursorInWorld_ = cameraManager->cursorTrace(mouse_pos, pos, hoverPos);
		}
		else
			cursorInWorld_ = cameraManager->cursorTrace(mouse_pos, hoverPos);
	}
	else
		cursorInWorld_ = cameraManager->cursorTrace(mouse_pos, hoverPos);

	hoverPosition_ = hoverPos;

	hoverTerrainPosition_ = hoverPosition_;
	hoverPosition_.z += 0.05f;

	hoverWater_ = (environment->water() && environment->water()->isFullWater(hoverPosition_));
	if(hoverWater_)
		hoverPosition_.z = environment->water()->GetZ(hoverPosition_.xi(), hoverPosition_.yi()) - 0.5f;

	if(!cursorInWorld_){
//		Vect2f mouse_pos = UI_Render::instance().relative2deviceCoords(mousePosition());
		Vect3f pos, dir;
		cameraManager->GetCamera()->GetWorldRay(mouse_pos,pos,dir);
		aimPosition_ = pos + dir * UI_DIRECT_CONTROL_CURSOR_DIST;
	}
	else
		aimPosition_ = hoverPosition_;

	aimDistance_ = 0.f;
	if(gameShell->underFullDirectControl()){
		if(UnitInterface* p = selectedUnit()){
			const UnitReal* rp = p->getUnitReal();
			Vect3f pos = rp->position();
			pos.z += .7f * rp->height();
/*
			if(const UnitInterface* target = hoverUnit()){
				if(target->rigidBody()){
					Vect3f pos1;
					int bodyPart(target->rigidBody()->computeBoxSectionPenetration(pos1, pos, aimPosition_));
					if(bodyPart >= 0)
						aimPosition_ = pos1;
				}
			}
*/
			aimDistance_ = pos.distance(aimPosition_);
		}
	}

/*
	aimPosition_ = hoverPosition_;
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

bool UI_LogicDispatcher::addLight(int light_index, const Vect3f& position) const
{
	MTL();

	if(cCamera* camera = cameraManager->GetCamera()){
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
	UI_LogicDispatcher::instance().addMark(*inf, 0.19f);
}

void UI_LogicDispatcher::addMark(const UI_MarkObjectInfo& inf, float time)
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
		mi->restartTimer(time);
		return;
	}

	uiStreamCommand.set(fCommandAddMark) << inf;
}

void UI_LogicDispatcher::addMovementMark()
{
	if(const Player* pl = player())
		addMark(UI_MarkObjectInfo(UI_MARK_MOVE_POINT, Se3f(QuatF::ID, hoverPosition_), &pl->race()->orderMark(hoverWater() ? UI_CLICK_MARK_MOVEMENT_WATER : UI_CLICK_MARK_MOVEMENT), selectManager->selectedUnit()));
}

void UI_LogicDispatcher::selectClickMode(UI_ClickModeID mode_id, const WeaponPrm* selected_weapon)
{
	MTG();
	
	if(mode_id != UI_CLICK_MODE_NONE && selected_weapon){
		// тогда выбираем только, если это оружие у юнита есть
		if(UnitInterface* unit = selectManager->selectedUnit())
			if(!safe_cast<UnitActing*>(unit->getUnitReal())->hasWeapon(selected_weapon->ID()))
				return;
	}

	if(mode_id == UI_CLICK_MODE_PATROL && clickMode_ != mode_id)
		selectManager->makeCommand(UnitCommand(COMMAND_ID_STOP));

	clickMode_ = mode_id;
	selectedWeapon_ = selected_weapon;
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

bool UI_LogicDispatcher::inputEventProcessed(const UI_InputEvent& event, UI_ControlBase* control)
{
	MTG();
	
	if(event.isMouseClickEvent()){
		uiStreamGraph2LogicCommand.set(fCommandSendEventButton) << Event::UI_BUTTON_CLICK << playerID() << control << control->actionFlags() << modifiers() << control->isEnabled();
	}

	if(event.isMouseClickEvent())
		selectClickMode(UI_CLICK_MODE_NONE);

	return true;
}

void UI_LogicDispatcher::focusControlProcess(const UI_ControlBase* lastHovered)
{
	if(hoverControl() != lastHovered){
		if(lastHovered)
			uiStreamGraph2LogicCommand.set(fCommandSendEventButton) << Event::UI_BUTTON_FOCUS_OFF << playerID() << lastHovered;
		uiStreamGraph2LogicCommand.set(fCommandSendEventButton) << Event::UI_BUTTON_FOCUS_ON << playerID() << hoverControl();
	}
}

void UI_LogicDispatcher::toggleBuildingInstaller(const AttributeBase* attr)
{
	MTG();
	if(attr && UI_Dispatcher::instance().isEnabled()){
		buildingInstaller_.InitObject(safe_cast<const AttributeBuilding*>(attr), true);
		buildingInstaller_.SetBuildPosition(hoverPosition_, buildingInstaller_.angle());
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
			cursorEffect_->SetCycled(true);
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

const MissionDescription* UI_LogicDispatcher::getMissionByName(const char* name, const UI_ActionDataSaveGameList& data)
{
	if(!name)
		return 0;
	return getMissionsList(data).find(name);
}

const MissionDescription* UI_LogicDispatcher::getMissionByName(const char* name) const
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

void UI_LogicDispatcher::buildRaceList(UI_ControlBase* control, int selected, bool withAny)
{
	const RaceTable::Strings& map = RaceTable::instance().strings();

	ComboStrings strings;
	if(withAny)
		strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_ANY_RACE));
	for(int idx = 0; idx < map.size(); ++idx)
		if(!map[idx].instrumentary())
			strings.push_back(map[idx].name());

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

const char* UI_LogicDispatcher::currentPlayerDisplayName() const
{
	if(gameShell->isNetClientConfigured(PNCWM_ONLINE_DW))
		return UI_LogicDispatcher::instance().currentProfile().lastInetName.c_str();
	else
		return UI_LogicDispatcher::instance().currentProfile().name.c_str();
}

void UI_LogicDispatcher::setCurrentMission(const MissionDescription& descr, bool inherit)
{
	if(descr.gameType() & GAME_TYPE_REEL){
		selectedMission_ = descr;
	}
	else if(inherit && currentMission()){
		MissionDescription old = selectedMission_;
		selectedMission_ = descr;
		if(MissionDescription* current = currentMission()){
			current->setUseMapSettings(old.useMapSettings());
			if(!current->isMultiPlayer())
				current->setPlayerName(current->activePlayerID(), current->activeCooperativeIndex(), currentPlayerDisplayName());
			else if(old.isMultiPlayer())
				current->setGameType(old.gameType());
		}
	}
	else {
		selectedMission_ = descr;
		if(currentMission() && !currentMission()->isMultiPlayer())
			currentMission()->setPlayerName(currentMission()->activePlayerID(), currentMission()->activeCooperativeIndex(), currentPlayerDisplayName());
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
	return netCenter();
}

void UI_LogicDispatcher::handleChatString(const class ChatMessage& _chatMsg)
{
	MTL();
	int rawIntData=_chatMsg.id;
	xassert(rawIntData < 0 || netCenter().gameCreated());

	if(rawIntData >= 0 && rawIntData != gameShell->CurrentMission.playerData(gameShell->CurrentMission.activePlayerID()).clan)
		return;

	string message;
	if(rawIntData >= 0)
		message = UI_Dispatcher::instance().privateMessageColor();

	message += _chatMsg.getText();//stringFromNet;

	if(netCenter().gameCreated()){
		chatMessages_.push_back(message);
		chatMessageTimes_.push_back(DelayTimer());
		chatMessageTimes_.back().start(UI_GlobalAttributes::instance().chatDelay() * 1000);
	}
	netCenter().handleChatString(message.c_str());
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

	bool su = (GlobalAttributes::instance().serverCanChangeClientOptions ? netCenter().isServer() : false);

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
			else if(netCenter().isNetGame()){
				if(netCenter().isServer())
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
		if(netCenter().isNetGame()
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

		su = su || (netCenter().isServer() && playerType == REAL_PLAYER_TYPE_AI);

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
				if(!netCenter().isNetGame() || netCenter().isServer())
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
			else if(netCenter().isNetGame()){
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
			if(netCenter().isNetGame())
				if(su || data->playerIndex() == mission->activePlayerID())
					state.enable();
				else
					state.disable();
			else
				state.enable();
				
			break;

		case TUNE_FLAG_READY:
			state.disable();
			if(netCenter().isNetGame() 
			&& playerType == REAL_PLAYER_TYPE_PLAYER
			&& mission->playerData(data->playerIndex()).usersIdxArr[data->teamIndex()] != USER_IDX_NONE)
				state.show();
			else
				state.hide();
			break;

		case TUNE_BUTTON_KICK:
			state.enable();
			if(netCenter().isServer() 
			&& playerType == REAL_PLAYER_TYPE_PLAYER
			&& data->playerIndex() != mission->activePlayerID()
			&& mission->playerData(data->playerIndex()).usersIdxArr[data->teamIndex()] != USER_IDX_NONE)
				state.show();
			else
				state.hide();
			break;

		case TUNE_FLAG_VARIABLE:
			state.show();
			if(netCenter().isNetGame()){
				if(netCenter().isServer())
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

void UI_LogicDispatcher::saveGame(const string& gameName)
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

	if(checkDiskOp(UI_DISK_OP_SAVE_GAME, path.c_str()))
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

void SpriteToBuf(cQuadBuffer<sVertexXYZWDT1>* buf, const sColor4c& color, int x1, int y1, int x2, int y2,
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

	sColor4c color = selectionPrm_.selectionBorderColor_;
	cCamera* cam = cameraManager->GetCamera();
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

bool UI_LogicDispatcher::clickAction(bool shiftPressed)
{
	if(gameShell->underFullDirectControl())
		return true;

	bool ret = false;
	Vect3f v;

	switch(clickMode_)	{
	case UI_CLICK_MODE_MOVE:
		selectManager->makeCommandSubtle(COMMAND_ID_POINT, hoverPosition_);
		addMovementMark();
		
		selectClickMode(UI_CLICK_MODE_NONE);
		ret = true;
		break;
	case UI_CLICK_MODE_ATTACK: {
		bool clearClickMode = false;
		if(!selectManager->isSelectionEmpty()){
			WeaponTarget target = attackTarget();
			if(selectManager->canAttack(target)){
				selectManager->makeCommandAttack(target, shiftPressed);

				if(const Player* pl = player()){
					if(hoverUnit_)
						addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET_UNIT, Se3f(QuatF::ID, cursorPosition()), &pl->race()->orderMark(UI_CLICK_MARK_ATTACK_UNIT), selectManager->selectedUnit(), hoverUnit_));
					else
						addMark(UI_MarkObjectInfo(UI_MARK_ATTACK_TARGET, Se3f(QuatF::ID, cursorPosition()), &pl->race()->orderMark(UI_CLICK_MARK_ATTACK), selectManager->selectedUnit(), hoverUnit_));
				}
			}
			else if(hoverUnit_ && hoverUnit_->player() == universe()->activePlayer()){
				selectManager->selectUnit(hoverUnit_, false);
				clearClickMode = true;
			}
		}
	
		if(!shiftPressed && (!selectedWeapon_ || selectedWeapon_->clearAttackClickMode() || clearClickMode))
			selectClickMode(UI_CLICK_MODE_NONE);

		ret = true;
		break;
							   }
	case UI_CLICK_MODE_PATROL:
		if(selectManager->isSelectionEmpty())
			selectClickMode(UI_CLICK_MODE_NONE);
		else {
			selectManager->makeCommandSubtle(COMMAND_ID_PATROL, hoverPosition_);
			addMovementMark();

//			selectClickMode(UI_CLICK_MODE_NONE);
			ret = true;
		}
		break;
	case UI_CLICK_MODE_REPAIR:
		if(universe()){
			Player* player = universe()->activePlayer();
			if(player && hoverUnit_ && (hoverUnit_->player()->isWorld() || hoverUnit_->player() == player)){
				selectManager->makeCommand(UnitCommand(COMMAND_ID_OBJECT, hoverUnit_));

				addMark(UI_MarkObjectInfo(UI_MARK_REPAIR_TARGET, Se3f(QuatF::ID, hoverPosition_), &player->race()->orderMark(UI_CLICK_MARK_REPAIR), selectManager->selectedUnit()));
			}

			selectClickMode(UI_CLICK_MODE_NONE);
			ret = true;
		}
		break;
	case UI_CLICK_MODE_RESOURCE:
		if(universe()){
			Player* player = universe()->activePlayer();
			if(player && hoverUnit_ && (hoverUnit_->attr().isResourceItem() || hoverUnit_->attr().isInventoryItem())){
				selectManager->makeCommand(UnitCommand(COMMAND_ID_OBJECT, hoverUnit_));

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
			netCenter().reset();
			netCenter().updateFilter();
			break;

		case UI_ACTION_CHAT_MESSAGE_BOARD:
			netCenter().clearChatBoard();
			break;
		
		case UI_ACTION_GAME_CHAT_BOARD:
			clearGameChat();
			break;
		
		case UI_ACTION_CHAT_EDIT_STRING:
			control->setText(0);
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText("");
			break;

		case UI_ACTION_INET_STATISTIC_SHOW:
			netCenter().queryGlobalStatistic(true);
			break;

		case UI_ACTION_PROFILES_LIST:{
			ComboStrings strings;
			for(Profiles::const_iterator it = profileSystem().profilesVector().begin(); it != profileSystem().profilesVector().end(); ++it)
				strings.push_back(it->name);
			UI_ControlComboList::setList(control, strings, profileSystem().currentProfileIndex());
			break;
									 }
		case UI_ACTION_ONLINE_LOGIN_LIST:
			if(UI_ControlStringList* lst = dynamic_cast<UI_ControlStringList*>(control)){
				lst->setList(currentProfile().onlineLogins);
			}
			break;

		case UI_ACTION_SAVE_GAME_NAME_INPUT:
			control->setText(currentProfile().lastSaveGameName.c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(currentProfile().lastSaveGameName.c_str());
			saveGameName_ = control->text();
			break;

		case UI_ACTION_REPLAY_NAME_INPUT:
			control->setText(currentProfile().lastSaveReplayName.c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(currentProfile().lastSaveReplayName.c_str());
			saveReplayName_ = control->text();
			break;
		
		case UI_ACTION_LAN_PLAYER_CLAN:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				ComboStrings strings;
				for(int i = 1; i <= 6; ++i){
					XBuffer buf;
					buf <= i;
					strings.push_back((const char*)buf);
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
			control->setText(currentProfile().lastCreateGameName.c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(currentProfile().lastCreateGameName.c_str());
			break;

		case UI_ACTION_INET_NAME:
			control->setText(currentProfile().lastInetName.c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(currentProfile().lastInetName.c_str());
			break;

		case UI_ACTION_INET_PASS:
			netCenter().resetPasswords();
			control->setText("");
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText("");
			break;

		case UI_ACTION_INET_PASS2:
			control->setText("");
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText("");
			break;
		
		case UI_ACTION_CDKEY_INPUT:
			control->setText(currentProfile().cdKey.c_str());
			if(UI_ControlEdit* edit = dynamic_cast<UI_ControlEdit*>(control))
				edit->setEditText(currentProfile().cdKey.c_str());
			break;

		case UI_ACTION_LAN_GAME_TYPE:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				ComboStrings strings;
				strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_INDIVIDUAL_LAN_GAME));
				strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_TEEM_LAN_GAME));
				lst->setList(strings);
			}
			break;

		case UI_ACTION_INET_FILTER_PLAYERS_COUNT:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				ComboStrings strings;
				strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_ANY_UNITS_SIZE));
				XBuffer buf;
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
			ComboStrings strings;
			strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_ANY_GAME));
			strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_CUSTOM_GAME));
			strings.push_back(GET_LOC_STR(UI_COMMON_TEXT_PREDEFINE_GAME));
			UI_ControlComboList::setList(control, strings, currentProfile().gameTypeFilter < 0 ? 0 : currentProfile().gameTypeFilter + 1);
			break;
											 }
		case UI_ACTION_QUICK_START_FILTER_POPULATION:
		case UI_ACTION_STATISTIC_FILTER_POPULATION:{
			ComboStrings strings;
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

				ComboStrings strings;
				lst->setSelectedString(-1);

				if(UI_ControlStringCheckedList* chekedList = dynamic_cast<UI_ControlStringCheckedList*>(lst)){

					for(MissionDescriptions::const_iterator it = missions_.begin(); it != missions_.end(); ++it)
						if(it->isBattle() && (id == UI_ACTION_MISSION_SELECT_FILTER || qsWorldsMgr.isMissionPresent(it->missionGUID())))
							strings.push_back(it->interfaceName());

					lst->setList(strings);

					if(filter.filterDisabled)
						chekedList->setSelect(strings);
					else {
						ComboStrings filterList;
						GUIDcontainer::const_iterator it;
						FOR_EACH(filter.filterList, it)
							if(const MissionDescription* mission = missions_.find(*it))
								filterList.push_back(mission->interfaceName());
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
							strings.push_back(it->interfaceName());
						}

						lst->setList(strings);
				}

				if(UI_ControlComboList* comboList = dynamic_cast<UI_ControlComboList*>(control))
					comboList->setText(lst->selectedString());
			}
			break;

		case UI_ACTION_QUICK_START_FILTER_RACE:
			buildRaceList(control, clamp(currentProfile().quickStartFilterRace + 1, 0, 3), true);
			break;

		case UI_ACTION_STATISTIC_FILTER_RACE:
			buildRaceList(control, clamp(currentProfile().statisticFilterRace, 0, 2));
			break;
		
		case UI_ACTION_OPTION_PRESET_LIST:{
			int preset = GameOptions::instance().getCurrentPreset();
			ComboStrings strings;
			splitComboList(strings, GameOptions::instance().getPresetList());
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
				ComboStrings strings;
				splitComboList(strings, GameOptions::instance().getList(action->Option()));
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
	}
}

void UI_LogicDispatcher::controlUpdate(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data, ControlState& controlState)
{
	switch(id){
		case UI_ACTION_PLAYER_PARAMETER:
			if(const UI_ActionDataPlayerParameter* p = safe_cast<const UI_ActionDataPlayerParameter*>(data)){
				if(universe() && universe()->activePlayer()){
					int val = round(universe()->activePlayer()->resource().findByIndex(p->parameter().key(), -1.f));
					if(val >= 0){
						XBuffer buf;
						buf <= val;
						control->setText(buf);
					}
				}
			}
			break;

		case UI_ACTION_UNIT_PARAMETER:
			if(const UI_ActionDataUnitParameter* p = safe_cast<const UI_ActionDataUnitParameter*>(data)){
				const AttributeBase* attr = p->attribute();
				const UnitReal* unit = 0;
				if(attr){
					if(universe() && universe()->activePlayer()){
						const RealUnits& units = universe()->activePlayer()->realUnits(attr);
						if(!units.empty())
							unit = units.front();
					}
				}
				else{
					if(p->useUnitList()){ // раскручиваем владельцев вверх до списка юнитов
						const UI_ControlBase* current = control;
						const UI_ControlContainer* parent = current->owner();
						while(parent){
							if(const UI_ControlUnitList* ul = dynamic_cast<const UI_ControlUnitList*>(parent)){
								xassert(ul->GetType() == UI_UNITLIST_SELECTED);
								dassert(std::find(ul->controlList().begin(), ul->controlList().end(), current) != ul->controlList().end());
								// вычисляем номер контрола в списке
								int child_index = std::distance(ul->controlList().begin(), std::find(ul->controlList().begin(), ul->controlList().end(), current));
								if(selectManager->selectedSlot() == child_index)
									control->setPermanentTransform(ul->activeTransform());
								else
									control->setPermanentTransform(UI_Transform::ID);
								if(UnitInterface* ui = selectManager->getUnitIfOne(child_index))
									unit = ui->getUnitReal();

								break;
							}
							else
								current = safe_cast<const UI_ControlBase*>(parent);
							parent = current->owner();
							xassert(parent && "<<список юнитов>> во владельцах не найден");
						}
					}
					else if(UnitInterface* ui = selectedUnit())
						unit = ui->getUnitReal();
				}
				if(unit){
					float current = -1.f, max = 1.f;
					UI_ControlProgressBar* bar = dynamic_cast<UI_ControlProgressBar*>(control);
					if(p->type() == UI_ActionDataUnitParameter::LOGIC){
						current = unit->parameters().findByIndex(p->parameterIndex(), -1.f);
						if(!(current < 0.f))
							max = unit->parametersMax().findByIndex(p->parameterIndex(), -1.f);
					
					}
					else if(p->type() == UI_ActionDataUnitParameter::LEVEL){
						if(unit->attr().isLegionary())
							if(bar)
								current = safe_cast<const UnitLegionary*>(unit)->levelProgress();
							else
								current = safe_cast<const UnitLegionary*>(unit)->level() + 1;
					}

					if(!(current < 0.f))
						if(bar){
							if(max > FLT_EPS)
								bar->setProgress(clamp(current / max, 0.0f, 1.0f));
							else
								bar->setProgress(0.f);
						}
						else{
							XBuffer buf;
							buf <= round(current);
							control->setText(buf);
						}
				}
				else
					if(UI_ControlProgressBar* bar = dynamic_cast<UI_ControlProgressBar*>(control))
						bar->setProgress(0);
					else
						control->setText("0");
			}
			break;

		case UI_ACTION_BIND_TO_IDLE_UNITS:
			if(int cnt = idleUnitsManager.idleUnitsCount(safe_cast<const UI_ActionDataIdleUnits*>(data)->type())){
				controlState.show();
				XBuffer buf;
				buf <= cnt;
				control->setText(buf);
			}
			else
				controlState.hide();
			break;
		
		case UI_ACTION_MINIMAP_ROTATION:
			control->setState(minimap().isRotateByCamera() ? 0 : 1);
			break;

		case UI_ACTION_SHOW_TIME:{
			float seconds = global_time()/1000;
			int h = seconds / 3600;
			seconds -= h * 3600;
			int m = seconds / 60;
			int s = seconds - m * 60;
			char buf[128];
			sprintf(buf, "%02u:%02u:%02u", h, m, s);
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
				char buf[128];
				sprintf(buf, "%02u:%02u", m, s);
				control->setText(buf);
			}
			else
				controlState.hide();
			break;
											 }
		case UI_ACTION_INET_NAT_TYPE:
			control->setText(netCenter().natType());
			break;
		case UI_ACTION_LAN_GAME_LIST:{
			const UI_ActionDataHostList* dat = safe_cast<const UI_ActionDataHostList*>(data);
			netCenter().updateGameList();
			ComboStrings  strings;
			int selected = netCenter().getGameList(dat->format(), strings, dat->startedGameColor());
			UI_ControlComboList::setList(control, strings, selected);
			break;
									 }
		case UI_ACTION_INET_STATISTIC_SHOW:{
			ComboStrings  strings;
			int selected = netCenter().getGlobalStatistic(safe_cast<const UI_ActionDataStatBoard*>(data)->format(), strings);
			UI_ControlComboList::setList(control, strings, selected);
			break;
										   }
		
		case UI_ACTION_INET_STATISTIC_QUERY:
			switch(safe_cast<const UI_ActionDataGlobalStats*>(data)->task()){
			case UI_ActionDataGlobalStats::GET_PREV:
			case UI_ActionDataGlobalStats::GOTO_BEGIN:
				if(netCenter().canGetPrevGlobalStats())
					controlState.enable();
				else
					controlState.disable();
				break;
			case UI_ActionDataGlobalStats::GET_NEXT:
				if(netCenter().canGetNextGlobalStats())
					controlState.enable();
				else
					controlState.disable();
				break;
			}
			break;
		case UI_ACTION_LAN_CHAT_CHANNEL_LIST:{
			netCenter().updateChatChannels();
			ComboStrings  strings;
			int selected = netCenter().getChatChannels(strings);
			UI_ControlComboList::setList(control, strings, selected);
			if(netCenter().autoSubscribeMode())
				controlState.disable();
			else
				controlState.enable();
			break;
										  }
		case UI_ACTION_LAN_CHAT_USER_LIST:{
			netCenter().updateChatUsers();
			ComboStrings  strings;
			int selected = netCenter().getChatUsers(strings);
			UI_ControlComboList::setList(control, strings, selected);
			break;
										  }
		case UI_ACTION_CHAT_MESSAGE_BOARD: {
			ComboStrings  strings;
			netCenter().getChatBoard(strings);
			if(UI_ControlStringList* sp = dynamic_cast<UI_ControlStringList*>(control))
				sp->setList(strings);
			else {
				string txt;
				joinComboList(txt, strings, '\n');
				control->setText(txt.c_str());
			}
			break;
										   }
		case UI_ACTION_GAME_CHAT_BOARD:
			if(UI_ControlStringList* sp = dynamic_cast<UI_ControlStringList*>(control))
				sp->setList(chatMessages_);
			else {
				string txt;
				joinComboList(txt, chatMessages_, '\n');
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
				ComboStrings strings;
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
					ComboStrings strings;
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
				sColor4f color = color_index <= GlobalAttributes::instance().playerAllowedColorSize() ? GlobalAttributes::instance().playerColors[color_index] : sColor4f(1.f, 1.f, 1.f);
				control->toggleBorder(true);
				control->setBorderColor(color);
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
					XBuffer buf;
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
				control->setState(mission->getUserData(pdata->playerIndex(), pdata->teamIndex()).flag_playerStartReady);
										}
			break;			

		case UI_ACTION_LAN_PLAYER_NAME:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_PLAYER_NAME, controlState, pdata))
				control->setText(mission->playerName(pdata->playerIndex(), pdata->teamIndex()));
									   }
			break;

		case UI_ACTION_LAN_PLAYER_STATISTIC_NAME:{
			const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
			if(const MissionDescription* mission = getControlStateByGameType(TUNE_PLAYER_STAT_NAME, controlState, pdata)){
				RealPlayerType type = mission->playerData(pdata->playerIndex()).realPlayerType;
				if(type == REAL_PLAYER_TYPE_AI)
					control->setText(GET_LOC_STR(UI_COMMON_TEXT_SLOT_AI));
				else
					control->setText(mission->playerName(pdata->playerIndex(), 0));
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
				string str = mission->missionDescription();
				expandTextTemplate(str);
				control->setText(str.c_str());
			}
			else
				control->setText(0);
			break;

		case UI_ACTION_BIND_GAME_PAUSE: {
			int currentState = (netCenter().isOnPause() ? UI_ActionDataPause::NET_PAUSE : 0)
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
			if(safe_cast<const UI_ActionDataBindErrorStatus*>(data)->errorStatus() == netCenter().status())
				controlState.show();
			else
				controlState.hide();
			break;

		case UI_ACTION_LAN_CREATE_GAME:
			if(netCenter().canCreateGame())
				controlState.enable();
			else
				controlState.disable();
			break;

		case UI_ACTION_LAN_JOIN_GAME:
			if(netCenter().canJoinGame())
				controlState.enable();
			else
				controlState.disable();
			break;

		case UI_ACTION_BIND_NET_PAUSE:
			if(netCenter().isOnPause())
				controlState.show();
			else
				controlState.hide();
			break;

		case UI_ACTION_NET_PAUSED_PLAYER_LIST: {
			ComboStrings names;
			netCenter().getPausePlayerList(names);
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control))
				lst->setList(names);
			else
				control->setText(names.empty() ? 0 : names[0].c_str());
			break;
											   }
		case UI_ACTION_PLAYER_UNITS_COUNT:
			if(universe() && universe()->activePlayer()){
				XBuffer buf;
				buf <= universe()->activePlayer()->unitNumber();
				control->setText(buf);
			}
			break;
		
		case UI_ACTION_PLAYER_STATISTICS:
			if(universe() && isGameActive()){
				const UI_ActionPlayerStatistic* action = safe_cast<const UI_ActionPlayerStatistic*>(data);
				if(action->statisticType() == UI_ActionPlayerStatistic::LOCAL){
					int index = gameShell->CurrentMission.findPlayerIndex(action->playerIndex());
					if(index >= 0){
						XBuffer buf;
						buf <= universe()->findPlayer(index)->playerStatistics()[action->type()];
						control->setText(buf);
						controlState.show();
					}
					else
						controlState.hide();
				}
				else {
					XBuffer buf;
					buf <= netCenter().getCurrentGlobalStatisticValue(action->type());
					control->setText(buf);
				}
			}
			break;
		
		case UI_ACTION_UNIT_HINT:{
			const UI_ActionDataUnitHint* action = safe_cast<const UI_ActionDataUnitHint*>(data);
			if(action->unitType() != UI_ActionDataUnitHint::SELECTED && !tipsEnabled())
				break;
			
			UnitInterface* unit = 0;
			const AttributeBase* attr = 0;

			bool need_delay = true;
			
			switch(action->unitType()){
			case UI_ActionDataUnitHint::HOVERED:
				unit = hoverUnit();
				if(unit){
					unit = unit->getUnitReal();
					attr = &unit->attr();
				}
				else
					attr = 0;
				break;
			case UI_ActionDataUnitHint::SELECTED:
				unit = selectedUnit();
				if(unit){
					unit = unit->getUnitReal();
					attr = &unit->attr();
				}
				else
					attr = 0;
				need_delay = false;
				break;
			case UI_ActionDataUnitHint::CONTROL:
				if(const UI_ControlBase* p = hoverControl_){
					const AttributeBase* selected = 0;
					if(const UnitInterface* unit = selectedUnit())
						selected = unit->selectionAttribute();
					attr = p->actionUnit(selected);
				}
				break;
			}

			if(attr){
				const char* hint = 0;
				switch(action->hintType()){
				case UI_ActionDataUnitHint::SHORT:
					hint = attr->interfaceName(action->lineNum());
					break;
				case UI_ActionDataUnitHint::FULL:
					hint = attr->interfaceDescription(action->lineNum());
					break;
				}

				if(hint && *hint){
					if(need_delay){
						HoverInfos::const_iterator it = std::find(hoverInfos_.begin(), hoverInfos_.end(), control);
						if(it != hoverInfos_.end()){
							if(it->unit() == attr){
								if(tipsUnitDelay_ > FLT_EPS)
									controlState.hide();
								else
									controlState.show();
								string str = hint;
								expandTextTemplate(str, unit, attr);
								if(!str.empty())
									control->setText(str.c_str());
								else {
									controlState.hide();
									tipsUnitDelay_ = UI_Dispatcher::instance().tipsDelay();
								}
							}
							else
								tipsUnitDelay_ = UI_Dispatcher::instance().tipsDelay();
						}
						else 
							tipsUnitDelay_ = UI_Dispatcher::instance().tipsDelay();

						setHoverInfo(UI_HoverInfo(control, hoverControl_, attr));
					}
					else {
						string str = hint;
						expandTextTemplate(str, unit, attr);
						if(!str.empty())
							control->setText(str.c_str());
						else 
							controlState.hide();
					}
				}
				else {
					controlState.hide();
					if(need_delay)
						setHoverInfo(UI_HoverInfo(control, hoverControl_, 0));
				}
			}
			else {
				controlState.hide();
				if(need_delay)
					setHoverInfo(UI_HoverInfo(control, hoverControl_, 0));
			}
			
			break;
								 }
		case UI_ACTION_UI_HINT: {
			const UI_ControlBase* hoverControl = hoverControl_;
			if(hoverControl != control){
				if(hoverControl && tipsEnabled()){
					if(*hoverControl->hint()){
						if(tipsControl_ == hoverControl){
							if(tipsControlDelay_ > FLT_EPS)
								controlState.hide();
							else
								controlState.show();
						}
						else {
							tipsControl_ = hoverControl;
							tipsControlDelay_ = UI_Dispatcher::instance().tipsDelay();
						}
						string str = hoverControl->hint();
						const AttributeBase* selected = 0;
						if(const UnitInterface* unit = selectedUnit())
							selected = unit->selectionAttribute();
						expandTextTemplate(str, 0, hoverControl->actionUnit(selected));
						if(!str.empty())
							control->setText(str.c_str());
						else {
							controlState.hide();
							tipsControl_ = 0;
						}
					}
					else {
						controlState.hide();
						tipsControl_ = 0;
					}
				}
				else {
					controlState.hide();
					tipsControl_ = 0;
				}
			}
			
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
				XBuffer buf;
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
				p->setProgress(loadProgress_);
			break;
		
		case UI_ACTION_BUILDING_CAN_INSTALL:
			controlState.apply(universe()->activePlayer()->canBuildStructure(safe_cast<const UI_ActionDataBuildingUpdate*>(data)->attribute()));
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
				ComboStrings strings;
				GameType saveType = getGameType(*action);
				const MissionDescriptions& missions = getMissionsList(*action);
				for(MissionDescriptions::const_iterator it = missions.begin(); it != missions.end(); ++it){
					if(saveType == GAME_TYPE_BATTLE && !it->isBattle())
						continue;
					if(it->missionGUID() == lastMission)
						lst->setSelectedString(strings.size());
					strings.push_back(it->interfaceName());
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
				but->setState(val);
			else if(UI_ControlStringList *lst = dynamic_cast<UI_ControlStringList*>(control))
				lst->setSelectedString(clamp(val, -1, lst->listSize()-1));
									   }
			break;
	}
}

void UI_LogicDispatcher::controlAction(UI_ControlActionID id, UI_ControlBase* control, const UI_ActionData* data)
{
	switch(id){
		case UI_ACTION_CONTROL_COMMAND:
			if(const UI_ActionDataControlCommand* action = safe_cast<const UI_ActionDataControlCommand*>(data)){
				switch(action->command()){
					case UI_ActionDataControlCommand::GET_CURRENT_SAVE_NAME:
						if(currentMission())
							control->setText(currentMission()->interfaceName());
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
						control->setText(currentProfile().cdKey.c_str());
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

		case UI_ACTION_PAUSE_GAME:
			if(safe_cast<const UI_ActionDataPauseGame*>(data)->onlyLocal() && netCenter().isNetGame())
				break;
			
			if(safe_cast<const UI_ActionDataPauseGame*>(data)->enable())
				gameShell->pauseGame(GameShell::PAUSE_BY_MENU);
			else
				gameShell->resumeGame(GameShell::PAUSE_BY_MENU);
			break;

		case UI_ACTION_POST_EFFECT:
			if(environment)
				environment->PEManager()->SetActive(
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
					int preset = indexInComboListString(GameOptions::instance().getPresetList(), control->text());
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
				ComboStrings strings;
				splitComboList(strings, GameOptions::instance().getList(action->Option()));
				if(const UI_ControlStringList* lst = UI_ControlComboList::getList(control))
					GameOptions::instance().setOption(action->Option(), lst->selectedStringIndex());
				else if(data >= 0 && !strings.empty())
					GameOptions::instance().setOption(action->Option(), (data + 1) % strings.size());
			}
			if(GameOptions::instance().needInstantApply(action->Option()))
				GameOptions::instance().setPartialOptionsApply();
			handleMessageReInitGameOptions();
			break;
							  }
		case UI_ACTION_MINIMAP_ROTATION:
			minimap().toggleRotateByCamera(!minimap().isRotateByCamera());
			break;

		case UI_ACTION_BIND_TO_IDLE_UNITS:
			if(UnitInterface* unit = idleUnitsManager.getUdleUnit(safe_cast<const UI_ActionDataIdleUnits*>(data)->type()))
				selectManager->selectUnit(unit, false);
			break;

		case UI_ACTION_LAN_DISCONNECT_SERVER:
			netCenter().reset();
			break;

		case UI_ACTION_LAN_ABORT_OPERATION:
			netCenter().abortCurrentOperation();
			break;

		case UI_ACTION_LAN_CHAT_CHANNEL_LIST:
			if(UI_ControlStringList* sp = dynamic_cast<UI_ControlStringList*>(control)){
				netCenter().selectChatChannel(sp->selectedStringIndex());
			}
			break;
		case UI_ACTION_LAN_CHAT_CHANNEL_ENTER:
			netCenter().enterChatChannel();
			break;

		case UI_ACTION_LAN_GAME_LIST:
			if(UI_ControlStringList* sp = dynamic_cast<UI_ControlStringList*>(control)){
				netCenter().selectGame(sp->selectedStringIndex());
				setCurrentMission(netCenter().selectedGame());
			}
			break;
		
		case UI_ACTION_INET_STATISTIC_SHOW:
			if(UI_ControlStringList* sp = UI_ControlComboList::getList(control))
				netCenter().selectGlobalStaticticEntry(sp->selectedStringIndex());
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
				if(const char* name = lst->selectedString()){
					profileName_ = name;
					handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_PROFILE_INPUT, UI_ActionDataControlCommand::GET_CURRENT_PROFILE_NAME)));
					handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_CDKEY_INPUT, UI_ActionDataControlCommand::GET_CURRENT_CDKEY)));
					handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_CDKEY_INPUT, UI_ActionDataControlCommand::EXECUTE)));
				}
			}
			break;
		
		case UI_ACTION_ONLINE_LOGIN_LIST:
			if(UI_ControlStringList* lst = dynamic_cast<UI_ControlStringList*>(control)){
				if(const char* name = lst->selectedString()){
					currentProfile().lastInetName = name;
					handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_INET_NAME, UI_ActionDataControlCommand::RE_INIT)));
				}
			}
			break;

		case UI_ACTION_MISSION_LIST:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				const UI_ActionDataSaveGameListFixed* action = safe_cast<const UI_ActionDataSaveGameListFixed*>(data);
				if(const MissionDescription* mission = getMissionByName(lst->selectedString(), *action)){
					MissionDescription md(*mission);
					if(gameShell->getNetClient()){
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
				if(const char* text = control->text()){
					int clan = atoi(text);
					mission->changePlayerData(pdata->playerIndex()).clan = clan;
					if(gameShell->getNetClient())
						gameShell->getNetClient()->changePlayerClan(pdata->playerIndex(), clan);
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
					const MissionDescription* clean = getMissionByName(mission->interfaceName());
					xassert(clean);
					MissionDescription md(*clean);
					if(netCenter().isNetGame())
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
					if(gameShell->getNetClient())
						gameShell->getNetClient()->changeRealPlayerType(pdata->playerIndex(), type);
				}
			}
			break;

		case UI_ACTION_LAN_PLAYER_RACE:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				const RaceProperty* val = 0;
				const char* name = control->text();
				const RaceTable::Strings& map = RaceTable::instance().strings();
				RaceTable::Strings::const_iterator it;
				FOR_EACH(map, it)
					if(!strcmp(it->name(), name))
						val = &*it;
				if(val){
					mission->changePlayerData(pdata->playerIndex()).race = Race(val->c_str());
					if(gameShell->getNetClient())
						gameShell->getNetClient()->changePlayerRace(pdata->playerIndex(), Race(val->c_str()));
				}
			}
			break;

		case UI_ACTION_LAN_PLAYER_DIFFICULTY:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				const DifficultyPrm* val = 0;
				const char* name = control->text();
				const DifficultyTable::Strings& map = DifficultyTable::instance().strings();
				DifficultyTable::Strings::const_iterator it;
				FOR_EACH(map, it)
					if(!strcmp(it->name(), name))
						val = &*it;
				if(val){
					mission->changePlayerData(pdata->playerIndex()).difficulty = Difficulty(val->c_str());
					if(gameShell->getNetClient())
						gameShell->getNetClient()->changePlayerDifficulty(pdata->playerIndex(), Difficulty(val->c_str()));
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
				if(gameShell->getNetClient())
					gameShell->getNetClient()->changePlayerColor(pdata->playerIndex(), color_index);
			}
			break;

		case UI_ACTION_LAN_PLAYER_SIGN:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				int sign_index = mission->playerData(pdata->playerIndex()).signIndex;
				if(++sign_index > GlobalAttributes::instance().playerAllowedSignSize())
					sign_index = 0;
				mission->changePlayerData(pdata->playerIndex()).signIndex = sign_index;
				if(gameShell->getNetClient())
					gameShell->getNetClient()->changePlayerSign(pdata->playerIndex(), sign_index);
			}
			break;

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
			currentProfile().cdKey = control->text();
			break;

		case UI_ACTION_PROFILE_CREATE:
			if(!profileName_.empty()){
				if(checkDiskOp(UI_DISK_OP_SAVE_PROFILE, ""))
					makeDiskOp(UI_DISK_OP_SAVE_PROFILE, "", GAME_TYPE_SCENARIO);
				else
					setDiskOp(UI_DISK_OP_SAVE_PROFILE, "");
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
			saveGameName_ = control->text();
			break;

		case UI_ACTION_REPLAY_NAME_INPUT:
			saveReplayName_ = control->text();
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
			currentProfile().lastCreateGameName = control->text();
			break;

		case UI_ACTION_INET_NAME:
			currentProfile().lastInetName = control->text();
			break;

		case UI_ACTION_INET_PASS:
			netCenter().setPassword(control->text());
			break;

		case UI_ACTION_INET_PASS2:
			netCenter().setPass2(control->text());
			break;

		case UI_ACTION_CHAT_EDIT_STRING:
			netCenter().setChatString(control->text());
			break;

		case UI_ACTION_CHAT_SEND_MESSAGE:
			if(netCenter().sendChatString(-1))
				handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_CHAT_EDIT_STRING, UI_ActionDataControlCommand::CLEAR)));
			break;

		case UI_ACTION_CHAT_SEND_CLAN_MESSAGE:
			if(netCenter().sendChatString(gameShell->CurrentMission.playerData(gameShell->CurrentMission.activePlayerID()).clan))
				handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_CHAT_EDIT_STRING, UI_ActionDataControlCommand::CLEAR)));
			break;

		case UI_ACTION_INET_LOGIN:
			netCenter().login();
			break;
		
		case UI_ACTION_INET_STATISTIC_QUERY:
			switch(safe_cast<const UI_ActionDataGlobalStats*>(data)->task()){
			case UI_ActionDataGlobalStats::REFRESH:
				netCenter().queryGlobalStatistic(true);
				break;
			case UI_ActionDataGlobalStats::GET_PREV:
				netCenter().getPrevGlobalStats();
				break;
			case UI_ActionDataGlobalStats::GET_NEXT:
				netCenter().getNextGlobalStats();
				break;
			case UI_ActionDataGlobalStats::FIND_ME:
				netCenter().getAroundMeGlobalStats();
				break;
			case UI_ActionDataGlobalStats::GOTO_BEGIN:
				netCenter().getGlobalStatisticFromBegin();
				break;
			}
			break;

		case UI_ACTION_INET_QUICK_START:
			netCenter().quickStart();
			break;

		case UI_ACTION_INET_REFRESH_GAME_LIST:
			netCenter().refreshGameList();
			break;

		case UI_ACTION_LAN_CREATE_GAME:
			netCenter().createGame();
			break;

		case UI_ACTION_LAN_JOIN_GAME:
			netCenter().joinGame();
			break;

		case UI_ACTION_INET_CREATE_ACCOUNT:
			netCenter().createAccount();
			break;
		
		case UI_ACTION_INET_CHANGE_PASSWORD:
			netCenter().changePassword();
			break;
		
		case UI_ACTION_INET_DELETE_ACCOUNT:
			netCenter().deleteAccount();
			break;

		case UI_ACTION_LAN_PLAYER_JOIN_TEAM:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				netCenter().teamConnect(pdata->playerIndex());
			}
			break;

		case UI_ACTION_LAN_PLAYER_LEAVE_TEAM:
			if(MissionDescription* mission = currentMission()){
				const UI_ActionDataPlayer* pdata = safe_cast<const UI_ActionDataPlayer*>(data);
				netCenter().teamDisconnect(pdata->playerIndex(), pdata->teamIndex());
			}
			break;

		case UI_ACTION_INET_FILTER_PLAYERS_COUNT:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				currentProfile().playersSlotFilter = (lst->selectedStringIndex() <= 0
					? -1
					: clamp(lst->selectedStringIndex() + 1, 2, NETWORK_SLOTS_MAX));
				netCenter().updateFilter();
			}
			break;

		case UI_ACTION_INET_FILTER_GAME_TYPE:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				currentProfile().gameTypeFilter = (lst->selectedStringIndex() <= 0
					? -1
					: clamp(lst->selectedStringIndex() - 1, 0, 1));
				netCenter().updateFilter();
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
					ComboStrings strings;
					lst->getSelect(strings);

					ComboStrings::const_iterator it;
					FOR_EACH(strings, it)
						if(const MissionDescription* mission = missions_.find(it->c_str()))
							filter.filterList.push_back(mission->missionGUID());

					filter.filterDisabled = (filter.filterList.size() == lst->listSize());
				}
				else {
					if(lst->selectedStringIndex() <= 0)
						filter.filterDisabled = true;
					else if(const char* name = lst->selectedString()){
						const MissionDescription* mission = missions_.find(name);
						xassert(mission);
						filter.filterDisabled = false;
						filter.filterList.push_back(mission->missionGUID());
					}
					else
						filter.filterDisabled = true;
				}

				netCenter().updateFilter();
			}
			break;

		case UI_ACTION_QUICK_START_FILTER_RACE:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control))
				currentProfile().quickStartFilterRace = clamp(lst->selectedStringIndex(), 0, 3) - 1;
			break;

		case UI_ACTION_QUICK_START_FILTER_POPULATION:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control))
				currentProfile().quickStartFilterGamePopulation = clamp(lst->selectedStringIndex(), 0, 3);
			break;

		case UI_ACTION_STATISTIC_FILTER_POPULATION:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				currentProfile().statisticFilterGamePopulation = clamp(lst->selectedStringIndex(), 0, 3);
				netCenter().queryGlobalStatistic();
			}
			break;

		case UI_ACTION_STATISTIC_FILTER_RACE:
			if(UI_ControlStringList* lst = UI_ControlComboList::getList(control)){
				currentProfile().statisticFilterRace = clamp(lst->selectedStringIndex(), 0, 2);
				netCenter().queryGlobalStatistic();
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
					if(gameShell->getNetClient())
						gameShell->getNetClient()->changeMissionDescription(MissionDescription::CMDV_ChangeTriggers, flags);
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

void UI_LogicDispatcher::showLoadProgress(float val)
{
	loadProgress_ = val;
	UI_Dispatcher::instance().quickRedraw();
}

void UI_LogicDispatcher::startLoadProgressSection(UI_LoadProgressSectionID id)
{
	float progress = 0.f;

	loadSection_ = id;
	loadStep_ = 0;

	for(LoadProgressSections::const_iterator it = loadProgressSections_.begin(); it != loadProgressSections_.end(); ++it){
		if(it->ID() != id)
			progress += it->size();
		else
			break;
	}

	showLoadProgress(progress);
}

void UI_LogicDispatcher::loadProgressUpdate()
{
	float progress = 0.f;
	for(LoadProgressSections::const_iterator it = loadProgressSections_.begin(); it != loadProgressSections_.end(); ++it){
		if(it->ID() == loadSection_){
			if(++loadStep_ > it->stepCount())
				loadStep_ = it->stepCount();

			float progress_inc = it->size() / float(it->stepCount()) * float(loadStep_);
			showLoadProgress(progress + progress_inc);
			return;
		}
		else
			progress += it->size();
	}

	xassert(0);
}

void UI_LogicDispatcher::expandTextTemplate(string& text, UnitInterface* unit, const AttributeBase* attr){
	const std::string::size_type npos = std::string::npos;
	std::string out;

	string::size_type begin = 0, end = 0;

	for(;;){
		if(end >= text.size())
			break;

		begin = text.find("{", end);
		if(begin == npos){
			out += text.substr(end);
			break;
		}else if(begin && text[begin-1] == '\\'){
			out += "{";
			end = begin + 1;
			continue;
		}else
			out += text.substr(end, begin - end);

		end = text.find("}", begin);
		if(end == npos){
			out += text.substr(begin);
			break;
		}

		if(text[begin + 1] == '$'){ // обязательный тег
			const char* par = getParam(text.substr(begin + 2, end - begin - 2).c_str(), unit, attr);
			if(par && *par)
				out += par;
			else {
				out.clear();
				break;
			}
		}
		else
			out += getParam(text.substr(begin + 1, end - begin - 1).c_str(), unit, attr);

		++end;
	}

	text = out;
}

const char* UI_LogicDispatcher::getParam(const char* name, UnitInterface* unit, const AttributeBase* attr)
{
	MTL();
	
	static XBuffer retBuf;
	retBuf.init();

	if(!attr && unit)
		attr = &unit->getUnitReal()->attr();

	xassert(name && *name);

	ParameterSet cacheParams;
	const ParameterSet* params = 0;
	
	int plrID = -1;
	if(name[1] == '#'){
		unsigned char numChar = *name;
		if(numChar >= 0x30 && numChar <= 0x39){
			name += 2;
			plrID = gameShell->CurrentMission.findPlayerIndex(numChar - 0x30);
		}
	}

	if(name[1] == '!'){ // это из параметров юнита
		switch(*name){
		case 'c': // текущие личные параметры
			if(unit)
				params = &(unit->getUnitReal()->parameters());
			break;
		case 'm': // максимальные личные параметры
			if(unit)
				params = &(unit->getUnitReal()->parametersMax());
			break;
		case 'b': // ресурсы для постройки
			if(attr)
				params = &(attr->creationValue);
			break;
		case 'i': // ресурсы для заказа
			if(attr)
				params = &(attr->installValue);
			break;
		case 'a': // необходимые параметры
			if(attr)
				params = &(attr->accessValue);
			break;
		case 'w':// параметр из личных параметров оружия
		case 'd': // параметр из урона оружия
		case 's': // из abnormalState
			if(unit){
				char type = *name;
				//имя параметра из оружия задается в виде: WeaponName/ParameterNameOrModifer
				//WeaponName - ключ локализации для имени оружия
				name+=2;
				if(const char* delimeter = strchr(name, '/')){
					const WeaponBase* weapon = 0;
					const UnitActing* shooter = safe_cast<const UnitActing*>(unit->getUnitReal());
					if(name != delimeter){
						string weaponLabel(name, delimeter);
						weapon = shooter->findWeapon(weaponLabel.c_str());
					}
					else if(int weaponID = shooter->selectedWeaponID())
						weapon = shooter->findWeapon(weaponID);  // текущее выбранное оружие

					if(weapon && weapon->isEnabled() && weapon->getParametersForUI(++delimeter, type, cacheParams))
						params = &cacheParams;
					name = delimeter - 2;
				}
			}
			break;
		case '*': // параметры из текущего контрола под мышой, стоимость команды показывается для заселекченного
			if(const UI_ControlBase* control = hoverControl())
				if(const AttributeBase* attr = selectManager->selectedAttribute())
					if(control->actionParameters(attr, cacheParams))
						params = &cacheParams;
					else
						params = 0;
		
			break;
		}
		name += 2;
	}
	else if(name[1] == '&'){ // это из параметров игрока
		const Player* plr = plrID < 0 ? universe()->activePlayer() : universe()->findPlayer(plrID);
		xassert(plr);
		switch(*name){
		case 'c': // текущие ресурсы игрока
			params = &(plr->resource());
			break;
		case 'm': // максимальные ресурсы игрока
			params = &(plr->resourceCapacity());
			break;
		}
		name += 2;
	}
	else if(*name == '@')
		return TextDB::instance().getText(name + 1);
	else {
		UI_TextTags::const_iterator it = uiTextTags.find(name);
		if(it != uiTextTags.end()){
			switch(it->second){
			case UI_TAG_PLAYER_DISPLAY_NAME:
				if(plrID >= 0){
					const Player* plr = universe()->findPlayer(plrID);
					retBuf < (plr->realPlayerType() & REAL_PLAYER_TYPE_PLAYER ? plr->name() : GET_LOC_STR(UI_COMMON_TEXT_AI));
				}
				break;
			case UI_TAG_PLAYERS_COUNT:
				retBuf <= universe()->playersNumber();
				break;
			case UI_TAG_CURRENT_GAME_NAME:
				retBuf < gameShell->CurrentMission.interfaceName();
				break;
			case UI_TAG_SELECTED_GAME_NAME:
				if(const MissionDescription* mission = currentMission())
					retBuf < mission->interfaceName();
				break;
			case UI_TAG_SELECTED_MULTIPLAYER_GAME_NAME:
				if(const char* name = netCenter().selectedGameName())
					retBuf < name;
				break;
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
				string get;
				getNetCenter().getCurrentChatChannelName(get);
				retBuf < get.c_str();
				break;
											 }
			case UI_TAG_HOTKEY: // hotkey для кнопки под мышкой
				if(const UI_ControlBase* hov = hoverControl())
					retBuf < hov->hotKey().toString().c_str();
				break;
			case UI_TAG_ACCESSIBLE: // что надо для доступности юнита
				if(attr)
					player()->printAccessible(retBuf, attr->accessBuildingsList, enableColorString(), disableColorString());
				break;
			case UI_TAG_ACCESSIBLE_PARAM: // что надо для доступности параметра
				if(const UI_ControlBase* control = hoverControl())
					if(const AttributeBase* attr = selectManager->selectedAttribute())
						if(const ProducedParameters* par = control->actionBuildParameter(attr))
							player()->printAccessible(retBuf, par->accessBuildingsList, enableColorString(), disableColorString());
				break;
			case UI_TAG_LEVEL: // уровень юнита
				if(unit && unit->getUnitReal()->attr().isLegionary())
					retBuf <= safe_cast<const UnitLegionary*>(unit->getUnitReal())->level() + 1;
				break;
			case UI_TAG_ACCOUNTSIZE: // сколько слотов занимает юнит
				if(attr)
					retBuf <= attr->accountingNumber;
				break;
			case UI_TAG_SUPPLYSLOTS: // сколько слотов занимает юнит (с раскраской)
				if(attr){
					if(universe()->activePlayer()->checkUnitNumber(attr))
						retBuf < enableColorString();
					else
						retBuf < disableColorString();
					retBuf <= attr->accountingNumber;
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
					retBuf < "0";
				retBuf <= time;
				break;
								 }
			case UI_TAG_TIME_AMPM:{
				xassert(environment);
				int time = environment->getTime();
				UI_CommonLocText tm = UI_COMMON_TEXT_TIME_AM;
				if(time > 12 || time < 1)
					tm = UI_COMMON_TEXT_TIME_PM;
				retBuf < getLocString(tm, tm == UI_COMMON_TEXT_TIME_AM ? "a.m." : "p.m.");
				break;
								  }
			case UI_TAG_TIME_H24:{
				xassert(environment);
				int time = environment->getTime();
				if(time < 10)
					retBuf < "0";
				retBuf <= time;
				break;
								 }
			case UI_TAG_TIME_M:{
				xassert(environment);
				float time = environment->getTime();
				time -= floor(time);
				time *= 60.f;
				if(time < 10.f)
					retBuf < "0";
				retBuf <= int(time);
				break;
							   }
			default:
				xxassert(false, "такого быть не должно!");
			    return "";
			}
			return retBuf.c_str();
		}
		return "";
	}

	if(params){
		switch(*name){
		case '?':  // чего не хватает у игрока
			player()->resource().printSufficient(retBuf, *params, enableColorString(), disableColorString());
		case '&': // чего не хватает у юнита
			if(unit)
				unit->getUnitReal()->parameters().printSufficient(retBuf, *params, enableColorString(), disableColorString());
			break;
		case '!': // просто напечатать все параметры
			params->toString(retBuf);
		default: // вывести конкретный параметр
			retBuf <= round(params->findByLabel(name));
		}
		return retBuf.c_str();
	}

	return "";
}

const Vect3f& UI_LogicDispatcher::cursorPosition() const
{
	return (selectedWeapon_ && !selectedWeapon_->targetOnWaterSurface()) ? hoverTerrainPosition_ : hoverPosition_;
}

WeaponTarget UI_LogicDispatcher::attackTarget() const
{
	UnitInterface* p = hoverUnit_;
	if(p && !p->attr().isActing()) p = 0;
	return WeaponTarget(p, cursorPosition(), selectedWeaponID());
}

void UI_LogicDispatcher::selectCursor(const UI_Cursor* cameraCursor)
{
#ifndef _FINAL_VERSION_
#define SET_CURSOR_REASON(reason) debugCursorReason_ = (reason)
#else
#define SET_CURSOR_REASON(reason)
#endif

	xassert(universe());
	UnitInterface* hovered_unit = hoverUnit();

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
		if(control->hoveredCursor()){
			SET_CURSOR_REASON("control own hover cursor");
			setCursor(control->hoveredCursor());
		}
		else if(gameShell->GameActive){
			SET_CURSOR_REASON("general interface");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_INTERFACE));
		}
		else {
			SET_CURSOR_REASON("main menu");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_MAIN_MENU));
		}
		showCursor();
	}
	else if(UI_Dispatcher::instance().hasFocusedControl()){
		if(gameShell->GameActive){
			SET_CURSOR_REASON("general interface, has focused control");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_INTERFACE));
		}
		else {
			SET_CURSOR_REASON("main menu, has focused control");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_MAIN_MENU));
		}
		showCursor();
	}
	else if(!player()->controlEnabled()){
		SET_CURSOR_REASON("player control disabled");
		setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_PLAYER_CONTROL_DISABLED));
	}
	else if(gameShell->underFullDirectControl() && !gameShell->isPaused(GameShell::PAUSE_BY_ANY)){
		if(UI_LogicDispatcher::instance().isAttackCursorEnabled()){
			SET_CURSOR_REASON("direct control, attack cursor");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_DIRECT_CONTROL_ATTACK));
		}
		else {
			SET_CURSOR_REASON("direct control, can't attack");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_DIRECT_CONTROL));
		}
	}
	else if (isMouseFlagSet(MK_LBUTTON) &&
		UI_GlobalAttributes::instance().cursor(UI_CURSOR_MOUSE_LBUTTON_DOWN))
	{
		SET_CURSOR_REASON("left button down");
		setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_MOUSE_LBUTTON_DOWN));	
	}
	else if	(isMouseFlagSet(MK_RBUTTON) &&
		UI_GlobalAttributes::instance().cursor(UI_CURSOR_MOUSE_RBUTTON_DOWN))
	{
		SET_CURSOR_REASON("right button down");
		setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_MOUSE_RBUTTON_DOWN));	
	}
	else if(clickMode() != UI_CLICK_MODE_NONE){
		switch(clickMode()){
				case UI_CLICK_MODE_MOVE:
					if(hoverPassable()){
						SET_CURSOR_REASON("click mode move, can walk");
						setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_WALK));
					}
					else {
						SET_CURSOR_REASON("click mode move, walk disabled");
						setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_WALK_DISABLED));
					}
					break;
				case UI_CLICK_MODE_PATROL:
					if(hoverPassable()){
						SET_CURSOR_REASON("click mode patrol, can walk");
						setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_PATROL));
					}
					else {
						SET_CURSOR_REASON("click mode patrol, walk disabled");
						setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_PATROL_DISABLED));
					}
					break;
				case UI_CLICK_MODE_ATTACK:
					if(isAttackCursorEnabled())
						if(hovered_unit)
							if(hovered_unit->player()->clan() == universe()->activePlayer()->clan()){
								SET_CURSOR_REASON("click mode attack, hover unit, eq clans (friend attack)");
								setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_FRIEND_ATTACK));
							}
							else {
								SET_CURSOR_REASON("click mode attack, hover not friendly unit");
								setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_ATTACK));
							}
						else {
							SET_CURSOR_REASON("click mode attack, attack possible");
							setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_ATTACK));
						}
					else {
						SET_CURSOR_REASON("click mode attack, attack disabled");
						setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_ATTACK_DISABLED));
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
				setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_ITEM_CAN_PIC));
			}
			else if(hovered_unit->attr().isResourceItem() && selectManager->canExtractResource(safe_cast<const UnitItemResource*>(hovered_unit))){
				SET_CURSOR_REASON("object hover, selected unit can extract from object");
				setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_ITEM_EXTRACT));
			}
			else {
				SET_CURSOR_REASON("object hover, this is resource or inventory item");
				setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_ITEM_OBJECT));
			}
		else if(hovered_unit->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>(hovered_unit->attr()).teleport && 
				selectManager->canTeleportate(safe_cast<const UnitBuilding*>(hovered_unit->getUnitReal()))){
			SET_CURSOR_REASON("object hover, can teleportate selected unit");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_TELEPORT));
		}
		else if(universe()->activePlayer()->clan() == hovered_unit->player()->clan())
			if(isAttackCursorEnabled()){
				SET_CURSOR_REASON("object hover, can attack, eq clans (friend attack)");
				setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_FRIEND_ATTACK));
			}
			else
				if(universe()->activePlayer() == hovered_unit->player())
					if(hovered_unit->attr().isTransport() && selectManager->canPutInTransport(safe_cast<UnitActing*>(hovered_unit))){
						SET_CURSOR_REASON("own object hover, this is transport, selected can put in");
						setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_TRANSPORT)); // для транспортов в которые можно сесть отдельный курсор
					}
					else if(hovered_unit->attr().isBuilding() && !safe_cast<const UnitBuilding*>(hovered_unit)->isConstructed()
						&& selectManager->canBuild(hovered_unit->getUnitReal())){
							SET_CURSOR_REASON("own object hover, this is not constructed building, selected build this");
							setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_CAN_BUILD)); // может достроить этот долгострой
						}
					else {
						SET_CURSOR_REASON("own object hover");
						setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_PLAYER_OBJECT));
					}
				else {
					SET_CURSOR_REASON("any object hover, eq clans, can't attack this");
					setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_ALLY_OBJECT));
				}
		else if(hovered_unit->player()->isWorld()){
			SET_CURSOR_REASON("any world object hover");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_WORLD_OBJECT));
		}
		else if(isAttackCursorEnabled()){
			SET_CURSOR_REASON("any enemy object hover, can attack");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_ENEMY_OBJECT));
		}
		else {
			SET_CURSOR_REASON("object hover, can't attack this");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_ATTACK_DISABLED));
		}
	}
	else if(cursorInWorld()) // Если пересеклись с землей
		if(!hoverPassable()) {
			SET_CURSOR_REASON("ipassible region in world");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_IMPASSABLE));
		}
		else if(hoverWater()) {
			SET_CURSOR_REASON("water in world");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_WATER));
		}
		else {
			SET_CURSOR_REASON("in world, can walk to this pint");
			setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_PASSABLE));
		}
	else if(gameShell->GameActive){
		SET_CURSOR_REASON("out of world");
		setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_IMPASSABLE));
	}
	else {
		SET_CURSOR_REASON("game not started, main menu");
		setCursor(UI_GlobalAttributes::instance().cursor(UI_CURSOR_MAIN_MENU));
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
	case UI_DISK_OP_SAVE_PROFILE:
		return !profileSystem().isProfileExist(path);
	}

	return true;
}

void UI_LogicDispatcher::setHoverInfo(const UI_HoverInfo& info)
{
	HoverInfos::iterator it = std::find(hoverInfos_.begin(), hoverInfos_.end(), info.ownerControl());

	if(it != hoverInfos_.end())
		it->set(info);
	else
		hoverInfos_.push_back(info);
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

void UI_LogicDispatcher::handleNetwork(eNetMessageCode message)
{
	UI_CommonLocText locKey = UI_COMMON_TEXT_LAST_ENUM;
	string str;

	switch(message){
	case NetGEC_ConnectionFailed:
		str="General error:ConnectionFailed";
		gameShell->getNetClient()->FinishGame();
		locKey = UI_COMMON_TEXT_ERROR_SESSION_TERMINATE;
		break;
	case NetGEC_HostTerminatedSession:
		gameShell->getNetClient()->FinishGame();
		locKey = UI_COMMON_TEXT_ERROR_SESSION_TERMINATE;
		str="HostTerminatedSession";
		break;
	case NetGEC_DWLobbyConnectionFailed:
		locKey = UI_COMMON_TEXT_ERROR_DISCONNECT;
		str="DWLobbyConnectionFailed";
		break;
	case NetGEC_DWLobbyConnectionFailed_MultipleLogons:
		locKey = UI_COMMON_TEXT_ERROR_DISCONNECT_MULTIPLE_LOGON;
		str="DWLobbyConnectionFailed_MultipleLogons";
		break;

	case NetGEC_GameDesynchronized:
		locKey = UI_COMMON_TEXT_ERROR_DESINCH;
		str="GameDesynchronized";
		break;

		//Init
	case NetRC_Init_Ok:
		break;
	case NetRC_Init_Err:
		gameShell->stopNetClient();
		locKey = UI_COMMON_TEXT_ERROR_CANT_CONNECT;
		str="Internet init error";
		break;


		//Create Account
	case NetRC_CreateAccount_Ok:
		break;
	case NetRC_CreateAccount_BadLicensed:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE_BAD_LIC;
		str="CreateAccount Err: BadLicensed";
		break;
	case NetRC_CreateAccount_IllegalOrEmptyPassword_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_PASSWORD;
		str="CreateAccount Err: NetRC_CreateAccount_IllegalOrEmptyPassword_Err";
		break;
	case NetRC_CreateAccount_IllegalUserName_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_BAD_NAME;
		str="CreateAccount Err: IllegalUserName";
		break;
	case NetRC_CreateAccount_VulgarUserName_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_VULGAR_NAME;
		str="CreateAccount Err: VulgarUserName";
		break;
	case NetRC_CreateAccount_UserNameExist_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_NAME_EXIST;
		str="CreateAccount Err: UserNameExist";
		break;
	case NetRC_CreateAccount_MaxAccountExceeded_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE_MAX;
		str="CreateAccount Err: MaxAccountExceeded";
		break;
	case NetRC_CreateAccount_Other_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_CREATE;
		str="CreateAccount Err: Unknown";
		break;

	case NetRC_ChangePassword_Ok:
		break;
	case NetRC_ChangePassword_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_CHANGE_PASSWORD;
		str="ChangePassword Err.";
		break;

	case NetRC_DeleteAccount_Ok:
		break;
	case NetRC_DeleteAccount_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_DELETE;
		str="DeleteAccount Err.";
		break;


		//Configurate
	case NetRC_Configurate_Ok:
		break;
	case NetRC_Configurate_ServiceConnect_Err:
		locKey = UI_COMMON_TEXT_ERROR_CONNECTION;
		str="Configurate: ServiceConnect Err";
		break;
	case NetRC_Configurate_UnknownName_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_UNKNOWN_NAME;
		str="Configurate: UnknownName";
		break;
	case NetRC_Configurate_IncorrectPassword_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_INCORRECT_PASSWORD;
		str="Configurate: IncorrectPassword";
		break;
	case NetRC_Configurate_AccountLocked_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_LOCKED;
		str="Configurate: AccountLocked";
		break;


	case NetRC_CreateGame_Ok:
		setCurrentMission(gameShell->getNetClient()->getCurrentMissionDescription());
		break;
	case NetRC_CreateGame_CreateHost_Err:
		locKey = UI_COMMON_TEXT_ERROR_CREATE_GAME;
		str="Create Game Error";
		break;

	case NetRC_QuickStart_Ok:
		break;
	case NetRC_QuickStart_Err:
		locKey = UI_COMMON_TEXT_ERROR_UNKNOWN;
		break;

	case NetRC_JoinGame_Ok:
		//setCurrentMission(gameShell->getNetClient()->getCurrentMissionDescription());
		break;
	case NetRC_JoinGame_GameSpyPassword_Err:
		locKey = UI_COMMON_TEXT_ERROR_ACCOUNT_INCORRECT_PASSWORD;
		break;
	case NetRC_JoinGame_GameSpyConnection_Err:
	case NetRC_JoinGame_Connection_Err:
		locKey = UI_COMMON_TEXT_ERROR_CONNECTION_GAME;
		str="Join Game: Connection Error";
		break;
	case NetRC_JoinGame_GameIsRun_Err:
		locKey = UI_COMMON_TEXT_ERROR_CONNECTION_GAME_IS_RUN;
		str="Join Game Connection Error: GameIsRun";
		break;
	case NetRC_JoinGame_GameIsFull_Err:
		locKey = UI_COMMON_TEXT_ERROR_CONNECTION_GAME_IS_FULL;
		str="Join Game Connection Error: GameIsFull";
		break;
	case NetRC_JoinGame_GameNotEqualVersion_Err:
		locKey = UI_COMMON_TEXT_ERROR_INCORRECT_VERSION;
		str="Join Game Connection Error: GameNotEqualVersion";
		break;

	case NetRC_ReadStats_Ok:
	//case NetRC_ReadStats_Empty:
	case NetRC_WriteStats_Ok:
	case NetRC_ReadGlobalStats_Ok:
	case NetRC_LoadInfoFile_Ok:
		break;
	case NetRC_ReadStats_Err:
	case NetRC_WriteStats_Err:
	case NetRC_ReadGlobalStats_Err:
		//locKey = UI_COMMON_TEXT_ERROR_UNKNOWN;
		break;
	
	case NetRC_LoadInfoFile_Err:
		locKey = UI_COMMON_TEXT_ERROR_CONNECTION;
		str="Read version info failed";
		break;

	case NetRC_Subscribe2ChatChanel_Ok:
		getNetCenter().chatSubscribeOK();
		return;
	case NetRC_Subscribe2ChatChanel_Err:
		getNetCenter().chatSubscribeFailed();
		return;

	case NetMsg_PlayerDisconnected:
	case NetMsg_PlayerExit:
		break;

	default:
		locKey = UI_COMMON_TEXT_ERROR_UNKNOWN;
		str="Unknown Message-Error";
	}

	switch(locKey){
	case UI_COMMON_TEXT_LAST_ENUM:
		getNetCenter().commit(UI_NET_OK);
		return;
	case UI_COMMON_TEXT_ERROR_CANT_CONNECT:
	case UI_COMMON_TEXT_ERROR_DISCONNECT:
	case UI_COMMON_TEXT_ERROR_DISCONNECT_MULTIPLE_LOGON:
		getNetCenter().commit(UI_NET_SERVER_DISCONNECT);
	    break;
	case UI_COMMON_TEXT_ERROR_SESSION_TERMINATE:
		getNetCenter().commit(UI_NET_TERMINATE_SESSION);
		break;
	default:
		getNetCenter().commit(UI_NET_ERROR);
	    break;
	}

#ifndef _FINAL_VERSION_
		XBuffer buf;
		buf < str.c_str() < " (" <= (int)message < ")\n";
		buf < getLocString(locKey, "NO COMMON LOCTEXT KEY");
		UI_Dispatcher::instance().messageBox(buf);
#else
		UI_Dispatcher::instance().messageBox(getLocString(locKey, "NO MESSAGE"));
#endif

}

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
	case UI_DISK_OP_SAVE_PROFILE:
		if(!profileName_.empty()){
			int idx = profileSystem().updateProfile(profileName_.c_str());
			if(idx >= 0)
				if(profileSystem().setCurrentProfile(idx)){
					handleMessage(ControlMessage(UI_ACTION_CONTROL_COMMAND, &UI_ActionDataControlCommand(UI_ACTION_PROFILES_LIST, UI_ActionDataControlCommand::RE_INIT)));
					profileReseted();
				}
		}
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
