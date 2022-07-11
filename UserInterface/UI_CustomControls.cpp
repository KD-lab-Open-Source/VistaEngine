#include "StdAfx.h"

#include "UI_CustomControls.h"

#include "CameraManager.h"

#include "Serialization.h"
#include "UI_Render.h"
#include "StreamCommand.h"

#include "Universe.h"
#include "..\Units\UnitObjective.h"
#include "..\Environment\Environment.h"
#include "..\Environment\Anchor.h"
#include "UI_Logic.h"
#include "..\Util\ComboListColor.h"
#include "..\Units\Triggers.h"

Singleton<UI_Minimap> minimap;

// ------------------- UI_MinimapSymbol

UI_MinimapSymbol::UI_MinimapSymbol()
{
	type_ = SYMBOL_RECTANGLE;

	scaleByEvent_ = false;

	useColor_ = false;
	color_ = sColor4f(1.f, 1.f, 1.f, 1.f);

	scale_ = 1.0f;
}

void UI_MinimapSymbol::serialize(Archive& ar)
{
	ar.serialize(type_, "type", "тип");
	ar.serialize(scaleByEvent_, "scaleByEvent", "маштабировать по размеру события/юнита");
	if(!scaleByEvent_)
		ar.serialize(scale_, "selfScale", "собственный масштаб символа");
	ar.serialize(useColor_, "useLegionColor", "красить в цвет легиона");
	if(type_ == SYMBOL_SPRITE)
		ar.serialize(sprite_, "sprite", "изображение");
	else if(!useColor_)
		ar.serialize(color_, "color", "собственный цвет символа");
}

int UI_MinimapSymbol::lifeTime() const
{ 
	return sprite_.isAnimated() ? max(sprite_.texture()->GetTotalTime(), logicTimePeriod) : logicTimePeriod;
}

bool UI_MinimapSymbol::redraw(const Rectf& object_pos, const sColor4f& legionColor, float time) const
{
	sColor4f color(useColor_ ? legionColor : color_);
	if(!useColor_)
		color.a *= legionColor.a;

	switch(type_){
	case SYMBOL_RECTANGLE:
		UI_Render::instance().drawRectangle(object_pos.scaled(Vect2f(scale_, scale_), object_pos.center()), color, false);
		break;
	
	case SYMBOL_SPRITE:
		UI_Render::instance().drawSprite(object_pos.scaled(Vect2f(scale_, scale_), object_pos.center()), sprite_, color, UI_BLEND_NORMAL, sprite_.phase(time, true));
		break;
	}

	return true;
}

// ------------------- UI_MinimapEvent

Vect2f UI_MinimapEvent::eventDefaultSize(100.f, 100.f);
float UI_MinimapEvent::currentFrameAlpha = 1.f;

bool UI_MinimapEvent::operator == (const UI_MinimapEvent& event) const
{
	return (uid_ && uid_ == event.uid_ || pos_.eq(event.pos_, 1.f)) && symbol_ == event.symbol_;
}

void UI_MinimapEvent::update(const UI_MinimapEvent& event)
{
	pos_ = event.pos_;
	color_ = event.color_;
	aliveTime_.start(event.aliveTime_());
}

void UI_MinimapEvent::redraw() const
{
	Vect2f size = size_;
	size /= minimap().worldSize();
	size *= minimap().scaledPosition().size();
		
	Vect2f pos = size;
	pos /= (-2.f);
	pos += minimap().world2minimap(pos_);

	symbol_.redraw(Rectf(pos, size), sColor4f(color_.r, color_.g, color_.b, color_.a * currentFrameAlpha), 0.001f * animationTime_());
}
// ------------------- UI_Minimap

UI_Minimap::UI_Minimap() :
	mapTexture_(0),
	viewZoneColor_(WHITE)
{
	controlPose_ = Rectf(0,0,0,0);
	worldSize_ = Vect2f(0, 0);
	position_  = controlPose_;
	rotation_.set(0.f);

	mapAlpha_ = 1.f;
	drawViewZone_ = true;
	canDrawFogOfWar_ = true;
	showFogOfWar_ = false;
	drawWater_ = true;
	canRotateByCamera_ = false;
	rotateByCamera_ = false;
	drawEvents_ = true;

	viewStartLocations_ = true;
	borderColor_ = sColor4f(1,1,1,0);
	mapAlign_ = UI_ALIGN_CENTER;
}

UI_Minimap::~UI_Minimap()
{
	dassert(!mapTexture_);
}

void fClearOldEventsCommand(void *data)
{
	minimap().clearOldEvents();
}

void UI_Minimap::logicQuant()
{
	uiStreamCommand.set(fClearOldEventsCommand);
}

void UI_Minimap::quant(float dt)
{
}

void UI_Minimap::redraw(float alpha)
{
	start_timer_auto();

	if(!inited() || alpha < FLT_EPS)
		return;

	if(isRotateByCamera())
		setRotate(M_PI/2.f - cameraManager->coordinate().psi());
	else
		scale_.set((float)UI_Render::instance().windowPosition().width(), (float)UI_Render::instance().windowPosition().height());

	if(mapTexture_)
	{
		/*
		(--) (+-)
		(-+) (++)
		v1------v3
		|		|
		|		|
		v2------v4
		*/

		Vect2f c1(-position_.width()/2.f, -position_.height()/2.f);
		Vect2f c2(-position_.width()/2.f, +position_.height()/2.f);
		Vect2f c3(+position_.width()/2.f, -position_.height()/2.f);
		Vect2f c4(+position_.width()/2.f, +position_.height()/2.f);
		
		c1 *= scale_;
		c2 *= scale_;
		c3 *= scale_;
		c4 *= scale_;

		c1 *= rotation_;
		c2 *= rotation_;
		c3 *= rotation_;
		c4 *= rotation_;

		c1 /= scale_;
		c2 /= scale_;
		c3 /= scale_;
		c4 /= scale_;
		
		UI_Render::instance().drawMiniMap(Vect2f(center_.x + c1.x, center_.y + c1.y),
										  Vect2f(center_.x + c2.x, center_.y + c2.y),
										  Vect2f(center_.x + c3.x, center_.y + c3.y),
										  Vect2f(center_.x + c4.x, center_.y + c4.y),
										  mapTexture_, isFogOfWar(), drawWater_, borderColor_,
										  mapAlpha_*alpha, sColor4f(0.f,1.0f,0.f,1.f));
	}

	if(drawEvents_){
		UI_MinimapEvent::currentFrameAlpha = mapAlpha_ * alpha;
		for(MinimapEvents::const_iterator it = events_.begin(); it != events_.end(); ++it)
			it->redraw();
	}
	
	if(viewStartLocations_)
		if(const MissionDescription* mission = UI_LogicDispatcher::instance().currentMission())
		{
			const UI_Font* font = font_ ? font_.get() : UI_Render::instance().defaultFont();
			for(int number = 0; number < mission->playersAmountMax(); ++number)
			{
				if(mission->playerData(number).realPlayerType!=REAL_PLAYER_TYPE_CLOSE){
					sColor4c color = WHITE;
					XBuffer buf;
					
					if(mission->useMapSettings()){
						buf <= number+1;
						color = sColor4c(GlobalAttributes::instance().playerColors[mission->playerData(number).colorIndex]);
					}
					else
						buf < "x";

					Rectf rect(world2minimap(mission->startLocation(mission->playerData(number).shuffleIndex)) - Vect2f(0.5f * font->size() / scale_.x, 0.5f * font->size() / scale_.y), Vect2f(font->size() / scale_.x, font->size() / scale_.y));
					UI_Render::instance().outText(rect, buf, &UI_TextFormat(color), UI_TEXT_ALIGN_CENTER, font, alpha);
				}
			}
		}

	if(drawViewZone_){
		Vect2f sq[4] = {
			screan2minimap(Vect2f(-.5f, -.5f)),
			screan2minimap(Vect2f(-.5f, 0.5f)),
			screan2minimap(Vect2f(0.5f, 0.5f)),
			screan2minimap(Vect2f(0.5f, -.5f))
		};

		sColor4f color(viewZoneColor_.r, viewZoneColor_.g, viewZoneColor_.b, viewZoneColor_.a * alpha);
		drawClippedLine(sq[0], sq[1], color);
		drawClippedLine(sq[1], sq[2], color);
		drawClippedLine(sq[2], sq[3], color);
		drawClippedLine(sq[3], sq[0], color);
	}
}

void UI_Minimap::drawClippedLine(const Vect2f& p0, const Vect2f& p1, const sColor4f& color) const
{
	Vect2f pos0(p0), pos1(p1);
	if(position_.clipLine(pos0, pos1))
		UI_Render::instance().drawLine(xformPoint(pos0), xformPoint(pos1), color);
}

void fAddEventCommand(void *data)
{
	UI_MinimapSymbolID* psymbol_type = (UI_MinimapSymbolID*)data;
	const UnitBase* unit = *(const UnitBase**)(psymbol_type+1);
	float rad = unit->minimapRadius();

	minimap().updateEvent(UI_MinimapEvent(universe()->activeRace()->minimapMark(*psymbol_type), 
		unit->position2D(), Vect2f(rad, rad),
		sColor4f(1.f, 1.f, 1.f, 1.f)));
}

void UI_Minimap::checkEvent(const Event& map_event)
{
	if(!inited())
		return;

	UI_MinimapSymbolID symbol_type = UI_MINIMAP_SYMBOL_DEFAULT;
	const UnitBase* unit = 0;

	switch(map_event.type())
	{
		case Event::ATTACK_OBJECT:
			{
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
		case Event::CREATE_OBJECT:
			symbol_type = UI_MINIMAP_SYMBOL_ADD_UNIT;
			goto label_out;
		case Event::COMPLETE_BUILDING:
			symbol_type = UI_MINIMAP_SYMBOL_BUILD_FINISH;
label_out:
			{
				const EventUnitPlayer& event = safe_cast_ref<const EventUnitPlayer&>(map_event);
				if(event.player() == universe()->activePlayer())
					unit = event.unit();
			}
			break;
		case Event::COMPLETE_UPGRADE:{
			symbol_type = UI_MINIMAP_SYMBOL_UPGRAGE_FINISH;
			const EventUnitUnitAttributePlayer& event = safe_cast_ref<const EventUnitUnitAttributePlayer&>(map_event);
			if(event.player() == universe()->activePlayer())
				unit = event.unit();
			break;
			}
		case Event::UI_MINIMAP_ACTION_CLICK:{
			MTG();
			Vect2f pos = minimap2world(safe_cast_ref<const EventMinimapClick&>(map_event).coord());
			if(pos.x >= 0.f && pos.x <= worldSize().x && pos.y >= 0.f && pos.y <= worldSize().y){
				Vect2f size;
				size.x = size.y = min(worldSize().x, worldSize().y) * 0.01f;
				addEvent(UI_MinimapEvent(universe()->activeRace()->minimapMark(UI_MINIMAP_SYMBOL_ACTION_CLICK), pos, size, universe()->activePlayer()->unitColor()), false);
			}
			return;
											}
		default:
			return;
	}
	
	MTL();

	if(unit && unit->attr().isActing() && symbol_type != UI_MINIMAP_SYMBOL_DEFAULT && (!isFogOfWar() || environment->isVisibleUnderForOfWar(unit)))
		uiStreamCommand.set(fAddEventCommand) << symbol_type << unit;

}

void UI_Minimap::clearEvents()
{
	MTG();
	events_.clear();
}

void UI_Minimap::clearOldEvents()
{
	MTG();
	events_.erase(remove_if(events_.begin(), events_.end(), not1(mem_fun_ref(&UI_MinimapEvent::alive))), events_.end());
}

void UI_Minimap::updateEvent(const UI_MinimapEvent& event, bool bg)
{
	MTG();
	MinimapEvents::iterator me = std::find(events_.begin(), events_.end(), event);
	if(me == events_.end())
		addEvent(event, bg);
	else
		me->update(event);
}

void fAddUnitMarkCommand(void *data)
{
	const UnitObjective* unit = *(const UnitObjective**)(data);

	Vect2f size;
	size.x = size.y = unit->minimapRadius();

	if(const UI_MinimapSymbol* symbol = unit->minimapSymbol(true))
		minimap().updateEvent(UI_MinimapEvent(*symbol, unit->position2D(), size, unit->player()->unitColor(), (int)unit), true);

	if(!minimap().isFogOfWar() || !(unit->hiddenGraphic() & UnitReal::HIDE_BY_FOW))
		if(const UI_MinimapSymbol* symbol = unit->minimapSymbol(false))
			minimap().updateEvent(UI_MinimapEvent(*symbol, unit->position2D(), size, unit->player()->unitColor(), (int)unit));
}

void UI_Minimap::addUnit(const UnitObjective* unit)
{
	MTL();

	if(!inited())
		return;

	uiStreamCommand.set(fAddUnitMarkCommand) << unit;
}

void fAddWorldMarkCommand(void *data)
{
	const Anchor* anchor = *(const Anchor**)data;
	if(const UI_MinimapSymbol* symbol = anchor->symbol())
		minimap().updateEvent(UI_MinimapEvent(*symbol, anchor->position2D(), Vect2f(anchor->radius(), anchor->radius()), sColor4f(1,1,1,1), (int)anchor));
}

void UI_Minimap::addAnchor(const Anchor* anchor)
{
	MTL();

	if(!inited())
		return;

	uiStreamCommand.set(fAddWorldMarkCommand) << anchor;
}

void UI_Minimap::init(const Vect2f& world_size, const char* texture_file_name)
{
	if(mapTexture_ && !_stricmp(mapTexture_->GetName(), texture_file_name) && worldSize_.xi())
		return;

	releaseMapTexture();

	worldSize_ = world_size;
	calcScaled();

	mapTexture_ = UI_Render::instance().createTexture(texture_file_name);
}

void UI_Minimap::relaxLoading()
{
	clearEvents();
	canDrawFogOfWar_ = true;
	drawEvents_ = false;
	rotateByCamera_ = false;
}

void UI_Minimap::setViewParameters(float mapAlpha, bool drawViewZone, bool drawFogOfWar, bool drawWater, bool drawEvents, sColor4f viewZoneColor, bool rotateByCamera, UI_Align mapAlign, sColor4f borderColor)
{
	mapAlpha_ = mapAlpha;
	drawViewZone_ = drawViewZone;
	drawEvents_ = drawEvents;
	canDrawFogOfWar_ = drawFogOfWar;
	drawWater_ = drawWater;
	viewZoneColor_ = viewZoneColor;
	canRotateByCamera_ = rotateByCamera;
	mapAlign_ = mapAlign;
	borderColor_ = borderColor;
}

void UI_Minimap::setViewStartLocationsParameters(bool viewStartLocations, UI_FontReference font)
{
	viewStartLocations_ = viewStartLocations;
	font_ = font;
}

void UI_Minimap::calcScaled()
{
	scale_.set((float)UI_Render::instance().windowPosition().width(), (float)UI_Render::instance().windowPosition().height());
	if(controlPose_.width() * controlPose_.height() > FLT_EPS && worldSize_.x * worldSize_.y > FLT_EPS)
	{
		center_.set(0, 0);

		float minX = worldSize_.x, maxX = -worldSize_.x, minY = worldSize_.y, maxY = -worldSize_.y;

		Rectf bound(0.0f, 0.0f, worldSize_.x, worldSize_.y);
		vector<Vect2f> vbp = bound.to_polygon();

		for(vector<Vect2f>::iterator it = vbp.begin(); it != vbp.end(); ++it){
			*it *= rotation_;
			minX = min(minX, (*it).x);
			maxX = max(maxX, (*it).x);
			minY = min(minY, (*it).y);
			maxY = max(maxY, (*it).y);
		}
		bound.set(0.0f, 0.0f, maxX - minX, maxY - minY);

		float scaleView = (float)UI_Render::instance().windowPosition().height() / UI_Render::instance().windowPosition().width();
		float scaleControl = controlPose_.height() / controlPose_.width() * scaleView;
		float scaleMinimap = bound.height() / bound.width();

		float mainScaleX = 1.f;
		float mainScaleY = scaleView;

		if(scaleMinimap > scaleControl)
		{	// вытянут вверх, пустые места по бокам
			mainScaleY = bound.height() / controlPose_.height();
			mainScaleX = mainScaleY / scaleView;
			bound.height(controlPose_.height());
			bound.width(bound.height() * scaleView / scaleMinimap);
		}
		else{
			// вытянут горизонтально, пустые места сверху и снизу
			mainScaleX = bound.width() / controlPose_.width();
			mainScaleY = mainScaleX * scaleView;
			bound.width(controlPose_.width());
			bound.height(bound.width() * scaleMinimap / scaleView);
		}
		bound.center(controlPose_.center());

		center_ = bound.center();

		switch(mapAlign_)
		{
		case UI_ALIGN_BOTTOM_RIGHT:
			center_.x += (controlPose_.width()-bound.width()) / 2.0f;
			center_.y += (controlPose_.height()-bound.height()) / 2.0f;
			break;
		case UI_ALIGN_BOTTOM_LEFT:
			center_.x -= (controlPose_.width()-bound.width()) / 2.0f;
			center_.y += (controlPose_.height()-bound.height()) / 2.0f;
			break;
		case UI_ALIGN_TOP_RIGHT:
			center_.x += (controlPose_.width()-bound.width()) / 2.0f;
			center_.y -= (controlPose_.height()-bound.height()) / 2.0f;
			break;
		case UI_ALIGN_TOP_LEFT:
			center_.x -= (controlPose_.width()-bound.width()) / 2.0f;
			center_.y -= (controlPose_.height()-bound.height()) / 2.0f;
			break;
		case UI_ALIGN_RIGHT:
			center_.x += (controlPose_.width()-bound.width()) / 2.0f;
			break;
		case UI_ALIGN_LEFT:
			center_.x -= (controlPose_.width()-bound.width()) / 2.0f;
			break;
		case UI_ALIGN_BOTTOM:
			center_.y += (controlPose_.height()-bound.height()) / 2.0f;
			break;
		case UI_ALIGN_TOP:
			center_.y -= (controlPose_.height()-bound.height()) / 2.0f;
			break;
		}

		position_.width(worldSize_.x / mainScaleX);
		position_.height(worldSize_.y / mainScaleY);
		
		position_.center(center_);
	}
	else {
		position_ = controlPose_;
		center_ = position_.center();
	}
}

void UI_Minimap::setPosition(const Rectf& pos){
	if(!(controlPose_ == pos)){
		controlPose_ = pos;
		calcScaled();
	}
}

void UI_Minimap::setRotate(float angle)
{
	rotation_.set(angle);
	calcScaled();
}

void UI_Minimap::releaseMapTexture()
{
	UI_Render::instance().releaseTexture(mapTexture_);
}

void UI_Minimap::pressEvent(const Vect2f& mouse_pos)
{
	if(inited())
		cameraManager->setCoordinate(
			CameraCoordinate(minimap2world(mouse_pos), cameraManager->coordinate().psi(), cameraManager->coordinate().theta(),  cameraManager->coordinate().distance())
			);
}

Vect2f UI_Minimap::minimap2world(const Vect2f& mouse) const{
	if(inited()){
		Vect2f tmp = mouse;
		
		invXformPoint(tmp);
		
		tmp -= position_.left_top();
		
		tmp /= position_.size();
		tmp *= worldSize_;
		return 	tmp;
	}
	return Vect2f(0.f, 0.f);
}

Vect2f UI_Minimap::world2minimap(const Vect2f& world) const{
	if(inited()){
		
		Vect2f tmp = world;
		tmp /= worldSize_;
		tmp *= position_.size();
		tmp += position_.left_top();

		return xformPoint(tmp);
	}
	return Vect2f(0.f, 0.f);
}

Vect2f UI_Minimap::screan2minimap(const Vect2f& scr) const{
	if(inited()){

		Vect3f pos, dir;
		cameraManager->GetCamera()->GetWorldRay(scr, pos, dir);

		dir *= pos.z / (dir.z >= 0 ? 0.0001f : -dir.z);
		Vect2f world(pos += dir);

		world /= worldSize_;
		world *= position_.size();
		world += position_.left_top();

		return  world;
	}
	return Vect2f(0.f, 0.f);
}

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_MinimapSymbol, SymbolType, "UI_MinimapSymbol::Type")
REGISTER_ENUM_ENCLOSED(UI_MinimapSymbol, SYMBOL_RECTANGLE, "прямоугольник")
REGISTER_ENUM_ENCLOSED(UI_MinimapSymbol, SYMBOL_SPRITE, "изображение")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_MinimapSymbol, SymbolType)
