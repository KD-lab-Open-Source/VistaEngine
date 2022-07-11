#ifndef __UI_BACKGROUND_SCENE_H__
#define __UI_BACKGROUND_SCENE_H__

#include "..\Render\inc\UMath.h"
#include "Handle.h"
#include "TypeLibrary.h"

class cScene;
class cCamera;
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

	bool operator == (const UI_BackgroundAnimation& animation) const {
		return animationGroupName_ == animation.animationGroupName_ &&
			chainName_ == animation.chainName_;
	}

	int animationGroupIndex() const { return animationGroupIndex_; }
	void setAnimationGroupIndex(int index){ animationGroupIndex_ = index; }

	const char* animationGroupName() const { return animationGroupName_.c_str(); }
	const char* chainName() const { return chainName_.c_str(); }

	bool reversed() const { return reversed_; }
	void setReversed(bool reversed){ reversed_ = reversed; }

	bool isCycled() const { return playMode_ == PLAY_PERMANENT || playMode_ == PLAY_HOVER_PERMANENT; }
	PlayMode playMode() const { return playMode_; }

	float duration() const { return duration_; }
	void setDuration(float duration){ duration_ = duration; }

	float phase() const { return (reversed_) ? (1.f - phase_) : phase_; }
	void phaseReverse(){ phase_ = 1.f - phase_; }
	void reset(){ phase_ = 0.f; }

	bool isFinished() const { return !isCycled() && (1.f - phase_ < FLT_EPS); }

	void quant(float dt);

private:

	PlayMode playMode_;

	int animationGroupIndex_;
	std::string animationGroupName_;

	std::string chainName_;

	float duration_; // Длительность (в с)
	bool reversed_;	// Проигрывать в прямом или обратном порядке

	float phase_;
};

typedef std::vector<UI_BackgroundAnimation> UI_BackgroundAnimations;

class UI_BackgroundModelSetup
{
public:
	UI_BackgroundModelSetup();

	bool operator == (const char* name) const { return !stricmp(modelName_.c_str(), name); }

	bool isEmpty() const { return modelName_.empty(); }
	const char* modelName() const { return modelName_.c_str(); }
	
	bool ownSkinColor() const { return useOwnColor_; }
	const sColor4f& skinColor() const { return skinColor_; }
	bool useEmblem() const { return useEmblem_; }

	void serialize(Archive& ar);

	void preLoad(cScene* scene, const Player* player) const;
	const char* groupComboList() const { return groupComboList_.c_str(); }
	const char* chainComboList() const { return chainComboList_.c_str(); }

private:

	std::string modelName_;
	bool useOwnColor_;
	bool useEmblem_;
	sColor4f skinColor_;

	std::string groupComboList_;
	std::string chainComboList_;

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

	void reset(){ animations_.clear(); }

	void setPosition(const MatXf& pos);

	bool isPlaying() const;
	bool isPlaying(UI_BackgroundAnimation::PlayMode mode) const;
	bool isPlaying(const UI_BackgroundAnimation& animation) const;
	
	//! Запустить анимацию объекта с начала, если она еще не запущена
	bool play(const UI_BackgroundAnimation& animation, bool reverse = false);
	//! Остановить анимацию объекта в текущем состоянии
	bool stop(const UI_BackgroundAnimation& animation);

	void quant(float dt);

	void getDebugInfo(XBuffer& buf) const;

private:

	cObject3dx* model_;
	UI_BackgroundAnimations animations_;
};

typedef std::vector<UI_BackgroundModel> UI_BackgroundModels;

class UI_BackgroundLight
{
public:
	UI_BackgroundLight();

	void serialize(Archive& ar);

	float lifeTime() const { return lifeTime_; }

	float radius() const { return radius_; }
	const sColor4f& color() const { return color_; }

private:

	float lifeTime_;

	float radius_;
	sColor4f color_;
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
	void setRenderTarget(cTexture* renderTarget, IDirect3DSurface9* depthBuffer);
	void setSky(cTexture* texture);

	void done();
	bool ready() const;

	void logicQuant(float dt);
	void graphQuant(float dt);
	void draw() const;

	void reset();

	int getModelIndex(const char* modelName) const;
	const UI_BackgroundModelSetup& modelSetup(int idx) const { xassert(idx >= 0 && idx < modelSetups_.size()); return modelSetups_[idx]; }

	bool selectModel(const char* model_name, bool need_load = true);

	bool isPlaying() const;
	bool isPlaying(UI_BackgroundAnimation::PlayMode mode) const;
	bool isPlaying(const UI_BackgroundAnimation& animation) const;

	bool addLight(int light_index, const Vect2f& position);
	
	//! Запустить анимацию объекта с начала, если она еще не запущена
	bool play(const UI_BackgroundAnimation& animation, bool reverse = false);
	//! Остановить анимацию объекта в текущем состоянии
	bool stop(const UI_BackgroundAnimation& animation);

	void setEnabled(bool state) { enabled_ = state; }
	bool inited() const { return (scene_ != 0); }

	const char* groupComboList() const;
	const char* chainComboList() const;

	const char* modelComboList() const { return modelComboList_.c_str(); }

	cScene* scene() const { return scene_; }

	void drawDebug2D() const;

	static UI_BackgroundScene& instance(){ return Singleton<UI_BackgroundScene>::instance(); }

private:

	cScene* scene_;
	cCamera* camera_;

	bool enabled_;

	int currentModelIndex_;

	UI_BackgroundModelSetups modelSetups_;
	UI_BackgroundModels models_;

	Vect3f cameraPosition_;
	Vect3f cameraAngles_;
	Vect3f modelAngles_;
	bool cameraPerspective_;
	float cameraFocus_;

	std::string modelComboList_;

	typedef std::vector<UI_BackgroundLight> Lights;
	Lights lights_;

	Vect3f lightDirection_;

	typedef SwapVector<UI_BackgroundLightController> LightControllers;
	LightControllers lightControllers_;

	void updateModelComboList();

	UI_BackgroundModel* currentModel()
	{
		if(currentModelIndex_ != -1)
			return &models_[currentModelIndex_];

		else return 0;
	}

	const UI_BackgroundModel* currentModel() const
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
