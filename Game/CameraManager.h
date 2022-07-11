#ifndef __CAMERA_MANAGER_H__
#define __CAMERA_MANAGER_H__

#include "Timers.h"
#include "XMath\Rectangle4f.h"
#include "Units\GlobalAttributes.h"
#include "BaseUniverseObject.h"
#include "UnitLink.h"

extern float HardwareCameraFocus;

class Camera;
class UnitInterface;

class CameraCoordinate
{
public:
	CameraCoordinate();
	CameraCoordinate(const Vect2f& position, float psi, float theta, float fi, float distance, float focus = HardwareCameraFocus, const Vect2f& dofParams = Vect2f::ZERO); 
	CameraCoordinate(const Vect3f& position, float psi, float theta, float fi, float distance, float focus = HardwareCameraFocus, const Vect2f& dofParams = Vect2f::ZERO); 

	CameraCoordinate operator+(const CameraCoordinate& coord) const;
	CameraCoordinate operator-(const CameraCoordinate& coord) const;
	CameraCoordinate operator*(float t) const;

	void check(bool restricted);
	void interpolate(const CameraCoordinate& coord0, const CameraCoordinate& coord1, float t);
	void interpolateHermite(const CameraCoordinate coords[4], float t);

	void cycle();
	void uncycle(const CameraCoordinate& coord0); // расцикливает углы по coord0

	const Vect3f& position() const { return position_; }
	void setPosition(const Vect3f& position) { position_ = position; }
	float psi() const { return psi_; }
	float theta() const { return theta_; }
	float fi() const { return fi_; }
	float distance() const { return distance_; }
	float focus() const { return focus_; }
	const Vect2f& dofParams() const { return dofParams_; }

	Vect3f& position() { return position_; }
	float& psi() { return psi_; }
	float& theta() { return theta_; }
	float& fi() { return fi_; }
	float& distance() { return distance_; }
	float& focus() { return focus_; }
	Vect2f& dofParams() { return dofParams_; }

	void serialize(Archive& ar);

	static int height(int x, int y);

	static CameraCoordinate ZERO;
private:
	Vect3f position_;
	float fi_;
	float psi_;
	float theta_;
	float distance_;
	float focus_;

	Vect2f dofParams_;
};

class CameraSpline : public BaseUniverseObject, public ShareHandleBase
{
public:
	typedef vector<CameraCoordinate> Spline;

	bool cycled;

	CameraSpline();

	const char* name() const { return name_.c_str(); };
	void setName(const char* name) { name_ = name; }

	void serialize(Archive& ar);
	
	void addPointAfter(int index, const CameraCoordinate& pos);

	void setPointPosition(int point, const Vect3f& pos);
	void erasePoint(int point);

	int findNearestCoordinate(const Vect2f& pos) const;

	void setPose(const Se3f& pose, bool init);
	
	void showDebug() const;
	void showInfo() const;
	void showEditor(int selectedNode) const;

	bool empty() const { return coordinates_.empty(); }
	size_t size() const { return coordinates_.size(); }
	CameraCoordinate& operator[](int index) { return coordinates_[index]; }
	const CameraCoordinate& operator[](int index) const { return coordinates_[index]; }
	void clear();
	Spline& spline() { return coordinates_; }
	const Spline& spline() const{ return coordinates_; }
    void push_back(const CameraCoordinate& coord){ coordinates_.push_back(coord); }
	void pop_back(){ coordinates_.pop_back(); }
	int stepDuration() const { return stepDuration_; }

	UniverseObjectClass objectClass() const{ return UNIVERSE_OBJECT_CAMERA_SPLINE; }

private:
	string name_;
	Spline coordinates_;
	int stepDuration_;
};

typedef vector< ShareHandle<CameraSpline> > CameraSplines;

class CameraManager
{
public:
	CameraManager(Camera* camera);
	~CameraManager();

	void reset();

	void setFocus(float focus);
	void setCoordinate(const CameraCoordinate& coord);
	void addFlyingHeight(float flyingHeight);
	
	float correctedFocus(float focus, Camera* camera = 0) const;
	float aspect() const { return aspect_; }
	const CameraCoordinate& coordinate() const { return coordinate_; }
	const MatXf& matrix() const { return matrix_; }

	const Vect3f& viewPosition() const { return coordinate_.position(); }
	const Vect3f& eyePosition() const { return eyePosition_; }

	bool isVisible(const Vect3f& position) const;

	void getCameraAxis(Vect3f& axisZ, Vect3f& axisX, Vect3f& axisY); // Возвращает вектора камеры в глобальных координатах. X и Y - экраные.

	void SetFrustumGame();
	void SetFrustumEditor(bool zFarInfinite);
	void SetDefaultCoordinate();
	void splineToDefaultCoordinate(float time, const Vect3f& position);

	void calcRayIntersection(const Vect2f& pos, Vect3f& v0,Vect3f& v1);

	Camera* GetCamera();

	const UnitInterface* unitFollow() const { return unitFollow_; }
	const UnitInterface* directControlUnit() const { return directControlUnit_; }
	const UnitInterface* directControlUnitPrev() const { return directControlUnitPrev_; }

	int mouseQuant(const Vect2f& mousePos);
	void tilt(const Vect2f& mouseDelta);
	void rotate(const Vect2f& mouseDelta);
	void fixEyePosition();
	void rotateAroundEye(const Vect2f& mouseDelta);
	void shift(const Vect2f& mouseDelta);
	void controlQuant(const Vect2i& deltaPos, const Vect2i& deltaOri, int deltaZoom);
	void mouseWheel(int delta);
	void logicQuant();
	void quant(float mouseDeltaX, float mouseDeltaY, float delta_time);

	void SetCameraFollow(const UnitInterface* unit = 0, int transitionTime = 0);
	void disableDirectControl();
	void enableDirectControl(const UnitInterface* unit, int transitionTime);
	void setUnitFollowMode(bool unitFollowDown, bool unitFollowRotate);
	void followQuant(float timeFactor);
	void directControlQuant(float delta_time, float timeFactor);
	void setDirectControlOffset(const Vect3f& offset, int time);
	void setDirectControlFreeCamera();
	const Vect3f& directControlShootingDirection() const;
	bool directControlFreeCamera() const { return directControlFreeCamera_; }
	bool interpolation() { return interpolation_; }

	bool cursorTrace(const Vect2f& pos2,Vect3f& v);

	bool restricted() const { return restricted_; }
	void setRestriction(bool restricted) { restricted_ = restricted; }
	void setCameraRestriction(const CameraRestriction& cameraRestriction) { cameraRestriction_ = cameraRestriction; }
	const CameraRestriction& cameraRestriction() const { return cameraRestriction_; }
	bool ownCameraRestriction() const{ return ownCameraRestriction_; }

	void setInvertMouse(bool invertMouse) { invertMouse_ = invertMouse ? -1 : 1; }
	void setCameraBorder(const CameraBorder& cameraBorder) { cameraBorder_ = cameraBorder; }
	const CameraBorder& cameraBorder() const { return cameraBorder_; }

	//int pathSize() const;
	
	int startReplayPath(int stepDuration, int cycles, UnitInterface* unit = 0); 
	void stopReplayPath();
	bool isPlayingBack() const { return replayIndex_ != -1; }


	CameraSpline* findSpline(const char* name);
	
	void selectSpline(CameraSpline* spline);

	void loadPath(const CameraSpline& spline, bool addCurrentPosition);
	void erasePath();

	const CameraSplines& splines() const { return splines_; }
	CameraSpline& spline() { return spline_; }
	const char* popupCameraSplineName() const;

	void startOscillation(int duration, float factor, const Vect3f& vector = Vect3f::ZERO);

	void drawBlackBars(float opacity = 1.f);

	// Функции для работы редактора.
	void showEditor();

    ///////////////////////////////////////////////////////////////////////////////
	void addSpline(CameraSpline* spline);
	void deleteSpline(CameraSpline* spline);
	const CameraSpline* loadedSpline() const { return loadedSpline_; }

	void deleteSelected();
    ///////////////////////////////////////////////////////////////////////////////

	CameraSplines& splines() { return splines_; }
	string splinesComboList() const;

	void setSkipCutScene(bool skip);
	bool cutSceneSkipped() const { return cutSceneSkipped_;	}

	void serialize(Archive& ar);
	
	// Камера вращается вокруг точки со скоростью _debugRotationAngleDelta;
	void enableAutoRotationMode(float angleDelta) { _debugRotation = true; _debugRotationAngleDelta = angleDelta; }
	void disableAutoRotationMode() { _debugRotation = false; }
	bool isAutoRotationMode() { return _debugRotation; }
	float getRotationAngleDelta() { return _debugRotationAngleDelta; }
	
	const sRectangle4f& frustumClip() const { return frustumClip_; }

	Vect2f calcZMinMax();

	void update();

private:
	int skipFirstIndex(int index);

	bool _debugRotation;
	float _debugRotationAngleDelta;
	Vect3f eyePositionRotate;
	sRectangle4f frustumClip_;
	Camera* camera_;
	MatXf matrix_;
	float aspect_;
	Vect3f eyePosition_;

	bool restricted_;
	bool firstPointAdded_;

	CameraCoordinate coordinate_;
	CameraCoordinate coordinateDelta_;
	float flyingHeight_;

	CameraCoordinate* selectedPoint;
	int selectedSpline; // если (-1) ничего не выбранно.
	
	int interpolationDuration_;
	int interpolationTimer_;
	int finishInterpolation_;
	CameraCoordinate interpolationPoints_[4];

	float  cameraPsiVelocity, cameraPsiForce;
	float  cameraThetaVelocity, cameraThetaForce;
	Vect3f  cameraZoomVelocity, cameraZoomForce;
	Vect3f cameraPositionVelocity, cameraPositionForce;
	Vect3f explodingVector_;
	
	//camera movement		 
	const UnitInterface* unitFollow_;
	const UnitInterface* directControlUnit_;
	const UnitInterface* directControlUnitPrev_;
	InterpolationGraphicsTimer unitFollowTimer_;
	InterpolationGraphicsTimer unitTransitionTimer_;
	bool unitFollowDown_;
	bool unitFollowRotate_;
	bool directControl_;
	float distanceFactor_;
	float distanceFactorNew_;
	int invertMouse_;
	Vect3f directControlShootingDirection_;
	float directControlPsi_;
	bool interpolation_;
	Vect3f prevPosition_;
	Vect3f deltaPosePrev_;
	Vect3f directControlOffset_;
	Vect3f directControlOffsetPrev_;
	Vect3f directControlOffsetNew_;
	bool directControlFreeCamera_;
	bool directControlFreeCameraStop_;

	CameraSpline spline_;
	const CameraSpline* loadedSpline_;

	int replayIndex_;
	int replayIndexMax_;

	CameraSplines splines_;
	bool ownCameraRestriction_;
	CameraRestriction cameraRestriction_;
	CameraBorder cameraBorder_;

	LogicTimer oscillatingTimer_;
	int explodingDuration_;
	float explodingFactor_;

	bool cutSceneSkipped_;

	mutable MTSection cameraLock_;

	void setPath(int index);
	float getSphereHeight(const Vect3f& position, float radius, float x, float y);
	float collisionCamera( const Vect3f& position, float radius );
	void collisionResolve();
	void callcTheta(float hDelta);
	void callcDistance(float hDelta);
};

extern CameraManager* cameraManager;

#endif //__CAMERA_MANAGER_H__
