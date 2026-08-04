#include "stdafx.h"
#include "Environment.h"
#include "Terra/VMAP.H"
#include "Render/src/Gradients.h"
#include "Serialization/Serialization.h"
#include "Serialization/Dictionary.h"
#include "Serialization/RangedWrapper.h"
#include "Serialization/ResourceSelector.h"
#include "Serialization/XPrmArchive.h"

#include "Water/Waves.h"
#include "Water/Fallout.h"
#include "Water/WaterWalking.h"
#include "Water/CoastSprites.h"
#include "Water/CloudShadow.h"
#include "Water/WaterGarbage.h"

#include "Render/src/LensFlare.h"
#include "Render/src/Scene.h"
#include "Render/SDLRenderDevice.h"
#include "VistaRender/postEffects.h"
#include "Render/src/Grass.h"
#include "Render/src/CChaos.h"
#include "Render/src/VisGeneric.h"

#include "Render/src/MultiRegion.h"
#include "Render/src/FogOfWar.h"
#include "Render/src/TileMap.h"
#include "Water/SkyObject.h"
#include "Water/FallLeaves.h"
#include "VistaRender/Flash.h"
#include "Physics/NormalMap.h"
#include "Physics/WindMap.h"
#include "Console.h"
#include "DebugPrm.h"
#include "DebugUtil.h"
#include "Serialization/EnumDescriptor.h"
#include "Units/ShowChangeController.h"
#include "Game/Universe.h"	// MAELSTROM_DATA: minimapAngle moved from here to Universe
#include "Environment/SourceManager.h"	// MAELSTROM_DATA: the world's sources used to live here
#include "VistaRender/FieldOfView.h"

#include "UserInterface/GameLoadManager.h"

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

	// The under-water post effect learns the camera's submersion before anything draws:
	// the fog override below and the capture decision both hang on it. The original fed
	// it in drawPostEffects, after the scene -- the SDL frame must know before.
	PostEffectUnderWater* underWater = (PostEffectUnderWater*)PEManager()->getEffect(PE_UNDER_WATER);
	if(underWater && water_)
		underWater->setUnderWater(water_->isUnderWater(camera->GetPos()));

	// The post effects sample the scene, so the scene must render into the device's
	// capture target: arm it before the first pass (the sky, below) opens. On the frames
	// where no effect will draw -- almost all of them -- nothing changes.
	if(cSDLRenderDevice* device = sdlRenderDevice())
		if(PEManager()->anyEffectWillDraw())
			device->armSceneCapture();

	// Distance fog: the colour comes from the time of day, the near and far planes from the
	// world, scaled into the camera's actual depth range. A negative range means "off", which
	// is how cD3DRender::SetGlobalFog read it too. While the camera is under water the
	// under-water effect overrides the planes, pulling them in as it sinks
	// (PostEffectUnderWater::setFog), as the original did here.
	if(isFogEnabled() && !isFogTempDisabled()){
		if(underWater && underWater->isActive())
			underWater->setFog(Color4f(environmentTime()->GetCurFogColor()));
		else {
			float range = camera->GetZPlane().y/max(GetGameFrustrumZMaxHorizontal(),GetGameFrustrumZMaxVertical());
			gb_RenderDevice->SetGlobalFog(Color4f(environmentTime()->GetCurFogColor()),
			                              Vect2f(fogStart()*range, fogEnd()*range));
		}
	}
	else
		gb_RenderDevice->SetGlobalFog(Color4f(environmentTime()->GetCurFogColor()), Vect2f(-1, -2));

	// The sky cubemap, which every reflective material samples. It redraws one of its six
	// faces per frame (cRenderCubemap::Draw), and only the sky scene goes into it, so the
	// cost is about one extra sky render a frame. It has to happen before the world draws:
	// the objects that reflect it are drawn below, and they read the texture it fills.
	environmentTime()->Draw();

	// The sky: the sun or the moon, then the cloud models, drawn through the sky camera's
	// own scene. It opens the frame -- everything below is drawn over it.
	environmentTime()->DrawEnviroment(camera);

	// The lens flare rides the sun: it follows its position while it is day and hides at
	// night. LensFlareRenderer draws it from inside the scene walk.
	if(environmentTime_->isDay()){
		lensFlare_->setFlareSource(environmentTime_->sunPosition(), environmentTime_->sunSize());
		lensFlare_->setVisible(true);
	}
	else
		lensFlare_->setVisible(false);

	// The screen flash's per-frame intensity interpolation. Without the bloom effect it
	// feeds (masked off in PostEffectManager, on D3D9 too) this is bookkeeping only.
	flash()->setIntensity();

	// The field-of-view map's coverage texture (Render-PORTING.md #10b): decay each cell's visibility
	// and rewrite the window the planar camera renders, as the original did at the end of
	// graphQuant. FieldOfViewMap::Draw then lays it into the terrain lightmap's RGB.
	fieldOfViewMap_->updateTexture();
}

// The post-effect stack. Monochrome and the under-water effect are ported (they record
// into SDLPostEffectRenderer; the device composites the scene capture through them); the
// rest of PEManager's chain is not -- see Documents/Render-PORTING.md #6 for what remains and why.
void Environment::drawPostEffects(float dt, Camera* camera)
{
	start_timer_auto();

	// The screen flash feeds the bloom effect, which the manager masks off (as it did on
	// D3D9 -- the nuke flash was invisible in retail P2 too). Kept for logic fidelity:
	// every call inside null-checks the bloom.
	flash()->draw();

	// Each enabled effect updates its fade state and records this frame's parameters.
	PEManager()->draw(dt);

	// Composite the capture through whatever was recorded into the swapchain. If the
	// capture was never armed this frame, it discards instead.
	if(cSDLRenderDevice* device = sdlRenderDevice())
		device->drawPostEffects();
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

#ifdef MAELSTROM_DATA
		// The minimap's rotation is an Environment field here and a Universe one in 2008,
		// so this world writes it in this block and Universe::serialize never sees it. We
		// are deserialized first (Universe::Universe), so hand it across. Maelstrom's
		// worlds need it -- they are 2048x4096 and turn the minimap 90 degrees to fit a
		// landscape panel.
		if(ar.isInput() && universe()){
			float minimapAngle = universe()->minimapAngle();
			if(ar.serialize(minimapAngle, "minimapAngle", "Угол поворота миникарты"))
				universe()->setMinimapAngle(minimapAngle);
		}

		// Everything the environment lights a world with -- the sun, shadow and sky
		// gradients, the sky models, the time of day -- is written flat in this block;
		// 2008 moved it under "environmentTime". Read where this world put it, or it
		// lights itself entirely from the constructed defaults: midday where it asked for
		// a quarter past nine, ambient 0.2 where it asked for 0.5, a default cloud layer
		// instead of its own. The terrain still looks about right, its colour being baked
		// per cell in the height map -- it is the objects standing on it that go black.
		environmentTime_->serializeMaelstrom(ar);
#else
		ar.serialize(*environmentTime_, "environmentTime", "Время");
#endif

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

#ifdef MAELSTROM_DATA
		// A world's sources -- the zones that hold its standing effects, its damage and
		// its unit generators -- are written here; 2008 split SourceManager out of
		// Environment and moved them into Universe's "sourceManager" block. Unread, every
		// placed effect in the world is simply absent: in Maelstrom's menu that is each
		// building fire, every smoke column and the green outflow from the pipe, all of
		// them SourceZones.
		//
		// Called inline, not through a named block: "sources" and "anchors" sit at this
		// level. Universe::Universe reads the environment last for this data, so the
		// players and their units are already in place and a source can be switched on as
		// it is read, which is what SourceManager::serialize does when it finishes.
		if(sourceManager)
			sourceManager->serialize(ar);
#endif

#ifdef MAELSTROM_DATA
		// The preset group had not been split out of the world yet. One object owned it --
		// EnvironmentAttributes, written as the world's "environmentColors" node -- and
		// 2008 dissolved that object: its fields became Environment's own, its fog-of-war
		// colours FogOfWar's, two of its constants cWater's, and the group as a whole moved
		// out of the world and into a preset file. The names survived the move; the depth
		// did not, and openBlock is a no-op in an XPrm archive (editor-only grouping, it
		// does not descend), so read at this level not one of these names is found.
		//
		// "ownAttributes" says whether the world carries its own copy -- 11 of Maelstrom's
		// 51 do, the other 40 taking the global one, which is what loadPreset() reads here.
		if(ar.isInput()){
			bool ownAttributes = true;
			ar.serialize(ownAttributes, "ownAttributes", "Собственные настройки среды");
			if(ownAttributes){
				// The object handed to openStruct is only used by binary archives, for
				// their size and type name; an XPrm archive descends by name alone.
				if(ar.openStruct(*this, "environmentColors", "Цвета среды")){
					serializeMaelstromColors(ar);
					ar.closeStruct("environmentColors");
				}
			}
			else if(!presetLoaded_)
				loadPreset();
		}
#else
		if(ar.isInput() && !presetLoaded_)
			loadPreset();
#endif
	}

#ifdef MAELSTROM_DATA
	// The rest of the group is written flat in the world's environment block, where this
	// reads it -- so run under the world filter as well. Maelstrom ships no
	// Scripts\Content\Presets\ at all, so waiting for loadPreset() would leave every world
	// on constructed defaults: fog at 1000-1400 where Menu.spg asks for 900-1200, a
	// 30-4000 camera frustum where it asks for 2-1300, and no weather, shore or lens flare
	// at all. What sits in the world's "environmentColors" node has been read by now; only
	// the names beside that node are still to come.
	if(ar.filter(SERIALIZE_WORLD_DATA | SERIALIZE_PRESET_DATA)){
#else
	if(ar.filter(SERIALIZE_PRESET_DATA)){
#endif
#ifndef MAELSTROM_DATA
		if(fogOfWar_)
			ar.serialize(*fogOfWar_, "fogOfWar", "Туман войны");
#endif

		if(ar.openBlock("Environment fog", "Туман на мире")){
			ar.serialize(fog_enable_, "fog_enable", "Включить туман");
#ifndef MAELSTROM_DATA
			ar.serialize(fog_start_, "fog_start", "Ближняя граница тумана");
			ar.serialize(fog_end_, "fog_end", "Дальняя граница тумана");
			ar.serialize(RangedWrapperi(height_fog_circle_, 0, 2000), "height_fog_circle", "Высота перехода к туману");
#endif
			ar.closeBlock();
		}

#ifndef MAELSTROM_DATA
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
#endif // !MAELSTROM_DATA

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

		// Capitalised by 2008. The archive tries these in order and stops at the first hit,
		// so Perimeter 2 matches on the first name and pays nothing for the second.
		ar.serialize(outside_, "|Outside|outside", "Внешняя среда");
		if(outside_ == ENVIRONMENT_EARTH)
			ar.serialize(outsideHeight_, "outsideHeight", "Высота внешней среды");

		cEffect::setVisibleRange(effectHideByDistance_, sqr(effectNearDistance_), sqr(effectFarDistance_));

		gb_VisGeneric->SetHideFactor(hideByDistanceFactor_);
		gb_VisGeneric->SetHideRange(hideByDistanceRange_);
		gb_VisGeneric->SetHideSmoothly(hideSmoothly_);

#ifndef MAELSTROM_DATA
		ar.serialize(*fallout_, "fallout", "Осадки");
		ar.serialize(*windMap, "windMap", "Ветер");

		if(water_){
			ar.serialize(*pCoastSprite, "coastSprites", "Прибрежные спрайты");
			ar.serialize(waterPlumeAtribute_, "|waterPlumeAtribute|waterPlume", "Следы на воде");
		}
#endif

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

#ifdef MAELSTROM_DATA
void Environment::serializeMaelstromColors(Archive& ar)
{
	// Field for field this is the group the preset branch of serialize() reads; only the
	// place the names sit in the file differs, so every name here is deliberately the
	// same one -- except the two FogOfWar renamed, which its own reader carries.
	//
	// timeColors_ is deliberately left unread. It is this node's own copy of the six sky
	// gradients, and it is not the set a world is lit by: Maelstrom lit from
	// EnvironmentTime's gradients, written flat in the environment block and read in
	// EnvironmentTime::serializeMaelstrom, and reached into a timeColors_ only for the
	// global ones -- its Environment.cpp has
	// ReplaceGlobal(GlobalAttributes::instance().environmentAttributes_.timeColors_).
	// What the editor saved here beside them is that global set, a 9-key ramp against
	// Menu.spg's own 8-key one, so reading it would overwrite a world's own lighting with
	// the global default. miniDetailTexResolution has no reader in this engine at all.
	if(fogOfWar_)
		fogOfWar_->serializeMaelstrom(ar);

	ar.serialize(fog_start_, "fog_start", "Ближняя граница тумана");
	ar.serialize(fog_end_, "fog_end", "Дальняя граница тумана");
	ar.serialize(RangedWrapperi(height_fog_circle_, 0, 2000), "height_fog_circle", "Высота перехода к туману");

	ar.serialize(effectHideByDistance_, "effectHideByDistance", "Скрывать эффекты при удалении");
	ar.serialize(effectNearDistance_, "effectNearDistance", "Ближняя граница эффектов");
	ar.serialize(effectFarDistance_, "effectFarDistance", "Дальняя граница эффектов");

	ar.serialize(RangedWrapperf(game_frustrum_z_min_, 1.0f, 100.0f), "game_frustrum_z_min", "Ближняя граница камеры");
	ar.serialize(RangedWrapperf(game_frustrum_z_max_vertical_, 100.0f, 13000.0f), "game_frustrum_z_max", "Дальняя граница камеры (в вертикальном положении)");
	game_frustrum_z_max_horizontal_ = game_frustrum_z_max_vertical_;
	ar.serialize(RangedWrapperf(game_frustrum_z_max_horizontal_, 100.0f, 13000.0f), "game_frustrum_z_max_horizontal", "Дальняя граница камеры (в горизонтальном положении)");
	ar.serialize(hideSmoothly_, "hideSmoothly", "Исчезать плавно");

	if(water_)
		water_->serializeMaelstrom(ar);

	ar.serialize(*fallout_, "fallout", "Осадки");
	ar.serialize(*windMap, "windMap", "Ветер");

	if(water_){
		ar.serialize(*pCoastSprite, "coastSprites", "Прибрежные спрайты");
		ar.serialize(waterPlumeAtribute_, "|waterPlumeAtribute|waterPlume", "Следы на воде");
	}
}

void Environment::loadPreset()
{
	// Maelstrom's preset file is Scripts\Content\GlobalAttributes: a world that does not
	// carry its own settings ("ownAttributes = false", 40 of the 51) took this copy, which
	// is what its Environment did with
	// environmentAttributes_ = GlobalAttributes::instance().environmentAttributes_.
	// The engine already reads this file as a library (Units/GlobalAttributes.cpp), but
	// nothing there descends into its environmentColors, so open it again for that node
	// alone. presetName_ is left alone: no Maelstrom install has a Presets directory.
	presetLoaded_ = true;
	XPrmIArchive ia;
	ia.setFilter(SERIALIZE_PRESET_DATA);
	if(ia.open("Scripts\\Content\\GlobalAttributes")
	&& ia.openStruct(*this, "GlobalAttributes", "Глобальные параметры")){
		if(ia.openStruct(*this, "environmentColors", "Цвета среды")){
			serializeMaelstromColors(ia);
			ia.closeStruct("environmentColors");
		}
		ia.closeStruct("GlobalAttributes");
	}
}
#else
void Environment::loadPreset()
{
	presetLoaded_ = true;
	XPrmIArchive ia;
	ia.setFilter(SERIALIZE_PRESET_DATA);
	if(ia.open(presetName_.c_str()))
		ia.serialize(*this, "environment", 0);
}
#endif

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


