#include "stdafx.h"
#include "ChainLightningController.h"

#include "Serialization\Serialization.h"
#include "Environment.h"
#include "DebugPrm.h"
#include "RenderObjects.h"
#include "Serialization\RangedWrapper.h"
#include "Universe.h"
#include "Squad.h"
#include "Render\Src\Scene.h"

void ChainLightningAttribute::serialize(Archive& ar)
{
	ar.serialize(RangedWrapperf(chainRadius_, 1.f, 2000.f), "chainRadius", "радиус цепного эффекта");
	ar.serialize(RangedWrapperi(chainFactor_, 1, 10), "chainFactor", "максимальная глубина развилок");
	ar.serialize(RangedWrapperf(unitChainRadius_, 1.f, 500.f), "unitChainRadius", "радиус отскока от юнита");
	ar.serialize(RangedWrapperi(unitChainFactor_, 1, 10), "unitChainFactor", "максимально количество вторичных молний от юнита");
	ar.serialize(strike_effect_, "strike_effect", "атака юнита в зоне");
	ar.serialize(RangedWrapperf(spreadingTime_, 0.f, 120.f), "spreadingTime", "время распространения");
	ar.serialize(abnormalState_, "abnormalState", "воздействие на юниты");
}

ChainLightningAttribute::ChainLightningAttribute()
{
	chainFactor_ = 3;
	unitChainFactor_ = 3;

	chainRadius_ = 150.f;
	unitChainRadius_ = 80.f;

	spreadingTime_ = 0.5f;
}

ChainLightningController::ChainLightningController(ChainLightningOwnerInterface* owner)
{
	owner_ = owner;
	attr_ = 0;

	phase_ = NOT_STARTED;
	grafValid_ = true;
}

ChainLightningController::~ChainLightningController()
{
	release();
}

void ChainLightningController::start(const ChainLightningAttribute* attr)
{
	xassert(owner_);
	attr_ = attr;

	if(phase_ != NOT_STARTED)
		release();


	if(!attr_->strike_effect_.get()){
		xxassert(!damage_.empty() || attr_->abnormalState_.isEnabled(), "без назначенного эффекта урон цепной молнией наносится не будет");
		return;
	}

	damage_ = owner_->damage();

	phase_ = FADE_ON;
	phaseTime_.start();

	grafValid_ = false;
}

void ChainLightningController::stop()
{
	if(phase_ == NOT_STARTED || phase_ == FADE_OFF)
		return;
	phase_ = FADE_OFF;
	phaseTime_.start();
}

class SourceLightningChainScanOperator
{
	typedef SwapVector<UnitActing*> UnitActings;

	class NearestFunctor{
		Vect3f center_;
		float nearestDistance2_;
		UnitActing* unit_;
		const UnitCache& unitCache_;

	public:
		NearestFunctor(const Vect3f& center, float radius, const UnitCache& unitCache)
			: center_(center)
			, unit_(0)
			, unitCache_(unitCache)
		{
			nearestDistance2_ = sqr(radius);
		}

		inline void operator() (UnitActing* pretendent){
			if(pretendent->alive() && unitCache_.find(pretendent->unitID().index()) == unitCache_.end()){
				Vect3f dist;
				dist.sub(pretendent->position(), center_);
				float dist2 = dist.norm2();
				if(dist2 < nearestDistance2_){
					dist2 = nearestDistance2_;
					unit_ = pretendent;
				}
			}
		}

		inline UnitActing* unit() { return unit_; }
	};

	const ChainLightningOwnerInterface* zone_;
	Vect2f center_;
	float radius_;
	UnitActings units_;
	const UnitCache& unitCache_;


public:
	SourceLightningChainScanOperator(const ChainLightningOwnerInterface* zone, float scanRadius, const UnitCache& unitCache)
		: zone_(zone)
		, center_(zone->position())
		, radius_(scanRadius)
		, unitCache_(unitCache)
	{
		units_.reserve(10);
	}

	void operator()(UnitBase* unit){
		if(	unit->attr().isActing()
			&& unit->position2D().distance2(center_) < sqr(radius_ + unit->radius())
			&& unitCache_.find(unit->unitID().index()) == unitCache_.end()
			&& zone_->canApply(safe_cast<UnitActing*>(unit)))
		{
			units_.push_back(safe_cast<UnitActing*>(unit));
		}
	}

	UnitActing* nearestUnit(const Vect3f& center, float radius){
		NearestFunctor nearUnitFinder(center, radius, unitCache_);
		UnitActings::iterator it;
		FOR_EACH(units_, it)
			nearUnitFinder(*it);
		return nearUnitFinder.unit();
	}
};


void ChainLightningController::update(const UnitActingList& units)
{
	if(phase_ == NOT_STARTED)
		return;
	xassert(owner_ && attr_);

	unitCache_.clear();
	chainGraf_.clear();

	ChainNodesLinks currentLevelTargets, nextLevelTargets;

	UnitActingList::const_iterator it;
	FOR_EACH(units, it)
		if((*it)->alive()){
			unitCache_.insert((*it)->unitID().index());
			chainGraf_.push_back(*it);
			currentLevelTargets.push_back(&chainGraf_.back());
		}

	SourceLightningChainScanOperator scanOperator(owner_, attr_->chainRadius_, unitCache_);
	universe()->unitGrid.Scan(owner_->position().xi(), owner_->position().yi(), round(attr_->chainRadius_), scanOperator);

	int strikeIndex = 0;
	for(int chainLevel = 0; chainLevel < attr_->chainFactor_; ++chainLevel){

		ChainNodesLinks::iterator it = currentLevelTargets.begin();
		bool hasChanges = false;
		while(it != currentLevelTargets.end() || hasChanges){
			if(it == currentLevelTargets.end()){
				it = currentLevelTargets.begin();
				hasChanges = false;
			}
			ChainNode* node = *it;
			xassert(node->unitNode);
			if(node->strikeIndex < 0)
				node->strikeIndex = strikeIndex++;
			if(node->size < attr_->unitChainFactor_)
				if(UnitActing* target = scanOperator.nearestUnit(node->unitNode->position(), attr_->unitChainRadius_)){
					unitCache_.insert(target->unitID().index());
					node->graf.push_back(target);
					++node->size; // в list size() дорогой
					nextLevelTargets.push_back(&node->graf.back());
					hasChanges = true;
				}
				++it;
		}

		if(nextLevelTargets.empty())
			break;

		currentLevelTargets.swap(nextLevelTargets);
	}

	if(strikeIndex > effects.size())
		effects.resize(strikeIndex, 0);
	else if(strikeIndex < effects.size()){
		for(int ind = strikeIndex; ind < effects.size(); ++ind)
			turnOff(effects[ind]);
		effects.resize(strikeIndex);
	}

	chainUpdateTimer_.start(logicRNDinterval(3, 5) * 100);
	grafValid_ = true;
}

Vect3f ChainLightningController::ChainNode::center() const
{
	xassert(unitNode);
	Vect3f center = unitNode->position();
	center.z += unitNode->height() / 2.f;
	return center;
}

bool ChainLightningController::checkLevel(int level) const
{
	switch(phase_){
	case WORKING:
		return true;
	case FADE_ON:
		return phaseTime_.time() > attr_->spreadingTime_ * 1000 * level / attr_->chainFactor_;
		break;
	case FADE_OFF:
		return phaseTime_.time() <= attr_->spreadingTime_ * 1000 * level / attr_->chainFactor_;
		break;
	}
	return false;
}

void ChainLightningController::apply(UnitActing* target, int level)
{
	if(isUnderEditor())
		return;

	// начальным эмитерам повреждения наносятся самим оружием
	// выключенный эффект урон не наносит
	if(level == 0 || phase_ == FADE_OFF)
		return;

	ParameterSet dmg = damage_;
	dmg *= float(attr_->chainFactor_ + 1 - level) / (attr_->chainFactor_ + 1);
	target->setDamage(dmg, owner_->owner());

	if(attr_->abnormalState_.isEnabled())
		target->setAbnormalState(attr_->abnormalState_, owner_->owner());
}

bool ChainLightningController::chainStrikeRecurse(ChainNode& node, int deep, bool release)
{
	if(node.strikeIndex < 0){ // конец графа
		xassert(node.size == 0 && "отвилок быть не должно");
		return node.unitNode; // возможно юнит умер, тогда нужно оторвать узел
	}

	xassert(node.strikeIndex < effects.size());
	cEffect*& effect = effects[node.strikeIndex]; // эффект для узла из пула

	if(!node.unitNode || release){ // узел умер или должен быть оторван от дерева
		turnOff(effect);

		if(node.size){ // появились живые оторванные узлы, нужно перестроить дерево
			ChainNodes::iterator it;
			FOR_EACH(node.graf, it)
				chainStrikeRecurse(*it, deep + 1, true);
			grafValid_ = false;
		}

		return false;
	}

	ChainNodes::iterator it = node.graf.begin();
	while(it != node.graf.end())
		if(chainStrikeRecurse(*it, deep + 1))
			++it;
		else{
			it = node.graf.erase(it);
			--node.size;
		}

	if(!node.size){ // бить дальше не в кого, эффект без концов тоже не нужен
		turnOff(effect);
		return true;
	}

	if(!attr_->strike_effect_.get())
		return true;

	if(checkLevel(deep)){
		xassert(node.size == node.graf.size());

		ChainNodes::const_iterator cit;

		if(effect){
			streamLogicCommand.set(fCommandSetTarget, effect) << (int)node.size << node.center();
			FOR_EACH(node.graf, cit)
				streamLogicCommand << cit->center();
		}
		else {
			effect = environment->scene()->CreateEffectDetached(*attr_->strike_effect_->getEffect(1.f), 0);
			
			vector<Vect3f> ends;
			ends.reserve(node.size);
			FOR_EACH(node.graf, cit)
				ends.push_back(cit->center());

			effect->SetTarget(node.center(), ends);
			
			attachSmart(effect);
		}

		FOR_EACH(node.graf, cit) // в один цикл нельзя, внутри apply может быть обращение к streamLogicCommand или обнуление линка
			apply(cit->unitNode, deep + 1);
	}
	else
		turnOff(effect);

	return true;
}


void ChainLightningController::quant()
{
	xassert(owner_);
	switch(phase_){
	case FADE_OFF:
		if(phaseTime_.time() >= 1000 * attr_->spreadingTime_){
			phase_ = NOT_STARTED;
			phaseTime_.stop();
			release();
	case NOT_STARTED:
			return;
		}
		break;
	case FADE_ON:
		if(phaseTime_.time() >= 1000 * attr_->spreadingTime_){
			phase_ = WORKING;
			phaseTime_.stop();
		}
		break;
	}

	ChainNodes::iterator it;
	FOR_EACH(chainGraf_, it)
		chainStrikeRecurse(*it, 0);
}



void ChainLightningController::turnOff(cEffect*& eff)
{
	if(eff){
		eff->Release();
		eff = 0;
	}
}

void ChainLightningController::release()
{
	chainUpdateTimer_.stop();
	
	phase_ = NOT_STARTED;
	phaseTime_.stop();

	Effects::iterator it;
	FOR_EACH(effects, it)
		turnOff(*it);
	effects.clear();
	
	chainGraf_.clear();
	grafValid_ = true;
}

void ChainLightningController::showDebugChainRecurse(const ChainNode& node, int deep) const
{
	if(!node.unitNode)
		return;

	if(showDebugWeapon.showLightningUnitChainRadius)
		show_vector(node.unitNode->position(), attr_->unitChainRadius_, node.unitNode->selected() ? Color4c::RED : Color4c::GREEN);
	ChainNodes::const_iterator it;
	FOR_EACH(node.graf, it){
		if(it->unitNode)
			show_line(node.unitNode->position(), it->unitNode->position(), checkLevel(deep) ? Color4c::GREEN : Color4c::WHITE);
		showDebugChainRecurse(*it, deep + 1);
	}
}

void ChainLightningController::showDebug() const
{
	if(!showDebugWeapon.showChainLightning)
		return;
	if(phase_ == NOT_STARTED)
		return;
	show_vector(owner_->position(), attr_->chainRadius_, Color4c::GREEN);
	ChainNodes::const_iterator it;
	FOR_EACH(chainGraf_, it){
		if(!it->unitNode)
			continue;
		show_line(owner_->position(), it->unitNode->position(), Color4c::GREEN);
		showDebugChainRecurse(*it, 0);
	}
}
