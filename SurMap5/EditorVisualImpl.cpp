#include "StdAfx.h"
#include "EditorVisual.h"
#include "..\Game\CameraManager.h"
#include "..\Game\Universe.h"
#include "..\Units\ExternalShow.h"
#include "..\Units\Squad.h"
#include "MainFrame.h"
#include "SurMapOptions.h"

#include "EventListeners.h"

namespace EditorVisual{

class Impl : public Interface, public WorldChangedListener
		
{
public:
	Impl();
	~Impl();
	void quant();
	void beforeQuant();
	void afterQuant();

	bool isVisible(UniverseObjectClass objectClass);
	void drawImpassabilityRadius(UnitBase& unit);
	void drawRadius(const Vect3f& position, float radius, RadiusType radiusType, bool selected);
	void drawCross(const Vect3f& position, float size, CrossType crossType, bool selected);
	void drawText(const Vect3f& position, const char* text, TextType textType);
	void drawOrientationArrow(const Se3f& pose, bool selected);

	// virtuals (WorldChangedListener):
	void onWorldChanged();
protected:
	void createAuxUnit();
	void killAuxUnit();
	UnitBase* impassabilityAuxUnit_;
};

Impl::Impl()
{
	static_cast<CMainFrame*>(AfxGetMainWnd())->eventWorldChanged().registerListener(this);
}

Impl::~Impl()
{
}

bool Impl::isVisible(UniverseObjectClass objectClass)
{
	switch(objectClass){
	case UNIVERSE_OBJECT_ANCHOR:
        return surMapOptions.showSources();
	case UNIVERSE_OBJECT_CAMERA_SPLINE:
        return surMapOptions.showCameras();		
	case UNIVERSE_OBJECT_ENVIRONMENT:
		return !surMapOptions.hideWorldModels();
	case UNIVERSE_OBJECT_SOURCE:
		return surMapOptions.showSources();
	case UNIVERSE_OBJECT_UNIT:
		return !surMapOptions.hideWorldModels();
	case UNIVERSE_OBJECT_UNKNOWN:
	default:
		return true;
	}
}

void Impl::createAuxUnit()
{
	killAuxUnit();
	const AttributeBase& attr = *AttributeReference(surMapOptions.showPathFindingReferenceUnit());
	if(&attr){
		impassabilityAuxUnit_ = universe()->worldPlayer()->buildUnit(&attr);
		impassabilityAuxUnit_->setAuxiliary(true);
		if(UnitReal* unitReal = dynamic_cast<UnitReal*>(impassabilityAuxUnit_)){
			unitReal->hide(UnitReal::HIDE_BY_EDITOR, true);
			if(UnitSquad* squad = unitReal->getSquadPoint())
				squad->setAuxiliary(true);
		}
		impassabilityAuxUnit_->setPose(Se3f(QuatF::ID, Vect3f(vMap.H_SIZE / 2.0f, vMap.V_SIZE / 2.0f, 0.0f)), true);
	}
}

void Impl::killAuxUnit()
{
	if(impassabilityAuxUnit_){
		impassabilityAuxUnit_->Kill();
		impassabilityAuxUnit_ = 0;
	}
}

void Impl::onWorldChanged()
{
	//createAuxUnit();
}

void Impl::quant()
{

}

void Impl::beforeQuant()
{
	if(surMapOptions.showPathFinding())
		createAuxUnit();
}

void Impl::afterQuant()
{
	killAuxUnit();
}

void Impl::drawImpassabilityRadius(UnitBase& unit)
{
	if(surMapOptions.showPathFinding() && impassabilityAuxUnit_ && unit.checkInPathTracking(impassabilityAuxUnit_))
		drawRadius(unit.position(), unit.radius(), RADIUS_IMPASSABILITY, true);
	
}

void Impl::drawCross(const Vect3f& position, float size, CrossType crossType, bool selected)
{
	sColor4c color = selected ? RED : GREEN;
	gb_RenderDevice->DrawLine(position + Vect3f(-size, 0.0f, 0.0f), position + Vect3f(size, 0.0f, 0.0f), color);
	gb_RenderDevice->DrawLine(position + Vect3f(0.0f, -size, 0.0f), position + Vect3f(0.0f, size, 0.0f), color);
}

void Impl::drawRadius(const Vect3f& position, float radius, RadiusType radiusType, bool selected)
{
	sColor4c color = selected ? RED : GREEN;
	switch(radiusType){
	case RADIUS_IMPASSABILITY:
		universe()->circleShow()->Circle(position, max(radius, 7.0f), MAGENTA);
		break;
	case RADIUS_OBJECT:
		universe()->circleShow()->Circle(position, max(radius, 7.0f), color);
		break;
	default:
		universe()->circleShow()->Circle(position, max(radius, 7.0f), color);
		break;
	}
}

void Impl::drawText(const Vect3f& position, const char* text, TextType textType)
{
	Vect3f e, w;
	cameraManager->GetCamera()->ConvertorWorldToViewPort(&position, &w, &e);
	sColor4c color;
	switch(textType){
	case TEXT_LABEL:
		color = RED;
		break;
	case TEXT_PROPERTIES:
		color = BLUE;
		e.y += 16;
		break;
	default:
		color = WHITE;
		break;
	}
	gb_RenderDevice->OutText(round(e.x), round(e.y), text, sColor4f(color));
}

void Impl::drawOrientationArrow(const Se3f& pose, bool selected)
{
	//Vect3f pos3d = To3D(Vect2i(round(pose.trans().x), round(pose.trans().y)));
	Vect3f pos3d = pose.trans();
	float scale = 100.0f;

	static const Vect3f points[] = {
		Vect3f ( 0.0f,  0.5f, 0.0f),
		Vect3f ( 0.0f, -0.5f, 0.0f),

		Vect3f ( 0.1f,  0.4f, 0.0f),
		Vect3f ( 0.0f,  0.5f, 0.0f),

		Vect3f (-0.1f,  0.4f, 0.0f),
		Vect3f ( 0.0f,  0.5f, 0.0f),
	};
	for (int i = 1; i < sizeof (points) / sizeof (points[0]); i += 2) {
		Vect3f a = points [i-1] * scale;
		Vect3f b = points [i] * scale;
		pose.rot().xform(a);
		pose.rot().xform(b);
		gb_RenderDevice->DrawLine(pos3d + a, pos3d + b, sColor4c (0, 200, 0));
	}
}

};

EditorVisual::Interface& editorVisual(){
	static EditorVisual::Impl impl;
	return impl;
}


