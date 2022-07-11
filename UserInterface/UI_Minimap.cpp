#include "StdAfx.h"
#include "UI_Minimap.h"
#include "UI_Render.h"
#include "UI_Logic.h"
#include "StreamCommand.h"
#include "CameraManager.h"
#include "Render\src\cCamera.h"
#include "Game\Player.h"
#include "Game\Universe.h"
#include "Units\UnitObjective.h"
#include "Units\IronBuilding.h"
#include "Units\Squad.h"
#include "Environment\Anchor.h"
#include "WBuffer.h"

void fMinimapLogic2GraphQuantCommand(void *data)
{
	minimap().logic2GraphQuant();
}

void UI_Minimap::logicQuant()
{
	{
		MTAuto lock(lock_);
		unitList_.swap(unitListLogic_);
		unitListLogic_.clear();
	}

	uiStreamCommand.set(fMinimapLogic2GraphQuantCommand);
}

void UI_Minimap::logic2GraphQuant()
{
	MTG();
	for(int idx = 0; idx < (int)EVENTS_SIZE; ++idx)
		events_[idx].erase(remove_if(events_[idx].begin(), events_[idx].end(), not1(mem_fun_ref(&UI_MinimapEvent::alive))), events_[idx].end());
	worldPoints_.erase(remove_if(worldPoints_.begin(), worldPoints_.end(), not1(mem_fun_ref(&WorldPoint::alive))), worldPoints_.end());

	UnitList unitList;
	{
		MTAuto lock(lock_);
		unitList.swap(unitList_);
	}

	UnitList::const_iterator it;
	FOR_EACH(unitList, it){
		if((*it)->attr().isObjective()){
			const UnitObjective* unit = safe_cast<const UnitObjective*>(*it);

			if(const UI_MinimapSymbol* symbol = unit->minimapSymbol(true))
				updateEvent(UI_MinimapEvent(*symbol, unit->position2D(), unit->minimapRadius(), unit->player()->unitColor(), unit->angleZ(), (int)unit), BACKGROUND);

			if(!isFogOfWar() || !(unit->hiddenGraphic() & UnitReal::HIDE_BY_FOW))
				if(const UI_MinimapSymbol* symbol = unit->minimapSymbol(false))
					updateEvent(UI_MinimapEvent(*symbol, unit->position2D(), unit->minimapRadius(), unit->player()->unitColor(), unit->angleZ(), (int)unit), UNITS);
		}
		else if((*it)->attr().isSquad()){
			const UnitSquad* squad = safe_cast<const UnitSquad*>(*it);

			if(const UI_MinimapSymbol* symbol = squad->minimapSymbol())
				updateEvent(UI_MinimapEvent(*symbol, squad->position2D(), 1.f, squad->player()->unitColor(), squad->angleZ(), (int)squad), UNITS);
		}
		else {
			xxassert(false, "Некорректный юнит на миникарте");
		}
	}

}

void UI_Minimap::quant(float dt)
{
	dragGuard_ = dragGuard_ && UI_LogicDispatcher::instance().isMouseFlagSet(MK_LBUTTON | MK_RBUTTON);
	
	if(isRotateByCamera())
		setRotate(M_PI_2 - cameraManager->coordinate().psi());
	else
		scale_ = UI_Render::instance().windowPosition().size();

	if(showWind_ && windMap){
		const UI_MinimapSymbol& symbol = universe()->activeRace()->windMark(windMap->getWindType());
		Vect2f dir(windMap->getDirection());
		float windForce = dir.norm();
		if(windForce > FLT_EPS){
			float angle = windMap->getWindType() == WindMap::DIRECTION_THROUGH_MAP ? acosf(dir.x / windForce) : 0.f;
			updateEvent(UI_MinimapEvent(symbol, worldSize_ / 2.f, min(worldSize().x, worldSize().y) * 0.01f * windForce, universe()->activePlayer()->unitColor(), M_PI_2 + angle, (int)&symbol), EVENTS);
		}
	}

	if(centerToSelect_)
		if(const UnitInterface* ui = UI_LogicDispatcher::instance().selectedUnit())
			setCenter(ui->getUnitReal()->position2D());

	renderPlazeZones();
}

void fAddEventCommand(void *data)
{
	UI_MinimapSymbolID* psymbol_type = (UI_MinimapSymbolID*)data;
	const UnitBase* unit = *(const UnitBase**)(psymbol_type+1);

	minimap().updateEvent(UI_MinimapEvent(universe()->activeRace()->minimapMark(*psymbol_type), 
		unit->position2D(), unit->minimapRadius(),
		Color4f(1.f, 1.f, 1.f, 1.f)), UI_Minimap::EVENTS);
}

void UI_Minimap::checkEvent(const Event& map_event)
{
	if(!inited())
		return;

	UI_MinimapSymbolID symbol_type = UI_MINIMAP_SYMBOL_DEFAULT;
	const UnitBase* unit = 0;

	switch(map_event.type())
	{
		case Event::ATTACK_OBJECT: {
			const EventUnitMyUnitEnemy& event = safe_cast_ref<const EventUnitMyUnitEnemy&>(map_event);
			unit = event.unitMy();
			if(!event.unitEnemy())
				break;
			else if(event.unitEnemy()->player() == unit->player())
				break;
			if(unit->player() == universe()->activePlayer())
				symbol_type = UI_MINIMAP_SYMBOL_UDER_ATTACK;
			else if(unit->player()->clan() == universe()->activePlayer()->clan())
				symbol_type = UI_MINIMAP_SYMBOL_CLAN_UDER_ATTACK;
			break;
								   }
		case Event::CREATE_OBJECT: {
			symbol_type = UI_MINIMAP_SYMBOL_ADD_UNIT;
			const EventUnitPlayer& event = safe_cast_ref<const EventUnitPlayer&>(map_event);
			if(event.player() == universe()->activePlayer())
				unit = event.unit();
								   }
		case Event::COMPLETE_BUILDING: {
			symbol_type = UI_MINIMAP_SYMBOL_BUILD_FINISH;
			const EventUnitPlayer& event = safe_cast_ref<const EventUnitPlayer&>(map_event);
			if(event.player() == universe()->activePlayer())
				unit = event.unit();
									   }
			break;
		case Event::COMPLETE_UPGRADE: {
			symbol_type = UI_MINIMAP_SYMBOL_UPGRAGE_FINISH;
			const EventUnitUnitAttributePlayer& event = safe_cast_ref<const EventUnitUnitAttributePlayer&>(map_event);
			if(event.player() == universe()->activePlayer() && event.unit()->attr().showUpgradeEvent_)
				unit = event.unit();
			break;
									  }
		case Event::UI_MINIMAP_ACTION_CLICK:{
			MTG();
			Vect2f pos = minimap2world(safe_cast_ref<const EventMinimapClick&>(map_event).coord());
			if(pos.x >= 0.f && pos.x <= worldSize().x && pos.y >= 0.f && pos.y <= worldSize().y){
				dragGuard_ = true;
				addEvent(UI_MinimapEvent(universe()->activeRace()->minimapMark(UI_MINIMAP_SYMBOL_ACTION_CLICK), pos, min(worldSize().x, worldSize().y) * 0.01f, universe()->activePlayer()->unitColor()), EVENTS);
			}
			return;
											}
		default:
			return;
	}
	
	MTL();

	if(unit && unit->attr().isActing() && symbol_type != UI_MINIMAP_SYMBOL_DEFAULT && (!isFogOfWar() || unit->isVisibleUnderForOfWar()))
		uiStreamCommand.set(fAddEventCommand) << symbol_type << unit;

}

void UI_Minimap::addSquad(const UnitSquad* squad)
{
	MTL();

	if(!inited())
		return;

	if(isFogOfWar()){
		LegionariesLinks::const_iterator it;
		FOR_EACH(squad->units(), it)
			if((*it)->hiddenGraphic() & UnitReal::HIDE_BY_FOW == 0)
				break;
		if(it == squad->units().end())
			return;
	}

	unitListLogic_.push_back(squad);
}

void UI_Minimap::addUnit(const UnitObjective* unit)
{
	MTL();

	if(!inited())
		return;

	unitListLogic_.push_back(unit);
}

void fAddWorldMarkCommand(void *data)
{
	const Anchor* anchor = *(const Anchor**)data;
	if(const UI_MinimapSymbol* symbol = anchor->symbol())
		minimap().updateEvent(UI_MinimapEvent(*symbol, anchor->position2D(), anchor->radius(), Color4f(1,1,1,1), 0, (int)anchor), UI_Minimap::BACKGROUND);
}

void UI_Minimap::addAnchor(const Anchor* anchor)
{
	MTL();

	if(!inited())
		return;

	if(anchor->symbol())
		uiStreamCommand.set(fAddWorldMarkCommand) << anchor;
}

void UI_Minimap::cameraToEvent()
{
	MTG();
	if(!worldPoints_.empty()){
		cameraManager->setCoordinate(
			CameraCoordinate(worldPoints_.back().pos, cameraManager->coordinate().psi(), cameraManager->coordinate().theta(), cameraManager->coordinate().fi(), cameraManager->coordinate().distance(), cameraManager->coordinate().focus())
			);
		worldPoints_.pop_back();
	}
}

void UI_Minimap::pressEvent(const Vect2f& mouse_pos)
{
	if(inited() && !dragGuard_)
		cameraManager->setCoordinate(
			CameraCoordinate(minimap2world(mouse_pos), cameraManager->coordinate().psi(), cameraManager->coordinate().theta(), cameraManager->coordinate().fi(), cameraManager->coordinate().distance(), cameraManager->coordinate().focus())
			);
}

Vect2f UI_Minimap::screen2minimap(const Vect2f& scr) const{
	if(inited()){

		Vect3f pos, dir;
		cameraManager->GetCamera()->GetWorldRay(scr, pos, dir);

		dir *= pos.z / (dir.z >= 0 ? 0.0001f : -dir.z);
		Vect2f world(pos += dir);

		world /= worldSize_;
		world *= position_.size();
		world += position_.left_top();

		return world;
	}
	return Vect2f(0.f, 0.f);
}

Vect2f UI_Minimap::screen2minimapTexture(const Vect2f& scr) const {
	if(inited()){
		Vect3f pos, dir;
		cameraManager->GetCamera()->GetWorldRay(scr, pos, dir);

		dir *= pos.z / (dir.z >= 0 ? 0.0001f : -dir.z);
		Vect2f world(pos += dir);

		world /= worldSize_;
		world -= Vect2f(.5f, .5f);
		world *= rotation_;
		world += Vect2f(.5f, .5f);

		return world;
	}

	return Vect2f(0.f, 0.f);
}

void UI_Minimap::drawStartLocations(float alpha)
{
	if(const MissionDescription* mission = UI_LogicDispatcher::instance().currentMission()){
		const UI_Font* font = font_ ? font_.get() : UI_Render::instance().defaultFont();
		for(int number = 0; number < mission->playersAmountMax(); ++number)
		{
			if(mission->playerData(number).realPlayerType!=REAL_PLAYER_TYPE_CLOSE){
				Color4c color = Color4c::WHITE;
				WBuffer buf;

				if(mission->useMapSettings()){
					buf <= number+1;
					color = Color4c(GlobalAttributes::instance().playerColors[mission->playerData(number).colorIndex]);
				}
				else
					buf < L"x";

				Rectf rect(world2minimap(mission->startLocation(mission->playerData(number).shuffleIndex)) - Vect2f(0.5f * font->size() / scale_.x, 0.5f * font->size() / scale_.y), Vect2f(font->size() / scale_.x, font->size() / scale_.y));
				UI_Render::instance().outText(rect, buf, &UI_TextFormat(color), UI_TEXT_ALIGN_CENTER, font, alpha);
			}
		}
	}
}


void fDrawZoneCommand(void* data)
{
	minimap().addZone(*(UI_Minimap::PlaceZone*)data);
}

void UI_Minimap::drawZone(const UnitBuilding* building)
{
	//if(isFogOfWar() || !isInstallZones())
	//	return;

	PlaceZone zone(PlaceZone::DRAW, Vect2i(building->position2D()), round(building->attr().producedPlacementZoneRadius), Color4c(building->player()->unitColor()));
	if(MT_IS_GRAPH())
		addZone(zone);
	else
		uiStreamCommand.set(fDrawZoneCommand) << zone;
}

class BuildingZoneScanOperator
{
public:
	typedef vector<const UnitBuilding*> Buildings;

	BuildingZoneScanOperator(const UnitBuilding* owner){
		owner_ = owner;
		radius_ = owner->attr().producedPlacementZoneRadius;
	}

	void operator()(UnitBase* p){
		if(p->alive() && p->attr().isBuilding() && p != owner_ 
			&& p->attr().producedPlacementZoneRadius > 0
			&& p->position2D().distance2(owner_->position2D()) < sqr(radius_ + p->attr().producedPlacementZoneRadius))
			units.push_back(safe_cast<const UnitBuilding*>(p));
	}

	const Buildings& buildings() const { return units; }


private:
	const UnitBuilding* owner_;
	float radius_;
	Buildings units;
};

void UI_Minimap::eraseZone(const UnitBuilding* building)
{
	//if(isFogOfWar() || !isInstallZones())
	//	return;

	PlaceZone zone(PlaceZone::ERASE, building->basementInstallPosition(), round(building->attr().producedPlacementZoneRadius * 1.2f), Color4c(building->player()->unitColor()));
	if(MT_IS_GRAPH())
		addZone(zone);
	else
		uiStreamCommand.set(fDrawZoneCommand) << zone;

	BuildingZoneScanOperator scanAround(building);
	universe()->unitGrid.Scan(zone.pos.xi(), zone.pos.yi(), round(AttributeBase::producedPlacementZoneRadiusMax() * 2.f), scanAround);

	BuildingZoneScanOperator::Buildings::const_iterator it;
	FOR_EACH(scanAround.buildings(), it){
		UI_Minimap::PlaceZone zone(PlaceZone::REDRAW, Vect2i((*it)->position2D()), round((*it)->attr().producedPlacementZoneRadius), Color4c((*it)->player()->unitColor()));
		if(MT_IS_GRAPH())
			addZone(zone);
		else
			uiStreamCommand.set(fDrawZoneCommand) << zone;
	}
}

BEGIN_ENUM_DESCRIPTOR(MinimapAlign, "MinimapAlign")
REGISTER_ENUM(UI_ALIGN_CENTER, "По центру")
REGISTER_ENUM(UI_ALIGN_LEFT, "По левому краю")
REGISTER_ENUM(UI_ALIGN_RIGHT, "По правому краю")
REGISTER_ENUM(UI_ALIGN_TOP, "По верхней границе")
REGISTER_ENUM(UI_ALIGN_BOTTOM, "По нижней границе")
REGISTER_ENUM(UI_ALIGN_TOP_LEFT, "Левый верхний угол")
REGISTER_ENUM(UI_ALIGN_TOP_RIGHT, "Правый верхний угол")
REGISTER_ENUM(UI_ALIGN_BOTTOM_LEFT, "Левый нижний угол")
REGISTER_ENUM(UI_ALIGN_BOTTOM_RIGHT, "Правый нижний угол")
END_ENUM_DESCRIPTOR(MinimapAlign)
