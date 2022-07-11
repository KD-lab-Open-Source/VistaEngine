#ifndef __VIEW_3DX_MAIN_VIEW_H_INCLUDED__
#define __VIEW_3DX_MAIN_VIEW_H_INCLUDED__

#include "kdw/Viewport.h"
#include "Render/3dx/Umath.h"
#include "Render/src/Frame.h"

#include "kdw/VBox.h"
#include "kdw/MenuBar.h"

#include "kdw/Space.h"
#include "kdw/Application.h"
#include "kdw/Navigator.h"

class Camera;
class cScene;
class cObject3dx;


class MainDocument : public kdw::NavigatorDocument, public kdw::IdleListener{
public:
	MainDocument();
	~MainDocument();

	void loadModel(const char* filename);
	void updateChainDuration();
	cObject3dx* model() const{ return model_; }
	cObject3dx* modelLogic() const{ return modelLogic_; }
	sigslot::signal0<>& signalModelChanged() { return signalModelChanged_; }

	void setLODLevel(int level);
	bool isLODEnabled() const;
	void onIdle();

	cScene* scene(){ return scene_; }
	cFrame& frame(){ return frame_; }

	kdw::NavigatorNodeRoot* createRoot();
protected:

	sigslot::signal0<> signalModelChanged_;
	cScene* scene_;
	cFrame frame_;
	cObject3dx* model_;
	cObject3dx* modelLogic_;

	float chainDuration_;

	double lastTime_;
	double timeFraction_;
};

extern MainDocument* globalDocument;

typedef kdw::NavigatorNode NavBase;

class NavRoot : public kdw::NavigatorNodeRoot{
public:
	NavRoot(MainDocument* document);
	MainDocument* document(){ return static_cast<MainDocument*>(kdw::NavigatorNodeRoot::document()); }

	void build();
};

class NavModel : public NavBase{
public:
	NavModel(cObject3dx* model = 0);
	//MainDocument* document();

	void build();
protected:
	cObject3dx* model_;
};

class NavModelLogic : public NavModel{
public:
	NavModelLogic(cObject3dx* model = 0);
};

class NavNode : public NavBase{
public:
	explicit NavNode(int nodeIndex = -1);
	void build();

	MainDocument* getDocument(){ return static_cast<MainDocument*>(kdw::NavigatorNode::getDocument()); }

protected:
    int nodeIndex_;
};

class MainView : public kdw::Viewport{
public:
	explicit MainView(bool continuousView = false);
	~MainView();

	// virtuals:
	void onRedraw();
	void onResize(int width, int height);
	void onInitialize();

	void onMouseButtonDown(kdw::MouseButton button);
	void onMouseButtonUp(kdw::MouseButton button);
	void onMouseMove();
	// ^^^

	void setOptionBump(bool bump);
	bool optionBump() const{ return optionBump_; }

	void setOptionShadows(bool shadows);
	bool optionShadows() const{ return optionShadows_; }

	void setOptionWireframe(bool wireframe);
	bool optionWireframe() const{ return optionWireframe_; }

	int lod()const { return lod_; }
	void setLod(int lod);
	bool hasLod(int lod);


    void onModelChanged();
	void serializeState(Archive& ar);

	MainDocument* document(){ return globalDocument; }

	void onTestLoad();
protected:
	void resetCameraPosition();
	void transformCamera(float deltaPsi, float deltaTheta, float deltaRoll, float deltaScale);

	Se3f cameraPose_;
	float cameraDistance_;

	int  lod_;
	bool optionBump_;
	bool optionWireframe_;
	bool optionShadows_;

	bool rotating_;
	bool translating_;
	bool zooming_;

	Camera* camera_;

	Vect2i lastMousePosition_;	
};

class MainViewSpace : public kdw::Space{
public:
	typedef MainViewSpace Self;
	MainViewSpace();
	~MainViewSpace();

	void updateMenus();
	void onMenuRenderOption();
	void onMenuLod(int lod);

	void serialize(Archive& ar);
protected:
	ShareHandle<MainView> view_;
};

#endif
