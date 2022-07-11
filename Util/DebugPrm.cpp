#include "stdafx.h"
#include "DebugPrm.h"
#include "EditArchive.h"
#include "XPrmArchive.h"
#include "MultiArchive.h"
#include "RangedWrapper.h"
#include "..\Environment\Environment.h"
#include "..\Terra\vmap.h"

WRAP_LIBRARY(DebugPrm, "DebugPrm", "DebugPrm", "Scripts\\TreeControlSetups\\Debug.dat", 0, false);

bool debugShowEnabled;
bool debugWireFrame;
bool debugShowWatch;
int debug_window_sx;
int debug_window_sy;
ShowDebugRigidBody showDebugRigidBody;
ShowDebugUnitBase showDebugUnitBase;
ShowDebugUnitReal showDebugUnitReal;
ShowDebugUnitInterface showDebugUnitInterface;
ShowDebugLegionary showDebugLegionary;
ShowDebugBuilding showDebugBuilding;
ShowDebugSquad showDebugSquad;
ShowDebugUnitEnvironment showDebugUnitEnvironment;
bool show_environment_type;
ShowDebugPlayer showDebugPlayer;
ShowDebugSource showDebugSource;
ShowDebugWeapon showDebugWeapon;
ShowDebugInterface showDebugInterface;
ShowDebugEffects showDebugEffects;
ShowDebugTerrain showDebugTerrain;

bool showDebugAnchors;
bool showDebugWaterHeight;
int show_filth_debug;
float show_vector_zmin;
float show_vector_zmax;
int mt_interface_quant;
int ht_intf_test;
bool disableHideUnderscore;
int terMaxTimeInterval;
int logicTimePeriod = 100;
float logicTimePeriodInv = 1.0f/logicTimePeriod;
float logicPeriodSeconds = logicTimePeriod / 1000.f;
int debug_show_briefing_log;
int debug_show_mouse_position;
int debug_show_energy_consumption;
int debug_allow_mainmenu_gamespy;
int debug_allow_replay;
bool show_second_map;
bool show_pathfinder_map;
bool show_pathtracking_map_for_selected;
bool show_normal_map;
bool show_wind_map;
bool debug_stop_time;
bool cameraGroundColliding;
bool showDebugCrashSystem;
bool showDebugNumSounds;
bool debugDisableFogOfWar;
bool debugDisableSpecialExitProcess;

ShowDebugRigidBody::ShowDebugRigidBody()
{
	boundingBox = 0;
	mesh = 0;
	radius = 0;
	inscribedRadius = 0;
	boundRadius = 0;
	wayPoints = 0;
	autoPTPoint = 0;
	contacts = 0;
	movingDirection = 0;
	localFrame = 0;
	acceleration = 0;
	linearScale = 1;
	angularScale = 200;
	rocketTarget = 0;
	velocityValue = 0;
	ptVelocityValue = false;
	average_movement = 0;
	showPathTracking_ObstaclesLines = false;
	showPathTracking_ImpassabilityLines = false;
	showPathTracking_VehicleRadius = false;
	ground_colliding = false;
	onLowWater = false;
	target = false;
	showMode = false;
	showDebugMessages = false;
	debugMessagesCount = 5;
	showDebugNumSounds = false;

}

void ShowDebugRigidBody::serialize(Archive& ar)
{
	ar.serialize(boundingBox, "boundingBox", 0);
	ar.serialize(mesh, "mesh", 0);
	ar.serialize(radius, "radius", 0);
	ar.serialize(inscribedRadius, "inscribedRadius", 0);
	ar.serialize(boundRadius, "boundRadius", 0);
	ar.serialize(wayPoints, "wayPoints", 0);
	ar.serialize(autoPTPoint, "autoPTPoint", 0);
	ar.serialize(contacts, "contacts", 0);
	ar.serialize(movingDirection, "movingDirection", 0);
	ar.serialize(localFrame, "localFrame", 0);
	ar.serialize(velocity, "velocity", 0);
	ar.serialize(acceleration, "acceleration", 0);
	ar.serialize(linearScale, "linearScale", 0);
	ar.serialize(angularScale, "angularScale", 0);
	ar.serialize(rocketTarget, "rocketTarget", 0);
	ar.serialize(velocityValue, "velocityValue", 0);
	ar.serialize(ptVelocityValue, "ptVelocityValue", 0);
	ar.serialize(average_movement, "average_movement", 0);
	ar.serialize(showPathTracking_ObstaclesLines, "showPathTracking_ObstaclesLines", 0);
	ar.serialize(showPathTracking_ImpassabilityLines, "showPathTracking_ImpassabilityLines", 0);
	ar.serialize(showPathTracking_VehicleRadius, "showPathTracking_VehicleRadius", 0);
	ar.serialize(ground_colliding, "ground_colliding", 0);
	ar.serialize(onLowWater, "onLowWater", 0);
	ar.serialize(target, "target", 0);
	ar.serialize(showMode, "showMode", 0);
	ar.serialize(showDebugMessages, "showDebugMessages", 0);
	ar.serialize(debugMessagesCount, "debugMessagesCount", 0);
}

ShowDebugUnitBase::ShowDebugUnitBase()
{
	radius = 0;
	libraryKey = 0;
	modelName = 0;
	abnormalState = 0;
	effects = 0;
	clan = 0;
	producedPlacementZone = 0;
}

void ShowDebugUnitBase::serialize(Archive& ar)
{
	ar.serialize(radius, "radius", 0);
	ar.serialize(libraryKey, "libraryKey", 0);
	ar.serialize(modelName, "modelName", 0);
	ar.serialize(abnormalState, "abnormalState", 0);
	ar.serialize(effects, "effects", 0);
	ar.serialize(clan, "clan", 0);
	ar.serialize(producedPlacementZone, "producedPlacementZone", 0);
	ar.serialize(lodDistance_,"lodDistance",0);
}

ShowDebugUnitInterface::ShowDebugUnitInterface()
{
	debugString = 0;
}

void ShowDebugUnitInterface::serialize(Archive& ar)
{
	ar.serialize(debugString, "debugString", 0);
}

ShowDebugUnitReal::ShowDebugUnitReal()
{
	target = 0;
	fireResponse = 0;
	toolzer = 0;
	positionValue = 0;
	unitID = 0;
	producedUnitQueue = 0;
	transportSlots = 0;
	docking = 0;
	chain = 0;
	parameters = 0;
	parametersMax = 0;
	waterDamage = 0;
	lavaDamage = 0;
	iceDamage = 0;
	earthDamage = 0;
	attackModes = 0;
	currentChain = 0;
	homePosition = false;
}

void ShowDebugUnitReal::serialize(Archive& ar)
{
	ar.serialize(target, "target", 0);
	ar.serialize(fireResponse, "fireResponse", 0);
	ar.serialize(toolzer, "toolzer", 0);
	ar.serialize(positionValue, "positionValue", 0);
	ar.serialize(unitID, "unitID", 0);
	ar.serialize(producedUnitQueue, "producedUnitQueue", 0);
	ar.serialize(transportSlots, "transportSlots", 0);
	ar.serialize(docking, "docking", 0);
	ar.serialize(chain, "chain", 0);
	ar.serialize(parameters, "parameters", 0);
	ar.serialize(parametersMax, "parametersMax", 0);
	ar.serialize(waterDamage, "waterDamage", 0);
	ar.serialize(iceDamage, "iceDamage", 0);
	ar.serialize(lavaDamage, "lavaDamage", 0);
	ar.serialize(earthDamage, "earthDamage", 0);
	ar.serialize(attackModes, "attackModes", 0);
	ar.serialize(currentChain, "currentChain", 0);
	ar.serialize(homePosition, "homePosition", 0);
}


ShowDebugLegionary::ShowDebugLegionary()
{
	invisibility = 0;
	level = false;
	transport = false;
	resourcerMode = 0;
	resourcerProgress = 0;
	resourcerCapacity = 0;
	trace = 0;
	aimed = 0;
}

void ShowDebugLegionary::serialize(Archive& ar)
{
	ar.serialize(invisibility, "invisibility", 0);
	ar.serialize(level, "level", 0);
	ar.serialize(transport, "transport", 0);
	ar.serialize(resourcerMode, "resourcerMode", 0);
	ar.serialize(resourcerProgress, "resourcerProgress", 0);
	ar.serialize(resourcerCapacity, "resourcerCapacity", 0);
	ar.serialize(trace, "trace", 0);
	ar.serialize(aimed, "aimed", 0);
}

ShowDebugBuilding::ShowDebugBuilding()
{
	status = false;
	basement = false;
}

void ShowDebugBuilding::serialize(Archive& ar)
{
	ar.serialize(status, "status", 0);
	ar.serialize(basement, "basement", 0);
}

ShowDebugSquad::ShowDebugSquad()
{
	position = 0;
	wayPoints = 0;
	umbrella = 0;
	fire_radius = 0;
	sight_radius = 0;
	described_radius = 0;
	attackAction = 0;
	squadToFollow = 0;
	order = 0;
	unitsNumber = 0;
}

void ShowDebugSquad::serialize(Archive& ar)
{
	ar.serialize(position, "position", 0);
	ar.serialize(wayPoints, "wayPoints", 0);
	ar.serialize(umbrella, "umbrella", 0);
	ar.serialize(fire_radius, "fire_radius", 0);
	ar.serialize(sight_radius, "sight_radius", 0);
	ar.serialize(described_radius, "described_radius", 0);
	ar.serialize(attackAction, "attackAction", 0);
	ar.serialize(squadToFollow, "squadToFollow", 0);
	ar.serialize(order, "order", 0);
	ar.serialize(unitsNumber, "unitsNumber", 0);
}

ShowDebugUnitEnvironment::ShowDebugUnitEnvironment()
{
	rigidBody = 0;
	modelName = 0;
	environmentType = 0;
	treeType = 0;
	treeMode = 0;
}

void ShowDebugUnitEnvironment::serialize(Archive& ar)
{
	ar.serialize(rigidBody, "rigidBody", 0);
	ar.serialize(modelName, "modelName", 0);
	ar.serialize(environmentType, "environmentType", 0);
	ar.serialize(treeType, "treeType", 0);
	ar.serialize(treeMode, "treeMode", 0);
}

ShowDebugPlayer::ShowDebugPlayer()
{
	placeOp = 0;
	scanBound = 0;
	resource = 0;
	showSelectedOnly = false;
	showStatistic = false;
	saveLogStatistic = false;
	showSearchRegion = false;
}

void ShowDebugPlayer::serialize(Archive& ar)
{
	ar.serialize(placeOp, "placeOp", 0);
	ar.serialize(scanBound, "scanBound", 0);
	ar.serialize(resource, "resource", 0);
	ar.serialize(showSelectedOnly, "showSelectedOnly", 0);
	ar.serialize(showStatistic, "showStatistic", 0);
	ar.serialize(saveLogStatistic, "saveLogStatistic", 0);
	ar.serialize(showSearchRegion, "showSearchRegion", 0);
}

ShowDebugSource::ShowDebugSource()
{
	enable = 0;
	name = 0;
	label = 0;
	radius = 0;
	axis = false;
	type = 0;
	zoneDamage = 0;
	zoneStateDamage = 0;
	dontShowInfo = false;
	showEnvironmentPoints = false;
	showLightningUnitChainRadius = false;
	sourceCount = 0;
}

void ShowDebugSource::serialize(Archive& ar)
{
	ar.serialize(enable, "enable", 0);
	ar.serialize(name, "name", 0);
	ar.serialize(label, "label", 0);
	ar.serialize(radius, "radius", 0);
	ar.serialize(axis, "axis", 0);
	ar.serialize(type, "type", 0);
	ar.serialize(zoneDamage, "zoneDamage", 0);
	ar.serialize(zoneStateDamage, "zoneStateDamage", 0);
	ar.serialize(dontShowInfo, "dontShowInfo", 0);
	ar.serialize(showEnvironmentPoints, "showEnvironmentPoints", 0);
	ar.serialize(showLightningUnitChainRadius, "showLightningUnitChainRadius", 0);
}				   

ShowDebugWeapon::ShowDebugWeapon()
{
	enable = 0;
	showSelectedOnly = false;
	direction = 0;
	horizontalAngle = 0;
	verticalAngle = 0;
	gripVelocity = 0;
	targetingPosition = false;
	ownerTarget = false;
	ownerSightRadius = false;
	autoTarget = false;
	angleValues = false;
	angleLimits = false;
	parameters = false;
	damage = false;
	load = false;
	fireRadius = false;
	showWeaponID = 0;
}

void ShowDebugWeapon::serialize(Archive& ar)
{
	ar.serialize(enable, "enable", 0);
	ar.serialize(showSelectedOnly, "showSelectedOnly", 0);
	ar.serialize(direction, "direction", 0);
	ar.serialize(horizontalAngle, "horizontalAngle", 0);
	ar.serialize(verticalAngle, "verticalAngle", 0);
	ar.serialize(gripVelocity, "gripVelocity", 0);
	ar.serialize(targetingPosition, "targetingPosition", 0);
	ar.serialize(ownerTarget, "ownerTarget", 0);
	ar.serialize(ownerSightRadius, "ownerSightRadius", 0);
	ar.serialize(autoTarget, "autoTarget", 0);
	ar.serialize(angleValues, "angleValues", 0);
	ar.serialize(angleLimits, "angleLimits", 0);
	ar.serialize(parameters, "parameters", 0);
	ar.serialize(damage, "damage", 0);
	ar.serialize(load, "load", 0);
	ar.serialize(fireRadius, "fireRadius", 0);
	ar.serialize(showWeaponID, "showWeaponID", 0);
}				   

ShowDebugInterface::ShowDebugInterface()
{
	showDebug = false;
	writeLog = 0;
	disableTextures = false;
	background = false;
	screens = false;
	controlBorder = false;
	hoveredControlBorder = false;
	hoveredControlExtInfo = false;
	focusedControlBorder = false;
	marks = false;
	hoverUnitBound = false;
	enableAllNetControls = false;
	showUpMarksCount = false;
	showSelectManager = false;
	cursorReason = false;
	showTransformInfo = false;
	logicDispatcher = false;
}

void ShowDebugInterface::serialize(Archive& ar)
{
	ar.serialize(showDebug, "showDebug", "Показывать интерфейсную информацию");
	ar.serialize(writeLog, "writeLogMode", "Писать в лог");
	ar.serialize(disableTextures, "disableTextures", "Не выводить текстуры");
	ar.serialize(background, "background", "Информация о фоновых 3D моделях");
	ar.serialize(screens, "screens", "Информация о текущих экранах");
	ar.serialize(controlBorder, "controlBorder", "Показывать границы контролов");
	ar.serialize(hoveredControlBorder, "hoveredControlBorder", "Показывать границу и имя контрола под мышкой");
	ar.serialize(hoveredControlExtInfo, "hoveredControlExtInfo", "Показывать расширенную информацию по контролу под мышкой");
	ar.serialize(showTransformInfo, "showTransformInfo", "Информация о трансформации кнопки");
	ar.serialize(focusedControlBorder, "focusedControlBorder", "Показывать границу контрола с фокусом ввода");
	ar.serialize(enableAllNetControls, "enableAllNetControls", "Разрешить менять все настройки миссии");
	ar.serialize(showSelectManager, "selectManager", "Информация о селекте");
	ar.serialize(marks, "marks", "Пометки");
	ar.serialize(hoverUnitBound, "hoverUnitBound", "Показать баунд юнита под мышкой");
	ar.serialize(showUpMarksCount, "showUpMarksCount", "Количество обработчиков взлетающих значений");
	ar.serialize(showAimPosition, "showAimPosition", "Точка прицеливания");
	ar.serialize(cursorReason, "cursorReason", "Причина выбора курсора");
	ar.serialize(logicDispatcher, "logicDispatcher", "Временная инфорация интерфейса");
}

int ShowDebugInterface::getShowUpMarksCount() const
{
	MTG();
	if(environment)
		return environment->showChangeControllers_.size();
	return 0;
}

ShowDebugEffects::ShowDebugEffects()
{
	showName = false;
	axis = false;
}

void ShowDebugEffects::serialize(Archive& ar)
{
	ar.serialize(showName, "showName", 0);
	ar.serialize(axis, "axis", 0);
}

ShowDebugTerrain::ShowDebugTerrain()
{
	showBuildingInfo=false;
	showTerrainSpecialInfo=vrtMap::SSI_NoShow;
}

void ShowDebugTerrain::serialize(Archive& ar)
{
	bool oldShowBuildingInfo=showBuildingInfo;
	int oldShowTerrainSpecialInfo=showTerrainSpecialInfo;
	vrtMap::eShowSpecialInfo ssi, oldssi;
	oldssi=ssi=static_cast<vrtMap::eShowSpecialInfo>(showTerrainSpecialInfo);
	ar.serialize(showBuildingInfo, "showBuildingInfo", 0);
	ar.serialize(ssi, "showTerrainInfo", 0);
	showTerrainSpecialInfo=ssi;
	if(oldShowBuildingInfo!=showBuildingInfo || oldssi!=ssi){
		vMap.toShowDbgInfo(showBuildingInfo);
		vMap.toShowSpecialInfo(ssi);
		vMap.WorldRender();
	}

}

DebugPrm::DebugPrm()
{	
	saveTextOnly_ = true;

	debugDamage = 99.5f;
	debugDamageArmor = 500.f;
	showNetStat = false;
	enableKeyHandlers = 0;
	forceDebugKeys = 0;

	debugWireFrame = 0;
	debugShowWatch = 0;
	debugShowEnabled = 0;
	debug_window_sx = 300;
	debug_window_sy = 400;
	show_environment_type = 0;
	show_filth_debug = 0;
	show_vector_zmin = 200;
	show_vector_zmax = 4000;
	mt_interface_quant = 1;
	ht_intf_test = 0;
	disableHideUnderscore = true;
	terMaxTimeInterval = 100;
	debug_show_briefing_log = 0;
	debug_show_mouse_position = 0;
	debug_show_energy_consumption = 0;
	debug_allow_mainmenu_gamespy = 0;
	debug_allow_replay = 0;
	show_second_map = false;
	show_pathfinder_map = false;
	show_pathtracking_map_for_selected = false;
	show_normal_map = false;
	show_wind_map = false;
	debug_stop_time = false;
	showDebugWaterHeight = false;
	showDebugAnchors = false;
	cameraGroundColliding = true;
	showDebugCrashSystem = false;
	debugDisableFogOfWar = false;
	debugDisableSpecialExitProcess = false;
}

void DebugPrm::serialize(Archive& ar)
{
#ifndef _FINAL_VERSION_
	ar.serialize(debugShowEnabled, "debugShowEnabled", 0);
	ar.serialize(enableKeyHandlers, "enableKeyHandlers", 0);
	ar.serialize(forceDebugKeys, "forceDebugKeys", 0);
	ar.serialize(debugDisableFogOfWar, "disableFogOfWar", 0);
#endif
	
#ifdef POLYGON_LIMITATION
	ar.serialize(polygon_limitation_max_polygon, "polygon_limitation_max_polygon", 0);
	ar.serialize(polygon_limitation_max_polygon_simply3dx, "polygon_limitation_max_polygon_simply3dx", 0);
#endif

	ar.serialize(showDebugRigidBody, "showDebugRigidBody", 0);
	ar.serialize(showDebugUnitBase, "showDebugUnitBase", 0);
	ar.serialize(showDebugUnitInterface, "showDebugUnitInterface", 0);
	ar.serialize(showDebugUnitReal, "showDebugUnitReal", 0);
	ar.serialize(showDebugLegionary, "showDebugLegionary", 0);
	ar.serialize(showDebugBuilding, "showDebugBuilding", 0);
	ar.serialize(showDebugSquad, "showDebugSquad", 0);
	ar.serialize(showDebugUnitEnvironment, "showDebugUnitEnvironment", 0);
	ar.serialize(showDebugPlayer, "showDebugPlayer", 0);
	ar.serialize(showDebugSource, "showDebugSource", 0);
	ar.serialize(showDebugWeapon, "showDebugWeapon", 0);
	ar.serialize(showDebugInterface, "showDebugInterface", 0);
	ar.serialize(showDebugEffects, "showDebugEffects", 0);
	ar.serialize(showDebugTerrain, "showDebugTerrain", 0);
	
	ar.serialize(debugWireFrame, "debugWireFrame", 0);
	ar.serialize(debugShowWatch, "debugShowWatch", 0);
	ar.serialize(debug_window_sx, "debug_window_sx", 0);
	ar.serialize(debug_window_sy, "debug_window_sy", 0);
	ar.serialize(show_environment_type, "show_environment_type", 0);
	ar.serialize(show_filth_debug, "show_filth_debug", 0);
	ar.serialize(show_vector_zmin, "show_vector_zmin", 0);
	ar.serialize(show_vector_zmax, "show_vector_zmax", 0);
	ar.serialize(mt_interface_quant, "mt_interface_quant", 0);
	ar.serialize(ht_intf_test, "ht_intf_test", 0);
	ar.serialize(disableHideUnderscore, "disableHideUnderscore", 0);
	ar.serialize(debug_show_briefing_log, "debug_show_briefing_log", 0);
	ar.serialize(debug_show_mouse_position, "debug_show_mouse_position", 0);
	ar.serialize(debug_show_energy_consumption, "debug_show_energy_consumption", 0);
	ar.serialize(debug_allow_mainmenu_gamespy, "debug_allow_mainmenu_gamespy", 0);
	ar.serialize(debug_allow_replay, "debug_allow_replay", 0);
	ar.serialize(show_second_map, "show_second_map", 0);
	ar.serialize(show_pathfinder_map, "show_pathfinder_map", 0);
	ar.serialize(show_pathtracking_map_for_selected, "show_pathtracking_map_for_selected", 0);
	ar.serialize(show_normal_map, "show_normal_map", 0);
	ar.serialize(show_wind_map, "show_wind_map", 0);
	ar.serialize(showDebugWaterHeight, "showDebugWaterHeight", 0);
	ar.serialize(showDebugCrashSystem, "showDebugCrashSystem", 0);
	ar.serialize(showDebugAnchors, "showDebugAnchors", 0);
	ar.serialize(debug_stop_time, "debug_stop_time", 0);
	ar.serialize(cameraGroundColliding, "cameraGroundColliding", 0);

	ar.serialize(terMaxTimeInterval, "terMaxTimeInterval", 0);
	ar.serialize(logicTimePeriod, "logicTimePeriod", 0);
	
	ar.serialize(debugDamage, "debugDamage", 0);
	ar.serialize(debugDamageArmor, "debugDamageArmor", 0);
	ar.serialize(showNetStat, "showNetStat", 0);
	ar.serialize(showDebugNumSounds,"showDebugNumSounds",0);

	ar.serialize(debugDisableSpecialExitProcess, "disableTriggeredExit", 0);
		
	logicTimePeriodInv = 1.0f/logicTimePeriod;
	logicPeriodSeconds = logicTimePeriod / 1000.f;
}


