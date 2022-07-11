#include "StdAfx.h"
#include "Serialization/SerializationFactory.h"
#include "kdw/ControllableViewport.h"
#include "Render/src/cCamera.h"
#include "Render/src/Scene.h"
#include "ToolPlace.h"
#include "EffectDocument.h"

ToolPlace::ToolPlace()
: tracking_(false)
, lastPosition_(Vect3f::ZERO)
, pressed_(false)
{

}

void ToolPlace::render(kdw::ToolViewport* viewport, kdw::SandboxRenderer* renderer)
{
	if(tracking_){
		Color4c color(255, 255, 255, 128);
		Vect3f pos(globalDocument->worldCenter());
		renderer->drawCircle(Se3f(QuatF::ID, pos), Vect3f::ID * 5, color);
		renderer->drawLine(pos, pos + Vect3f::K * 20.0f, color);
	}
}

bool ToolPlace::onMouseButtonUp(kdw::ToolViewport* viewport, kdw::MouseButton button)
{
	if(button == kdw::MOUSE_BUTTON_LEFT)
		pressed_ = false;
	return false;
}

bool ToolPlace::onMouseButtonDown(kdw::ToolViewport* viewport, kdw::MouseButton button)
{
	if(button == kdw::MOUSE_BUTTON_LEFT){
		if(tracking_){
			globalDocument->setEffectOrigin(lastPosition_);
			globalDocument->onEffectOriginChanged();
			pressed_ = true;
		}
	}
	return false;
}

bool ToolPlace::onMouseMove(kdw::ToolViewport* viewport)
{
	Vect3f v, pos, dir;
	Vect2i pt = viewport->mousePosition();
	Vect2i size = viewport->size();
	viewport->camera()->GetWorldRay(Vect2f(float(pt.x)/size.x - 0.5f, float(pt.y)/size.y - 0.5f), pos, dir);
	if(viewport->scene()->GetTileMap()){
		if(viewport->scene()->TraceDir(pos, dir, &v)){
			lastPosition_ = v - Vect3f(globalDocument->worldCenter());
			tracking_ = true;
		}
		else
			tracking_ = false;
	}
	if(pressed_){
		onMouseButtonDown(viewport, kdw::MOUSE_BUTTON_LEFT);
	}
	return true;
}

namespace kdw{
REGISTER_CLASS(Tool, ToolPlace, "Place Effect")
}
