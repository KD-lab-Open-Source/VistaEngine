#include "StdAfx.h"

#include "Environment.h"

#include "..\terra\terra.h"
#include "RenderObjects.h"
#include "..\render\src\Gradients.h"
#include "Serialization.h"
#include "Dictionary.h"
#include "RangedWrapper.h"
#include "ResourceSelector.h"
#include "..\Game\Universe.h"
#include "..\Game\CameraManager.h"
#include "..\Units\UnitAttribute.h"
#include "..\Units\ExternalShow.h"
#include "..\Units\ShowChangeController.h"
#include "Actions.h"
#include "SourceShield.h"

#include "..\Game\TransparentTracking.h"
#include "..\Water\Waves.h"
#include "..\Water\Fallout.h"
#include "..\Water\WaterWalking.h"
#include "..\Water\CoastSprites.h"
#include "..\Water\CloudShadow.h"
#include "..\Water\WaterGarbage.h"

#include "Region.h"
#include "..\UserInterface\UserInterface.h"
#include "..\UserInterface\UI_CustomControls.h"
#include "GameOptions.h"

#include "..\Render\src\LensFlare.h"
#include "..\Render\src\postEffects.h"
#include "..\Render\src\Grass.h"
#include "IVisD3D.h"

#include "..\Water\CircleManager.h"
#include "..\Render\src\MultiRegion.h"
#include "Anchor.h"
#include "..\Water\SkyObject.h"
#include "windMap.h"
#include "..\Water\FallLeaves.h"

#include "..\Util\Console.h"
#include "..\UserInterface\UI_Logic.h"

REGISTER_CLASS(Anchor, Anchor, "Якорь на мире");

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

Environment* environment;

FORCE_REGISTER_CLASS(SourceBase, SourceZone, "Зона на мире");

bool Environment::flag_ViewWaves=true;
bool Environment::flag_EnableTimeFlow=false;

Environment::Environment(int worldSizeX, int worldSizeY, cScene* scene, cTileMap* terrain,const MissionDescription& description)
: chaosWorldGround0("Scripts\\resource\\Textures\\WorldGround.tga")
, chaosWorldGround1("Scripts\\resource\\Textures\\WorldGround01.tga")
, chaosOceanBump("Scripts\\resource\\Textures\\OceanBump.tga")
, env_earth_texture("Scripts\\resource\\Textures\\Ground_018.tga")
, water_ice_snow_texture("Scripts\\Resource\\balmer\\snow.tga")
, water_ice_bump_texture("Scripts\\Resource\\balmer\\snow_bump.tga")
, water_ice_cleft_texture("Scripts\\Resource\\balmer\\ice_cleft.tga")
, sourceGrid(vMap.H_SIZE, vMap.V_SIZE)

, fogOfWar_(0)
, fog_enable_(false)
, fogTempDisabled_(false)

, sourceOnMouse_(0)
, water_(0)
, waterBubble_(0)
, pCoastSprite(0)
, fixedWaves_(0)
, grassMap(0)
, chaos(0)
, env_earth(0)
, temperature_(0)

, cameraRestriction_(*new CameraRestriction)
, cameraBorder_(*new CameraBorder)
{
	scene_ = scene;
	waterHeight = 27;
	water_is_lava=false;
	ice_snow_texture="Scripts\\Resource\\balmer\\snow.tga";
	ice_bump_texture="Scripts\\Resource\\balmer\\snow_bump.tga";
	lava_texture="Scripts\\Resource\\balmer\\lava.tga";

	anywhereIce = false;

	environmentTime_ = new cEnvironmentTime(scene_);
	environmentTime_->Init();

	DofParams = Vect2f(100,1000);
	dofPower = 4.f;
	enableDOF = false;

	if(description.is_water){
		water_ = new cWater;
		water_->Init(scene_);

		waterBubble_ = new cWaterBubble(water_, terrain);
		scene_->AttachObj(water_);
		scene_->AttachObj(waterBubble_);
	}

	ownCameraRestriction_ = false;
	ownAttributes_ = true;
	minimapAngle_ = 0.f;
	
	flash_ = new cFlash;

	PEManager_ = new PostEffectManager();
	if(PEManager_)
	{
		PEManager_->Init();

		enableBloom = false;
		bloomLuminance = 0;

		if(PEManager_->GetEffect(PE_UNDER_WATER))
			((UnderWaterEffect*)PEManager_->GetEffect(PE_UNDER_WATER))->SetWater(water_);
		underWaterColor = sColor4f(0,0,0.2f);
		underWaterFogColor = sColor4f(0,0,1.f);
		underWaterFogPlanes = Vect2f(100.f,1000.f);
		underWaterSpeedDistortion = 10;
		underWaterAlways = false;


	}
	//bloomEffect_ = new BloomEffect();
	//bloomEffect_->Init();

	//underWaterEffect_ = new UnderWaterEffect(water_);
	//underWaterEffect_->Init();

	//monochromeEffect_ = new MonochromeEffect();
	//monochromeEffect_->Init();
	//colorDodgeEffect_ = new ColorDodgeEffect();
	//colorDodgeEffect_->Init();

	//dofEffect_ = new DOFEffect();
	//dofEffect_->Init();

	//mirageEffect_ = new MirageEffect();
	//mirageEffect_->Init();

	grassMap=NULL;
	grassMap = new GrassMap();
	if(grassMap)
	{
		grassMap->Init(scene_);
		scene_->AttachObj(grassMap);
	}

	if(description.is_fog_of_war)
		fogOfWar_ = scene_->CreateFogOfWar();
	reload();

	waterPFHeight = 0;

	need_alpha_tracking = false;

	cloud_shadow = new cCloudShadow;
	scene_->AttachObj(cloud_shadow);

	if(description.is_temperature && water_)
	{
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

	sndEnvManager = new SoundEnvironmentManager();

}

void Environment::clearSources()
{
	flushNewSources();
	std::for_each(sources_.begin(), sources_.end(), bind2nd(mem_fun(&SourceBase::setActivity), false));
	sources_.clear();
}

Environment::~Environment()
{
	clearSources();
	if(sourceOnMouse_) sourceOnMouse_->setActivity(false);
	std::for_each(sounds_.begin(), sounds_.end(), std::mem_fun_ref(&SoundController::release));
	sounds_.clear();
	RELEASE(fallLeaves_);
	delete &cameraBorder_;
	delete &cameraRestriction_;
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
	//delete bloomEffect_;
	//delete underWaterEffect_;//Trash video memory
	//delete monochromeEffect_;
	//delete dofEffect_;
	//delete mirageEffect_;
	//delete colorDodgeEffect_;
	delete PEManager_;
	RELEASE(grassMap);
	delete sndEnvManager;
}

void Environment::flushNewSources()
{
	sources_.insert(sources_.end(), newSources_.begin(), newSources_.end());
	newSources_.clear();
}

void Environment::setSourceOnMouse(const SourceBase* source)
{
	if(sourceOnMouse_)
		sourceOnMouse_->kill();
	
	if(!source) return;
	
	sourceOnMouse_ = addSource(source);
	sourceOnMouse_->setPose(Se3f(QuatF::ID, UI_LogicDispatcher::instance().hoverPosition()), true);
	sourceOnMouse_->setActivity(true);
}

void Environment::logicQuant()
{
	start_timer_auto();
	statistics_add(sources, sources_.size());

	if(water_){
		start_timer(water);
		water_->AnimateLogic();
		stop_timer(water);
	}

	if(temperature_){
		start_timer(temperature);
		temperature_->LogicQuant();
		stop_timer(temperature);
	}
		
	if(fogOfWar_ && scene_->IsFogOfWarEnabled()){
		start_timer(fow);
		fogOfWar_->AnimateLogic();
		stop_timer(fow);
	}

	start_timer(sources);
	flushNewSources();

	if(sourceOnMouse_)
		sourceOnMouse_->setPose(Se3f(sourceOnMouse_->orientation(), UI_LogicDispatcher::instance().hoverPosition()), false);

	Sources::iterator it;
	for(it = sources_.begin(); it != sources_.end();){
		if((*it)->isDead()){
			it = sources_.erase(it);
		}
		else {
			if((*it)->isAlive()){
				(*it)->quant();
				if((*it)->needActivation())
					(*it)->setActivity(true);
			}
			else 
				(*it)->incDeadCnt();
			++it;
		}
	}

	flushNewSources();
	stop_timer(sources);

	showDebugSource.sourceCount = sources_.size();
	
	start_timer(1);
	SoundControllers::iterator snd;
	for(snd = sounds_.begin(); snd != sounds_.end();){
		if(!snd->owner()){
			snd->release();
			snd = sounds_.erase(snd);
		}
		else {
			snd->quant();
			++snd;
		}
	}
	stop_timer(1);

	if((isUnderEditor() ? flag_EnableTimeFlow : true) && !debug_stop_time){
		float timeScale_ = environmentTime()->IsDay() ? environmentAttributes_.dayTimeScale_ : environmentAttributes_.nightTimeScale_;
		if(timeScale_){
			float time = environmentTime()->GetTime();
			time = fmod(time + logicTimePeriod*timeScale_/(3600*1000.f), 24);
			environmentTime()->SetTime(time);
		}
	}

	log_var(isDay());

	if(universe()->GetTransparentTracking() && cameraManager)
		universe()->GetTransparentTracking()->Tracking(cameraManager->GetCamera());

	if(!isUnderEditor()){
		Anchors::const_iterator ait;
		FOR_EACH(anchors_, ait)
			if((*ait)->type() == Anchor::MINIMAP_MARK && (*ait)->selected())
				minimap().addAnchor(*ait);
	}
}

void Environment::setTimeScale(float dayTimeScale, float nightTimeScale)
{
	environmentAttributes_.dayTimeScale_ = dayTimeScale;
	environmentAttributes_.nightTimeScale_ = nightTimeScale;
}

void Environment::setCoastSpritesAttributtes(const CoastSpritesAttributes& coastSprites) 
{ 
	environmentAttributes_.coastSprites_ = coastSprites; 
}

void Environment::graphQuant(float dt)
{
	start_timer_auto();

	if(water_)
		water_->SetCurReflectSkyColor(environmentTime_->GetCurReflectSkyColor());

	environmentTime()->Draw();

	if(isFogEnabled() && !isFogTempDisabled()){
		UnderWaterEffect* underWater = (UnderWaterEffect*)PEManager()->GetEffect(PE_UNDER_WATER);
		if (underWater && underWater->IsActive())
			underWater->SetFog(sColor4f(environmentTime()->GetCurFogColor()));
		else
		{
			float zmax_real=cameraManager->getZMinMax().y;
			float range=zmax_real/max(GetGameFrustrumZMaxHorizontal(),GetGameFrustrumZMaxVertical());
			gb_RenderDevice->SetGlobalFog(sColor4f(environmentTime()->GetCurFogColor()),Vect2f(fogStart()*range,fogEnd()*range));
		}
	}
	else
		gb_RenderDevice->SetGlobalFog(sColor4f(environmentTime()->GetCurFogColor()),Vect2f(-1, -2));

	environmentTime()->DrawEnviroment(cameraManager->GetCamera());

	if(environmentTime_->IsDay()){
		lensFlare_->setFlareSource(environmentTime_->sunPosition(), environmentTime_->sunSize());
		lensFlare_->setVisible(true);
	}
	else{
		lensFlare_->setVisible(false);
	}

	flash()->setIntensity();
}

void Environment::drawUI(float dt)
{
	MTG();
	ShowChangeControllers::iterator shit;
	for(shit = showChangeControllers_.begin(); shit != showChangeControllers_.end();){
		if(!(*shit)->alive())
			shit = showChangeControllers_.erase(shit);
		else {
			(*shit)->quant();
			++shit;
		}
	}
}

void fCommandAddShowChangeController(XBuffer& stream)
{
	SharedShowChangeController share; // неявный share->decrRef() при разрушении локального объекта
	stream.read(share);
	environment->showChangeControllers_.push_back(share);
}

ShowChangeController* Environment::addShowChangeController(const ShowChangeController& ctrl)
{
	MTL();
	SharedShowChangeController share(new ShowChangeController(ctrl));
	share->addRef(); // что бы не протух пока лежит в потоке
	streamLogicCommand.set(fCommandAddShowChangeController) << share;
	return share;
}

namespace{

struct BlackRectangle{
	float x1, y1, x2, y2;
};

};

void Environment::drawBlackBars(float opacity)
{
	if(::isUnderEditor() || cameraManager->aspect() - FLT_COMPARE_TOLERANCE > 4.0f / 3.0f)
		return;

	gb_RenderDevice->SetNoMaterial(ALPHA_BLEND, MatXf::ID);

	int oldAlphaBlend = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHABLENDENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	int oldAlphaTest  = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHATESTENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

	int oldZEnable    = gb_RenderDevice3D->GetRenderState(D3DRS_ZENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE, FALSE);	

	float topSize = 0.5f + cameraManager->frustumClip().ymin();
	float bottomSize = 0.5f - cameraManager->frustumClip().ymax();

	float leftSize = 0.5f + cameraManager->frustumClip().xmin();
	float rightSize = 0.5f - cameraManager->frustumClip().xmax();

	float width = gb_RenderDevice->GetSizeX();
	float height = gb_RenderDevice->GetSizeY();

	BlackRectangle rectangles[] = {
	    { -1.0f, -1.0f, width + 1.0f, height * topSize },
		{ -1.0f, -1.0f, width * leftSize, height + 1.0f},

		{ width * (1.0f - rightSize), -1.0f, width + 1.0f, height  + 1.0f},
	    { -1.0f, height * (1.0f - bottomSize), width + 1.0f, height + 1.0f},
	};

	const int numRects = sizeof(rectangles) / sizeof(rectangles[0]);
	const int numPoints = numRects * 2 * 3;
	sColor4c color(0, 0, 0, round(opacity * 255.0f));

	int npoint = 0;
	cVertexBuffer<sVertexXYZWD>* buffer = gb_RenderDevice->GetBufferXYZWD();
	sVertexXYZWD* pv = buffer->Lock(numPoints);
	for(BlackRectangle* it = &rectangles[0]; it != &rectangles[0] + numRects; ++it){
		BlackRectangle& p = *it;

		pv[0].x=p.x1; pv[0].y=p.y1; pv[0].z=0.001f; pv[0].w=0.001f; pv[0].diffuse = color;
		pv[1].x=p.x1; pv[1].y=p.y2; pv[1].z=0.001f; pv[1].w=0.001f; pv[1].diffuse = color;
		pv[2].x=p.x2; pv[2].y=p.y1; pv[2].z=0.001f; pv[2].w=0.001f; pv[2].diffuse = color;

		pv[3].x=p.x2; pv[3].y=p.y1; pv[3].z=0.001f; pv[3].w=0.001f; pv[3].diffuse = color;
		pv[4].x=p.x1; pv[4].y=p.y2; pv[4].z=0.001f; pv[4].w=0.001f; pv[4].diffuse = color;
		pv[5].x=p.x2; pv[5].y=p.y2; pv[5].z=0.001f; pv[5].w=0.001f; pv[5].diffuse = color;
		pv += 6;		
	}
	buffer->Unlock(numPoints);
	buffer->DrawPrimitive(PT_TRIANGLELIST, numPoints / 3);

	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE, oldAlphaTest);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE, oldZEnable);
}

void Environment::drawPostEffects(float dt)
{
	start_timer_auto();

	flash()->draw();
	if(PEManager()->GetEffect(PE_UNDER_WATER))
		((UnderWaterEffect*)PEManager()->GetEffect(PE_UNDER_WATER))->SetIsUnderWater(water_->isUnderWater(cameraManager->GetCamera()->GetPos()));
	PEManager()->Draw(dt);
}

void Environment::showEditor()
{
	cFont* pFont=gb_VisGeneric->CreateFont(default_font_name.c_str(), 16, 1);
	gb_RenderDevice->SetFont(pFont);

    cCamera* camera=cameraManager->GetCamera();

	if(!showDebugSource.dontShowInfo){
		Sources::iterator it;
		FOR_EACH(sources_,it){
			SourceBase* source = *it;
			if(source->isAlive())
				source->showEditor();
		}
	}

	if(Environment::flag_ViewWaves && fixedWaves_)
		fixedWaves_->ShowInfo();

	gb_RenderDevice->SetFont(NULL);
	pFont->Release();

	Anchors::iterator ait;
	FOR_EACH(anchors_, ait)
		(*ait)->showEditor();

}

void Environment::showDebug() const
{
	if(showDebugSource.enable){
		Sources::const_iterator i;
		FOR_EACH(sources_, i)
			(*i)->showDebug();
	}
	
	if(showDebugAnchors){
		Anchors::const_iterator it;
		FOR_EACH(anchors_, it)
			(*it)->showDebug();
		
		const Anchor* fst = findAnchor("__first_debug_anchor");
		Vect3f start = fst ? fst->position() : Vect3f::ZERO;

		const Anchor* scn = findAnchor("__second_debug_anchor");
		Vect3f finish = scn ? scn->position() : Vect3f::ZERO;

		if(fst && scn){
			Vect3f intersect;
			bool abil = SourceShield::traceShieldsThrough(start, finish, 0, &intersect);
			show_vector(start, 3.f, YELLOW);
			show_vector(finish, 3.f, YELLOW);
			if(abil)
				show_line(start, finish, GREEN);
			else{
				show_line(start, finish, RED);
				show_line(intersect + Vect3f (-5.0f, 0.0f, 0.0f), intersect + Vect3f (5.0f, 0.0f, 0.0f), RED);
				show_line(intersect + Vect3f (0.0f, -5.0f, 0.0f), intersect + Vect3f (0.0f, 5.0f, 0.0f), RED);
				show_line(intersect + Vect3f (0.0f, 0.0f, -5.0f), intersect + Vect3f (0.0f, 0.0f, 5.0f), RED);
			}
		}
	}
}

SourceBase* Environment::createSource(const SourceAttribute* attribute, const Se3f& pose, bool allow_unlimited_lifetime, bool* startFlag)
{
	if(attribute->isEmpty())
		return 0;

	if(!(1 << vMap.getSurKind(pose.trans().xi(), pose.trans().yi()) & attribute->source()->surfaceKind()))
		return 0;

	switch(attribute->source()->surfaceClass()){
		case SourceBase::SOURCE_SURFACE_GROUND:
			if(water_ && water_->isFullWater(pose.trans()))
				return 0;
			break;
		case SourceBase::SOURCE_SURFACE_WATER:
			if(!water_ || !water_->isFullWater(pose.trans()) || temperature_ && temperature_->isOnIce(pose.trans()))
				return 0;
			break;
		case SourceBase::SOURCE_SURFACE_ICE:
			if(!temperature_ || !temperature_->isOnIce(pose.trans()) || !water_ || !water_->isWater(pose.trans()))
				return 0;
			break;
	}

	SourceBase* source = addSource(attribute->source());

	universe()->checkEvent(EventSource(Event::CREATE_SOURCE, attribute));

	source->setPose(pose, true);

	bool fakeStart = true;
	bool& needStart = startFlag ? *startFlag : fakeStart;
	
	if(attribute->activationDelay() > FLT_EPS){
		source->setActivationTime(attribute->activationDelay() * 1000.0f);
		needStart = false;
	}
	else if(needStart){
		source->setActivity(!source->waiting_target());
		needStart = false;
	}
	else
		needStart = !source->waiting_target();

	if(attribute->lifeTime() > FLT_EPS)
		source->setKillTimer(attribute->lifeTime() * 1000.0f);
	else if(!allow_unlimited_lifetime)
		source->setKillTimer(SourceBase::MAX_DEFAULT_LIFETIME);

	return source;
}

SourceBase* Environment::addSource (const SourceBase* original)
{
	xassert(original);
	SourceBase* result = original->clone();
	newSources_.push_back(result);

	result->setName(SourceReference(original).c_str());
	xassert(!result->active());
	result->enable();

	return result;
}

Anchor* Environment::addAnchor()
{
	anchors_.push_back(new Anchor());
	return anchors_.back();
}

Anchor* Environment::addAnchor(const Anchor* original)
{
	xassert(original);
	anchors_.push_back(new Anchor(*original));
	return anchors_.back();
}

bool Environment::soundAttach(const SoundAttribute* sound, const BaseUniverseObject *obj)
{
	MTL();
	SoundControllers::const_iterator it = std::find(
						sounds_.begin(),
						sounds_.end(),
						SoundControllerToken(sound, obj));

	if(it != sounds_.end())
		return false;

	sounds_.push_back(SoundController());
	sounds_.back().init(sound, obj);
	sounds_.back().start();
	return true;
}

void Environment::soundRelease(const SoundAttribute* sound, const BaseUniverseObject *obj)
{
	MTL();
	SoundControllers::iterator it = std::find(
					sounds_.begin(),
					sounds_.end(),
					SoundControllerToken(sound, obj));

	if(it != sounds_.end()){
		it->release();
		sounds_.erase(it);
	}
}

bool Environment::soundIsPlaying(const SoundAttribute* sound, const BaseUniverseObject *obj)
{
	MTL();
	SoundControllers::iterator it = std::find(
					sounds_.begin(),
					sounds_.end(),
					SoundControllerToken(sound, obj));

	if(it == sounds_.end())
		return false;
	
	return it->isPlaying();
}

void Environment::reload()
{
	if (water_) {
		water_->SetEnvironmentWater(waterHeight);
		water_->SetRainConstant(-environmentAttributes_.rainConstant_ * rain_multiplicator);
	}
	// запускаем эффекты
	Sources::iterator sit;
	FOR_EACH(sources_, sit) {
		SourceBase* source = *sit;
		source->enable();
		if(source->active()) {
			source->setActivity(false);
			source->setActivity(true);
		}
	}
}

void Environment::deselectAll()
{
	{
		Sources::iterator it;
		FOR_EACH(sources_, it)
			(*it)->setSelected(false);
	}
	{
		Anchors::iterator it;
		FOR_EACH(anchors_, it)
			(*it)->setSelected(false);
	}
}

void Environment::deleteSelected()
{
	Sources::iterator i;
	FOR_EACH(sources_, i)
		if((*i)->selected() && (*i)->isAlive()){
			(*i)->kill();
		}

	{
		for(Anchors::iterator it = anchors_.begin(); it != anchors_.end();){
			if((*it)->selected()){
				it = anchors_.erase(it);
			} else {
				++it;
			}			
		}		
	}
}

STARFORCE_API void Environment::serializeParameters(Archive& ar) 
{
	ar.serialize(ownAttributes_, "ownAttributes", "Собственные настройки общих параметров");
	if(ownAttributes_)
		ar.serialize(environmentAttributes_, "environmentColors", "Общие параметры");
	else if(ar.isInput())
		environmentAttributes_ = GlobalAttributes::instance().environmentAttributes_;

	sColor4f reflection_color(0,0,0,0);
	float reflection_brightnes = 0;
	if(water_){	
		if(!ar.isEdit() && ar.isOutput())
			waterHeight = water_->GetEnvironmentWater();
		if (ar.isOutput() && ar.isEdit())
		{
			reflection_brightnes = water_->GetReflectionBrightnes();
			reflection_color = water_->GetReflectionColor();
		}

		ar.serialize(RangedWrapperi(waterHeight, 0, 255), "waterHeight", "ВОДА: Высота воды");

		if(ar.isEdit())//понимаю что криво, дас. Компромисс между инкапсуляцией полнотой сериализации cWater и редактированием.
		{
			ar.serialize(reflection_color, "reflection_color1", "ВОДА: Цвет и прозрачность отражения");
			ar.serialize(reflection_brightnes, "reflection_brightnes", "ВОДА: Коэффициент яркости отражения неба");
			sColor4f c;
			if(ar.isOutput())c=water_->GetLavaColor();
			ar.serialize(c, "lava_color_", "ВОДА: Лава цвет");
			if(ar.isInput())water_->SetLavaColor(c);
			if(ar.isOutput())c=water_->GetLavaColorAmbient();
			ar.serialize(c, "lava_color_ambient_", "ВОДА: Лава ambient цвет");
			if(ar.isInput())water_->SetLavaColorAmbient(c);

		}

		if(temperature_)
		{
			ar.serialize(anywhereIce,"anywhereIce","Лед на всей карте");
			if(ar.isInput())
			{
				temperature_->SetOutIce(anywhereIce);
				temperature_->InitGrid();
			}
		}
		ar.openBlock("undegroundEffect","Подводный эффект");
		ar.serialize(underWaterAlways, "underWaterAlways", "Всегда включенный");
		ar.serialize(underWaterColor, "underWaterColor", "Цвет подводного эффекта");
		ar.serialize(underWaterFogPlanes.x,"underWaterFogStart","ближняя граница подводного тумана");
		ar.serialize(underWaterFogPlanes.y,"underWaterFogEnd","дальняя граница подводного тумана");
		ar.serialize(underWaterSpeedDistortion, "underWaterSpeedDistortion", "Скорость искажения");
		ar.serialize(ResourceSelector(underWaterTextureName, ResourceSelector::TEXTURE_OPTIONS),"underWaterTextureName","Текстура для искажения");
		ar.closeBlock();
		if(ar.isInput()){
			UnderWaterEffect* underWater = (UnderWaterEffect*)PEManager()->GetEffect(PE_UNDER_WATER);
			if(underWater)
			{
				underWater->SetActiveAlways(underWaterAlways);
				underWater->SetColor(underWaterColor);
				underWater->SetFogParameters(underWaterFogPlanes);
				underWater->SetTexture(underWaterTextureName);
				underWater->SetWaveSpeed(underWaterSpeedDistortion*1e-2f);
				underWater->SetEnvironmentFog(Vect2f(environmentAttributes_.fog_start_, environmentAttributes_.fog_end_));
			}
		}

		ar.openBlock("bloomEffect","Эффект свечения");
		ar.serialize(enableBloom, "enableBloom", "Включить эффект");
		ar.serialize(bloomLuminance, "bloomLuminance", "Интенсивность свечения");
		ar.closeBlock();
		ar.openBlock("DofEffect","DOF эффект");
		ar.serialize(enableDOF,"EnaleDOF","Включить");
		gb_VisGeneric->SetEnableDOF(enableDOF);
		ar.serialize(DofParams.x,"NearPlane","Дистанция фокуса");
		ar.serialize(DofParams.y,"FarPlane","Размер фокуса");
		ar.serialize(dofPower,"dofPower","Сила размытия");
		if (ar.isInput())
		{
			DOFEffect* dof = (DOFEffect*)PEManager()->GetEffect(PE_DOF);
			if(dof)
				dof->SetDofPower(dofPower);
			SetDofParams(DofParams);
		}
		ar.closeBlock();
		bloomLuminance = clamp(bloomLuminance,0,100);
		BloomEffect* bloom = (BloomEffect*)PEManager()->GetEffect(PE_BLOOM);
		if(bloom)
		{
			bloom->SetDefaultLuminance(0.15f-bloomLuminance*0.0009f);
			bloom->SetActive(enableBloom);
			bloom->RestoreDefaults();
		}

		if(ar.isEdit() && ar.isInput()){
			water_->SetDampfK(environmentAttributes_.water_dampf_k_); 
		}
		
		ar.serialize(water_is_lava, "water_is_lava", "Включить лаву");
		ar.serialize(RangedWrapperf(waterPFHeight, 2.f, 250.f), "waterPFHeight", "Относительный уровень глубокой воды");
		
		if(ar.isInput())	
			water()->setRelativeWaterLevel(waterPFHeight);
	}

	Outside_Environment outside;
	if (!ar.isInput())
		if (chaos)
			outside = ENVIRONMENT_CHAOS;
		else if (water_&&water_->IsShowEnvironmentWater())
			outside = ENVIRONMENT_WATER;
		else if (env_earth)
			outside = ENVIRONMENT_EARTH;
		else outside = ENVIRONMENT_NO;
	ar.serialize(outside, "outside", "Внешняя среда");
	if (water_ && ar.isInput ()) {
		water_->SetRainConstant(-environmentAttributes_.rainConstant_ * rain_multiplicator);
		water_->SetEnvironmentWater(waterHeight);
		water_->ShowEnvironmentWater(outside == ENVIRONMENT_WATER);
		if(ar.isEdit())
		{
			water_->SetReflectionColor(reflection_color);
			water_->SetReflectionBrightnes(reflection_brightnes);
		}

		SetWaterTechnique();
	}

	ar.serialize(fog_enable_,"fog_enable","Включить туман");

	ar.serialize(need_alpha_tracking,"need_alpha_tracking","Включить прозрачность объектов");
	if (environmentTime_)
	{
		if(ar.isInput())
			environmentTime_->SetFogCircle(fog_enable_);
		environmentTime_->serializeParameters (ar);
		if(ar.isInput())
			environmentTime_->setFogHeight(environmentAttributes_.height_fog_circle_);
	}
	
	
	cEffect::test_far_visible = environmentAttributes_.effectHideByDistance_;
	cEffect::near_distance = sqr(environmentAttributes_.effectNearDistance_);
	cEffect::far_distance = sqr(environmentAttributes_.effectFarDistance_);
	gb_VisGeneric->SetHideFactor(environmentAttributes_.hideByDistanceFactor_);
	gb_VisGeneric->SetHideRange(environmentAttributes_.hideByDistanceRange_);
	gb_VisGeneric->SetHideSmoothly(environmentAttributes_.hideSmoothly_);

	ar.openBlock("Camera restrictions", "настройки камеры");
	{
		ar.serialize(ownCameraRestriction_, "selfCameraRestriction", "&использовать собственные ограничения камеры");
		if(!ar.serialize(cameraBorder_, "cameraBorder", "границы выезда за край миры")){ // CONVERSION 26.09.06 - перенести выше, после конверсии
			if(ownCameraRestriction_)
                cameraBorder_.serialize(ar);
			else
				cameraBorder_ = GlobalAttributes::instance().cameraBorder;
		}
		if(ar.isInput())
			cameraManager->setCameraBorder(cameraBorder_);
		if(ownCameraRestriction_){
			cameraRestriction_.serialize(ar);
			if(ar.isInput())
				cameraManager->setCameraRestriction(cameraRestriction_);
		}
		else
			if(ar.isInput())
				cameraManager->setCameraRestriction(GlobalAttributes::instance().cameraRestriction);
	}
	ar.closeBlock();

	if(water_){
		//ar.openBlock("Ocean waves", "Волны");
		//	waterPlumeAttribute_.serialize(ar);
			fixedWaves_->serialize(ar);
		//ar.closeBlock();
		if(pCoastSprite)
			pCoastSprite->Init(environmentAttributes_.coastSprites_);
	}

	fallout_->SetupAttributes(environmentAttributes_.fallout_);

	//ar.openBlock ("Wind", "Ветер");
	//windMap->serialize(ar);
	//ar.closeBlock ();
	windMap->init(environmentAttributes_.windMap_);

	if(water_){
		sColor4c border_color(water_->GetEarchColor());

		CKeyColor opacityGradient(water_->GetOpacity());
		water_->ClampOpacity(opacityGradient);

		ar.serialize(static_cast<WaterGradient&>(opacityGradient), "zLevelOpacityGradient", "Прозрачность воды на разной глубине");
		ar.serialize(reinterpret_cast<sColor3c&>(border_color), "border_color", "Цвет земли за границей мира");

		if(ar.isInput()){
			water_->ClampOpacity(opacityGradient);
			water_->SetOpacity(opacityGradient);
			water_->SetEarthColor(border_color);
		}
	}
	//fallout_->serializeColors (ar);

	if(fogOfWar_ && ar.isInput()){
		fogOfWar_->SetFogMinimapAplha(environmentAttributes_.fogMinimapAlpha_);
		fogOfWar_->SetFogColor(environmentAttributes_.fogOfWarColor_);

		if(::isUnderEditor() && environmentAttributes_.scout_area_alpha_ != fogOfWar_->GetScoutAreaAlpha() && universe())
			universe()->activePlayer()->fogOfWarMap()->ClearMap();
		fogOfWar_->SetScoutAreaAlpha(environmentAttributes_.scout_area_alpha_);
	}

	if (environmentTime_)
		environmentTime_->serializeColors (ar);

	ContainerMiniTextures &mini = scene_->GetTilemapDetailTextures();
	ar.serialize(minimapAngle_, "minimapAngle", "Угол поворота миникарты");
	if(!ar.isEdit()){
		vector<string> miniDetailTex;
		if (ar.isOutput())
			miniDetailTex = mini.TexNames();
		ar.serialize(miniDetailTex, "miniDetailTex", "mini detail textures");
		if (ar.isInput() && !miniDetailTex.empty())
		{
			//Плохо так, нужна какаято косвенная адресация, дальше хуже будет, если появятся материалы на эти текстуры.
			//Да и печально когда так жёстко все забито!
			int sz = miniDetailTex.size();
			if(sz==mini.TexNames().size())
			{
				for(int i=0; i<sz; i++)
					mini.SetTexture(i, miniDetailTex[i].c_str());
			}else
			{
				for(int i=0; i<sz; i++)
					mini.SetTexture(i==0?0:(i+1), miniDetailTex[i].c_str());
			}
		}
	}

	ar.serialize(*lensFlare_, "lensFlare_", "Блик камеры");
	ar.serialize(*fallLeaves_, "fallLeaves", "Падающие листья");
	if(!ar.isEdit())
		ar.serialize(shadowWrapper_, "shadowWrapper", 0);
	
	if(ar.isInput() && cameraManager){
		cameraManager->SetFrustum();
		if(lensFlare_)
			lensFlare_->setCameraClip(environmentAttributes_.game_frustrum_z_min_, max(environmentAttributes_.game_frustrum_z_max_horizontal_, environmentAttributes_.game_frustrum_z_max_vertical_));
	}

	if(grassMap){
		ar.openBlock("Grass", "Трава");
		grassMap->serialize(ar);
		ar.closeBlock();
	}

	terMapPoint->GetMiniDetailRes() = 1<<environmentAttributes_.miniDetailTexResolution_;
    //// СЕРИАЛИЗОВАТЬ ТЕКСТУРЫ ЗДЕСЬ: //////////////////////////////////////////////////////////////////////////////
	ar.serialize(ResourceSelector(ice_snow_texture,ResourceSelector::TEXTURE_OPTIONS),"ice_snow_texture","ЛЕД: Текстура снега");
	ar.serialize(ResourceSelector(ice_bump_texture,ResourceSelector::TEXTURE_OPTIONS),"ice_bump_texture","ЛЕД: Текстура бампа");
	ar.serialize(ResourceSelector(lava_texture,ResourceSelector::TEXTURE_OPTIONS),"lava_texture","Лава: Текстура");
	if (ar.isInput()) {
		SetLavaTexture(lava_texture.c_str());
	}
    ar.serialize(ResourceSelector(env_earth_texture, ResourceSelector::TEXTURE_OPTIONS),
                        "env_earth_texture", "Текстура окружающей земли");

	ar.serialize(ResourceSelector(chaosWorldGround0, ResourceSelector::TEXTURE_OPTIONS),
                        "chaosWorldGround0", "ХАОС: Текстура 1");
	ar.serialize(ResourceSelector(chaosWorldGround1, ResourceSelector::TEXTURE_OPTIONS),
                        "chaosWorldGround1", "ХАОС: Текстура 2");
    ar.serialize(ResourceSelector(chaosOceanBump, ResourceSelector::TEXTURE_OPTIONS),
                        "chaosOceanBump", "ХАОС: Bump текстура");

	ar.serialize(ResourceSelector(water_ice_snow_texture, ResourceSelector::TEXTURE_OPTIONS),
                        "water_ice_snow_texture", "ЛЕД НА ВОДЕ: Текстура снега");
    ar.serialize(ResourceSelector(water_ice_bump_texture, ResourceSelector::TEXTURE_OPTIONS),
                        "water_ice_bump_texture", "ЛЕД НА ВОДЕ: Bump текстура");
	ar.serialize(ResourceSelector(water_ice_cleft_texture, ResourceSelector::TEXTURE_OPTIONS),
                        "water_ice_cleft_texture", "ЛЕД НА ВОДЕ: Текстура трещин");
	if(cloud_shadow)
		cloud_shadow->serialize(ar);

	//// ТЕКСТУРЫ ЗАКОНЧИЛИСЬ, ПОСТ-СЕРИАЛИЗАЦИОННАЯ ИНИЦИАЛИЗАЦИЯ: /////////////////////////////////////////////////

	if(ar.isInput())
	{
		if (outside ==ENVIRONMENT_CHAOS)
		{
			if (!chaos)	
			{
				chaos = terScene->CreateChaos(Vect2f(vMap.H_SIZE, vMap.V_SIZE),
					chaosWorldGround0.c_str(), chaosWorldGround1.c_str(), chaosOceanBump.c_str(), 5, true);
				if (terScene->GetTileMap()) 
					terScene->GetTileMap()->UpdateMap(Vect2i(0,0), Vect2i((int)vMap.H_SIZE, (int)vMap.V_SIZE));
			}
			else
				chaos->SetTextures( chaosWorldGround0.c_str(), chaosWorldGround1.c_str(), chaosOceanBump.c_str());
		} else if (chaos) 
		{
			RELEASE(chaos);
			if (terScene->GetTileMap()) 
				terScene->GetTileMap()->UpdateMap(Vect2i(0,0), Vect2i((int)vMap.H_SIZE, (int)vMap.V_SIZE));
		}
		if (outside == ENVIRONMENT_EARTH)
		{
			if (!env_earth)
			{
				env_earth = new cEnvironmentEarth(env_earth_texture.c_str());
				scene_->AttachObj(env_earth);
			//	environmentTime()->GetNormalScene()->AttachObj(env_earth);
			}
			else env_earth->SetTexture(env_earth_texture.c_str());
		}
		else if (env_earth) RELEASE(env_earth);

		if(temperature_)
			temperature_->SetTexture(water_ice_snow_texture.c_str(),water_ice_bump_texture.c_str(),water_ice_cleft_texture.c_str());

		universe()->TurnOnTransparentTracking(need_alpha_tracking);
	}

	environmentTime_->ReplaceGlobal(GlobalAttributes::instance().environmentAttributes_.timeColors_);
	if(ar.isInput())
		environmentTime_->SetTime(environmentTime_->GetTime());
}

void Environment::SetDofParams(Vect2f& params)
{
	if(!environment->PEManager())
		return;
	DOFEffect* dof = (DOFEffect*)environment->PEManager()->GetEffect(PE_DOF);
	if(dof)
		dof->SetDofParams(params);

}

void Environment::SetWaterTechnique()
{
	if(GameOptions::instance().getBool(OPTION_REFLECTION))
	{
		water_->SetTechnique(water_is_lava?cWater::WATER_LAVA:cWater::WATER_LINEAR_REFLECTION);
	}else
	{
		water_->SetTechnique(water_is_lava?cWater::WATER_LAVA:cWater::WATER_REFLECTION);
	}
}

MultiRegion* TileMapRegion();

void ConvertRegionToMultiregion(MultiRegion& out,Column& in,int type)
{
	xassert(out.height()==in.size());
	for(int y=0;y<out.height();y++)
	{
		CellLine& cellline=in[y];
		MultiRegion::Line& multiline=out.lines()[y];
		multiline.clear();
		list<Cell>::iterator it;
		FOR_EACH(cellline,it)
		{
			MultiRegion::Interval tmp;
			tmp.type=type;
			tmp.x0=it->xl;
			multiline.push_back(tmp);
			tmp.type=0;
			tmp.x0=it->xr+1;
			multiline.push_back(tmp);
		}
	}
}

STARFORCE_API void Environment::serializeSourcesAndAnchors(Archive& ar)
{
	SECUROM_MARKER_HIGH_SECURITY_ON(14);
	
	if(ar.isInput())
		clearSources();

	if(!ar.isEdit()) {
		ar.serialize(sources_, "sources", 0);

		// удаляем нулевые источники
		Sources::iterator it;
		for(it = sources_.begin(); it != sources_.end();) {
			if(!*it) {
				it = sources_.erase(it);
			} else {
				++it;
			}
		}
	}

	if(!ar.isEdit())
		ar.serialize(anchors_, "anchors", 0);

	if(ar.isInput())
		reload();

	SECUROM_MARKER_HIGH_SECURITY_OFF(14);
}

STARFORCE_API void Environment::serialize(Archive& ar) 
{
	SECUROM_MARKER_HIGH_SECURITY_ON(15);
	start_timer_auto();

	serializeParameters(ar);
	if (water_)	{
		ar.serialize(*water_,"Water", 0);

		//waterBubble_->serialize(ar);

		if(temperature_)
			ar.serialize(*temperature_,"temperature", 0);

	}

	serializeSourcesAndAnchors(ar);

	terMapPoint->GetTerra()->LockColumn();
	MultiRegion& region=*TileMapRegion();
	ar.serialize(region, "MultiDetailRegion", "MultiDetailRegion");

	if(ar.isInput ())
	{
		if(region.width()!=vMap.H_SIZE ||
		   region.height()!=vMap.V_SIZE)
		{
			kdError("3d", TRANSLATE("Регион мелкодетальной текстуры имеет некорректные размеры и будет очищен."));
			region.init(vMap.H_SIZE,vMap.V_SIZE,1);
		}

		region.validate();
		region.fixRegion(1);
		for(int i=0; i< terMapPoint->GetZeroplastNumber();i++){
			char str[100];
			sprintf(str, "DetailRegion%d", i);	
			string name = i==0 ? "iceRegion" : str;
			RegionDispatcher cur_detail_region(vMap.V_SIZE);
			if(ar.serialize(cur_detail_region, name.c_str(), name.c_str()))
			{
				MultiRegion cur_region(vMap.H_SIZE,vMap.V_SIZE);
				ConvertRegionToMultiregion(cur_region,cur_detail_region.getEditColumn(),i+1);
				TileMapRegion()->add(cur_region);
			}
		}
	}
	
	terMapPoint->GetTerra()->UnlockColumn();

	if(ar.isInput () && false)//Code to create view3dx sun data
	{
		if(environmentTime_)
			environmentTime_->ExportSunParameters("sun.inl");
	}

	SECUROM_MARKER_HIGH_SECURITY_OFF(15);
}



void Environment::setShowFogOfWar (bool show)
{
	if (fogOfWar_) {
		scene_->EnableFogOfWar(show);
		minimap().toggleShowFogOfWar(show);
		if (!show)
			universe()->ClearOverpatchingFOW();
	}
}

string Environment::sourceNamesComboList(SourceType sourceType) 
{
	string comboList;
	Sources::const_iterator i;
	FOR_EACH(sources_, i)
	{
		SourceBase* p=*i;
		if(p->type()==sourceType || sourceType==SOURCE_MAX)
		if(strlen(p->label())){
			comboList += p->label();
			comboList += "|";
		}
	}
	return comboList;
}

SourceBase* Environment::getSource(SourceType sourceType, const char* sourceName)
{
	Sources::iterator i;
	FOR_EACH(sources_, i)
	{
		SourceBase* p=*i;
		if((p->type()==sourceType || sourceType==SOURCE_MAX)
			&& strcmp(p->label(), sourceName)==0)
			return p;
	}

	return 0;
}

Anchor* Environment::findAnchor(const char* anchorName) const
{
	Anchors::const_iterator it;
	FOR_EACH(anchors_, it){
		Anchor* p = *it;
		if(*p == anchorName)
			return p;
	}

	return 0;
}

void Environment::getTypeSources(SourceType type,Sources& out)
{
	Sources::iterator it;
	out.clear();
	FOR_EACH(sources_,it)
	if((*it)->type()==type)
	{
		out.push_back(*it);
	}
}

/////////////////////////////////////////////////////////
cFlash::cFlash()
{
	color_ = sColor4c(255,255,255);
	active_ = false;
	
	inited_ = false;
	intensity_[0] = intensity_[1] = 0.0f;
	count_=0;
	intensitySum_ = 0.0f;
}

cFlash::~cFlash()
{
}

// графический квант
void cFlash::draw()
{
	if (!active_ || !inited_)
		return;

	cInterfaceRenderDevice* rd = gb_RenderDevice;
	sColor4f color = sColor4f(color_);
	float intFactor = timer_();
	float factor = clamp(((1.f - intFactor) * intensity_[0] + intFactor * intensity_[1]),0,1);
	color.a *= factor;
	float l = 0.15f;
	BloomEffect* bloom = (BloomEffect*)environment->PEManager()->GetEffect(PE_BLOOM);
	if(bloom)
	{
		if (bloom->IsActive())
			l = bloom->GetDefaultLuminance();
		bloom->SetLuminace(l-(l-0.06f)*color.a);
		color*=color.a;
		bloom->SetColor(color);
	}
}

void cFlash::init(float _intensity){
	//addIntensity(_intensity);
	intensity_[0] = intensity_[1] = _intensity;
	inited_ = true;
}

void cFlash::setActive(bool _active)
{
	if(!_active && count_>0)
		return;
	active_ = _active;

	BloomEffect* bloom = (BloomEffect*)environment->PEManager()->GetEffect(PE_BLOOM);
	if(!active_)
	{
		inited_ = false;
		if(bloom)
			bloom->RestoreDefaults();
	}
	if(bloom)
		bloom->SetExplode(active_);
}

void cFlash::setColor(sColor4c _color)
{
	color_ = _color;
}

// вызывать из логического кванта
void cFlash::addIntensity(float _intensity)
{
	intensitySum_ += _intensity;
}

void cFlash::setIntensity()
{
	if(!active_)
		return;
	if(inited_){
		intensity_[0] = intensity_[1];
		intensity_[1] = intensitySum_;
		timer_.start(logicTimePeriod);
	}else
		init(intensitySum_);
	intensitySum_ = 0.0f;
}

bool Environment::isUnderWaterSilouette(const Vect3f& pos)
{
	if(!water()->isUnderWater(pos))
		return false;
	UnderWaterEffect* underWater = (UnderWaterEffect*)PEManager()->GetEffect(PE_UNDER_WATER);
	return !(underWater && underWater->isUnderWater());
}

bool Environment::isVisibleUnderForOfWar(const UnitBase* unit, bool checkAllwaysVisible) const
{
	xassert(unit);
	return	fogOfWar_
		? (checkAllwaysVisible && unit->attr().fow_mode == FVM_ALLWAYS ? true : fogOfWar_->GetSelectedMap()->isVisible(unit->position2D()))
		: true;
}

bool Environment::isPointVisibleUnderForOfWar(const Vect2i& pos) const
{
	return	fogOfWar_? fogOfWar_->GetSelectedMap()->isVisible(pos) : true;
}

bool Environment::isDay() const
{
	return environmentTime_->IsDay();
}

float Environment::getTime() const
{
	return environmentTime_->GetTime();
}

bool Environment::dayChanged() const
{
	return environmentTime_->DayChanged();
}

void Environment::setCameraRestriction(const CameraRestriction& restriction, bool ownCameraRestriction)
{
	 cameraRestriction_ = restriction;
	 ownCameraRestriction_ = ownCameraRestriction;
}

void Environment::setCameraBorder(const CameraBorder& cameraBorder)
{
	 cameraBorder_ = cameraBorder;
}


WaterPlumeAttribute::WaterPlumeAttribute()
{
	waterPlumeTextureName = ".\\Resource\\TerrainData\\Textures\\G_Tex_WaterRings_001.tga";
	waterPlumeFrequency = 0.3f;
}

void WaterPlumeAttribute::serialize(Archive& ar)
{
	float circle_time = 1.0f / waterPlumeFrequency;
	ar.serialize(circle_time, "circle_time", "Время кругов");
	waterPlumeFrequency = 1.0f / circle_time;

	ar.serialize(ResourceSelector(waterPlumeTextureName, ResourceSelector::TEXTURE_OPTIONS),
		"cyrcle_texture_name", "ШЛЕЙФ : Текстура кругов");
}
