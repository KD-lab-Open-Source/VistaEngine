#include "StdAfx.h"
#include "UserInterface.h"
#include "CommonLocText.h"

#include "UI_Logic.h"
#include "UI_Render.h"
#include "Render/Inc/IRenderDevice.h"
#include "Render/Src/cCamera.h"
#include "Render/d3d/D3DRender.h"
#include "Render/src/VisGeneric.h"
#include "VistaRender/postEffects.h"

#include "XTL/SafeCast.h"
#include "Serialization/Serialization.h"
#include "Serialization/ResourceSelector.h"
#include "Serialization/XPrmArchive.h"
#include "Serialization/SerializationFactory.h"
#include "kdw/PropertyEditor.h"
#include "Units/UnitItemResource.h"
#include "Units/UnitItemInventory.h"

#include "ShowHead.h"
#include "GameLoadManager.h"
#include "UI_Controls.h"
#include "UI_UnitView.h"
#include "UI_Minimap.h"
#include "UI_StreamVideo.h"
#include "UI_Actions.h"
#include "UI_NetCenter.h"

#include "SystemUtil.h"
#include "Units/CircleManagerParam.h"

#include "Units/GlobalAttributes.h"

#include "Game/SoundApp.h"
#include "Universe.h"
#include "Serialization/StringTable.h"
#include "WBuffer.h"
#include "UnicodeConverter.h"

#include "StreamCommand.h"
extern StreamDataManager uiStreamLogicCommand;
extern StreamDataManager uiStreamGraphCommand;

extern Singleton<UI_StreamVideo> streamVideo;

// UI_Dispatcher::instance() (inherited from LibraryWrapper<UI_Dispatcher>) is
// called below before WRAP_LIBRARY(UI_Dispatcher,...) defines its explicit
// specialization. Declare the specializations up front so clang doesn't
// implicitly instantiate them first (MSVC was lax about the ordering).
template<> UI_Dispatcher& LibraryWrapper<UI_Dispatcher>::instance();
template<> void LibraryWrapper<UI_Dispatcher>::serializeLibrary(Archive& ar);

const wchar_t* getColorString(const Color4f& color)
{
	static wchar_t buf[32];
	swprintf(buf, L"&%02X%02X%02X", color.GetR(), color.GetG(), color.GetB());
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
	isPreloaded_ = false;
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
	}
	else
		ar.serialize(ModelSelector(backgroundModelName_), "backgroundModelName", "3D ������");

	UI_ControlContainer::serialize(ar);

	if(isUnderEditor())
		updateActivationOffsets();

	ar.serialize(activationTime_, "activationTime", "����� ���������");
	ar.serialize(deactivationTime_, "deactivationTime", "����� �����������");

	ar.serializeArray(activationOffsets_, "activationOffsets", 0);

	//ar.serialize(activationDelay_, "activationDelay_", 0);
	//ar.serialize(deactivationDelay_, "deactivationDelay_", 0);

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

void UI_Screen::preLoad()
{
	if(!isPreloaded_){
		isPreloaded_ = true;
		__super::preLoad();
	}
}

void UI_Screen::release()
{
	if(isPreloaded_){
		isPreloaded_ = false;
		__super::release();
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

	if(!disableInputAtChangeState_ || !(isActivating() || isDeactivating())){
		if(!focused_control){
			for(ControlList::reverse_iterator it = controls_.rbegin(); it != controls_.rend(); ++it){
				if((*it)->hoverUpdate(cursor_pos))
					return true;
			}
		}
		else if(focused_control->hoverUpdate(cursor_pos))
			return true;
	}

	UI_LogicDispatcher::instance().setHoverControl(0);
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
	MTG();

	start_timer_auto();

	UI_BackgroundScene::instance().selectModel(backgroundModelName_.c_str());

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->relax(activation);

	float duration = (activation ? activationTime_ : deactivationTime_);

	if(duration > FLT_EPS){
		for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
			(*it)->setActivationTransform(duration, activation);
	}

	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		if((*it)->isVisible())
			(*it)->startBackgroundAnimation(UI_BackgroundAnimation::PLAY_STARTUP, true, !activation);
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


struct ShowHotKeys
{
	vector<UI_ControlHotKey>& lst_;
	ShowHotKeys(vector<UI_ControlHotKey>& l) : lst_(l) {}

	void serialize(Archive& ar){
		vector<UI_ControlHotKey>::iterator it;
		FOR_EACH(lst_, it)
			ar.serialize(it->hotKey, it->ref.referenceString(), it->ref.referenceString());
	}
};

void UI_Screen::saveHotKeys() const
{
	vector<UI_ControlHotKey> lst;
	for(ControlList::const_iterator it = controls_.begin(); it != controls_.end(); ++it)
		(*it)->getHotKeyList(lst);

	XBuffer fname;
	fname < name() < ".HotKeys.txt";

	Serializer hotKeysSerializer(ShowHotKeys(lst), fname.c_str(), 0);
	if(kdw::edit(hotKeysSerializer, "Scripts\\TreeControlSetups\\screenHotKeys", 0)){
		WBuffer keyName;
		XStream out(fname.c_str(), XS_OUT);
		vector<UI_ControlHotKey>::const_iterator it;
		FOR_EACH(lst, it){
			string outName;
			w2a(outName, it->hotKey.toString(keyName));
			out < it->ref.referenceString() < ";" < outName.c_str() < "\r\n";
		}
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
	selectionBorderColor_ = Color4f(1,1,1,1);
	selectionTextureName_ = "Scripts\\Resource\\Textures\\selectionFrame.tga";
	selectionCornerTextureName_ = "Scripts\\Resource\\Textures\\selectionFrameCorner.tga";
	needSelectionCenter_ = false;
	selectionCenterTextureName_ = "Scripts\\Resource\\Textures\\selectionFrameCenter.tga";
}

void SelectionBorderColor::serialize(Archive& ar)
{
	static ResourceSelector::Options textureOptions("*.tga", "Resource\\TerrainData\\Textures", "Will select location of an file textures");
	ar.serialize(selectionBorderColor_, "selectionBorderColor", "����� ���������: ����");
	ar.serialize(ResourceSelector(selectionTextureName_, textureOptions), "selectionTextureName", "����� ���������: ��������");
	ar.serialize(ResourceSelector(selectionCornerTextureName_, textureOptions), "selectionCornerTextureName", "����� ���������: �������� �������� ������ ����");
	ar.serialize(needSelectionCenter_, "needSelectionCenter", "����� ���������: ��������� ��������");
	ar.serialize(ResourceSelector(selectionCenterTextureName_, textureOptions), "selectionCenterTextureName", "����� ���������: �������� ������");
}


// ------------------- UI_Dispatcher

WRAP_LIBRARY(UI_Dispatcher, "UI_Attribute", "UI_Attribute", "Scripts\\Content\\UI_Attributes", 0, 0);

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

	UI_BackgroundScene::instance().drawDebugInfo();

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
		if(currentScreen_ != graphCurrentScreen_){
			buf < "Logic screen: " < (currentScreen_ ? currentScreen_->name() : "<empty>") < "\n";
			buf < "Graph screen: " < (graphCurrentScreen_ ? graphCurrentScreen_->name() : "<empty>");
		}
		else
			buf < "Screen: " < (currentScreen_ ? currentScreen_->name() : "<empty>");

		UI_Render::instance().outDebugText(Vect2f(0.f, 0.3f), buf.c_str(), &Color4c::YELLOW);
	}

	if(graphCurrentScreen_)
		graphCurrentScreen_->drawDebug2D();

	if(isModalMessageMode())
		messageScreen_->drawDebug2D();

	if(showDebugInterface.focusedControlBorder){
		if(const UI_ControlBase* focused_control = focusedControl_)
			UI_Render::instance().drawRectangle(focused_control->transfPosition(), Color4f(0.5f,0.5f,1.0f,0.5f), true);
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

		wstring msg;
		if(task->getMessagePrefix(msg))
			sendMessage(message_setup, msg.c_str());
	}
	else 
		uiStreamGraph2LogicCommand.set(fCommandSetTask) << (void*)(&message_setup) << int(state) << (char)is_secondary;
}

bool UI_Dispatcher::getTaskList(std::wstring& str, bool reverse) const
{
	MTL();

	if(tasks_.empty())
		return false;

	str.clear();
	wstring msg;
	
	if(reverse){
		UI_Tasks::const_reverse_iterator it = tasks_.rbegin();
		for(;it != tasks_.rend(); ++it){
			if(it->getText(msg)){
				UI_LogicDispatcher::instance().expandTextTemplate(msg, ExpandInfo(ExpandInfo::MESSAGE));
				if(!str.empty())
					str += L"&>\n";
				str += msg;
			}
		}
	}
	else {
		UI_Tasks::const_iterator it;
		FOR_EACH(tasks_, it){
			if(it->getText(msg)){
				UI_LogicDispatcher::instance().expandTextTemplate(msg, ExpandInfo(ExpandInfo::MESSAGE));
				if(!str.empty())
					str += L"&>\n";
				str += msg;
			}
		}
	}

	return !str.empty();
}

bool UI_Dispatcher::getMessageQueue(std::wstring& str, const UI_MessageTypeReferences& filter, bool reverse, bool firstOnly) const
{
	MTL();

	if(messageQueue_.empty())
		return false;

	str.clear();
	wstring msg;

	if(reverse){
		MessageQueue::const_reverse_iterator it = messageQueue_.rbegin();
		for(;it != messageQueue_.rend(); ++it)
			if(find(filter.begin(), filter.end(), it->type()) != filter.end()){
				if(firstOnly){
					if(it->getText(str)){
						UI_LogicDispatcher::instance().expandTextTemplate(str, ExpandInfo(ExpandInfo::MESSAGE));
						break;
					}
				}
				else {
					if(it->getText(msg)){
						UI_LogicDispatcher::instance().expandTextTemplate(msg, ExpandInfo(ExpandInfo::MESSAGE));
						if(!str.empty())
							str += L"&>\n";
						str += msg;
					}
				}
			}
	}
	else {
		MessageQueue::const_iterator it;
		FOR_EACH(messageQueue_, it)
			if(find(filter.begin(), filter.end(), it->type()) != filter.end()){
				if(firstOnly){
					if(it->getText(str)){
						UI_LogicDispatcher::instance().expandTextTemplate(str, ExpandInfo(ExpandInfo::MESSAGE));
						break;
					}
				}
				else {
					if(it->getText(msg)){
						UI_LogicDispatcher::instance().expandTextTemplate(msg, ExpandInfo(ExpandInfo::MESSAGE));
						if(!str.empty())
							str += L"&>\n";
						str += msg;
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
	setFocus(0);
	
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
	minimap().releaseMapTexture();
}

void UI_Dispatcher::resetScreens()
{
	start_timer_auto();

	if(needResetScreens_ && !exitGameInProgress_){
		initBgScene();

		xassert(currentScreen_ == graphCurrentScreen_);
		if(currentScreen_)
			currentScreen_->release();
		if(preloadedScreen_ && preloadedScreen_ != currentScreen_)
			preloadedScreen_->release();
		preloadedScreen_ = 0;

		// ����� ������� ������� � ��������� ���������
		UI_Dispatcher clearUI;
		XPrmIArchive ia(0);
		if(ia.open("Scripts\\Content\\UI_Attributes"))
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

	//ar.serialize(needChangeScreen_, "needChangeScreen_", 0);
	//ar.serialize(nextScreen_, "nextScreen_", 0);

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
	
	initBgScene();

	ui_ControlMapReference[ui_ControlMapBackReference[string()] = 0] = string();
	controlID_ = 1;
	ScreenContainer::iterator scr;
	FOR_EACH(screens(), scr)
		scr->updateIndex();

	wcsncpy(privateMessageColor_, getColorString(UI_GlobalAttributes::instance().privateMessage()), 7);
	privateMessageColor_[7] = 0;
	
	wcsncpy(systemMessageColor_, getColorString(UI_GlobalAttributes::instance().systemMessage()), 7);
	systemMessageColor_[7] = 0;
}

void UI_Dispatcher::quickRedraw()
{
	start_timer_auto();

	logicQuant();

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

void UI_Dispatcher::logicQuant()
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
			UI_LogicDispatcher::instance().logicPreQuant();

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

		UI_LogicDispatcher::instance().logicQuant(logicPeriodSeconds);

		if(debugShowEnabled)
			showDebugInfo();
	}

	uiStreamLogicCommand.lock();
	uiStreamLogicCommand << uiStreamCommand;
	uiStreamCommand.clear();
	uiStreamLogicCommand.unlock();
}

void UI_Dispatcher::quant(float dt, bool load_mode)
{
	MTG();
	start_timer_auto();

	graphQuantCommit();
	++debugGraphQuant_;

	uiStreamLogicCommand.lock();
	uiStreamLogicCommand.execute();
	uiStreamLogicCommand.unlock();

	streamVideo().ui_quant();

	if(!load_mode){
		minimap().quant(dt);
		showHead().quant(dt);
	}

	if(isEnabled_){
		if(isModalMessageMode()){
			if(load_mode)
				messageScreen_->hoverUpdate(UI_LogicDispatcher::instance().mousePosition(), 0);
			else {
				const UI_ControlBase* lastHovered = UI_LogicDispatcher::instance().hoverControl();
				messageScreen_->hoverUpdate(UI_LogicDispatcher::instance().mousePosition(), focusedControl_);
				UI_LogicDispatcher::instance().focusControlProcess(lastHovered);
			}
			
			if(graphCurrentScreen_)
				graphCurrentScreen_->quant(dt);

			messageScreen_->quant(dt);
		}
		else if(graphCurrentScreen_){
			if(load_mode)
				graphCurrentScreen_->hoverUpdate(UI_LogicDispatcher::instance().mousePosition(), 0);
			else {
				const UI_ControlBase* lastHovered = UI_LogicDispatcher::instance().hoverControl();
				graphCurrentScreen_->hoverUpdate(UI_LogicDispatcher::instance().mousePosition(), focusedControl_);
				UI_LogicDispatcher::instance().focusControlProcess(lastHovered);
			}

			graphCurrentScreen_->quant(dt);
		}

		UI_BackgroundScene::instance().graphQuant(dt);
	}

	if(!load_mode){
		GameOptions::instance().commitQuant(dt);
		if(GameOptions::instance().needRollBack()){
			GameOptions::instance().restoreSettings();
			UI_LogicDispatcher::instance().handleMessageReInitGameOptions();
		}
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

	if(!GlobalAttributes::instance().drawUnitSideSpritesAfterInterface)
		UI_LogicDispatcher::instance().drawUnitSideSprites();
	
	if(isEnabled_){
		UI_BackgroundScene::instance().draw();
	
		if(isUnderEditor()){
			if(Camera* cam = UI_Render::instance().camera()){
				D3DVIEWPORT9 vp = { 0, 0, cam->GetRenderSize().x, cam->GetRenderSize().y, 0.0f, 1.0f };
				RDCALL(gb_RenderDevice3D->D3DDevice_->SetViewport(&vp));
			}
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

	if(GlobalAttributes::instance().drawUnitSideSpritesAfterInterface)
		UI_LogicDispatcher::instance().drawUnitSideSprites();

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

void fCommandUI_RemoveMessage(void* data)
{
	const UI_MessageSetup* setup = *(const UI_MessageSetup**)data;
	bool independent = *(bool*)(setup + 1);
	UI_Dispatcher::instance().removeMessage(*setup, independent);
}

void fCommandUI_MessageBox(void* data)
{
	const wchar_t* text = (const wchar_t*)data;

	UI_Dispatcher::instance().messageBox(text);
}

void UI_Dispatcher::messageBox(const wchar_t* message)
{
	if(MT_IS_LOGIC()){
		if(!message){
			messageBoxMessage_.clear();
		}
		else if(isModalScreen_){
			messageBoxMessage_ += L'\n';
			messageBoxMessage_ += message;
		}
		else if(messageScreen_){
			messageBoxMessage_ = message;
//			messageScreen_->logicInit();
			messageScreen_->init();
/*
			messageScreen_->init();
			messageScreen_->logicQuant();
			messageScreen_->activate();
			messageScreen_->logicQuant();
*/
			isModalScreen_ = true;
		}
	}
	else if(message)
		uiStreamGraph2LogicCommand.set(fCommandUI_MessageBox) << XBuffer((void*)message, (wcslen(message)+1) * sizeof(wchar_t));
	else
		uiStreamGraph2LogicCommand.set(fCommandUI_MessageBox) << (void*)0;
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

//void fCommandUI_SendMessage(void* data)
//{
//	const UI_MessageSetup** setup = (const UI_MessageSetup**)data;
//	UI_Dispatcher::instance().sendMessage(**setup, *(float*)(setup + 1));
//}

void UI_Dispatcher::sendMessage(UI_MessageID id)
{
	MTL();
	//if(MT_IS_LOGIC())
		sendMessage(UI_GlobalAttributes::instance().messageSetup(id));
	//else
	//	uiStreamGraph2LogicCommand.set(fCommandUI_SendMessage) << &UI_GlobalAttributes::instance().messageSetup(id) << 1.f;
}

void UI_Dispatcher::sendMessage(const UI_MessageSetup& message_setup, const wchar_t* custom_prefix)
{
	MTL();

	if(message_setup.isEmpty())
		return;

	MessageQueue::iterator it = std::find(messageQueue_.begin(), messageQueue_.end(), message_setup);
	if(it != messageQueue_.end())
		it->start();
	else {
		messageQueue_.push_back(UI_Message(message_setup));
		messageQueue_.back().setMessagePrefix(custom_prefix);
	}
}

void UI_Dispatcher::sendMessage(const UI_MessageSetup& message_setup, float alpha)
{
	MTL();

	if(message_setup.isEmpty())
		return;

	//if(MT_IS_LOGIC()){
		MessageQueue::iterator it = std::find(messageQueue_.begin(), messageQueue_.end(), message_setup);
		if(it != messageQueue_.end()){
			it->start();
			it->setAlpha(alpha);
		}
		else {
			messageQueue_.push_back(UI_Message(message_setup));
			messageQueue_.back().setAlpha(alpha);
		}
	//}
	//else
	//	uiStreamGraph2LogicCommand.set(fCommandUI_SendMessage) << &message_setup << alpha;
}

bool UI_Dispatcher::removeMessage(const UI_MessageSetup& message_setup, bool independent)
{
	start_timer_auto();

	if(message_setup.isEmpty())
		return false;

	if(MT_IS_LOGIC()){
		MessageQueue::iterator it = std::find(messageQueue_.begin(), messageQueue_.end(), message_setup);
		if(it != messageQueue_.end()){
			if(((*it).messageSetup().isVoiceInterruptable() || independent) && (*it).messageSetup().hasVoice()){
				if(voiceManager().validatePlayingFile((*it).messageSetup().text()->voice())){
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

	UI_Render::instance().releaseResources();

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
				UI_BackgroundScene::instance().selectModel(0);
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
		fullkey = addModifiersState(VK_LBUTTON);
		break;
	case UI_INPUT_MOUSE_RBUTTON_DOWN:
		fullkey = addModifiersState(VK_RBUTTON);
		break;
	case UI_INPUT_MOUSE_MBUTTON_DOWN:
		fullkey = addModifiersState(VK_MBUTTON);
		break;
	case UI_INPUT_MOUSE_LBUTTON_UP:

		break;
	case UI_INPUT_MOUSE_RBUTTON_UP:

		break;
	case UI_INPUT_MOUSE_MBUTTON_UP:

		break;
	case UI_INPUT_MOUSE_LBUTTON_DBLCLICK:
		fullkey = addModifiersState(VK_LDBL);
		break;
	case UI_INPUT_MOUSE_RBUTTON_DBLCLICK:
		fullkey = addModifiersState(VK_RDBL);
		break;
	case UI_INPUT_MOUSE_MBUTTON_DBLCLICK:

		break;
	case UI_INPUT_MOUSE_WHEEL_UP:
		fullkey = addModifiersState(VK_WHEELUP);
		break;
	case UI_INPUT_MOUSE_WHEEL_DOWN:
		fullkey = addModifiersState(VK_WHEELDN);
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
		event.setFlag(UI_InputEvent::MK_MENU);
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
	UI_BackgroundScene::instance().init(gb_VisGeneric);
}

void UI_Dispatcher::finitBgScene()
{
	UI_BackgroundScene::instance().done();
}

void UI_Dispatcher::setCurrentScreenGraph(UI_Screen* screen)
{
	MTG();
	if(graphCurrentScreen_ != screen){
		if(preloadedScreen_)
			if(screen == 0){
				xxassert(false, XBuffer() < "�������� ���������� �� ������������� ��� ���������������� ������: \"" < preloadedScreen_->name() < "\""); 
			}
			else if(screen != preloadedScreen_){
				xxassert(false, XBuffer() < "�������� ������: \"" < screen->name() < "\"\n�� ������������� ��� ����������������: \"" < preloadedScreen_->name() < "\""); 
			}

		if(screen)
			screen->preLoad();

		if(preloadedScreen_ && preloadedScreen_ != screen && preloadedScreen_ != graphCurrentScreen_)
			preloadedScreen_->release();

		preloadedScreen_ = 0;

		if(graphCurrentScreen_)
			graphCurrentScreen_->release();

		graphCurrentScreen_ = screen;

		GameLoadManager::instance().init();
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
	setCurrentScreenLogic(screen(UI_Screen::LOADING));
	setCurrentScreenGraph(screen(UI_Screen::LOADING));
}

bool UI_Dispatcher::playVoice(const UI_MessageSetup& setup)
{
	bool interrupt = true;
	if(voiceManager().isPlaying()){
		MessageQueue::const_iterator i;
		FOR_EACH(messageQueue_, i)
			if((*i).messageSetup().text() && (*i).messageSetup().text()->hasVoice() && 
				voiceManager().validatePlayingFile((*i).messageSetup().text()->voice())){
					interrupt = (*i).messageSetup().isVoiceInterruptable() && setup.isCanInterruptVoice();
					break;
				}
	}
	if((interrupt || !voiceLockTimer_.busy()) && setup.playVoice(interrupt)){
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
REGISTER_ENUM(UI_UNITLIST_SQUAD, "����������� ���������� �����")
REGISTER_ENUM(UI_UNITLIST_SQUADS_IN_WORLD, "������ ������� �� ����")
REGISTER_ENUM(UI_UNITLIST_PRODUCTION, "������� ������������")
REGISTER_ENUM(UI_UNITLIST_PRODUCTION_COMPACT, "���������� ������� ������������")
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
REGISTER_ENUM(UI_MINIMAP_SYMBOL_UNIT_WAITING, "������ ����")
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
REGISTER_ENUM(UI_CURSOR_ASSEMBLY_POINT, "��������� ����� ������ �����")
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
REGISTER_ENUM(UI_ACTION_LOCALIZE_CONTROL, "���������� �����������\\��������������")
REGISTER_ENUM(UI_ACTION_HOVER_INFO, "���������� �����������\\���������� ��� ���������")
REGISTER_ENUM(UI_ACTION_LINK_TO_ANCHOR, "���������� �����������\\��������� � ������� �� ����")
REGISTER_ENUM(UI_ACTION_LINK_TO_PARENT, "���������� �����������\\��������� � ����������� ��������")
REGISTER_ENUM(UI_ACTION_LINK_TO_MOUSE, "���������� �����������\\��������� � ��������� ����")
REGISTER_ENUM(UI_ACTION_HOT_KEY, "���������� �����������\\��������� ������� �������")
REGISTER_ENUM(UI_ACTION_EXTERNAL_CONTROL, "���������� �����������\\���������� ������ �������")
REGISTER_ENUM(UI_ACTION_STATE_MARK, "��������\\������� ��������� ������")
REGISTER_ENUM(UI_ACTION_GLOBAL_VARIABLE, "��������\\�����������/��������� ���������� ����������")
REGISTER_ENUM(UI_ACTION_CHANGE_STATE, "���������� �����������\\����������� ���������")
REGISTER_ENUM(UI_ACTION_AUTOCHANGE_STATE, "���������� �����������\\��������� ���������")
REGISTER_ENUM(UI_ACTION_CLICK_FOR_TRIGGER, "���������� �����������\\�������� ���� � �������")
REGISTER_ENUM(UI_ACTION_POST_EFFECT, "���������� �����������\\��������/��������� ����-������")
REGISTER_ENUM(UI_ACTION_PROFILES_LIST, "�������\\������ ���������")
REGISTER_ENUM(UI_ACTION_PROFILE_DIFFICULTY, "�������\\���������� ��������� �����������")
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
REGISTER_ENUM(UI_ACTION_UNIT_PARAMETER, "����� ��������\\����� ��������� �����")
REGISTER_ENUM(UI_ACTION_PLAYER_UNITS_COUNT, "����� ��������\\����� ���������� ������ ������")
REGISTER_ENUM(UI_ACTION_PLAYER_STATISTICS, "����� ��������\\����� ���������� �� �������")
REGISTER_ENUM(UI_ACTION_UNIT_HINT, "����� ��������\\����� ��������� � �����")
REGISTER_ENUM(UI_ACTION_UI_HINT, "����� ��������\\����� ��������� � ��������")
REGISTER_ENUM(UI_ACTION_PAUSE_GAME, "�������\\��������/��������� �����")
REGISTER_ENUM(UI_ACTION_CLICK_MODE, "�������\\����� ����� �� ���")
REGISTER_ENUM(UI_ACTION_CONTEX_APPLY_CLICK_MODE, "�������\\��������� ������� ����� ����� �� ���")
REGISTER_ENUM(UI_ACTION_ACTIVATE_WEAPON, "�������\\�������� ������")
REGISTER_ENUM(UI_ACTION_DEACTIVATE_WEAPON, "�������\\��������� ������")
REGISTER_ENUM(UI_ACTION_WEAPON_STATE, "��������\\��������� ������")
REGISTER_ENUM(UI_ACTION_CANCEL, "�������\\������ ��������")
REGISTER_ENUM(UI_ACTION_BIND_TO_UNIT, "��������\\� ����������� �����")
REGISTER_ENUM(UI_ACTION_BIND_TO_HOVERED_UNIT, "��������\\� ����������� �����")
REGISTER_ENUM(UI_ACTION_UNIT_ON_MAP, "��������\\���� c��������� �� ����")
REGISTER_ENUM(UI_ACTION_SQUAD_ON_MAP, "��������\\����� c��������� �� ����")
REGISTER_ENUM(UI_ACTION_UNIT_IN_TRANSPORT, "��������\\���� � ����������")
REGISTER_ENUM(UI_ACTION_BINDEX, "��������\\����������� �������� � �����")
REGISTER_ENUM(UI_ACTION_BIND_TO_UNIT_STATE, "��������\\��������� �������� �����")
REGISTER_ENUM(UI_ACTION_UNIT_HAVE_PARAMS, "��������\\�������� ������� ����������")
REGISTER_ENUM(UI_ACTION_BIND_TO_IDLE_UNITS, "�������\\����� � �������� �����������")
REGISTER_ENUM(UI_ACTION_FIND_UNIT, "�������\\������� � ���������� �����")
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
REGISTER_ENUM(UI_ACTION_BUILDING_CAN_INSTALL, "��������\\����� ��������� ����� ��� ������")
REGISTER_ENUM(UI_ACTION_UNIT_SELECT, "�������\\������ �����/������")
REGISTER_ENUM(UI_ACTION_UNIT_DESELECT, "�������\\�������� �����/������ �� ������ ���������")
REGISTER_ENUM(UI_ACTION_SET_SELECTED, "�������\\������� ������� � �������")
REGISTER_ENUM(UI_ACTION_SELECTION_OPERATE, "�������\\������ �� �������� ��������")
REGISTER_ENUM(UI_ACTION_PRODUCTION_PROGRESS, "����� ��������\\����������� ��������� ������������ �����")
REGISTER_ENUM(UI_ACTION_PRODUCTION_PARAMETER_PROGRESS, "����� ��������\\����������� ��������� ������������ ����������")
REGISTER_ENUM(UI_ACTION_PRODUCTION_CURRENT_PROGRESS, "����� ��������\\����������� �������� ��������� ������������")
REGISTER_ENUM(UI_ACTION_RELOAD_PROGRESS, "����� ��������\\����������� ����������� ������")
REGISTER_ENUM(UI_ACTION_OPTION, "�����\\��������� ����� ����")
REGISTER_ENUM(UI_ACTION_SET_KEYS, "�����\\��������� ����������")
REGISTER_ENUM(UI_ACTION_LOADING_PROGRESS, "����� ��������\\����������� ��������� ��������")
REGISTER_ENUM(UI_ACTION_MERGE_SQUADS, "�������\\������� �������")
REGISTER_ENUM(UI_ACTION_SPLIT_SQUAD, "�������\\��������� ������")
REGISTER_ENUM(UI_BIND_PRODUCTION_QUEUE, "��������\\�� ��������� � ������� ������������")
REGISTER_ENUM(UI_ACTION_MESSAGE_LIST, "����� ��������\\����� ������� ���������")
REGISTER_ENUM(UI_ACTION_TASK_LIST, "����� ��������\\����� ������ �����")
REGISTER_ENUM(UI_ACTION_UNIT_FACE, "����� ��������\\����������� �����")
REGISTER_ENUM(UI_ACTION_SOURCE_ON_MOUSE, "�������\\���������� ���������� �� ���� (������ ��� ����)")
REGISTER_ENUM(UI_ACTION_MINIMAP_ROTATION, "�������\\��������� �������� ���������")
REGISTER_ENUM(UI_ACTION_MINIMAP_DRAW_WATER, "�������\\����������� ��������� ���� �� ���������")
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
REGISTER_ENUM(UI_ACTION_INET_DIRECT_ADDRESS, "������� ����\\����� �������")
REGISTER_ENUM(UI_ACTION_INET_DIRECT_JOIN_GAME, "������� ����\\�������������� � ���������� �������")
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
REGISTER_ENUM(UI_ACTION_INVENTORY_QUICK_ACCESS_MODE, "�������\\����� ������ ��������� �������� �������")
END_ENUM_DESCRIPTOR(UI_ControlActionID)

BEGIN_ENUM_DESCRIPTOR(UI_ClickModeID, "UI_ClickModeID")
REGISTER_ENUM(UI_CLICK_MODE_NONE, "��������")
REGISTER_ENUM(UI_CLICK_MODE_MOVE, "���� ��� �����������")
REGISTER_ENUM(UI_CLICK_MODE_ASSEMBLY, "������� ����� ����� �����")
REGISTER_ENUM(UI_CLICK_MODE_ATTACK, "���� ��� �����")
REGISTER_ENUM(UI_CLICK_MODE_PLAYER_ATTACK, "���� ��� ����� ������-�������")
REGISTER_ENUM(UI_CLICK_MODE_PATROL, "����� ��������������")
REGISTER_ENUM(UI_CLICK_MODE_REPAIR, "���� ��� �������")
REGISTER_ENUM(UI_CLICK_MODE_RESOURCE, "���� ��� ����� ��������")
END_ENUM_DESCRIPTOR(UI_ClickModeID)

BEGIN_ENUM_DESCRIPTOR(UI_ClickModeMarkID, "UI_ClickModeMarkID")
REGISTER_ENUM(UI_CLICK_MARK_MOVEMENT, "�����������")
REGISTER_ENUM(UI_CLICK_MARK_ASSEMBLY_POINT, "����� ����� ����� �������")
REGISTER_ENUM(UI_CLICK_MARK_ATTACK, "�����")
REGISTER_ENUM(UI_CLICK_MARK_REPAIR, "������")
REGISTER_ENUM(UI_CLICK_MARK_MOVEMENT_WATER, "����������� �� ����")
REGISTER_ENUM(UI_CLICK_MARK_ATTACK_UNIT, "����� �����")
REGISTER_ENUM(UI_CLICK_MARK_PATROL, "��������������")
REGISTER_ENUM(UI_CLICK_MARK_WAIPOINT, "����� ����������")
REGISTER_ENUM(UI_CLICK_MARK_WAY, "������� ����������")
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
REGISTER_ENUM_ENCLOSED(UI_ActionUnitState, UI_UNITSTATE_SQUAD_CAN_QUERY_UNITS, "����� �������� ����� � �����")
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
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitHint, INVENTORY, "��� �����/�������� �� ��������� ��� ��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitHint, UNIT_LIST, "��� ����� �� ������ ������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitHint, UI_ActionDataUnitHintUnitType)

BEGIN_ENUM_DESCRIPTOR(UI_TaskStateID, "UI_TaskStateID")
REGISTER_ENUM(UI_TASK_ASSIGNED, "���������")
REGISTER_ENUM(UI_TASK_COMPLETED, "���������")
REGISTER_ENUM(UI_TASK_FAILED, "���������")
REGISTER_ENUM(UI_TASK_DELETED, "�������")
END_ENUM_DESCRIPTOR(UI_TaskStateID)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitParameter, ParameterClass, "ParameterClass")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, LOGIC, "������ ��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, LEVEL, "������� �����")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, GROUND, "��������� �����")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, PRODUCTION_PROGRESS, "�������� ������������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitParameter, ParameterClass)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitParameter, UnitType, "UnitType")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, SELECTED, "����������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, SPECIFIC, "���������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, PLAYER, "����-�����")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, UNIT_LIST, "�� ������ ������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataUnitParameter, HOVERED, "��� ������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataUnitParameter, UnitType)

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
REGISTER_ENUM_ENCLOSED(UI_NetCenter, DIRECT, "������ ����������")
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

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataParams, Type, "UI_ActionDataParams::Type")
REGISTER_ENUM_ENCLOSED(UI_ActionDataParams, SELECTED_UNIT_THEN_PLAYER, "������������ ���� ���� �� ������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataParams, PLAYER_UNIT, "�����-������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataParams, SELECTRD_UNIT_ONLY, "������ ������������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataParams, PLAYER_ONLY, "������ ������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataParams, Type)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataParams, ThatDoThanExist, "UI_ActionDataParams::ThatDoThanExist")
REGISTER_ENUM_ENCLOSED(UI_ActionDataParams, HIDE, "��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataParams, SHOW, "��������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataParams, DISABLE, "���������")
REGISTER_ENUM_ENCLOSED(UI_ActionDataParams, ENABLE, "���������")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ActionDataParams, ThatDoThanExist)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlCustomList, Type, "UI_ControlCustomList::Type")
REGISTER_ENUM_ENCLOSED(UI_ControlCustomList, TASK_LIST, "�������")
REGISTER_ENUM_ENCLOSED(UI_ControlCustomList, MESSAGE_LIST, "���������")
REGISTER_ENUM_ENCLOSED(UI_ControlCustomList, COMMON_LIST, "�����")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlCustomList, Type)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlCustomList, Side, "UI_ControlCustomList::Side")
REGISTER_ENUM_ENCLOSED(UI_ControlCustomList, TOP, "����")
REGISTER_ENUM_ENCLOSED(UI_ControlCustomList, BOTTOM, "���")
REGISTER_ENUM_ENCLOSED(UI_ControlCustomList, LEFT, "����")
REGISTER_ENUM_ENCLOSED(UI_ControlCustomList, RIGHT, "�����")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_ControlCustomList, Side)
