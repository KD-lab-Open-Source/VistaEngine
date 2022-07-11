#include "StdAfx.h"

#include "Universe.h"
#include "CameraManager.h"

#include "UI_UnitView.h"
#include "UI_Render.h"
#include "Render\Src\cCamera.h"
#include "Render\Src\Scene.h"
#include "Render\src\VisGeneric.h"

UI_UnitView::UI_UnitView() : scene_(0),
	camera_(0),
	model_(0),
	attribute_(0)
{
	windowPosition_ = Rectf(0,0,0,0);
	needDraw_ = false;
}

UI_UnitView::~UI_UnitView()
{
}

bool UI_UnitView::init()
{
	scene_ = gb_VisGeneric->CreateScene();

	camera_ = scene_->CreateCamera();
	camera_->setAttribute(ATTRCAMERA_PERSPECTIVE);

	Se3f pos;
	pos.trans().set(0,0,1024);
	pos.rot().set(0,1,0,0);
	MatXf CameraMatrix(pos);
	CameraMatrix.Invert();
	camera_->SetPosition(CameraMatrix);
	camera_->setAttribute(ATTRCAMERA_CLEARZBUFFER);

	scene_->SetSunDirection(Vect3f(0,0,-1));
	scene_->SetSunColor(Color4f(0,0,0,1), Color4f(1,1,1,1), Color4f(1,1,1,1));

	return true;
}

bool UI_UnitView::release()
{
	if(model_){
		model_->Release();
		model_ = 0;
	}
	if(camera_){
		camera_->Release();
		camera_ = 0;
	}
	if(scene_){
		scene_->Release();
		scene_ = 0;
	}

	attribute_ = 0;
	needDraw_ = false;

	return true;
}

void UI_UnitView::draw()
{
	if(needDraw_)
		scene_->Draw(camera_);
	needDraw_ = false;
}

bool UI_UnitView::setPosition(const Rectf& pos)
{
	float focus = cameraManager->correctedFocus(1.0f);

	//if(pos != windowPosition_){
		windowPosition_ = pos;
		Rectf rect = UI_Render::instance().relative2deviceCoords(pos) + Vect2f(0.5f, 0.5f);
		camera_->SetFrustum(                         
			&rect.center(), // центр камеры
			&sRectangle4f(-rect.width()/2, -rect.height()/2, rect.width()/2, rect.height()/2), // видимая область камеры
			&Vect2f(focus, focus),                        // фокус камеры
			&Vect2f(30.0f, 10000.0f)                    // ближайший и дальний z-плоскости отсечения
			);

		return true;
	//}

	return false;
}

bool UI_UnitView::setAttribute(const AttributeBase* attribute)
{
	if(attribute != attribute_){
		attribute_ = attribute;

		if(model_){
			model_->Release();
			model_ = 0;
		}
		
		if(attribute_ && attribute_->modelName.empty()) // деревья имеют пустой modelName
			attribute_ = 0;

		if(attribute_){
			const InterfaceTV& tvsetup = attribute_->interfaceTV;
			model_ = scene_->CreateObject3dxDetached(attribute_->modelName.c_str());
				
			float scale = tvsetup.radius()/model_->GetBoundRadius();
			model_->SetScale(scale);

			sBox6f bound;
			model_->GetBoundBox(bound);

			Vect3f pos = Vect3f(tvsetup.position());
			pos.y += (bound.max.z - bound.min.z)/2;

			Vect3f angles = tvsetup.orientation();
			modelPosition_ = MatXf(Mat3f(G2R(angles.x), X_AXIS)*Mat3f(G2R(angles.y), Y_AXIS)*Mat3f(G2R(angles.z), Z_AXIS), pos);
			model_->SetPosition(modelPosition_);
			
			pose_interpolator = Se3f(modelPosition_);
			pose_interpolator.initialize();

			if(universe())
				universe()->activePlayer()->setModelSkinColor(model_);
			
			phase_.initPeriod(tvsetup.chain().reversed, tvsetup.chain().period);
			model_->SetChain(tvsetup.chain().chainIndex());
			
			model_->Attach();
		}
		return true;
	}

	return false;
}

void UI_UnitView::logicQuant(float dt)
{
	if(!isEmpty()){
		modelPosition_.rot() *= Mat3f(M_PI*dt, Z_AXIS);
		pose_interpolator = Se3f(modelPosition_);
		pose_interpolator(model_);
		const InterfaceTV& tvsetup = attribute_->interfaceTV;
		phase_.quant(tvsetup.chain().reversed, true, false);
		interpolator_phase = phase_.phase();
		interpolator_phase(model_, tvsetup.chain().animationGroup());
	}
}
