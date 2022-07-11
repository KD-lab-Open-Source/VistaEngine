#ifndef __UI_BACKGROUND_SCENE_H__
#define __UI_BACKGROUND_SCENE_H__

#include "XMath\Colors.h"
#include "Handle.h"
#include "UI_Effect.h"
#include "XTL\SwapVector.h"

class cScene;
class Camera;
class cTexture;
class cObject3dx;
class cVisGeneric;
class cUnkLight;
class Player;

class Archive;

class UI_BackgroundAnimation
{
public:
	UI_BackgroundAnimation();

	/// режимы проигрывания анимации
	enum PlayMode {
		/// при заходе на экран, выходе с экрана, при появлении и исчезновении кнопок
		PLAY_STARTUP,
		/// всё время в фоне
		PLAY_PERMANENT,
		/// при наведении мыши
		PLAY_HOVER_STARTUP,
		/// постоянно при наведении мыши
		PLAY_HOVER_PERMANENT
	};

	void serialize(Archive& ar);

	const char* animationGroupName() const { return animationGroupName_.c_str(); }
	const char* chainName() const { return chainName_.c_str(); }

	const UI_EffectAttributeAttachable& effect() const { return effect_; }

	bool isCycled() const { return playMode_ == PLAY_PERMANENT || playMode_ == PLAY_HOVER_PERMANENT; }
	PlayMode playMode() const { return playMode_; }
	bool reversed() const { return reversed_; }
	float duration() const { return duration_; }

private:
	PlayMode playMode_;

	std::string animationGroupName_;
	std::string chainName_;

	float duration_; // Длительность (в с)
	bool reversed_;	// Проигрывать в прямом или обратном порядке

	UI_EffectAttributeAttachable effect_;
};

typedef SwapVector<UI_BackgroundAnimation> UI_BackgroundAnimations;

class UI_BackgroundAnimationController
{
public:
	UI_BackgroundAnimationController(const UI_BackgroundAnimation* animation);
	void setAnimation(const UI_BackgroundAnimation* animation);

	const UI_BackgroundAnimation* attr() const { return attr_; }
	
	bool operator == (const UI_BackgroundAnimation* animation) const { return attr_ == animation; }

	float phase() const { return (reversed_) ? (1.f - phase_) : phase_; }
	void setPhase(float phase){ phase_ = (reversed_) ? (1.f - phase) : phase; }
	void phaseReverse(){ phase_ = 1.f - phase_; }
	void reset(){ phase_ = 0.f; }

	int animationGroupIndex() const { return animationGroupIndex_; }
	void setAnimationGroupIndex(int index){ animationGroupIndex_ = index; }

	bool reversed() const { return reversed_; }
	void setReversed(bool reversed){ reversed_ = reversed; }

	bool isFinished() const { xassert(attr_); return !attr_->isCycled() && (1.f - phase_ < FLT_EPS); }

	void quant(float dt);
private:
	const UI_BackgroundAnimation* attr_;
	int animationGroupIndex_;

	bool reversed_;
	float phase_;
};

typedef SwapVector<UI_BackgroundAnimationController> UI_BackgroundAnimationControllers;

class UI_BackgroundModelSetup
{
public:
	UI_BackgroundModelSetup();

	bool operator == (const char* name) const { return !stricmp(modelName_.c_str(), name); }

	bool isEmpty() const { return modelName_.empty(); }
	const char* modelName() const { return modelName_.c_str(); }
	
	bool ownSkinColor() const { return useOwnColor_; }
	const Color4f& skinColor() const { return skinColor_; }
	bool useEmblem() const { return useEmblem_; }

	float scale() const { return scale_; }
	void setScale(float _scale) { scale_ = _scale; }

	void serialize(Archive& ar);

	void preLoad(cScene* scene, const Player* player) const;
	const char* groupComboList() const { return groupComboList_.c_str(); }
	const char* chainComboList() const { return chainComboList_.c_str(); }
	const char* nodeComboList() const { return nodeComboList_.c_str(); }

private:

	std::string modelName_;
	bool useOwnColor_;
	bool useEmblem_;
	Color4f skinColor_;

	float scale_;

	std::string groupComboList_;
	std::string chainComboList_;
	std::string nodeComboList_;

	void updateComboLists();
};

typedef std::vector<UI_BackgroundModelSetup> UI_BackgroundModelSetups;

class UI_BackgroundModel
{
public:
	UI_BackgroundModel();

	void load(cScene* scene, const UI_BackgroundModelSetup& setup, const Player* player);
	bool isLoaded() const { return (model_ != 0); }
	void release();

	void setPosition(const MatXf& pos);

	bool isPlaying() const;
	bool isPlaying(UI_BackgroundAnimation::PlayMode mode) const;
	bool isPlaying(const UI_BackgroundAnimation* animation) const;
	
	//! Запустить анимацию объекта с начала, если она еще не запущена
	bool play(const UI_BackgroundAnimation* animation, bool reverse = false);
	//! Остановить анимацию объекта в текущем состоянии
	bool stop(const UI_BackgroundAnimation* animation);

	void quant(float dt);

	void drawDebugInfo(Camera* camera) const;
	void getDebugInfo(Camera* camera, XBuffer& buf) const;

	const cObject3dx* model() const { return model_; }

private:
	void startEffect(const UI_EffectAttributeAttachable* attr);
	void stopEffect(const UI_EffectAttributeAttachable* attr);

	cObject3dx* model_;
	UI_BackgroundAnimationControllers animations_;

	typedef SwapVector<UI_EffectControllerAttachable3D> Effects;
	Effects effects_;
};

typedef std::vector<UI_BackgroundModel> UI_BackgroundModels;

class UI_BackgroundLight
{
public:
	UI_BackgroundLight();

	void serialize(Archive& ar);

	float lifeTime() const { return lifeTime_; }

	float radius() const { return radius_; }
	const Color4f& color() const { return color_; }

private:

	float lifeTime_;

	float radius_;
	Color4f color_;
};

class UI_BackgroundLightController
{
public:
	UI_BackgroundLightController();

	bool start(const UI_BackgroundLight* prm, const Vect3f& position);
	/// возвращает true если источник живой
	bool quant(float dt);

	void release();

private:

	float lifeTime_;
	cUnkLight* light_;
};

struct IDirect3DSurface9;

class UI_BackgroundScene
{
public:
	UI_BackgroundScene();
	~UI_BackgroundScene();

	void serialize(Archive& ar);

	void init(cVisGeneric* visGeneric);
	void setCamera();
	void setFocus(float focus);
	float focus() const { return cameraFocus_; }
	void setRenderTarget(cTexture* renderTarget, IDirect3DSurface9* depthBuffer);
	void setSky(cTexture* texture);

	void done();
	bool ready() const;

	void logicQuant(float dt);
	void graphQuant(float dt);
	void draw() const;

	int getModelIndex(const char* modelName) const;
	const UI_BackgroundModelSetup& modelSetup(int idx) const { xassert(idx >= 0 && idx < modelSetups_.size()); return modelSetups_[idx]; }

	void selectModel(const char* model_name);

	bool isPlaying() const;
	bool isPlaying(UI_BackgroundAnimation::PlayMode mode) const;
	bool isPlaying(const UI_BackgroundAnimation* animation) const;

	bool addLight(int light_index, const Vect2f& position);
	
	//! Запустить анимацию объекта с начала, если она еще не запущена
	bool play(const UI_BackgroundAnimation* animation, bool reverse = false);
	//! Остановить анимацию объекта в текущем состоянии
	bool stop(const UI_BackgroundAnimation* animation);

	bool startEffect(const UI_EffectAttribute* attr, const class UI_ControlBase* owner);
	void stopEffect(const UI_ControlBase* owner, bool immediately = false);

	void setEnabled(bool state) { enabled_ = state; }
	bool inited() const { return (scene_ != 0); }

	const char* groupComboList() const;
	const char* chainComboList() const;
	const char* nodeComboList() const;

	const char* modelComboList() const { return modelComboList_.c_str(); }

	cScene* scene() const { return scene_; }
	Camera* camera() const { return camera_; }

	void drawDebugInfo() const;
	void drawDebug2D() const;

	static UI_BackgroundScene& instance(){ return Singleton<UI_BackgroundScene>::instance(); }

	const UI_BackgroundModel* currentModel() const
	{
		if(currentModelIndex_ != -1)
			return &models_[currentModelIndex_];

		else return 0;
	}

private:

	cScene* scene_;
	Camera* camera_;

	bool enabled_;

	int currentModelIndex_;

	UI_BackgroundModelSetups modelSetups_;
	UI_BackgroundModels models_;

	Vect3f cameraPosition_;
	Vect3f cameraAngles_;
	Vect3f modelPosition_;	
	Vect3f modelAngles_;
	bool cameraPerspective_;
	
	const static float scale2focus;
	float cameraFocus_;

	std::string modelComboList_;

	typedef std::vector<UI_BackgroundLight> Lights;
	Lights lights_;

	Vect3f lightDirection_;

	typedef SwapVector<UI_BackgroundLightController> LightControllers;
	LightControllers lightControllers_;

	typedef SwapVector<UI_EffectControllerAttachable2D> UI_Effects;
	UI_Effects effects_;

	void updateModelComboList();

	UI_BackgroundModel* currentModel()
	{
		if(currentModelIndex_ != -1)
			return &models_[currentModelIndex_];

		else return 0;
	}

	const UI_BackgroundModelSetup* currentModelSetup() const
	{
		if(currentModelIndex_ != -1)
			return &modelSetups_[currentModelIndex_];

		else return 0;
	}
};

#endif // __UI_BACKGROUND_SCENE_H__
