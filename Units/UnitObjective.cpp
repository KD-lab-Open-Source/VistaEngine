#include "StdAfx.h"
#include "UnitObjective.h"
#include "UnitPad.h"
#include "Universe.h"
#include "Triggers.h"
#include "IronBuilding.h"
#include "RenderObjects.h"
#include "CameraManager.h"
#include "UserInterface\UserInterface.h"
#include "UserInterface\UI_Render.h"
#include "UserInterface\UI_Logic.h"
#include "UserInterface\UI_Minimap.h"
#include "Physics\crash\CrashSystem.h"
#include "Render\Src\cCamera.h"
#include "Render\src\Scene.h"

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

	universe()->addVisibleUnit(this);

	if(!hiddenLogic() && (!attr().isActing() || player() == universe()->activePlayer() || !safe_cast<const UnitActing*>(this)->isInvisible()))
		minimap().addUnit(this);

	// обработка изменения параметров
	xassert(showChangeControllers_.size() == attr().parameterShowSettings.size());
	xassert(parameterShowTimers_.size() == attr().parameterShowSettings.size());

	if(isConstructed() && !isUpgrading())
	for(int ind = 0; ind < attr().parameterShowSettings.size(); ++ind){
		const ParameterShowSetting& cur = attr().parameterShowSettings[ind];
		float currentValue = parameters_.findByIndex(cur.paramTypeRef.key());

		float diff = showChangeControllers_[ind].value() - currentValue;
		showChangeControllers_[ind].update(currentValue);

		if(!cur.delayShowTime)
			continue;

		switch (cur.showEvent)
		{
		case SHOW_AT_PARAMETER_CHANGE:
			if(abs(diff) > FLT_EPS)
				break;
			continue;
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
}

bool UnitObjective::corpseQuant()
{
	if(!__super::corpseQuant())
		return false;

	if(deathAttr().explodeReference->animatedDeath){
		if(deathAttr().explodeReference->alphaDisappear){
			float opacityNew(0.0002f * (chainDelayTimer.timeRest()));
			if(opacityNew < 1.0f)
				setOpacity(opacityNew);
		}
		return false;
	}

	stopEffect(&deathAttr().effectAttrFly);
	if(deathAttr().explodeReference->enableExplode)
		universe()->crashSystem->addCrashModel(deathAttr(), model(), position(), lastContactPoint_, lastContactWeight_, 
		GlobalAttributes::instance().debrisLyingTime, UnitBase::rigidBody()->velocity());
	return true;
}

void UnitObjective::fowQuant()
{
	if(!alive())
		return;

	if(player()->fogOfWarMap()){
		if(!(hiddenGraphic() & (HIDE_BY_TELEPORT | HIDE_BY_TRANSPORT | HIDE_BY_TRIGGER))){
			if(fow_handle == -1)
				fow_handle = player()->fogOfWarMap()->createVisible();
			player()->fogOfWarMap()->moveVisibleOuterRadius(fow_handle, pose().trans().x, pose().trans().y, sightRadius() * attr().sightFogOfWarFactor);
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
		return isUpgrading();
	case SHOW_AT_BUILD:
		return !isConstructed();
	case SHOW_AT_PRODUCTION:
		if(attr().isActing())
			return safe_cast<const UnitActing*>(this)->isProducing();
		break;
	case SHOW_AT_DIRECT_CONTROL:
		if(attr().isActing()) // FIXME - rigidBody() обещали переписать
			return safe_cast<const UnitActing*>(this)->directControl();
		break;
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
		if(attr().isActing())
			phase = safe_cast<const UnitActing*>(this)->currentProductionProgress();
		else {
			phase = 0.f;
			return false;
		}
		break;

	case ParameterShowSetting::UPGRADE_PROGRESS:
		if(attr().isActing())
			phase = safe_cast<const UnitActing*>(this)->upgradeProgres(par.dataType, false);
		else {
			phase = 0.f;
			return false;
		}
		break;

	case ParameterShowSetting::FINISH_UPGRADE:
		if(attr().isActing())
			phase = safe_cast<const UnitActing*>(this)->upgradeProgres(par.dataType, true);
		else {
			phase = 0.f;
			return false;
		}
		break;

	case ParameterShowSetting::GROUND_PROGRESS:
		phase = player()->currentCapacity();
		break;

	case ParameterShowSetting::RELOAD_PROGRESS:
		if(attr().isActing())
			if(par.weaponPrmRef)
				if(safe_cast<const UnitActing*>(this)->hasWeapon(par.weaponPrmRef->ID()))
					phase = safe_cast<const UnitActing*>(this)->UnitActing::weaponChargeLevel(par.weaponPrmRef->ID());
				else {
					phase = 0.f;
					return false;
				}
			else
				phase = safe_cast<const UnitActing*>(this)->UnitActing::weaponChargeLevel(0);
		else {
			phase = 0.f;
			return false;
		}
		break;

	default:
		return false;
	}

	if(par.notShowEmpty && phase < FLT_EPS)
		return false;

	return true;
}

bool UnitObjective::unvisible() const
{
	xassert(model());
	return model()->unvisible();
}

class DrawStrip2D
{
	sVertexXYZWD* pb_;
	Color4c	clr_;
	int num_;

public:
	DrawStrip2D() : pb_(0)
	{
		clr_ = Color4c::WHITE;
		num_ = 0;
	}

	void begin(const Color4c& d)
	{
		clr_ = d;
		cVertexBuffer<sVertexXYZWD>* buf = gb_RenderDevice->GetBufferXYZWD();
		pb_ = buf->Lock(24);
		xassert(num_ == 0);
	}

	void end()
	{
		cVertexBuffer<sVertexXYZWD>* buf = gb_RenderDevice->GetBufferXYZWD();
		buf->Unlock(num_);
		if(num_ >= 4){
			gb_RenderDevice->SetNoMaterial(ALPHA_BLEND, MatXf::ID);
			buf->DrawPrimitive(PT_TRIANGLESTRIP, num_ - 2);
		}
		num_ = 0;
	}

	void set(int x0, int y0, int x1, int y1)
	{
		cVertexBuffer<sVertexXYZWD>* buf = gb_RenderDevice->GetBufferXYZWD();

		sVertexXYZWD* pb = pb_ + num_;
		pb->x=x0; pb->y=y0; pb->z=0.001f; pb->w=0.001f; pb->diffuse=clr_;
		++pb;
		pb->x=x1; pb->y=y1; pb->z=0.001f; pb->w=0.001f; pb->diffuse=clr_;
		
		num_ += 2;
		if(num_ + 4 > buf->GetSize()){
			gb_RenderDevice->SetNoMaterial(ALPHA_BLEND, MatXf::ID);
			buf->Unlock(num_);
			buf->DrawPrimitive(PT_TRIANGLESTRIP, num_ - 2);

			pb_ = buf->Lock(24);
			pb_[0].x=x0; pb_[0].y=y0; pb_[0].z=0.001f; pb_[0].w=0.001f; pb_[0].diffuse=clr_;
			pb_[1].x=x1; pb_[1].y=y1; pb_[1].z=0.001f; pb_[1].w=0.001f; pb_[1].diffuse=clr_;

			num_ = 2;
		}
	}

	void set(int x0, int y0, float r1, float r2, float angle)
	{
		float s = sinf(angle);
		float c = cosf(angle);
		set(x0 + r1 * c, y0 + r1 * s, x0 + r2 * c, y0 + r2 * s);
	}
};

void UnitObjective::graphQuant(float dt)
{
	__super::graphQuant(dt);

	if(!alive())
		return;

	const UI_Render& rd = UI_Render::instance();
	if(rd.windowPosition().width() <= 0 || rd.windowPosition().height() <= 0)
		return;

	DrawStrip2D strip;

	Vect3f pos, pv, e;
	bool unvis = (hiddenGraphic() || unvisible()), unscreen = false;

	if(!unvis){
		pos = interpolatedPose().trans();
		cameraManager->GetCamera()->ConvertorWorldToViewPort(&pos, &pv, &e);

		Rectf view(0, rd.windowPosition().top(), rd.renderSize().x, rd.windowPosition().height());
		if(unscreen = (pv.z < 0.1f || !view.point_inside((Vect2f&)e)))
			unvis = true;

		if(usedByTrigger() && player()->active())
			player()->race()->workForAISprite().draw(pos, attr().initialHeightUIParam, player()->unitColor());
	}
	
	if(unscreen && attr().useOffscreenSprites)
		UI_LogicDispatcher::instance().addUnitOffscreenSprite(this);

	for(int ind = 0; ind < attr().parameterShowSettings.size(); ++ind)
	{
		showChangeControllers_[ind].quant();

		if(unvis)
			continue;
		
		const ParameterShowSetting& cur = attr().parameterShowSettings[ind];

		bool need_draw = parameterShowTimers_[ind].busy() || checkShowEvent(cur.showEvent);

		if(!need_draw)
			switch(cur.showEvent){
			case SHOW_AT_SELECT:
			case SHOW_AT_HOVER:
			case SHOW_AT_HOVER_OR_SELECT:
				need_draw = UI_LogicDispatcher::instance().showAllUnitParametersMode();
			}

		if(!need_draw)
			continue;

		float phase;
		if(getParameterValue(cur, phase)){
			Color4f phase_clr;
			phase_clr.interpolate(Color4f(cur.minColor), Color4f(cur.maxColor), phase);
			
			float radiusFactor = min(cameraManager->GetCamera()->GetFocusViewPort().x / pv.z, 20.f);
			radiusFactor = round(radiusFactor * 50.f) / 50.f;
			int x = round(e.x + radiusFactor * cur.paramOffset.x);
			int y = round(e.y + clamp(radiusFactor, 3.f, 5.f) * cur.paramOffset.y);

			if(cur.shape == ParameterShowSetting::CIRCLE){
				strip.begin(Color4c(phase_clr));

				float r1 = radiusFactor * cur.innerRadius;
				float r2 = radiusFactor * cur.radius;
				
				float angle = cur.startAngle * M_PI / 180.f;
				float end = cur.endAngle * M_PI / 180.f;
				
				float step = M_PI * clamp(4.f / r2, 0.066666667f, 0.16666667f);

				if(cur.direction){
					if(angle >= end)
						angle -= 2.f * M_PI;
					end = angle + phase * (end - angle);
					while(angle < end){
						strip.set(x, y, r1, r2, angle);
						angle += step;
						if(angle >= end)
							strip.set(x, y, r1, r2, end);
					}
				}
				else {
					if(angle <= end)
						angle += 2.f * M_PI;
					end = angle - phase * (angle - end);
					while(angle > end){
						strip.set(x, y, r1, r2, angle);
						angle -= step;
						if(angle <= end)
							strip.set(x, y, r1, r2, end);
					}
				}

				strip.end();
			}
			else {
				int len = round(cur.radius * radiusFactor);
				if(ParameterShowSetting::ALIGN_CENTER == cur.alignType)
					x -= len / 2;
				else
					x -= len;

				rd.drawRectangle( // Подложка
					rd.relativeCoords(Recti(x, y, len, 2)), cur.backgroundColor);
				rd.drawRectangle( // Значение
					rd.relativeCoords(Recti(x, y, round(phase * len), 2)), phase_clr);
				rd.drawRectangle( // Рамка
					rd.relativeCoords(Recti(x - 1, y - 1, len + 1, 3)), cur.borderColor, true);
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
		player()->removeUnitAccount(this);

	if(player()->fogOfWarMap()){
		player()->fogOfWarMap()->deleteVisible(fow_handle);
		fow_handle = -1;
	}

	__super::Kill();
}

void UnitObjective::changeUnitOwner(Player* playerIn)
{
	if(!dead())
		player()->removeUnitAccount(this);

	if(player()->fogOfWarMap()){ 
		player()->fogOfWarMap()->deleteVisible(fow_handle);
		fow_handle = -1;
	}

	__super::changeUnitOwner(playerIn);
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
