#include "StdAfx.h"
#include "UserInterface.h"
#include "CommonLocText.h"

#include "UI_Logic.h"
#include "UI_Render.h"
#include "IRenderDevice.h"
#include "IVisGeneric.h"
#include "IVisD3D.h"
#include "..\Render\d3d\D3DRender.h"
#include "..\Render\src\postEffects.h"

#include "SafeCast.h"
#include "Serialization.h"
#include "EditArchive.h"
#include "ResourceSelector.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "..\Units\UnitItemResource.h"
#include "..\Units\UnitItemInventory.h"

#include "ShowHead.h"
#include "UI_Controls.h"
#include "UI_UnitView.h"
#include "UI_CustomControls.h"
#include "UI_StreamVideo.h"
#include "UI_Actions.h"
#include "UI_NetCenter.h"

#include "..\Util\SystemUtil.h"
#include "..\Units\CircleManagerParam.h"

#include "..\Game\SoundApp.h"
#include "Universe.h"

#include "StreamCommand.h"
extern StreamDataManager uiStreamLogicCommand;
extern StreamDataManager uiStreamGraphCommand;

extern Singleton<UI_StreamVideo> streamVideo;

const char* getColorString(const sColor4f& color)
{
	static char buf[32];
	sprintf(buf, "&%02X%02X%02X", color.GetR(), color.GetG(), color.GetB());

	return buf;
}

// ------------------- UI_Screen

void fCommandActivateUI_Screen(void* data)
{
	UI_Screen** scr = (UI_Screen**)data;
	bool flag = *(bool*)(scr + 1);
	(*scr)->initActivationActions(flag);
}

void fCommandSetTask(void* data)
{
	const UI_MessageSetup** setup = (const UI_MessageSetup**)data;
	int* state = (int*)(setup + 1);
	bool is_secondary = *(char*)(state + 1) != 0;

	UI_Dispatcher::instance().setTask(UI_TaskStateID(*state), *setup[0], is_secondary);
}

UI_Screen::UI_Screen() :
	activationTime_(0.0f),
	deactivationTime_(0.0f),
	activationDelay_(0.0f),
	deactivationDelay_(0.0f),
	disableInputAtChangeState_(true)
{
	type_ = ORDINARY;
	active_ = false;
}

UI_Screen::~UI_Screen()
{
}

void UI_Screen::serialize(Archive& ar)
{
	ar.serialize(type_, "type", "��� ������");

	if(ar.isEdit()){
		ComboListString model_str(UI_BackgroundScene::instance().modelComboList(), backgroundModelName_.c_str());

		ar.serialize(model_str, "backgroundModelName", "3D ������");

		if(ar.isInput())
			backgroundModelName_ = model_str;

		UI_BackgroundScene::instance().selectModel(backgroundModelName_.c_str(), false);
	}
	else
		ar.serialize(backgroundModelName_, "backgroundModelName", "3D ������");

	UI_ControlContainer::serialize(ar);

	if(isUnderEditor())
		updateActivationOffsets();

	ar.serialize(activationTime_, "activationTime", "����� ���������");
	ar.serialize(deactivationTime_, "deactivationTime", "����� �����������");

	ar.serializeArray(activationOffsets_, "activationOffsets", 0);

	ar.serialize(activationDelay_, "activationDelay_", 0);
	ar.serialize(deactivationDelay_, "deactivationDelay_", 0);

	ar.serialize(disableInputAtChangeState_, "disableInputAtChangeState", "������������ ���� ��� ���������/�����������");

	if(!ar.isEdit() && ar.isInput())
		updateControlOwners();
}

void UI_Screen::logicInit()
{
	start_timer_auto();
	for(ControlList::iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->logicInit();
}

void UI_Screen::preloadBgScene(cScene* scene, const Player* player) const
{
	if(scene){
		int  model = UI_BackgroundScene::instance().getModelIndex(backgroundModelName_.c_str());
		if(model >= 0)
			UI_BackgroundScene::instance().modelSetup(model).preLoad(scene, player);
	}
}

void UI_Screen::init()
{
	MTL();
	start_timer_auto();

	__super::init();
	logicInit();
}

void UI_Screen::activate()
{
	MTL();
	
	deactivationDelay_ = 0.0f;
	activationDelay_ = activationTime();

	uiStreamCommand.set(fCommandActivateUI_Screen);
	uiStreamCommand << (void*)this;
	uiStreamCommand << true;

	active_ = true;
}

void UI_Screen::deactivate()
{
	activationDelay_ = 0.0f;
	deactivationDelay_ = deactivationTime();

	if(MT_IS_GRAPH())
		initActivationActions(false);
	else{
		uiStreamCommand.set(fCommandActivateUI_Screen);
		uiStreamCommand << (void*)this;
		uiStreamCommand << false;
	}

	active_ = false;
}

void UI_Screen::logicQuant()
{
	start_timer_auto();

	ControlList::iterator it;
	for(it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->logicQuant();
}

void UI_Screen::quant(float dt)
{
	start_timer_auto();
	
	UI_AUTOLOCK();

	activationDelay_ -= dt;
	deactivationDelay_ -= dt;

	ControlList::iterator it;
	for(it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->quant(dt);
}

bool UI_Screen::redraw() const
{
	start_timer_auto();

	UI_AUTOLOCK();
	UI_ControlContainer::redraw();
	
	return true;
}

bool UI_Screen::handleInput(const UI_InputEvent& event, UI_ControlBase* focused_control)
{
	UI_AUTOLOCK();
	if(disableInputAtChangeState_ && (isActivating() || isDeactivating())) 
		return false;

	bool ret = false;

	if(event.isMouseEvent())
		if(!focused_control){
			for(ControlList::reverse_iterator it = controls_.rbegin(); it != controls_.rend(); ++it)
				if((*it)->handleInput(event)){
					ret = true;
					break;
				}
		}
		else {
			UI_ControlBase* p = focused_control->findControl(event.cursorPos());
			if(!p)
				p = focused_control;
			if(p->handleInput(event))
				ret = true;
		}
	else
		if(!focused_control){
			for(ControlList::reverse_iterator it = controls_.rbegin(); it != controls_.rend(); ++it)
				if((*it)->handleInput(event)){
					ret = true;
					break;
				}
		}
		else
			if(focused_control->handleInput(event))
				ret = true;

	return ret;
}

bool UI_Screen::hoverUpdate(const Vect2f& cursor_pos, UI_ControlBase* focused_control)
{
	start_timer_auto();

	UI_LogicDispatcher::instance().setHoverControl(0);

	if(disableInputAtChangeState_ && (isActivating() || isDeactivating())) 
		return false;

	if(!focused_control){
		for(ControlList::reverse_iterator it = controls_.rbegin(); it != controls_.rend(); ++it){
			if((*it)->hoverUpdate(cursor_pos))
				return true;
		}
	}
	else 
		return focused_control->hoverUpdate(cursor_pos);

	return false;
}

void UI_Screen::handleMessage(const ControlMessage& msg){
	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->handleAction(msg);
}

void UI_Screen::updateControlOwners()
{
	if(hasName()){
		for(ControlList::iterator it = controls_.begin(); it != controls_.end(); ++it){
			(*it)->setOwner(this);
			(*it)->updateControlOwners();
		}
	}
}

void UI_Screen::updateIndex()
{
	for(ControlList::iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->updateIndex();
}

void UI_Screen::controlComboListBuild(std::string& list_string, UI_ControlFilterFunc filter) const
{
	if(hasName()){
		for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
			(*it)->controlComboListBuild(list_string, filter);
	}
}

bool UI_Screen::isActivating() const
{
	return activationDelay_ > 0.0f;
}

bool UI_Screen::isDeactivating() const
{
	return deactivationDelay_ > 0.0f;
}

float UI_Screen::controlsActivationTime() const
{
	float time = 0.f;

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		time = max(time, (*it)->activationTime());

	return time;
}

float UI_Screen::controlsDeactivationTime() const
{
	float time = 0.f;

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		time = max(time, (*it)->deactivationTime());

	return time;
}

void UI_Screen::updateActivationOffsets()
{
	for(int i = 0; i < 4; i++)
		activationOffsets_[i] = 0.f;

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->updateActivationOffsets(activationOffsets_);
}

void UI_Screen::initActivationActions(bool activation)
{
	start_timer_auto();
	float duration = (activation ? activationTime_ : deactivationTime_);

	if(duration > FLT_EPS){
		for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
			(*it)->setActivationTransform(duration, activation);
	}

	UI_BackgroundScene::instance().selectModel(backgroundModelName_.c_str());

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it){
		if((*it)->isVisible())
			(*it)->startBackgroundAnimation(UI_BackgroundAnimation::PLAY_STARTUP, true, !activation);
	}
}

float UI_Screen::activationOffset(ActivationMove direction) const
{
	float offs = 0.f;

	switch(direction){
		case ACTIVATION_MOVE_LEFT:
			offs = -UI_Render::instance().screenBorderLeft();
			break;
		case ACTIVATION_MOVE_TOP:
			offs = -UI_Render::instance().screenBorderTop();
			break;
		case ACTIVATION_MOVE_RIGHT:
			offs = UI_Render::instance().screenBorderRight();
			break;
		case ACTIVATION_MOVE_BOTTOM:
			offs = UI_Render::instance().screenBorderBottom();
			break;
	}

	return activationOffsets_[direction] + offs;
}

void UI_Screen::drawDebug2D() const
{
	__super::drawDebug2D();
}

void UI_Screen::saveHotKeys() const
{
	vector<UI_ControlHotKey> lst;
	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->getHotKeyList(lst);

	XBuffer fname;
	fname < name() < ".HotKeys.txt";

	EditArchive ea(0, TreeControlSetup(200, 0, 800, 700, "Scripts\\TreeControlSetups\\screenHotKeys"));
	ea.setTranslatedOnly(false);
	if(ea.edit(lst, fname.c_str())){
		XStream out(fname.c_str(), XS_OUT);
		vector<UI_ControlHotKey>::const_iterator it;
		FOR_EACH(lst, it)
			out < it->ref.referenceString() < ";" < it->hotKey.toString().c_str() < "\r\n";
	}		
}

void UI_Screen::getHotKeyList(vector<sKey>& list) const
{
	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->getHotKeyList(list);
}

// --------------------------------------------------------------------------

SelectionBorderColor::SelectionBorderColor()
{
	selectionBorderColor_ = sColor4f(1,1,1,1);
	selectionTextureName_ = "Scripts\\Resource\\Textures\\selectionFrame.tga";
	selectionCornerTextureName_ = "Scripts\\Resource\\Textures\\selectionFrameCorner.tga";
	needSelectionCenter_ = false;
	selectionCenterTextureName_ = "Scripts\\Resource\\Textures\\selectionFrameCenter.tga";
}

void SelectionBorderColor::serialize(Archive& ar)
{
	ar.serialize(selectionBorderColor_, "selectionBorderColor", "����� ���������: ����");
	ar.serialize(ResourceSelector(selectionTextureName_,ResourceSelector::TEXTURE_OPTIONS), "selectionTextureName", "����� ���������: ��������");
	ar.serialize(ResourceSelector(selectionCornerTextureName_,ResourceSelector::TEXTURE_OPTIONS), "selectionCornerTextureName", "����� ���������: �������� �������� ������ ����");
	ar.serialize(needSelectionCenter_, "needSelectionCenter", "����� ���������: ��������� ��������");
	ar.serialize(ResourceSelector(selectionCenterTextureName_,ResourceSelector::TEXTURE_OPTIONS), "selectionCenterTextureName", "����� ���������: �������� ������");
}


// ------------------- UI_Dispatcher

WRAP_LIBRARY(UI_Dispatcher, "UI_Attribute", "UI_Attribute", "Scripts\\Content\\UI_Attributes", 0, false);
WRAP_LIBRARY(UI_GlobalAttributes, "UI_GlobalAttributes", "UI_GlobalAttributes", "Scripts\\Content\\UI_GlobalAttributes", 0, false);

void UI_Dispatcher::showDebugInfo() const
{
	start_timer_auto();

	if(!showDebugInterface.needShow())
		return;

	UI_LogicDispatcher::instance().showDebugInfo();
}

void UI_Dispatcher::drawDebugInfo() const
{
	start_timer_auto();

	if(!showDebugInterface.needShow())
		return;

	UI_LogicDispatcher::instance().drawDebugInfo();
}

void UI_Dispatcher::drawDebug2D() const
{
	start_timer_auto();

	MTG();

	if(!showDebugInterface.needShow())
		return;

	UI_BackgroundScene::instance().drawDebug2D();

	if(showDebugInterface.screens){
		XBuffer buf;
		if(preloadedScreen_)
			buf < "Preloaded screen: " < preloadedScreen_->name() < "\n";
		if(currentScreen_)
			buf < "Logic screen: " < currentScreen_->name() < "\n";
		if(graphCurrentScreen_)
			buf < "Graph screen: " < graphCurrentScreen_->name() < "\n";

		UI_Render::instance().outDebugText(Vect2f(0.05f, 0.3f), buf.c_str(), &YELLOW);
	}

	if(graphCurrentScreen_)
		graphCurrentScreen_->drawDebug2D();

	if(isModalMessageMode())
		messageScreen_->drawDebug2D();

	if(showDebugInterface.focusedControlBorder){
		if(const UI_ControlBase* focused_control = focusedControl_)
			UI_Render::instance().drawRectangle(focused_control->transfPosition(), sColor4f(0.5f,0.5f,1.0f,0.5f), true);
	}

	UI_LogicDispatcher::instance().drawDebug2D();
}

void UI_Dispatcher::updateDebugControl()
{
	debugControl_ = UI_LogicDispatcher::instance().hoverControl();
}

void UI_Dispatcher::setTask(UI_TaskStateID state, const UI_MessageSetup& message_setup, bool is_secondary)
{
	start_timer_auto();

	if(MT_IS_LOGIC()){
		const UI_Task* task = 0;
		UI_Tasks::iterator it = std::find(tasks_.begin(), tasks_.end(), message_setup);
		if(it != tasks_.end()){
			if(state == UI_TASK_DELETED)
				tasks_.erase(it);
			else {
				it->setState(state);
				task = &*it;
			}
		}
		else {
			if(state != UI_TASK_DELETED){
				tasks_.push_back(UI_Task());
				tasks_.back().set(state, message_setup, is_secondary);
				task = &tasks_.back();
			}
		}

		if(!task)
			return;

		playVoice(message_setup);

		string msg;
		if(task->getText(msg, true))
			sendMessage(message_setup, msg.c_str());
	}
	else 
		uiStreamGraph2LogicCommand.set(fCommandSetTask) << (void*)(&message_setup) << int(state) << (char)is_secondary;
}

bool UI_Dispatcher::getTaskList(std::string& str, bool reverse) const
{
	MTL();

	if(tasks_.empty())
		return false;

	str.clear();
	
	if(reverse){
		UI_Tasks::const_reverse_iterator it = tasks_.rbegin();
		for(;it != tasks_.rend(); ++it){
			string msg;
			if(it->getText(msg)){
				UI_LogicDispatcher::instance().expandTextTemplate(msg);
				str += msg;
				str.push_back('\n');
			}
		}
	}
	else {
		UI_Tasks::const_iterator it;
		FOR_EACH(tasks_, it){
			string msg;
			if(it->getText(msg)){
				UI_LogicDispatcher::instance().expandTextTemplate(msg);
				str += msg;
				str.push_back('\n');
			}
		}
	}

	if(!str.empty())
		str.pop_back();

	return true;
}

bool UI_Dispatcher::getMessageQueue(std::string& str, const UI_MessageTypeReferences& filter, bool reverse) const
{
	MTL();

	if(messageQueue_.empty())
		return false;

	str.clear();

	if(reverse){
		MessageQueue::const_reverse_iterator it = messageQueue_.rbegin();
		for(;it != messageQueue_.rend(); ++it){
			if(find(filter.begin(), filter.end(), it->type()) != filter.end())
				if(const char* text = it->text()){
					if(*text){
						string msg(text);
						UI_LogicDispatcher::instance().expandTextTemplate(msg);
						str += "&>"; str += msg; str += "\n";
					}
				}
		}
	}
	else {
		MessageQueue::const_iterator it;
		FOR_EACH(messageQueue_, it){
			if(find(filter.begin(), filter.end(), it->type()) != filter.end())
				if(const char* text = it->text()){
					if(*text){
						string msg(text);
						UI_LogicDispatcher::instance().expandTextTemplate(msg);
						str += "&>"; str += msg; str += "\n";
					}
				}
		}
	}

	return !str.empty();
}

UI_Dispatcher::UI_Dispatcher()
: currentScreen_(0)
, graphCurrentScreen_(0)
, messageScreen_(0)
, focusedControl_(0)
{
	isEnabled_ = true;
	
	isModalScreen_ = false;

	exitGameInProgress_ = false;
	needResetScreens_ = false;

	controlID_ = 1;

	uiStreamCommand.lock();
	uiStreamGraph2LogicCommand.lock();

	needChangeScreen_ = false;

	autoConfirmDiskOp_ = true;

	privateMessageColor_[0] = 0;
	systemMessageColor_[0] = 0;

	tipsDelay_ = 0.f;


	UI_Render::instance().setFontDirectory(getLocDataPath("Fonts\\"));
	UI_Render::instance().createFonts();

	graphQuantCommit();
	debugLogQuant_ = 0;
	debugGraphQuant_ = 0;
	debugControl_ = 0;
}

UI_Dispatcher::~UI_Dispatcher()
{
	screens_.clear();
}

bool UI_Dispatcher::canExit() const
{
	MTG();
	if(!exitGameInProgress_ && graphCurrentScreen_)
		return !graphCurrentScreen_->isDeactivating();
	return true;
}

void UI_Dispatcher::exitMissionPrepare()
{
	MTG();
	if(!exitGameInProgress_){
		if(graphCurrentScreen_ && graphCurrentScreen_->isActive())
			graphCurrentScreen_->deactivate();
	}
}

void UI_Dispatcher::exitGamePrepare()
{
	exitGameInProgress_ = true;
}

void UI_Dispatcher::clearMessageQueue(bool full)
{
	if(full)
		messageQueue_.clear();
	else{
		MessageQueue::iterator i = messageQueue_.begin();
		while(i != messageQueue_.end())
		{
			if((*i).messageSetup().isInterruptOnDestroy())
				i = messageQueue_.erase(i);
			else
				++i;
		}
	}

}

void UI_Dispatcher::reset()
{
	// ��� ��� ������ ���� ���� �����
	MTL();
	MTG();
	UI_LogicDispatcher::instance().reset();

	graphQuantCommit();
	debugLogQuant_ = 0;
	debugGraphQuant_ = 0;

	clearMessageQueue();
	tasks_.clear();

	uiStreamGraphCommand.lock();
	uiStreamGraphCommand.execute();
	uiStreamGraphCommand.unlock();

	uiStreamLogicCommand.lock();
	uiStreamLogicCommand.execute();
	uiStreamLogicCommand.unlock();
	// ��� ����� ������ ����� � ���
	uiStreamGraph2LogicCommand.execute();
	uiStreamCommand.execute();

	voiceLockTimer_.stop();

	streamVideo().release();

	UI_LogicDispatcher::instance().setHoverControl(0);

	minimap().clearEvents();
}

void UI_Dispatcher::resetScreens()
{
	start_timer_auto();

	if(needResetScreens_ && !exitGameInProgress_){
		xassert(currentScreen_ == graphCurrentScreen_);
		if(currentScreen_)
			currentScreen_->release();
		if(preloadedScreen_ && preloadedScreen_ != currentScreen_)
			preloadedScreen_->release();
		preloadedScreen_ = 0;

		// ����� ������� ������� � ��������� ���������
		UI_Dispatcher clearUI;
		MultiIArchive ia;
		if(ia.open("Scripts\\Content\\UI_Attributes", "..\\ContentBin\\", ""))
			ia.serialize(clearUI, "UI_Attribute", 0);
		else
			ErrH.Abort("UI_Attributes corruption");
		ia.close();

		ScreenContainer::iterator scr;
		FOR_EACH(clearUI.screens(), scr)
			if(UI_Screen *current_screen = screen(scr->name()))
				if(current_screen->type() == UI_Screen::GAME_INTERFACE){
					*current_screen = *scr;
					current_screen->updateControlOwners();
				}

		parseMarkScreens();
		needResetScreens_ = false;
	}

	isModalScreen_ = false;
}

void UI_Dispatcher::relaxLoading()
{
	MTL();
	MTG();

	minimap().relaxLoading();
	// ����� �� ��������� ������ ��������� ���� �� ���������� �� ��������� �����
	isModalScreen_ = false;
	//setCurrentScreenLogic(0);
	//setCurrentScreenGraph(0);
	UI_LogicDispatcher::instance().setHoverControl(0);
	//UI_LogicDispatcher::instance().setLoadProgress(0);
	graphQuantCommit();
}

void UI_Dispatcher::serializeUIEditorPrm(Archive& ar)
{
	ar.serialize(tipsDelay_, "tipsDelay", "�������� ��������� ���������");
	ar.serialize(autoConfirmDiskOp_, "autoConfirmDiskOp", "��������� ���������� ������/�������/�������� ��� �������������");
	ar.serialize(UI_BackgroundScene::instance(), "backgroundScene", "������� �����");
}

void UI_Dispatcher::serializeUserSave(Archive& ar)
{
	ar.serialize(messageQueue_, "messageQueue", 0);
	ar.serialize(tasks_, "tasks", 0);
}

void UI_Dispatcher::serialize(Archive& ar)
{
	serializeUIEditorPrm(ar);

	ar.serialize(screens_, "screens_", "������");

	ar.serialize(needChangeScreen_, "needChangeScreen_", 0);
	ar.serialize(nextScreen_, "nextScreen_", 0);

	if(ar.isInput()){
		screenComboListUpdate();
		parseMarkScreens();

		if(!isUnderEditor()){
			ingameHotKeyList_.clear();
			ingameHotKeyList_.reserve(128);

			for(ScreenContainer::iterator it = screens_.begin(); it != screens_.end(); ++it){
				if(it->type() == UI_Screen::GAME_INTERFACE)
					it->getHotKeyList(ingameHotKeyList_);
			}
		}
	}
}

void UI_Dispatcher::init()
{
	UI_LogicDispatcher::instance().init();
	
	//	for(UI_TextureLibrary::Map::iterator it = UI_TextureLibrary::instance().map().begin(); it != UI_TextureLibrary::instance().map().end(); ++it)
//		it->second->createTexture();

	initBgScene();

	ui_ControlMapReference[ui_ControlMapBackReference[string()] = 0] = string();
	controlID_ = 1;
	ScreenContainer::iterator scr;
	FOR_EACH(screens(), scr)
		scr->updateIndex();

	strncpy(privateMessageColor_, getColorString(UI_GlobalAttributes::instance().privateMessage()), 7);
	privateMessageColor_[7] = 0;
	
	strncpy(systemMessageColor_, getColorString(UI_GlobalAttributes::instance().systemMessage()), 7);
	systemMessageColor_[7] = 0;
}

void UI_Dispatcher::quickRedraw()
{
	start_timer_auto();

	logicQuant(true);

	uiStreamLogicCommand.lock();
	uiStreamLogicCommand.execute();
	uiStreamLogicCommand.unlock();

	gb_RenderDevice->Fill(0,0,0);
	gb_RenderDevice->BeginScene();

	graphQuantCommit();

	redraw();

	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();
}

void UI_Dispatcher::logicQuant(bool pause)
{
	minimap().logicQuant();

	uiStreamGraphCommand.lock();
	uiStreamGraphCommand.execute();
	uiStreamGraphCommand.unlock();

	if(InterlockedDecrement(&graphAfterLogicQuantSequence) >= 0){
		start_timer_auto();

		++debugLogQuant_;

		UI_LogicDispatcher::instance().destroyLinkQuant();

		if(UI_LogicDispatcher::instance().isGameActive())
			UI_LogicDispatcher::instance().logicQuant(logicPeriodSeconds);

		if(needChangeScreen_ && (!currentScreen_ || !currentScreen_->isDeactivating()))
			setCurrentScreenLogic(nextScreen_.screen());
		else {
			if(isModalMessageMode())
				messageScreen_->logicQuant();
			if(currentScreen_)
				currentScreen_->logicQuant();
		}

		MessageQueue::iterator it;
		for(it = messageQueue_.begin(); it != messageQueue_.end();)
			if(it->timerEnd())
				it = messageQueue_.erase(it);
			else
				++it;

		if(debugShowEnabled)
			showDebugInfo();
	}

	uiStreamLogicCommand.lock();
	uiStreamLogicCommand << uiStreamCommand;
	uiStreamCommand.clear();
	uiStreamLogicCommand.unlock();
}

void UI_Dispatcher::quant(float dt)
{
	MTG();
	start_timer_auto();

	graphQuantCommit();
	++debugGraphQuant_;

	uiStreamLogicCommand.lock();
	uiStreamLogicCommand.execute();
	uiStreamLogicCommand.unlock();

	minimap().quant(dt);
	
	streamVideo().ui_quant();

	showHead().quant(dt);

	if(isEnabled_){
		if(isModalMessageMode()){
			const UI_ControlBase* lastHovered = UI_LogicDispatcher::instance().hoverControl();
			messageScreen_->hoverUpdate(UI_LogicDispatcher::instance().mousePosition(), focusedControl_);
			UI_LogicDispatcher::instance().focusControlProcess(lastHovered);
			
			if(graphCurrentScreen_)
				graphCurrentScreen_->quant(dt);

			messageScreen_->quant(dt);
		}
		else if(graphCurrentScreen_){
			const UI_ControlBase* lastHovered = UI_LogicDispatcher::instance().hoverControl();
			graphCurrentScreen_->hoverUpdate(UI_LogicDispatcher::instance().mousePosition(), focusedControl_);
			UI_LogicDispatcher::instance().focusControlProcess(lastHovered);
			graphCurrentScreen_->quant(dt);
		}

		if (UI_BackgroundScene::instance().ready())
			UI_BackgroundScene::instance().graphQuant(dt);
	}

	GameOptions::instance().commitQuant(dt);
	if(GameOptions::instance().needRollBack()){
		GameOptions::instance().restoreSettings();
		UI_LogicDispatcher::instance().handleMessageReInitGameOptions();
	}

	if(UI_LogicDispatcher::instance().isGameActive())
		UI_LogicDispatcher::instance().graphQuant(dt);

	uiStreamGraphCommand.lock();
	uiStreamGraphCommand << uiStreamGraph2LogicCommand;
	uiStreamGraph2LogicCommand.clear();
	uiStreamGraphCommand.unlock();
}

void UI_Dispatcher::redraw() const
{
	MTG();
	start_timer_auto();

	if(isEnabled_){
		// �������� �� ����� (������ ��� ���������)
		if (UI_BackgroundScene::instance().ready())
			UI_BackgroundScene::instance().draw();
	
		if(::isUnderEditor()){
			// �.� SetClipRect() ��� RenderTarget ��������� �� �������� ���������� �������� SetViewport
			D3DVIEWPORT9 vp = { 0, 0, UI_Render::instance().camera()->GetRenderSize().x, UI_Render::instance().camera()->GetRenderSize().y, 0.0f, 1.0f };
			RDCALL(gb_RenderDevice3D->lpD3DDevice->SetViewport(&vp));
		}

		if(graphCurrentScreen_)
			graphCurrentScreen_->redraw();

		if(UI_LogicDispatcher::instance().isGameActive()){
			showHead().draw();
			UI_UnitView::instance().draw();
		}

		if(isModalMessageMode())
			messageScreen_->redraw();
	}

	UI_LogicDispatcher::instance().redraw();

	drawDebug2D();
}

void UI_Dispatcher::setFocus(UI_ControlBase* control)
{
	if(UI_ControlBase* p = focusedControl_){
		if(p != control)
			p->onFocus(false);
	}

	focusedControl_ = control; 
	if(control)
		control->onFocus(true);
}

void fCommandUI_SendMessage(void* data)
{
	const UI_MessageSetup* setup = *(const UI_MessageSetup**)data;

	const char* text = (const char*)(setup + 1);

	UI_Dispatcher::instance().sendMessage(*setup, text);
}

void fCommandUI_RemoveMessage(void* data)
{
	const UI_MessageSetup* setup = *(const UI_MessageSetup**)data;
	bool independent = *(bool*)(setup + 1);
	UI_Dispatcher::instance().removeMessage(*setup, independent);
}

void fCommandUI_MessageBox(void* data)
{
	const char* text = (const char*)data;

	UI_Dispatcher::instance().messageBox(text);
}

void UI_Dispatcher::messageBox(const char* message)
{
	if(MT_IS_LOGIC()){
		if(!message){
			messageBoxMessage_.clear();
		}
		else if(isModalScreen_){
			messageBoxMessage_ += '\n';
			messageBoxMessage_ += message;
		}
		else if(messageScreen_){
			messageBoxMessage_ = message;
			messageScreen_->logicInit();
			isModalScreen_ = true;
		}
	}
	else if(message)
		uiStreamGraph2LogicCommand.set(fCommandUI_MessageBox) << XBuffer((void*)message, strlen(message)+1);
	else
		uiStreamGraph2LogicCommand.set(fCommandUI_MessageBox) << (const char*)0;
}

bool UI_Dispatcher::specialExitProcess()
{
	if(debugDisableSpecialExitProcess)
		return false;

	if(UI_Screen* scr = screen(UI_Screen::EXIT_AD)){
		selectScreen(0);
		messageScreen_ = scr;
		isModalScreen_ = true;
		return true;
	}
	return false;
}

void UI_Dispatcher::closeMessageBox()
{
	isModalScreen_ = false;
}

bool UI_Dispatcher::sendMessage(UI_MessageID id)
{
	if(MT_IS_LOGIC())
		return sendMessage(UI_GlobalAttributes::instance().messageSetup(id));
	else
		uiStreamGraph2LogicCommand.set(fCommandUI_SendMessage) << &UI_GlobalAttributes::instance().messageSetup(id);
	return true;
}

bool UI_Dispatcher::sendMessage(const UI_MessageSetup& message_setup, const char* custom_text)
{
	start_timer_auto();

	if(message_setup.isEmpty() || (!message_setup.hasText() && !message_setup.hasVoice()))
		return false;

	if(MT_IS_LOGIC()){
		MessageQueue::iterator it = std::find(messageQueue_.begin(), messageQueue_.end(), message_setup);
		if(it != messageQueue_.end()){
			it->setCustomText(custom_text);
			it->start();
			return true;
		}

		messageQueue_.push_back(UI_Message(message_setup, custom_text));
	}
	else {
		if(!custom_text) custom_text = "";
		uiStreamGraph2LogicCommand.set(fCommandUI_SendMessage) << &message_setup << XBuffer((void*)custom_text, strlen(custom_text));
	}

	return true;
}

bool UI_Dispatcher::removeMessage(const UI_MessageSetup& message_setup, bool independent)
{
	start_timer_auto();

	if(message_setup.isEmpty() || (!message_setup.hasText() && !message_setup.hasVoice()))
		return false;

	if(MT_IS_LOGIC()){
		MessageQueue::iterator it = std::find(messageQueue_.begin(), messageQueue_.end(), message_setup);
		if(it != messageQueue_.end()){
			if(((*it).messageSetup().isVoiceInterruptable() || independent) && (*it).messageSetup().hasVoice()){
				if(voiceManager.validatePlayingFile((*it).messageSetup().text()->voice())){
					(*it).messageSetup().stopVoice();
					voiceLockTimer_.stop();
				}
			}
			messageQueue_.erase(it);
			return true;
		}
	}
	else
		uiStreamGraph2LogicCommand.set(fCommandUI_RemoveMessage) << &message_setup << independent;

	
	return true;
}

bool UI_Dispatcher::releaseResources()
{
	start_timer_auto();

	UI_LogicDispatcher::instance().releaseResources();

	minimap().releaseMapTexture();

	UI_Render::instance().releaseDefaultFont();

	for(UI_FontLibrary::Strings::const_iterator itf = UI_FontLibrary::instance().strings().begin(); itf != UI_FontLibrary::instance().strings().end(); ++itf)
		if(itf->get())
			itf->get()->releaseFont();

	for(UI_TextureLibrary::Strings::const_iterator it = UI_TextureLibrary::instance().strings().begin(); it != UI_TextureLibrary::instance().strings().end(); ++it)
		if(it->get())
			it->get()->releaseTexture();

	streamVideo().release();

	finitBgScene();
	return true;
}

void UI_Dispatcher::preloadScreen(UI_Screen* scr, cScene* scene, const Player* player)
{
	if(!scr)
		return;

	start_timer_auto();

	if(scr->isActive()){
		xxassert(false, XBuffer() < "������� ������������ ��������� ������");
		return;
	}
	
	if(preloadedScreen_){
		if(preloadedScreen_ == scr)
			return;
		xxassert(false, XBuffer() < "������������ ������ " < scr->name() < " �� ������������� ��� ���������������� " < preloadedScreen_->name()); 
		preloadedScreen_->release();
	}

	scr->preLoad();
	scr->preloadBgScene(scene, player);

	preloadedScreen_ = scr;
}

void fCommandUI_SelectScreen(void *ptr)
{
	UI_Dispatcher::instance().selectScreen(*(UI_Screen**)(ptr));
}

void UI_Dispatcher::selectScreen(UI_Screen* scr)
{
	if(MT_IS_LOGIC()){
		if(scr == currentScreen_)
			return;

		start_timer_auto();

		if(currentScreen_){
			currentScreen_->deactivate();

			needChangeScreen_ = true;
			if(scr)
				nextScreen_.init(scr);
			else {
				nextScreen_.clear();
				UI_BackgroundScene::instance().selectModel(0, false);
			}
		}
		else {
			setCurrentScreenLogic(scr);
		}
	}
	else
		uiStreamGraph2LogicCommand.set(fCommandUI_SelectScreen) << (void*)scr;
}

float UI_Dispatcher::getSelectScreenTime(const UI_Screen* scr) const
{
	start_timer_auto();

	if(scr == currentScreen_)
		return 0.f;
	return (currentScreen_ ? currentScreen_->deactivationTime() : 0.f)
		+ (scr ? scr->activationTime() : 0.f);
}


void UI_Dispatcher::convertInputEvent(UI_InputEvent& event)
{
	int fullkey = event.keyCode();

	event.setCursorPos(UI_Render::instance().device2relativeCoords(event.cursorPos()));

	switch(event.ID())
	{
	case UI_INPUT_MOUSE_MOVE:
		break;
	case UI_INPUT_MOUSE_LBUTTON_DOWN:
		fullkey = sKey(VK_LBUTTON, true).fullkey;
		break;
	case UI_INPUT_MOUSE_RBUTTON_DOWN:
		fullkey = sKey(VK_RBUTTON, true).fullkey;
		break;
	case UI_INPUT_MOUSE_MBUTTON_DOWN:
		fullkey = sKey(VK_MBUTTON, true).fullkey;
		break;
	case UI_INPUT_MOUSE_LBUTTON_UP:

		break;
	case UI_INPUT_MOUSE_RBUTTON_UP:

		break;
	case UI_INPUT_MOUSE_MBUTTON_UP:

		break;
	case UI_INPUT_MOUSE_LBUTTON_DBLCLICK:
		fullkey = sKey(VK_LDBL, true).fullkey;
		break;
	case UI_INPUT_MOUSE_RBUTTON_DBLCLICK:
		fullkey = sKey(VK_RDBL, true).fullkey;
		break;
	case UI_INPUT_MOUSE_MBUTTON_DBLCLICK:

		break;
	case UI_INPUT_MOUSE_WHEEL_UP:
		fullkey = sKey(VK_WHEELUP, true).fullkey;
		break;
	case UI_INPUT_MOUSE_WHEEL_DOWN:
		fullkey = sKey(VK_WHEELDN, true).fullkey;
		break;
	case UI_INPUT_KEY_DOWN:

		break;
	case UI_INPUT_KEY_UP:

		break;
	case UI_INPUT_CHAR:

		break;
	}
	
	event.setKeyCode(fullkey);

	if(isAltPressed())
		event.setFlags(event.flags() | MK_MENU);
}

bool UI_Dispatcher::handleInput(const UI_InputEvent& _event)
{
	//MTG();
	start_timer_auto();

	UI_InputEvent event = _event;
	convertInputEvent(event);

	UI_LogicDispatcher::instance().updateInput(event);

	bool ret = false;

	if(isEnabled_){
		if(event.isMouseClickEvent())
			if(UI_ControlBase* fc = focusedControl_)
				if(!fc->hitTest(event.cursorPos()))
					setFocus(0);

		if(isModalMessageMode())
			ret = messageScreen_->handleInput(event, focusedControl_);
		else if(graphCurrentScreen_ && UI_LogicDispatcher::instance().isInputEnabled())
			ret = graphCurrentScreen_->handleInput(event, focusedControl_);

		if(!ret)
			ret = UI_LogicDispatcher::instance().handleInput(event);
	}

	return ret;
}

void UI_Dispatcher::handleMessage(const ControlMessage& msg)
{
	if(isModalMessageMode())
		messageScreen_->handleMessage(msg);
	else if(MT_IS_GRAPH()){
		if(graphCurrentScreen_)
			graphCurrentScreen_->handleMessage(msg);
	}
	else
		if(currentScreen_)
			currentScreen_->handleMessage(msg);
}

UI_Screen* UI_Dispatcher::screen(const char* screen_name)
{
	for(ScreenContainer::iterator it = screens_.begin(); it != screens_.end(); ++it){
		if(!strcmp(it->name(), screen_name))
			return &*it;
	}

	return 0;
}

UI_Screen* UI_Dispatcher::screen(UI_Screen::ScreenType type_)
{
	for(ScreenContainer::iterator it = screens_.begin(); it != screens_.end(); ++it)
		if(it->type() == type_)
			return &*it;
	return 0;
}

bool UI_Dispatcher::addScreen (const char* name)
{
    UI_Screen newScreen;
	newScreen.setName (name);
	screens_.push_back (newScreen);
	init ();
	screenComboListUpdate();
	return true;
}

struct IsContainerWithName {
	IsContainerWithName (const char* name) {
		name_ = name;
	}
	bool operator() (UI_ControlContainer& container) const {
		return (container.hasName () && name_ == container.name ());
	}
	std::string name_;
};

bool UI_Dispatcher::removeScreen (const char* name)
{
	ScreenContainer::iterator it = std::find_if (screens_.begin (), screens_.end (), 
												 IsContainerWithName (name));
	if (it != screens_.end ()) {
		screens_.erase (it);
		screenComboListUpdate();
		return true;
	} else {
		return false;
	}
}

void UI_Dispatcher::updateControlOwners()
{
	for(ScreenContainer::iterator it = screens_.begin(); it != screens_.end(); ++it)
		it->updateControlOwners();
}

bool UI_Dispatcher::controlComboListBuild(std::string& list_string, UI_ControlFilterFunc filter) const
{
	list_string.clear();

	for(ScreenContainer::const_iterator it = screens_.begin(); it != screens_.end(); ++it)
		it->controlComboListBuild(list_string, filter);

	return true;
}

void UI_Dispatcher::screenComboListUpdate()
{
	screenComboList_.clear();

	for(ScreenContainer::iterator it = screens_.begin(); it != screens_.end(); ++it){
		if(it->hasName()){
			screenComboList_ += "|";
			screenComboList_ += it->name();
		}
	}
}

void UI_Dispatcher::parseMarkScreens()
{
	messageScreen_ = screen(UI_Screen::MESSAGE);
}


void UI_Dispatcher::initBgScene()
{
	UI_BackgroundScene::instance().init(UI_Render::instance().visGeneric());
}

void UI_Dispatcher::finitBgScene()
{
	UI_BackgroundScene::instance().done();
}


void UI_Dispatcher::setCurrentScreenGraph(UI_Screen* screen)
{
	MTG();
	if(graphCurrentScreen_ != screen){
		xassert(!preloadedScreen_ || preloadedScreen_ == screen);
		preloadedScreen_ = 0;
		if(graphCurrentScreen_)
			graphCurrentScreen_->release();
		graphCurrentScreen_ = screen;
	}
}

void fCommandSetCurrentScreenGraph(void* data)
{
	UI_Screen** scr = (UI_Screen**)data;
	UI_Dispatcher::instance().setCurrentScreenGraph(*scr);
}

void UI_Dispatcher::setCurrentScreenLogic(UI_Screen* screen)
{
	MTL();

	start_timer_auto();

	if(currentScreen_ == screen)
		return;

	currentScreen_ = screen;
	needChangeScreen_ = false;	

	if(currentScreen_){
		if(currentScreen_->type() == UI_Screen::GAME_INTERFACE)
			needResetScreens_ = true;
		currentScreen_->init();
		currentScreen_->logicQuant();
		currentScreen_->activate();
		currentScreen_->logicQuant();
	}

	uiStreamCommand.set(fCommandSetCurrentScreenGraph);
	uiStreamCommand << (void*)currentScreen_;
}

void UI_Dispatcher::setLoadingScreen()
{
	MTL();
	MTG();
	UI_LogicDispatcher::instance().setLoadProgress(0.f);
	setCurrentScreenLogic(screen(UI_Screen::LOADING));
	setCurrentScreenGraph(screen(UI_Screen::LOADING));
}

bool UI_Dispatcher::playVoice(const UI_MessageSetup& setup)
{
	bool interrupt = true;
	if(voiceManager.isPlaying()){
		MessageQueue::const_iterator i;
		FOR_EACH(messageQueue_, i)
			if((*i).messageSetup().text() && (*i).messageSetup().text()->hasVoice() && 
				voiceManager.validatePlayingFile((*i).messageSetup().text()->voice())){
					interrupt = (*i).messageSetup().isVoiceInterruptable() && setup.isCanInterruptVoice();
					break;
				}
	}
	if((interrupt || !voiceLockTimer_()) && setup.playVoice(interrupt)){
		if(!setup.isVoiceInterruptable())
			voiceLockTimer_.start(round(setup.voiceDuration() * 1000.f));
		return true;
	}

	return false;
}

bool UI_Dispatcher::isIngameHotKey(const sKey& key) const
{
	KeyList::const_iterator it = std::find(ingameHotKeyList_.begin(), ingameHotKeyList_.end(), key);
	return it != ingameHotKeyList_.end();
}

void UI_Dispatcher::setEnabled(bool isEnabled) 
{
	isEnabled_ = isEnabled;
	if(universe())
		universe()->setInterfaceEnabled(isEnabled);
}

UI_GlobalAttributes::UI_GlobalAttributes()
{
	cursors_.resize(UI_CURSOR_LAST_ENUM);
	chatDelay_ = 5.f;
	messageSetups_.resize(UI_MESSAGE_ID_MAX);
	privateMessage_ = sColor4f(0, 1, 0, 1); 
	systemMessage_ = sColor4f(0, 1, 0, 1);
}

void UI_GlobalAttributes::setCursor(UI_CursorType cur_type, const UI_Cursor* cursor)
{
	xassert(cur_type >= 0 && cur_type < UI_CURSOR_LAST_ENUM);
	cursors_[cur_type] = cursor;
}

const UI_Cursor* UI_GlobalAttributes::cursor(UI_CursorType cur_type) const
{
	xassert(cur_type >= 0 && cur_type < UI_CURSOR_LAST_ENUM);
	return cursors_[cur_type];
}

const UI_Cursor* UI_GlobalAttributes::getMoveCursor(int dir){
	switch(dir){
		case 1:
			return cursor(UI_CURSOR_SCROLL_UP);
		case 2:
			return cursor(UI_CURSOR_SCROLL_LEFT);
		case 3:
			return cursor(UI_CURSOR_SCROLL_UPLEFT);
		case 4:
			return cursor(UI_CURSOR_SCROLL_RIGHT);
		case 5:
			return cursor(UI_CURSOR_SCROLL_UPRIGHT);
		case 8:
			return cursor(UI_CURSOR_SCROLL_BOTTOM);
		case 10:
			return cursor(UI_CURSOR_SCROLL_BOTTOMLEFT);
		case 12:
			return cursor(UI_CURSOR_SCROLL_BOTTOMRIGHT);
		default:
			return 0;
	}
}

void UI_GlobalAttributes::serialize(Archive& ar)
{
	UI_Task::serializeColors(ar);

	ar.serialize(privateMessage_, "privateMessage", "���� �������/��������� ���������");
	ar.serialize(systemMessage_, "systemMessage", "���� ���������� ���������");
	ar.serialize(chatDelay_, "chatMessageDelay", "����� ����������� �������� ���-��������� (���)");

	int i;
	ar.openBlock("cursors", "�������");
	for(i = 0; i < UI_CURSOR_LAST_ENUM; i++)
		ar.serialize(cursors_[i], getEnumName(UI_CursorType(i)), getEnumNameAlt(UI_CursorType(i)));
	ar.closeBlock();

	ar.openBlock("locTexts", "����� ������ ��� �����������");
	if(ar.isEdit())
		CommonLocText::instance().serialize(ar);
	ar.closeBlock();

	ar.openBlock("messageSetups", "���������");
	for(i = 0; i < UI_MESSAGE_ID_MAX; i++)
		ar.serialize(messageSetups_[i], getEnumName(UI_MessageID(i)), getEnumNameAlt(UI_MessageID(i)));
	ar.closeBlock();
}

BEGIN_ENUM_DESCRIPTOR(TripleBool, "TripleBool")
REGISTER_ENUM(UI_YES, "��")
REGISTER_ENUM(UI_NO, "���")
REGISTER_ENUM(UI_ANY, "�����")
END_ENUM_DESCRIPTOR(TripleBool)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_Screen, ScreenType, "UI_ScreenType")
REGISTER_ENUM_ENCLOSED(UI_Screen, ORDINARY, "�������")
REGISTER_ENUM_ENCLOSED(UI_Screen, GAME_INTERFACE, "������� ���������")
REGISTER_ENUM_ENCLOSED(UI_Screen, LOADING, "��������")
REGISTER_ENUM_ENCLOSED(UI_Screen, MESSAGE, "��������� ���������")
REGISTER_ENUM_ENCLOSED(UI_Screen, EXIT_AD, "������� ��� ������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_Screen, ScreenType)

BEGIN_ENUM_DESCRIPTOR(UI_ControlShowModeID, "UI_ControlShowModeID")
REGISTER_ENUM(UI_SHOW_NORMAL, "������� �����")
REGISTER_ENUM(UI_SHOW_HIGHLITED, "������ ����������")
REGISTER_ENUM(UI_SHOW_ACTIVATED, "������ ������������")
REGISTER_ENUM(UI_SHOW_DISABLED, "������ ���������")
END_ENUM_DESCRIPTOR(UI_ControlShowModeID)

BEGIN_ENUM_DESCRIPTOR(UI_TextAlign, "UI_TextAlign")
REGISTER_ENUM(UI_TEXT_ALIGN_LEFT, "�� ������ ����")
REGISTER_ENUM(UI_TEXT_ALIGN_CENTER, "�� ������")
REGISTER_ENUM(UI_TEXT_ALIGN_RIGHT, "�� ������� ����")
END_ENUM_DESCRIPTOR(UI_TextAlign)

BEGIN_ENUM_DESCRIPTOR(UI_TextVAlign, "UI_TextVAlign")
REGISTER_ENUM(UI_TEXT_VALIGN_TOP, "�� �������� ����")
REGISTER_ENUM(UI_TEXT_VALIGN_CENTER, "�� ������")
REGISTER_ENUM(UI_TEXT_VALIGN_BOTTOM, "�� ������� ����")
END_ENUM_DESCRIPTOR(UI_TextVAlign)

BEGIN_ENUM_DESCRIPTOR(UI_BlendMode, "UI_BlendMode")
REGISTER_ENUM(UI_BLEND_NORMAL, "�������")
REGISTER_ENUM(UI_BLEND_ADD, "��������")
END_ENUM_DESCRIPTOR(UI_BlendMode)

BEGIN_ENUM_DESCRIPTOR(UI_SliderOrientation, "UI_SliderOrientation")
REGISTER_ENUM(UI_SLIDER_HORIZONTAL, "�� �����������")
REGISTER_ENUM(UI_SLIDER_VERTICAL, "�� ���������")
END_ENUM_DESCRIPTOR(UI_SliderOrientation)

BEGIN_ENUM_DESCRIPTOR(UI_ControlCustomType, "UI_ControlCustomType")
REGISTER_ENUM(UI_CUSTOM_CONTROL_MINIMAP, "���������")
REGISTER_ENUM(UI_CUSTOM_CONTROL_HEAD, "��������� �� ������")
REGISTER_ENUM(UI_CUSTOM_CONTROL_SELECTION, "��������� ����")
END_ENUM_DESCRIPTOR(UI_ControlCustomType)

BEGIN_ENUM_DESCRIPTOR(UI_ControlUnitListType, "UI_ControlUnitListType")
REGISTER_ENUM(UI_UNITLIST_SELECTED, "���������")
REGISTER_ENUM(UI_UNITLIST_PRODUCTION, "������� ������������")
REGISTER_ENUM(UI_UNITLIST_TRANSPORT, "���������")
REGISTER_ENUM(UI_UNITLIST_PRODUCTION_SQUAD, "������������ � �����")
END_ENUM_DESCRIPTOR(UI_ControlUnitListType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlEdit, CharType, "CharType")
REGISTER_ENUM_ENCLOSED(UI_ControlEdit, ALNUM, "���. ������� � �����")
REGISTER_ENUM_ENCLOSED(UI_ControlEdit, LATIN, "��������� ����� � �����")
REGISTER_ENUM_ENCLOSED(UI_ControlEdit, ALLCHAR, "��� �������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlEdit, CharType)

BEGIN_ENUM_DESCRIPTOR(UI_MinimapSymbolType, "UI_MinimapSymbolType")
REGISTER_ENUM(UI_MINIMAP_SYMBOLTYPE_NONE, "�� ����������")
REGISTER_ENUM(UI_MINIMAP_SYMBOLTYPE_DEFAULT, "�� ��������� ��� �����")
REGISTER_ENUM(UI_MINIMAP_SYMBOLTYPE_SELF, "����������� ���������")
END_ENUM_DESCRIPTOR(UI_MinimapSymbolType)

BEGIN_ENUM_DESCRIPTOR(UI_MinimapSymbolID, "UI_MinimapSymbolID")
REGISTER_ENUM(UI_MINIMAP_SYMBOL_DEFAULT, "������ �� ���������")
REGISTER_ENUM(UI_MINIMAP_SYMBOL_UNIT, "����")
REGISTER_ENUM(UI_MINIMAP_SYMBOL_UDER_ATTACK, "���� �������")
REGISTER_ENUM(UI_MINIMAP_SYMBOL_CLAN_UDER_ATTACK, "��� ���� �������")
REGISTER_ENUM(UI_MINIMAP_SYMBOL_ADD_UNIT, "���� ��������")
REGISTER_ENUM(UI_MINIMAP_SYMBOL_BUILD_FINISH, "������ ���������")
REGISTER_ENUM(UI_MINIMAP_SYMBOL_UPGRAGE_FINISH, "������� ��������")
REGISTER_ENUM(UI_MINIMAP_SYMBOL_ACTION_CLICK, "����� �������� ��������� �� ���������")
END_ENUM_DESCRIPTOR(UI_MinimapSymbolID)

BEGIN_ENUM_DESCRIPTOR(UI_CursorType, "��� �������")
REGISTER_ENUM(UI_CURSOR_PASSABLE, "���� ������������")
REGISTER_ENUM(UI_CURSOR_IMPASSABLE, "���� ��������������")
REGISTER_ENUM(UI_CURSOR_WATER, "����")
REGISTER_ENUM(UI_CURSOR_PLAYER_OBJECT, "������ ������")
REGISTER_ENUM(UI_CURSOR_ALLY_OBJECT, "������ ��������")
REGISTER_ENUM(UI_CURSOR_ENEMY_OBJECT, "������ �����")
REGISTER_ENUM(UI_CURSOR_ITEM_OBJECT, "�������")
REGISTER_ENUM(UI_CURSOR_ITEM_CAN_PIC, "������� ����� �����")
REGISTER_ENUM(UI_CURSOR_ITEM_EXTRACT, "����� ������ ������")
REGISTER_ENUM(UI_CURSOR_INTERFACE, "���������")
REGISTER_ENUM(UI_CURSOR_MAIN_MENU, "������� ����")
REGISTER_ENUM(UI_CURSOR_WAITING, "����� ������")
REGISTER_ENUM(UI_CURSOR_MOUSE_LBUTTON_DOWN, "����� ������ ���� ������")
REGISTER_ENUM(UI_CURSOR_MOUSE_RBUTTON_DOWN, "������ ������ ���� ������")
REGISTER_ENUM(UI_CURSOR_WALK, "����")
REGISTER_ENUM(UI_CURSOR_WALK_DISABLED, "������ ����")
REGISTER_ENUM(UI_CURSOR_PATROL, "��������� ����� ��������������")
REGISTER_ENUM(UI_CURSOR_PATROL_DISABLED, "��������� ����� �������������� ���� ������")
REGISTER_ENUM(UI_CURSOR_ATTACK, "��������� �����")
REGISTER_ENUM(UI_CURSOR_FRIEND_ATTACK, "��������� ������")
REGISTER_ENUM(UI_CURSOR_ATTACK_DISABLED, "������ ���������")
REGISTER_ENUM(UI_CURSOR_PLAYER_CONTROL_DISABLED, "���������� ���������")
REGISTER_ENUM(UI_CURSOR_WORLD_OBJECT, "������ ����")
REGISTER_ENUM(UI_CURSOR_SCROLL_UP, "������ ����� �����")
REGISTER_ENUM(UI_CURSOR_SCROLL_LEFT, "������ ����� �����")
REGISTER_ENUM(UI_CURSOR_SCROLL_UPLEFT, "������ ����� �����-�����")
REGISTER_ENUM(UI_CURSOR_SCROLL_RIGHT, "������ ����� ������")
REGISTER_ENUM(UI_CURSOR_SCROLL_UPRIGHT, "������ ����� �����-������")
REGISTER_ENUM(UI_CURSOR_SCROLL_BOTTOM, "������ ����� ����")
REGISTER_ENUM(UI_CURSOR_SCROLL_BOTTOMLEFT, "������ ����� ����-�����")
REGISTER_ENUM(UI_CURSOR_SCROLL_BOTTOMRIGHT, "������ ����� ����-������")
REGISTER_ENUM(UI_CURSOR_ROTATE, "�������� �����")
REGISTER_ENUM(UI_CURSOR_ROTATE_DIRECT_CONTROL, "�������� ����� ��� ������ ����������")
REGISTER_ENUM(UI_CURSOR_DIRECT_CONTROL_ATTACK, "��������� ������ ��� ������ ����������")
REGISTER_ENUM(UI_CURSOR_DIRECT_CONTROL, "������ ��� ������ ����������")
REGISTER_ENUM(UI_CURSOR_TRANSPORT, "������� � ���������")
REGISTER_ENUM(UI_CURSOR_CAN_BUILD, "�������/�����������")
REGISTER_ENUM(UI_CURSOR_TELEPORT, "������������")
END_ENUM_DESCRIPTOR(UI_CursorType)

BEGIN_ENUM_DESCRIPTOR(UI_ControlActionID, "UI_ControlActionID")
REGISTER_ENUM(UI_ACTION_NONE, "���")
REGISTER_ENUM(UI_ACTION_INVERT_SHOW_PRIORITY, "��������\\��� �������� ������ ��������� (�)")
REGISTER_ENUM(UI_ACTION_EXPAND_TEMPLATE, "���������� �����������\\�������� �������")
REGISTER_ENUM(UI_ACTION_EXTERNAL_CONTROL, "���������� �����������\\���������� ������ �������")
REGISTER_ENUM(UI_ACTION_STATE_MARK, "��������\\������� ��������� ������")
REGISTER_ENUM(UI_ACTION_GLOBAL_VARIABLE, "��������\\�����������/��������� ���������� ����������")
REGISTER_ENUM(UI_ACTION_CHANGE_STATE, "���������� �����������\\����������� ���������")
REGISTER_ENUM(UI_ACTION_AUTOCHANGE_STATE, "���������� �����������\\��������� ���������")
REGISTER_ENUM(UI_ACTION_ANIMATION_CONTROL, "���������� �����������\\���������� ���������")
REGISTER_ENUM(UI_ACTION_CLICK_FOR_TRIGGER, "���������� �����������\\�������� ���� � �������")
REGISTER_ENUM(UI_ACTION_POST_EFFECT, "���������� �����������\\��������/��������� ����-������")
REGISTER_ENUM(UI_ACTION_PROFILES_LIST, "�������\\������ ���������")
REGISTER_ENUM(UI_ACTION_ONLINE_LOGIN_LIST, "���� � ���������\\������ �������")
REGISTER_ENUM(UI_ACTION_PROFILE_INPUT, "�������\\���� ����� ������ ��������")
REGISTER_ENUM(UI_ACTION_CDKEY_INPUT, "�������\\���� CD-Key")
REGISTER_ENUM(UI_ACTION_PROFILE_CREATE, "�������\\������� ����� �������")
REGISTER_ENUM(UI_ACTION_PROFILE_DELETE, "�������\\������� ������� �������")
REGISTER_ENUM(UI_ACTION_PROFILE_SELECT, "�������\\������� �������")
REGISTER_ENUM(UI_ACTION_DELETE_ONLINE_LOGIN_FROM_LIST, "���� � ���������\\������� ������ ����� �� ������")
REGISTER_ENUM(UI_ACTION_PROFILE_CURRENT, "�������\\������� �������")
REGISTER_ENUM(UI_ACTION_MISSION_LIST, "��������, ���������� � ��������� ����\\������ ������ ������")
REGISTER_ENUM(UI_ACTION_LAN_USE_MAP_SETTINGS, "��������, ���������� � ��������� ����\\������������ ����������������� ���������")
REGISTER_ENUM(UI_ACTION_LAN_GAME_TYPE, "��������, ���������� � ��������� ����\\������� ��� ������� ����")
REGISTER_ENUM(UI_ACTION_LAN_GAME_LIST, "������� ����\\������ ������� ���")
REGISTER_ENUM(UI_ACTION_LAN_CHAT_USER_LIST, "������� ����\\������ ������� � ����")
REGISTER_ENUM(UI_ACTION_LAN_DISCONNECT_SERVER, "������� ����\\��������� ������ ������� ����")
REGISTER_ENUM(UI_ACTION_LAN_ABORT_OPERATION, "������� ����\\�������� ������� ��������")
REGISTER_ENUM(UI_ACTION_LAN_GAME_NAME_INPUT, "������� ����\\���� �������� ������� ����")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_READY, "������� ����\\���� ����������")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_JOIN_TEAM, "������� ����\\������������ � �������")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_LEAVE_TEAM, "������� ����\\�����������/������� �� �������")
REGISTER_ENUM(UI_ACTION_BIND_PLAYER, "��������, ���������� � ��������� ����\\�������� � ������")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_NAME, "��������, ���������� � ��������� ����\\��� ������")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_STATISTIC_NAME, "��������, ���������� � ��������� ����\\��� ������/��������� ��� ����������")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_COLOR, "��������, ���������� � ��������� ����\\���� ������")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_SIGN, "��������, ���������� � ��������� ����\\������� ������")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_RACE, "��������, ���������� � ��������� ����\\���� ������")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_DIFFICULTY, "��������, ���������� � ��������� ����\\���� ��������� ������")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_CLAN, "��������, ���������� � ��������� ����\\���� ������")
REGISTER_ENUM(UI_ACTION_LAN_PLAYER_TYPE, "��������, ���������� � ��������� ����\\��� ������")
REGISTER_ENUM(UI_ACTION_SAVE_GAME_NAME_INPUT, "��������, ���������� � ��������� ����\\���� �������� ���� ��� ����������")
REGISTER_ENUM(UI_ACTION_GAME_SAVE, "��������, ���������� � ��������� ����\\��������� ����")
REGISTER_ENUM(UI_ACTION_DELETE_SAVE, "��������, ���������� � ��������� ����\\������� save")
REGISTER_ENUM(UI_ACTION_GAME_START, "��������, ���������� � ��������� ����\\�������� ����")
REGISTER_ENUM(UI_ACTION_GAME_RESTART, "��������, ���������� � ��������� ����\\������������� ������� ����")
REGISTER_ENUM(UI_ACTION_REPLAY_NAME_INPUT, "��������, ���������� � ��������� ����\\���� �������� ������")
REGISTER_ENUM(UI_ACTION_REPLAY_SAVE, "��������, ���������� � ��������� ����\\��������� ������")
REGISTER_ENUM(UI_ACTION_RESET_MISSION, "��������, ���������� � ��������� ����\\�������� ������� ��������� ������")
REGISTER_ENUM(UI_ACTION_MISSION_DESCRIPTION, "��������, ���������� � ��������� ����\\�������� ������")
REGISTER_ENUM(UI_ACTION_PLAYER_PARAMETER, "����� ��������\\����� ��������� ������")
REGISTER_ENUM(UI_ACTION_UNIT_PARAMETER, "����� ��������\\����� ��������� �������� �����")
REGISTER_ENUM(UI_ACTION_PLAYER_UNITS_COUNT, "����� ��������\\����� ���������� ������ ������")
REGISTER_ENUM(UI_ACTION_PLAYER_STATISTICS, "����� ��������\\����� ���������� �� �������")
REGISTER_ENUM(UI_ACTION_UNIT_HINT, "����� ��������\\����� ��������� � �����")
REGISTER_ENUM(UI_ACTION_UI_HINT, "����� ��������\\����� ��������� � ��������")
REGISTER_ENUM(UI_ACTION_PAUSE_GAME, "�������\\��������/��������� �����")
REGISTER_ENUM(UI_ACTION_CLICK_MODE, "�������\\����� ����� �� ���")
REGISTER_ENUM(UI_ACTION_ACTIVATE_WEAPON, "�������\\�������� ������")
REGISTER_ENUM(UI_ACTION_CANCEL, "�������\\������ ��������")
REGISTER_ENUM(UI_ACTION_BIND_TO_UNIT, "��������\\� ����������� �����")
REGISTER_ENUM(UI_ACTION_UNIT_ON_MAP, "��������\\c��������� �� ����")
REGISTER_ENUM(UI_ACTION_UNIT_IN_TRANSPORT, "��������\\���� � ����������")
REGISTER_ENUM(UI_ACTION_BINDEX, "��������\\����������� �������� � �����")
REGISTER_ENUM(UI_ACTION_BIND_TO_UNIT_STATE, "��������\\��������� �������� �����")
REGISTER_ENUM(UI_ACTION_UNIT_HAVE_PARAMS, "��������\\���������� ���������� � �����")
REGISTER_ENUM(UI_ACTION_BIND_TO_IDLE_UNITS, "�������\\����� � �������� �����������")
REGISTER_ENUM(UI_ACTION_BIND_GAME_TYPE, "��������\\�������� � ���� ����")
REGISTER_ENUM(UI_ACTION_BIND_GAME_LOADED, "��������\\��������: ���� ���������")
REGISTER_ENUM(UI_ACTION_BIND_ERROR_TYPE, "��������\\�������� � �������� �������")
REGISTER_ENUM(UI_ACTION_BIND_NET_PAUSE, "��������\\�������� � ������� �����")
REGISTER_ENUM(UI_ACTION_BIND_GAME_PAUSE, "��������\\�������� � �����")
REGISTER_ENUM(UI_ACTION_NET_PAUSED_PLAYER_LIST, "����� ��������\\��� �������� ���� �� �����")
REGISTER_ENUM(UI_ACTION_BIND_NEED_COMMIT_SETTINGS, "�����\\�������� � ������������� ������������� ��������")
REGISTER_ENUM(UI_ACTION_OPTION_PRESET_LIST, "�����\\������ ��������")
REGISTER_ENUM(UI_ACTION_OPTION_APPLY, "�����\\��������� ���������")
REGISTER_ENUM(UI_ACTION_OPTION_CANCEL, "�����\\�������� ���������")
REGISTER_ENUM(UI_ACTION_COMMIT_GAME_SETTINGS, "�����\\����������� ����� ���������")
REGISTER_ENUM(UI_ACTION_ROLLBACK_GAME_SETTINGS, "�����\\�������� ����� ���������")
REGISTER_ENUM(UI_ACTION_NEED_COMMIT_TIME_AMOUNT, "�����\\������� ����������� ������� ��� ������������� ��������")
REGISTER_ENUM(UI_ACTION_BIND_SAVE_LIST, "��������\\�������� � ������� ����������� ���/�������")
REGISTER_ENUM(UI_ACTION_UNIT_COMMAND, "�������\\������� ���������� �����/������")
REGISTER_ENUM(UI_ACTION_BUILDING_INSTALLATION, "�������\\��������� ������")
REGISTER_ENUM(UI_ACTION_BUILDING_CAN_INSTALL, "��������\\����� ��������� ������")
REGISTER_ENUM(UI_ACTION_UNIT_SELECT, "�������\\������ �����/������")
REGISTER_ENUM(UI_ACTION_UNIT_DESELECT, "�������\\�������� �����/������ �� ������ ���������")
REGISTER_ENUM(UI_ACTION_SET_SELECTED, "�������\\������� ������� � �������")
REGISTER_ENUM(UI_ACTION_SELECTION_OPERATE, "�������\\������ �� �������� ��������")
REGISTER_ENUM(UI_ACTION_PRODUCTION_PROGRESS, "����� ��������\\����������� ��������� ������������")
REGISTER_ENUM(UI_ACTION_PRODUCTION_PARAMETER_PROGRESS, "����� ��������\\����������� ��������� ������������ ����������")
REGISTER_ENUM(UI_ACTION_RELOAD_PROGRESS, "����� ��������\\����������� ����������� ������")
REGISTER_ENUM(UI_ACTION_OPTION, "�����\\��������� ����� ����")
REGISTER_ENUM(UI_ACTION_SET_KEYS, "�����\\��������� ����������")
REGISTER_ENUM(UI_ACTION_LOADING_PROGRESS, "����� ��������\\����������� ��������� ��������")
REGISTER_ENUM(UI_ACTION_MERGE_SQUADS, "�������\\������� �������")
REGISTER_ENUM(UI_ACTION_SPLIT_SQUAD, "�������\\��������� ������")
REGISTER_ENUM(UI_BIND_PRODUCTION_QUEUE, "��������\\�� ��������� � ������� ������������")
REGISTER_ENUM(UI_ACTION_MESSAGE_LIST, "����� ��������\\����� ������� ���������")
REGISTER_ENUM(UI_ACTION_TASK_LIST, "����� ��������\\����� ������ �����")
REGISTER_ENUM(UI_ACTION_SOURCE_ON_MOUSE, "�������\\���������� ���������� �� ����")
REGISTER_ENUM(UI_ACTION_MINIMAP_ROTATION, "�������\\��������� �������� ���������")
REGISTER_ENUM(UI_ACTION_GET_MODAL_MESSAGE, "���� ���������\\����� ���������")
REGISTER_ENUM(UI_ACTION_OPERATE_MODAL_MESSAGE, "���� ���������\\���������� �����")
REGISTER_ENUM(UI_ACTION_SHOW_TIME, "����� ��������\\����� � ������ ����")
REGISTER_ENUM(UI_ACTION_SHOW_COUNT_DOWN_TIMER, "����� ��������\\������ ��������� �������")
REGISTER_ENUM(UI_ACTION_INET_CREATE_ACCOUNT, "���� � ���������\\������� �������")
REGISTER_ENUM(UI_ACTION_INET_CHANGE_PASSWORD, "���� � ���������\\������� ������")
REGISTER_ENUM(UI_ACTION_INET_LOGIN, "���� � ���������\\������������")
REGISTER_ENUM(UI_ACTION_LAN_CHAT_CHANNEL_LIST, "���� � ���������\\������ ������� ����")
REGISTER_ENUM(UI_ACTION_LAN_CHAT_CHANNEL_ENTER, "���� � ���������\\����� �� ��������� �����")
REGISTER_ENUM(UI_ACTION_INET_QUICK_START, "���� � ���������\\������� �����")
REGISTER_ENUM(UI_ACTION_INET_REFRESH_GAME_LIST, "���� � ���������\\�������� ������ ���")
REGISTER_ENUM(UI_ACTION_LAN_CREATE_GAME, "������� ����\\������� ������ ������� ����")
REGISTER_ENUM(UI_ACTION_LAN_JOIN_GAME, "������� ����\\�������������� � ��������� ����")
REGISTER_ENUM(UI_ACTION_INET_DELETE_ACCOUNT, "���� � ���������\\������� �������")
REGISTER_ENUM(UI_ACTION_INET_NAME, "���� � ���������\\�����")
REGISTER_ENUM(UI_ACTION_INET_PASS, "���� � ���������\\������")
REGISTER_ENUM(UI_ACTION_INET_PASS2, "���� � ���������\\������ ������/����� ������")
REGISTER_ENUM(UI_ACTION_MISSION_SELECT_FILTER, "���� � ���������\\������ �� ������� ��� �����")
REGISTER_ENUM(UI_ACTION_MISSION_QUICK_START_FILTER, "���� � ���������\\������ �� ������� ��� �������� ������")
REGISTER_ENUM(UI_ACTION_QUICK_START_FILTER_POPULATION, "���� � ���������\\������ �� ���� ���� ��� �������� ������")
REGISTER_ENUM(UI_ACTION_QUICK_START_FILTER_RACE, "���� � ���������\\������ �� ���� ��� �������� ������")
REGISTER_ENUM(UI_ACTION_INET_FILTER_PLAYERS_COUNT, "���� � ���������\\������ �� ���������� �������")
REGISTER_ENUM(UI_ACTION_INET_FILTER_GAME_TYPE, "���� � ���������\\������ �� ���� ����")
REGISTER_ENUM(UI_ACTION_CHAT_EDIT_STRING, "������� ����\\������ ��������� ����")
REGISTER_ENUM(UI_ACTION_CHAT_SEND_MESSAGE, "������� ����\\�������� ��������� � ����� ���")
REGISTER_ENUM(UI_ACTION_CHAT_SEND_CLAN_MESSAGE, "������� ����\\�������� ��������� � �������� ���")
REGISTER_ENUM(UI_ACTION_CHAT_MESSAGE_BOARD, "������� ����\\������ ��������� � ����")
REGISTER_ENUM(UI_ACTION_LAN_CHAT_CLEAR, "������� ����\\�������� ���")
REGISTER_ENUM(UI_ACTION_GAME_CHAT_BOARD, "������� ����\\����������� ��������� �������� ����")
REGISTER_ENUM(UI_ACTION_INET_STATISTIC_QUERY, "���� � ���������\\��������� ���������� ����������")
REGISTER_ENUM(UI_ACTION_INET_STATISTIC_SHOW, "���� � ���������\\����������� ���������� ����������")
REGISTER_ENUM(UI_ACTION_STATISTIC_FILTER_RACE, "���� � ���������\\������ �� ���� ��� ���������� ����������")
REGISTER_ENUM(UI_ACTION_STATISTIC_FILTER_POPULATION, "���� � ���������\\������ �� ���� ���� ��� ���������� ����������")
REGISTER_ENUM(UI_ACTION_DIRECT_CONTROL_CURSOR, "������ ����������\\������")
REGISTER_ENUM(UI_ACTION_DIRECT_CONTROL_WEAPON_LOAD, "������ ����������\\������� ������")
REGISTER_ENUM(UI_ACTION_DIRECT_CONTROL_TRANSPORT, "������ ����������\\������� � ���������/������� �� ����������")
REGISTER_ENUM(UI_ACTION_DIRECT_CONTROL_TRANSPORT, "������ ����������\\������� � ���������/������� �� ����������")
REGISTER_ENUM(UI_ACTION_CONFIRM_DISK_OP, "��������, ���������� � ��������� ����\\����������� ��� ��������� ���������� �����/�������/������")
END_ENUM_DESCRIPTOR(UI_ControlActionID)

BEGIN_ENUM_DESCRIPTOR(UI_ClickModeID, "UI_ClickModeID")
REGISTER_ENUM(UI_CLICK_MODE_NONE, "UI_CLICK_MODE_NONE")
REGISTER_ENUM(UI_CLICK_MODE_MOVE, "���� ��� �����������")
REGISTER_ENUM(UI_CLICK_MODE_ATTACK, "���� ��� �����")
REGISTER_ENUM(UI_CLICK_MODE_PATROL, "����� ��������������")
REGISTER_ENUM(UI_CLICK_MODE_REPAIR, "���� ��� �������")
REGISTER_ENUM(UI_CLICK_MODE_RESOURCE, "���� ��� ����� ��������")
END_ENUM_DESCRIPTOR(UI_ClickModeID)

BEGIN_ENUM_DESCRIPTOR(UI_ClickModeMarkID, "UI_ClickModeMarkID")
REGISTER_ENUM(UI_CLICK_MARK_MOVEMENT, "�����������")
REGISTER_ENUM(UI_CLICK_MARK_ATTACK, "�����")
REGISTER_ENUM(UI_CLICK_MARK_REPAIR, "������")
REGISTER_ENUM(UI_CLICK_MARK_MOVEMENT_WATER, "����������� �� ����")
REGISTER_ENUM(UI_CLICK_MARK_ATTACK_UNIT, "����� �����")
REGISTER_ENUM(UI_CLICK_MARK_PATROL, "��������������")
END_ENUM_DESCRIPTOR(UI_ClickModeMarkID)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataBindGameType, UI_BindGameType, "UI_ActionDataBindLanType")
REGISTER_ENUM_ENCLOSED(UI_ActionDataBindGameType, UI_GT_NETGAME, "�������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataBindGameType, UI_GT_BATTLE, "����")
REGISTER_ENUM_ENCLOSED(UI_ActionDataBindGameType, UI_GT_SCENARIO, "�����������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataBindGameType, UI_BindGameType)

BEGIN_ENUM_DESCRIPTOR(ScenarioGameType, "ScenarioGameType")
REGISTER_ENUM(SCENARIO_GAME_TYPE_PREDEFINE, "Predefine")
REGISTER_ENUM(SCENARIO_GAME_TYPE_CUSTOM, "Custom")
REGISTER_ENUM(SCENARIO_GAME_TYPE_ANY,  "�����")
END_ENUM_DESCRIPTOR(ScenarioGameType)

BEGIN_ENUM_DESCRIPTOR(TeamGameType, "TeamGameType")
REGISTER_ENUM(TEAM_GAME_TYPE_INDIVIDUAL, "��� ������")
REGISTER_ENUM(TEAM_GAME_TYPE_TEEM, "��������� �����")
REGISTER_ENUM(TEAM_GAME_TYPE_ANY, "����� �����")
END_ENUM_DESCRIPTOR(TeamGameType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionTriggerVariable, UI_ActionTriggerVariableType, "UI_ActionTriggerVariableType")
REGISTER_ENUM_ENCLOSED(UI_ActionTriggerVariable, GLOBAL, "���������� ����������")
REGISTER_ENUM_ENCLOSED(UI_ActionTriggerVariable, MISSION_DESCRIPTION, "������������ ����������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionTriggerVariable, UI_ActionTriggerVariableType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionBindEx, UI_ActionBindExType, "UI_ActionBindExType")
REGISTER_ENUM_ENCLOSED(UI_ActionBindEx, UI_BIND_PRODUCTION, "������������")
REGISTER_ENUM_ENCLOSED(UI_ActionBindEx, UI_BIND_PRODUCTION_SQUAD, "������������ � �����")
REGISTER_ENUM_ENCLOSED(UI_ActionBindEx, UI_TRANSPORT, "���������")
REGISTER_ENUM_ENCLOSED(UI_ActionBindEx, UI_BIND_ANY, "�����")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionBindEx, UI_ActionBindExType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionBindEx, UI_ActionBindExSelSize, "UI_ActionBindExSelSize")
REGISTER_ENUM_ENCLOSED(UI_ActionBindEx, UI_SEL_ANY, "�����")
REGISTER_ENUM_ENCLOSED(UI_ActionBindEx, UI_SEL_SINGLE, "���������")
REGISTER_ENUM_ENCLOSED(UI_ActionBindEx, UI_SEL_MORE_ONE, "������ ������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionBindEx, UI_ActionBindExSelSize)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionBindEx, UI_ActionBindExQueueSize, "UI_ActionBindExQueueSize")
REGISTER_ENUM_ENCLOSED(UI_ActionBindEx, UI_QUEUE_ANY, "�����")
REGISTER_ENUM_ENCLOSED(UI_ActionBindEx, UI_QUEUE_EMPTY, "������")
REGISTER_ENUM_ENCLOSED(UI_ActionBindEx, UI_QUEUE_NOT_EMPTY, "�� ������") 
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionBindEx, UI_ActionBindExQueueSize)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionUnitState, UI_ActionUnitStateType, "UI_ActionUnitStateType")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_SELF_ATTACK, "����� �����")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_WEAPON_MODE, "��������� �����")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_AUTO_TARGET_FILTER, "����� ������ �����")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_WALK_ATTACK_MODE, "����� � ��������")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_RUN, "������/���")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_AUTO_FIND_TRANSPORT, "�������������� ����� ����������")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_CAN_DETONATE_MINES, "���� ���� ��� ���������")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_CAN_UPGRADE, "����� ������������")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_IS_UPGRADING, "������ �����������")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_CAN_BUILD, "����� ���������� �����")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_IS_BUILDING, "������ ���������� �����")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_CAN_PRODUCE_PARAMETER, "����� ���������� ��������")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_IS_IDLE, "���� ������ �� ������")
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_COMMON_OPERABLE, "����� � ���������� ����� ������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionUnitState, UI_ActionUnitStateType)

BEGIN_ENUM_DESCRIPTOR(UI_OptionType, "UI_OptionType")
REGISTER_ENUM(UI_OPTION_UPDATE, "���� �����")
REGISTER_ENUM(UI_OPTION_APPLY, "��������� ����")
REGISTER_ENUM(UI_OPTION_CANCEL, "�������� ����")
REGISTER_ENUM(UI_OPTION_DEFAULT, "�������� �� ���������")
END_ENUM_DESCRIPTOR(UI_OptionType)

BEGIN_ENUM_DESCRIPTOR(PlayControlAction, "PlayControlAction")
REGISTER_ENUM(PLAY_ACTION_PLAY, "Play")
REGISTER_ENUM(PLAY_ACTION_PAUSE, "Pause")
REGISTER_ENUM(PLAY_ACTION_STOP, "Stop")
REGISTER_ENUM(PLAY_ACTION_RESTART, "Restart")
END_ENUM_DESCRIPTOR(PlayControlAction)

BEGIN_ENUM_DESCRIPTOR(UI_MessageID, "UI_MessageID")
REGISTER_ENUM(UI_MESSAGE_NOT_ENOUGH_RESOURCES_FOR_BUILDING, "������������ �������� ��� �������������")
REGISTER_ENUM(UI_MESSAGE_NOT_ENOUGH_RESOURCES_FOR_SHOOTING, "������������ �������� ��� �������")
REGISTER_ENUM(UI_MESSAGE_UNIT_LIMIT_REACHED, "����� ���������� ������")
REGISTER_ENUM(UI_MESSAGE_TASK_ASSIGNED, "������ ���������")
REGISTER_ENUM(UI_MESSAGE_TASK_COMPLETED, "������ ���������")
REGISTER_ENUM(UI_MESSAGE_TASK_FAILED, "������ ���������")
END_ENUM_DESCRIPTOR(UI_MessageID)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitHint, UI_ActionDataUnitHintType, "UI_ActionDataUnitHintType")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitHint, FULL, "������ ��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitHint, SHORT, "������� ��������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitHint, UI_ActionDataUnitHintType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitHint, UI_ActionDataUnitHintUnitType, "UI_ActionDataUnitHintUnitType")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitHint, SELECTED, "��� �������������� �����")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitHint, HOVERED, "��� ����� ��� ��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitHint, CONTROL, "��� ����� �� ������� ������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitHint, UI_ActionDataUnitHintUnitType)

BEGIN_ENUM_DESCRIPTOR(UI_Align, "UI_Align")
REGISTER_ENUM(UI_ALIGN_CENTER, "�� ������")
REGISTER_ENUM(UI_ALIGN_LEFT, "�� ������ ����")
REGISTER_ENUM(UI_ALIGN_RIGHT, "�� ������� ����")
REGISTER_ENUM(UI_ALIGN_TOP, "�� ������� �������")
REGISTER_ENUM(UI_ALIGN_BOTTOM, "�� ������ �������")
REGISTER_ENUM(UI_ALIGN_TOP_LEFT, "����� ������� ����")
REGISTER_ENUM(UI_ALIGN_TOP_RIGHT, "������ ������� ����")
REGISTER_ENUM(UI_ALIGN_BOTTOM_LEFT, "����� ������ ����")
REGISTER_ENUM(UI_ALIGN_BOTTOM_RIGHT, "������ ������ ����")
END_ENUM_DESCRIPTOR(UI_Align)

BEGIN_ENUM_DESCRIPTOR(UI_TaskStateID, "UI_TaskStateID")
REGISTER_ENUM(UI_TASK_ASSIGNED, "���������")
REGISTER_ENUM(UI_TASK_COMPLETED, "���������")
REGISTER_ENUM(UI_TASK_FAILED, "���������")
REGISTER_ENUM(UI_TASK_DELETED, "�������")
END_ENUM_DESCRIPTOR(UI_TaskStateID)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitParameter, ParameterClass, "ParameterClass")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, LOGIC, "������ ��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, LEVEL, "������� �����")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitParameter, ParameterClass)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionPlayerStatistic, Type, "UI_ActionPlayerStatistic::Type")
REGISTER_ENUM_ENCLOSED(UI_ActionPlayerStatistic, LOCAL, "��������� ����������")
REGISTER_ENUM_ENCLOSED(UI_ActionPlayerStatistic, GLOBAL, "���������� (onLine)")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionPlayerStatistic, Type)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(AtomAction, AtomActionType, "AtomActionType")
REGISTER_ENUM_ENCLOSED(AtomAction, SHOW, "��������")
REGISTER_ENUM_ENCLOSED(AtomAction, HIDE, "��������")
REGISTER_ENUM_ENCLOSED(AtomAction, ENABLE_SHOW, "��������� �����")
REGISTER_ENUM_ENCLOSED(AtomAction, DISABLE_SHOW, "��������� �����")
REGISTER_ENUM_ENCLOSED(AtomAction, ENABLE, "������� ���������")
REGISTER_ENUM_ENCLOSED(AtomAction, DISABLE, "������� �����������")
REGISTER_ENUM_ENCLOSED(AtomAction, VIDEO_PLAY, "����� play")
REGISTER_ENUM_ENCLOSED(AtomAction, VIDEO_PAUSE, "����� pause")
REGISTER_ENUM_ENCLOSED(AtomAction, SET_FOCUS, "������� ��������")
END_ENUM_DESCRIPTOR_ENCLOSED(AtomAction, AtomActionType)

BEGIN_ENUM_DESCRIPTOR(StateMarkType, "StateMarkType")
REGISTER_ENUM(UI_STATE_MARK_NONE, "������")
REGISTER_ENUM(UI_STATE_MARK_UNIT_SELF_ATTACK, "����� �����")
REGISTER_ENUM(UI_STATE_MARK_UNIT_WEAPON_MODE, "��������� �����")
REGISTER_ENUM(UI_STATE_MARK_UNIT_WALK_ATTACK_MODE, "����� � ��������")
REGISTER_ENUM(UI_STATE_MARK_UNIT_AUTO_TARGET_FILTER, "����� ������ �����")
REGISTER_ENUM(UI_STATE_MARK_DIRECT_CONTROL_CURSOR, "������ � ������ ����������")
END_ENUM_DESCRIPTOR(StateMarkType)

BEGIN_ENUM_DESCRIPTOR(UI_DirectControlCursorType, "UI_DirectControlCursorType")
REGISTER_ENUM(UI_DIRECT_CONTROL_CURSOR_NONE, "������� ������")
REGISTER_ENUM(UI_DIRECT_CONTROL_CURSOR_ENEMY, "������ �� �����")
REGISTER_ENUM(UI_DIRECT_CONTROL_CURSOR_ALLY, "������ �� ������")
REGISTER_ENUM(UI_DIRECT_CONTROL_CURSOR_TRANSPORT, "������ �� ���������")
END_ENUM_DESCRIPTOR(UI_DirectControlCursorType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataModalMessage, Action, "UI_ActionDataModalMessageAction")
REGISTER_ENUM_ENCLOSED(UI_ActionDataModalMessage, CLOSE, "�������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataModalMessage, CLEAR, "��������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataModalMessage, Action)

BEGIN_ENUM_DESCRIPTOR(UI_UserSelectEvent, "UI_UserMouseEvent")
REGISTER_ENUM(UI_MOUSE_LBUTTON_DBLCLICK, "������� ����")
REGISTER_ENUM(UI_MOUSE_LBUTTON_DOWN, "����� ����")
REGISTER_ENUM(UI_MOUSE_RBUTTON_DOWN, "������ ����")
REGISTER_ENUM(UI_MOUSE_MBUTTON_DOWN, "������� ����")
REGISTER_ENUM(UI_USER_ACTION_HOTKEY, "������� �������")
END_ENUM_DESCRIPTOR(UI_UserSelectEvent)

BEGIN_ENUM_DESCRIPTOR(UI_UserEventMouseModifers, "UI_UserMouseEventModifers")
REGISTER_ENUM(UI_MOUSE_MODIFER_SHIFT, "Shift")
REGISTER_ENUM(UI_MOUSE_MODIFER_CTRL, "Control")
REGISTER_ENUM(UI_MOUSE_MODIFER_ALT, "Alt")
END_ENUM_DESCRIPTOR(UI_UserEventMouseModifers)

BEGIN_ENUM_DESCRIPTOR(UI_NetStatus, "UI_NetStatus")
REGISTER_ENUM(UI_NET_WAITING, "�������� �������")
REGISTER_ENUM(UI_NET_ERROR, "������ ��������")
REGISTER_ENUM(UI_NET_OK, "�������� ����������")
END_ENUM_DESCRIPTOR(UI_NetStatus)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_NetCenter, NetType, "UI_NetCenterNetType")
REGISTER_ENUM_ENCLOSED(UI_NetCenter, LAN, "���� � ��������� ����")
REGISTER_ENUM_ENCLOSED(UI_NetCenter, ONLINE, "���� � ���������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_NetCenter, NetType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataSelectionOperate, SelectionCommand, "UI_ActionDataSelectionOperate")
REGISTER_ENUM_ENCLOSED(UI_ActionDataSelectionOperate, LEAVE_TYPE_ONLY, "�������� ������ ������ ����� ����")
REGISTER_ENUM_ENCLOSED(UI_ActionDataSelectionOperate, LEAVE_SLOT_ONLY, "�������� ������ ���� ����")
REGISTER_ENUM_ENCLOSED(UI_ActionDataSelectionOperate, SAVE_SELECT, "��������� ������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataSelectionOperate, RETORE_SELECT, "������������ ������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataSelectionOperate, ADD_SELECT, "�������� � �������� �������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataSelectionOperate, SUB_SELECT, "������� �� �������� �������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataSelectionOperate, SelectionCommand)

BEGIN_ENUM_DESCRIPTOR(GameListInfoType, "GameListInfoType")
REGISTER_ENUM(GAME_INFO_TAB, "������ ������������")
REGISTER_ENUM(GAME_INFO_START_STATUS, "������ ������������")
REGISTER_ENUM(GAME_INFO_GAME_NAME, "��� ����")
REGISTER_ENUM(GAME_INFO_HOST_NAME, "��� �����")
REGISTER_ENUM(GAME_INFO_WORLD_NAME, "��� ����")
REGISTER_ENUM(GAME_INFO_PLAYERS_NUMBER, "���������� ������� (������� �� ���������)")
REGISTER_ENUM(GAME_INFO_PLAYERS_CURRENT, "������� ���������� ������� � ����")
REGISTER_ENUM(GAME_INFO_PLAYERS_MAX, "����������� ���������� �������")
REGISTER_ENUM(GAME_INFO_PING, "����")
REGISTER_ENUM(GAME_INFO_NAT_TYPE, "��� �������� �����������")
REGISTER_ENUM(GAME_INFO_NAT_COMPATIBILITY, "������������� ���� �����������")
REGISTER_ENUM(GAME_INFO_GAME_TYPE, "��� ����")
END_ENUM_DESCRIPTOR(GameListInfoType)

BEGIN_ENUM_DESCRIPTOR(GameStatisticType, "GameStatisticType")
REGISTER_ENUM(STAT_FREE_FOR_ALL, "������ �� ����")
REGISTER_ENUM(STAT_1_VS_1, "1 �� 1")
REGISTER_ENUM(STAT_2_VS_2, "2 �� 2")
REGISTER_ENUM(STAT_3_VS_3, "3 �� 3")
END_ENUM_DESCRIPTOR(GameStatisticType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(ShowStatisticType, Type, "ShowStatisticType::Type")
REGISTER_ENUM_ENCLOSED(ShowStatisticType, STAT_VALUE, "�������� ����������")
REGISTER_ENUM_ENCLOSED(ShowStatisticType, POSITION, "�����")
REGISTER_ENUM_ENCLOSED(ShowStatisticType, NAME, "���")
REGISTER_ENUM_ENCLOSED(ShowStatisticType, RATING, "�������")
REGISTER_ENUM_ENCLOSED(ShowStatisticType, SPACE, "������ ��������")
END_ENUM_DESCRIPTOR_ENCLOSED(ShowStatisticType, Type)

BEGIN_ENUM_DESCRIPTOR(PostEffectType, "PostEffectType")
REGISTER_ENUM(PE_DOF, "��������������")
REGISTER_ENUM(PE_BLOOM, "��������")
REGISTER_ENUM(PE_COLOR_DODGE, "����������")
REGISTER_ENUM(PE_MONOCHROME, "�����-�����")
END_ENUM_DESCRIPTOR(PostEffectType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataPause, PauseType, "UI_ActionDataPause::PauseType")
REGISTER_ENUM_ENCLOSED(UI_ActionDataPause, USER_PAUSE, "���������������� �����")
REGISTER_ENUM_ENCLOSED(UI_ActionDataPause, MENU_PAUSE, "������� ����")
REGISTER_ENUM_ENCLOSED(UI_ActionDataPause, NET_PAUSE, "������� �����")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataPause, PauseType)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataGlobalStats, Task, "UI_ActionDataGlobalStats::Task")
REGISTER_ENUM_ENCLOSED(UI_ActionDataGlobalStats, REFRESH, "��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataGlobalStats, GOTO_BEGIN, "������� � ������ ��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataGlobalStats, FIND_ME, "����� ���� � ��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataGlobalStats, GET_PREV, "���������� �������� ��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataGlobalStats, GET_NEXT, "��������� �������� ��������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataGlobalStats, Task)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlTextList, ScrollType, "UI_ControlTextList::ScrollType")
REGISTER_ENUM_ENCLOSED(UI_ControlTextList, ORDINARY, "�������")
REGISTER_ENUM_ENCLOSED(UI_ControlTextList, AUTO_SMOOTH, "�������������� �������")
REGISTER_ENUM_ENCLOSED(UI_ControlTextList, AT_END_IF_SIZE_CHANGE, "������� � ����� ��� ��������� ���������� �����")
REGISTER_ENUM_ENCLOSED(UI_ControlTextList, AUTO_SET_AT_END, "������� � �����")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlTextList, ScrollType)