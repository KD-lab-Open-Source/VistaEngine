#include "StdAfx.h"

#include "..\Environment\Environment.h"
#include "Universe.h"
#include "Player.h"
#include "RenderObjects.h"
#include "terra.h"
#include "CameraManager.h"
#include "Triggers.h"
#include "Serialization.h"
#include "RangedWrapper.h"
#include "XPrmArchive.h"
#include "TerraInterface.inl"
#include "..\physics\crash\CrashSystem.h"
#include "..\physics\SecondMap.h"
#include "ShowHead.h"
#include "PFTrap.h"
#include "..\UserInterface\UserInterface.h"
#include "..\UserInterface\UI_CustomControls.h"
#include "..\UserInterface\UI_Logic.h"
#include "..\UserInterface\UI_BackgroundScene.h"
#include "GameOptions.h"
#include "..\UserInterface\SelectManager.h"
#include "..\Terra\terTools.h"
#include "TransparentTracking.h"
#include "NormalMap.h"
#include "WindMap.h"
#include "..\Water\CircleManager.h"
#include "..\Water\SkyObject.h"
#include "NumDetailTexture.h"
#include "UnitActing.h"
#include "StreamCommand.h"
#include "IronBullet.h"
#include "MergeOptions.h"
#include "..\Units\ExternalShow.h"
#include "PlayerStatistics.h"
#include "..\Sound\SoundSystem.h" // @dilesoft

BEGIN_ENUM_DESCRIPTOR(RealPlayerType, "RealPlayerType");
REGISTER_ENUM(REAL_PLAYER_TYPE_CLOSE, "Closed");
REGISTER_ENUM(REAL_PLAYER_TYPE_OPEN, "Open");
REGISTER_ENUM(REAL_PLAYER_TYPE_PLAYER, "Player");
REGISTER_ENUM(REAL_PLAYER_TYPE_AI, "Computer");
REGISTER_ENUM(REAL_PLAYER_TYPE_WORLD, "World");
END_ENUM_DESCRIPTOR(RealPlayerType);

TerraInterface* CreateTerraInterface();

PROGRESSCALLBACK Universe::progressCallback_ = 0;
Universe* Universe::universe_ = NULL;

//------------------------------------------

bool Channel::isInFogOfWar() // @dilesoft
{
	if(sound_->system_->gameActive_&&stopInFogOfWar_&&universe()->activePlayer()->fogOfWarMap()&&is3DSound_&&
	   universe()->activePlayer()->fogOfWarMap()->getFogState(ds3DBuffer_.vPosition.x,ds3DBuffer_.vPosition.y) != FOGST_NONE)
	{
		return true;
	}
	return false;
}

Universe::Universe(MissionDescription& mission, XPrmIArchive* ia) :
unitGrid(vMap.H_SIZE, vMap.V_SIZE)
{
	start_timer_auto();

	xassert(vMap.H_SIZE && vMap.V_SIZE);
	xassert(!universe());
	universe_ = this;

	loadProgressUpdate();

	userSave_ = mission.userSave();
	randomScenario_ = !mission.useMapSettings() && !userSave_;
	gameType_ = mission.gameType();

	circleShow_=new cCircleShow;

	circleManager_=new CircleManager;
	circleManager_->SetDrawOrder(GlobalAttributes::instance().circleManagerDrawOrder);
	terScene->AttachObj(circleManager_);

	circleManagerTeam_=new CircleManager;
	circleManagerTeam_->SetDrawOrder(GlobalAttributes::instance().circleManagerDrawOrder);
	terScene->AttachObj(circleManagerTeam_);

	gb_VisGeneric->SetUseLogicQuant(true);
	gb_VisGeneric->SetLogicQuant(0);

	secondMap = new SecondMap(vMap.H_SIZE,vMap.V_SIZE);

	transparent_tracking = NULL;
	if(!isUnderEditor())
		setLogicFp();
	GlobalAttributes::instance();
	AuxAttributeLibrary::instance();
    
	loadProgressUpdate();

	for(int i = 0; i < RaceTable::instance().size(); i++)
		RaceTable::instance()[i].setUnused();

	XRndSet(1);
	logicRnd.set(1);
	xm_random_generator.set(1);

	countDownTime_ = 0;

	missionSignature = 0;

	RigidBodyUnit::clearPathTracking();
	tileMapUpdateRegions.clear();
    
	normalMap = new NormalMap(vMap.H_SIZE,vMap.V_SIZE);
	windMap = new WindMap(vMap.H_SIZE,vMap.V_SIZE);
	pathFinder = new PathFinder(vMap.H_SIZE,vMap.V_SIZE);
	pathFinder->enableAutoImpassability(GlobalAttributes::instance().enableAutoImpassability);

	quant_counter_ = 0;

	active_player_ = 0;

	enableEventChecking_ = false;

	global_time.setTime(mission.globalTime); // Нужно установить время до загрузки spg

	if(ia){
		missionSignature = ia->crc();
		ia->serialize(MissionDescription(), "header", 0); // для избежания скипования при загрузке
		vMap.loadGameMap(*ia);
	}
	
	loadProgressUpdate();

	// Создание игроков
	int actActivePlayerID=0, actActiveCoopIdx=0;
	for(int i = 0; i < mission.playersAmountMax(); i++){
		SlotData& playerData = mission.changePlayerData(i);
		int curPlayerID=Players.size();
		if(playerData.realPlayerType == REAL_PLAYER_TYPE_PLAYER){
			for(int k=0; k<NETWORK_TEAM_MAX; k++){
				if(playerData.usersIdxArr[k]!=USER_IDX_NONE){
					if(playerData.usersIdxArr[k]==mission.activeUserIdx()){
						actActivePlayerID=curPlayerID;
						actActiveCoopIdx=k;
					}
					if(curPlayerID < Players.size() && findPlayer(curPlayerID))
						findPlayer(curPlayerID)->addCooperativePlayer( mission.constructPlayerData(i, k, curPlayerID));
					else
						addPlayer(mission.constructPlayerData(i, k, curPlayerID));
				}
			}
		}
		else if( playerData.realPlayerType == REAL_PLAYER_TYPE_AI)
			addPlayer(mission.constructPlayerData(i, 0, curPlayerID));
	}

	for(int i = 0; i < mission.auxPlayersAmount(); i++){
		Player* player = addPlayer(PlayerData(Players.size(), REAL_PLAYER_TYPE_PLAYER));
		player->setAuxPlayerType(AUX_PLAYER_TYPE_COMMON_FRIEND);
	}

	Player* world_player = addPlayer(PlayerData(Players.size(), REAL_PLAYER_TYPE_WORLD));

	terMapPoint = terScene->CreateMap(CreateTerraInterface(), NumDetailTextures);

	environment = new Environment(vMap.H_SIZE, vMap.V_SIZE, terScene, terMapPoint, mission);
	if(environment->water())
		environment->water()->SetChangeTile(waterChangePF);

	if(environment->temperature())
		environment->temperature()->SetChangeTile(iceChangePF);

	GameOptions::instance().environmentSetup();
	if(mission.gameType() & GAME_TYPE_REEL)
		environment->setShowFogOfWar(false);

	if(environment->fogOfWar())
		std::for_each(Players.begin(), Players.end(), std::mem_fun(&Player::initFogOfWarMap));

	//setActivePlayer(mission.activePlayerID(), mission.activeCooperativeIndex());
	setActivePlayer(actActivePlayerID, actActiveCoopIdx);

	UI_BackgroundScene::instance().setSky(environment->environmentTime()->GetCubeMap());

	crashSystem = new CrashSystem;

	UnitID::clearCounter();

	if(ia){
		ia->serialize(*cameraManager, "camera", 0);
		ia->serialize(*this, "universe", 0);   
		ia->serialize(*environment, "environment", 0);
	}

	if(Players.size() == 1){
		addPlayer(PlayerData(Players.size(), REAL_PLAYER_TYPE_WORLD));
		Player* player = Players.front();
		PlayerDataEdit data;
		player->getPlayerData(data);
		data.realPlayerType = REAL_PLAYER_TYPE_PLAYER;
		strcpy(data.playerName, "Player0");
		player->setPlayerData(data);
	}

	if(isRandomScenario()){ 
		PlayerVect::iterator pi;
		FOR_EACH(Players, pi)
			if(!(*pi)->isWorld() && (*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER)
				(*pi)->createRandomScenario(Vect2f(mission.startLocation((*pi)->shuffleIndex())));
	}

	enableEventChecking_ = true;

	interfaceEnabled_ = UI_Dispatcher::instance().isEnabled();

	//---------------------
	
	normalMap->updateRect(0,0,vMap.H_SIZE-1, vMap.V_SIZE-1);
	windMap->updateRect(0,0,vMap.H_SIZE-1, vMap.V_SIZE-1);
	pathFinder->updateRect(0,0,vMap.H_SIZE-1, vMap.V_SIZE-1);
	
	//-----------------

	loadProgressUpdate();
	dprintf("Universe created\n");
}

Universe::~Universe()
{
	DBGCHECK;

	activeMessages_.clear();
	disabledMessages_.clear();
	activeAnimation_.clear();

	AttributeBase::releaseModel(); // DEBUG

	gb_VisGeneric->SetUseLogicQuant(true);
	
	TurnOnTransparentTracking(false);

	enableEventChecking_ = false;

	PlayerVect::iterator pi;
	FOR_EACH(Players, pi){
		(*pi)->setTriggersDisabled();
		(*pi)->Quant();
		(*pi)->killAllUnits();
		(*pi)->Quant();
		(*pi)->Quant();
	}

	interpolationQuant();

    streamLock.lock();
	streamCommand.process(0);
	streamCommand.clear();
	streamInterpolator.clear();
	streamLock.unlock();

	delete crashSystem;
	crashSystem = 0;

	MTAuto lock(fowModelsLock_);
	vector<cObject3dx*>::iterator obj;
	FOR_EACH(fowModels, obj)
		RELEASE(*obj);

	UI_BackgroundScene::instance().setSky(0);

	uiStreamCommand.execute();

	streamLogicCommand.process(0);
	streamLogicCommand.clear();
	streamLogicPostCommand.process(0);
	streamLogicPostCommand.clear();

	clearDeletedUnits(true);
	
	FOR_EACH(Players, pi)
		delete *pi;
	Players.clear();

	delete environment;
	environment = 0;

	minimap().clearEvents();

	uiStreamCommand.execute();

	streamLogicCommand.process(0);
	streamLogicCommand.clear();
	streamLogicPostCommand.process(0);
	streamLogicPostCommand.clear();

	minimap().clearEvents();

	FogOfWarMaps::iterator i;
	FOR_EACH(fogOfWarMaps_, i)
		delete i->second;

	delete pathFinder;
	pathFinder = 0;

	delete normalMap;
	normalMap = 0;

	delete windMap;
	windMap = 0;

	delete circleShow_;
	circleShow_ = 0;

	RELEASE(circleManager_);
	RELEASE(circleManagerTeam_);
	RELEASE(terMapPoint);
	terScene->DeleteAutoObject();
//overloading	terScene->Compact();

	UI_Dispatcher::instance().reset();

	//Например ктото не удалил эффект, который обращается к воде, которой уже нет.
	//Так будут только ассерты и лики памяти, но не более.
	delete cameraManager;
	cameraManager = 0;
	RELEASE(terScene);
	terScene = gb_VisGeneric->CreateScene();
	cameraManager = new CameraManager(terScene->CreateCamera());

	delete secondMap;
	secondMap = 0;

	DebugPrm::instance().saveLibrary();

	xassert(this == universe());
	universe_ = 0;
    
	DBGCHECK;
}

//----------------------------------------
struct MapUpdateOperator
{
	int x0,y0,x1,y1;

	MapUpdateOperator(int _x0,int _y0,int _x1,int _y1)
	{
		x0 = _x0;
		x1 = _x1;
		y0 = _y0;
		y1 = _y1;
	}

	void operator()(UnitBase* p)
	{		
		if(!p->dead())
			p->mapUpdate(x0,y0,x1,y1);
	}
};

struct MapUpdateSourceOperator
{
	int x0,y0,x1,y1;

	MapUpdateSourceOperator(int _x0,int _y0,int _x1,int _y1)
	{
		x0 = _x0;
		x1 = _x1;
		y0 = _y0;
		y1 = _y1;
	}

	void operator()(SourceBase* p)
	{		
		if(!p->dead())
			p->mapUpdate(x0,y0,x1,y1);
	}
};

void Universe::ClearOverpatchingFOW()
{
	PlayerVect::iterator pi;
	UnitList::const_iterator uit;
	FOR_EACH(Players, pi){
		CUNITS_LOCK(*pi);
		FOR_EACH((*pi)->units(), uit)
			if((*uit)->model())
				(*uit)->model()->ClearAttr(ATTRUNKOBJ_IGNORE);
	}
	
	MTAuto lock(fowModelsLock_);
	vector<cObject3dx*>::iterator obj;
	FOR_EACH(fowModels, obj)
		RELEASE(*obj);
	fowModels.clear();
}

FogOfWarMap* Universe::createFogOfWarMap(int clan)
{
	if(gameType() & GAME_TYPE_BATTLE){
		if(!fogOfWarMaps_.exists(clan))
			fogOfWarMaps_[clan] = environment->fogOfWar()->CreateMap();
		return fogOfWarMaps_[clan];
	}
	else{
		int index = fogOfWarMaps_.empty() ? 0 : (fogOfWarMaps_.end() - 1)->first + 1;
		fogOfWarMaps_[index] = environment->fogOfWar()->CreateMap();
		return fogOfWarMaps_[index];
	}
}

void Universe::fowQuant()
{
	const FogOfWarMap* fow = activePlayer()->fogOfWarMap();
	if(!fow)
		return;

	{
		MTAuto lock(fowModelsLock_);
		for(vector<cObject3dx*>::iterator obj = fowModels.begin(); obj != fowModels.end();){
			const Vect3f pos = (*obj)->GetPosition().trans();
			if(fow->getFogState(pos.xi(), pos.yi()) == FOGST_NONE){
				//RELEASE(*obj);
				streamLogicCommand.set(fCommandRelease, *obj);
				obj = fowModels.erase(obj);
			}
			else
				++obj;
		}
	}

	FogOfWarMaps::iterator i;
	FOR_EACH(fogOfWarMaps_, i)
		i->second->quant();
}

void Universe::addFowModel( cObject3dx* model )
{
	MTAuto lock(fowModelsLock_);
	cObject3dx *fowModel = new cObject3dx(model);
	fowModel->SetScene(terScene);
	fowModel->Attach();
	fowModels.push_back(fowModel);
}

void fCommandCalcLogicDelay1(XBuffer& stream)
{
	int startClock;
	stream.read(startClock);
	statistics_add(logicDelay1, xclock() - startClock);
}

void fCommandCalcLogicDelay2(XBuffer& stream)
{
	int startClock;
	stream.read(startClock);
	statistics_add(logicDelay2, xclock() - startClock);
}

void Universe::Quant()
{
	start_timer_auto();

	streamLogicCommand.set(fCommandCalcLogicDelay1) << xclock();

	DBGCHECK;

	profiler_quant(quantCounter() + 1);
	gb_VisGeneric->SetLogicQuant(quantCounter());

	streamLogicCommand.put(streamLogicPostCommand);
	streamLogicPostCommand.clear();

	clearDeletedUnits(false);

	global_time.next_frame();

	show_dispatcher.clear();

	pathFinder->recalcPathFinder();

	windMap->quant();

	if(!isUnderEditor()){
		triggerQuant(false);

		start_timer(0);
		PlayerVect::iterator pi;
		FOR_EACH(Players, pi)
			(*pi)->CollisionQuant();
		stop_timer(0);
	}

	start_timer(1);
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi)
		(*pi)->Quant();
	stop_timer(1);

	if(crashSystem)
		crashSystem->moveQuant(logicPeriodSeconds);
	
	start_timer(2);
	fowQuant();
	stop_timer(2);

	start_timer(3);
	vector<sRect>::iterator rc;
	FOR_EACH(vMap.changedAreas,rc){
		normalMap->updateRect(rc->x,rc->y,rc->sx,rc->sy);
		windMap->updateRect(rc->x,rc->y,rc->sx,rc->sy);
		pathFinder->updateRect(rc->x,rc->y,rc->sx,rc->sy); // Наверно необязательно обновлять тут..!
		crashSystem->updateRegion(rc->x,rc->y,rc->x+rc->sx,rc->y+rc->sy);

		MapUpdateOperator unit_op((int)(rc->x),(int)(rc->y),(int)(rc->x + rc->sx),(int)(rc->y + rc->sy));
		unitGrid.Scan(unit_op.x0, unit_op.y0, unit_op.x1, unit_op.y1, unit_op);
		unitGrid.scanAgain();

		if(!isUnderEditor()){
			MapUpdateSourceOperator sourceOp((int)(rc->x),(int)(rc->y),(int)(rc->x + rc->sx),(int)(rc->y + rc->sy));
			environment->sourceGrid.Scan(sourceOp.x0, sourceOp.y0, sourceOp.x1, sourceOp.y1, sourceOp);
			environment->sourceGrid.scanAgain();
		}
	}
	
	list<Rectf>::iterator tmi;
	FOR_EACH(tileMapUpdateRegions,tmi){
		MapUpdateOperator unit_op((int)(tmi->left()),(int)(tmi->top()),(int)(tmi->right()),(int)(tmi->bottom()));
		unitGrid.Scan(unit_op.x0, unit_op.y0, unit_op.x1, unit_op.y1, unit_op);
		unitGrid.scanAgain();

		normalMap->updateRect((int)(tmi->left()),(int)(tmi->top()),(int)(tmi->width()),(int)(tmi->height()));
		pathFinder->updateRect((int)(tmi->left()),(int)(tmi->top()),(int)(tmi->width()),(int)(tmi->height()));
	}
	stop_timer(3);
	
	unitGrid.scanNext();
	vMap.changedAreas.clear();
	tileMapUpdateRegions.clear();

	environment->logicQuant();

	if(!isUnderEditor())
        terToolsDispatcher.quant();
	
	vMap.renderQuant();

	//interpolationQuant();

	if(debugShowEnabled)
		showDebugInfo();

	streamLogicCommand.set(fCommandCalcLogicDelay2) << xclock();
}

void Universe::interpolationQuant()
{
	streamLock.lock();

	quant_counter_++;

	visibleUnits_.swap(visibleUnits);
	visibleUnits_.clear();

	streamCommand.put(streamLogicCommand);
	streamLogicCommand.clear();

	streamInterpolator.clear();
	streamInterpolator.put(streamLogicInterpolator);
	streamLogicInterpolator.clear();

	streamLock.unlock();
}
 
void Universe::graphQuant(float dt)
{
	if(isUnderEditor()){
		PlayerVect::iterator pi;
		FOR_EACH(Players, pi)
			(*pi)->showEditor();
	}

	if(debugShowEnabled)
		show_dispatcher.draw();
}

UnitReal* Universe::findUnit(const AttributeBase* attr)
{
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi){
		UnitReal* unit = (*pi)->findUnit(attr);
		if(unit)
			return unit;
	}
	return 0;
}

UnitReal* Universe::findUnit(const AttributeBase* attr, const Vect2f& nearPosition, float distanceMin, const ConstructionState& state,  bool onlyVisible)
{
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi){
		UnitReal* unit = (*pi)->findUnit(attr, nearPosition, distanceMin, state, onlyVisible);
		if(unit)
			return unit;
	}
	return 0;
}

UnitReal* Universe::findUnitByLabel(const char* label)
{
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi){
		UnitReal* unit = (*pi)->findUnitByLabel(label);
		if(unit)
			return unit;
	}
	return 0;
}

bool Universe::forcedDefeat(int playerID)
{
	Player* player = findPlayer(playerID);
	player->setDefeat();
	return true;
}

//----------------------------------------------
void Universe::refreshAttribute()
{
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi)
		(*pi)->refreshAttribute();
}

//---------------------------------------------------------
STARFORCE_API void Universe::serialize(Archive& ar)
{
	start_timer_auto();

	UnitID::serializeCounter(ar);

	if(ar.isOutput()){
		PlayerVect::iterator pi;
		FOR_EACH(Players, pi){
			if(!(*pi)->isWorld() && (*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER){
				XBuffer name;
				name < "player" <= (*pi)->shuffleIndex();
				ar.serialize(**pi, name, 0);
			}
		}
		int auxCounter = 0;
		FOR_EACH(Players, pi){
			if(!(*pi)->isWorld() && (*pi)->auxPlayerType() != AUX_PLAYER_TYPE_ORDINARY_PLAYER){
				XBuffer name;
				name < "auxPlayer" <= auxCounter++;
				ar.serialize(**pi, name, 0);
			}
		}
		ar.serialize(*worldPlayer(), "world", 0);
	}
	else{
		int auxCounter = 0;
		PlayerVect::iterator pi;
		FOR_EACH(Players, pi){
			XBuffer name;
			if(!(*pi)->isWorld())
				if((*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER)
					name < "player" <= (*pi)->shuffleIndex();
				else
					name < "auxPlayer" <= auxCounter++;
			else
				name < "world";
			ar.serialize(**pi, name, 0);
		}
	}

	if(userSave()){
		ar.serialize(intVariables_, "intVariables", 0);
		if(environment->fogOfWar()){
			FogOfWarMaps::iterator i;
			FOR_EACH(fogOfWarMaps_, i){
				XBuffer name;
				name < "fogOfWarMap" <= i->first;
				ar.serialize(*i->second, name, 0);
			}
		}
	}
}

STARFORCE_API bool Universe::universalSave(const MissionDescription& mission, bool userSave, Archive& oa)
{
	SECUROM_MARKER_HIGH_SECURITY_ON(8);

	userSave_ = userSave;

#ifndef _FINAL_VERSION_
	if(isUnderEditor())
		collectWorldSheets();
#endif

	MissionDescription md = mission;
	md.resetToSave(userSave);
	
	oa.serialize(md, "header", 0);
	vMap.saveGameMap(oa);
	oa.serialize(*cameraManager, "camera", 0);
	oa.serialize(*this, "universe", 0);
	oa.serialize(*environment, "environment", 0);

	SECUROM_MARKER_HIGH_SECURITY_OFF(8);
	return oa.close();
}

void Universe::relaxLoading()
{
	AttributeLibrary::Map::const_iterator ai;
	FOR_EACH(AttributeLibrary::instance().map(), ai){
		if(ai->get())
			ai->get()->preload();
	}

	for(int i = 0; i < AttributeProjectileTable::instance().size(); i++)
		AttributeProjectileTable::instance()[i].preload();

	PlayerVect::iterator pi;
	FOR_EACH(Players, pi)
		(*pi)->relaxLoading();
}

//------------------------------------------
void Universe::showDebugInfo()
{
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi)
		(*pi)->showDebugInfo();
	
	if(show_second_map)
		for(int i = 0; i < secondMap->sizeX(); i++)
			for(int j = 0; j < secondMap->sizeY(); j++) {
				int h = (*universe()->secondMap)(i,j);
				show_line(Vect3f(secondMap->m2w(i), secondMap->m2w(j), 0), Vect3f(secondMap->m2w(i), secondMap->m2w(j), h), RED);
			}

	if(show_pathfinder_map)
		pathFinder->showDebugInfo();

	if(show_normal_map)
		normalMap->showDebugInfo();

	if(show_wind_map)
		windMap->showDebugInfo();

	environment->showDebug();
	
	if(showDebugWaterHeight)
		environment->water()->showDebug();
	
	if(showDebugCrashSystem)
		crashSystem->showDebugInfo();
}
void Universe::drawDebug2D() const
{
}
//----------------------------------------------------------

Player* Universe::addPlayer(const PlayerData& playerData)
{
	Player* player = new Player(playerData);
	Players.push_back(player);
	return player;
}

void Universe::updateSkinColor()
{
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi)
		(*pi)->updateSkinColor();
}

//-----------------------------------------------------
void Universe::checkEvent(const Event& event)
{
	start_timer_auto();

	if(enableEventChecking_){
		switch(event.type()){
		case Event::CHANGE_PLAYER_AI: {
			const EventChangePlayerAI& eventAI = safe_cast_ref<const EventChangePlayerAI&>(event);
			Player* player = findPlayer(eventAI.playerID());
			player->setAI(!player->isAI());
			break;
			}
		}

		PlayerVect::iterator pi;
		FOR_EACH(Players, pi)
			(*pi)->checkEvent(event);
	}
	else
		minimap().checkEvent(event);
}

void Universe::triggerQuant(bool pause)
{
	start_timer_auto();

	PlayerVect::iterator pi;
	FOR_EACH(Players, pi)
		(*pi)->triggerQuant(pause);
}

void Universe::deleteUnit(UnitBase* unit)
{
	MTL();
	int quant=quantCounter();

	if(deletedUnits_.empty() || deletedUnits_.back().quant!=quant)
	{
		DeleteData d;
		d.quant=quant;
		deletedUnits_.push_back(d);
	}

	DeleteData& dd=deletedUnits_.back();
	dd.unit.push_back(unit);
}

void Universe::clearDeletedUnits(bool delete_all)
{
	const int wait_to_delete=10;//Количество квантов, котороё ждётся
							//в логическом кванте, прежде, чем удалить объект

	if(!delete_all){
		if(useHT_){
			while(gb_VisGeneric->GetGraphLogicQuant() < quantCounter() - wait_to_delete + 2)
				Sleep(10);
		}
		//else{
		//	xassert(!(gb_VisGeneric->GetGraphLogicQuant()<quantCounter() - wait_to_delete + 2));
		//}
	}

	list<DeleteData>::iterator itd=deletedUnits_.begin();
	
	while(itd!=deletedUnits_.end()){
		if(!delete_all && itd->quant > quantCounter() - wait_to_delete){
			itd++;
			continue;
		}

		list<UnitBase*>& lst = itd->unit;

		list<UnitBase*>::iterator it;
		FOR_EACH(lst,it){
			UnitBase* p=*it;
			delete p;
		}
		lst.clear();

		itd = deletedUnits_.erase(deletedUnits_.begin());
	}
}

void Universe::setControlEnabled(bool controlEnabled) 
{
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi)
		(*pi)->setControlEnabled(controlEnabled);
}

void Universe::deselectAll()
{
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi){
		UnitList::const_iterator ui;
		FOR_EACH((*pi)->units(), ui){
			if(*ui && (*ui)->alive())
				(*ui)->setSelected(false);
		}
	}
}

void Universe::deleteSelected()
{
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi){
		UnitList::const_iterator ui;
		FOR_EACH((*pi)->units(), ui)
			if((*ui)->selected())
				(*ui)->Kill();
	}
}

void Universe::collectWorldSheets()
{
	bool changed = false;
	PlayerVect::iterator pi;
	FOR_EACH(Players, pi){
		UnitList::const_iterator ui;
		FOR_EACH((*pi)->units(), ui){
			UnitList::const_iterator uj;
			FOR_EACH((*pi)->units(), uj){
				if(*ui != *uj && (*ui)->position2D().distance2((*uj)->position2D()) < 4.f && &(*ui)->attr() == &(*uj)->attr() && !(*ui)->dead() && !(*uj)->dead()){
					(*ui)->Kill();
					changed = true;
					break;
				}
			}
		}

		if((*pi)->isWorld())
			break;

		UnitList tmpUnitList = (*pi)->units();
		FOR_EACH(tmpUnitList, ui)
			if((*ui)->attr().isEnvironment() || (*ui)->attr().isResourceItem() || (*ui)->attr().isInventoryItem()){
				(*ui)->changeUnitOwner(worldPlayer());
				changed = true;
			}
	} 

	if(changed)
		FOR_EACH(Players, pi){
			(*pi)->Quant();
			(*pi)->Quant();
		}
}

void Universe::TurnOnTransparentTracking(bool on)
{
	if (on) {
		if (!transparent_tracking) {
			transparent_tracking = new cTransparentTracking();
			PlayerVect::iterator iplayer;
			UnitList::const_iterator iunit;
			FOR_EACH(Players, iplayer)
				FOR_EACH((*iplayer)->units(), iunit)
				transparent_tracking->RegisterUnit(*iunit);
		}
	}
	else if (transparent_tracking) {
		delete transparent_tracking;
		transparent_tracking = NULL;
	}
}

void Universe::setActivePlayer(int playerID, int cooperativeIndex)
{
	if(activePlayer())
		activePlayer()->SetDeactivePlayer();

	active_player_ = findPlayer(playerID);
	activePlayer()->setActivePlayer(cooperativeIndex);

	UI_Dispatcher::instance().clearTexts();
	UI_LogicDispatcher::instance().disableDirectControl();

	if(circleManager_){
		circleManager_->SetParam(active_player_->race()->circle);
		circleManager_->SetLegionColor(active_player_->unitColor());
	}
	if(circleManagerTeam_){
		circleManagerTeam_->SetParam(active_player_->race()->circleTeam);
		circleManagerTeam_->SetLegionColor(active_player_->unitColor());
	}

	if(environment->fogOfWar()){
		xassert(active_player_->fogOfWarMap() != 0);
		environment->fogOfWar()->SelectMap(active_player_->fogOfWarMap());
	}
}

void Universe::exportPlayers(PlayerDataVect& playerDataVect) const 
{
	PlayerVect::const_iterator pi;
	FOR_EACH(Players, pi)
		if(!(*pi)->isWorld()){
			playerDataVect.push_back(PlayerDataEdit());
			(*pi)->getPlayerData(playerDataVect.back());
		}
}

void Universe::importPlayers(const PlayerDataVect& playerDataVect)
{
	PlayerVect playersOld;
	swap(playersOld, Players);

	for(int i = 0; i < playerDataVect.size(); i++){ 
		PlayerDataEdit playerData = playerDataVect[i];
		Player* player = 0;
		if(playerData.playerID == -1){
			player = new Player(playerData);
		}
		else{
			PlayerVect::iterator pi;
			FOR_EACH(playersOld, pi){ 
				if(playerData.playerID == (*pi)->playerID() && !(*pi)->isWorld()){
					player = *pi;
					playersOld.erase(pi);
					break;
				}
			}
			if(!player)
				player = new Player(playerData);
		}

		XBuffer name;
		name < "Player" <= i;
		playerData.setName(name);
		playerData.playerID = playerData.shuffleIndex = i;
		player->setPlayerData(playerData);
		Players.push_back(player);
	}

	Players.push_back(playersOld.back());

	collectWorldSheets();
}

struct SourcesAhchorsSerializer {
	void serialize(Archive& ar) {
		environment->serializeSourcesAndAnchors(ar);
	}
};

bool Universe::mergeWorld(const MergeOptions& options) 
{
	XPrmIArchive ia(options.worldFile.c_str());
	
	if(!UnitID::setMergeOffset())
		return false;

	if(options.mergeCameras)
		ia.serialize(*cameraManager, "camera", 0);

	if(options.mergePlayers || options.mergeWorld){
		ia.openStruct("universe", 0, typeid(Universe).name());
		int auxCounter = 0;
		PlayerVect::iterator pi;
		FOR_EACH(Players, pi){
			XBuffer name;
			if(!(*pi)->isWorld()){
				if(!options.mergePlayers)
					continue;
				if((*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER)
					name < "player" <= (*pi)->shuffleIndex();
				else
					name < "auxPlayer" <= auxCounter++;
			}
			else{
				if(!options.mergeWorld)
					continue;
				name < "world";
			}

			(*pi)->killAllUnits();
			(*pi)->Quant();
			(*pi)->Quant();
			ia.serialize(**pi, name, 0);
		}
		ia.closeStruct("universe");
	}

	if(options.mergeSourcesAndAnchors)
		ia.serialize(SourcesAhchorsSerializer(), "environment", 0);
	
	return true;
}


void Universe::setInterfaceEnabled(bool interfaceEnabled)
{
	if(interfaceEnabled_ != interfaceEnabled){
		interfaceEnabled_ = interfaceEnabled;
		PlayerVect::iterator pi;
		FOR_EACH(Players, pi)
			(*pi)->interfaceToggled();
	}
}

int Universe::playersNumber() const
{
	int counter = 0;
	PlayerVect::const_iterator pi;
	FOR_EACH(Players, pi)
		if(!(*pi)->isWorld() && (*pi)->auxPlayerType() == AUX_PLAYER_TYPE_ORDINARY_PLAYER && !(*pi)->isDefeat())
			counter++;
	return counter;
}
