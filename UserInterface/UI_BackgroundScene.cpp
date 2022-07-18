#include "stdafx.h"

#include "Serialization.h"
#include "ResourceSelector.h"
#include "Rect.h"

#include "..\Render\src\Scene.h"
#include "..\Render\src\cCamera.h"
#include "..\Render\3dx\Node3dx.h"

#include "..\Environment\Environment.h"
#include "..\Game\Universe.h"
#include "..\Game\Player.h"
#include "CameraManager.h"

#include "UI_Render.h"
#include "UI_BackgroundScene.h"

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(UI_BackgroundAnimation, PlayMode, "UI_BackgroundAnimation::PlayMode")
REGISTER_ENUM_ENCLOSED(UI_BackgroundAnimation, PLAY_STARTUP, "при появлении")
REGISTER_ENUM_ENCLOSED(UI_BackgroundAnimation, PLAY_PERMANENT, "постоянно")
REGISTER_ENUM_ENCLOSED(UI_BackgroundAnimation, PLAY_HOVER_STARTUP, "появление при наведении мыши")
REGISTER_ENUM_ENCLOSED(UI_BackgroundAnimation, PLAY_HOVER_PERMANENT, "постоянно при наведении мыши")
END_ENUM_DESCRIPTOR_ENCLOSED(UI_BackgroundAnimation, PlayMode)

// ------------------------------- UI_BackgroundModelSetup

UI_BackgroundModelSetup::UI_BackgroundModelSetup() :
	skinColor_(1.0f, 1.0f, 1.0f, 1.0f)
{
	useOwnColor_ = true;
	useEmblem_ = true;
}

void UI_BackgroundModelSetup::serialize(Archive& ar)
{
	ar.serialize(ModelSelector(modelName_, ModelSelector::DEFAULT_OPTIONS), "fileName", "Модель");

	ar.serialize(useOwnColor_, "useOwnColor", "Собственный цвет легиона");
	if(useOwnColor_)
		ar.serialize(skinColor_, "skinColor", "Цвет");

	ar.serialize(useEmblem_, "useEmblem", "Использовать эмблему легиона");
 
	if(ar.isInput() && ::isUnderEditor())
		updateComboLists();
}

void UI_BackgroundModelSetup::updateComboLists()
{
	groupComboList_ = "";
	chainComboList_ = "";

	if(modelName_.empty())
		return;

	cScene* scene = gb_VisGeneric->CreateScene();
	cObject3dx* model = scene->CreateObject3dx(modelName_.c_str());
	if(model){
		string comboList;
		int number = model->GetAnimationGroupNumber();
		int i;
		for(i = 0; i < number; i++){
			if(!comboList.empty())
				comboList += "|";
			comboList += model->GetAnimationGroupName(i);
		}
		groupComboList_ = comboList;

		comboList = "";
		number = model->GetChainNumber();
		for(i = 0; i < number; i++){
			if(!comboList.empty())
				comboList += "|";
			comboList += model->GetChain(i)->name;
		}
		chainComboList_ = comboList;
		model->Release();
	}

	scene->Release();
}

void UI_BackgroundModelSetup::preLoad(cScene* scene, const Player* player) const
{
	xassert(scene);
	
	cObject3dx* model = scene->CreateObject3dx(modelName(), NULL);
	xassert(model);
	// @dilesoft
	if (!model) {
		return;
	}

	model->DisableDetailLevel();
	
	if(player){
		sColor4f color(ownSkinColor() ? skinColor() : player->unitColor());
		model->SetSkinColor(color, useEmblem() ? player->unitSign() : 0);
	}
	else
		model->SetSkinColor(skinColor(), 0);

	RELEASE(model);
}

// ------------------------------- UI_BackgroundModel

UI_BackgroundModel::UI_BackgroundModel() : model_(0)
{

}

void UI_BackgroundModel::load(cScene* scene, const UI_BackgroundModelSetup& setup, const Player* player)
{
	MTG();
	release();

	if(setup.isEmpty())
		return;

	model_ = scene->CreateObject3dx(setup.modelName(), NULL);
	model_->DisableDetailLevel();
	if(player){
		sColor4f color(setup.ownSkinColor() ? setup.skinColor() : player->unitColor());
		model_->SetSkinColor(color, setup.useEmblem() ? player->unitSign() : 0);
	}
	else
		model_->SetSkinColor(setup.skinColor(), 0);

}

void UI_BackgroundModel::release()
{
	if(model_){
		model_->Release();
		model_ = 0;
	}

	MTG();
	animations_.clear();
}

void UI_BackgroundModel::setPosition(const MatXf& pos)
{
	MTG();
	if(model_)
		model_->SetPosition(pos);
}

bool UI_BackgroundModel::isPlaying() const
{
	MTG();
	return !animations_.empty();
}

bool UI_BackgroundModel::isPlaying(UI_BackgroundAnimation::PlayMode mode) const
{
	MTG();
	for(UI_BackgroundAnimations::const_iterator it = animations_.begin(); it != animations_.end(); ++it){
		if(it->playMode() == mode)
			return true;
	}

	return false;
}

bool UI_BackgroundModel::isPlaying(const UI_BackgroundAnimation& animation) const
{
	MTG();
	return (std::find(animations_.begin(), animations_.end(), animation) != animations_.end());
}
	
bool UI_BackgroundModel::play(const UI_BackgroundAnimation& animation, bool reverse)
{
	MTG();
	if(!model_ || animation.duration() < FLT_EPS) return false;

	UI_BackgroundAnimations::iterator it = std::find(animations_.begin(), animations_.end(), animation);

	if(it != animations_.end()){
		bool reverse_mode = reverse ? !animation.reversed() : animation.reversed();
		if(it->reversed() != reverse_mode){
			it->setReversed(reverse_mode);
			it->phaseReverse();
		}

		int groupIndex = model_->GetAnimationGroup(it->animationGroupName());
		if(groupIndex < 0){
			xassertStr(0, XBuffer() < "В анимационной цепочке контрола указанна несуществующая группа: " < it->animationGroupName());
			return false;
		}
		model_->SetAnimationGroupPhase(groupIndex, it->phase());
		return true;
	}

	animations_.push_back(animation);
	if(reverse)
		animations_.back().setReversed(!animation.reversed());

	animations_.back().reset();
	
	int groupIndex = model_->GetAnimationGroup(animations_.back().animationGroupName());
	if(groupIndex < 0){
		xassertStr(0, XBuffer() < "В анимационной цепочке контрола указанна несуществующая группа: " < animations_.back().animationGroupName());
		return false;
	}

	animations_.back().setAnimationGroupIndex(groupIndex);

	model_->SetAnimationGroupChain(groupIndex, animations_.back().chainName());
	model_->SetAnimationGroupPhase(groupIndex, animations_.back().phase());

	return true;
}

bool UI_BackgroundModel::stop(const UI_BackgroundAnimation& animation)
{
	MTG();
	UI_BackgroundAnimations::iterator it = std::find(animations_.begin(), animations_.end(), animation);

	if(it != animations_.end()){
		animations_.erase(it);
		return true;
	}

	return false;
}


void UI_BackgroundModel::quant(float dt)
{
	MTG();
	if(!model_) return;	

	for(UI_BackgroundAnimations::iterator it = animations_.begin(); it != animations_.end(); ++it){
		it->quant(dt);
		int animationGroup = it->animationGroupIndex();
		if(animationGroup >= 0){
			model_->SetAnimationGroupChain(animationGroup, it->chainName());
			model_->SetAnimationGroupPhase(animationGroup, it->phase());
		}
	}

/*	for(UI_BackgroundAnimations::reverse_iterator it1 = animations_.rbegin(); it1 != animations_.rend(); ++it1){
		UI_BackgroundAnimations::reverse_iterator it2 = it1;
		--it2;
		for(; it2 != animations_.rend(); ++it2){
			if(it2->animationGroupIndex() == it1->animationGroupIndex())
				it2->reset();
		}

	}*/

	animations_.erase(remove_if(animations_.begin(), animations_.end(),
		std::mem_fun_ref(&UI_BackgroundAnimation::isFinished)), animations_.end());
}

void UI_BackgroundModel::getDebugInfo(XBuffer& buf) const
{
	MTG();
	for(UI_BackgroundAnimations::const_iterator it = animations_.begin(); it != animations_.end(); ++it){
		buf < it->animationGroupName() < " / " < it->chainName() < " " <= it->phase() < "\n";
	}
}

// ------------------------------- UI_BackgroundLight

UI_BackgroundLight::UI_BackgroundLight()
{
	color_ = sColor4f(1,1,1,1);
	radius_ = 100.f;

	lifeTime_ = 0.5f;
}

void UI_BackgroundLight::serialize(Archive& ar)
{
	ar.serialize(lifeTime_, "lifeTime", "Время жизни");
	ar.serialize(radius_, "radius", "Радиус");
	ar.serialize(color_, "color", "Цвет");
}

// ------------------------------- UI_BackgroundLightController

UI_BackgroundLightController::UI_BackgroundLightController() : light_(0)
{
}

bool UI_BackgroundLightController::start(const UI_BackgroundLight* prm, const Vect3f& position)
{
	release();

	if(light_ = UI_BackgroundScene::instance().scene()->CreateLightDetached(ATTRLIGHT_SPHERICAL_OBJECT)){
		light_->SetDiffuse(prm->color());
		light_->SetRadius(prm->radius());
		light_->SetPosition(MatXf(Mat3f::ID, position));
		light_->Attach();
	}

	lifeTime_ = max(0.1f, prm->lifeTime());

	return true;
}

bool UI_BackgroundLightController::quant(float dt)
{
	lifeTime_ -= dt;
	if(lifeTime_ < 0.f){
		release();
		return false;
	}

	return true;
}

void UI_BackgroundLightController::release()
{
	if(light_){
		light_->Release();
		light_ = 0;
	}
}

// ------------------------------- UI_BackgroundAnimation

UI_BackgroundAnimation::UI_BackgroundAnimation()
{
	animationGroupIndex_ = -1;

	playMode_ = PLAY_STARTUP;

	reversed_ = false;
	duration_ = 0.f;
	phase_ = 0.f;
}

void UI_BackgroundAnimation::serialize(Archive& ar)
{
	if(!ar.isEdit()){
		ar.serialize(animationGroupName_, "animationGroupName", "Анимационная группа");
		ar.serialize(chainName_, "chainName", "Цепочка");
	}
	else {
		ComboListString group_str(UI_BackgroundScene::instance().groupComboList() ? UI_BackgroundScene::instance().groupComboList() : "", animationGroupName_.c_str());
		ar.serialize(group_str, "animationGroupName", "Анимационная группа");
		ComboListString chain_str(UI_BackgroundScene::instance().chainComboList() ? UI_BackgroundScene::instance().chainComboList() : "", chainName_.c_str());
		ar.serialize(chain_str, "chainName", "Цепочка");

		if(ar.isInput()){
			animationGroupName_ = group_str;
			chainName_ = chain_str;
		}
	}

	ar.serialize(playMode_, "playMode", "Режим проигрывания");
	ar.serialize(duration_, "duration", "Длительность");
	ar.serialize(reversed_, "reversed", "Проигрывать в обратную сторону");
}

void UI_BackgroundAnimation::quant(float dt)
{
	MTG();
	if(duration_ > FLT_EPS)
		phase_ += dt / duration_;

	if(isCycled())
		phase_ = cycle(phase_, 1.f);
	else
		phase_ = clamp(phase_, 0.f, 1.f);
}

// ------------------------------- UI_BackgroundScene

UI_BackgroundScene::UI_BackgroundScene() :
	scene_(0),
	camera_(0),
	enabled_(true),
	cameraPosition_(0.0f, 0.0f, 1024.f),
	cameraAngles_(-180.0f, 0.0f, 0.0f),
	modelAngles_(90.0f, 0.0f, 0.0f),
	lightDirection_(0.f, 2.5f, -2.5f)
{
	cameraFocus_ = 1.f;
	currentModelIndex_ = -1;
	cameraPerspective_ = true;
}

UI_BackgroundScene::~UI_BackgroundScene()
{
}

void UI_BackgroundScene::init(cVisGeneric* visGeneric)
{
	if(inited())
		done();

	scene_ = visGeneric->CreateScene();

	camera_ = scene_->CreateCamera();

	if(cameraPerspective_)
		camera_->SetAttr(ATTRCAMERA_PERSPECTIVE);

	camera_->SetAttr(ATTRCAMERA_CLEARZBUFFER);
	camera_->SetAttr(ATTRCAMERA_NOCLEARTARGET);

	setCamera();
	
	scene_->SetSunDirection(lightDirection_);

	models_.resize(modelSetups_.size());

	if(currentModelIndex_ != -1)
		models_[currentModelIndex_].load(scene_, modelSetups_[currentModelIndex_], universe() ? universe()->activePlayer() : 0);
}

void UI_BackgroundScene::setRenderTarget(cTexture* renderTarget, IDirect3DSurface9* depthBuffer)
{
	xassert(camera_ && renderTarget && depthBuffer);
	camera_->SetRenderTarget(renderTarget, depthBuffer);
}

void UI_BackgroundScene::setCamera()
{
	if(!camera_)
		return;

	Rectf pos;
	float focus;

	cTexture* renderTarget = camera_->GetRenderTarget();
	int renderWidth;
	int renderHeight;
	if(renderTarget){
		renderWidth = renderTarget->GetWidth();
		renderHeight = renderTarget->GetHeight();
	}
	else{
		renderWidth = gb_RenderDevice->GetSizeX();
		renderHeight = gb_RenderDevice->GetSizeY();
	}
	float aspect = float(renderWidth) / float(renderHeight);
	if(aspect > 4.0f / 3.0f){
		pos = UI_Render::instance().deviceCoords(Rectf(0,0, renderWidth, renderHeight));
		focus = cameraManager->correctedFocus(cameraFocus_, camera_);
	}
	else{
		pos = UI_Render::instance().deviceCoords(UI_Render::instance().windowPosition());
		focus = cameraFocus_;
	}

	MatXf cameraMatrix(Mat3f(G2R(cameraAngles_.x), X_AXIS) * Mat3f(G2R(cameraAngles_.y), Y_AXIS) *
		Mat3f(G2R(cameraAngles_.z), Z_AXIS), cameraPosition_);

	cameraMatrix.Invert();

	camera_->SetPosition(cameraMatrix);

	Vect2f size(pos.width(), pos.height());

	float descale_x = max(1.0f, size.x + 0.0001f);
    float descale_y = max(1.0f, size.y + 0.0001f);

	Vect2f center = pos.center() + Vect2f(0.5f, 0.5f);
	sRectangle4f clip(-size.x / 2 / descale_x, -size.y / 2 / descale_y, 
		               size.x / 2 / descale_x,  size.y / 2 / descale_y);

	camera_->SetFrustum(&center, &clip, &Vect2f(size.x * focus, size.x * focus), 0);
}

void UI_BackgroundScene::setSky(cTexture* texture)
{
	MTG();
	if(scene_)
		scene_->SetSkyCubemap(texture);
}

void UI_BackgroundScene::done()
{
	MTL();
	
	reset();
	RELEASE(camera_);

	for(UI_BackgroundModels::iterator it = models_.begin(); it != models_.end(); ++it)
		it->release();

	for(LightControllers::iterator il = lightControllers_.begin(); il != lightControllers_.end(); ++il)
		il->release();

	models_.clear();
	lightControllers_.clear();

	RELEASE(scene_);
}

bool UI_BackgroundScene::ready() const
{
	return (enabled_ && inited());
}

void UI_BackgroundScene::logicQuant(float dt) 
{
	MTL();

	for(LightControllers::iterator il = lightControllers_.begin(); il != lightControllers_.end();){
		if(!il->quant(dt))
			il = lightControllers_.erase(il);
		else
			++il;
	}
}

void UI_BackgroundScene::graphQuant(float dt) 
{
	if(!enabled_) return;	

	MTG();

	start_timer_auto();

	if(environment){

		scene_->SetSunColor(environment->scene()->GetSunAmbient(),
			environment->scene()->GetSunDiffuse(), environment->scene()->GetSunSpecular());
		scene_->SetSunDirection(environment->scene()->GetSunDirection());
	}

	for(UI_BackgroundModels::iterator it = models_.begin(); it != models_.end(); ++it){
		it->quant(dt);
		if(it->isLoaded() && !it->isPlaying(UI_BackgroundAnimation::PLAY_STARTUP) && &*it != currentModel())
			it->release();
	}

	scene_->SetDeltaTime(dt*1000.f);
}

bool UI_BackgroundScene::play(const UI_BackgroundAnimation& animation, bool reverse)
{
	if(UI_BackgroundModel* model = currentModel())
		return model->play(animation, reverse);

	return false;
}

bool UI_BackgroundScene::stop(const UI_BackgroundAnimation& animation)
{
	if(UI_BackgroundModel* model = currentModel())
		return model->stop(animation);

	return false;
}

bool UI_BackgroundScene::isPlaying() const
{
	if(const UI_BackgroundModel* model = currentModel())
		return model->isPlaying();

	return false;
}

bool UI_BackgroundScene::isPlaying(UI_BackgroundAnimation::PlayMode mode) const
{
	if(const UI_BackgroundModel* model = currentModel())
		return model->isPlaying(mode);

	return false;
}

bool UI_BackgroundScene::isPlaying(const UI_BackgroundAnimation& animation) const
{
	if(const UI_BackgroundModel* model = currentModel())
		return model->isPlaying(animation);

	return false;
}

bool UI_BackgroundScene::addLight(int light_index, const Vect2f& position)
{
	MTL();

	if(!ready())
		return false;

	lightControllers_.push_back(UI_BackgroundLightController());

	Vect3f pos;
	camera_->ConvertorCameraToWorld(&position, &pos);

	if(light_index >= 0 && light_index < lights_.size())
		return lightControllers_.back().start(&lights_[light_index], pos);

	UI_BackgroundLight light;
	return lightControllers_.back().start(&light, pos);
}

void UI_BackgroundScene::reset()
{
	for(UI_BackgroundModels::iterator it = models_.begin(); it != models_.end(); ++it)
		it->reset();

//	currentModelIndex_ = -1;
}

int UI_BackgroundScene::getModelIndex(const char* modelName) const
{
	UI_BackgroundModelSetups::const_iterator it = std::find(modelSetups_.begin(), modelSetups_.end(), modelName);
	return it != modelSetups_.end() ? std::distance(modelSetups_.begin(), it) : -1;
}

bool UI_BackgroundScene::selectModel(const char* model_name, bool need_load)
{
	if(!model_name || !*model_name){
		currentModelIndex_ = -1;
		return false;
	}

	currentModelIndex_ = getModelIndex(model_name);

	if(currentModelIndex_ >= 0){
		if(need_load && !models_[currentModelIndex_].isLoaded()){
			models_[currentModelIndex_].load(scene_, modelSetup(currentModelIndex_), universe() ? universe()->activePlayer() : 0);
			scene_->HideAllObjectLights(false);

			MatXf pos(Mat3f(G2R(modelAngles_.x), X_AXIS) * Mat3f(G2R(modelAngles_.y), Y_AXIS) *
				Mat3f(G2R(modelAngles_.z), Z_AXIS), Vect3f::ZERO);

			models_[currentModelIndex_].setPosition(pos);
		}

		return true;
	}

	return false;
}

void UI_BackgroundScene::draw() const
{
	xassert(scene_);
	start_timer_auto();
	scene_->Draw(camera_);
}

void UI_BackgroundScene::serialize(Archive& ar)
{
	ar.serialize(enabled_, "enabled", "Активна");

	ar.serialize(modelSetups_, "models", "Модели");

	if(ar.isInput())
		updateModelComboList();

	ar.serialize(lightDirection_, "lightDirection", "Направление освещения");
	ar.serialize(lights_, "lights", "Источники света");

	ar.openBlock("camera", "Камера");

	ar.serialize(cameraPosition_, "position", "Позиция");
//	ar.serialize(cameraAngles_, "angles", "Поворот");
	ar.serialize(cameraFocus_, "focusx", "Фокус");
	ar.serialize(cameraPerspective_, "perspective", "Перспектива");

	ar.closeBlock();
}

const char* UI_BackgroundScene::groupComboList() const
{
	if(const UI_BackgroundModelSetup* setup = currentModelSetup())
		return setup->groupComboList();

	return 0;
}

const char* UI_BackgroundScene::chainComboList() const
{
	if(const UI_BackgroundModelSetup* setup = currentModelSetup())
		return setup->chainComboList();

	return 0;
}

void UI_BackgroundScene::drawDebug2D() const
{
	if(showDebugInterface.background){
		XBuffer buf(1024);

		for(UI_BackgroundModels::const_iterator it = models_.begin(); it != models_.end(); ++it){
			if(it->isPlaying()){
				buf < "UI_BackgroundScene:\n";
				it->getDebugInfo(buf);
				buf < "----------------------------\n";
			}
		}

		UI_Render::instance().outDebugText(Vect2f(0.01f, 0.2f), buf);
	}
}

void UI_BackgroundScene::updateModelComboList()
{
	modelComboList_.clear();

	for(UI_BackgroundModelSetups::iterator it = modelSetups_.begin(); it != modelSetups_.end(); ++it){
		modelComboList_ += "|";
		modelComboList_ += it->modelName();
	}
}

