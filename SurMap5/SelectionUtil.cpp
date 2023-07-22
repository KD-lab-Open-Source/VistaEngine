#include "StdAfx.h"
#include "SelectionUtil.h"

#include "UniverseObjectAction.h"
#include "..\Game\Universe.h"
#include "..\Game\CameraManager.h"
#include "..\Game\RenderObjects.h"
#include "..\Units\UnitEnvironment.h"
#include "..\Environment\Environment.h"
#include "..\Environment\Anchor.h"
#include "EditorVisual.h"

#include "SurToolPathEditor.h"
#include "SurToolCameraEditor.h"
#include "SurToolEnvironmentEditor.h"

#include "Serialization.h"
#include "EditArchive.h"

bool forFirstSelected(UniverseObjectAction& action, bool includeDead)
{
	if(!universe())
		return false;

    const PlayerVect& players = universe()->Players;
    PlayerVect::const_iterator pit;
    FOR_EACH (players, pit){
        Player& player = **pit;
        const UnitList& units = player.units();
        UnitList::const_iterator it;
        FOR_EACH(units, it){
            UnitBase* unit = *it;
            if(unit && !unit->auxiliary() && (includeDead || unit->alive()) && unit->selected() && !unit->attr().isSquad()) {
                action(*unit);
                return true;
            }
        }
    }

	if(!environment)
		return false;
	{
		Environment::Sources::iterator it;
		FOR_EACH(environment->sources(),it){
			SourceBase& source=**it;
			if(source.selected() && (includeDead || source.isAlive())) {
				action(source);
				return true;
			}
		}
	}

	{
		Environment::Anchors::const_iterator it;
		FOR_EACH(environment->anchors(),it){
            Anchor& anchor = **it;
			if(anchor.selected()){
				action(anchor);
				return true;
			}
		}
	}

	if(!cameraManager)
		return false;

	{
		CameraSplines& splines = cameraManager->splines();
		CameraSplines::iterator it;
		FOR_EACH(splines, it){
			CameraSpline& spline = **it;
			if(spline.selected()){
				action(spline);
				return true;
			}
		}
	}

	return false;
}

void forEachUniverseObject(UniverseObjectAction& action, bool includeDead)
{
	if(!universe())
		return;

    const PlayerVect& players = universe()->Players;
    PlayerVect::const_iterator pit;
    FOR_EACH (players, pit) {
        Player& player = **pit;
        const UnitList units = player.units();
        UnitList::const_iterator it;
        FOR_EACH(units, it){
            UnitBase* unit = *it;
            if(unit && !unit->auxiliary() && (includeDead || unit->alive()) && !unit->attr().isSquad())
                action(*unit);
        }
    }

	if(!environment)
		return;
	{
		Environment::Sources::iterator it;
		FOR_EACH(environment->sources(),it){
			SourceBase& source=**it;
			if(includeDead || source.isAlive())
				action(source);
		}
	}
	{
		Environment::Anchors::const_iterator it;
		FOR_EACH(environment->anchors(),it){
            Anchor& anchor = **it;
            action(anchor);
		}
	}
	if(!cameraManager)
		return;
	{
		CameraSplines& splines = cameraManager->splines();
		CameraSplines::iterator it;
		FOR_EACH(splines, it){
			if(CameraSpline* spline = *it)
				action(*spline);
			else
				xassert(0);
		}
	}
}

void forEachSelected(UniverseObjectAction& action, bool includeDead)
{
	forEachUniverseObject(UniverseObjectActions::DoForSelected(action), includeDead);
}

void deselectAll()
{
	if(!environment || !universe())
		return;

    environment->deselectAll();
    universe()->deselectAll();
	
	CameraSplines::iterator it;
	FOR_EACH(cameraManager->splines(), it)
		(*it)->setSelected(false);
}

BaseUniverseObject* unitHoverAll(const Vect2f& pos, bool ignore_non_selectable)
{
	Vect3f v0,v1;
	cameraManager->calcRayIntersection(pos, v0, v1);
	Vect3f v01 = v1 - v0;

	float dist, distMin = FLT_INF;
	UnitBase* unitMin = 0;

	PlayerVect::iterator pi;
	FOR_EACH(universe()->Players, pi){
		const UnitList& unit_list=(*pi)->units();
		UnitList::const_iterator i_unit;
		FOR_EACH(unit_list, i_unit){
			UnitBase* unit = *i_unit;
			if(unit->alive()){
				if(unit->model()){
					if(unit->model()->Intersect(v0,v1) &&
						distMin > (dist = unit->position().distance2(v0))){
							distMin = dist;
							unitMin = unit;
						}
				}
				else{
					Vect3f v0x = unit->position() - v0;
					Vect3f v_normal, v_tangent;
					decomposition(v01, v0x, v_normal, v_tangent);
					if(v_tangent.norm2() < sqr(unit->radius()) && distMin > (dist = v_normal.norm2())){
						distMin = dist;
						unitMin = unit;
					}
				}
			}
		}
	}

	return unitMin;
}

bool objectBoxTest(const Vect3f& pos, const sPlane4f* box)
{
	float leftDist = box[1].GetDistance(pos);
	float rightDist = box[2].GetDistance(pos);
	float topDist = box[3].GetDistance(pos);
	float bottomDist = box[4].GetDistance(pos);

	return
			(leftDist >= 0 && rightDist >= 0)
		&&	(topDist >= 0 && bottomDist >= 0);
}

Vect2i worldToScreen(const Vect3f& worldCoords)
{
	Vect3f e, w;
	Vect3f position(worldCoords);
	cameraManager->GetCamera()->ConvertorWorldToViewPort(&position, &w, &e);
	return Vect2i(round(e.x), round(e.y));
}

Vect3f screenPointToGround(const Vect2i& mouse_pos)
{
	Vect2f pos_in(mouse_pos.x / float(gb_RenderDevice->GetSizeX()) - 0.5f,
				  mouse_pos.y / float(gb_RenderDevice->GetSizeY()) - 0.5f);

	Vect3f pos,dir;
	cameraManager->GetCamera()->GetWorldRay(pos_in, pos, dir);
	Vect3f result;
	// проверяем пересечение с ландшафтом
	if(terScene->TraceDir(pos, dir, &result)){
		xassert(round(result.x) >= 0 && round(result.x) < vMap.H_SIZE &&
				round(result.y) >= 0 && round(result.y) < vMap.V_SIZE);					
		return result;
	}
	else{
		float rayLength = 5000.0f;
		Vect3f normal(Vect3f::K);
		// проверяем на пересечение с горизонтальной плоскостью
		if(intersect(normal, pos, pos + dir * rayLength, &result) && 
			round(result.x) >= 0 && round(result.x) < vMap.H_SIZE &&
			round(result.y) >= 0 && round(result.y) < vMap.V_SIZE)
			return result;
		else{
			// проверяем на пересечение с гранями bound-а мира
			Vect3f offset(Vect3f::ZERO);

			Vect3f flow[] = { Vect3f::I, Vect3f::ZERO,
				-Vect3f::I, Vect3f::I* vMap.H_SIZE,
				Vect3f::J, Vect3f::ZERO,
				-Vect3f::J, Vect3f::J * vMap.V_SIZE }; 
			const int size = sizeof(flow) / sizeof(flow[0]);

			Vect3f result(1e6f, 1e6f, 0.0f);
			Vect2f center(pos.x, pos.y);

			for(int i = 0; i < size; i += 2){
				Vect3f normal = flow[i];
				Vect3f offset = flow[i + 1];

				Vect3f point;
				if(intersect(normal, pos + offset, pos + offset + dir * rayLength, &point, false)){
					if(Vect2f(point.x + offset.x, point.y + offset.y).distance2(center) < Vect2f(result.x, result.y).distance2(center))
						result = point + offset;
				}
			}
			xassert(!result.eq(Vect3f(vMap.H_SIZE * 1e5f, vMap.V_SIZE* 1e5f, 0.0f)));
			result.x = clamp(result.x, 0.0f, vMap.H_SIZE - 1.0f);
			result.y = clamp(result.y, 0.0f, vMap.V_SIZE - 1.0f);
			result = To3D(Vect2f(result.x, result.y));
			return result;
		}
	}
}


bool intersect(const Vect3f& normal, const Vect3f& start, const Vect3f& end, Vect3f* result, bool withBackface)
{
	const float epsilon = 1e-6f;
	Vect3f pn = end - start;

	float n_pn = dot(normal, pn);
	if (n_pn >- epsilon && n_pn < epsilon)
		return false;
	float t = dot(normal, (/*a*/-start)) / n_pn;
	if (t < 0.0f || t > 1.0f)
		return false;

	if (result)
		*result = pn * t + start;
	return true;
}

void awakePhysics(BaseUniverseObject& object)
{
	UniverseObjectClass objectClass = object.objectClass();
	if(objectClass == UNIVERSE_OBJECT_UNIT || objectClass == UNIVERSE_OBJECT_ENVIRONMENT){
		UnitBase& unit = safe_cast_ref<UnitBase&>(object);
		if(unit.rigidBody())
			unit.rigidBody()->awake();
	}
}

bool isObjectFiltered(BaseUniverseObject& object)
{
	return editorVisual().isVisible(object.objectClass());
}

bool selectByScreenRectangle(Vect2i p1, Vect2i p2, bool select, bool deselectOutside)
{
	using namespace UniverseObjectActions;
    xassert(p1.x < 10e5 && p1.x > -10e5);
    xassert(p1.y < 10e5 && p1.y > -10e5);
    xassert(p2.x < 10e5 && p2.x > -10e5);
    xassert(p2.y < 10e5 && p2.y > -10e5);

    if(p1.x > p2.x)
        swap(p1.x,p2.x);
    if(p1.y > p2.y)
        swap(p1.y,p2.y);

    float width = gb_RenderDevice->GetSizeX();
    float height = gb_RenderDevice->GetSizeY();

    float x0 = float(p1.x) / width - 0.5f;
    float x1 = float(p2.x) / width - 0.5f;
    float y0 = float(p1.y) / height - 0.5f;
    float y1 = float(p2.y) / height - 0.5f;

    if (x0 > x1)
        swap(x0, x1);
    if (y0 > y1)
        swap(y0, y1);

    const float inf = 0.0001f;
    x0 -= inf;
    y0 -= inf;
    x1 += inf;
    y1 += inf;
    
    sPlane4f planeClip[5];
    cameraManager->GetCamera()->GetPlaneClip(planeClip, &sRectangle4f(x0, y0, x1, y1));

    bool selectionChanged = false;
    forEachUniverseObject(SelectByObjectBoxTest(selectionChanged, planeClip, select, deselectOutside), false);
	return selectionChanged;
}

void deleteSelectedUniverseObjects()
{
	environment->deleteSelected();
	universe()->deleteSelected();
	cameraManager->deleteSelected();
}


Vect3f projectScreenPointOnPlane(const Vect3f& normal, const Vect3f& offset, const Vect2i& screenPoint)
{
	Vect2f pos_in (screenPoint.x/float(gb_RenderDevice->GetSizeX()) - 0.5f,
				   screenPoint.y/float(gb_RenderDevice->GetSizeY()) - 0.5f);

	Vect3f pos, dir;
	cameraManager->GetCamera()->GetWorldRay (pos_in, pos, dir);
	Vect3f point;
	if (intersect (normal, (pos - offset), (pos + dir * 5000.0f - offset), &point)) {
		return point + offset;
	} else {
		return Vect3f::ZERO;
	}
}

// ---------------------------------------------------------------------------

namespace UniverseObjectActions{

DoForSelected::DoForSelected(UniverseObjectAction& action)
: action_(action)
{
}

void DoForSelected::operator()(BaseUniverseObject& object)
{
	if(object.selected())
		action_(object);
}

// ---------------------------------------------------------------------------

StorePose::StorePose(std::vector<PoseRadius>& poses)
: poses_(poses)
{
	poses_.clear ();
}

void StorePose::operator()(BaseUniverseObject& object)
{
	poses_.push_back(PoseRadius(Se3f (object.orientation(), object.position()), object.radius()));
}
//
// ---------------------------------------------------------------------------

RestorePose::RestorePose(std::vector<PoseRadius>& poses, bool init)
: poses_(poses)
, init_(init)
{
	it_ = poses_.begin ();
}

void RestorePose::operator()(BaseUniverseObject& object) {
	object.setPose(*it_, init_);
	if (fabsf(object.radius() - it_->radius) > FLT_COMPARE_TOLERANCE)
		object.setRadius(it_->radius);
	++it_;
}

// ---------------------------------------------------------------------------

RadiusExtractor::RadiusExtractor()
: impl_ (new RadiusExtractorImpl)
{ 
	impl_->count_ = 0; 
	impl_->radius_ = 0.0f;
	impl_->minimum_ = Vect3f::ZERO;
	impl_->maximum_ = Vect3f::ZERO;
}


void RadiusExtractor::operator()(BaseUniverseObject& object)
{
	act(object.radius(), object.position());
}

void RadiusExtractor::act(float radius, const Vect3f& v)
{
	if(impl_->count_ == 0){
		impl_->radius_ = radius;
		impl_->minimum_ = v;
		impl_->maximum_ = v;
	}else{
		if(impl_->minimum_.x > v.x)
			impl_->minimum_.x = v.x;
		if(impl_->minimum_.y > v.y)
			impl_->minimum_.y = v.y;
		if(impl_->maximum_.x < v.x)
			impl_->maximum_.x = v.x;
		if(impl_->maximum_.y < v.y)
			impl_->maximum_.y = v.y;
	}
	++impl_->count_;
}

float RadiusExtractor::radius() const
{
	if (fabs(impl_->minimum_.x - impl_->maximum_.x) < 1e-6 && fabs(impl_->minimum_.y - impl_->maximum_.y) < 1e-6) {
		return impl_->radius_;
	} else {
		return (impl_->maximum_ - impl_->minimum_).norm() * 0.5f;
	}
}

Vect3f RadiusExtractor::center() const
{
	return (impl_->minimum_ + impl_->maximum_) * 0.5f; 
}

int RadiusExtractor::count() const
{
	 return impl_->count_; 
}

// ---------------------------------------------------------------------------

SelectByObjectBoxTest::SelectByObjectBoxTest(bool& selectionChanged, const sPlane4f* planeClip, bool select, bool deselectOutside, bool selectHidden)
: planeClip_(planeClip)
, selectionChanged_(selectionChanged)
, select_(select)
, deselectOutside_(deselectOutside)
, selectHidden_(selectHidden)
{

}

void SelectByObjectBoxTest::operator()(BaseUniverseObject& object)
{
	if(isObjectFiltered(object)){
		if(::objectBoxTest(object.position(), planeClip_)){
			if(object.selected() != select_){
				selectionChanged_ = true;
				object.setSelected(select_);
			}
		}
		else{
			if(object.selected() && deselectOutside_){
				object.setSelected(false);
				selectionChanged_ = true;
			}
		}
	}
	else{
		object.setSelected(false);
		selectionChanged_ = true;
	}
}

// ---------------------------------------------------------------------------

GetSelectionInfo::GetSelectionInfo(bool& haveEditor, bool& movable, std::string& text)
: haveEditor_(haveEditor)
, text_(text)
, movable_(movable)
{
	haveEditor_ = false;
	movable_ = false;
	text_ = "";
}

void GetSelectionInfo::operator()(BaseUniverseObject& object)
{
	switch(object.objectClass()){
	case UNIVERSE_OBJECT_UNIT:
		text_ = TRANSLATE(ClassCreatorFactory<UnitBase>::instance().nameAlt(typeid(object).name()));
		movable_ = true;
		return;
	case UNIVERSE_OBJECT_ENVIRONMENT:
		{
		UnitEnvironment& unitEnvironment = safe_cast_ref<UnitEnvironment&>(object);
		text_ = unitEnvironment.modelName();
		if(!text_.empty()){
			std::string::size_type pos = text_.rfind ("\\") + 1;
			text_ = std::string(text_.begin() + pos, text_.end());
		}
		movable_ = true;
		}
		return;
	case UNIVERSE_OBJECT_SOURCE:
		haveEditor_ = true;
		movable_ = true;
		text_ = TRANSLATE("Редактировать траекторию");
		return;
	case UNIVERSE_OBJECT_CAMERA_SPLINE:
		haveEditor_ = true;
		movable_ = true;
		text_ = TRANSLATE("Редактировать сплайн");
		return;
	default:
		movable_ = true;
		return;
	}
}

// ---------------------------------------------------------------------------

EditorCreator::EditorCreator(ShareHandle<CSurToolBase>& editor)
: editor_(editor)
{
}

void EditorCreator::operator()(BaseUniverseObject& object)
{
	// TODO: сделать фабрику
	editor_ = 0;

	switch(object.objectClass()){
	case UNIVERSE_OBJECT_SOURCE:
		editor_ = new CSurToolPathEditor(&object);
		return;
	case UNIVERSE_OBJECT_CAMERA_SPLINE:
		editor_ = new CSurToolCameraEditor(&object);
		return;
	case UNIVERSE_OBJECT_ENVIRONMENT:
		editor_ = new CSurToolEnvironmentEditor(&object);
		return;
	}
}
// ---------------------------------------------------------------------------

void Cloner::operator()(BaseUniverseObject& object)
{
	switch(object.objectClass()){
	case UNIVERSE_OBJECT_SOURCE:
		{
		SourceBase* source = safe_cast<SourceBase*>(&object);
		sources_.push_back(environment->addSource(source));
		((SourceBase*)sources_.back())->setActivity(source->active());
		}
		return;
	case UNIVERSE_OBJECT_UNIT:
	case UNIVERSE_OBJECT_ENVIRONMENT:
		{
		UnitBase* unit = safe_cast<UnitBase*>(&object);
		Player* player = unit->player();
		UnitBase* newUnit = player->buildUnit(&unit->attr());

		EditOArchive oar;
		oar.serialize(*unit, "unit", "unit");
		EditIArchive iar(oar);
		iar.serialize(*newUnit, "unit", "unit");

		newUnit->setPose(unit->pose(), true);
		units_.push_back(newUnit);
		}
	default:
		return;
	};
}

};

// ---------------------------------------------------------------------------
