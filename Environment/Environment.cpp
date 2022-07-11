#include "StdAfx.h"
#include "Environment.h"
#include "Terra\vmap.h"
#include "Render\src\Gradients.h"
#include "Serialization\Serialization.h"
#include "Serialization\Dictionary.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\ResourceSelector.h"
#include "Serialization\XPrmArchive.h"

#include "Water\Waves.h"
#include "Water\Fallout.h"
#include "Water\WaterWalking.h"
#include "Water\CoastSprites.h"
#include "Water\CloudShadow.h"
#include "Water\WaterGarbage.h"

#include "Render\src\LensFlare.h"
#include "Render\src\Scene.h"
#include "VistaRender\postEffects.h"
#include "Render\src\Grass.h"
#include "Render\src\CChaos.h"
#include "Render\Src\VisGeneric.h"

#include "Render\src\MultiRegion.h"
#include "Render\Src\FogOfWar.h"
#include "Render\Src\TileMap.h"
#include "Water\SkyObject.h"
#include "Water\FallLeaves.h"
#include "VistaRender\Flash.h"
#include "Physics\NormalMap.h"
#include "Physics\WindMap.h"
#include "Console.h"
#include "DebugPrm.h"
#include "DebugUtil.h"
#include "Serialization\EnumDescriptor.h"
#include "Units\ShowChangeController.h"
#include "VistaRender\FieldOfView.h"

#include "UserInterface\GameLoadManager.h"

namespace{
	ResourceSelector::Options presetOptions("*.set", "Scripts\\Content\\Presets", "", false, false);
	ResourceSelector::Options textureOptions("*.tga", "Resource\\TerrainData\\Textures");
}

BEGIN_ENUM_DESCRIPTOR(CoastSpritesMode, "Режимы прибрежных спрайтов")
REGISTER_ENUM(CSM_NOTHING, "Отключено")
REGISTER_ENUM(CSM_MOVING, "Двигающиеся спрайты")
REGISTER_ENUM(CSM_SIMPLE, "Неподвижные спрайты")
END_ENUM_DESCRIPTOR(CoastSpritesMode)

BEGIN_ENUM_DESCRIPTOR(Outside_Environment, "Внешняя среда")
REGISTER_ENUM(ENVIRONMENT_NO, "ничего")
REGISTER_ENUM(ENVIRONMENT_WATER, "вода")
REGISTER_ENUM(ENVIRONMENT_CHAOS, "хаос")
REGISTER_ENUM(ENVIRONMENT_EARTH, "земля")
END_ENUM_DESCRIPTOR(Outside_Environment)

Environment* environment=0;

bool Environment::flag_ViewWaves=true;
bool Environment::flag_EnableTimeFlow=false;

Environment::Environment(cScene* scene, cTileMap* tileMap, bool isWater, bool isFogOfWar, bool isTemperature)
: chaosWorldGround0("Scripts\\resource\\Textures\\WorldGround.tga")
, chaosWorldGround1("Scripts\\resource\\Textures\\WorldGround01.tga")
, chaosOceanBump("Scripts\\resource\\Textures\\OceanBump.tga")
, env_earth_texture("Scripts\\resource\\Textures\\Ground_018.tga")
, water_ice_snow_texture("Scripts\\Resource\\balmer\\snow.tga")
, water_ice_bump_texture("Scripts\\Resource\\balmer\\snow_bump.tga")
, water_ice_cleft_texture("Scripts\\Resource\\balmer\\ice_cleft.tga")

, fogOfWar_(0)
, fog_enable_(true)
, fogTempDisabled_(false)

, water_(0)
, waterBubble_(0)
, pCoastSprite(0)
, fixedWaves_(0)
, grassMap(0)
, chaos(0)
, env_earth(0)
, temperature_(0)
{
	environment = this;
	scene_ = scene;
	tileMap_ = tileMap;
	ice_snow_texture="Scripts\\Resource\\balmer\\snow.tga";
	ice_bump_texture="Scripts\\Resource\\balmer\\snow_bump.tga";

	presetName_ = "Scripts\\Content\\Presets\\global.set";
	presetLoaded_ = false;

	outside_ = ENVIRONMENT_WATER;
	outsideHeight_ = 0;

	fog_start_ = 1000.f;
	fog_end_ = 1400.f;
	height_fog_circle_ = 1000;
	
	game_frustrum_z_min_ = 30.f;
	game_frustrum_z_max_vertical_ = 4000.f;
	game_frustrum_z_max_horizontal_ = 4000.f;
	hideSmoothly_ = true;
	
	hideByDistanceFactor_ = 40.f; 
	hideByDistanceRange_ = 100.f;
	
	effectHideByDistance_ = true;
	effectNearDistance_ = 50.f;
	effectFarDistance_ = 1200.f;
	
	dayTimeScale_ = 250.f;
	nightTimeScale_ = 500.f;
	
	environmentTime_ = new EnvironmentTime(scene_);

	DofParams = Vect2f(100,1000);
	dofPower = 4.f;
	enableDOF = false;

	if(isWater){
		water_ = new cWater;
		water_->Init();

		waterBubble_ = new cWaterBubble(water_);
		scene_->AttachObj(water_);
		scene_->AttachObj(waterBubble_);
	}

	PEManager_ = new PostEffectManager();
	if(PEManager_){
		PEManager_->init();

		enableBloom = false;
		bloomLuminance = 0;

//		if(PostEffectUnderWater* eff = (PostEffectUnderWater*)PEManager_->getEffect(PE_UNDER_WATER))
//			eff->setWater(water_);
		underWaterColor = Color4f(0,0,0.2f);
		underWaterFogColor = Color4f(0,0,1.f);
		underWaterFogPlanes = Vect2f(100.f,1000.f);
		underWaterSpeedDistortion = 10;
		underWaterAlways = false;
	}

	flash_ = new Flash(PEManager_);

	minimapWaterColor_ = Color4f::BLUE;

	minimapZonesAlpha_ = 1.f;

	grassMap = 0;
	grassMap = new GrassMap();
	if(grassMap){
		string path = vMap.getTargetName("");
		grassMap->Init(path.c_str());
		scene_->AttachObj(grassMap);
	}

	if(isFogOfWar)
		fogOfWar_ = scene_->CreateFogOfWar();

	cloud_shadow = new cCloudShadow;
	scene_->AttachObj(cloud_shadow);

	if(isTemperature && water_){
		temperature_=new cTemperature;
		temperature_->Init(Vect2i((int)vMap.H_SIZE,(int)vMap.V_SIZE),5,water_);
		scene_->AttachObj(temperature_);
	}

	lensFlare_ = new LensFlareRenderer();
	scene_->AttachObj(lensFlare_);

	fallout_ = new cFallout;
	fallout_->Init(0.5, water_, temperature_);
	scene_->AttachObj(fallout_);

	if(water_){
		pCoastSprite = new cCoastSprites(water_,temperature_);
		scene_->AttachObj(pCoastSprite);
		fixedWaves_ = new cFixedWavesContainer(water_,temperature_);
		scene_->AttachObj(fixedWaves_);
	}

	fallLeaves_ = new cFallLeaves;
	scene_->AttachObj(fallLeaves_);

	normalMap = new NormalMap(vMap.H_SIZE,vMap.V_SIZE);
	windMap = new WindMap(vMap.H_SIZE,vMap.V_SIZE);

	fieldOfViewMap_ = new FieldOfViewMap(vMap.H_SIZE, vMap.V_SIZE);
	scene_->AttachObj(fieldOfViewMap_);

	loadGlobalParameters();
}

Environment::~Environment()
{
	environment = 0;
	RELEASE(fallLeaves_);
	RELEASE(pCoastSprite);
	delete environmentTime_;
	RELEASE(lensFlare_);
	RELEASE(temperature_);
	RELEASE(fogOfWar_);
	RELEASE(water_);
	RELEASE(waterBubble_);
	RELEASE(fallout_);
	RELEASE(fixedWaves_);
	RELEASE(chaos);
	RELEASE(env_earth);
	RELEASE(cloud_shadow);
	delete flash_;
	delete PEManager_;
	RELEASE(grassMap);

	delete normalMap;
	normalMap = 0;

	delete windMap;
	windMap = 0;
}

void Environment::setTileMap(cTileMap* terrain)
{
	tileMap_ = terrain;
}

void Environment::logicQuant()
{
	start_timer_auto();

	if(water_){
		start_timer_auto1(water);
		water_->AnimateLogic();
	}

	if(temperature_){
		start_timer_auto1(temperature);
		temperature_->LogicQuant();
	}
		
	if(fogOfWar_ && scene_->IsFogOfWarEnabled()){
		start_timer_auto1(fow);
		fogOfWar_->AnimateLogic();
	}

	if((isUnderEditor() ? flag_EnableTimeFlow : true) && !debug_stop_time)
		environmentTime()->logicQuant();
}

void Environment::graphQuant(float dt, Camera* camera)
{
	start_timer_auto();

	if(water_)
		water_->SetCurReflectSkyColor(environmentTime_->GetCurReflectSkyColor());

	environmentTime()->Draw();

	if(isFogEnabled() && !isFogTempDisabled()){
		PostEffectUnderWater* underWater = (PostEffectUnderWater*)PEManager()->getEffect(PE_UNDER_WATER);
		if(underWater && underWater->isActive())
			underWater->setFog(Color4f(environmentTime()->GetCurFogColor()));
		else{
			float range = camera->GetZPlane().y/max(GetGameFrustrumZMaxHorizontal(),GetGameFrustrumZMaxVertical());
			gb_RenderDevice->SetGlobalFog(Color4f(environmentTime()->GetCurFogColor()),Vect2f(fogStart()*range,fogEnd()*range));
		}
	}
	else
		gb_RenderDevice->SetGlobalFog(Color4f(environmentTime()->GetCurFogColor()),Vect2f(-1, -2));

	environmentTime()->DrawEnviroment(camera);

	if(environmentTime_->isDay()){
		lensFlare_->setFlareSource(environmentTime_->sunPosition(), environmentTime_->sunSize());
		lensFlare_->setVisible(true);
	}
	else{
		lensFlare_->setVisible(false);
	}

	flash()->setIntensity();

	fieldOfViewMap_->updateTexture();
}

void Environment::drawPostEffects(float dt, Camera* camera)
{
	start_timer_auto();

	flash()->draw();
	if(PostEffectUnderWater* eff = (PostEffectUnderWater*)PEManager()->getEffect(PE_UNDER_WATER))
		eff->setUnderWater(water_->isUnderWater(camera->GetPos()));

	PEManager()->draw(dt);
}

void Environment::showEditor()
{
	if(flag_ViewWaves && fixedWaves_)
		fixedWaves_->ShowInfo();
}

void Environment::serialize(Archive& ar) 
{
	start_timer_auto();

	if(ar.filter(SERIALIZE_WORLD_DATA))
		ar.serialize(ResourceSelector(presetName_, presetOptions), "presetName", "Имя файла для сохранения настроек");

	ar.serialize(*tileMap_, "tileMap", "Настройки поверхности");

	if(ar.filter(SERIALIZE_WORLD_DATA))
		GameLoadManager::instance().setProgress(0.5f);

	if(ar.filter(SERIALIZE_WORLD_DATA | SERIALIZE_PRESET_DATA)){
		if(water_)
			ar.serialize(*water_, "Water", "Вода");

		ar.serialize(minimapWaterColor_, "minimapWaterColor", "Цвет воды на миникарте");

		ar.serialize(minimapZonesAlpha_, "minimapZonesAlpha", "Прозрачность зон на миникарте");

		ar.serialize(*environmentTime_, "environmentTime", "Время");

		if(grassMap)
			ar.serialize(*grassMap, "Grass", "Трава");
	}

	if(ar.filter(SERIALIZE_GLOBAL_DATA))
		ar.serialize(*fieldOfViewMap_, "fieldOfViewMap", "Сектора видимости");

	if(ar.filter(SERIALIZE_WORLD_DATA)){
		if(water_)
			fixedWaves_->serialize(ar);

		if(temperature_)
			ar.serialize(*temperature_, "temperature", 0);

		if(ar.isInput() && !presetLoaded_)
			loadPreset();
	}

	if(ar.filter(SERIALIZE_PRESET_DATA)){
		if(fogOfWar_)
			ar.serialize(*fogOfWar_, "fogOfWar", "Туман войны");

		if(ar.openBlock("Environment fog", "Туман на мире")){
			ar.serialize(fog_enable_, "fog_enable", "Включить туман");
			ar.serialize(fog_start_, "fog_start", "Ближняя граница тумана");
			ar.serialize(fog_end_, "fog_end", "Дальняя граница тумана");
			ar.serialize(RangedWrapperi(height_fog_circle_, 0, 2000), "height_fog_circle", "Высота перехода к туману");
			ar.closeBlock();
		}

		if(ar.openBlock("Efects", "Эффекты")){
			ar.serialize(effectHideByDistance_, "effectHideByDistance", "Скрывать эффекты при удалении");
			ar.serialize(effectNearDistance_, "effectNearDistance", "Ближняя граница эффектов");
			ar.serialize(effectFarDistance_, "effectFarDistance", "Дальняя граница эффектов");
			ar.closeBlock();
		}

		if(ar.openBlock("Camera frustrum", "Обрезка кадра")){
			ar.serialize(RangedWrapperf(game_frustrum_z_min_, 1.0f, 100.0f), "game_frustrum_z_min", "Ближняя граница камеры");
			ar.serialize(RangedWrapperf(game_frustrum_z_max_vertical_, 100.0f, 13000.0f), "game_frustrum_z_max", "Дальняя граница камеры (в вертикальном положении)");
			if(ar.isInput())
				game_frustrum_z_max_horizontal_ = game_frustrum_z_max_vertical_;
			ar.serialize(RangedWrapperf(game_frustrum_z_max_horizontal_, 100.0f, 13000.0f), "game_frustrum_z_max_horizontal", "Дальняя граница камеры (в горизонтальном положении)");
			ar.serialize(hideSmoothly_, "hideSmoothly", "Исчезать плавно");
			ar.closeBlock();
		}

		if(water_){	
			if(ar.openBlock("undegroundEffect","Подводный эффект")){
				ar.serialize(underWaterAlways, "underWaterAlways", "Всегда включенный");
				ar.serialize(underWaterColor, "underWaterColor", "Цвет подводного эффекта");
				ar.serialize(underWaterFogPlanes.x,"underWaterFogStart","ближняя граница подводного тумана");
				ar.serialize(underWaterFogPlanes.y,"underWaterFogEnd","дальняя граница подводного тумана");
				ar.serialize(underWaterSpeedDistortion, "underWaterSpeedDistortion", "Скорость искажения");
				ar.serialize(ResourceSelector(underWaterTextureName, textureOptions),"underWaterTextureName","Текстура для искажения");
				ar.closeBlock();
			}
			if(ar.isInput()){
				if(PostEffectUnderWater* underWater = (PostEffectUnderWater*)PEManager()->getEffect(PE_UNDER_WATER)){
					underWater->setActiveAlways(underWaterAlways);
					underWater->setColor(underWaterColor);
					underWater->setFogParameters(underWaterFogPlanes);
					underWater->setTexture(underWaterTextureName.c_str());
					underWater->setWaveSpeed(underWaterSpeedDistortion*1e-2f);
					underWater->setEnvironmentFog(Vect2f(fog_start_, fog_end_));
				}
			}

			if(ar.openBlock("bloomEffect","Эффект свечения")){
				ar.serialize(enableBloom, "enableBloom", "Включить эффект");
				ar.serialize(bloomLuminance, "bloomLuminance", "Интенсивность свечения");
				ar.closeBlock();
			}
			if(ar.openBlock("DofEffect","DOF эффект")){
				ar.serialize(enableDOF,"EnaleDOF","Включить");
				gb_VisGeneric->SetEnableDOF(enableDOF);
				ar.serialize(DofParams.x,"NearPlane","Дистанция фокуса");
				ar.serialize(DofParams.y,"FarPlane","Размер фокуса");
				ar.serialize(dofPower,"dofPower","Сила размытия");
				if (ar.isInput()){
					if(PostEffectDOF* dof = (PostEffectDOF*)PEManager()->getEffect(PE_DOF)){
						dof->setDofPower(dofPower);
						dof->setDofParams(DofParams);
					}
				}
				ar.closeBlock();
			}
			bloomLuminance = clamp(bloomLuminance,0.0f,100.0f);
			if(PostEffectBloom* bloom = (PostEffectBloom*)PEManager()->getEffect(PE_BLOOM)){
				bloom->SetDefaultLuminance(0.15f-bloomLuminance*0.0009f);
				bloom->setActive(enableBloom);
				bloom->RestoreDefaults();
			}

			if(!ar.isEdit() && ar.isInput()){ // CONVERSION 30.10.07
				float waterPFHeight = 2;
				ar.serialize(RangedWrapperf(waterPFHeight, 2.f, 250.f), "waterPFHeight", "Относительный уровень глубокой воды");
				water()->setRelativeWaterLevel(waterPFHeight);
			}
		}

		ar.serialize(outside_, "Outside", "Внешняя среда");
		if(outside_ == ENVIRONMENT_EARTH)
			ar.serialize(outsideHeight_, "outsideHeight", "Высота внешней среды");

		cEffect::setVisibleRange(effectHideByDistance_, sqr(effectNearDistance_), sqr(effectFarDistance_));

		gb_VisGeneric->SetHideFactor(hideByDistanceFactor_);
		gb_VisGeneric->SetHideRange(hideByDistanceRange_);
		gb_VisGeneric->SetHideSmoothly(hideSmoothly_);

		ar.serialize(*fallout_, "fallout", "Осадки");
		ar.serialize(*windMap, "windMap", "Ветер");

		if(water_){
			ar.serialize(*pCoastSprite, "coastSprites", "Прибрежные спрайты");
			ar.serialize(waterPlumeAtribute_, "|waterPlumeAtribute|waterPlume", "Следы на воде");
		}

		ar.serialize(*lensFlare_, "lensFlare_", "Блик камеры");
		ar.serialize(*fallLeaves_, "fallLeaves", "Падающие листья");

		ar.serialize(ResourceSelector(ice_snow_texture, textureOptions),"ice_snow_texture","ЛЕД: Текстура снега");
		ar.serialize(ResourceSelector(ice_bump_texture, textureOptions),"ice_bump_texture","ЛЕД: Текстура бампа");

		ar.serialize(ResourceSelector(env_earth_texture, textureOptions), "env_earth_texture", "Текстура окружающей земли");

		ar.serialize(ResourceSelector(chaosWorldGround0, textureOptions), "chaosWorldGround0", "ХАОС: Текстура 1");
		ar.serialize(ResourceSelector(chaosWorldGround1, textureOptions), "chaosWorldGround1", "ХАОС: Текстура 2");
		ar.serialize(ResourceSelector(chaosOceanBump, textureOptions), "chaosOceanBump", "ХАОС: Bump текстура");

		ar.serialize(ResourceSelector(water_ice_snow_texture, textureOptions), "water_ice_snow_texture", "ЛЕД НА ВОДЕ: Текстура снега");
		ar.serialize(ResourceSelector(water_ice_bump_texture, textureOptions), "water_ice_bump_texture", "ЛЕД НА ВОДЕ: Bump текстура");
		ar.serialize(ResourceSelector(water_ice_cleft_texture, textureOptions), "water_ice_cleft_texture", "ЛЕД НА ВОДЕ: Текстура трещин");
		if(cloud_shadow)
			cloud_shadow->serialize(ar);


		if(ar.isInput()){
			environmentTime_->SetFogCircle(fog_enable_);
			environmentTime_->setFogHeight(height_fog_circle_);

			if(water_){
				water_->ShowEnvironmentWater(outside_ == ENVIRONMENT_WATER);
				if(temperature_){
					temperature_->SetOutIce(water_->waterIsIce());
					temperature_->InitGrid();
				}
			}


			if(lensFlare_)
				lensFlare_->setCameraClip(game_frustrum_z_min_, max(game_frustrum_z_max_horizontal_, game_frustrum_z_max_vertical_));

			if(outside_ == ENVIRONMENT_CHAOS){
				if(!chaos){
					chaos = new cChaos(Vect2f(vMap.H_SIZE, vMap.V_SIZE),
						chaosWorldGround0.c_str(), chaosWorldGround1.c_str(), chaosOceanBump.c_str(), 5, true);
					scene()->AttachObj(chaos);
					if(scene()->GetTileMap()) 
						scene()->GetTileMap()->updateMap(Vect2i(0,0), Vect2i((int)vMap.H_SIZE, (int)vMap.V_SIZE));
				}
				else
					chaos->SetTextures( chaosWorldGround0.c_str(), chaosWorldGround1.c_str(), chaosOceanBump.c_str());
			} 
			else if(chaos){
				RELEASE(chaos);
				if(scene()->GetTileMap()) 
					scene()->GetTileMap()->updateMap(Vect2i(0,0), Vect2i((int)vMap.H_SIZE, (int)vMap.V_SIZE));
			}

			if(outside_ == ENVIRONMENT_EARTH){
				if(!env_earth){
					env_earth = new cEnvironmentEarth(env_earth_texture.c_str(), outsideHeight_);
					scene_->AttachObj(env_earth);
				}
				else 
					env_earth->SetTexture(env_earth_texture.c_str());
			}
			else if(env_earth) 
				RELEASE(env_earth);

			if(temperature_)
				temperature_->SetTexture(water_ice_snow_texture.c_str(),water_ice_bump_texture.c_str(),water_ice_cleft_texture.c_str());
		}
	}
}

void Environment::loadPreset()
{
	presetLoaded_ = true;
	XPrmIArchive ia;
	ia.setFilter(SERIALIZE_PRESET_DATA);
	if(ia.open(presetName_.c_str()))
		ia.serialize(*this, "environment", 0);
}

void Environment::savePreset()
{
	XPrmOArchive oa(presetName_.c_str());
	oa.setFilter(SERIALIZE_PRESET_DATA);
	oa.serialize(*this, "environment", 0);
}

void Environment::loadGlobalParameters()
{
	XPrmIArchive ia;
	ia.setFilter(SERIALIZE_GLOBAL_DATA);
	if(ia.open("Scripts\\Content\\GlobalEnvironment"))
		ia.serialize(*this, "environment", 0);
}

void Environment::saveGlobalParameters()
{
	XPrmOArchive oa("Scripts\\Content\\GlobalEnvironment");
	oa.setFilter(SERIALIZE_GLOBAL_DATA);
	oa.serialize(*this, "environment", 0);
}

bool Environment::isUnderWaterSilouette(const Vect3f& pos)
{
	if(!water()->isUnderWater(pos))
		return false;
	PostEffectUnderWater* underWater = (PostEffectUnderWater*)PEManager()->getEffect(PE_UNDER_WATER);
	return !(underWater && underWater->isUnderWater());
}

bool Environment::isVisibleUnderForOfWar(const Vect2i& pos) const
{
	return	fogOfWar_? fogOfWar_->GetSelectedMap()->isVisible(pos) : true;
}

bool Environment::isDay() const
{
	return environmentTime_->isDay();
}

float Environment::getTime() const
{
	return environmentTime_->GetTime();
}

bool Environment::dayChanged() const
{
	return environmentTime_->DayChanged();
}


