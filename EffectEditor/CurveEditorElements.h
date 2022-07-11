#ifndef __CURVE_EDITOR_ELEMENTS_HINCLUDED__
#define __CURVE_EDITOR_ELEMENTS_HINCLUDED__

#include "kdw/CurveEditorElement.h"
#include "kdw/Win32/Types.h"
#include "Render/src/NParticleKey.h"
#include "XTL/sigslot.h"

class NodeEmitter;
class NodeEffect;
class NodeCurve;
class CurveWrapperBase;
class EmitterKeyInterface;

class GenerationTimeLineElement;
class GenerationPointElement : public kdw::CurveEditorElement{
public:
	GenerationPointElement();
	void setPosition(Vect2f pos) { position_ = pos; }
	Vect2f position() const{ return position_; }
	void setParent(GenerationTimeLineElement* parent);
	void redraw(HDC dc, const kdw::ViewTransform& view);
	void move(Vect2f delta, const kdw::ViewTransform& view);
	void commit();

	kdw::CurveEditorElement* hitTest(Vect2f position, const kdw::ViewTransform& view);
	GenerationTimeLineElement* parent(){ return parent_; }
protected:
	Vect2f position_;
	GenerationTimeLineElement* parent_;
};

class GenerationTimeLineElement : public kdw::CurveEditorElement{
public:
	static const int OFFSET = 17;
	GenerationTimeLineElement(NodeEmitter* emitter);

	void rebuild();
	void commit();
	void redraw(HDC dc, const kdw::ViewTransform& view);
	kdw::CurveEditorElement* hitTest(Vect2f position, const kdw::ViewTransform& view);
	int pointIndex(GenerationPointElement* point);

	void pointMoved(GenerationPointElement* point);

	bool getBoundingBox(Rectf& rect);

	NodeEmitter* emitter(){ return emitter_; }
protected:
	typedef std::vector<ShareHandle<GenerationPointElement> > Points;
	Points points_;
	NodeEmitter* emitter_;
};

class CurveCKey;
class CurvePoint : public kdw::CurveEditorElement{
public:
	CurvePoint(Vect2f position, CurveCKey* parent);

	void redraw(HDC dc, const kdw::ViewTransform& view);
	void move(Vect2f delta, const kdw::ViewTransform& view);
	void commit();
	void remove();
	kdw::CurveEditorElement* hitTest(Vect2f position, const kdw::ViewTransform& view);
	Vect2f position() const{ return position_; }
	void setPosition(const Vect2f& pos){ position_ = pos; }

	CurveCKey* parent(){ return parent_; }
protected:
	CurveCKey* parent_;
	Vect2f position_;
};

class CurveCKey : public kdw::CurveEditorElement, public sigslot::has_slots{
public:
	CurveCKey(NodeCurve* wrapper, bool active);
	~CurveCKey();

	void rebuild();
	void commit();
	void redraw(HDC dc, const kdw::ViewTransform& view);
	kdw::CurveEditorElement* hitTest(Vect2f position, const kdw::ViewTransform& view);
	void click(Vect2f position, const kdw::ViewTransform& view);

	void pointMoved(CurvePoint* point);
	int pointIndex(CurvePoint* point);
	void moveAllPoints(Vect2f delta, const kdw::ViewTransform& view);

	bool getBoundingBox(Rectf& rect);
	bool active() const{ return active_; }
	NodeEmitter* emitter();
	NodeCurve* node();
	CurveWrapperBase* wrapper();
protected:
	kdw::CurveEditorElement* addPoint(float position);
	void removePoint(CurvePoint* point);
	void selectPoint(CurvePoint* point);

	typedef std::vector<float> InsertedPoints;
	InsertedPoints insertedPoints_;

	typedef std::vector<int> RemovedPoints;
	RemovedPoints removedPoints_;

	typedef std::vector< ShareHandle<CurvePoint> > Points;
	int pointToSelect_;
	bool active_;
	Points points_;
	float length_;
	ShareHandle<NodeCurve> nodeCurve_;
	friend class CurvePoint;
};

class EmitterLifeTimeElement : public kdw::CurveEditorElement{
public:
	EmitterLifeTimeElement(NodeEmitter* nodeEmitter, int index, const char* title);

	void redraw(HDC dc, const kdw::ViewTransform& view);
	void commit();
	void move(Vect2f delta, const kdw::ViewTransform& view);
	bool getBoundingBox(Rectf& rect);
	void rebuild();
	NodeEmitter* node();
	kdw::CurveEditorElement* hitTest(Vect2f position, const kdw::ViewTransform& view);
protected:
	NodeEmitter* nodeEmitter_;

	typedef std::vector<Vect2f> Points;
	std::string name_;
	Points points_;
	Vect2f endPoint_;
	int index_;
	int currentPoint_;
	bool currentEndPoint_;
};

class EffectLifeTimeElement : public kdw::CurveEditorElement{
public:
	EffectLifeTimeElement(NodeEffect* nodeEffect);
	kdw::CurveEditorElement* hitTest(Vect2f position, const kdw::ViewTransform& view);
	void redraw(HDC dc, const kdw::ViewTransform& view);
	bool getBoundingBox(Rectf& rect);
	void rebuild();
protected:
	typedef std::vector<ShareHandle<EmitterLifeTimeElement> > Emitters;
	NodeEffect* nodeEffect_;
	Emitters emitters_;
};
#endif
