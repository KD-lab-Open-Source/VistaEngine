#include "StdAfx.h"
#include "UI_Minimap.h"
#include "UI_RenderBase.h"
#include "Render\D3d\D3DRender.h"
#include "Environment\Environment.h"
#include "Water\Water.h"
#include "Render\src\FogOfWar.h"

Singleton<UI_Minimap> minimap;

// --------------------------------------------------------------------------

UI_MinimapEvent::UI_MinimapEvent(const UI_MinimapEventStatic& symbol, const Vect2f& object_pos, float radius, const Color4f& color, float anglez, int id)
: uid_(id)
{
	symbol_ = symbol;
	pos_ = object_pos;
	size_ = symbol_.scaleByEvent ? Vect2f(radius, radius) : Vect2f(100.f * symbol_.scale, 100.f * symbol_.scale);
	rotation_ = anglez;
	color_ = color;
	aliveTime_.start(symbol_.lifeTime() + 1);
	animationTime_.start();
}

bool UI_MinimapEvent::operator == (const UI_MinimapEvent& event) const
{
	return uid_ ? uid_ == event.uid_ : (pos_.eq(event.pos_, 1.f) && symbol_ == event.symbol_);
}

void UI_MinimapEvent::update(const UI_MinimapEvent& event)
{
	symbol_ = event.symbol_;
	pos_ = event.pos_;
	size_ = event.size_;
	rotation_ = event.rotation_;
	color_ = event.color_;
	aliveTime_.start(event.aliveTime_.timeRest());
}

// --------------------------------------------------------------------------

UI_Minimap::UI_Minimap()
{
	worldSize_ = Vect2f(0, 0);

	controlPose_ = Rectf(0,0,0,0);
	position_  = controlPose_;
	rotation_.set(0.f);

	drawViewZone_ = true;
	canDrawFogOfWar_ = true;
	showFogOfWar_ = false;
	showPlaceZones_ = false;
	drawWater_ = true;
	canRotateByCamera_ = false;
	rotateByCamera_ = false;
	rotationScale_ = true;
	centerToSelect_ = false;
	useUserScale_ = false;
	useUserDrag_ = false;
	drawEvents_ = false;
	showWind_ = false;
	
	dragGuard_ = false;

	mapTexture_ = 0;
	mask_ = 0;
	placeZones_ = 0;

	viewStartLocations_ = true;
	viewZoneColor_ = Color4f::WHITE;
	borderColor_ = Color4f::WHITE;
	waterColor_ = Color4f::BLUE;
	mapAlign_ = UI_ALIGN_CENTER;
	rotationAngle_ = 0.f;

	scale_.set(1024.f, 768.f);
	userScale_ = 1.f;
	userShift_.set(0, 0);
}

UI_Minimap::~UI_Minimap()
{
	dassert(!mapTexture_);
	dassert(!placeZones_);
}

void UI_Minimap::clearEvents()
{
	MTL();
	MTG();

	for(int idx = 0; idx < EVENTS_SIZE; ++idx){
		events_[idx].clear();
	}
	worldPoints_.clear();

	rectangles_.clear();

	unitList_.clear();
	unitListLogic_.clear();
}

void UI_Minimap::updateEvent(const UI_MinimapEvent& event, EventType type)
{
	MTG();

	MinimapEvents& events = events_[type];
	MinimapEvents::iterator me = std::find(events.begin(), events.end(), event);
	if(me == events.end()){
		float time = event.validTime();
		if(time > 0.f){
			WorldPoint point(event.position());
			point.aliveTimer.start(round(time * 1000.f));
			worldPoints_.push_back(point);
		}
		addEvent(event, type);
	}
	else
		me->update(event);
}

void UI_Minimap::releaseMapTexture()
{
	UI_RenderBase::instance().releaseTexture(mapTexture_);
	worldSize_ = Vect2f(0.f, 0.f);
	if(placeZones_)
		placeZones_->Release();
	placeZones_ = 0;
}

void UI_Minimap::init(const Vect2f& world_size, const char* texture_file_name)
{
	if(mapTexture_ && !_stricmp(mapTexture_->name(), texture_file_name) && worldSize_.xi())
		return;

	releaseMapTexture();

	worldSize_ = world_size;
	
	reposition();

	mapTexture_ = UI_RenderBase::instance().createTexture(texture_file_name);
}

void UI_Minimap::relaxLoading()
{
	clearEvents();

	rotation_.set(0.f);

	canDrawFogOfWar_ = true;
	drawEvents_ = false;
	rotateByCamera_ = false;
	
	userScale_ = 1.f;
	userShift_.set(0, 0);
}

void UI_Minimap::reposition()
{
	scale_ = UI_RenderBase::instance().windowPosition().size();
	if(controlPose_.width() * controlPose_.height() > FLT_EPS && worldSize_.x * worldSize_.y > FLT_EPS)
	{
		center_.set(0, 0);

		float minX = worldSize_.x, maxX = -worldSize_.x, minY = worldSize_.y, maxY = -worldSize_.y;

		Rectf bound(0.0f, 0.0f, worldSize_.x, worldSize_.y);
		vector<Vect2f> vbp = bound.to_polygon();

		for(vector<Vect2f>::iterator it = vbp.begin(); it != vbp.end(); ++it){
			if(rotationScale_)
				*it *= rotation_;
			minX = min(minX, (*it).x);
			maxX = max(maxX, (*it).x);
			minY = min(minY, (*it).y);
			maxY = max(maxY, (*it).y);
		}
		bound.set(0.0f, 0.0f, userScale_ * (maxX - minX), userScale_ * (maxY - minY));

		float scaleView = (float)UI_RenderBase::instance().windowPosition().height() / UI_RenderBase::instance().windowPosition().width();
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

		center_ += userShift_;
		position_.center(center_);
	}
	else {
		position_ = controlPose_;
		center_ = position_.center();
	}
}

void UI_Minimap::setScale(float newScale, const Vect2f& target)
{
	MTG();
	if(!inited())
		return;

	Vect2f targetOld(target);
	targetOld -= position_.center();

	float scaleOld = userScale_;
	userScale_ = clamp(newScale, 0.01f, 2.f);

	if(fabs(userScale_ - scaleOld) < FLT_COMPARE_TOLERANCE)
		return;

	if(fabs(userScale_ - 1.f) > FLT_EPS){
		Vect2f targetNew(targetOld);
		targetNew /= scaleOld;
		targetNew *= userScale_;

		drag(targetNew -= targetOld);
	}
	else
		userShift_.set(0, 0);
	
	reposition();
}

void UI_Minimap::zoomIn(const Vect2f& target)
{
	if(useUserScale_)
		setScale(clamp(userScale_ / 1.15f, 0.15f, 1.f), target);
}

void UI_Minimap::zoomOut(const Vect2f& target)
{
	if(useUserScale_)
		setScale(clamp(userScale_ * 1.15f, 0.15f, 1.f), target);
}

void UI_Minimap::drag(const Vect2f& delta)
{
	if(!useUserDrag_ || !inited())
		return;

	if(fabs(userScale_ - 1.f) > FLT_EPS){
		Vect2f range(position_.width() - controlPose_.width(), position_.height() - controlPose_.height());
		if(range.x < 0.f)
			range.x = 0.f;
		if(range.y < 0.f)
			range.y = 0.f;
		range *= 0.5f;
		userShift_.x = clamp(userShift_.x + delta.x, -range.x, range.x);
		userShift_.y = clamp(userShift_.y + delta.y, -range.y, range.y);
	}
	else
		userShift_.set(0, 0);

	reposition();
}

void UI_Minimap::setPosition(const Rectf& pos)
{
	if(controlPose_ != pos){
		controlPose_ = pos;
		reposition();
	}
}

void UI_Minimap::setRotate(float angle)
{
	if(fabs(rotationAngle_ - angle) > FLT_EPS){
		rotationAngle_ = angle;
		rotation_.set(rotationAngle_);
		reposition();
	}
}

void UI_Minimap::setCenter(const Vect2f& world)
{
	if(inited()){
		userShift_ += controlPose_.center();
		userShift_ -= world2minimap(world);
		reposition();
	}
}

bool UI_Minimap::hitTest(Vect2f mouse) const
{
	if(inited()){
		invXformPoint(mouse);
		return position_.point_inside(mouse);
	}
	return false;
}

Vect2f UI_Minimap::minimap2world(const Vect2f& mouse) const
{
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

Vect2f UI_Minimap::world2minimap(const Vect2f& world) const
{
	if(inited()){
		Vect2f tmp = world;
		tmp /= worldSize_;
		tmp *= position_.size();
		tmp += position_.left_top();

		return xformPoint(tmp);
	}
	return Vect2f(0.f, 0.f);
}

void UI_Minimap::redraw(float alpha)
{
	start_timer_auto();

	if(!inited() || alpha < FLT_EPS)
		return;

	if(mapTexture_)
		drawMiniMap(alpha);

	gb_RenderDevice3D->SetNoMaterial(ALPHA_BLEND, MatXf::ID);

	if(mask_){
		gb_RenderDevice3D->psMiniMapBorder->SetUseBorder(true);
		gb_RenderDevice3D->SetTexture(0, mask_);
		gb_RenderDevice3D->SetSamplerData(0, sampler_clamp_linear);
	}
	else
		gb_RenderDevice3D->psMiniMapBorder->SetUseBorder(false);

	if(drawEvents_){
		drawEvents(BACKGROUND, alpha);
		drawEvents(UNITS, alpha);
		drawEvents(EVENTS, alpha);
	}

	if(drawViewZone_)
		drawViewZone(alpha);

	if(!mask_){
		Color4c brd(borderColor_);
		brd.a *= alpha;
		if(brd.a > 0){
			drawLine(position_.left_top(), position_.right_top(), brd, true);
			drawLine(position_.right_top(), position_.right_bottom(), brd, true);
			drawLine(position_.right_bottom(), position_.left_bottom(), brd, true);
			drawLine(position_.left_bottom(), position_.left_top(), brd, true);
		}
	}

	flushLines();

	if(viewStartLocations_)
		drawStartLocations(alpha);
}

void UI_Minimap::drawMiniMap(float alpha)
{
	Color4f fog(0.f, 0.f, 0.f, alpha);
	Color4f terra(0.f, 1.0f, 0.f, 1.f);

	/*
	(--) (+-)
	(-+) (++)
	v1------v3
	|		|
	|		|
	v2------v4
	*/

	Vect2f c[4] = {
		Vect2f(-position_.width(), -position_.height()),
		Vect2f(-position_.width(), +position_.height()),
		Vect2f(+position_.width(), -position_.height()),
		Vect2f(+position_.width(), +position_.height())
	};
	Vect2i vi[4];

	for(int cidx = 0; cidx < 4; ++cidx){
		Vect2f& ci = c[cidx];
		ci *= 0.5f;
		ci *= scale_;
		ci *= rotation_;
		ci /= scale_;
		ci += center_;
		vi[cidx] = UI_RenderBase::instance().screenCoords(ci);
	}

	gb_RenderDevice3D->SetBlendStateAlphaRef(ALPHA_BLEND);
	
	if(mapTexture_){
		gb_RenderDevice3D->psMiniMap->SetUseTerraColor(false);
		gb_RenderDevice3D->SetSamplerData(0, sampler_clamp_linear);
		gb_RenderDevice3D->SetTexture(0, mapTexture_);
	}
	else
		gb_RenderDevice3D->psMiniMap->SetUseTerraColor(true);

	if(environment){
		if(drawWater_ && environment->water()){
			gb_RenderDevice3D->psMiniMap->SetUseWater(true);
			gb_RenderDevice3D->SetSamplerData(1, sampler_clamp_linear);
			gb_RenderDevice3D->SetTexture(1, environment->water()->GetTextureMiniMap());
		}
		else
			gb_RenderDevice3D->psMiniMap->SetUseWater(false);

		if(isFogOfWar() && environment->fogOfWar()){
			gb_RenderDevice3D->psMiniMap->SetUseAdditionTexture(true, false);
			gb_RenderDevice3D->SetSamplerData(2, sampler_clamp_linear);
			gb_RenderDevice3D->SetTexture(2, environment->fogOfWar()->GetTexture());
			gb_RenderDevice3D->psMiniMap->SetAdditionAlpha(environment->fogOfWar()->GetInvFogAlpha());
		}
		else if(isInstallZones() && placeZones_){
			gb_RenderDevice3D->psMiniMap->SetUseAdditionTexture(false, true);
			gb_RenderDevice3D->SetSamplerData(2, sampler_clamp_linear);
			gb_RenderDevice3D->SetTexture(2, placeZones_);
			gb_RenderDevice3D->psMiniMap->SetAdditionAlpha(environment->minimapZonesAlpha());
		}
		else
			gb_RenderDevice3D->psMiniMap->SetUseAdditionTexture(false, false);
	}
	else {
		gb_RenderDevice3D->psMiniMap->SetUseWater(false);
		gb_RenderDevice3D->psMiniMap->SetUseAdditionTexture(false, false);
	}

	if(mask_){
		gb_RenderDevice3D->psMiniMap->SetUseBorder(true);
		gb_RenderDevice3D->SetSamplerData(3, sampler_clamp_linear);
		gb_RenderDevice3D->SetTexture(3, mask_);
	}
	else
		gb_RenderDevice3D->psMiniMap->SetUseBorder(false);

	gb_RenderDevice3D->psMiniMap->Select(waterColor_, terra);

	cVertexBuffer<sVertexXYZWDT4>* vbuf = gb_RenderDevice3D->GetBufferXYZWDT4();
	sVertexXYZWDT4* v = vbuf->Lock(4);
	
	v[0].z = v[0].w = v[1].z = v[1].w = v[2].z = v[2].w = v[3].z = v[3].w = 0.001f;
	v[0].diffuse = v[1].diffuse = v[2].diffuse = v[3].diffuse = fog;
	
	v[0].x = vi[0].x - 0.5f; v[0].y = vi[0].y - 0.5f;
	v[1].x = vi[1].x - 0.5f; v[1].y = vi[1].y - 0.5f;
	v[2].x = vi[2].x - 0.5f; v[2].y = vi[2].y - 0.5f;
	v[3].x = vi[3].x - 0.5f; v[3].y = vi[3].y - 0.5f;

	v[0].u1() = v[0].u2() = v[0].u3() = 0;
	v[0].v1() = v[0].v2() = v[0].v3() = 0;

	v[1].u1() = v[1].u2() = v[1].u3() = 0;
	v[1].v1() = v[1].v2() = v[1].v3() = 1;

	v[2].u1() = v[2].u2() = v[2].u3() = 1;
	v[2].v1() = v[2].v2() = v[2].v3() = 0;

	v[3].u1() = v[3].u2() = v[3].u3() = 1;
	v[3].v1() = v[3].v2() = v[3].v3() = 1;

	// координаты для рамки обрезки миникарты
	if(mask_){
		for(int i = 0; i < 4; i++)
			setMaskUV(c[i], v[i].u4(), v[i].v4());
	}
	else {
		v[0].u4() = 0;
		v[0].v4() = 0;
		v[1].u4() = 0;
		v[1].v4() = 1;
		v[2].u4() = 1;
		v[2].v4() = 0;
		v[3].u4() = 1;
		v[3].v4() = 1;
	}

	vbuf->Unlock(4);
	vbuf->DrawPrimitive(PT_TRIANGLESTRIP, 2);
}

void UI_Minimap::drawEvents(EventType type, float alpha)
{
	MinimapEvents::const_iterator it;
	FOR_EACH(events_[type], it){
		const UI_MinimapEvent& event = *it;

		Vect2f size = event.size();
		size /= worldSize_;
		size *= position_.size();

		Vect2f pos = size;
		pos /= (-2.f);
		pos += world2minimap(event.position());

		Color4c color = event.color();
		color.a *= alpha;

		switch(event.symbol().type){
		case UI_MinimapSymbol::SYMBOL_RECTANGLE:
			rectangles_.push_back(Rectangle(pos, pos + size, color, event.rotation()));
			break;

		case UI_MinimapSymbol::SYMBOL_SPRITE:
			if(!event.symbol().sprite.isEmpty())
				drawSprite(Sprite(event.symbol().sprite, Rectf(pos, size), color, event.rotation()), 0.001f * event.time());
			break;
		}
	}	

	flushRectangles();
	flushSprites();
}

void UI_Minimap::drawViewZone(float alpha)
{
	Vect2f sq[4] = {
		screen2minimap(Vect2f(-.5f, -.5f)),
		screen2minimap(Vect2f(-.5f, 0.5f)),
		screen2minimap(Vect2f(0.5f, 0.5f)),
		screen2minimap(Vect2f(0.5f, -.5f))
	};

	Color4c color(viewZoneColor_);
	color.a *= alpha;

	drawLine(sq[0], sq[1], color);
	drawLine(sq[1], sq[2], color);
	drawLine(sq[2], sq[3], color);
	drawLine(sq[3], sq[0], color);
}

void UI_Minimap::drawLine(Vect2f pos0, Vect2f pos1, const Color4c& color, bool noCLip)
{
	if(noCLip || position_.clipLine(pos0, pos1)){
		if(mask_){
			Line2d line;
			line.c = color;
			line.p0 = xformPoint(pos0);
			line.p1 = xformPoint(pos1);
			lines_.push_back(line);
		}
		else {
			Vect2i pi0 = UI_RenderBase::instance().screenCoords(xformPoint(pos0));
			Vect2i pi1 = UI_RenderBase::instance().screenCoords(xformPoint(pos1));
			gb_RenderDevice3D->DrawLine(pi0.x, pi0.y, pi1.x, pi1.y, color);
		}
	}
}

void UI_Minimap::flushLines()
{
	gb_RenderDevice3D->psMiniMapBorder->SetUseTexture(false);
	gb_RenderDevice3D->psMiniMapBorder->Select();

	cVertexBuffer<sVertexXYZWDT1>* vbuf = gb_RenderDevice3D->GetBufferXYZWDT1();

	int npoint = 0;
	sVertexXYZWDT1* v = vbuf->Lock();
	
	Lines2d::const_iterator it;
	FOR_EACH(lines_, it){
		const Line2d& line = *it;
		sVertexXYZWDT1& v0 = v[npoint];
		sVertexXYZWDT1& v1 = v[npoint+1];

		Vect2i p0s = UI_RenderBase::instance().screenCoords(line.p0);
		Vect2i p1s = UI_RenderBase::instance().screenCoords(line.p1);

		setMaskUV(line.p0, v0.u1(), v0.v1());
		setMaskUV(line.p1, v1.u1(), v1.v1());

		v0.diffuse = v1.diffuse = line.c;

		v0.z = v1.z = 0.001f;
		v0.w = v1.w = 0.001f;

		v0.x = p0s.x;
		v0.y = p0s.y;

		v1.x = p1s.x;
		v1.y = p1s.y;
		
		npoint += 2;
		if(npoint >= vbuf->GetSize() - 2)
		{
			vbuf->Unlock(npoint);
			vbuf->DrawPrimitive(PT_LINELIST, npoint / 2);
			v = vbuf->Lock();
			npoint = 0;
		}
	}

	vbuf->Unlock(npoint);
	if(npoint)
		vbuf->DrawPrimitive(PT_LINELIST, npoint / 2);

	lines_.clear();
}

void UI_Minimap::drawSprite(const Sprite& sprite, float time)
{
	xassert(sprite.sprite && !sprite.sprite->isEmpty());
	if(sprite.sprite->isAnimated())
		animatedSprites_.push_back(AnimatedSprite(sprite, sprite.sprite->phase(time, true)));
	else
		spritesmap_[sprite.sprite->texture()].push_back(sprite);
}

void UI_Minimap::writeSprite(sVertexXYZWDT2* pv, const Sprite& data)
{
	const UI_Sprite& sprite = *data.sprite;

	Color4c color(
		round(data.color.r * sprite.diffuseColor().r / 255.f),
		round(data.color.g * sprite.diffuseColor().g / 255.f),
		round(data.color.b * sprite.diffuseColor().b / 255.f),
		round(data.color.a * sprite.diffuseColor().a / 255.f));


	Mat2f rot(data.rot + rotationAngle_);

	/*
	c0-----c3
	|		|
	|		|
	c1-----c2
	*/
	Vect2f c[4] = {
		Vect2f(-data.pos.width(), -data.pos.height()),
		Vect2f(-data.pos.width(), +data.pos.height()),
		Vect2f(+data.pos.width(), +data.pos.height()),
		Vect2f(+data.pos.width(), -data.pos.height())
	};
	Vect2i vi[4];
	Vect2f uv[4];
	/*
	v0,5----v4
	|		|
	|		|
	v1------v2,3
	*/
	Vect2f pos = data.pos.center();
	for(int idx = 0; idx < 4; ++idx){
		Vect2f& ci = c[idx];
		ci *= 0.5f;
		ci *= scale_;
		ci *= rot;
		ci /= scale_;
		ci += pos;
		vi[idx] = UI_RenderBase::instance().screenCoords(ci);
		setMaskUV(ci, uv[idx].x, uv[idx].y);
	}
	pv[0].diffuse = pv[1].diffuse = pv[2].diffuse = pv[3].diffuse = pv[4].diffuse = pv[5].diffuse = color;
	pv[0].w = pv[0].z = pv[1].w = pv[1].z = pv[2].w = pv[2].z = pv[3].w = pv[3].z = pv[4].w = pv[4].z = pv[5].w = pv[5].z = 0.001f;

	pv[5].u1() = pv[0].u1() = uv[0].x;
	pv[5].v1() = pv[0].v1() = uv[0].y;
	pv[3].u1() = pv[2].u1() = uv[2].x;
	pv[3].v1() = pv[2].v1() = uv[2].y;
	pv[1].u1() = uv[1].x;
	pv[1].v1() = uv[1].y;
	pv[4].u1() = uv[3].x;
	pv[4].v1() = uv[3].y;

	pv[5].x = pv[0].x = vi[0].x - 0.5f;
	pv[5].y = pv[0].y = vi[0].y - 0.5f;
	pv[3].x = pv[2].x = vi[2].x - 0.5f;
	pv[3].y = pv[2].y = vi[2].y - 0.5f;
	pv[1].x = vi[1].x - 0.5f;
	pv[1].y = vi[1].y - 0.5f;
	pv[4].x = vi[3].x - 0.5f;
	pv[4].y = vi[3].y - 0.5f;

	//gb_RenderDevice3D->DrawLine(vi[0].x, vi[0].y, vi[1].x, vi[1].y, Color4c::CYAN);
	//gb_RenderDevice3D->DrawLine(vi[2].x, vi[2].y, vi[1].x, vi[1].y, Color4c::CYAN);
	//gb_RenderDevice3D->DrawLine(vi[2].x, vi[2].y, vi[3].x, vi[3].y, Color4c::CYAN);
	//gb_RenderDevice3D->DrawLine(vi[0].x, vi[0].y, vi[3].x, vi[3].y, Color4c::CYAN);

	pv[0].u2() = pv[5].u2() = pv[4].u2() = sprite.textureCoords().left();
	pv[1].u2() = pv[2].u2() = pv[3].u2() = sprite.textureCoords().right();
	pv[0].v2() = pv[5].v2() = pv[1].v2() = sprite.textureCoords().top();
	pv[4].v2() = pv[2].v2() = pv[3].v2() = sprite.textureCoords().bottom();
}

void UI_Minimap::flushSprites()
{
	gb_RenderDevice3D->psMiniMapBorder->SetUseTexture(true);
	gb_RenderDevice3D->SetSamplerData(1, sampler_clamp_linear);
	gb_RenderDevice3D->psMiniMapBorder->Select();

	cVertexBuffer<sVertexXYZWDT2>* vbuf = gb_RenderDevice3D->GetBufferXYZWDT2();

	Spritesmap::iterator spit = spritesmap_.begin();
	for(;spit != spritesmap_.end(); ++spit)
	{
		Sprites& sprites = spit->second;
		if(sprites.empty())
			continue;

		int npoint = 0;
		sVertexXYZWDT2* v = vbuf->Lock();

		gb_RenderDevice3D->SetTexture(1, spit->first);

		Sprites::const_iterator it;
		FOR_EACH(sprites, it){
			writeSprite(v + npoint, *it);

			npoint += 6;
			if(npoint >= vbuf->GetSize() - 6)
			{
				vbuf->Unlock(npoint);
				vbuf->DrawPrimitive(PT_TRIANGLELIST, npoint / 3);
				v = vbuf->Lock();
				npoint = 0;
			}
		}

		vbuf->Unlock(npoint);
		if(npoint)
			vbuf->DrawPrimitive(PT_TRIANGLELIST, npoint / 3);

		sprites.clear();
	}

	if(animatedSprites_.empty())
		return;

	AnimatedSprites::const_iterator it;
	FOR_EACH(animatedSprites_, it){
		gb_RenderDevice3D->SetTexturePhase(1, it->sprite->texture(), it->phase);
		writeSprite(vbuf->Lock(6), *it);
		vbuf->Unlock(6);
		vbuf->DrawPrimitive(PT_TRIANGLELIST, 2);
	}
	animatedSprites_.clear();
}

void UI_Minimap::flushRectangles()
{
	if(rectangles_.empty())
		return;

	gb_RenderDevice3D->psMiniMapBorder->SetUseTexture(false);
	gb_RenderDevice3D->psMiniMapBorder->Select();

	cVertexBuffer<sVertexXYZWDT1>* vbuf = gb_RenderDevice3D->GetBufferXYZWDT1();

	int npoint = 0;
	sVertexXYZWDT1* v = vbuf->Lock();

	Rectangles::const_iterator it;
	FOR_EACH(rectangles_,it){
		sVertexXYZWDT1* pv = v + npoint;

		Vect2f v0 = UI_RenderBase::instance().screenCoords(it->v1);
		Vect2f v1 = UI_RenderBase::instance().screenCoords(it->v2);
		setMaskUV(it->v1, pv[0].u1(), pv[0].v1());
		setMaskUV(it->v2, pv[2].u1(), pv[2].v1());
		/*
		v0,5----v4
		|		|
		|		|
		v1------v2,3
		*/
		pv[0].w = pv[0].z = pv[4].w = pv[4].z = pv[2].w = pv[2].z = pv[1].w = pv[1].z = 0.001f;
		pv[0].diffuse = pv[4].diffuse = pv[2].diffuse = pv[1].diffuse = it->color;
		pv[0].x = pv[1].x = v0.x - 0.5f; pv[1].u1() = pv[0].u1();
		pv[4].x = pv[2].x = v1.x - 0.5f; pv[4].u1() = pv[2].u1();
		pv[0].y = pv[4].y = v0.y - 0.5f; pv[4].v1() = pv[0].v1();
		pv[2].y = pv[1].y = v1.y - 0.5f; pv[1].v1() = pv[2].v1();
		pv[3] = pv[2];
		pv[5] = pv[0];
		
		npoint += 6;
		if(npoint >= vbuf->GetSize() - 6)
		{
			vbuf->Unlock(npoint);
			vbuf->DrawPrimitive(PT_TRIANGLELIST, npoint / 3);
			v = vbuf->Lock();
			npoint = 0;
		}
	}

	vbuf->Unlock(npoint);
	if(npoint)
		vbuf->DrawPrimitive(PT_TRIANGLELIST, npoint / 3);

	rectangles_.clear();
}

void UI_Minimap::renderPlazeZones()
{
	MTG();

	struct TextureWorker
	{
		cTexture* texture;
		int width;
		int height;
		BYTE* data;
		int pitch;

		TextureWorker(cTexture* t) {
			data = (texture = t)->LockTexture(pitch);
			width = texture->GetWidth();
			height = texture->GetHeight();
		}
		
		~TextureWorker() {
			texture->UnlockTexture();
		}

		void draw(int y, int x, int xr, DWORD color){
			if(y < 0 || y >= height)
				return;
			x = max(x, 0);
			DWORD* ptr = reinterpret_cast<DWORD*>(data + y * pitch) + x;
			for(xr = min(xr, width); x < xr; ++x, ++ptr)
				*ptr = color;
		}

		void redraw(int y, int x, int xr, DWORD color){
			if(y < 0 || y >= height)
				return;
			x = max(x, 0);
			DWORD* ptr = reinterpret_cast<DWORD*>(data + y * pitch) + x;
			for(xr = min(xr, width); x < xr; ++x, ++ptr)
				if(!*ptr)
					*ptr = color;
		}
		void erase(int y, int x, int xr, DWORD color){
			if(y < 0 || y >= height)
				return;
			x = max(x, 0);
			DWORD* ptr = reinterpret_cast<DWORD*>(data + y * pitch) + x;
			for(xr = min(xr, width); x < xr; ++x, ++ptr)
				if(*ptr == color)
					*ptr = 0;
		}
	};

	if(!environment || !inited())
		return;

	if(isFogOfWar() || !isInstallZones()){
		zoneDrawTasks_.clear();
		return;
	}

	if(zoneDrawTasks_.empty())
		return;

	if(!placeZones_)
		placeZones_ = GetTexLibrary()->CreateTexture(worldSize_.x > 2048 ? 512 : 256, worldSize_.y > 2048 ? 512 : 256, false);

	start_timer_auto();

	TextureWorker worker(placeZones_);
	Vect2f scale((float)worker.width / worldSize_.x, (float)worker.height / worldSize_.y);

	ZoneDrawTasks::const_iterator zit;
	FOR_EACH(zoneDrawTasks_, zit){
		DWORD color = zit->color.RGBA();
		Vect2f pos = zit->pos;
		switch(zit->type){
			case PlaceZone::DRAW:
				for(float yl = 0; yl < zit->radius; ++yl){
					float xl = sqrtf(sqr(zit->radius) - sqr(yl));
					int x1 = round(scale.x * (pos.x - xl));
					int x2 = round(scale.x * (pos.x + xl));
					worker.draw(round(scale.y * (pos.y + yl)), x1, x2, color);
					worker.draw(round(scale.y * (pos.y - yl)), x1, x2, color);
				}
				break;
			case PlaceZone::REDRAW:
				for(float yl = 0; yl < zit->radius; ++yl){
					float xl = sqrtf(sqr(zit->radius) - sqr(yl));
					int x1 = round(scale.x * (pos.x - xl));
					int x2 = round(scale.x * (pos.x + xl));
					worker.redraw(round(scale.y * (pos.y + yl)), x1, x2, color);
					worker.redraw(round(scale.y * (pos.y - yl)), x1, x2, color);
				}
				break;
			case PlaceZone::ERASE:
				for(float yl = 0; yl < zit->radius; ++yl){
					float xl = sqrtf(sqr(zit->radius) - sqr(yl));
					int x1 = round(scale.x * (pos.x - xl));
					int x2 = round(scale.x * (pos.x + xl));
					worker.erase(round(scale.y * (pos.y + yl)), x1, x2, color);
					worker.erase(round(scale.y * (pos.y - yl)), x1, x2, color);
				}
				break;
		}
	}
	
	zoneDrawTasks_.clear();
}