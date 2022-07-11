#pragma once

#include "kdw\Viewport.h"
#include "kdw\HBox.h"
#include "kdw\Label.h"
#include "Render\3dx\Static3dxBase.h"
#include "Render\Inc\IVisGenericInternal.h"
#include "Render\3dx\Node3dx.h"
#include "Render\3dx\Simply3dx.h"

using namespace kdw;

class EnvironmentTime;

namespace FT {
	class Font;
}

class NodeSerializer : public ComboListString
{
public:
	NodeSerializer() { nodes_ = 0; }
	NodeSerializer(const StaticNodes& nodes, bool enableEmpty);
	bool serialize(Archive& ar, const char* name, const char* nameAlt);
	int index() const;

private:
	const StaticNodes* nodes_;

	void add(const StaticNodes& nodes, int parent);
};

class View3dx : public kdw::Viewport
{
public:

	enum Lod { LOD0, LOD1, LOD2 };

	enum ObjectsNumber {
		OBJECTS_NUMBER_1 = 1,
		OBJECTS_NUMBER_2 = 2,
		OBJECTS_NUMBER_10 = 10
	};

	enum ControlMode {
		CONTROL_MODE_OBJECT,
		CONTROL_MODE_CAMERA,
		CONTROL_MODE_LIGHT
	};

	View3dx();
	virtual ~View3dx();

	void loadObject(LPCSTR fname);
	void SetSunDirection(const Vect3f& dAngle);
	void ObjectControl(Vect3f& dPos,Vect3f& dAngle, float dScale);

	void serialize(Archive& ar);

	kdw::HBox* statusLine() { return statusLine_; }

private:
	kdw::HBox* statusLine_;
	kdw::Label* timeLabel_;
	kdw::Label* fpsLabel_;
	kdw::Label* phaseLabel_;
	kdw::Label* polygonssPerSecondLabel_;
	kdw::Label* polygonsLabel_;
	kdw::Label* DIPsLabel_;

	Lod lod_;

	struct Item
	{
		string name;
		ComboListString data;
		Item() {}
		Item(const string& _name, const ComboListString& _data) : name(_name), data(_data) {}
		bool serialize(Archive& ar, const char* name, const char* nameAlt);
	};
	typedef vector<Item> Items;
	Items visibilitySets_;

	bool chainsSimultaneously_;
	ComboListString chain_;
	Items animationGroups_;

	string logoTexture_;
	Color4f skinColor_;

	bool hideObject_;
	bool showLights_;
	bool lighting_;
	bool useShadow_;
	BaseGraphObject* tileMap_;
	float timeLight_;
	bool useTimeLight_;
	bool bump_;
	bool anisotropic_;

	int	timePrev_;
	float scaleTime_;
	float framePhase_;
	float framePeriod_;
	bool pauseAnimation_;
	bool reverseAnimation_;

	ObjectsNumber objectsNumber_;
	bool isModelCamera_;
	bool isCamera43_;

	EnvironmentTime* environmentTime_;

	string fileName_;

	cScene* scene_;		
	Camera* camera_;

	FT::Font* pFont;

	typedef vector<UnknownHandle<cObject3dx> > Objects;
	Objects objects_;
	UnknownHandle<cObject3dx> object_;
	UnknownHandle<cObject3dx> logic_;

	ControlMode controlMode_;
	int dwScrX,dwScrY;
	bool wireFrame_;
	bool mouseLDown_;
	bool mouseRDown_;
	Vect2i PointMouseLDown;
	Vect2i PointMouseRDown;
	bool showGraphicsBound_;
	bool showGraphicsNodes_;
	bool showBoundSpheres_;
	bool showFog_;
	bool showNormals_;
	bool showDebrises_;
	bool showSpline_;

	bool scale_normal;

	bool showLogicBound_;
	bool showLogicNodes_;

	int mipMapLevel_;
	bool debugMipMap_;

	NodeSerializer selectedLogicNode_;
	NodeSerializer selectedGraphicsNode_;
	bool check_graph_by_logic;
	Vect3f userRotation_;

	Vect3f objectPosition_;
	QuatF objectOrientation_;
	Se3f cameraPose_;
	float sunAlpha_;
	float sunTheta_;

	bool showZeroPlane_;

	vector<cSimply3dx*> debrises;

	void SetScale(bool normal);

	void ResetCameraPosition();
	void SetCameraPosition(float du,float dv,float dscale);

	void ModelInfo();

	void OnScreenShoot();

	void UpdateCameraFrustum();
	void SetDirectLight(float time);
	void applyUserTransform();

	void Draw();

	void DrawSpline();
	void DrawZeroPlane();
	void InitFont();

	void destroyDebrises();
	void createDebrises();

	void updateObject();

	void onInitialize();
	void onRedraw();
	void onResize(int width, int height);
	void onMouseButtonDown(MouseButton button);
	void onMouseButtonUp(MouseButton button);
	void onMouseMove();
	void onMouseWheel(int delta);

	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};

