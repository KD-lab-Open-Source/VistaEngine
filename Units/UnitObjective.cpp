#include "StdAfx.h"
#include "UnitObjective.h"
#include "Universe.h"
#include "Triggers.h"
#include "IronBuilding.h"
#include "RenderObjects.h"
#include "CameraManager.h"
#include "..\UserInterface\UserInterface.h"
#include "..\UserInterface\UI_Render.h"
#include "..\UserInterface\UI_Logic.h"
#include "..\UserInterface\UI_CustomControls.h"

void ItemHideScaner::operator()(UnitBase* unit) 
{
	if(unit->attr().isBuilding() && safe_cast_ref<const AttributeBuilding&>(unit->attr()).placementZone == placementZone_)
		needHide_ = true;
}

UnitObjective::UnitObjective(const UnitTemplate& data) 
: UnitReal(data)
, isRegisteredInRealUnits_(false)
, isRegisteredInPlayerStatistics_(true)
{
	fow_handle = -1;

	preParameters_ = parameters();

	AttributeBase::ParamShowContainer::const_iterator show_it;
	FOR_EACH(attr().parameterShowSettings, show_it)
		showChangeControllers_.push_back(ShowChangeController(this, &show_it->showChangeSettings, parameters_.findByIndex(show_it->paramTypeRef.key()), attr().initialHeightUIParam));
	parameterShowTimers_.resize(attr().parameterShowSettings.size());
}

UnitObjective::~UnitObjective()
{
	xassert(fow_handle==-1);
}

void UnitObjective::serialize (Archive& ar) 
{
	__super::serialize(ar);

	ShowChangeControllers::iterator it;
	FOR_EACH(showChangeControllers_, it)
		it->setOwner(this);

	if(universe()->userSave())
		ar.serialize(isRegisteredInPlayerStatistics_, "isRegisteredInPlayerStatistics", 0);
}

void UnitObjective::Quant()
{
	__super::Quant();

	if(!alive())
		return;

	if(cameraManager->isVisible(position()))
		universe()->addVisibleUnit(this);

	if(!hiddenLogic() && (!attr().isActing() || player() == universe()->activePlayer() || !safe_cast<const UnitActing*>(this)->isInvisible()))
		minimap().addUnit(this);

	// обработка изменения параметров
	xassert(showChangeControllers_.size() == attr().parameterShowSettings.size());
	xassert(parameterShowTimers_.size() == attr().parameterShowSettings.size());

	for(int ind = 0; ind < attr().parameterShowSettings.size(); ++ind){
		const ParameterShowSetting& cur = attr().parameterShowSettings[ind];

		showChangeControllers_[ind].update(parameters_.findByIndex(cur.paramTypeRef.key()));

		if(cur.type_ != ParameterShowSetting::LOGIC || !cur.delayShowTime)
			continue;

		int key = cur.paramTypeRef.key();
		float diff = preParameters_.findByIndex(key) - parameters_.findByIndex(key);

		switch (cur.showEvent)
		{
		case SHOW_AT_PARAMETER_INCREASE:
			if(diff < -FLT_EPS)
				break;
			continue;
		case SHOW_AT_PARAMETER_DECREASE:
			if(diff > FLT_EPS)
				break;
			continue;
		default:
			continue;
		}

		parameterShowTimers_[ind].start(cur.delayShowTime);
	}
	preParameters_ = parameters_;

}

void UnitObjective::fowQuant()
{
	if(!alive())
		return;

	if(player()->fogOfWarMap()){
		if(!(hiddenGraphic() & (HIDE_BY_TELEPORT | HIDE_BY_TRANSPORT | HIDE_BY_TRIGGER))){
			if(fow_handle == -1)
				fow_handle = player()->fogOfWarMap()->createVisible();
			player()->fogOfWarMap()->moveVisibleOuterRadius(fow_handle, pose().trans().x, pose().trans().y, sightRadius());
		}
		else{
			player()->fogOfWarMap()->deleteVisible(fow_handle);
			fow_handle = -1;
		}
	}

 	FogOfWarMap* fow = universe()->activePlayer()->fogOfWarMap();
	if(!fow || !model())
		return;

	if(!terScene->IsFogOfWarEnabled()){
		if(hiddenGraphic() & HIDE_BY_FOW)
			hide(HIDE_BY_FOW, false);
		return;
	}

	switch(attr().fow_mode){
	case FVM_HISTORY_TRACK:
		switch(fow->getFogState(position2D().xi(), position2D().yi())){
		case FOGST_NONE:
			hide(HIDE_BY_FOW, false);
			break;
		case FOGST_HALF:
			if(!(hiddenGraphic() & HIDE_BY_FOW)){
				universe()->addFowModel(model());
				hide(HIDE_BY_FOW, true);
			}
			break;
		case FOGST_FULL:
			hide(HIDE_BY_FOW, true);
			break;
		}
		break;
	case FVM_NO_FOG:
		hide(HIDE_BY_FOW, fow->getFogState(position2D().xi(), position2D().yi()) != FOGST_NONE);
		break;
	}
}

bool UnitObjective::checkShowEvent(ShowEvent event)
{
	bool hovered = (UI_LogicDispatcher::instance().hoverUnit() == this);

	switch (event){
	case SHOW_AT_HOVER_OR_SELECT:
		return hovered || selected();
	case SHOW_AT_HOVER:
		return hovered;
	case SHOW_AT_SELECT:
		return selected();
	case SHOW_ALWAYS:
		return true;
	case SHOW_AT_NOT_HOVER_OR_SELECT:
		return !hovered && !selected();
	case SHOW_AT_BUILD_OR_UPGRADE:
		if(isUpgrading())
			return true;
	case SHOW_AT_BUILD:
		return !isConstructed();
	case SHOW_AT_DIRECT_CONTROL:
		if(attr().isActing()) // FIXME - rigidBody() обещали переписать
			return safe_cast<const UnitActing*>(this)->directControl();
	}
	return false;
}

bool UnitObjective::getParameterValue(const ParameterShowSetting& par, float &phase)
{
	phase = 0.0f;
	switch(par.type_){
	case ParameterShowSetting::LOGIC:{
		int idx = par.paramTypeRef.key();
		phase = parametersMax().findByIndex(idx);
		if(phase > FLT_EPS)
			phase = clamp(parameters().findByIndex(idx) / phase, 0.0f, 1.0f);
		else
			return false;
		break;
									 }
	case ParameterShowSetting::PRODUCTION_PROGRESS:
		phase = productionProgress();
		break;

	case ParameterShowSetting::UPGRADE_PROGRESS:
		if(attr().isActing())
			phase = safe_cast<const UnitActing*>(this)->upgradeProgres(par.dataType, false);
		else
			return false;
		break;

	case ParameterShowSetting::FINISH_UPGRADE:
		if(attr().isActing())
			phase = safe_cast<const UnitActing*>(this)->upgradeProgres(par.dataType, true);
		else
			return false;
		break;

	default:
		return false;
	}

	if(par.notShowEmpty && phase < FLT_EPS)
		return false;

	return true;
}

void UnitObjective::graphQuant(float dt)
{
	__super::graphQuant(dt);

	if(!alive())
		return;

	if(!UI_Dispatcher::instance().isEnabled())
		return;
	// все что ниже относится к выводу информации которая должна отключаться вместе с интерфейсом

	if(isShowAISign() && player()->active())
		player()->race()->workForAISprite().draw(interpolatedPose().trans(), attr().initialHeightUIParam);

	if(runMode() && (!attr().isActing() || !safe_cast<const UnitActing*>(this)->isDirectControl()))
		player()->race()->runModeSprite().draw(interpolatedPose().trans(), attr().initialHeightUIParam);

	if(UI_Render::instance().windowPosition().width() > 0 && UI_Render::instance().windowPosition().height() > 0){
		Vect3f e;
		Vect3f pv;
		Vect3f pos = interpolatedPose().trans();
		cameraManager->GetCamera()->ConvertorWorldToViewPort(&pos,&pv,&e);

		for(int ind = 0; ind < attr().parameterShowSettings.size(); ++ind)
		{
			showChangeControllers_[ind].quant();

			if(hiddenGraphic() || pv.z < 0.1f)
				continue;
			
			const ParameterShowSetting& cur = attr().parameterShowSettings[ind];

			bool need_draw = checkShowEvent(cur.showEvent);

			if(!need_draw)
				need_draw = parameterShowTimers_[ind]();

			if(!need_draw)
				switch(cur.showEvent){
				case SHOW_AT_SELECT:
				case SHOW_AT_HOVER:
				case SHOW_AT_HOVER_OR_SELECT:
					need_draw = UI_LogicDispatcher::instance().showAllUnitParametersMode();
				}

			if(!need_draw)
				continue;

			float radiusFactor = min(cameraManager->GetCamera()->GetFocusViewPort().x / pv.z, 20.f);
			float radiusFactorY = max(2.f, radiusFactor);
			float len = cur.barLength * radiusFactor;
			float half = 0.0f;
			if (ParameterShowSetting::ALIGN_CENTER == cur.alignType)
				half = len/2.0f;
			
			float phase;
			if(getParameterValue(cur, phase)){
				sColor4f phase_clr;
				phase_clr.interpolate(cur.minValColor, cur.maxValColor, phase);
				UI_Render::instance().drawRectangle( // --> Рамка
					UI_Render::instance().relativeCoords(Recti(
					e.x + radiusFactor * cur.paramOffset.x - half - 1, 
					e.y + radiusFactorY * cur.paramOffset.y - 1, 
					len + 1, 3)),
					cur.borderColor, true);
				UI_Render::instance().drawRectangle( // --> Подложка
					UI_Render::instance().relativeCoords(Recti(
					e.x + radiusFactor * cur.paramOffset.x - half, 
					e.y + radiusFactorY * cur.paramOffset.y, 
					len, 2)),
					cur.backgroundColor);
				UI_Render::instance().drawRectangle( // --> Значение
					UI_Render::instance().relativeCoords(Recti(
					e.x + radiusFactor * cur.paramOffset.x - half, 
					e.y + radiusFactorY * cur.paramOffset.y, 
					phase * len, 2)),
					phase_clr);
			}
		}
	}
}

const UI_MinimapSymbol* UnitObjective::minimapSymbol(bool permanent) const
{
	if(permanent)
		return attr().hasPermanentSymbol_ ? &attr().minimapPermanentSymbol_ : 0;

	switch(attr().minimapSymbolType_){
	case UI_MINIMAP_SYMBOLTYPE_DEFAULT:
		return &player()->race()->minimapMark(UI_MINIMAP_SYMBOL_UNIT);
	case UI_MINIMAP_SYMBOLTYPE_SELF:
		return &attr().minimapSymbol_;
	}

	return 0;
}

void UnitObjective::setPose(const Se3f& poseIn, bool initPose)
{
	__super::setPose(poseIn, initPose);

	fowQuant();
}

void UnitObjective::Kill()
{
	unregisterInPlayerStatistics();
	if(!dead())
		player()->removeObjectiveUnit(this);

	if(player()->fogOfWarMap()){
		player()->fogOfWarMap()->deleteVisible(fow_handle);
		fow_handle = -1;
	}

	__super::Kill();
}

void UnitObjective::changeUnitOwner(Player* playerIn)
{
	if(!dead())
		player()->removeObjectiveUnit(this);

	if(player()->fogOfWarMap()){ 
		player()->fogOfWarMap()->deleteVisible(fow_handle);
		fow_handle = -1;
	}

	__super::changeUnitOwner(playerIn);
}

void UnitObjective::applyParameterArithmeticsImpl(const ArithmeticsData& arithmetics)
{
	__super::applyParameterArithmeticsImpl(arithmetics);
	applyArithmetics(arithmetics);
}

void UnitObjective::applyParameterArithmetics(const ParameterArithmetics& unitArithmetics)
{
	bool applyOnPlayer = false;
	ParameterArithmetics::Data::const_iterator i;
	FOR_EACH(unitArithmetics.data, i){
		const ArithmeticsData& arithmetics = *i;
		if(arithmetics.unitType & ArithmeticsData::TAKEN)
			applyParameterArithmeticsImpl(arithmetics);
		if(arithmetics.unitType & ~ArithmeticsData::TAKEN)
			applyOnPlayer = true;
	}

	if(applyOnPlayer)
		player()->applyParameterArithmetics(&attr(), unitArithmetics);
}

void UnitObjective::registerInPlayerStatistics()
{
	universe()->checkEvent(EventUnitPlayer(Event::CREATE_OBJECT, this, this->player()));
}

void UnitObjective::unregisterInPlayerStatistics(UnitBase* agressor)
{ 
	if(isRegisteredInPlayerStatistics()){
		universe()->checkEvent(EventUnitMyUnitEnemy(Event::KILL_OBJECT, this, agressor));
		setRegisteredInPlayerStatistics(false);
	}
}

void DockingController::showDebug()
{
	if(dock_)
		dock_->modelLogic()->DrawLogic(cameraManager->GetCamera(), tileIndex_);
}
