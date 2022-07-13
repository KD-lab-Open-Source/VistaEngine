#ifndef __SELECTION_UTIL_H_INCLUDED__
#define __SELECTION_UTIL_H_INCLUDED__

#include "SurToolAux.h"
//class CSurToolBase;
#include "Render\3dx\Umath.h"
#include "UniverseObjectAction.h"
#include "XMath\Plane.h"


namespace UniverseObjectActions{

struct DoForSelected : public UniverseObjectAction{
	DoForSelected(UniverseObjectAction& action);
	void operator()(BaseUniverseObject& object);
	UniverseObjectAction& action_;
};


struct StorePose : UniverseObjectAction {
    StorePose(std::vector<PoseRadius>& poses);
	void operator()(BaseUniverseObject& object);
	std::vector<PoseRadius>& poses_;
};

struct RestorePose : UniverseObjectAction{
	RestorePose(std::vector<PoseRadius>& poses, bool init);
	void operator()(BaseUniverseObject& object);

	bool init_;
	std::vector<PoseRadius>::iterator it_;
    std::vector<PoseRadius>& poses_;
};


struct RadiusExtractor : UniverseObjectAction
{
    struct RadiusExtractorImpl : public ShareHandleBase{
        Vect3f minimum_;
        Vect3f maximum_;
        float radius_;
		int count_;
    };

	RadiusExtractor();

	void act(float radius, const Vect3f& v);
	void operator()(BaseUniverseObject& object);

    float radius() const;
    Vect3f center() const;
	int count() const;

    ShareHandle<RadiusExtractorImpl> impl_;
};

struct SelectByObjectBoxTest : UniverseObjectAction{
    SelectByObjectBoxTest(bool& selectionChanged, const Plane* planeClip, bool select, bool deselectOutside, bool selectHidden = false);
    void operator()(BaseUniverseObject& object);

    bool select_;
	bool selectHidden_;
    bool deselectOutside_;
    bool& selectionChanged_;
    const Plane* planeClip_;
};

struct GetSelectionInfo : UniverseObjectAction{
	bool& haveEditor_;
	bool& movable_;
	std::string& text_;

	GetSelectionInfo(bool& haveEditor, bool& movable, std::string& text);

	void operator()(BaseUniverseObject& object);
};

struct EditorCreator : UniverseObjectAction{
	ShareHandle<CSurToolBase>& editor_;

	EditorCreator(ShareHandle<CSurToolBase>& editor);
	void operator()(BaseUniverseObject& unit);
};

typedef std::vector<BaseUniverseObject*> ClonedUnits;
typedef std::vector<BaseUniverseObject*> ClonedSources;

struct Cloner : UniverseObjectAction{
	ClonedUnits& units_;
	ClonedSources& sources_;
	
	Cloner(ClonedUnits& units, ClonedSources& sources)
	: units_(units)
	, sources_(sources)
	{
	};

	void operator()(BaseUniverseObject& object);
};

};

bool forFirstSelected(UniverseObjectAction& action, bool includeDead = false);
void forEachUniverseObject(UniverseObjectAction& action, bool includeDead);
void forEachSelected(UniverseObjectAction& action, bool includeDead = false);

void deselectAll();
void deleteSelectedUniverseObjects();
bool objectBoxTest(const Vect3f& pos, const Plane* box);
bool isObjectFiltered(BaseUniverseObject& object);
void awakePhysics(BaseUniverseObject& object);

bool selectByScreenRectangle(Vect2i p1, Vect2i p2, bool select, bool deselectOutside = false);
bool intersect(const Vect3f& normal, const Vect3f& start, const Vect3f& end, Vect3f* result = 0, bool withBackface = true);

Vect2i worldToScreen(const Vect3f& worldCoords);
Vect3f screenPointToGround(const Vect2i& mouse_pos);
Vect3f projectScreenPointOnPlane (const Vect3f& normal, const Vect3f& offset, const Vect2i& screenPoint);
BaseUniverseObject* unitHoverAll(const Vect2f& pos, bool ignore_non_selectable);

#endif
