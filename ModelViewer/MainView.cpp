#include "StdAfx.h"
#include "MainView.h"
#include "kdw/Serialization.h"
#include "kdw/CommandManager.h"
#include "kdw/Navigator.h"

#include "XMath/XMathLib.h"

#include "Render/src/Scene.h"

KDW_REGISTER_CLASS(Widget, MainView, "ModelViewer\\Окно просмотра");

#include "Render/inc/IVisD3D.h"
#include "Render/inc/IRenderDevice.h"
extern RENDER_API cInterfaceRenderDevice* gb_RenderDevice;
extern RENDER_API cVisGeneric*            gb_VisGeneric;

// --------------------------------------------------------------------------- 
//REGISTER_CLASS(NavBase, NavModel, "Модели")

NavRoot::NavRoot(MainDocument* document)
: kdw::NavigatorNodeRoot(document)
{

}

void NavRoot::build()
{
	MainDocument* document = this->document();
	if(cObject3dx* model = document->model()){
		NavModel* navModel = add(new NavModel(model));
		navModel->build();
	}
	if(cObject3dx* modelLogic = document->modelLogic()){
		NavModelLogic* navModel = add(new NavModelLogic(modelLogic));
		navModel->build();
	}
}


// --------------------------------------------------------------------------- 
REGISTER_CLASS(NavBase, NavModel, "Графическая Модель")
REGISTER_CLASS(NavBase, NavModelLogic, "Логическая Модель")

NavModel::NavModel(cObject3dx* model)
: NavBase("")
, model_(model)
{
	setLabel("graph");
}

NavModelLogic::NavModelLogic(cObject3dx* model)
: NavModel(model)
{
	setLabel("logic");
}

void NavModel::build()
{
	setExpanded(true);
	xassert(model_);
	NavNode* node = add(new NavNode(0));
	node->build();
	node->setExpanded(true);
}


// --------------------------------------------------------------------------- 
REGISTER_CLASS(NavBase, NavNode, "Узлы")

NavNode::NavNode(int nodeIndex)
: NavBase("Node")
, nodeIndex_(nodeIndex)
{
	
}

void NavNode::build()
{
	MainDocument* document = getDocument();
	cObject3dx* model = document->model();

	setLabel(model->GetNodeName(nodeIndex_));
	
	int count = model->GetNodeNumber();
	for(int i = 0; i < count; ++i){
		if(model->GetParentNumber(i) == nodeIndex_){
			add(new NavNode(i))->build();
		}
	}
}

// --------------------------------------------------------------------------- 

MainDocument* globalDocument = 0;

MainDocument::MainDocument()
: model_(0)
, modelLogic_(0)
, scene_(0)
, lastTime_(0.0)
, timeFraction_(0.0)
, chainDuration_(1.0f)
{
	lastTime_ = double(xclock());
	if(!gb_VisGeneric){
		gb_RenderDevice = CreateIRenderDevice(false); // тут создается gbVisGeneric
	}
	gb_RenderDevice->AddRef();

	xassert(gb_VisGeneric);
	scene_ = gb_VisGeneric->CreateScene();
 	if(!scene_){
		xassert(0);
		return;
	}
}

MainDocument::~MainDocument()
{
	RELEASE(model_);
	RELEASE(scene_);
	gb_RenderDevice->DecRef();
}

kdw::NavigatorNodeRoot* MainDocument::createRoot()
{
	return new NavRoot(this);
}

void MainDocument::loadModel(const char* filename)
{
	if(model_)
		RELEASE(model_);
	if(modelLogic_)
		RELEASE(modelLogic_);
    model_ = scene_->CreateObject3dx(filename);
	if(model_){
		model_->SetPosition(Se3f(QuatF::ID, Vect3f(0.0f, 0.0f, 0.0f)));
		model_->SetSkinColor(Color4c(255, 0, 0, 255));
		model_->SetAttr(ATTRUNKOBJ_NO_USELOD);
	}
    modelLogic_ = scene_->CreateLogic3dx(filename);
	if(modelLogic_){
		model_->SetPosition(Se3f(QuatF::ID, Vect3f(0.0f, 0.0f, 0.0f)));
	}

	updateChainDuration();
	signalModelChanged_.emit();
	signalChanged_.emit();
}

void MainDocument::updateChainDuration()
{
	if(model_){
		int chainIndex = model_->GetAnimationGroupChain(0);
		cAnimationChain* chain = model_->GetChain(chainIndex);
		chainDuration_ = chain->time;
	}
	else{
		chainDuration_ = 1.0f;
	}
	frame_.Set(chainDuration_ * 1000.0f, 0.0f);
}

void MainDocument::setLODLevel(int level)
{
	if(model_ == 0)
		return;
	gb_VisGeneric->SetUseLod(false);
	model_->SetLod(level);

}

bool MainDocument::isLODEnabled() const
{
	if(model_)
		return model_->GetStatic()->is_lod;
	else
		return false;
}

void MainDocument::onIdle()
{
	int time = xclock();
	double dt = (double)time - (double)lastTime_;
	if(dt < 0 || dt > 1000)
		dt = 0;
	double fraction = timeFraction_ + dt * 1.0; // -1.0
	int intTime = round(fraction);
	frame_.AddPhase(float(intTime));
	scene_->SetDeltaTime(intTime);
	lastTime_ = time;
	timeFraction_ = fraction - intTime;
	if(model_)
		model_->SetPhase(frame_.GetPhase());
}

// ---------------------------------------------------------------------------

MainView::MainView(bool continuousUpdate)
: kdw::Viewport(continuousUpdate)
//, cameraTargetPosition_(0.0f, 0.0f, 0.0f)
, cameraPose_(QuatF::ID, Vect3f(0.0f, 100.0f, 0.0f))
, cameraDistance_(100.0f)

, optionWireframe_(false)
, optionBump_(true)
, optionShadows_(true)
, lod_(0)

, rotating_(false)
, translating_(false)
, zooming_(false)
{
	document()->signalModelChanged().connect(this, &MainView::onModelChanged);

	setFillColor(64, 92, 64);
	camera_ = document()->scene()->CreateCamera();
	camera_->SetAttr(ATTRCAMERA_PERSPECTIVE); // перспектива
	xassert(camera_);
	
	resetCameraPosition();
}

MainView::~MainView()
{
    RELEASE(camera_);
}

void MainView::onInitialize()
{
	transformCamera(0.0f, 0.0f, 0.0f, 1.0f);
}

void MainView::onResize(int width, int height)
{
	Viewport::onResize(width, height);
}

void MainView::onRedraw()
{
	gb_RenderDevice->SetRenderState(RS_FILLMODE, optionWireframe_ ? FILL_WIREFRAME : FILL_SOLID);
	gb_VisGeneric->SetEnableBump(optionBump_);

	cScene* scene = document()->scene();
	cFrame& frame = document()->frame();

	camera_->SetFrustum(&Vect2f(0.5f,0.5f),&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f), &Vect2f(1.f,1.f), &Vect2f(10.0f,10000.0f));
	xassert(document());
	cObject3dx* model = document()->model();
	if(model){
		if(lod_ == 0){
			model->ClearAttr(ATTRUNKOBJ_NO_USELOD);
		}
		else{
			model->SetLod(lod_ - 1);
		}
	}
	scene->Draw(camera_);
}

void MainView::resetCameraPosition()
{
	cameraPose_.rot().set(0.0f, 1.0f, 0.0f, 0.0f);
	cameraPose_.trans().set(1024.0f, 1024.0f, 100.0f);

	transformCamera(0.0f, 0.0f, 0.0f, 1.0f);
	if(renderWindow()){
		renderDevice()->SelectRenderWindow(renderWindow());
		camera_->SetFrustum(&Vect2f(0.5f,0.5f),&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f), &Vect2f(1.f,1.f), &Vect2f(5.0f,3000.0f));
		renderDevice()->SelectRenderWindow(0);
	}
}


void MainView::transformCamera(float deltaPsi, float deltaTheta, float deltaRoll, float dscale)
{	
	if(renderWindow())
		renderDevice()->SelectRenderWindow(renderWindow());
	Vect3f cameraDirection(0.0f, 0.0f, cameraDistance_);
	cameraPose_.rot().xform(cameraDirection);
	Vect3f cameraTarget = cameraPose_.trans() + cameraDirection;
	QuatF rotation;
	rotation.mult(cameraPose_.rot(), QuatF(deltaPsi, Vect3f(0.0f, 1.0f, 0.0f)));
	rotation.mult(rotation, QuatF(deltaTheta, Vect3f(1.0f, 0.0f, 0.0f)));
	cameraPose_.rot().mult(rotation, QuatF(deltaRoll, Vect3f(0.0f, 0.0f, 1.0f)));
	cameraPose_.rot().normalize();
	
	Vect3f rotationOffset(0.0f, 0.0f, cameraDistance_ * dscale);
	cameraPose_.rot().xform(rotationOffset);
	cameraPose_.trans() = cameraTarget - rotationOffset;
	cameraDistance_ *= dscale;

	Se3f pos = cameraPose_;
	MatXf m(pos);
	m.Invert();

	if(renderWindow()){
		camera_->SetPosition(m);
		camera_->SetFrustum(&Vect2f(0.5f,0.5f),&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f), &Vect2f(1.f,1.f), &Vect2f(10.0f,3000.0f));
		renderDevice()->SelectRenderWindow(0);
	}
}



void MainView::onTestLoad()
{
	// "C:\\Projects\\VistaEngine\\Resource\\Models\\R_Antonova.3DX"
	//loadModel("c:\\temp\\tree\\Tree_Leafy_003.3DX");
	//loadModel("C:\\Projects\\VistaEngine\\Resource\\Models\\A_Crusher.3DX");
	//signalModelChanged_.emit();
}

void MainView::onMouseMove()
{
	Vect2i delta = mousePosition() - lastMousePosition_;
	float pixelSize = 1.0f / max(1.0f, min(size().x * 0.5f, size().y * 0.5f));
	if(rotating_){
		float deltaPsi = delta.x * M_PI * pixelSize * 0.5f;
		float deltaTheta = delta.y * M_PI * pixelSize * 0.5f;
		transformCamera(deltaPsi, deltaTheta, 0.0f, 1.0f);
	}
	if(translating_){
		
		Vect3f translationDelta = 0.5f * delta.y * camera_->GetWorldJ() * pixelSize * cameraDistance_
			                    - 0.5f * delta.x * camera_->GetWorldI() * pixelSize * cameraDistance_;
		cameraPose_.trans() += translationDelta;
		
		transformCamera(0.0f, 0.0f, 0.0f, 1.0f);
	}
	if(zooming_){
		transformCamera(0.0f, 0.0f, 0.0f, 1.0f + -pixelSize * delta.y * 0.5f);
	}
	Vect2i mousePos = mousePosition();
	lastMousePosition_.x = mousePos.x;
	lastMousePosition_.y = mousePos.y;
}

void MainView::onMouseButtonDown(kdw::MouseButton button)
{
	if(button == kdw::MOUSE_BUTTON_LEFT){
		rotating_ = true;
	}
	else if(button == kdw::MOUSE_BUTTON_MIDDLE){
		translating_ = true;
	}
	else if(button == kdw::MOUSE_BUTTON_RIGHT){
		zooming_ = true;
	}

	if(rotating_ || translating_ || zooming_)
		captureMouse();
}

void MainView::onMouseButtonUp(kdw::MouseButton button)
{
	if(button == kdw::MOUSE_BUTTON_LEFT){
		rotating_ = false;
	}
	else if(button == kdw::MOUSE_BUTTON_MIDDLE){
		translating_ = false;
	}
	else if(button == kdw::MOUSE_BUTTON_RIGHT){
		zooming_ = false;
	}

	if(!rotating_ && !translating_ && !zooming_)
		releaseMouse();
}

void MainView::onModelChanged()
{
	transformCamera(0.0f, 0.0f, 0.0f, 1.0f);
}


void MainView::setOptionWireframe(bool wireframe)
{
	optionWireframe_ = wireframe;
}

void MainView::setOptionBump(bool bump)
{
	optionBump_ = bump;
}

void MainView::setOptionShadows(bool shadows)
{
	optionShadows_ = shadows;
}

bool MainView::hasLod(int lod)
{
	if(document()){
		if(cObject3dx* model = document()->model()){
			return model->GetStatic()->lods.size() >= lod;
		}
	}
    return false;
}

void MainView::setLod(int lod)
{
	lod_ = lod;
}

void MainView::serializeState(Archive& ar)
{
	ar.serialize(cameraPose_, "cameraPose", 0);
    ar.serialize(cameraDistance_, "cameraDistance", 0);

	ar.serialize(optionWireframe_, "wireframe", 0);
	ar.serialize(optionBump_, "bump", 0);
	ar.serialize(optionShadows_, "shadows", 0);
	ar.serialize(lod_, "lod", 0);

	if(ar.isInput())
		transformCamera(0.0f, 0.0f, 0.0f, 1.0f);
}


// ---------------------------------------------------------------------------

MainViewSpace::MainViewSpace()
: view_(0)
{

	view_ = new MainView(true);
	view_->document()->signalModelChanged().connect(this, &Self::updateMenus);

	updateMenus();
	setMenu("view");
		
	add(view_);
}

MainViewSpace::~MainViewSpace()
{
}

void MainViewSpace::updateMenus()
{
	commands().add("view.render.wireframe")
		.connect(this, &Self::onMenuRenderOption)
		.check(view_->optionWireframe());
	commands().add("view.render.bump")
		.connect(this, &Self::onMenuRenderOption)
		.check(view_->optionBump());
	commands().add("view.render.shadows")
		.connect(this, &Self::onMenuRenderOption)
		.check(view_->optionShadows());
	commands().add("view.lod.auto")
		.connect(this, &Self::onMenuLod, 0)
		.check(view_->lod() == 0);
	commands().add("view.lod.1")
		.connect(this, &Self::onMenuLod, 1)
		.check(view_->lod() == 1)
		.enable(view_->hasLod(1));
	commands().add("view.lod.2")
		.connect(this, &Self::onMenuLod, 2)
		.check(view_->lod() == 2)
		.enable(view_->hasLod(2));
	commands().add("view.lod.3")
		.connect(this, &Self::onMenuLod, 3)
		.check(view_->lod() == 3)
		.enable(view_->hasLod(3));
}

void MainViewSpace::serialize(Archive& ar)
{
	Space::serialize(ar);
	view_->serializeState(ar);
	if(ar.isInput())
		updateMenus();
}

void MainViewSpace::onMenuRenderOption()
{
	view_->setOptionWireframe(commands().add("view.render.wireframe").checked());
	view_->setOptionBump(commands().add("view.render.bump").checked());
	view_->setOptionShadows(commands().add("view.render.shadows").checked());
}

void MainViewSpace::onMenuLod(int lod)
{
	commands().add("view.lod.auto").check(lod == 0);
	commands().add("view.lod.1").check(lod == 1);
	commands().add("view.lod.2").check(lod == 2);
	commands().add("view.lod.3").check(lod == 3);

	view_->setLod(lod);
}

KDW_REGISTER_CLASS(Space, MainViewSpace, "3D Вид")

KDW_REGISTER_CLASS(Space, NavigatorSpace, "Навигатор")
