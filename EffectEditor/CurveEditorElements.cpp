#include "StdAfx.h"
#include "CurveEditorElements.h"
#include "kdw/CurveEditor.h"
#include "EffectDocument.h"
#include "kdw/Win32/Rectangle.h"
#include "kdw/Win32/Handle.h"
#include <windows.h>

namespace{
Win32::Handle<HBRUSH> brushGreen = CreateSolidBrush(RGB(0, 255, 0));
Win32::Handle<HBRUSH> brushRed = CreateSolidBrush(RGB(255, 0, 0));
Win32::Handle<HBRUSH> brushYellow = CreateSolidBrush(RGB(255, 255, 0));
Win32::Handle<HBRUSH> brushWhite = CreateSolidBrush(RGB(255, 255, 255));
Win32::Handle<HBRUSH> brushBlue = CreateSolidBrush(RGB(0, 0, 255));
Win32::Handle<HBRUSH> brushBlack = CreateSolidBrush(RGB(0, 0, 0));
				
Win32::Handle<HPEN> penBlue = CreatePen(PS_SOLID, 5, RGB(0, 0, 255));
Win32::Handle<HPEN> penBlueThin = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
Win32::Handle<HPEN> penRed = CreatePen(PS_SOLID, 4, RGB(255, 0, 0));
Win32::Handle<HPEN> penRedPoint = CreatePen(PS_DOT, 1, RGB(255, 0, 0));
Win32::Handle<HPEN> penGrayPoint = CreatePen(PS_DOT, 1, RGB(128, 128, 128));
Win32::Handle<HPEN> penGreen = CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
Win32::Handle<HPEN> penCyan = CreatePen(PS_SOLID, 2, RGB(0, 128, 255));
Win32::Handle<HPEN> penGray = CreatePen(PS_SOLID, 4, RGB(192, 192, 192));
Win32::Handle<HPEN> penGrayThin = CreatePen(PS_SOLID, 1, RGB(192, 192, 192));
}

// ---------------------------------------------------------------------------

GenerationPointElement::GenerationPointElement()
: parent_(0)
, position_(Vect2f::ZERO)
{
}

void GenerationPointElement::commit()
{
	parent_->commit();
}

void GenerationPointElement::setParent(GenerationTimeLineElement* parent)
{
	parent_ = parent;
}

void GenerationPointElement::redraw(HDC dc, const kdw::ViewTransform& view)
{
	bool selected = parent_->pointIndex(this) == parent_->emitter()->selectedGenerationPoint();
	HGDIOBJ oldPen = ::SelectObject(dc, penBlueThin);
	HGDIOBJ oldBrush = ::SelectObject(dc, selected ? brushRed : brushWhite);

	Vect2i point(view.transformX(position_.x), view.viewInnerBounds().bottom());
	int radius = 6;
	int offset = GenerationTimeLineElement::OFFSET;
	Ellipse(dc, point.x - radius, point.y - radius + offset, point.x + radius, point.y + radius + offset);

	::SelectObject(dc, oldPen);
	::SelectObject(dc, oldBrush);
}


void GenerationPointElement::move(Vect2f delta, const kdw::ViewTransform& view)
{
	position_.x = max(position_.x + delta.x, 0.0f);
	parent_->pointMoved(this);
}

kdw::CurveEditorElement* GenerationPointElement::hitTest(Vect2f position, const kdw::ViewTransform& view)
{

	Vect2i pos(view.transformX(position_.x), view.viewInnerBounds().bottom() + GenerationTimeLineElement::OFFSET);
	Recti rect(pos - Vect2i(4, 4), Vect2i(8, 8));
	Vect2i positioni(view(position));
	if(rect.point_inside(positioni)){
		int point = parent()->emitter()->selectedGenerationPoint();
		int pointIndex = parent()->pointIndex(this);
		if(point != pointIndex){
			parent()->emitter()->setSelectedGenerationPoint(pointIndex);
			parent()->editor()->rebuild();
		}

		return this;
	}
	else
		return 0;
}

// ---------------------------------------------------------------------------

GenerationTimeLineElement::GenerationTimeLineElement(NodeEmitter* emitter)
: emitter_(emitter)
{
	rebuild();
}


void GenerationTimeLineElement::rebuild()
{
	int count = emitter()->get()->generationPointCount();
	points_.resize(count);
	for(int i = 0; i < count; ++i){
		if(!points_[i])
			points_[i] = new GenerationPointElement();
		float time = emitter()->get()->getGenerationPointTime(i);
		points_[i]->setPosition(Vect2f(time, 0.0f));
		points_[i]->setParent(this);
	}
}


void GenerationTimeLineElement::commit()
{
	globalDocument->history().pushEmitter();

	int count = emitter()->get()->generationPointCount();
	xassert(count <= points_.size());
	emitter()->get()->emitter_life_time = points_[count - 1]->position().x;
	for(int i = 0; i < count - 1; ++i){
		ShareHandle<GenerationPointElement>& point = points_[i];
		if(!point)
			point = new GenerationPointElement();
 		emitter_->get()->setGenerationPointTime(i, point->position().x);
	}
	xassert(editor_);
	editor_->rebuild();
}

void GenerationTimeLineElement::redraw(HDC dc, const kdw::ViewTransform& view)
{
	int count = emitter()->get()->generationPointCount(); 
	float right = emitter()->get()->getGenerationPointTime(count - 1);
	float left = emitter()->get()->getGenerationPointTime(0);
	Vect2i start(view.transformX(left), view.viewInnerBounds().bottom() + OFFSET);
	Vect2i end(view.transformX(right), view.viewInnerBounds().bottom() + OFFSET);

	HGDIOBJ oldPen = ::SelectObject(dc, HGDIOBJ(penBlue));

	MoveToEx(dc, start.x, start.y, 0);
	LineTo(dc, end.x, end.y);

	::SelectObject(dc, oldPen);

	Points::iterator it;
	FOR_EACH(points_, it){
		GenerationPointElement& point = **it;
		point.redraw(dc, view);
	}
}

kdw::CurveEditorElement* GenerationTimeLineElement::hitTest(Vect2f position, const kdw::ViewTransform& view)
{
	Points::iterator it;
	FOR_EACH(points_, it){
		ShareHandle<GenerationPointElement>& point = *it;
		if(kdw::CurveEditorElement* result = point->hitTest(position, view))
			return result;
	}
	return 0;
}

int GenerationTimeLineElement::pointIndex(GenerationPointElement* point)
{
	Points::iterator it;
	int index = 0;
	FOR_EACH(points_, it){
		if(point == *it)
			return index;
		++index;
	}
	return -1;
}

void GenerationTimeLineElement::pointMoved(GenerationPointElement* point)
{
	Vect2f pos = point->position();
	float minLength = 0.001f;
	for(int i = 0; i < points_.size(); ++i){
		if(points_[i] == point){
			if(i > 0){
				if(pos.x < points_[i - 1]->position().x + minLength)
					pos.x = points_[i - 1]->position().x + minLength;
			}
			else{
				pos.x = 0.0f;
			}
			if(i < points_.size() - 1){
				if(pos.x > points_[i + 1]->position().x - minLength)
					pos.x = points_[i + 1]->position().x - minLength;
			}
			point->setPosition(pos);
			break;
		}
	}
}

bool GenerationTimeLineElement::getBoundingBox(Rectf& rect)
{
	int count = emitter()->get()->generationPointCount();
	rect.set(0.0f, 0.0f, emitter()->get()->getGenerationPointTime(count - 1), 0.0f);
	return true;
}

// ---------------------------------------------------------------------------

EmitterLifeTimeElement::EmitterLifeTimeElement(NodeEmitter* nodeEmitter, int index, const char* title)
: nodeEmitter_(nodeEmitter)
, endPoint_(Vect2i::ZERO)
, index_(index)
, currentPoint_(-1)
, currentEndPoint_(false)
{
	name_ = title;
	rebuild();
}


NodeEmitter* EmitterLifeTimeElement::node()
{
	return nodeEmitter_;
}

kdw::CurveEditorElement* EmitterLifeTimeElement::hitTest(Vect2f position, const kdw::ViewTransform& view)
{
	currentEndPoint_ = false;
	bool f = false;
	currentPoint_ = -1;
	Win32::Rect rc(view(points_.front()), view(points_.back()));
	InflateRect(&rc, 4, 4);
	POINT pt = { int(view(position).x), int(view(position).y) };
	if(PtInRect(&rc, pt)){
		for(int i = 0; i < points_.size(); ++i){
			if (abs(pt.x - view(points_[i]).x)<=4 && abs(pt.y - view(points_[i]).y)<=4){
				currentPoint_ = i;
				if(currentPoint_)
					break;
			}
		}
		f = true;
	}
	Win32::Rect rcp(view(endPoint_), view(endPoint_));
	InflateRect(&rcp, 4, 4);
	if(PtInRect(&rcp, pt)){
		currentEndPoint_ = true;
		f = true;
	}
	if(f) 
		return this;
	return 0;
}

void EmitterLifeTimeElement::move(Vect2f delta, const kdw::ViewTransform& view)
{
	if(currentPoint_ < 0)
	{
		if(currentEndPoint_)
			endPoint_.x += delta.x;
		else{
			Points::iterator it;
			FOR_EACH(points_, it)
				it->x += delta.x;
		}
	}
	else{
		points_[currentPoint_].x += delta.x;
		if(currentPoint_ > 0)
			if (points_[currentPoint_].x<points_[currentPoint_-1].x) 
				points_[currentPoint_].x = points_[currentPoint_-1].x;
		if	(currentPoint_<points_.size()-1)
			if (points_[currentPoint_].x>points_[currentPoint_+1].x) 
				points_[currentPoint_].x = points_[currentPoint_+1].x;
	}
}

void EmitterLifeTimeElement::commit()
{
	EmitterKeyInterface* emitter = nodeEmitter_->get();
	if(currentPoint_ < 0 && !currentEndPoint_)
		emitter->emitter_create_time = points_.front().x;

	if(currentPoint_ >= 0){
		if(currentPoint_ == 0 || currentPoint_ == points_.size() - 1){
			float t = points_[0].x;
			if(t < 0.0f)
				t = 0.0f;
			float dt = emitter->emitter_create_time - t;
			emitter->emitter_create_time = t;
			emitter->emitter_life_time += dt;
			
			t = points_.back().x;
			t -= emitter->emitter_create_time;
			if(t < 0.05f)
				t = 0.05f;
			emitter->emitter_life_time = t;
			//nodeEmitter_->setDirty(true);
		}
		else{
			float t = points_[currentPoint_].x - emitter->emitter_create_time;
			emitter->setGenerationPointTime(currentPoint_, t/* / emitter->emitter_life_time*/);
		}
	}

	if(currentEndPoint_){
		// if(!pKey->IsCLight() && !pKey->IsLight())
		float t = endPoint_.x - emitter->emitter_create_time;


		emitter->setParticleLifeTime(t);
	}
	if(emitter->emitter_create_time < 0)
		emitter->emitter_create_time = 0;

	//_pWndGraph->GetParent()->SendMessage(WM_SLIDER_UPDATE);
}

void EmitterLifeTimeElement::rebuild()
{
	xassert(nodeEmitter_);

	endPoint_ = Vect2i(0, 0);

	EmitterKeyInterface* emitter = nodeEmitter_->get();
	
	float time_from = emitter->emitter_create_time;
	int count = emitter->generationPointCount();

	NodeEffect* nodeEffect = safe_cast<NodeEffect*>(nodeEmitter_->parent());
	
	float y = float(nodeEffect->childrenCount() - index_);
	for(int i = 0; i < count; ++i){
		float x = emitter->getGenerationPointTime(i) + time_from;
		points_.push_back(Vect2f(x, y));
	}
	if(count == 1){
		float x = emitter->getEmitterEndTime();
		points_.push_back(Vect2f(x, y));
	}
	float px = time_from + emitter->getParticleLongestLifeTime();
	endPoint_ = Vect2f(px, y);
}

void EmitterLifeTimeElement::redraw(HDC dc, const kdw::ViewTransform& view)
{
	if(points_.empty())
		return;

	NodeEmitter* nodeEmitter = nodeEmitter_;
	
	bool active = globalDocument->activeEmitter() == nodeEmitter;

	//------ Время частиц (пунктирная)
	if(active)
		SelectObject(dc, penRedPoint);
	else
		SelectObject(dc, penGrayPoint);
	
	Vect2i endPoint = view(endPoint_);
	Vect2i front = view(points_.front());
	Vect2i back = view(points_.back());

	MoveToEx(dc, back.x, back.y, 0);
	LineTo(dc, endPoint.x, endPoint.y);

	//------- Время эмиттера (сплошная)
	if(active)
		SelectObject(dc, penRed);
	else
		SelectObject(dc, penGray);

	MoveToEx(dc, front.x, front.y, 0);
	LineTo(dc, back.x, back.y);

	SelectObject(dc, GetStockObject(BLACK_PEN));

	SelectObject(dc, brushBlue);
	Win32::Rect rc(endPoint.x, endPoint.y, endPoint.x, endPoint.y);
	InflateRect(&rc, 4, 4);
	Ellipse(dc, rc.left, rc.top, rc.right, rc.bottom);

	SelectObject(dc, brushYellow);
	//PointList::iterator i;
	for(int i = 0; i < points_.size() - 1; i++){
		Vect2i point = view(points_[i]);
		Win32::Rect rc(point.x, point.y, point.x, point.y);
		InflateRect(&rc, 4, 4);
		Ellipse(dc, rc.left, rc.top, rc.right, rc.bottom);
	}
	int c = 0;//nodeEmitter->GetGenerationPointNum();
	if(c == 1)
		SelectObject(dc, GetStockObject(BLACK_BRUSH));

	rc = Win32::Rect(back.x, back.y, back.x, back.y);
	InflateRect(&rc, 4, 4);
	Ellipse(dc, rc.left, rc.top, rc.right, rc.bottom);

	// Draw the name
	COLORREF oldcol = SetTextColor(dc, RGB(0,0,0));

	int x;
	if(endPoint.x > back.x)
		x = endPoint.x + 10;
	else
		x = back.x + 10;
	int y = front.y + 3;
	UINT oldalign = SetTextAlign(dc, TA_LEFT|TA_BASELINE);

	TextOut(dc, x, y, name_.c_str(), strlen(name_.c_str()));

	SetTextColor(dc, oldcol);
	SetTextAlign(dc, oldalign);
}

bool EmitterLifeTimeElement::getBoundingBox(Rectf& rect)
{
	if(points_.empty())
		rect.set(0.0f, 0.0f, 0.0f, 0.0f);
	else{
		rect.set(float(points_[0].x), float(points_[0].y), 0.0f, 0.0f);
		Points::iterator it;
		FOR_EACH(points_, it){
			Vect2f pt = *it;
			if(pt.x < rect.left()){
				float right = rect.right();
				rect.left(pt.x);
				rect.width(right - pt.x);
			}
			if(pt.x > rect.right()){
				rect.width(pt.x - rect.left());
			}
			if(pt.y < rect.top()){
				float bottom = rect.bottom();
				rect.top(pt.y);
				rect.height(bottom - pt.y);
			}
			if(pt.y > rect.bottom()){
				rect.height(pt.y - rect.top());
			}
		}
		rect = rect.merge(Rectf(endPoint_.x, endPoint_.y, 0.0f, 0.0f));
	}
	return true;
}

// ---------------------------------------------------------------------------
EffectLifeTimeElement::EffectLifeTimeElement(NodeEffect* nodeEffect)
: nodeEffect_(nodeEffect)
{
	rebuild();
}

void EffectLifeTimeElement::rebuild()
{
	emitters_.clear();

	int count = nodeEffect_->childrenCount();
	for(int i = 0; i < count; ++i){
		NodeEmitter* emitter = nodeEffect_->child(i);
		emitters_.push_back(new EmitterLifeTimeElement(emitter, i, emitter->name()));
	}
}

void EffectLifeTimeElement::redraw(HDC dc, const kdw::ViewTransform& view)
{
	Emitters::iterator it;
	FOR_EACH(emitters_, it){
		EmitterLifeTimeElement* emitter = *it;
		emitter->redraw(dc, view);
	}
}

kdw::CurveEditorElement* EffectLifeTimeElement::hitTest(Vect2f position, const kdw::ViewTransform& view)
{
	Emitters::iterator it;
	FOR_EACH(emitters_, it){
		EmitterLifeTimeElement* emitter = *it;
		if(CurveEditorElement* result = emitter->hitTest(position, view))
			return result;
	}
	return 0;
}

bool EffectLifeTimeElement::getBoundingBox(Rectf& rect)
{
	if(emitters_.empty())
		return false;
	else{
		emitters_.front()->getBoundingBox(rect);
		Emitters::iterator it;
		FOR_EACH(emitters_, it){
			EmitterLifeTimeElement* emitter = *it;
			Rectf temp;
			if(emitter->getBoundingBox(temp)){
				rect = rect.merge(temp);
			}
			
		}
		return true;
	}
}

// --------------------------------------------------------------------------- 

CurvePoint::CurvePoint(Vect2f position, CurveCKey* parent)
: position_(position)
, parent_(parent)
{ 

}

void CurvePoint::redraw(HDC dc, const kdw::ViewTransform& view)
{
	bool selected = parent()->active() && (parent()->node()->selectedPoint() == parent()->pointIndex(this));

	Vect2i point = view(position_);
	int radius = 5;
	
	HGDIOBJ oldBrush = ::SelectObject(dc, selected ? brushRed : brushWhite);
	HGDIOBJ oldPen = ::SelectObject(dc, penGrayThin);
	Ellipse(dc, point.x - radius, point.y - radius, point.x + radius, point.y + radius);
	::SelectObject(dc, oldBrush);
	::SelectObject(dc, oldPen);
}

void CurvePoint::move(Vect2f delta, const kdw::ViewTransform& view)
{
	if(!parent()->wrapper()->keyPerGeneration()){
		position_.x += delta.x;
	}
	else{
		if(parent()->wrapper()->size() == 1){
			parent()->moveAllPoints(Vect2f(0.0f, delta.y), view);
			return;
		}
	}
	position_.y += delta.y;
	parent()->pointMoved(this);
}

void CurvePoint::commit()
{
	parent()->commit();
}

void CurvePoint::remove()
{
	parent_->removePoint(this);
}

kdw::CurveEditorElement* CurvePoint::hitTest(Vect2f position, const kdw::ViewTransform& view)
{
	int index = parent()->pointIndex(this);
	int radius = 4;
	Vect2i pos  = view(position_);
	Recti rect(pos.x - radius, pos.y - radius, radius * 2, radius * 2);
	if(rect.point_inside(view(position))){
		if(parent()->wrapper()->keyPerGeneration()){
			parent()->emitter()->setSelectedGenerationPoint(index);
		}
		parent()->selectPoint(this);
		globalDocument->onCurveChanged(false);
		parent()->editor()->recalculateViewport();
		return this;
	}
	else
		return 0;
}

// --------------------------------------------------------------------------- 

CurveCKey::CurveCKey(NodeCurve* nodeCurve, bool active)
: nodeCurve_(nodeCurve)
, active_(active)
, length_(1.0f)
, pointToSelect_(-1)
{
	rebuild();
}

CurveCKey::~CurveCKey()
{
	xassert(!nodeCurve_ || nodeCurve_->numRef() > 1);
}

void CurveCKey::rebuild()
{
	int generationPoint = nodeCurve_->parent()->selectedGenerationPoint();
	xassert(nodeCurve_ != 0);
	CurveWrapperBase* wrapper = nodeCurve_->wrapper();
	if(!wrapper->keyPerGeneration()){
		int count = wrapper->size();
		xassert(count >= 0);
		points_.resize(count);

		for(int i = 0; i < count; ++i){
			float time = emitter()->get()->particleKeyTime(generationPoint, i);
			float value = wrapper->value(i);

			Vect2f position(time, value);
			if(points_[i]){
				points_[i]->setPosition(position);
				if(pointToSelect_ == i){
					selectPoint(points_[i]);
					editor()->setSelectedElement(points_[i], true);
					pointToSelect_ = -1;
				}
			}
			else
				points_[i] = new CurvePoint(position, this);
		
			if(i == 0 || i == count - 1)
                points_[i]->setRemovable(false);
			else
				points_[i]->setRemovable(true);
		}

		//globalDocument->onCurveChanged();
	}
	else{
		int numPoints = emitter()->get()->generationPointCount();
		int count = max(numPoints, 2);
		points_.resize(count);
		for(int i = 0; i < count; ++i){
			Vect2f position;
			if(i < numPoints){
				position.x = emitter()->get()->getGenerationPointTime(i);
				position.y = wrapper->value(i);
			}
			else{
				position.x = emitter()->get()->emitter_life_time;
				position.y = wrapper->value(0);
			}

			if(points_[i])
				points_[i]->setPosition(position);
			else
				points_[i] = new CurvePoint(position, this);
            points_[i]->setRemovable(false);
		}
	}
	length_ = points_.back()->position().x - points_.front()->position().x;
}

void CurveCKey::redraw(HDC dc, const kdw::ViewTransform& view)
{
	HPEN pen = active_ ? (wrapper()->keyPerGeneration() ? penCyan : penGreen) : penGray;
	if(!points_.empty()){
		HGDIOBJ oldBrush = SelectObject(dc, HBRUSH(GetStockObject(NULL_BRUSH)));
		HGDIOBJ oldPen = SelectObject(dc, pen);
		Vect2i point(view(points_[0]->position()));
		MoveToEx(dc, point.x, point.y, 0);

		Points::iterator it;
		FOR_EACH(points_, it){
			CurvePoint* currentPoint = *it;
			xassert(currentPoint);
			if(currentPoint){
				Vect2i point(view(currentPoint->position()));
				LineTo(dc, point.x, point.y);
			}
		}
		::SelectObject(dc, oldPen);
		::SelectObject(dc, oldBrush);
	}

	Points::iterator it;
	FOR_EACH(points_, it){
		CurvePoint* point = *it;
		xassert(point);
		point->redraw(dc, view);
	}
}

void CurveCKey::pointMoved(CurvePoint* point)
{
	if(point == points_.front()){
		point->setPosition(Vect2f(0.0f, point->position().y));
	}
	else{
		if(point == points_.back()){
			float scalePoint = points_.front()->position().x;

			float scale = max(0.01f, (point->position().x - scalePoint) / length_);
			
			for(int i = 1; i < points_.size() - 1; ++i){
				Vect2f pos = points_[i]->position();
                points_[i]->setPosition(Vect2f((pos.x - scalePoint) * scale + scalePoint, pos.y));
			}
			length_ = point->position().x - scalePoint;
		}
		Vect2f pos = point->position();	
		for(int i = 1; i < points_.size(); ++i){
			if(points_[i] == point){
				if(pos.x < points_[i - 1]->position().x)
					pos.x = points_[i - 1]->position().x;
				if(i < points_.size() - 1){
					if(pos.x > points_[i + 1]->position().x)
						pos.x = points_[i + 1]->position().x;
				}
				point->setPosition(pos);
				break;
			}
		}
	}
}

int CurveCKey::pointIndex(CurvePoint* point)
{
	int index = 0;
	Points::iterator it;
	FOR_EACH(points_, it){
		if(*it == point)
			return index;
		++index;
	}
	return -1;
}

void CurveCKey::moveAllPoints(Vect2f delta, const kdw::ViewTransform& view)
{
	Points::iterator it;
	FOR_EACH(points_, it){
		CurvePoint* point = *it;
		point->setPosition(point->position() + delta);
		pointMoved(point);
	}
}

bool CurveCKey::getBoundingBox(Rectf& rect)
{
	Rectf bound(0.0f, 0.0f, 0.0f, 0.0f);
	Points::iterator it;
	FOR_EACH(points_, it){
		CurvePoint* point = *it;
 		xassert(point);
		bound = bound.merge(Rectf(point->position(), Vect2f(0.0f, 0.0f)));
	}
	rect = bound;
	return true;
}

NodeEmitter* CurveCKey::emitter()
{
	return safe_cast<NodeEmitter*>(nodeCurve_->parent());
}

kdw::CurveEditorElement* CurveCKey::addPoint(float position)
{
	/*
	for(int i = 0, j = 1; j < points_.size(); ++j, ++i){
		Vect2f pos1 = points_[i]->position();
		Vect2f pos2 = points_[j]->position();
		if(position > pos1.x && position < pos2.x){
			float pos = (position - pos1.x) / (pos2.x - pos1.x);
			Vect2f newPosition = (pos2 - pos1) * pos + pos1;
			CurvePoint* point = new CurvePoint(newPosition, this);
			points_.insert(points_.begin() + j, point);
			return point;
		}
	}
	*/
	insertedPoints_.push_back(position);
	return 0;
}

void CurveCKey::removePoint(CurvePoint* point)
{
	int index = pointIndex(point);
    removedPoints_.push_back(index);
	commit();
}

void CurveCKey::selectPoint(CurvePoint* point)
{
	int index = pointIndex(point);
	node()->setSelectedPoint(index);
}

NodeCurve* CurveCKey::node()
{
	return nodeCurve_;
}

CurveWrapperBase* CurveCKey::wrapper()
{
	return nodeCurve_->wrapper();
}


bool pointOnLine(const Vect2f& p, const Vect2f& p1, const Vect2f& p2, float precision = 1.0f)
{
	Rectf rect(p1, p2 - p1);
	rect.validate();
	if(rect.point_inside(p)){
		Vect2f toPoint(p.x - p1.x, p.y - p1.y);
		Vect2f line(p2.x - p1.x, p2.y - p1.y);
		Vect2f normal(-line.y, line.x);
		normal.normalize(1.0f);

		float dist = fabsf(normal.dot(toPoint));
		
		return (dist < precision);
	}

	return false;
}

void CurveCKey::click(Vect2f position, const kdw::ViewTransform& view)
{
	if(active()){
		for(int i = 0, j = 1; j < points_.size(); ++j, ++i){
			if(pointOnLine(position, points_[i]->position(), points_[j]->position(), view.untransformX(10) - view.untransformX(0))){
				addPoint(position.x);
				commit();
			}		
		}
	}
}

kdw::CurveEditorElement* CurveCKey::hitTest(Vect2f position, const kdw::ViewTransform& view)
{
	if(!active_)
		return 0;
	Points::iterator it;
	FOR_EACH(points_, it){
		CurvePoint* point = *it;
		kdw::CurveEditorElement* result = point->hitTest(position, view);
		if(result)
			return result;
	}
	return 0;
}


float inBetweenPosition(CurveWrapperBase* wrapper, float time, int& beforeIndex){
	
	for(int i = 1; i < wrapper->size(); ++i){
		if(wrapper->time(i - 1) < time && wrapper->time(i) > time){
            beforeIndex = i;
			float len = wrapper->time(i) - wrapper->time(i - 1);
			if(len > FLT_EPS)
				return (time - wrapper->time(i - 1)) / len;
			else
				return 0.0f;
		}
	}
    beforeIndex = wrapper->size();
	return 0.5f;
}


void CurveCKey::commit()
{
	globalDocument->history().pushEmitter();

	int generationPoint = nodeCurve_->parent()->selectedGenerationPoint();
	CurveWrapperBase* wrapper = nodeCurve_->wrapper();
	if(!wrapper->keyPerGeneration()){
		emitter()->get()->changeParticleLifeTime(generationPoint, length_ + points_.front()->position().x);
		int count = points_.size();

		for(int i = 0; i < count; ++i){
			float time = points_[i]->position().x;
			float value = points_[i]->position().y;
			wrapper->setPoint(i, time, value);
			emitter()->get()->setParticleKeyTime(generationPoint, i, time);
			emitter()->buildKey();	
		}
		for(int i = 0; i < removedPoints_.size(); ++i){
			int pointIndex = removedPoints_[i];
			emitter()->get()->deleteParticleKey(pointIndex);
		}
		removedPoints_.clear();
		for(int j = 0; j < insertedPoints_.size(); ++j){
			float time = insertedPoints_[j];
			int before = 0;
			float position = inBetweenPosition(wrapper, time / length_, before);
			emitter()->get()->insertParticleKey(before, position);
			pointToSelect_ = before;
		}
		insertedPoints_.clear();
	}
	else{
		int count = emitter()->get()->generationPointCount();
		for(int i = 0; i < count; ++i){
			float value = points_[i]->position().y;
			wrapper->setValue(i, value);		
		}
	}
	globalDocument->onCurveChanged(true);
	xassert(editor_);
	editor_->rebuild();
}
