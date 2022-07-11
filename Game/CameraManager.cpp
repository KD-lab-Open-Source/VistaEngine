#include "StdAfx.h"
#include "CameraManager.h"

#include "..\UserInterface\UI_Logic.h"
#include "..\UserInterface\UI_Render.h"
#include "GameOptions.h"

#include "UnitActing.h"
#include "Terra.h"
#include "RenderObjects.h"
#include "EditorVisual.h"
#include "SafeMath.h"
#include "Squad.h"
#include "Serialization.h"
#include "..\Water\Water.h"
#include "..\Environment\Environment.h"

//---------------------------------------
float visibilityDistance2 = sqr(700);
float HardwareCameraFocus = 0.8f;

REGISTER_CLASS(CameraSpline, CameraSpline, "Сплайн камеры")

float CUT_SCENE_TOP = -0.390625f;
float CUT_SCENE_BOTTOM = 0.390625f;

namespace{
	const sColor4c DEFAULT_COLOR(200, 200, 200, 200);
	const sColor4c DEFAULT_TRANSPARENT_COLOR(200, 200, 200, 64);

	const sColor4c ACTIVE_COLOR(GREEN);
	const sColor4c ACTIVE_TRANSPARENT_COLOR(0, 255, 0, 64);

	const sColor4c SELECTED_COLOR(RED);
	const sColor4c SELECTED_TRANSPARENT_COLOR(255, 0, 0, 64);
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
void SetCameraPosition(cCamera *UCamera,const MatXf& Matrix)
{
	MatXf ml=MatXf::ID;
	ml.rot()[2][2]=-1;

	MatXf mr=MatXf::ID;
	mr.rot()[1][1]=-1;
	MatXf CameraMatrix;
	CameraMatrix=mr*ml*Matrix;

	UCamera->SetPosition(CameraMatrix);
}

//-------------------------------------------------------------------

CameraCoordinate::CameraCoordinate(const Vect2f& position, float psi, float theta, float distance, const Vect2f& dofParams)
:	psi_(psi), 
	theta_(theta),
	distance_(distance),
	dofParams_(dofParams)
{
	if(vMap.isWorldLoaded()){
		int x = clamp(position.xi(), 0, vMap.H_SIZE-1);
		int y = clamp(position.yi(), 0, vMap.V_SIZE-1);
		position_ = to3D(position, vMap.GetApproxAlt(x,y));
	}
	else
		position_ = to3D(position, 0);

	//check();
}

CameraCoordinate::CameraCoordinate()
: position_(Vect3f::ZERO)
, psi_(0.0f)
, theta_(0.0f)
, distance_(0.0f)
, dofParams_(100.0f, 1000.0f)
{
}

CameraCoordinate::CameraCoordinate(const Vect3f& position, float psi, float theta, float distance, const Vect2f& dofParams)
:	psi_(psi), 
	theta_(theta),
	distance_(distance),
	position_(position),
	dofParams_(dofParams)
{

}

CameraCoordinate CameraCoordinate::operator*(float t) const
{
	return CameraCoordinate(position()*t, psi()*t, theta()*t, distance()*t, dofParams()*t);
}

CameraCoordinate CameraCoordinate::operator+(const CameraCoordinate& coord) const
{
	return CameraCoordinate(position() + coord.position(), psi() + coord.psi(), theta() + coord.theta(), distance() + coord.distance(), dofParams() + coord.dofParams());
}

CameraCoordinate CameraCoordinate::operator-(const CameraCoordinate& coord0) const
{
	CameraCoordinate coord1 = *this;
	coord1.uncycle(coord0);
	return CameraCoordinate(coord1.position() - coord0.position(), coord1.psi() - coord0.psi(), coord1.theta() - coord0.theta(), coord1.distance() - coord0.distance(), coord1.dofParams() - coord0.dofParams());
}

void CameraCoordinate::uncycle(const CameraCoordinate& coord0)
{
	psi_ = ::uncycle(psi_, coord0.psi(), 2*M_PI);
	theta_ = ::uncycle(theta_, coord0.theta(), 2*M_PI);
}
void CameraCoordinate::cycle()
{
	psi_ = cycleAngle(psi_);
	theta_ = cycleAngle(theta_);
}


void CameraCoordinate::interpolate(const CameraCoordinate& coord0, const CameraCoordinate& coord1, float t)
{
	position_.interpolate(coord0.position(), coord1.position(), t);
	dofParams_ = Vect2f(coord0.dofParams().x * ( 1.0f - t) + coord1.dofParams().x * t, coord0.dofParams().y * ( 1.0f - t) + coord1.dofParams().y * t);
	psi_ = ::cycle(coord0.psi() + getDist(coord1.psi(), coord0.psi(), 2*M_PI)*t, 2*M_PI);
	theta_ = ::cycle(coord0.theta() + getDist(coord1.theta(), coord0.theta(), 2*M_PI)*t, 2*M_PI);
	distance_ = coord0.distance() + (coord1.distance() - coord0.distance())*t;
}

void CameraCoordinate::interpolateHermite(const CameraCoordinate coords[4], float u)
{
	float t2 = u*u;
	float t3 = u*t2;
	*this = coords[3]*((-t2+t3)/2.0f) + coords[0]*(-u/2.0+t2-t3/2.0) + coords[2]*(2.0*t2-3.0/2.0*t3+u/2.0) + coords[1]*(1.0-5.0/2.0*t2+3.0/2.0*t3);
}

void CameraCoordinate::check(bool restricted)
{
	const CameraRestriction& cameraRestriction_ = cameraManager->cameraRestriction();
	const CameraBorder& cameraBorder_ = cameraManager->cameraBorder();

	//float z = 0; // высота земли в данной точке
	//float zm = 100;
	////position_.z = z;
	//
	//if(distance() < cameraRestriction_.CAMERA_ZOOM_TERRAIN_THRESOLD1)
	//	position_.z = z;
	//else if(distance() > cameraRestriction_.CAMERA_ZOOM_TERRAIN_THRESOLD2)
	//	position_.z = zm;
	//else{
	//	float t = (distance() - cameraRestriction_.CAMERA_ZOOM_TERRAIN_THRESOLD1)/(cameraRestriction_.CAMERA_ZOOM_TERRAIN_THRESOLD2 - cameraRestriction_.CAMERA_ZOOM_TERRAIN_THRESOLD1);
	//	position_.z = z + t*(zm - z);
	//}

	float scroll_border = (distance() - cameraRestriction_.CAMERA_ZOOM_MIN)/(cameraRestriction_.CAMERA_ZOOM_MAX - cameraRestriction_.CAMERA_ZOOM_MIN)*cameraRestriction_.CAMERA_WORLD_SCROLL_BORDER;
	position_.x = clamp(position().x, scroll_border, vMap.H_SIZE - scroll_border);
	position_.y = clamp(position().y, scroll_border, vMap.V_SIZE - scroll_border);

	if(restricted){
		distance_ = clamp(distance(), cameraRestriction_.CAMERA_ZOOM_MIN, cameraRestriction_.CAMERA_ZOOM_MAX);
		//максимально допустимый наклон на данной высоте
		//  линейно от CAMERA_THETA_MIN на CAMERA_ZOOM_MIN
		//          до CAMERA_THETA_MAX на CAMERA_ZOOM_MAX
		float t = 1 - (distance() - cameraRestriction_.CAMERA_ZOOM_MIN)/(cameraRestriction_.CAMERA_ZOOM_MAX - cameraRestriction_.CAMERA_ZOOM_MIN);
		float theta_max = cameraRestriction_.CAMERA_THETA_MIN + t*(cameraRestriction_.CAMERA_THETA_MAX - cameraRestriction_.CAMERA_THETA_MIN);

		theta_ = clamp(theta(), 0, theta_max);

	}
	else{
		distance_ = clamp(distance(), 10, 10000);
		if(!::isUnderEditor()	)
			theta_ = clamp(theta(), 0, M_PI);
	}
 	
	Vect3f camera_position;
	camera_position.setSpherical(psi_, theta_, distance_);
	camera_position += position_;

	Vect3f old_camera_pos = camera_position;

	if(!isUnderEditor()){
		camera_position.x = clamp(camera_position.x, cameraBorder_.CAMERA_WORLD_BORDER_LEFT, vMap.H_SIZE - cameraBorder_.CAMERA_WORLD_BORDER_RIGHT);
		camera_position.y = clamp(camera_position.y, cameraBorder_.CAMERA_WORLD_BORDER_TOP, vMap.V_SIZE - cameraBorder_.CAMERA_WORLD_BORDER_BOTTOM);
	}

	if(restricted){
		int h = 0; //vMap.getVoxelW(camera_position.xi(), camera_position.yi());
		camera_position.z = clamp(camera_position.z, h + cameraRestriction_.CAMERA_MIN_HEIGHT, h + cameraRestriction_.CAMERA_MAX_HEIGHT);
	}
	else
		camera_position.z = clamp(camera_position.z, 0, 10000);


	position_ += (camera_position - old_camera_pos);

}

int CameraCoordinate::height() const 
{
	return vMap.isLoad() ? vMap.GetAlt(vMap.XCYCL(round(position().x)),vMap.YCYCL(round(position().y))) >> VX_FRACTION : 0;
}

//-------------------------------------------------------------------

CameraManager::CameraManager(cCamera* camera)
: coordinate_(Vect2f(500, 500), 0, 0, 300)
, frustumClip_(-0.5f, -0.5f, 0.5f, 0.5f)
, aspect_(4.0f / 3.0f)
{
	Camera = camera;
	cameraRestriction_ = GlobalAttributes::instance().cameraRestriction;
	cameraMouseZoom = false;

	setFocus(HardwareCameraFocus);
	
	matrix_ = MatXf::ID;
	eyePosition_ = Vect3f::ZERO;
//	focus_ = 1.0f;

	restricted_ = GameOptions::instance().getBool(OPTION_CAMERA_RESTRICTION);
	unitFollowDown_ = GameOptions::instance().getBool(OPTION_CAMERA_UNIT_DOWN_FOLLOW);
	unitFollowRotate_ = GameOptions::instance().getBool(OPTION_CAMERA_UNIT_ROTATE);
	setInvertMouse(GameOptions::instance().getBool(OPTION_CAMERA_RESTRICTION));

	replayIndex_ = -1;
	replayIndexMax_ = 0;
	interpolationTimer_ = 0;

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
	isSplineUnitFollow = false;

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

	unit_follow = 0;
	unit_follow_prev = 0;

	update();
}

CameraManager::~CameraManager()
{
	Camera->Release();
}

void CameraManager::setCoordinate(const CameraCoordinate& coord) 
{ 
	coordinate_ = coord;
	coordinate_.cycle();
	update();
}

void CameraManager::setFocus(float focus)
{
	focus_ = focus;
	Camera->SetAttr(ATTRCAMERA_PERSPECTIVE);
	SetFrustumGame();
	update();
}

void CameraManager::update()
{
	Vect3f position;
	position.setSpherical(coordinate().psi(), coordinate().theta(), coordinate().distance());
	position += coordinate().position();
//	if(oscillatingTimer_()){
//		float t = (float)(explodingDuration_ - oscillatingTimer_())/1000;
//		position.x += explodingFactor_*cameraExplodingPrm.x(t);
//		position.y += explodingFactor_*cameraExplodingPrm.y(t);
//		position.z += explodingFactor_*cameraExplodingPrm.z(t);
//	}

	Mat3f oscillatingMatrix = Mat3f::ID;

	if(oscillatingTimer_()){
		float t = (float)(explodingDuration_ - oscillatingTimer_())/1000;
		Vect3f value = explodingVector_*sin(4*M_PI*t)*(1/(sqr(t)+0.1f)-0.7f);
		position += value;
		Vect3f axisX(cos(coordinate().psi()+M_PI/2.0f), sin(coordinate().psi()+M_PI/2.0f), 0);
		oscillatingMatrix.set(Vect3f(cos(coordinate().psi()), sin(coordinate().psi()), 0), value.dot(axisX)*0.003f);
	}
	else
		explodingVector_ = Vect3f::ZERO;

	eyePosition_ = position;

	matrix_ = MatXf::ID;
	matrix_.rot() = Mat3f(coordinate().theta(), X_AXIS)*Mat3f(M_PI/2 - coordinate().psi(), Z_AXIS) * oscillatingMatrix;
//	matrix_.rot() = Mat3f(coordinate().theta(), X_AXIS)*Mat3f(M_PI/2 - coordinate().psi(), Z_AXIS);
	matrix_ *= MatXf(Mat3f::ID, -position);	

	SetCameraPosition(Camera, matrix_);
}

Rectf aspectedWorkArea(const Rectf& windowPosition, float aspect)
{
	Rectf aspectWindowPosition;
	if(float(windowPosition.width()) / float(windowPosition.height()) < aspect){
		float height = windowPosition.width() / aspect;
		aspectWindowPosition.set(windowPosition.left(),
								  windowPosition.top() + (windowPosition.height() - height) * 0.5f,
								  windowPosition.width(),
								  height);
	}
	else{
		float width = windowPosition.height() * aspect;
		aspectWindowPosition.set(windowPosition.left() + (windowPosition.width() - width) * 0.5f,
								  windowPosition.top(),
								  width,
								  windowPosition.height());
	}
	return aspectWindowPosition;
}

void CameraManager::SetFrustumEditor()
{
	Vect2f z=calcZMinMax();
	Vect2f center(0.5f, 0.5f);
      	
	frustumClip_.set(-0.5f, -0.5f, 0.5f, 0.5f);

	Camera->SetFrustum(								// устанавливается пирамида видимости
		&Vect2f(0.5f,0.5f),							// центр камеры
		&frustumClip_,								// видимая область камеры
		&Vect2f(focus_,focus_),						// фокус камеры
		&z
		);
}

void CameraManager::SetFrustumGame()
{
	Vect2f z=calcZMinMax();

	Rectf windowPosition(0, 0, gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY());
	aspect_ = windowPosition.width() / windowPosition.height();
	float aspect = max(4.0f / 3.0f, aspect_);
	Rectf aspectWindowPosition(aspectedWorkArea(windowPosition, aspect));

	Vect2f center(aspectWindowPosition.center());
      	
	frustumClip_.set(-0.5f * (aspectWindowPosition.left() - center.x) / (windowPosition.left() - center.x),
					 -0.5f * (aspectWindowPosition.top() - center.y)  / (windowPosition.top() - center.y),
					 0.5f * (aspectWindowPosition.right() - center.x)  / (windowPosition.right() - center.x),
					 0.5f * (aspectWindowPosition.bottom() - center.y)  / (windowPosition.bottom() - center.y)
					 );


	float focus = correctedFocus(focus_);

	Camera->SetFrustum(			// устанавливается пирамида видимости
		&Vect2f(0.5f,0.5f),		// центр камеры
		&frustumClip_,			// видимая область камеры
		&Vect2f(focus, focus),	// фокус камеры
		&z
		);
}

void CameraManager::SetFrustum()
{
	if(::isUnderEditor())
		SetFrustumEditor();
	else
		SetFrustumGame();
}

Vect2f CameraManager::calcZMinMax()
{
	Vect2f z(30.0f,13000.0f);
	if(environment)
	{
		z.x=environment->GetGameFrustrumZMin();
		float angle=coordinate().theta();
		float c=fabsf(angle)/(M_PI/2);
		c=clamp(c,0,1);
		z.y=environment->GetGameFrustrumZMaxHorizontal()*c+
			environment->GetGameFrustrumZMaxVertical()*(1-c);
	}

	return z;
}

void CameraManager::calcRayIntersection(const Vect2f& pos, Vect3f& v0,Vect3f& v1)
{
	Camera->ConvertorCameraToWorld(&pos, &v1);
	if(Camera->GetAttr(ATTRCAMERA_PERSPECTIVE)){
		MatXf matrix=Camera->GetPosition();
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
	
	//const CameraRestriction& cameraRestriction_ = GetCameraParam();
	//	cameraMouseZoom = isPressed(VK_LBUTTON) && isPressed(VK_RBUTTON);
	
	if(!unit_follow){
		cameraPositionForce.x += deltaPos.x*cameraRestriction_.CAMERA_SCROLL_SPEED_DELTA;
		cameraPositionForce.y += deltaPos.y*cameraRestriction_.CAMERA_SCROLL_SPEED_DELTA;
	}
	
	cameraPsiForce += deltaOri.x*cameraRestriction_.CAMERA_KBD_ANGLE_SPEED_DELTA;
	cameraThetaForce += deltaOri.y*cameraRestriction_.CAMERA_KBD_ANGLE_SPEED_DELTA;
	
	cameraZoomForce.z += deltaZoom*cameraRestriction_.CAMERA_ZOOM_SPEED_DELTA;
}

int CameraManager::mouseQuant(const Vect2f& mousePos)
{
	if(interpolationTimer_ || unit_follow)
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

bool CameraManager::cursorTrace(const Vect2f& pos2, Vect3f& v)
{
	if(!vMap.isWorldLoaded())
		return false;

	Vect3f pos,dir;
	GetCamera()->GetWorldRay(pos2, pos, dir);

	if (!terScene->TraceDir(pos,dir,&v)) {
		Rectf rect(Vect2f(vMap.H_SIZE-1, vMap.V_SIZE-1));
		Vect2f p1(pos);
		//Vect2f p2(dir * 20000);
		
		dir *= pos.z / (dir.z >= 0 ? 0.0001f : -dir.z);
		Vect2f p2(pos += dir);
		
		if(rect.clipLine(p1, p2))
			v = Vect3f(p2, vMap.GetApproxAlt(p2.xi(), p2.yi()));
		return false;
	}
	else
		environment->water()->Trace(pos,v,v);

	return true;
}

bool CameraManager::cursorTrace(const Vect2f& pos2, const Vect3f& start_pos, Vect3f& v)
{
	if(!vMap.isWorldLoaded())
		return false;

	Vect3f pos,dir;
	GetCamera()->GetWorldRay(pos2, pos, dir);

	float dist = pos.distance(start_pos);
	pos += dir * dist;

	if (!terScene->TraceDir(pos,dir,&v)) {
		Rectf rect(Vect2f(vMap.H_SIZE-1, vMap.V_SIZE-1));
		Vect2f p1(pos);
		//Vect2f p2(dir * 20000);
		
		dir *= pos.z / (dir.z >= 0 ? 0.0001f : -dir.z);
		Vect2f p2(pos += dir);
		
		if(rect.clipLine(p1, p2))
			v = Vect3f(p2, vMap.GetApproxAlt(p2.xi(), p2.yi()));
		return false;
	}
	else
		environment->water()->Trace(pos,v,v);

	return true;
}

void CameraManager::getWorldPoint(const Vect2f& scr, Vect3f& pos, Vect3f *_dir)
{
	Vect3f tmp;
	Vect3f &dir = _dir ? *_dir : tmp;

	GetCamera()->GetWorldRay(scr, pos, dir);
	
	// пересекаем с плоскостью зеропласта
	pos += dir * ((pos.z - vMap.initialHeight) / (dir.z >= 0 ? 0.0001 : -dir.z) );
}

void CameraManager::shift(const Vect2f& mouseDelta)
{
	if(interpolationTimer_ || unit_follow)
		return;

	Vect2f delta = mouseDelta;
	Vect3f v1, v2;
	if(cursorTrace(Vect2f::ZERO, v1) && cursorTrace(delta, v2))
		delta = v2 - v1; 
	else
		delta = Vect2f::ZERO;
	
	coordinate_.position() -= to3D(delta, 0);
}

void CameraManager::mouseWheel(int delta)
{
	Vect3f direction = UI_LogicDispatcher::instance().hoverPosition() - coordinate_.position();
	if(direction.norm2() > sqr(cameraRestriction_.CAMERA_ZOOM_SPEED_DELTA))
		direction.normalize(cameraRestriction_.CAMERA_ZOOM_SPEED_DELTA);
	else
		direction = Vect3f::ZERO;
	
	if(delta > 0){
		if(GameOptions::instance().getBool(OPTION_CAMERA_ZOOM_TO_CURSOR))
			cameraZoomForce += direction;
		cameraZoomForce.z -= cameraRestriction_.CAMERA_ZOOM_SPEED_DELTA;
	}
	else if(delta < 0)
		cameraZoomForce.z += cameraRestriction_.CAMERA_ZOOM_SPEED_DELTA;
}

void CameraManager::quant(float mouseDeltaX, float mouseDeltaY, float delta_time)
{
	MTAuto lock(cameraLock_);

	xassert(delta_time < 0.1f + FLT_COMPARE_TOLERANCE);
	float timeFactor = delta_time*20.f; // !!! PerimeterCameraControlFPS  - убрать теперь сложно 

	if(unit_follow && !unit_follow->alive())
		unit_follow = 0;
	
	if(isAutoRotationMode())
		coordinate_.psi() = coordinate().psi() + _debugRotationAngleDelta;
	else if(fabs(_debugRotationAngleDelta) > 0.001f){
		_debugRotationAngleDelta -= _debugRotationAngleDelta > 0 ? 0.005f : -0.005f;
		coordinate_.psi() = coordinate().psi() + _debugRotationAngleDelta;
	}

	if(interpolationTimer_){
		float t = (frame_time() - interpolationTimer_)/(float)interpolationDuration_;
		bool splineEnd = false;
		if(t >= 1){
			if(replayIndex_ != -1){
				if(++replayIndex_ < replayIndexMax_){
					setPath(replayIndex_);
					t = 0;
				}
				else{
					t = 1;
					if(unit_follow)
						splineEnd = true;
					stopReplayPath();
					erasePath();
				}
			}
			else{
				interpolationTimer_ = 0;
				t = 1;
				erasePath();
			}
		}
    
		if(!splineEnd) {
			coordinate_.interpolateHermite(interpolationPoints_, t);
//			environment->SetDofParams(coordinate_.dofParams());
			if(isSplineUnitFollow && unit_follow)
				coordinate_.position() += unit_follow->interpolatedPose().trans();
			else	
				coordinate_.position() += spline_.position();
			coordinate_.check(false);
		}
	}
	else{
		
		if(!directControl_ || !unit_follow)
			followQuant(timeFactor);

		//зум вслед за мышью
		if(cameraMouseZoom)
			cameraZoomForce.z += mouseDeltaY*cameraRestriction_.CAMERA_ZOOM_MOUSE_MULT;
		
		//zoom
		cameraZoomVelocity += cameraZoomForce * timeFactor * 3.0f;
		coordinate_.distance() += cameraZoomVelocity.z*timeFactor;
		coordinate_.position() += Vect3f(cameraZoomVelocity.x*timeFactor, cameraZoomVelocity.y*timeFactor, 0);
		
		if(restricted() && !directControl_){
			//if(!cameraMouseTrack){
			//при зуме камера должна принимать макс. допустимый наклон
//			if(fabs(cameraZoomVelocity) > 1.0f)
			if(cameraZoomVelocity.z < -1.0f)
				cameraThetaForce += cameraRestriction_.CAMERA_KBD_ANGLE_SPEED_DELTA;
			//}
		}
		
		//move
		cameraPositionVelocity += cameraPositionForce * timeFactor * 3.0f;
		
		float d = coordinate().distance()/cameraRestriction_.CAMERA_MOVE_ZOOM_SCALE*timeFactor;
		coordinate_.position() += Mat3f(-M_PI/2 + coordinate().psi(), Z_AXIS)*cameraPositionVelocity*d;
		
		//rotate
		cameraPsiVelocity   += cameraPsiForce * timeFactor * 3.0f;
		cameraThetaVelocity += cameraThetaForce * timeFactor * 3.0f;
		
		if(directControl_ && unit_follow_prev){
			unit_follow = unit_follow_prev;
			unit_follow_prev = 0;
			directControlFreeCamera_ = false;
		}
		if(directControl_ && unit_follow){
			float deltaPsi = cameraPsiVelocity * timeFactor;
			Vect3f dir(Vect3f::J_);
			unit_follow->model()->GetNodePositionMats(unit_follow->attr().directControlNode).se().xformVect(dir);
			float psi(atan2(dir.y, dir.x));
			float theta(0);
			if(!interpolation_ && (!directControlFreeCamera_ || directControlFreeCameraStop_)){
				float velocity(cycle(psi - directControlPsi_, 2.0f * M_PI));
				if(velocity > M_PI)
					velocity = velocity - 2.0f * M_PI;
				velocity /= timeFactor;
				float psiMin(cycle(coordinate_.psi() + deltaPsi - psi, 2.0f * M_PI));
				if(psiMin > M_PI)
					psiMin = 2.0f * M_PI - psiMin;
				if(psiMin > cameraRestriction().directControlPsiMax){
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
				unit_follow->model()->GetNodePositionMats(unit_follow->attr().directControlNode).se().xformVect(dir);
				QuatF(coordinate_.psi(), Vect3f::K).invXform(dir);
				theta = atan2(dir.x, dir.z);
			}
			directControlPsi_ = psi;
			coordinate_.psi() += deltaPsi;
			coordinate_.theta() += invertMouse_ * cameraThetaVelocity * timeFactor * cameraRestriction().directControlThetaFactor;
			if(!directControlFreeCamera_ || directControlFreeCameraStop_){
				float thetaNew = clamp(coordinate_.theta(), unit_follow->attr().directControlThetaMin + theta, unit_follow->attr().directControlThetaMax + theta);
				if(unitTransitionTimer_.was_started()){
					float factor = unitTransitionTimer_();
					coordinate_.theta() = (1.0f - factor) * coordinate_.theta() + factor * thetaNew;
				}else
					coordinate_.theta() = thetaNew;
			}
		}else{
			coordinate_.psi() += cameraPsiVelocity * timeFactor;
			coordinate_.theta() += cameraThetaVelocity * timeFactor;
		}

		if(timeFactor > FLT_EPS){
			cameraPsiVelocity   *= clamp(1.0f - cameraRestriction_.CAMERA_ANGLE_SPEED_DAMP * timeFactor, 0.0f, 1.0f);
			cameraThetaVelocity *= clamp(1.0f - cameraRestriction_.CAMERA_ANGLE_SPEED_DAMP * timeFactor, 0.0f, 1.0f);
			cameraZoomVelocity  *= clamp(1.0f - cameraRestriction_.CAMERA_ZOOM_SPEED_DAMP * timeFactor, 0.0f, 1.0f);
			cameraPositionVelocity *= clamp(1.0f - cameraRestriction_.CAMERA_SCROLL_SPEED_DAMP * timeFactor, 0.0f, 1.0f);
		}
		
		cameraThetaForce = cameraPsiForce = 0;
		cameraZoomForce = Vect3f::ZERO;
		cameraPositionForce = Vect3f::ZERO;
		
		if(!unit_follow)
			coordinate_.check(restricted());

		if(directControl_)
			directControlQuant(delta_time);
	}

	if(cameraGroundColliding)
		collisionResolve();

	update();
}

void CameraManager::setTarget(const CameraCoordinate& coord, int duration) 
{ 
	interpolationPoints_[0] = interpolationPoints_[1] = coordinate_;
	interpolationPoints_[2] = interpolationPoints_[3] = coord;
	interpolationTimer_ = frame_time();
	interpolationDuration_ = duration; 
}

void CameraManager::followQuant(float timeFactor)
{
	if(unit_follow){
		CameraCoordinate coordinateNew = coordinate_;

		if(unitFollowRotate_ && !unit_follow->attr().rotToTarget) {
			Vect3f dir(-Vect3f::J);
			unit_follow->interpolatedPose().xformVect(dir);
			coordinateNew.psi() = atan2(dir.y, dir.x);
			
			//cameraPsiVelocity += cameraPsiForce;
			//coordinateNew.psi() += cameraPsiVelocity*timeFactor;
			cameraPsiForce = 0;
		}

		coordinateNew.position() = unit_follow->interpolatedPose().trans();

		if(unitFollowDown_) {
			// Держим юнита в нижней части экрана.
			Vect3f offset(-100,0,0);
			QuatF(coordinateNew.psi(), Vect3f::K).xform(offset);
			coordinateNew.position() += offset;

			if(!unit_follow->rigidBody() || !unit_follow->rigidBody()->isUnit() || !safe_cast<RigidBodyUnit*>(unit_follow->rigidBody())->flyingMode()){
				float distanceNew = cameraRestriction().unitFollowDistance;
				float thetaNew = cameraRestriction().unitFollowTheta;
				if(unit_follow->attr().rotToTarget){
					distanceNew = cameraRestriction().unitHumanFollowDistance;
					thetaNew = cameraRestriction().unitHumanFollowTheta;
				}
				coordinateNew.theta() = thetaNew;
				coordinateNew.distance() = distanceNew;
			}
		}
		float timer = unitFollowTimer_();
		if(timer < 1.f - FLT_EPS){
			coordinate_.interpolate(coordinate_, coordinateNew, timer);
			coordinateDelta_ = CameraCoordinate();
		}
		else{
			coordinateDelta_.interpolate(coordinateDelta_, CameraCoordinate(), timeFactor*cameraRestriction_.CAMERA_FOLLOW_AVERAGE_TAU);
			coordinateDelta_.interpolate(coordinateDelta_, (coordinateNew - coordinate_), timeFactor*cameraRestriction_.CAMERA_FOLLOW_AVERAGE_TAU);
			coordinate_ = coordinateNew - coordinateDelta_;
		}
	}
	else {
		float groundZ = 0;
		if(coordinate_.position().x >= 0 && coordinate_.position().x < vMap.H_SIZE &&
		coordinate_.position().y >= 0 && coordinate_.position().y < vMap.V_SIZE)
			groundZ = vMap.GetApproxAlt(coordinate_.position().xi(), coordinate_.position().yi());

		if(!::isUnderEditor() &&  fabsf(coordinate_.position().z - groundZ) > 1.0f) 
			coordinate_.position().z += (groundZ - coordinate().position().z)*cameraRestriction_.CAMERA_KBD_ANGLE_SPEED_DELTA*timeFactor;
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

void CameraManager::directControlQuant(float delta_time)
{
	if(unit_follow){
		CameraCoordinate coordinateNew = coordinate_;

		unit_follow->model()->Update();
		if(interpolation_){
			coordinateNew.theta() = M_PI/2.0f;
			prevPosition_ = unit_follow->model()->GetNodePositionMats(unit_follow->attr().directControlNode).trans();
			deltaPosePrev_ = Vect3f::ZERO;
		}else{
			Vect3f deltaPose_(unit_follow->model()->GetNodePositionMats(unit_follow->attr().directControlNode).trans());
			deltaPose_ -= prevPosition_;
			deltaPose_ /= delta_time;
			deltaPosePrev_.interpolate(deltaPosePrev_, deltaPose_, cameraRestriction().directControlRelaxation);
			prevPosition_.scaleAdd(deltaPosePrev_, delta_time);
		}

		coordinateNew.position() = prevPosition_;

		if(unit_follow->attr().directControlNode == 0)
			coordinateNew.position().z += unit_follow->height() * 0.7f;
			
		if(unitTransitionTimer_.was_started()){
			float factor = unitTransitionTimer_();
			directControlOffset_.interpolate(directControlOffsetPrev_, directControlOffsetNew_, unitTransitionTimer_());
			if(1.0f - factor < FLT_EPS)
				unitTransitionTimer_.stop();
		}else if(directControlFreeCameraStop_){
			directControlFreeCameraStop_ = false;
			directControlFreeCamera_ = false;
		}

		Vect3f offset = directControlOffset_;
		
		QuatF cameraOrientation(coordinateNew.theta() - M_PI/2.0f, Vect3f::J);
		cameraOrientation.premult(QuatF(coordinateNew.psi(), Vect3f::K));
		cameraOrientation.xform(offset);
		directControlShootingDirection_ = offset;
		directControlShootingDirection_.Normalize();
		
		coordinateNew.position() += offset;

		if(unit_follow->attr().rotToTarget)
			coordinateNew.distance() = cameraRestriction().unitHumanFollowDistance;
		else
			coordinateNew.distance() = cameraRestriction().unitFollowDistance;

		float timer(unitFollowTimer_());
		interpolation_ = timer < 1.f - FLT_EPS;
		if(interpolation_)
			coordinate_.interpolate(coordinate_, coordinateNew, timer);
		else			
			coordinate_ = coordinateNew;
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
		unit_follow_prev = unit_follow;
		if(unit_follow_prev)
			directControlFreeCamera_ = true;
	}
	unit_follow = unit;
	unitFollowTimer_.start(transitionTime + 1);
	interpolation_ = true;
}

void CameraManager::setUnitFollowMode(bool unitFollowDown, bool unitFollowRotate)
{
	unitFollowDown_ = unitFollowDown;
	unitFollowRotate_ = unitFollowRotate;
}

void CameraManager::startReplayPath(int duration, int cycles, UnitInterface* unit)
{
	interpolationDuration_ = duration; 

	if(spline_.empty())
		return;
	
	if(unit) {
		MTAuto lock(cameraLock_);
		unit_follow = unit;
		isSplineUnitFollow = true;
	}
	
	if(spline_.size() == 1){
		if(!duration){
			setCoordinate(spline_[0]);
			return;
		}
		else{
			CameraCoordinate coord = spline_[0];
			spline_.spline().push_back(coord);
		}
	}

	if(spline_.cycled)
		replayIndexMax_ = cycles*spline_.size();
	else
		replayIndexMax_ = cycles*spline_.size() - 1;
	
	setPath(replayIndex_ = 0);
}

void CameraManager::stopReplayPath()
{
	replayIndex_ = -1;
	interpolationTimer_ = 0;
	if(isSplineUnitFollow) {
		MTAuto lock(cameraLock_);
		isSplineUnitFollow = false;
		unit_follow = 0;
	}
}

void CameraManager::setPath(int index) 
{ 
	xassert(!spline_.empty());
	index %= spline_.size();

	if(replayIndexMax_ >= spline_.size()){ // Зацикленное повторение последовательности
		interpolationPoints_[0] = spline_[(index - 1 + spline_.size()) % spline_.size()];
		interpolationPoints_[1] = spline_[index];
		interpolationPoints_[2] = spline_[(index + 1) % spline_.size()];
		interpolationPoints_[3] = spline_[(index + 2) % spline_.size()];
	}
	else{
		interpolationPoints_[0] = spline_[clamp(index - 1, 0, spline_.size() - 1)];
		interpolationPoints_[1] = spline_[index];
		interpolationPoints_[2] = spline_[clamp(index + 1, 0, spline_.size() - 1)];
		interpolationPoints_[3] = spline_[clamp(index + 2, 0, spline_.size() - 1)];
	}

	interpolationTimer_ = frame_time();
}

void CameraManager::erasePath() 
{ 
	stopReplayPath();
	spline_.clear(); 
	loadedSpline_ = 0;
	coordinate_.psi() = cycle(coordinate().psi(), 2*M_PI);
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
	//spline_.clear();
	if(addCurrentPosition){
		CameraCoordinate coord = coordinate();
		CameraCoordinate coord0;
		coord0 = spline[0];
		coord.uncycle(coord0);
		coord.position() -= spline_.position();
		spline_.spline().insert(spline_.spline().begin(), coord);
	}
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
		position.Normalize(factor);
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

void drawWireModel(const Vect3f* points, int numPoints, const Se3f& pos, sColor4c color, float scale = 1.0f)
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
	sColor4c pointsColor(active ? ACTIVE_COLOR : DEFAULT_COLOR);
	sColor4c linesColor(active ? ACTIVE_TRANSPARENT_COLOR : DEFAULT_TRANSPARENT_COLOR);
	sColor4c selectedColor(SELECTED_COLOR);

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

	sColor4c color(selected ? selectedColor : pointsColor);
	sColor4c transparentColor(selected ? selectedColor : linesColor);

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
			showInfoPoint(CameraCoordinate(position() + Vect3f::K * 30.0f, 0.0f, M_PI / 2.0f, 0.0f), selected(), false, true);

		CameraCoordinate cc, prev_cc;


		sColor4c pointsColor(ACTIVE_COLOR);
		sColor4c linesColor(ACTIVE_TRANSPARENT_COLOR);

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
	ar.serialize(position_, "position", 0);
	ar.serialize(psi_, "psi", 0);
	ar.serialize(theta_, "theta", 0);
	ar.serialize(distance_, "distance", 0);
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
	ar.serialize(coordinates_, "spline", 0);
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
	ar.serialize(splines_, "cameraSplines", "Камеры");
}

void CameraManager::setSkipCutScene(bool skip) 
{ 
	cutSceneSkipped_ = skip; 
	if(skip) 
		lastSkipTime_.start(); 
}

void CameraManager::collisionResolve()
{
	Vect3f position;
	position.setSpherical(coordinate().psi(), coordinate().theta(), coordinate().distance());
	position += coordinate().position();

	if(oscillatingTimer_()){
		float t = (float)(explodingDuration_ - oscillatingTimer_())/1000;
		Vect3f value = explodingVector_*sin(4*M_PI*t)*(1/(sqr(t)+0.1f)-0.7f);
		position += value;
	}

	if(position.z > (256 + environment->GetGameFrustrumZMin()*1.5f))
		return;

	float penetration = collisionCamera(position, environment->GetGameFrustrumZMin()*1.5f);
	if(penetration > FLT_EPS){
		if(directControl_)
			if(directControlFreeCamera_){
				float height = cosf(coordinate_.theta()) * coordinate_.distance();
				float dist = sinf(coordinate_.theta()) * coordinate_.distance();
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

float CameraManager::collisionCamera( const Vect3f& position, float radius )
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
			float z = vMap.GVBuf[vMap.offsetGBuf(i,j)];

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
	float height = cos(coordinate_.theta()) * coordinate_.distance();
	float radiusOld = sqrt(sqr(coordinate_.distance()) - sqr(height));
	coordinate_.theta() = atan2(radiusOld, (height+hDelta));
	coordinate_.distance() = sqrt(sqr(height+hDelta) + sqr(radiusOld));
}

void CameraManager::callcDistance(float hDelta)
{
	float cosTheta = -cosf(coordinate_.theta());
	if(cosTheta > FLT_EPS){
		float offsetNorm = directControlOffset_.norm();
		if(coordinate_.distance() > offsetNorm){
			coordinate_.distance() -= hDelta / cosTheta;
			if(coordinate_.distance() < offsetNorm)
				coordinate_.distance() = offsetNorm;
		}
	}
}

cCamera* CameraManager::GetCamera()
{
	return Camera;
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
	axisZ.Normalize();

	axisX = Vect3f(cos(coordinate().psi()+M_PI/2.0f), sin(coordinate().psi()+M_PI/2.0f), 0);

	axisY.cross(axisZ, axisX);	
	axisY = -axisY;
}

float CameraManager::correctedFocus(float focus, cCamera* camera) const
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

	if(vMap.isWorldLoaded()){
		int x = clamp(position.xi(), 0, vMap.H_SIZE-1);
		int y = clamp(position.yi(), 0, vMap.V_SIZE-1);
		coord.position() = Vect3f(position.x, position.y, vMap.GetApproxAlt(x,y));
	}

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
		unit_follow_prev = 0;
		splineToDefaultCoordinate(1000.0f, unit_follow ? unit_follow->position() : coordinate_.position());
		unit_follow = 0;
	}else
		SetCameraFollow();
}

void CameraManager::enableDirectControl(const UnitInterface* unit, int transitionTime)
{
	MTAuto lock(cameraLock_);
	SetCameraFollow(unit, transitionTime);
	directControl_ = true;
	unit_follow_prev = 0;
	directControlFreeCamera_ = false;
	directControlFreeCameraStop_ = false;
	unitTransitionTimer_.stop();
	if(unit)
		directControlOffset_ = safe_cast<const UnitActing*>(unit)->directControlOffset();
}
