#include "StdAfx.h"
#include "CameraManager.h"

#include "UserInterface\UI_Logic.h"
#include "UserInterface\UI_Render.h"
#include "GameOptions.h"

#include "UnitActing.h"
#include "vmap.h"
#include "RenderObjects.h"
#include "EditorVisual.h"
#include "XMath\SafeMath.h"
#include "Squad.h"
#include "Serialization\Serialization.h"
#include "Water\Water.h"
#include "Environment\Environment.h"
#include "Render\D3D\D3DRender.h"
#include "Render\Src\cCamera.h"
#include "Render\src\Scene.h"
#include "Serialization\SerializationFactory.h"

//---------------------------------------
float visibilityDistance2 = sqr(750.f);

float HardwareCameraFocus = 0.8f;

REGISTER_CLASS(CameraSpline, CameraSpline, "Сплайн камеры")

float CUT_SCENE_TOP = -0.390625f;
float CUT_SCENE_BOTTOM = 0.390625f;

namespace{
	const Color4c DEFAULT_COLOR(200, 200, 200, 200);
	const Color4c DEFAULT_TRANSPARENT_COLOR(200, 200, 200, 64);

	const Color4c ACTIVE_COLOR(Color4c::GREEN);
	const Color4c ACTIVE_TRANSPARENT_COLOR(0, 255, 0, 64);

	const Color4c SELECTED_COLOR(Color4c::RED);
	const Color4c SELECTED_TRANSPARENT_COLOR(255, 0, 0, 64);
};

//---------------------------------------
// Camera exploding
struct HarmonicOscillator // f(t) = exp(-t*decay)*(A[0]*sin(omega*t) + A[1]*sin(2*omega*t) + ...)
{
	float decay;
	float omega;
	float amplitude;
	int N;
	float A[4];

	HarmonicOscillator(float amplitudeIn, float omegaIn) {
		N = 4;
		A[0] = 1;
		A[1] = 0.5f;
		A[2] = 0.3f;
		A[3] = 0.1f;
		decay = 0;
		omega = omegaIn;
		amplitude = amplitudeIn;
	}
	float operator()(float t){ // in seconds
		float phase = omega*t;
		float f = 0;
		for(int i = 0; i < N; i++)
			f += A[i]*sin(phase*(i + 1));
		return f*exp(-t*decay)*amplitude; 
	}
};

struct CameraExplodingPrm
{
	HarmonicOscillator x, y, z;

	CameraExplodingPrm() : x(20, 100), y(20, 1), z(20, 99) {}
};

CameraExplodingPrm cameraExplodingPrm;

//---------------------------------------
void SetCameraPosition(Camera* camera,const MatXf& Matrix)
{
	MatXf ml=MatXf::ID;
	ml.rot()[2][2]=-1;

	MatXf mr=MatXf::ID;
	mr.rot()[1][1]=-1;
	MatXf CameraMatrix;
	CameraMatrix=mr*ml*Matrix;

	camera->SetPosition(CameraMatrix);
}

//-------------------------------------------------------------------

CameraCoordinate CameraCoordinate::ZERO(Vect3f::ZERO, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, Vect2f::ZERO);

CameraCoordinate::CameraCoordinate(const Vect2f& position, float psi, float theta, float fi, float distance, float focus, const Vect2f& dofParams)
	: psi_(psi)
	, theta_(theta)
	, fi_(fi)
	, distance_(distance)
	, position_(position, height(position.xi(), position.yi()))
	, focus_(focus)
	, dofParams_(dofParams)
{
	//check();
}

CameraCoordinate::CameraCoordinate()
	: position_(Vect3f::ZERO)
	, psi_(0.0f)
	, theta_(0.0f)
	, fi_(0.0f)
	, distance_(0.0f)
	, focus_(HardwareCameraFocus)
	, dofParams_(100.0f, 1000.0f)
{
}

CameraCoordinate::CameraCoordinate(const Vect3f& position, float psi, float theta, float fi, float distance, float focus, const Vect2f& dofParams)
	: psi_(psi)
	, theta_(theta)
	, fi_(fi)
	, distance_(distance)
	, position_(position)
	, focus_(focus)
	, dofParams_(dofParams)
{

}

CameraCoordinate CameraCoordinate::operator*(float t) const
{
	return CameraCoordinate(position()*t, psi()*t, theta()*t, fi()*t, distance()*t, focus()*t, dofParams()*t);
}

CameraCoordinate CameraCoordinate::operator+(const CameraCoordinate& coord) const
{
	return CameraCoordinate(position() + coord.position(), psi() + coord.psi(), theta() + coord.theta(), fi() + coord.fi(), distance() + coord.distance(), focus() + coord.focus(), dofParams() + coord.dofParams());
}

CameraCoordinate CameraCoordinate::operator-(const CameraCoordinate& coord0) const
{
	CameraCoordinate coord1 = *this;
	coord1.uncycle(coord0);
	return CameraCoordinate(coord1.position() - coord0.position(), coord1.psi() - coord0.psi(), coord1.theta() - coord0.theta(), coord1.fi() - coord0.fi(), coord1.distance() - coord0.distance(), coord1.focus() - coord0.focus(), coord1.dofParams() - coord0.dofParams());
}

void CameraCoordinate::uncycle(const CameraCoordinate& coord0)
{
	psi_ = ::uncycle(psi_, coord0.psi(), 2*M_PI);
	theta_ = ::uncycle(theta_, coord0.theta(), 2*M_PI);
	fi_ = ::uncycle(fi_, coord0.fi(), 2*M_PI);
}
void CameraCoordinate::cycle()
{
	psi_ = cycleAngle(psi_);
	theta_ = cycleAngle(theta_);
	fi_ = cycleAngle(fi_);
}


void CameraCoordinate::interpolate(const CameraCoordinate& coord0, const CameraCoordinate& coord1, float t)
{
	position_.interpolate(coord0.position(), coord1.position(), t);
	dofParams_ = Vect2f(coord0.dofParams().x * ( 1.0f - t) + coord1.dofParams().x * t, coord0.dofParams().y * ( 1.0f - t) + coord1.dofParams().y * t);
	psi_ = ::cycle(coord0.psi() + getDist(coord1.psi(), coord0.psi(), 2*M_PI)*t, 2*M_PI);
	theta_ = ::cycle(coord0.theta() + getDist(coord1.theta(), coord0.theta(), 2*M_PI)*t, 2*M_PI);
	fi_ = ::cycle(coord0.fi() + getDist(coord1.fi(), coord0.fi(), 2*M_PI)*t, 2*M_PI);
	distance_ = coord0.distance() + (coord1.distance() - coord0.distance())*t;
	focus_ = coord0.focus() + (coord1.focus() - coord0.focus())*t;
}

void CameraCoordinate::interpolateHermite(const CameraCoordinate coords[4], float u)
{
	float t2 = u*u;
	float t3 = u*t2;
	*this = coords[3] * (0.5f * (-t2 + t3)) + coords[0] * (-0.5f * u + t2 - 0.5f * t3) 
		+ coords[2] * (2.0f * t2 - 1.5f * t3 + 0.5f * u) + coords[1] * (1.0f - 2.5f * t2 + 1.5f * t3);
	cycle();
}

void CameraCoordinate::check(bool restricted)
{
	const CameraRestriction& cameraRestriction_ = cameraManager->cameraRestriction();
	const CameraBorder& cameraBorder_ = cameraManager->cameraBorder();

	if(restricted){
		distance_ = clamp(distance(), cameraRestriction_.zoomMin, cameraRestriction_.zoomMax);
		float scroll_border = (distance() - cameraRestriction_.zoomMin)/(cameraRestriction_.zoomMax - cameraRestriction_.zoomMin)*cameraRestriction_.CAMERA_WORLD_SCROLL_BORDER;
		position_.x = clamp(position().x, scroll_border, vMap.H_SIZE - scroll_border);
		position_.y = clamp(position().y, scroll_border, vMap.V_SIZE - scroll_border);

		float t = clamp(1 - (distance() - cameraRestriction_.zoomMin)/(cameraRestriction_.zoomMaxTheta - cameraRestriction_.zoomMin), 0, 1.f);
		float theta_max = cameraRestriction_.thetaMaxHigh + t*(cameraRestriction_.thetaMaxLow - cameraRestriction_.thetaMaxHigh);
		float theta_min = cameraRestriction_.thetaMinHigh + t*(cameraRestriction_.thetaMinLow - cameraRestriction_.thetaMinHigh);
		theta_ = clamp(theta(), theta_min, theta_max);
	}
	else{
		distance_ = clamp(distance(), 10.0f, 10000.0f);
		if(!isUnderEditor())
			theta_ = clamp(theta(), 0.0f, M_PI);
	}
 	
	focus_ = clamp(focus(), 0.1f, 10.0f);

	Vect3f camera_position;
	camera_position.setSpherical(psi_, theta_, distance_);
	camera_position += position_;

	Vect3f old_camera_pos = camera_position;

	if(restricted){
		if(!isUnderEditor()){
			camera_position.x = clamp(camera_position.x, cameraBorder_.CAMERA_WORLD_BORDER_LEFT, vMap.H_SIZE - cameraBorder_.CAMERA_WORLD_BORDER_RIGHT);
			camera_position.y = clamp(camera_position.y, cameraBorder_.CAMERA_WORLD_BORDER_TOP, vMap.V_SIZE - cameraBorder_.CAMERA_WORLD_BORDER_BOTTOM);
		}
		camera_position.z = clamp(camera_position.z, cameraRestriction_.heightMin, cameraRestriction_.heightMax);
	}else
		camera_position.z = clamp(camera_position.z, 0.0f, 10000.0f);

	position_ += (camera_position - old_camera_pos);

}

int CameraCoordinate::height(int x, int y) 
{
	x = clamp(x, 0, vMap.H_SIZE - 1);
	y = clamp(y, 0, vMap.V_SIZE - 1);
	if(GlobalAttributes::instance().cameraRestriction.aboveWater && water)
		return water->GetZFast(x, y);
	return vMap.isWorldLoaded() ? vMap.getApproxAlt(x, y) : 0;
}

//-------------------------------------------------------------------

CameraManager::CameraManager(Camera* camera)
: coordinate_(Vect2f(500, 500), 0, 0, 0, 300)
, frustumClip_(-0.5f, -0.5f, 0.5f, 0.5f)
, aspect_(4.0f / 3.0f)
, firstPointAdded_(false)
, flyingHeight_(0.0f)
{
	camera_ = camera;
	cameraRestriction_ = GlobalAttributes::instance().cameraRestriction;
	
	coordinateDelta_ = CameraCoordinate::ZERO;
	
	eyePositionRotate = Vect3f::ZERO;

	matrix_ = MatXf::ID;
	eyePosition_ = Vect3f::ZERO;
	
	ownCameraRestriction_ = false;

	restricted_ = GameOptions::instance().getBool(OPTION_CAMERA_RESTRICTION);
	unitFollowDown_ = GameOptions::instance().getBool(OPTION_CAMERA_UNIT_DOWN_FOLLOW);
	unitFollowRotate_ = GameOptions::instance().getBool(OPTION_CAMERA_UNIT_ROTATE);
	setInvertMouse(GameOptions::instance().getBool(OPTION_CAMERA_RESTRICTION));

	replayIndex_ = -1;
	replayIndexMax_ = 0;
	interpolationTimer_ = 0;
	finishInterpolation_ = 0;

	cameraPsiVelocity = cameraPsiForce = 0;
	cameraThetaVelocity = cameraThetaForce = 0;
	cameraZoomVelocity = cameraZoomForce = Vect3f::ZERO;
	cameraPositionVelocity = cameraPositionForce = Vect3f::ZERO;

	explodingDuration_ = 0;

	selectedPoint = NULL;
	selectedSpline = -1;
	_debugRotation = false;
	_debugRotationAngleDelta = 0.0f;

	explodingVector_ = Vect3f::ZERO;
	loadedSpline_ = 0;

	directControl_ = false;

	directControlShootingDirection_ = Vect3f::I;

	directControlPsi_ = 0.0f;

	interpolation_ = false;

	prevPosition_ = Vect3f::ZERO;
	deltaPosePrev_ = Vect3f::ZERO;

	directControlOffset_ = Vect3f::ZERO;
	directControlOffsetPrev_ = Vect3f::ZERO;
	directControlOffsetNew_ = Vect3f::ZERO;
	directControlFreeCamera_ = false;
	directControlFreeCameraStop_ = false;

	unitFollow_ = 0;
	directControlUnit_ = 0;
	directControlUnitPrev_ = 0;
	distanceFactor_ = 1.0f;
	distanceFactorNew_ = 1.0f;

	update();
}

CameraManager::~CameraManager()
{
	camera_->Release();
}

void CameraManager::setCoordinate(const CameraCoordinate& coord) 
{ 
	coordinate_ = coord;
	coordinate_.cycle();
	update();
}

void CameraManager::addFlyingHeight(float flyingHeight) 
{ 
	flyingHeight_ += flyingHeight; 
}

void CameraManager::setFocus(float focus)
{
	coordinate_.focus() = focus;
	update();
}

void CameraManager::update()
{
	Vect3f position;
	position.setSpherical(coordinate().psi(), coordinate().theta(), coordinate().distance()*distanceFactor_);
	position += coordinate().position();

	Mat3f oscillatingMatrix = Mat3f::ID;
	if(oscillatingTimer_.busy()){
		float t = (float)(explodingDuration_ - oscillatingTimer_.timeRest())/1000;
		Vect3f value = explodingVector_*sin(4*M_PI*t)*(1/(sqr(t)+0.1f)-0.7f);
		position += value;
		Vect3f axisX(cos(coordinate().psi()+M_PI_2), sin(coordinate().psi()+M_PI_2), 0);
		oscillatingMatrix.set(Vect3f(cos(coordinate().psi()), sin(coordinate().psi()), 0), value.dot(axisX)*0.003f);
	}
	else
		explodingVector_ = Vect3f::ZERO;

	eyePosition_ = position;

	matrix_ = MatXf::ID;
	matrix_.rot() = Mat3f(coordinate().theta(), X_AXIS)*Mat3f(coordinate().fi(), Y_AXIS)*Mat3f(M_PI_2 - coordinate().psi(), Z_AXIS) * oscillatingMatrix;
//	matrix_.rot() = Mat3f(coordinate().theta(), X_AXIS)*Mat3f(M_PI_2 - coordinate().psi(), Z_AXIS);
	matrix_ *= MatXf(Mat3f::ID, -position);	

	camera_->setAttribute(ATTRCAMERA_PERSPECTIVE);
	SetFrustumGame();
	SetCameraPosition(camera_, matrix_);
}

void CameraManager::SetFrustumEditor(bool zFarInfinite)
{
	Vect2f z=calcZMinMax();
	if(zFarInfinite)
		z.y = 12000;
	Vect2f center(0.5f, 0.5f);
      	
	frustumClip_.set(-0.5f, -0.5f, 0.5f, 0.5f);

	camera_->SetFrustum(								// устанавливается пирамида видимости
		&Vect2f(0.5f,0.5f),							// центр камеры
		&frustumClip_,								// видимая область камеры
		&Vect2f(coordinate().focus(), coordinate().focus()),	// фокус камеры
		&z
		);
}

void CameraManager::SetFrustumGame()
{
	Vect2f z=calcZMinMax();

	Rectf windowPosition(0.0f, 0.0f, gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY());
	aspect_ = windowPosition.width() / windowPosition.height();
	float aspect = max(4.0f / 3.0f, aspect_);
	Rectf aspectWindowPosition(aspectedWorkArea(windowPosition, aspect));

	Vect2f center(aspectWindowPosition.center());
      	
	frustumClip_.set(-0.5f * (aspectWindowPosition.left() - center.x) / (windowPosition.left() - center.x),
					 -0.5f * (aspectWindowPosition.top() - center.y)  / (windowPosition.top() - center.y),
					 0.5f * (aspectWindowPosition.right() - center.x)  / (windowPosition.right() - center.x),
					 0.5f * (aspectWindowPosition.bottom() - center.y)  / (windowPosition.bottom() - center.y)
					 );


	float focus = correctedFocus(coordinate().focus());

	camera_->SetFrustum(			// устанавливается пирамида видимости
		&Vect2f(0.5f,0.5f),		// центр камеры
		&frustumClip_,			// видимая область камеры
		&Vect2f(focus, focus),	// фокус камеры
		&z
		);
}

Vect2f CameraManager::calcZMinMax()
{
	Vect2f z(30.0f,13000.0f);
	if(environment)
	{
		z.x=environment->GetGameFrustrumZMin();
		float angle=coordinate().theta();
		float c=fabsf(angle)/(M_PI_2);
		c=clamp(c,0.0f,1.0f);
		z.y=environment->GetGameFrustrumZMaxHorizontal()*c+
			environment->GetGameFrustrumZMaxVertical()*(1-c);
	}

	return z;
}

void CameraManager::calcRayIntersection(const Vect2f& pos, Vect3f& v0,Vect3f& v1)
{
	camera_->ConvertorCameraToWorld(pos, v1);
	if(camera_->getAttribute(ATTRCAMERA_PERSPECTIVE)){
		MatXf matrix=camera_->GetPosition();
		v0 = matrix.invXformVect(matrix.trans(),v0);
		v0.negate();	
	}else{
		v0.x = v1.x;
		v0.y = v1.y;
		v0.z = v1.z + 10.0f;
	}

	Vect3f dir = v1 - v0;
	float m = 9999.9f/dir.norm();
	dir *= m;
	v1 = v0 + dir;	
}

bool CameraManager::isVisible(const Vect3f& position) const
{
	return viewPosition().distance2(position) < visibilityDistance2 || eyePosition().distance2(position) < visibilityDistance2;
}

//---------------------------------------------------
void CameraManager::controlQuant(const Vect2i& deltaPos, const Vect2i& deltaOri, int deltaZoom)
{
	if(interpolationTimer_)
		return;
	
	if(!directControlUnit()){
		cameraPositionForce.x += deltaPos.x*cameraRestriction_.CAMERA_SCROLL_SPEED_DELTA;
		cameraPositionForce.y += deltaPos.y*cameraRestriction_.CAMERA_SCROLL_SPEED_DELTA;
	}
	
	cameraPsiForce += deltaOri.x*cameraRestriction_.CAMERA_KBD_ANGLE_SPEED_DELTA;
	cameraThetaForce += deltaOri.y*cameraRestriction_.CAMERA_KBD_ANGLE_SPEED_DELTA;
	
	cameraZoomForce.z += deltaZoom*cameraRestriction_.zoomKeyAcceleration*coordinate().distance();
}

int CameraManager::mouseQuant(const Vect2f& mousePos)
{
	if(interpolationTimer_ || directControlUnit())
		return 0;

	//const CameraRestriction& cameraRestriction_ = GetCameraParam();

	int dir = 0;

	if(fabs(mousePos.x + 0.5f) < cameraRestriction_.CAMERA_BORDER_SCROLL_AREA_HORZ){
		if(coordinate().position().x > 0){
			cameraPositionForce.x -= cameraRestriction_.CAMERA_BORDER_SCROLL_SPEED_DELTA;
			dir += 2;
		}
	}
	else if(fabs(mousePos.x - 0.5f) < cameraRestriction_.CAMERA_BORDER_SCROLL_AREA_HORZ){
		if(coordinate().position().x < vMap.H_SIZE) {
			cameraPositionForce.x += cameraRestriction_.CAMERA_BORDER_SCROLL_SPEED_DELTA;
			dir += 4;
		}
	}
	
	if(fabs(mousePos.y + 0.5f) < cameraRestriction_.CAMERA_BORDER_SCROLL_AREA_UP){
		if(coordinate().position().y > 0){
			cameraPositionForce.y -= cameraRestriction_.CAMERA_BORDER_SCROLL_SPEED_DELTA;
			dir += 1;
		}
	}
	else if(fabs(mousePos.y - 0.5f) < cameraRestriction_.CAMERA_BORDER_SCROLL_AREA_DN){
		if(coordinate().position().y < vMap.V_SIZE){
			cameraPositionForce.y += cameraRestriction_.CAMERA_BORDER_SCROLL_SPEED_DELTA;
			dir += 8;
		}
	}

	return dir;
}

void CameraManager::tilt(const Vect2f& mouseDelta)
{
	if(fabs(mouseDelta.x) > fabs(mouseDelta.y)){
		cameraPsiForce += mouseDelta.x*cameraRestriction_.CAMERA_MOUSE_ANGLE_SPEED_DELTA;
	}
	else{
		cameraThetaForce -= mouseDelta.y*cameraRestriction_.CAMERA_MOUSE_ANGLE_SPEED_DELTA;
	}
}

void CameraManager::rotate(const Vect2f& mouseDelta)
{
	cameraPsiForce += mouseDelta.x*cameraRestriction_.CAMERA_MOUSE_ANGLE_SPEED_DELTA;
	cameraThetaForce -= mouseDelta.y*cameraRestriction_.CAMERA_MOUSE_ANGLE_SPEED_DELTA;
}

void CameraManager::fixEyePosition()
{
	Vect3f offset = Vect3f::I;
	offset.scale(coordinate_.distance());
	QuatF cameraOrientation(coordinate_.theta() - M_PI_2, Vect3f::J, 0);
	cameraOrientation.premult(QuatF(coordinate_.psi(), Vect3f::K, 0));
	cameraOrientation.xform(offset);
	eyePositionRotate = coordinate_.position() + offset;
}

void CameraManager::rotateAroundEye(const Vect2f& mouseDelta)
{
	coordinate_.psi() += mouseDelta.x;
	coordinate_.theta() += mouseDelta.y;
	Vect3f offset = Vect3f::I;
	offset.scale(coordinate_.distance());
	QuatF cameraOrientation(coordinate_.theta() - M_PI_2, Vect3f::J, 0);
	cameraOrientation.premult(QuatF(coordinate_.psi(), Vect3f::K, 0));
	cameraOrientation.xform(offset);
	coordinate_.position() = eyePositionRotate - offset;
}

bool CameraManager::cursorTrace(const Vect2f& pos2D, Vect3f& v)
{
	if(!vMap.isWorldLoaded())
		return false;

	Vect3f pos,dir;
	GetCamera()->GetWorldRay(pos2D, pos, dir);

	if(directControl_ && directControlUnit()){
		float dist = directControlUnit()->position2D().distance(pos);
		pos.z += dist*dir.z/(((Vect2f&)dir).norm() + 0.01f);
		(Vect2f&)pos += (Vect2f&)dir*dist;
	}

	if(!terScene->TraceDir(pos,dir,&v)){
		Rectf rect(Vect2f(vMap.H_SIZE-1, vMap.V_SIZE-1));
		Vect2f p1(pos);
		
		dir *= pos.z / (dir.z >= 0 ? 0.0001f : -dir.z);
		Vect2f p2(pos += dir);
		
		if(!rect.clipLine(p1, p2))
			p2 = clampWorldPosition(p2, 0.0f);
		v = Vect3f(p2, vMap.getApproxAlt(p2.xi(), p2.yi()));

		return false;
	}
	else
		environment->water()->Trace(pos,v,v);

	return true;
}

void CameraManager::shift(const Vect2f& mouseDelta)
{
	if(interpolationTimer_ || directControlUnit())
		return;

	Vect2f delta = mouseDelta;
	Vect3f v1, v2;
	if(cursorTrace(Vect2f::ZERO, v1) && cursorTrace(delta, v2))
		delta = v2 - v1; 
	else
		delta = Vect2f::ZERO;
	
	coordinate_.position() -= Vect3f(delta, 0);
}

void CameraManager::mouseWheel(int delta)
{
	
	float zoomAcc = cameraRestriction_.zoomWheelImpulse*coordinate().distance();

	if(delta > 0){
		if(GameOptions::instance().getBool(OPTION_CAMERA_ZOOM_TO_CURSOR)){
			Vect3f direction = UI_LogicDispatcher::instance().hoverPosition() - coordinate_.position();
			direction.normalize(clamp(zoomAcc, 0.0f, direction.norm()));
			cameraZoomVelocity += direction;
		}
		cameraZoomVelocity.z -= zoomAcc;
	}
	else if(delta < 0){
		if(GameOptions::instance().getBool(OPTION_CAMERA_ZOOM_TO_CURSOR)){
			Vect3f direction = UI_LogicDispatcher::instance().hoverPosition() - coordinate_.position();
			direction.normalize(clamp(zoomAcc, 0.0f, direction.norm()));
			cameraZoomVelocity -= direction;
		}
		cameraZoomVelocity.z += zoomAcc;
	}
}

void CameraManager::logicQuant()
{
	if(finishInterpolation_ > 0){
		MTAuto lock(cameraLock_);
		if(--finishInterpolation_ == 0)
			interpolationTimer_ = 0;
	}
	
	if(!isUnderEditor()){
		if(unitFollow() && !unitFollow()->alive()){
			MTAuto lock(cameraLock_);
			unitFollow_ = 0;
		}
		if(directControlUnit() && !directControlUnit()->alive()){
			MTAuto lock(cameraLock_);
			directControlUnit_ = 0;
			coordinate_.fi() = 0.0f;
		}
	}
}

void CameraManager::quant(float mouseDeltaX, float mouseDeltaY, float delta_time)
{
	MTAuto lock(cameraLock_);

	xassert(delta_time < 0.1f + FLT_COMPARE_TOLERANCE);
	float timeFactor = delta_time*20.f; // !!! PerimeterCameraControlFPS  - убрать теперь сложно 

	if(isAutoRotationMode())
		coordinate_.psi() = coordinate().psi() + _debugRotationAngleDelta;
	else if(fabs(_debugRotationAngleDelta) > 0.001f){
		_debugRotationAngleDelta -= _debugRotationAngleDelta > 0 ? 0.005f : -0.005f;
		coordinate_.psi() = coordinate().psi() + _debugRotationAngleDelta;
	}

	if(interpolationTimer_){
		if(finishInterpolation_ == 0){
			float t = (frame_time() - interpolationTimer_)/(float)interpolationDuration_;
			bool splineEnd = false;
			if(t >= 1){
				if(replayIndex_ != -1){
					++replayIndex_;
					if(replayIndex_ < replayIndexMax_){
						setPath(replayIndex_);
						t = 0;
					}else{
						t = 1;
						if(unitFollow())
							splineEnd = true;
						erasePath();
					}
				}else{
					t = 1;
					erasePath();
				}
			}
	    
			if(!splineEnd){
				coordinate_.interpolateHermite(interpolationPoints_, t);
	//			environment->SetDofParams(coordinate_.dofParams());
				if(unitFollow())
					coordinate_.position() += unitFollow()->interpolatedPose().trans();
				else
					coordinate_.position() += spline_.position();
				coordinate_.check(false);
			}
		}
	}else{
		if(!directControl_ && directControlUnit()){
			followQuant(timeFactor);
		}else{
			distanceFactorNew_ = 1.0f;
			distanceFactor_ = distanceFactor_ + (distanceFactorNew_ - distanceFactor_) * 0.1f;
			float groundZ = 0;
			if(coordinate_.position().x >= 0 && coordinate_.position().x < vMap.H_SIZE &&
			coordinate_.position().y >= 0 && coordinate_.position().y < vMap.V_SIZE)
				groundZ = CameraCoordinate::height(coordinate_.position().xi(), coordinate_.position().yi()) + flyingHeight_;
			if(fabsf(coordinate_.position().z - groundZ) > 1.0f) 
				coordinate_.position().z += (groundZ - coordinate().position().z) * cameraRestriction_.CAMERA_KBD_ANGLE_SPEED_DELTA * timeFactor;
		}
	
		//zoom
		cameraZoomVelocity += cameraZoomForce*delta_time;
		coordinate_.distance() += cameraZoomVelocity.z*delta_time;
		coordinate_.position() += Vect3f(cameraZoomVelocity.x, cameraZoomVelocity.y, 0)*delta_time;
		if(GameOptions::instance().getBool(OPTION_CAMERA_ZOOM_TO_CURSOR))
			coordinate_.position() += Vect3f(cameraZoomVelocity.x / 2.f, cameraZoomVelocity.y / 2.f, 0)*delta_time;
		
		if(restricted() && !directControl_ && cameraZoomVelocity.z < -1.0f)
			cameraThetaForce += cameraRestriction_.CAMERA_KBD_ANGLE_SPEED_DELTA; //при зуме камера должна принимать макс. допустимый наклон
		
		//move
		cameraPositionVelocity += cameraPositionForce * timeFactor * 3.0f;
		
		float d = coordinate().distance()/cameraRestriction_.CAMERA_MOVE_ZOOM_SCALE*timeFactor;
		coordinate_.position() += Mat3f(-M_PI_2 + coordinate().psi(), Z_AXIS)*cameraPositionVelocity*d;
		
		//rotate
		cameraPsiVelocity   += cameraPsiForce * timeFactor * 3.0f;
		cameraThetaVelocity += cameraThetaForce * timeFactor * 3.0f;
		
		if(directControl_)
			directControlQuant(delta_time, timeFactor);
		else{
			coordinate_.psi() += cameraPsiVelocity * timeFactor;
			coordinate_.theta() += cameraThetaVelocity * timeFactor;
			if(!directControlUnit() || !unitFollowDown_)
				coordinate_.check(restricted());
		}

		cameraPsiVelocity   *= clamp(1.0f - cameraRestriction_.CAMERA_ANGLE_SPEED_DAMP * timeFactor, 0.0f, 1.0f);
		cameraThetaVelocity *= clamp(1.0f - cameraRestriction_.CAMERA_ANGLE_SPEED_DAMP * timeFactor, 0.0f, 1.0f);
		cameraZoomVelocity  *= clamp(1.0f - cameraRestriction_.zoomDamping*delta_time, 0.0f, 1.0f);
		cameraPositionVelocity *= clamp(1.0f - cameraRestriction_.CAMERA_SCROLL_SPEED_DAMP * timeFactor, 0.0f, 1.0f);
		
		cameraThetaForce = cameraPsiForce = 0;
		cameraZoomForce = Vect3f::ZERO;
		cameraPositionForce = Vect3f::ZERO;
	}
	
	if(cameraGroundColliding)
		collisionResolve();

	update();
}

void CameraManager::followQuant(float timeFactor)
{
	CameraCoordinate coordinateNew = coordinate_;

	if(unitFollowRotate_ && !directControlUnit()->attr().rotToTarget) {
		Vect3f dir(Vect3f::J_);
		directControlUnit()->interpolatedPose().xformVect(dir);
		coordinateNew.psi() = atan2f(dir.y, sqrtf(sqr(dir.x) + sqr(dir.z)));
		dir = Vect3f::I_;
		directControlUnit()->interpolatedPose().xformVect(dir);
		coordinateNew.fi() = atan2f(dir.z, sqrtf(sqr(dir.x) + sqr(dir.y)));
		cameraPsiForce = 0;
	}

	coordinateNew.position() = directControlUnit()->interpolatedPose().trans();

	Vect3f offset = directControlUnit()->attr().syndicateControlOffset;

	QuatF cameraOrientation(coordinateNew.theta() - M_PI_2, Vect3f::J, 0);
	cameraOrientation.premult(QuatF(coordinateNew.psi(), Vect3f::K, 0));
	cameraOrientation.xform(offset);
	
	coordinateNew.position() += offset;

	if(unitFollowDown_) {
		// Держим юнита в нижней части экрана.
		Vect3f offset(-100,0,0);
		QuatF(coordinateNew.psi(), Vect3f::K).xform(offset);
		coordinateNew.position() += offset;

		if(!directControlUnit()->rigidBody() || !directControlUnit()->rigidBody()->isUnit() || !safe_cast<RigidBodyUnit*>(directControlUnit()->rigidBody())->flyingMode()){
			float distanceNew = cameraRestriction().unitFollowDistance;
			float thetaNew = cameraRestriction().unitFollowTheta;
			if(directControlUnit()->attr().rotToTarget){
				distanceNew = cameraRestriction().unitHumanFollowDistance;
				thetaNew = cameraRestriction().unitHumanFollowTheta;
			}
			coordinateNew.theta() = thetaNew;
			coordinateNew.distance() = distanceNew;
		}
	}
	float timer = unitFollowTimer_.factor();
	if(timer < 1.f - FLT_EPS){
		distanceFactor_ = distanceFactor_ + (distanceFactorNew_ - distanceFactor_) * timer;
		coordinate_.interpolate(coordinate_, coordinateNew, timer);
		coordinateDelta_ = CameraCoordinate::ZERO;
	}
	else{
		distanceFactor_ = distanceFactorNew_;
		coordinateDelta_.interpolate(coordinateDelta_, CameraCoordinate::ZERO, timeFactor*cameraRestriction_.CAMERA_FOLLOW_AVERAGE_TAU);
		coordinateDelta_.interpolate(coordinateDelta_, (coordinateNew - coordinate_), timeFactor*cameraRestriction_.CAMERA_FOLLOW_AVERAGE_TAU);
		coordinate_ = coordinateNew - coordinateDelta_;
	}
}

void CameraManager::setDirectControlOffset(const Vect3f& offset, int time)
{
	unitTransitionTimer_.start(time);
	directControlOffsetPrev_ = directControlOffset_;
	directControlOffsetNew_ = offset;
	directControlFreeCameraStop_ = true;
}

void CameraManager::setDirectControlFreeCamera()
{
	setDirectControlOffset(Vect3f::ZERO, 1000);
	directControlFreeCameraStop_ = false;
	directControlFreeCamera_ = true;
}

void CameraManager::directControlQuant(float delta_time, float timeFactor)
{
	if(directControlUnitPrev()){
		directControlUnit_ = directControlUnitPrev();
		directControlUnitPrev_ = 0;
		directControlFreeCamera_ = false;
		coordinate_.fi() = 0.0f;
	}
	if(directControlUnit()){
		float deltaPsi = cameraPsiVelocity * timeFactor;
		Vect3f dir(Vect3f::J_);
		directControlUnit()->model()->GetNodePositionMats(directControlUnit()->attr().directControlNode).se().xformVect(dir);
		float psi(atan2(dir.y, dir.x));
		float theta(0);
		if(!interpolation_ && (!directControlFreeCamera_ || directControlFreeCameraStop_)){
			float psiMin(cycle(coordinate_.psi() + deltaPsi - psi, 2.0f * M_PI));
			if(psiMin > M_PI)
				psiMin = 2.0f * M_PI - psiMin;
			if(psiMin > cameraRestriction().directControlPsiMax){
				float velocity(cycle(psi - directControlPsi_, 2.0f * M_PI));
				if(velocity > M_PI)
					velocity = velocity - 2.0f * M_PI;
				velocity /= timeFactor + 1.0e-5f;
				if(psiMin < 1.5f * cameraRestriction().directControlPsiMax){
					if((fabsf(cameraPsiVelocity) > FLT_EPS) && (velocity / cameraPsiVelocity  < 1.0f)){
						float factor(2.0f * (psiMin / cameraRestriction().directControlPsiMax - 1.0f));
						cameraPsiVelocity = (1.0f - factor) * cameraPsiVelocity + factor * velocity;
						deltaPsi = cameraPsiVelocity * timeFactor;
					}
				}else{
					cameraPsiVelocity = velocity;
					deltaPsi = cycle(psi - coordinate_.psi(), 2.0f * M_PI);
					deltaPsi += 1.5f * SIGN(deltaPsi - M_PI) * cameraRestriction().directControlPsiMax;
				}
			}
			dir = Vect3f::K;
			directControlUnit()->model()->GetNodePositionMats(directControlUnit()->attr().directControlNode).se().xformVect(dir);
			QuatF(coordinate_.psi(), Vect3f::K).invXform(dir);
			theta = atan2(dir.x, dir.z);
		}
		directControlPsi_ = psi;
		coordinate_.psi() += deltaPsi;
		coordinate_.theta() += invertMouse_ * cameraThetaVelocity * timeFactor * cameraRestriction().directControlThetaFactor;
		if(!directControlFreeCamera_ || directControlFreeCameraStop_){
			float thetaNew = clamp(coordinate_.theta(), directControlUnit()->attr().directControlThetaMin + theta, directControlUnit()->attr().directControlThetaMax + theta);
			if(unitTransitionTimer_.started()){
				float factor = unitTransitionTimer_.factor();
				coordinate_.theta() = (1.0f - factor) * coordinate_.theta() + factor * thetaNew;
			}else
				coordinate_.theta() = thetaNew;
		}
	
		CameraCoordinate coordinateNew = coordinate_;

		directControlUnit()->model()->Update();
		if(interpolation_){
			coordinateNew.theta() = M_PI_2;
			prevPosition_ = directControlUnit()->model()->GetNodePositionMats(directControlUnit()->attr().directControlNode).trans();
			deltaPosePrev_ = Vect3f::ZERO;
		}else{
			Vect3f deltaPose_(directControlUnit()->model()->GetNodePositionMats(directControlUnit()->attr().directControlNode).trans());
			deltaPose_ -= prevPosition_;
			deltaPose_ /= delta_time;
			deltaPosePrev_.interpolate(deltaPosePrev_, deltaPose_, cameraRestriction().directControlRelaxation);
			prevPosition_.scaleAdd(deltaPosePrev_, delta_time);
		}

		coordinateNew.position() = prevPosition_;

		if(directControlUnit()->attr().directControlNode == 0)
			coordinateNew.position().z += directControlUnit()->height() * 0.7f;
			
		if(unitTransitionTimer_.started()){
			float factor = unitTransitionTimer_.factor();
			directControlOffset_.interpolate(directControlOffsetPrev_, directControlOffsetNew_, factor);
			if(1.0f - factor < FLT_EPS)
				unitTransitionTimer_.stop();
		}else if(directControlFreeCameraStop_){
			directControlFreeCameraStop_ = false;
			directControlFreeCamera_ = false;
		}

		Vect3f offset = directControlOffset_;
		
		QuatF cameraOrientation(coordinateNew.theta() - M_PI_2, Vect3f::J, 0);
		cameraOrientation.premult(QuatF(coordinateNew.psi(), Vect3f::K, 0));
		cameraOrientation.xform(offset);
		directControlShootingDirection_ = offset;
		directControlShootingDirection_.normalize();
		
		coordinateNew.position() += offset;

		if(directControlUnit()->attr().rotToTarget)
			coordinateNew.distance() = cameraRestriction().unitHumanFollowDistance;
		else
			coordinateNew.distance() = cameraRestriction().unitFollowDistance;

		float factor = unitFollowTimer_.factor();
		interpolation_ = factor < 1.f - FLT_EPS;
		if(interpolation_){
			distanceFactor_ = distanceFactor_ + (distanceFactorNew_ - distanceFactor_) * factor;
			coordinate_.interpolate(coordinate_, coordinateNew, factor);
		}else{
			distanceFactor_ = distanceFactorNew_;
			coordinate_ = coordinateNew;
		}
	}
	else
		disableDirectControl();

	if(!directControlFreeCamera_ || directControlFreeCameraStop_)
		UI_LogicDispatcher::instance().updateAimPosition();
}

const Vect3f& CameraManager::directControlShootingDirection() const
{
	return directControlShootingDirection_;	
}

void CameraManager::SetCameraFollow(const UnitInterface* unit, int transitionTime)
{
	MTAuto lock(cameraLock_);
	if(directControl_){
		directControlUnitPrev_ = directControlUnit();
		if(directControlUnitPrev())
			directControlFreeCamera_ = true;
	}
	coordinate_.fi() = 0.0f;
	directControlUnit_ = unit;
	unitFollowTimer_.start(transitionTime + 1);
	interpolation_ = true;
	if(unit && unit->attr().isActing() && safe_cast<const UnitActing*>(unit)->isSyndicateControl())
		distanceFactorNew_ = unit->attr().syndicatControlCameraRestrictionFactor;
	else
		distanceFactorNew_ = 1.0f;
}

void CameraManager::setUnitFollowMode(bool unitFollowDown, bool unitFollowRotate)
{
	unitFollowDown_ = unitFollowDown;
	unitFollowRotate_ = unitFollowRotate;
}

int CameraManager::startReplayPath(int duration, int cycles, UnitInterface* unit)
{
	interpolationDuration_ = duration; 

	if(spline_.empty())
		return 0;
	
	MTAuto lock(cameraLock_);
	unitFollow_ = unit;
	
	if(spline_.size() == 1){
		if(!duration){
			setCoordinate(spline_[0]);
			return 0;
		}
		else{
			CameraCoordinate coord = spline_[0];
			spline_.spline().push_back(coord);
		}
	}

	if(firstPointAdded_ && cycles > 1)
		replayIndexMax_ = cycles*(spline_.size() - 1);
	else{
		if(spline_.cycled)
			replayIndexMax_ = cycles*spline_.size();
		else
			replayIndexMax_ = cycles*spline_.size() - 1;
	}

	setPath(replayIndex_ = 0);
	return replayIndexMax_*interpolationDuration_;
}

void CameraManager::stopReplayPath()
{
	MTAuto lock(cameraLock_);
	replayIndex_ = -1;
	if(finishInterpolation_ == 0)
		finishInterpolation_ = 4;
	unitFollow_ = 0;
}


int CameraManager::skipFirstIndex(int index)
{
	int splineSize = spline_.size();
	return (index <= 0) ? 0 : ((index - 1) % (splineSize - 1) + 1);
}

void CameraManager::setPath(int index) 
{ 
	xassert(!spline_.empty());

	if(replayIndexMax_ >= spline_.size()){ // Зацикленное повторение последовательности
		if(firstPointAdded_){
			interpolationPoints_[0] = spline_[skipFirstIndex(index - 1)];
			interpolationPoints_[1] = spline_[skipFirstIndex(index)];
			interpolationPoints_[2] = spline_[skipFirstIndex(index + 1)];
			interpolationPoints_[3] = spline_[skipFirstIndex(index + 2)];
		}
		else{
			index %= spline_.size();

			interpolationPoints_[0] = spline_[(index - 1 + spline_.size()) % spline_.size()];
			interpolationPoints_[1] = spline_[index];
			interpolationPoints_[2] = spline_[(index + 1) % spline_.size()];
			interpolationPoints_[3] = spline_[(index + 2) % spline_.size()];
		}
	}
	else{
		interpolationPoints_[0] = spline_[clamp(index - 1, 0, spline_.size() - 1)];
		interpolationPoints_[1] = spline_[index];
		interpolationPoints_[2] = spline_[clamp(index + 1, 0, spline_.size() - 1)];
		interpolationPoints_[3] = spline_[clamp(index + 2, 0, spline_.size() - 1)];
	}

	interpolationPoints_[0].uncycle(interpolationPoints_[1]);
	interpolationPoints_[2].uncycle(interpolationPoints_[1]);
	interpolationPoints_[3].uncycle(interpolationPoints_[1]);

	interpolationTimer_ = frame_time();
	finishInterpolation_ = 0;
}

void CameraManager::erasePath() 
{ 
	stopReplayPath();
	spline_.clear(); 
	coordinate_.psi() = cycle(coordinate().psi(), 2*M_PI);
	coordinate_.fi() = 0.0f;
	coordinate_.focus() = HardwareCameraFocus;
}


void CameraManager::addSpline(CameraSpline* spline)
{
	splines_.push_back(spline);
}


void CameraManager::selectSpline(CameraSpline* spline)
{
	xassert(std::find(splines_.begin(), splines_.end(), spline) != splines_.end());
	
}


void CameraManager::loadPath(const CameraSpline& spline, bool addCurrentPosition)
{
	if(isPlayingBack())
        stopReplayPath();		
	spline_ = spline;
	loadedSpline_ = &spline;
	if(addCurrentPosition){
		CameraCoordinate coord = coordinate();
		coord.position() -= spline_.position();
		spline_.spline().insert(spline_.spline().begin(), coord);
	}
	firstPointAdded_ = addCurrentPosition;
}


CameraSpline* CameraManager::findSpline(const char* name)
{
	CameraSplines::const_iterator si;
	FOR_EACH(splines_, si)
		if(strcmp((*si)->name(), name) == 0)
			return *si;
	return 0;
}

const char* CameraManager::popupCameraSplineName() const
{
	vector<const char*> items;
	CameraSplines::const_iterator si;
	FOR_EACH(splines_, si)
		items.push_back((*si)->name());

	return popupMenu(items);
}

//-----------------------------------
void CameraManager::startOscillation(int duration, float factor, const Vect3f& vector)
{
	if(vector.eq(Vect3f::ZERO)){
		Vect3f position;
		position.setSpherical(coordinate().psi(), coordinate().theta(), coordinate().distance());
		position.normalize(factor);
		explodingVector_ = position;
	}
	else
		explodingVector_ = vector;

	oscillatingTimer_.start(explodingDuration_ = duration);
}

void CameraManager::reset()
{
	oscillatingTimer_.stop();
	stopReplayPath();
}

void CameraManager::showEditor()
{
	xassert(isUnderEditor());
	if(!splines_.empty())
		for(int si = 0; si < splines_.size(); si++) 
			splines_[si]->showInfo();
}

void drawWireModel(const Vect3f* points, int numPoints, const Se3f& pos, Color4c color, float scale = 1.0f)
{
	for(int i = 1; i < numPoints; i += 2){
		Vect3f a = points[i-1] * scale;
		Vect3f b = points[i] * scale;
		pos.xformPoint(a);
		pos.xformPoint(b);
		gb_RenderDevice->DrawLine(a, b, color);
	}
}

static void showInfoPoint(const CameraCoordinate& coord, bool active, bool selected, bool last)
{
	Color4c pointsColor(active ? ACTIVE_COLOR : DEFAULT_COLOR);
	Color4c linesColor(active ? ACTIVE_TRANSPARENT_COLOR : DEFAULT_TRANSPARENT_COLOR);
	Color4c selectedColor(SELECTED_COLOR);

	float crossSize = 10.0f;

	float psi = coord.psi();
	float theta = coord.theta();

	Vect3f zAxis(sinf(theta) * cosf(psi), sinf(theta) * sinf(psi), cosf(theta));
	Vect3f xAxis(cosf(psi + M_PI*0.5f), sinf(psi + M_PI * 0.5f), 0.0f);
	Vect3f yAxis;
	yAxis.cross(zAxis, xAxis);
	zAxis = -zAxis;

	Mat3f rotation(xAxis.x, zAxis.x, yAxis.x,
				   xAxis.y, zAxis.y, yAxis.y,
				   xAxis.z, zAxis.z, yAxis.z);

	Vect3f offset = -zAxis * coord.distance();

	Color4c color(selected ? selectedColor : pointsColor);
	Color4c transparentColor(selected ? selectedColor : linesColor);

	Vect3f pos = coord.position();
	Vect3f pos3d = To3D(pos);
	gb_RenderDevice->DrawLine(pos3d, pos, color);
	gb_RenderDevice->DrawLine(pos, pos + offset, transparentColor);

	gb_RenderDevice->DrawLine(pos3d - Vect3f::I * crossSize, pos3d + Vect3f::I * crossSize, color);
	gb_RenderDevice->DrawLine(pos3d - Vect3f::J * crossSize, pos3d + Vect3f::J * crossSize, color);

	if(last){
		static const Vect3f points[112] = {/*{{{*/
			Vect3f(0.375168f, 0.519649f, -0.376589f), Vect3f(0.375168f, -0.489539f, -0.376589f),
			Vect3f(0.375168f, 0.519649f, -0.376589f), Vect3f(-0.375168f, 0.519649f, -0.376589f),
			Vect3f(0.375168f, 0.519649f, -0.376589f), Vect3f(0.375169f, 0.519649f, 0.373748f),
			Vect3f(0.375168f, -0.489539f, -0.376589f), Vect3f(-0.375169f, -0.489539f, -0.376589f),
			Vect3f(0.375168f, -0.489539f, -0.376589f), Vect3f(0.375168f, -0.489539f, 0.373748f),
			Vect3f(-0.375169f, -0.489539f, -0.376589f), Vect3f(-0.375168f, 0.519649f, -0.376589f),
			Vect3f(-0.375169f, -0.489539f, -0.376589f), Vect3f(-0.375169f, -0.489539f, 0.373748f),
			Vect3f(-0.375168f, 0.519649f, -0.376589f), Vect3f(-0.375168f, 0.519649f, 0.373748f),
			Vect3f(0.375169f, 0.519649f, 0.373748f), Vect3f(0.375168f, -0.489539f, 0.373748f),
			Vect3f(0.375169f, 0.519649f, 0.373748f), Vect3f(-0.375168f, 0.519649f, 0.373748f),
			Vect3f(0.375168f, -0.489539f, 0.373748f), Vect3f(-0.375169f, -0.489539f, 0.373748f),
			Vect3f(-0.375169f, -0.489539f, 0.373748f), Vect3f(-0.375168f, 0.519649f, 0.373748f),
			Vect3f(-0.159892f, 0.613776f, 0.158472f), Vect3f(0.159892f, 0.613776f, 0.158472f),
			Vect3f(-0.159892f, 0.613776f, 0.158472f), Vect3f(-0.159892f, 0.613776f, -0.161312f),
			Vect3f(0.159892f, 0.613776f, 0.158472f), Vect3f(0.159892f, 0.613776f, -0.161312f),
			Vect3f(-0.159892f, 0.613776f, -0.161312f), Vect3f(0.159892f, 0.613776f, -0.161312f),
			Vect3f(-0.159892f, 0.613776f, -0.161312f), Vect3f(-0.505623f, 1.178541f, -0.507043f),
			Vect3f(0.505623f, 1.178541f, -0.507043f), Vect3f(-0.505623f, 1.178541f, -0.507043f),
			Vect3f(0.159892f, 0.613776f, -0.161312f), Vect3f(0.505623f, 1.178541f, -0.507043f),
			Vect3f(0.505623f, 1.178541f, -0.507043f), Vect3f(0.505623f, 1.178541f, 0.504203f),
			Vect3f(0.159892f, 0.613776f, 0.158472f), Vect3f(0.505623f, 1.178541f, 0.504203f),
			Vect3f(-0.159892f, 0.613776f, 0.158472f), Vect3f(-0.505623f, 1.178541f, 0.504203f),
			Vect3f(-0.505623f, 1.178541f, -0.507043f), Vect3f(-0.505623f, 1.178541f, 0.504203f),
			Vect3f(0.505623f, 1.178541f, 0.504203f), Vect3f(-0.505623f, 1.178541f, 0.504203f),
			Vect3f(-0.264148f, -0.133700f, 1.124388f), Vect3f(-0.264148f, -0.024287f, 0.860241f),
			Vect3f(0.264148f, -0.133700f, 1.124388f), Vect3f(0.264148f, -0.024287f, 0.860241f),
			Vect3f(-0.264148f, -0.024287f, 0.860241f), Vect3f(-0.264148f, -0.133700f, 0.596093f),
			Vect3f(0.264148f, -0.024287f, 0.860241f), Vect3f(0.264148f, -0.133700f, 0.596093f),
			Vect3f(-0.264148f, -0.133700f, 0.596093f), Vect3f(-0.264148f, -0.397848f, 0.486680f),
			Vect3f(0.264148f, -0.133700f, 0.596093f), Vect3f(0.264148f, -0.397848f, 0.486680f),
			Vect3f(-0.264148f, -0.397848f, 0.486680f), Vect3f(-0.264148f, -0.661995f, 0.596093f),
			Vect3f(0.264148f, -0.397848f, 0.486680f), Vect3f(0.264148f, -0.661995f, 0.596093f),
			Vect3f(-0.264148f, -0.661995f, 0.596093f), Vect3f(-0.264148f, -0.771409f, 0.860241f),
			Vect3f(0.264148f, -0.661995f, 0.596093f), Vect3f(0.264148f, -0.771409f, 0.860241f),
			Vect3f(-0.264148f, -0.771409f, 0.860241f), Vect3f(-0.264148f, -0.661995f, 1.124388f),
			Vect3f(0.264148f, -0.771409f, 0.860241f), Vect3f(0.264148f, -0.661995f, 1.124388f),
			Vect3f(-0.264148f, -0.661995f, 1.124388f), Vect3f(-0.264148f, -0.397848f, 1.233802f),
			Vect3f(0.264148f, -0.661995f, 1.124388f), Vect3f(0.264148f, -0.397847f, 1.233802f),
			Vect3f(-0.264148f, -0.133700f, 1.124388f), Vect3f(-0.264148f, -0.397848f, 1.233802f),
			Vect3f(0.264148f, -0.133700f, 1.124388f), Vect3f(0.264148f, -0.397847f, 1.233802f),
			Vect3f(0.232474f, 0.453242f, 1.189009f), Vect3f(0.232474f, 0.685716f, 1.092715f),
			Vect3f(-0.232474f, 0.453242f, 1.189009f), Vect3f(-0.232474f, 0.685717f, 1.092715f),
			Vect3f(0.232474f, 0.453242f, 1.189009f), Vect3f(0.232474f, 0.220768f, 1.092716f),
			Vect3f(-0.232474f, 0.453242f, 1.189009f), Vect3f(-0.232474f, 0.220767f, 1.092715f),
			Vect3f(0.232474f, 0.220768f, 1.092716f), Vect3f(0.232474f, 0.124473f, 0.860241f),
			Vect3f(-0.232474f, 0.220767f, 1.092715f), Vect3f(-0.232474f, 0.124473f, 0.860241f),
			Vect3f(0.232474f, 0.124473f, 0.860241f), Vect3f(0.232474f, 0.220767f, 0.627766f),
			Vect3f(-0.232474f, 0.124473f, 0.860241f), Vect3f(-0.232474f, 0.220767f, 0.627766f),
			Vect3f(0.232474f, 0.220767f, 0.627766f), Vect3f(0.232474f, 0.453242f, 0.531472f),
			Vect3f(-0.232474f, 0.220767f, 0.627766f), Vect3f(-0.232474f, 0.453242f, 0.531472f),
			Vect3f(0.232474f, 0.453242f, 0.531472f), Vect3f(0.232474f, 0.685717f, 0.627766f),
			Vect3f(-0.232474f, 0.453242f, 0.531472f), Vect3f(-0.232474f, 0.685717f, 0.627766f),
			Vect3f(0.232474f, 0.685717f, 0.627766f), Vect3f(0.232474f, 0.782011f, 0.860241f),
			Vect3f(-0.232474f, 0.685717f, 0.627766f), Vect3f(-0.232474f, 0.782011f, 0.860241f),
			Vect3f(0.232474f, 0.782011f, 0.860241f), Vect3f(0.232474f, 0.685716f, 1.092715f),
			Vect3f(-0.232474f, 0.782011f, 0.860241f), Vect3f(-0.232474f, 0.685717f, 1.092715f),
		};/*}}}*/
		drawWireModel(points, sizeof(points) / sizeof(points[0]),
					  Se3f(rotation, pos + offset), color, 10.0f);
	}
	else{
		static const Vect3f points[38] = {/*{{{*/
			Vect3f(-0.317010f, 0.000234f, 0.235646f), Vect3f(0.317011f, 0.000234f, 0.235646f),
			Vect3f(-0.317010f, 0.000234f, 0.235646f), Vect3f(-0.317010f, 0.000234f, -0.239870f),
			Vect3f(0.317011f, 0.000234f, 0.235646f), Vect3f(0.317010f, 0.000234f, -0.239870f),
			Vect3f(-0.317010f, 0.000234f, -0.239870f), Vect3f(0.317010f, 0.000234f, -0.239870f),
			Vect3f(-0.317010f, 0.000234f, -0.239870f), Vect3f(-1.002474f, 1.119966f, -0.753968f),
			Vect3f(1.002475f, 1.119966f, -0.753968f), Vect3f(-1.002474f, 1.119966f, -0.753968f),
			Vect3f(0.317010f, 0.000234f, -0.239870f), Vect3f(1.002475f, 1.119966f, -0.753968f),
			Vect3f(1.002475f, 1.119966f, -0.753968f), Vect3f(1.002476f, 1.119966f, 0.749744f),
			Vect3f(0.317011f, 0.000234f, 0.235646f), Vect3f(1.002476f, 1.119966f, 0.749744f),
			Vect3f(-0.317010f, 0.000234f, 0.235646f), Vect3f(-1.002475f, 1.119966f, 0.749744f),
			Vect3f(-1.002474f, 1.119966f, -0.753968f), Vect3f(-1.002475f, 1.119966f, 0.749744f),
			Vect3f(1.002476f, 1.119966f, 0.749744f), Vect3f(-1.002475f, 1.119966f, 0.749744f),
			Vect3f(-0.200495f, 1.119966f, 0.749744f), Vect3f(0.200495f, 1.119966f, 0.749744f),
			Vect3f(0.200495f, 1.119966f, 0.749744f), Vect3f(0.200495f, 1.119966f, 1.081420f),
			Vect3f(-0.200495f, 1.119966f, 0.749744f), Vect3f(-0.200495f, 1.119966f, 1.081420f),
			Vect3f(0.200495f, 1.119966f, 1.081420f), Vect3f(0.379570f, 1.119966f, 1.081420f),
			Vect3f(-0.200495f, 1.119966f, 1.081420f), Vect3f(-0.379569f, 1.119966f, 1.081420f),
			Vect3f(-0.379569f, 1.119966f, 1.081420f), Vect3f(0.000000f, 1.119966f, 1.465300f),
			Vect3f(0.379570f, 1.119966f, 1.081420f), Vect3f(0.000000f, 1.119966f, 1.465300f),
		};/*}}}*/
		drawWireModel(points, sizeof(points) / sizeof(points[0]),
					  Se3f(rotation, pos + offset), color, 10.0f);
	}
}

void CameraSpline::showEditor(int selectedNode) const
{
	CameraCoordinate prev_cc;
	CameraCoordinate cc;
	

	int splineSize = size();
	int start = 0;

	for(int i = start; i < splineSize; i++){
		prev_cc = (*this)[i];
		prev_cc.position() += position();
		showInfoPoint(prev_cc, selected(), i == selectedNode, i == splineSize - 1);
	}

}

void CameraSpline::showDebug() const
{
	showInfo();
}

void CameraSpline::showInfo() const
{
	if(!isUnderEditor() || editorVisual().isVisible(objectClass())){
		float crossSize = 15.0f;
		gb_RenderDevice->DrawLine(position() - Vect3f::I * crossSize, position() + Vect3f::I * crossSize, selected() ? ACTIVE_COLOR : ACTIVE_TRANSPARENT_COLOR);
		gb_RenderDevice->DrawLine(position() - Vect3f::J * crossSize, position() + Vect3f::J * crossSize, selected() ? ACTIVE_COLOR : ACTIVE_TRANSPARENT_COLOR);

		if(!coordinates_.empty()){
			CameraCoordinate coord = coordinates_.back();
			coord.position() += position();
			showInfoPoint(coord , selected(), false, true);
		}
		else
			showInfoPoint(CameraCoordinate(position() + Vect3f::K * 30.0f, 0.0f, M_PI / 2.0f, 0.f, 0.0f), selected(), false, true);

		CameraCoordinate cc, prev_cc;


		Color4c pointsColor(ACTIVE_COLOR);
		Color4c linesColor(ACTIVE_TRANSPARENT_COLOR);

		if(selected()){
			if(!empty()&&(size()>1)){ 
				CameraCoordinate coords[4];
				int i;
				for(i = 0; i < (size()-1); i++){
					prev_cc = (*this)[i];
					if (i == 0) 
						if (cycled)
							coords[0] = (*this)[size()-1];
						else
							coords[0] = (*this)[0];
					else coords[0] = (*this)[i-1];

					coords[1] = (*this)[i];
					coords[2] = (*this)[i+1];

					if (i >= size()-2) 
						if (cycled)
							coords[3] = (*this)[0];
						else
							coords[3] = (*this)[size()-1];
					else coords[3] = (*this)[i+2];
	        
					for (int j = 0; j < 10; j++) {
						cc.interpolateHermite(coords, 0.1 + (float)0.1*j);
						gb_RenderDevice->DrawLine(position() + prev_cc.position(), position() + cc.position(), linesColor);
						prev_cc = cc;
					}
				}

				if(cycled){
					coords[0] = (*this)[size()-2];
					coords[1] = (*this)[size()-1];
					coords[2] = (*this)[0];
					coords[3] = (*this)[1];
	        
					prev_cc = (*this)[size()-1];

					for (int j = 0; j < 10; j++) {
						cc.interpolateHermite(coords, 0.1 + (float)0.1*j);
						gb_RenderDevice->DrawLine(position() + prev_cc.position(), position() + cc.position(), linesColor);
						prev_cc = cc;
					}
				}
			}
		}
	}
}

void CameraManager::deleteSelected()
{
	CameraSplines::iterator it;
	for(it = splines_.begin(); it != splines_.end();){
		CameraSpline* spline = *it;
		
		if(spline->selected())
			it = splines_.erase(it);
		else
			++it;
	}
}

int CameraSpline::findNearestCoordinate(const Vect2f& pos) const
{
	return -1;
}

void CameraManager::deleteSpline(CameraSpline* spline)
{
	CameraSplines::iterator it = std::find(splines_.begin(), splines_.end(), spline);

	if(it == splines_.end()){
		xassert(0);
		return;
	}
	splines_.erase(it);
}

void CameraSpline::erasePoint(int point)
{
	xassert(point >= 0 && point < coordinates_.size());
	coordinates_.erase(coordinates_.begin() + point);
}

void CameraSpline::setPointPosition(int point, const Vect3f& pos)
{
	xassert(point >= 0 && point < coordinates_.size());
	coordinates_[point].position() = pos;
}

void CameraSpline::addPointAfter(int index, const CameraCoordinate& coord)
{
	xassert(index >= 0 && index < size());
	coordinates_.insert(coordinates_.begin() + index + 1, coord);
}

void CameraCoordinate::serialize(Archive& ar) 
{
	ar.serialize(position_, "position", "Позиция");
	ar.serialize(psi_, "psi", "Поворот вокруг X");
	ar.serialize(theta_, "theta", "Поворот вокруг Z");
	ar.serialize(fi_, "fi", "Поворот вокруг Y");
	ar.serialize(distance_, "distance", "Дистанция");
	ar.serialize(focus_, "focus", "Фокус");
	ar.serialize(dofParams_, "dofParams", 0);
}

void CameraSpline::setPose(const Se3f& pose, bool init)
{
	__super::setPose(pose, init);
}

void CameraSpline::serialize(Archive& ar) 
{
	ar.serialize(name_, "name", "&Имя");
	ar.serialize(stepDuration_, "stepDuration", "Длительность");
	ar.serialize(cycled, "cycled", "Замкнутый");
	ar.serialize(coordinates_, "spline", "Координаты");
	ar.serialize(pose_, "pose", 0);
}

void CameraSpline::clear() 
{
	coordinates_.clear(); 
}

CameraSpline::CameraSpline() : name_("splineXX")
, stepDuration_(1000)
, cycled(false)
{
	pose_ = Se3f::ID;
	radius_ = 10.0f;
}

void CameraManager::serialize(Archive& ar) 
{
	if(ar.filter(SERIALIZE_WORLD_DATA))
		ar.serialize(splines_, "cameraSplines", "Камеры");

	ar.serialize(ownCameraRestriction_, "selfCameraRestriction", "&использовать собственные ограничения камеры");
	ar.serialize(cameraBorder_, "cameraBorder", "границы выезда за край миры");
	if(ownCameraRestriction_){
		ar.serialize(cameraRestriction_, "cameraRestriction", "Ограничения камеры");
	}
	else if(ar.isInput()){
		cameraRestriction_ = GlobalAttributes::instance().cameraRestriction;
	}
}

void CameraManager::setSkipCutScene(bool skip) 
{ 
	cutSceneSkipped_ = skip; 
}

void CameraManager::collisionResolve()
{
	Vect3f position;
	position.setSpherical(coordinate().psi(), coordinate().theta(), coordinate().distance() * distanceFactor_);
	position += coordinate().position();

	if(oscillatingTimer_.busy()){
		float t = (float)(explodingDuration_ - oscillatingTimer_.timeRest())/1000;
		Vect3f value = explodingVector_*sin(4*M_PI*t)*(1/(sqr(t)+0.1f)-0.7f);
		position += value;
	}

	float penetration = collisionCamera(position, environment->GetGameFrustrumZMin());
	if(penetration > FLT_EPS){
		if(directControl_)
			if(directControlFreeCamera_){
				float height = cosf(coordinate_.theta()) * coordinate_.distance() * distanceFactor_;
				float dist = sinf(coordinate_.theta()) * coordinate_.distance() * distanceFactor_;
				coordinate_.theta() = atan2f(dist, height + penetration);
			}else
				callcDistance(penetration);
		else
			callcTheta(penetration);
	}
}

float CameraManager::getSphereHeight(const Vect3f& position, float radius, float x, float y)
{
	float slx = sqr(position.x - x);
	float sly = sqr(position.y - y);

	if(slx < FLT_EPS && sly < FLT_EPS)
		return position.z - radius;

	float lz = sqrt(sqr(radius) - slx - sly);

	return position.z - lz;
}

float CameraManager::collisionCamera(const Vect3f& position, float radius )
{
	float penetrationMax = 0;

	int hMax = (vMap.H_SIZE >> kmGrid) - 1;
	int vMax = (vMap.V_SIZE >> kmGrid) - 1;
	float factor = 1.f/(1 << kmGrid);

	int xl = clamp(round((position.x - radius)*factor), 0, hMax);
	int xr = clamp(round((position.x + radius)*factor), 0, hMax);
	int yl = clamp(round((position.y - radius)*factor), 0, vMax);
	int yr = clamp(round((position.y + radius)*factor), 0, vMax);

	for(int i = xl; i <= xr; i++){
		float x = i << kmGrid;
		for(int j = yl; j <= yr; j++) {
			float y = j << kmGrid;
			float z = vMap.gVBuf[vMap.offsetGBuf(i,j)];

			if((sqr(position.x - x)+sqr(position.y - y)) < sqr(radius)){
				float sz = getSphereHeight(position, radius, x, y);
				if((z - sz) > penetrationMax)
					penetrationMax = z - sz;
			}
		}
	}
	
	return penetrationMax;
}

void CameraManager::callcTheta(float hDelta)
{
	float height = cos(coordinate_.theta()) * coordinate_.distance() * distanceFactor_;
	float radiusOld = sqrt(sqr(coordinate_.distance() * distanceFactor_) - sqr(height));
	coordinate_.theta() = atan2(radiusOld, (height+hDelta));
	coordinate_.distance() = sqrt(sqr(height+hDelta) + sqr(radiusOld)) / distanceFactor_;
}

void CameraManager::callcDistance(float hDelta)
{
	float cosTheta = -cosf(coordinate_.theta());
	if(cosTheta > FLT_EPS){
		float offsetNorm = directControlOffset_.norm();
		if(coordinate_.distance() * distanceFactor_ > offsetNorm){
			coordinate_.distance() -= hDelta / (cosTheta * distanceFactor_);
			if(coordinate_.distance() * distanceFactor_ < offsetNorm)
				coordinate_.distance() = offsetNorm / distanceFactor_;
		}
	}
}

Camera* CameraManager::GetCamera()
{
	return camera_;
}

string CameraManager::splinesComboList() const
{
	string comboList;
	CameraSplines::const_iterator i;
	FOR_EACH(splines_, i)
		if(strcmp((*i)->name(), "") != 0){
			comboList += (*i)->name();
			comboList += "|";
		}
	return comboList;
}

void CameraManager::getCameraAxis( Vect3f& axisZ, Vect3f& axisX, Vect3f& axisY )
{
	axisZ.setSpherical(coordinate().psi(), coordinate().theta(), coordinate().distance());
	axisZ.normalize();

	axisX = Vect3f(cos(coordinate().psi()+M_PI_2), sin(coordinate().psi()+M_PI_2), 0.0f);

	axisY.cross(axisZ, axisX);	
	axisY = -axisY;
}

float CameraManager::correctedFocus(float focus, Camera* camera) const
{
	float x2 = 4.0f / 3.0f;
	float x1 = (camera && camera->GetRenderTarget())
		? (float(camera->GetRenderTarget()->GetWidth()) / float(camera->GetRenderTarget()->GetHeight()))
		: (float(gb_RenderDevice->GetSizeX()) / float(gb_RenderDevice->GetSizeY()));

	return focus * x2 / x1;
}

void CameraManager::SetDefaultCoordinate()
{
	coordinate_.theta() = GlobalAttributes::instance().cameraDefaultTheta_;
	coordinate_.distance() = GlobalAttributes::instance().cameraDefaultDistance_;
}

void CameraManager::splineToDefaultCoordinate( float time, const Vect3f& position )
{
	CameraSpline spline;
	CameraCoordinate coord = coordinate();
	coord.theta() = GlobalAttributes::instance().cameraDefaultTheta_;
	coord.distance() = GlobalAttributes::instance().cameraDefaultDistance_;

	coord.position() = Vect3f(position.x, position.y, CameraCoordinate::height(position.xi(), position.yi()));
	
	spline.push_back(coord);
	loadPath(spline, true);
	startReplayPath(time, 1);
}

void CameraManager::disableDirectControl()
{
	MTAuto lock(cameraLock_);
	if(directControl_){
		directControl_ = false;
		directControlFreeCameraStop_ = false;
		directControlFreeCamera_ = false;
		unitTransitionTimer_.stop();
		directControlUnitPrev_ = 0;
		splineToDefaultCoordinate(1000.0f, directControlUnit() ? directControlUnit()->position() : coordinate_.position());
		directControlUnit_ = 0;
		coordinate_.fi() = 0.0f;
	}else
		SetCameraFollow();
}

void CameraManager::enableDirectControl(const UnitInterface* unit, int transitionTime)
{
	MTAuto lock(cameraLock_);
	SetCameraFollow(unit, transitionTime);
	directControl_ = true;
	directControlUnitPrev_ = 0;
	directControlFreeCamera_ = false;
	directControlFreeCameraStop_ = false;
	unitTransitionTimer_.stop();
	if(unit)
		directControlOffset_ = safe_cast<const UnitActing*>(unit)->directControlOffset();
}

namespace{

struct BlackRectangle{
	float x1, y1, x2, y2;
};

};

void CameraManager::drawBlackBars(float opacity)
{
	if(isUnderEditor() || aspect() - FLT_COMPARE_TOLERANCE > 4.0f / 3.0f)
		return;

	gb_RenderDevice->SetNoMaterial(ALPHA_BLEND, MatXf::ID);

	int oldAlphaBlend = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHABLENDENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

	int oldAlphaTest  = gb_RenderDevice3D->GetRenderState(D3DRS_ALPHATESTENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

	int oldZEnable    = gb_RenderDevice3D->GetRenderState(D3DRS_ZENABLE);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE, FALSE);	

	float topSize = 0.5f + frustumClip().ymin();
	float bottomSize = 0.5f - frustumClip().ymax();

	float leftSize = 0.5f + frustumClip().xmin();
	float rightSize = 0.5f - frustumClip().xmax();

	float width = gb_RenderDevice->GetSizeX();
	float height = gb_RenderDevice->GetSizeY();

	BlackRectangle rectangles[] = {
	    { -1.0f, -1.0f, width + 1.0f, height * topSize },
		{ -1.0f, -1.0f, width * leftSize, height + 1.0f},

		{ width * (1.0f - rightSize), -1.0f, width + 1.0f, height  + 1.0f},
	    { -1.0f, height * (1.0f - bottomSize), width + 1.0f, height + 1.0f},
	};

	const int numRects = sizeof(rectangles) / sizeof(rectangles[0]);
	const int numPoints = numRects * 2 * 3;
	Color4c color(0, 0, 0, round(opacity * 255.0f));

	int npoint = 0;
	cVertexBuffer<sVertexXYZWD>* buffer = gb_RenderDevice->GetBufferXYZWD();
	sVertexXYZWD* pv = buffer->Lock(numPoints);
	for(BlackRectangle* it = &rectangles[0]; it != &rectangles[0] + numRects; ++it){
		BlackRectangle& p = *it;

		pv[0].x=p.x1; pv[0].y=p.y1; pv[0].z=0.001f; pv[0].w=0.001f; pv[0].diffuse = color;
		pv[1].x=p.x1; pv[1].y=p.y2; pv[1].z=0.001f; pv[1].w=0.001f; pv[1].diffuse = color;
		pv[2].x=p.x2; pv[2].y=p.y1; pv[2].z=0.001f; pv[2].w=0.001f; pv[2].diffuse = color;

		pv[3].x=p.x2; pv[3].y=p.y1; pv[3].z=0.001f; pv[3].w=0.001f; pv[3].diffuse = color;
		pv[4].x=p.x1; pv[4].y=p.y2; pv[4].z=0.001f; pv[4].w=0.001f; pv[4].diffuse = color;
		pv[5].x=p.x2; pv[5].y=p.y2; pv[5].z=0.001f; pv[5].w=0.001f; pv[5].diffuse = color;
		pv += 6;		
	}
	buffer->Unlock(numPoints);
	buffer->DrawPrimitive(PT_TRIANGLELIST, numPoints / 3);

	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHABLENDENABLE, oldAlphaBlend);
	gb_RenderDevice3D->SetRenderState(D3DRS_ALPHATESTENABLE, oldAlphaTest);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZENABLE, oldZEnable);
}
