#include "stdafx.h"
#include "SourceManager.h"
#include "Anchor.h"
#include "UserInterface\UI_Logic.h"
#include "UserInterface\UserInterface.h"
#include "UserInterface\UI_Minimap.h"
#include "SourceShield.h"
#include "Actions.h"
#include "Units\ShowChangeController.h"
#include "Units\UnitAttribute.h"
#include "Game\Universe.h"
#include "Game\CameraManager.h"
#include "Environment.h"
#include "Water\Water.h"
#include "Water\Ice.h"
#include "Serialization\SerializationFactory.h"
#include "Terra\vMap.h"

REGISTER_CLASS(Anchor, Anchor, "Якорь на мире");

SourceManager* sourceManager;

SourceManager::SourceManager()
: sourceGrid(vMap.H_SIZE, vMap.V_SIZE)
{
	sourceManager = this;

}

SourceManager::~SourceManager()
{
	clearSources();
	
	if(sourceOnMouse_) 
		sourceOnMouse_->setActivity(false);

	std::for_each(sounds_.begin(), sounds_.end(), std::mem_fun_ref(&SoundController::release));
	sounds_.clear();

	sourceManager = 0;
}

void SourceManager::flushNewSources()
{
	sources_.insert(sources_.end(), newSources_.begin(), newSources_.end());
	newSources_.clear();
}

void SourceManager::setSourceOnMouse(const SourceBase* source)
{
	if(sourceOnMouse_)
		sourceOnMouse_->kill();
	
	if(!source) return;
	
	sourceOnMouse_ = addSource(source);
	sourceOnMouse_->setPose(Se3f(QuatF::ID, UI_LogicDispatcher::instance().hoverPosition()), true);
	sourceOnMouse_->setActivity(true);
}

void SourceManager::serialize(Archive& ar)
{
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

	if(ar.isInput()){
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
}

void SourceManager::logicQuant()
{
	start_timer_auto();
	statistics_add(sources, sources_.size());

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
	
	start_timer(sounds);
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
	stop_timer(sounds);

	start_timer(anchors);
	Anchors::iterator ait;
	FOR_EACH(anchors_, ait){
		if((*ait)->selected())
			minimap().addAnchor(*ait);
	}
	stop_timer(anchors);
}

void SourceManager::drawUI(float dt)
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
	sourceManager->showChangeControllers_.push_back(share);
}

ShowChangeController* SourceManager::addShowChangeController(const ShowChangeController& ctrl)
{
	MTL();
	SharedShowChangeController share(new ShowChangeController(ctrl));
	share->addRef(); // что бы не протух пока лежит в потоке
	streamLogicCommand.set(fCommandAddShowChangeController) << share;
	return share;
}

void SourceManager::clearSources()
{
	flushNewSources();
	std::for_each(sources_.begin(), sources_.end(), bind2nd(mem_fun(&SourceBase::setActivity), false));
	sources_.clear();
}

void SourceManager::showEditor()
{
    Camera* camera=cameraManager->GetCamera();

	if(!showDebugSource.dontShowInfo){
		Sources::iterator it;
		FOR_EACH(sources_,it){
			SourceBase* source = *it;
			if(source->isAlive())
				source->showEditor();
		}
	}

	Anchors::iterator ait;
	FOR_EACH(anchors_, ait)
		(*ait)->showEditor();
}

void SourceManager::showDebug() const
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
		
		LabelObject fst("__first_debug_anchor");
		Vect3f start = fst ? fst->position() : Vect3f::ZERO;

		LabelObject scn("__second_debug_anchor");
		Vect3f finish = scn ? scn->position() : Vect3f::ZERO;

		if(fst && scn){
			Vect3f intersect;
			bool abil = SourceShield::traceShieldsThrough(start, finish, 0, &intersect);
			show_vector(start, 3.f, Color4c::YELLOW);
			show_vector(finish, 3.f, Color4c::YELLOW);
			if(abil)
				show_line(start, finish, Color4c::GREEN);
			else{
				show_line(start, finish, Color4c::RED);
				show_line(intersect + Vect3f (-5.0f, 0.0f, 0.0f), intersect + Vect3f (5.0f, 0.0f, 0.0f), Color4c::RED);
				show_line(intersect + Vect3f (0.0f, -5.0f, 0.0f), intersect + Vect3f (0.0f, 5.0f, 0.0f), Color4c::RED);
				show_line(intersect + Vect3f (0.0f, 0.0f, -5.0f), intersect + Vect3f (0.0f, 0.0f, 5.0f), Color4c::RED);
			}
		}
	}
}

bool SourceManager::checkEnvironment(const Vect3f& pos, int types) const
{
	if(types == SOURCE_SURFACE_ANY)
		return true;
	
	start_timer_auto();

	cWater* water = environment->water();
	cTemperature* temperature = environment->temperature();

	bool fullWater = false;
	if((types & (SOURCE_SURFACE_GROUND | SOURCE_SURFACE_WATER)) != 0)
		fullWater = (water && water->isFullWater(pos));

	bool onIce = false;
	if((types & (SOURCE_SURFACE_WATER | SOURCE_SURFACE_ICE)) != 0)
		onIce = (temperature && temperature->isOnIce(pos));

	bool onWater = fullWater;
	if((types & SOURCE_SURFACE_ICE) != 0 && !onWater)
		onWater = (water && water->isWater(pos));

	if((types & SOURCE_SURFACE_GROUND) != 0)
		if(!fullWater)
			return true;

	if((types & SOURCE_SURFACE_WATER) != 0)
		if(fullWater && !onIce)
			return true;

	if((types & SOURCE_SURFACE_ICE) != 0)
		if(onIce && onWater)
			return true;

	return false;
}

SourceBase* SourceManager::createSource(const SourceAttribute* attribute, const Se3f& pose, bool allow_unlimited_lifetime, bool* startFlag)
{
	if(attribute->isEmpty())
		return 0;
	
	start_timer_auto();

	if(!(1 << vMap.getSurKind(pose.trans().xi(), pose.trans().yi()) & attribute->source()->surfaceKind()))
		return 0;

	if(!checkEnvironment(pose.trans(), attribute->source()->surfaceInstallClass()))
		return 0;

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

SourceBase* SourceManager::addSource(const SourceBase* original)
{
	xassert(original);
	SourceBase* result = original->clone();
	newSources_.push_back(result);

	xassert(!result->active());
	result->enable();

	return result;
}

Anchor* SourceManager::addAnchor()
{
	anchors_.push_back(new Anchor());
	return anchors_.back();
}

Anchor* SourceManager::addAnchor(const Anchor* original)
{
	xassert(original);
	anchors_.push_back(new Anchor(*original));
	return anchors_.back();
}

void SourceManager::removeAnchor(const Anchor* src)
{
	Anchors::iterator it;
	FOR_EACH(anchors_, it)
		if(*it == src){
			if(*it)
				(*it)->Kill();
			anchors_.erase(it);
			return;
		}
}

bool SourceManager::soundAttach(const SoundAttribute* sound, const BaseUniverseObject *obj)
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

void SourceManager::soundRelease(const SoundAttribute* sound, const BaseUniverseObject *obj)
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

bool SourceManager::soundIsPlaying(const SoundAttribute* sound, const BaseUniverseObject *obj)
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

void SourceManager::deselectAll()
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

void SourceManager::deleteSelected()
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

string SourceManager::sourceLabelsComboList() const
{
	string comboList;
	Sources::const_iterator i;
	FOR_EACH(sources_, i)
		if(strlen((*i)->label())){
			comboList += (*i)->label();
			comboList += "|";
		}
	return comboList;
}

SourceBase* SourceManager::findSource(const char* sourceName) const 
{
	Sources::const_iterator i;
	FOR_EACH(sources_, i)
		if(!strcmp((*i)->label(), sourceName))
			return *i;

	return 0;
}

BaseUniverseObject* SourceManager::findAnchor(const char* anchorName) const
{
	Anchors::const_iterator it;
	FOR_EACH(anchors_, it){
		Anchor* p = *it;
		if(!strcmp(p->label(), anchorName))
			return p;
	}

	BaseUniverseObject* source = findSource(anchorName);
	if(source)
		return source;

	return universe()->findUnitByLabel(anchorName);
}

string SourceManager::anchorLabelsComboList() const
{
	string comboList;
	Anchors::const_iterator i;
	FOR_EACH(anchors_, i)
		if(strlen((*i)->label())){
			comboList += (*i)->label();
			comboList += "|";
		}

	string sources = sourceLabelsComboList();
	if(!sources.empty()){
		comboList += sources;
		comboList += "|";
	}

	string units = universe()->unitLabelsComboList();
	if(!units.empty()){
		comboList += units;
		comboList += "|";
	}

	return comboList;
}


void SourceManager::getTypeSources(SourceType type,Sources& out)
{
	Sources::iterator it;
	out.clear();
	FOR_EACH(sources_,it)
		if((*it)->type()==type)
			out.push_back(*it);
}

