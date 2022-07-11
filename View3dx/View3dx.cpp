// WinVGView.cpp : implementation of the CWinVGView class
//

#include "stdafx.h"
#include "View3dx.h"
#include <mmsystem.h>
#include "ModelInfo.h"
#include "DrawGraph.h"
#include "Render\D3D\D3DRender.h"
#include "XMath\xmathLib.h"
#include "Terra\vmap.h"
#include "Water\SkyObject.h"
#include "Render\inc\fps.h"
#include "Render\src\FT_Font.h"
#include "Render\src\Scene.h"
#include "Serialization\XPrmArchive.h"
#include "kdw/PropertyEditor.h"
#include "kdw/Filedialog.h"
#include "kdw/Label.h"
#include "VistaRender\StreamInterpolation.h"
#include "Serialization\RangedWrapper.h"
#include "Serialization\Decorators.h"
#include "Serialization\EnumDescriptor.h"
#include "Serialization\ResourceSelector.h"
#include "Render\src\TileMap.h"
#include "Render\Src\VisGeneric.h"
#include "FileUtils\FileUtils.h"

using namespace FT;

FPS fps;

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(View3dx, Lod, "Lod")
REGISTER_ENUM_ENCLOSED(View3dx, LOD0, "Лод 0")
REGISTER_ENUM_ENCLOSED(View3dx, LOD1, "Лод 1")
REGISTER_ENUM_ENCLOSED(View3dx, LOD2, "Лод 2")
END_ENUM_DESCRIPTOR_ENCLOSED(View3dx, Lod)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(View3dx, ObjectsNumber, "ObjectsNumber")
REGISTER_ENUM_ENCLOSED(View3dx, OBJECTS_NUMBER_1, "1")
REGISTER_ENUM_ENCLOSED(View3dx, OBJECTS_NUMBER_2, "2")
REGISTER_ENUM_ENCLOSED(View3dx, OBJECTS_NUMBER_10, "10")
END_ENUM_DESCRIPTOR_ENCLOSED(View3dx, ObjectsNumber)

BEGIN_ENUM_DESCRIPTOR_ENCLOSED(View3dx, ControlMode, "ControlMode")
REGISTER_ENUM_ENCLOSED(View3dx, CONTROL_MODE_OBJECT, "Объектом")
REGISTER_ENUM_ENCLOSED(View3dx, CONTROL_MODE_CAMERA, "Камерой")
REGISTER_ENUM_ENCLOSED(View3dx, CONTROL_MODE_LIGHT, "Источником света")
END_ENUM_DESCRIPTOR_ENCLOSED(View3dx, ControlMode)

View3dx::View3dx()
: kdw::Viewport(CreateIRenderDevice(false), true)
{
	cScene::DisableSkyCubeMap();
	
	wireFrame_ = false;
	mouseLDown_=mouseRDown_=0;

	logic_=NULL;
	lod_ = LOD0;
	chainsSimultaneously_ = true;
	skinColor_ = Color4f::RED;

	scene_ = 0;		
	camera_ = 0;

	tileMap_ = 0;
	useShadow_ = false;
	timeLight_ = 10;
	useTimeLight_ = false;
	lighting_ = true;
	bump_ = true;
	anisotropic_ = true;
	showLights_ = true;
	hideObject_ = false;

	scale_normal = true;

	scaleTime_ = 1;
	fillColor_.set(50,100,50);
	controlMode_ = CONTROL_MODE_OBJECT;
	framePeriod_ = 2000;
	framePhase_ = 0;
	objectPosition_.set(1024,1024,0);
	objectOrientation_ = QuatF::ID;
	ResetCameraPosition();
	sunAlpha_ = -M_PI/2;
	sunTheta_ = -M_PI/4;
	showZeroPlane_ = false;
	showGraphicsBound_ = false;
	showBoundSpheres_ = false;
	showGraphicsNodes_ = false;
	showNormals_ = false;
	showLogicBound_ = false;
	showLogicNodes_ = false;
	showDebrises_ = false;
	showSpline_ = false;

	objectsNumber_ = OBJECTS_NUMBER_1;

	mipMapLevel_ = 0;
	debugMipMap_ = false;

	pFont = 0;

	pauseAnimation_ = false;
	reverseAnimation_ = false;
	showFog_=false;

	isModelCamera_ = false;
	isCamera43_ = false;

	check_graph_by_logic = false;
	userRotation_ = Vect3f::ZERO;

	vMap.create("empty");

	environmentTime_ = 0;

	statusLine_ = new kdw::HBox;
	statusLine_->add(timeLabel_ = new kdw::Label("0"), true, true, true);
	statusLine_->add(fpsLabel_ = new kdw::Label(""), true, true, true);
	statusLine_->add(phaseLabel_ = new kdw::Label(""), true, true, true);
	statusLine_->add(polygonssPerSecondLabel_ = new kdw::Label(""), true, true, true);
	statusLine_->add(polygonsLabel_ = new kdw::Label(""), true, true, true);
	statusLine_->add(DIPsLabel_ = new kdw::Label(""), true, true, true);
}

View3dx::~View3dx()
{
	RELEASE(tileMap_);
	streamLogicCommand.clear();
	delete environmentTime_;
	
	destroyDebrises();
	objects_.clear();
	object_ = 0;
	logic_ = 0;

	fontManager().releaseFont(pFont);

	RELEASE(camera_);
	RELEASE(scene_);
}

void View3dx::onInitialize()
{
	gb_RenderDevice->setGlobalRenderWindow(renderWindow());

	gb_VisGeneric->SetTilemapDetail(false);
	gb_VisGeneric->SetUseMeshCache(false);
	scene_ = gb_VisGeneric->CreateScene(); 
	camera_ = scene_->CreateCamera();
	camera_->setAttribute(ATTRCAMERA_PERSPECTIVE); // перспектива
	ResetCameraPosition();

	UpdateCameraFrustum();

	scene_->SetSunDirection(Vect3f(0,0,-1));

	InitFont();
}

void View3dx::onRedraw()
{
	__super::onRedraw();

	if(scene_ && !pauseAnimation_){
		int time = xclock();
		float dt = (time - timePrev_)*scaleTime_;
		if(dt<0 || dt>1000)
			dt=0;
		
		framePhase_ = fmodFast(framePhase_ + dt*(reverseAnimation_ ? -1 : 1)/framePeriod_, 1.0001f);
		scene_->SetDeltaTime(dt);
		timePrev_ = time;
	}

	Draw();
}

void View3dx::UpdateCameraFrustum()
{
	xassert(camera_);
	if(isModelCamera_){
		xassert(gb_RenderDevice);
		const Vect2f org_size(0.123886f, 0.166526f);//(0.206206f, 0.170271f);
		Vect2f sz1(gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY());
		const float k = 4.0f/3;
		float k1 = sz1.x/sz1.y;
		//float half_width = org_size.x/2;
		//float half_height = (k1/k)*(org_size.y)/2;
		float fov = object_->GetFov();
		//static Vect2f focus(1.0f/sqrt(2.f),1.0f/sqrt(2.f));
		Vect2f focus(1.f/(2*tanf(fov*0.5f)),1.f/(2*tanf(fov*0.5f)));
		static Vect2f center(0.5f,0.5f);
		//sRectangle4f rel_pos(- half_width, - half_height, half_width, half_height);
		float half_w = 512.f/sz1.x*0.5f;
		float half_h = 294.f/sz1.y*0.5f;
		sRectangle4f rel_pos(0.5f-half_w,0.5f-half_h, 0.5f+half_w, 0.5f+half_h);
		camera_->SetFrustumPositionAutoCenter(rel_pos,focus.x);
		//gb_Camera->SetFrustum(&center,	NULL, &focus, &Vect2f(10.0f,100000.0f));
	}
	else if(isCamera43_){
		Vect2f sz1(gb_RenderDevice->GetSizeX(), gb_RenderDevice->GetSizeY());
		const float k = 4.0f/3;
		float k1 = sz1.x/sz1.y;
		float hw = 0.5f;
		float hh = 0.5f;
		if(k1>k)
			hw = (sz1.y/3*4)/sz1.x*0.5f;
		else
			hh = (sz1.x/4*3)/sz1.y*0.5f;
		float fov = object_->GetFov();
		sRectangle4f rel_pos(0.5f-hw,0.5f-hh, 0.5f+hw, 0.5f+hh);
		Vect2f focus(1.f/(2*tanf(fov*0.5f)),1.f/(2*tanf(fov*0.5f)));
		camera_->SetFrustumPositionAutoCenter(rel_pos,focus.x);
	}
	else{
		camera_->SetFrustum(&Vect2f(0.5f,0.5f),&sRectangle4f(-0.5f,-0.5f,0.5f,0.5f),		
			&Vect2f(1.f,1.f), &Vect2f(10.0f,3000.0f));

//		gb_Camera->SetFrustrumPositionAutoCenter(sRectangle4f(0,0,1,1),0.01f);
	}
}

void View3dx::Draw()
{	
	if(!scene_) 
		return;

	Objects::iterator it;
	FOR_EACH(objects_, it)
		(*it)->SetPhase(framePhase_);

	if(logic_)
		logic_->SetPhase(framePhase_);

	if(isModelCamera_ || isCamera43_){
		if(object_){
			int ix_camera = object_->FindNode("Camera01");
			int ix_light = object_->FindNode("FDirect01");
			object_->Update();
			if (ix_light!=-1){
				Se3f me = object_->GetNodePosition(ix_light);
				MatXf m(me);
				Mat3f rot(Vect3f(1,0,0), M_PI);
				m.rot()=rot*m.rot();
				m.Invert();
				scene_->SetSunDirection(m.rot().zrow());
			}
			if (ix_camera!=-1){
				MatXf cam(object_->GetNodePosition(ix_camera));

				Mat3f rot(Vect3f(1,0,0), M_PI);
				cam.rot()=cam.rot()*rot;
				cam.Invert();

				camera_->SetPosition(cam);
			}
		}
	}

	//gb_RenderDevice->Fill(backColor_.r,backColor_.g,backColor_.b,0);
	//gb_RenderDevice->BeginScene();

	Color4f fog_color(0.5f,0.5f,0);
	if(showFog_)
		gb_RenderDevice->SetGlobalFog(fog_color,Vect2f(100,300));
	else
		gb_RenderDevice->SetGlobalFog(fog_color,Vect2f(-1, -2));


	gb_RenderDevice->SetRenderState(RS_FILLMODE,wireFrame_ ? FILL_WIREFRAME : FILL_SOLID);

	scene_->Draw(camera_);
	if(showZeroPlane_)
		DrawZeroPlane();

	if(showGraphicsNodes_){
		gb_RenderDevice->SetDrawTransform(camera_);
		Objects::iterator it;
		FOR_EACH(objects_, it)
			(*it)->DrawLogic(camera_, selectedGraphicsNode_.index());
	}

	if(object_){
		if(showGraphicsBound_){
			sBox6f Box;
			object_->GetBoundBox(Box);
			gb_RenderDevice->DrawBound(object_->GetPosition(), Box.min, Box.max, true, Color4c::BLUE);
		}

		if(showBoundSpheres_)
			object_->drawBoundSpheres();
	}

	if(logic_){
		if(showLogicBound_)
			logic_->DrawLogicBound();
		if(showLogicNodes_)
			logic_->DrawLogic(camera_, selectedLogicNode_.index());
	}

	if(showGraphicsNodes_ || showLogicNodes_)
		gb_RenderDevice->OutText(0,0,"&FF0000X&00FF00Y&0000FFZ",Color4f(1,1,1));

	if(showSpline_)
		DrawSpline();

	if(showNormals_){
		TriangleInfo all;
		object_->GetTriangleInfo(all,TIF_TRIANGLES|TIF_NORMALS|TIF_POSITIONS|TIF_ZERO_POS|TIF_ONE_SCALE);
		float scale = object_->GetScale();

		float size = object_->GetBoundRadius()/(16*scale);
		MatXf mat = object_->GetPosition();
		mat.rot()*=scale;
		for(int i=0;i<all.positions.size();i++){
			//gb_RenderDevice->DrawPoint(mat*point[i],Color4c(255,255,0,255));
			Vect3f pos=mat*all.positions[i];
			Vect3f norm=mat.rot()*all.normals[i];
			norm*=size;
			gb_RenderDevice->DrawLine(pos,pos+norm,Color4c(255,255,0,255));
		}

		gb_RenderDevice->FlushLine3D(false,true);
	}


	//gb_RenderDevice->EndScene();
	//gb_RenderDevice->Flush();
	
	fps.quant();

	static int prev_time=0;
 
	if(object_ && (xclock()-prev_time>200)){
		XBuffer buffer;
		buffer.SetDigits(2);

		float chainTime = 0;
		if(object_->GetAnimationGroupNumber() > 0)
			chainTime = object_->GetChain(object_->GetAnimationGroupChain(0))->time;
		
		buffer < "Time: "<= chainTime < " sec.";
		timeLabel_->setText(buffer);
		
		float cur_fps = fps.GetFPS();
		buffer.init();
		buffer < "FPS: " <= cur_fps;
		fpsLabel_->setText(buffer);

		buffer.init();
		buffer < "phase: " <= framePhase_;
		phaseLabel_->setText(buffer);

		buffer.init();
		buffer <= cur_fps*gb_RenderDevice->GetDrawNumberPolygon()/1000000.0f < " mpoly/s";
		polygonssPerSecondLabel_->setText(buffer);

		buffer.init();
		buffer <= gb_RenderDevice->GetDrawNumberPolygon() < " polygon";
		polygonsLabel_->setText(buffer);

		buffer.init();
		buffer <= gb_RenderDevice->GetDrawNumberObjects() < " DIPs";
		DIPsLabel_->setText(buffer);

		prev_time=xclock();
	}
}

MatXf TransposeMeshScr(cObject3dx *UObject,Camera *Camera1,Vect3f &dPos,Vect3f &dAngle)
{
	MatXf CameraMatrix=Camera1->GetPosition();
	MatXf LocalMatrix=UObject->GetPosition();
	Mat3f InvCamera;
	InvCamera.invert(CameraMatrix.rot());
	MatXf Rot = MatXf(
		Mat3f((float)G2R(-dAngle.z), X_AXIS)*
		Mat3f((float)G2R(-dAngle.x), Y_AXIS)*
		Mat3f((float)G2R(dAngle.y), Z_AXIS), 
		Vect3f::ZERO);


	LocalMatrix.set(InvCamera*Rot.rot()*CameraMatrix.rot()*LocalMatrix.rot(),
		LocalMatrix.trans()+dPos.x*CameraMatrix.rot().xrow()
		-dPos.y*CameraMatrix.rot().yrow());
	UObject->SetPosition(LocalMatrix);

	return LocalMatrix;
}

void View3dx::ResetCameraPosition()
{
	cameraPose_.rot().set(0,1,0,0);
	cameraPose_.trans().set(1024,1024,1024);
	SetCameraPosition(0,0,1);
}

void View3dx::SetCameraPosition(float du,float dv,float dscale)
{
	float dist=objectPosition_.distance(cameraPose_.trans())*dscale;

	QuatF rot;
	rot.mult(cameraPose_.rot(),QuatF(du,Vect3f(0,1,0)));
	cameraPose_.rot().mult(rot,QuatF(dv,Vect3f(1,0,0)));

	cameraPose_.trans() = objectPosition_ + Mat3f(cameraPose_.rot())*Vect3f(0,0,-dist);
	Se3f pos = cameraPose_;
	MatXf m(pos);
	m.Invert();
	if(camera_)
		camera_->SetPosition(m);
}

void View3dx::loadObject(LPCSTR fName)
{
	fileName_ = fName;
	if(check_command_line("convert3dx")){
		Static3dxFile::convertFile(fName);
		ErrH.Exit();
	}

	object_ = 0;
	logic_ = 0;
	objects_.clear();
	destroyDebrises();
	
	MatXf pos = MatXf(Se3f(objectOrientation_, objectPosition_));

	if(objectsNumber_ == 1){
		cObject3dx* object = scene_->CreateObject3dx(fName);
		if(!object)
			return;
		objects_.push_back(object);
		object->SetPosition(pos);
	}
	else{
		int num=objectsNumber_/2;
		bool first=true;
		for(int x=-num;x<num;x++)
			for(int y=-num;y<num;y++){
				cObject3dx* object = scene_->CreateObject3dx(fName);
				if(!object)
					return;
				objects_.push_back(object);
				const float mul=500.0f/objectsNumber_;
				object->SetPosition(MatXf(Mat3f::ID, Vect3f(objectPosition_.x+x*mul,objectPosition_.y+y*mul,objectPosition_.z)));
			}
	}

	object_ = objects_.front();

	visibilitySets_.clear();
	for(int i = 0; i < object_->GetVisibilitySetNumber(); i++){
		string comboList;
		for(int j = 0; j < object_->GetVisibilityGroupNumber(VisibilitySetIndex(i)); j++){
			if(j)
				comboList += "|";
			comboList += object_->GetVisibilityGroupName(VisibilityGroupIndex(j), VisibilitySetIndex(i));
		}
		visibilitySets_.push_back(Item(object_->GetVisibilitySetName(VisibilitySetIndex(i)), ComboListString(comboList.c_str(), object_->GetVisibilityGroupName(VisibilityGroupIndex(0), VisibilitySetIndex(i)))));
	}

	string comboList;
	for(int i = 0; i < object_->GetChainNumber(); i++){
		if(i)
			comboList += "|";
		comboList += object_->GetChain(i)->name;
	}
	chain_ = ComboListString(comboList.c_str(), object_->GetChain(0)->name.c_str());

	animationGroups_.clear();
	for(int i = 0; i < object_->GetAnimationGroupNumber(); i++)
		animationGroups_.push_back(Item(object_->GetAnimationGroupName(i), chain_));

	Objects::iterator it;
	FOR_EACH(objects_, it)
		(*it)->setAttribute(ATTRUNKOBJ_SHADOW);

	selectedGraphicsNode_ = NodeSerializer(object_->GetStatic()->nodes, true);

	logic_ = scene_->CreateLogic3dx(fName);
	if(logic_){
		selectedLogicNode_ = NodeSerializer(logic_->GetStatic()->nodes, true);
		MatXf pos = object_->GetPosition();
		logic_->SetPosition(pos);
	}

	SetScale(scale_normal);

	updateObject();
}

void View3dx::onResize(int width, int height) 
{
	__super::onResize(width, height);
	
	if(width <= 0 || height <= 0) return;

	dwScrX = width;
	dwScrY = height;
	gb_RenderDevice->ChangeSize(width, height, RENDERDEVICE_MODE_WINDOW);
	camera_->Update();
	//SetScale(scale_normal);
	UpdateCameraFrustum();
}

void View3dx::SetScale(bool normal)
{
	float mul=1;
	Objects::iterator it;
	FOR_EACH(objects_, it){ 
		cObject3dx* object = *it;
		float Radius = object->GetBoundRadius();

//		if(num_object_linear==1)
//			object->SetPosition(&MatXf(Matrix.rot(),Vect3f(1024,1024,0))); 
		if(Radius<=0)
			Radius=1;
		mul=300/(objectsNumber_*Radius);
		if(!normal)
			mul=1;
		object->SetScale(mul);
	} 

	if(logic_)
		logic_->SetScale(mul);
}

void View3dx::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	Vect3f dPos(0,0,0),dAngleGrad(0,0,0);
	float dScale=1;

	if(nChar=='D')		
		dPos.set(  0, 1,  0);
	else if(nChar=='C')		
		dPos.set(  0,-1,  0);
	else if(nChar=='F')		
		dPos.set( 1,  0,  0);
	else if(nChar=='V')		
		dPos.set(-1,  0,  0);
	else if(nChar=='S')		
		dScale=1.03f*nRepCnt;
	else if(nChar=='X')		
		dScale=0.98f*nRepCnt;
	else if(nChar==VK_DOWN)	
		dAngleGrad.set( 0,  0,  5);
	else if(nChar==VK_UP)	
		dAngleGrad.set( 0,  0, -5);
	else if(nChar==VK_LEFT)	
		dAngleGrad.set( 5,  0, 0);
	else if(nChar==VK_RIGHT)
		dAngleGrad.set(-5,  0, 0);
	else if(nChar==VK_SUBTRACT)	
		scaleTime_ = max(1e-3f,scaleTime_*0.9f); 
	else if(nChar==VK_ADD)		
		scaleTime_ = min(1e3f,scaleTime_*1.1f);
	else if(nChar=='P')
		pauseAnimation_ = !pauseAnimation_;

	ObjectControl(dPos*(float)nRepCnt,dAngleGrad*(float)nRepCnt,dScale);
}

void View3dx::SetSunDirection(const Vect3f& dAngle)
{
	if(scene_){
		const float mul=0.01f;
		sunAlpha_ += dAngle.x*mul;
		sunTheta_+=dAngle.z*mul;

		Vect3f d;
		d.x=cosf(sunTheta_)*cosf(sunAlpha_);
		d.y=cosf(sunTheta_)*sinf(sunAlpha_);
		d.z=sinf(sunTheta_);
		scene_->SetSunDirection(d);
	}
}

void View3dx::ObjectControl(Vect3f& dPos,Vect3f& dAngle, float dScale)
{
	switch(controlMode_)
	{
		case CONTROL_MODE_OBJECT:
			{
				Objects::iterator i;
				FOR_EACH(objects_, i){
					cObject3dx* object = *i;
					float Scale = 1;
					MatXf LocalMatrix = TransposeMeshScr(object,camera_,dPos,dAngle);
					Scale = object->GetScale();
					Scale *= dScale;
					if(fabsf(dScale-1.0f)>FLT_EPS)
						object->SetScale(Scale);

				}
				if(logic_){
					logic_->SetPosition(object_->GetPosition());
					logic_->SetScale(object_->GetScale());
				}
				objectPosition_ = object_->GetPosition().trans();
				objectOrientation_ = object_->GetPositionSe().rot();
			}
			break;
		case CONTROL_MODE_CAMERA:
			if(camera_){
				const float mul=0.01f;
				float du=dAngle.x*mul,dv=dAngle.z*mul;
				SetCameraPosition(du,dv,1/dScale);
			}
			break;
		case CONTROL_MODE_LIGHT:
			SetSunDirection(dAngle);
			break;
	}
}

void View3dx::onMouseButtonDown(MouseButton button)
{
	__super::onMouseButtonDown(button);

	switch(button){
	case MOUSE_BUTTON_LEFT:
		mouseLDown_ = 1;
		PointMouseLDown = mousePosition();
		break;

	case MOUSE_BUTTON_RIGHT:
		mouseRDown_ = 1;
		PointMouseRDown = mousePosition();
		break;
	}
}

void View3dx::onMouseButtonUp(MouseButton button)
{
	__super::onMouseButtonUp(button);

	switch(button){
	case MOUSE_BUTTON_LEFT:
		mouseLDown_ = 0;
		break;

	case MOUSE_BUTTON_RIGHT:
		mouseRDown_ = 0;
		break;
	}
}

void View3dx::onMouseMove() 
{
	__super::onMouseMove();

	Vect2i point = mousePosition();
	if(mouseLDown_) 
		ObjectControl(Vect3f(0,0,0),Vect3f(point.x-PointMouseLDown.x, 0, point.y - PointMouseLDown.y),1);
	if(mouseRDown_) 
		ObjectControl(Vect3f(point.x - PointMouseRDown.x, point.y - PointMouseRDown.y,0),Vect3f(0,0,0),1);
	
	PointMouseLDown = point;
	PointMouseRDown = point;
}

void View3dx::onMouseWheel(int delta) 
{
	__super::onMouseWheel(delta);

	ObjectControl(Vect3f(0,0,0),Vect3f(0,0,0),1 + delta/2000.f);
}

void View3dx::ModelInfo() 
{
	if(object_)
		MsgModelInfo(object_);
}

void View3dx::OnScreenShoot()
{
	static screenshot_x=800;
	if(screenshot_x<16)
		screenshot_x=16;

	const char* masks[] = {
		"Bitmap (.bmp)", "*.bmp",
			0
	};

	FileDialog dialog(this, false, masks, "");
	if(!dialog.showModal())
		return;

	int cx=screenshot_x;
	int cy=round(cx*(dwScrY/(float)dwScrX));

	if(gb_RenderDevice->ChangeSize(cx,cy,RENDERDEVICE_MODE_WINDOW|RENDERDEVICE_MODE_ALPHA|RENDERDEVICE_MODE_RETURNERROR)){
		camera_->Update();
		
		gb_RenderDevice->Fill(fillColor_.r,fillColor_.g,fillColor_.b,0);
		gb_RenderDevice->BeginScene();

		Draw();

		gb_RenderDevice->EndScene();
		gb_RenderDevice->Flush();

		if(!gb_RenderDevice->SetScreenShot(setExtention(dialog.fileName(), "bmp").c_str()))
			xassert(0 && "Cannot save screenshoot");
	}
	else
		xassert(0 && "Cannot initialize grapics to save screen shoot");

	gb_RenderDevice->ChangeSize(dwScrX,dwScrY,RENDERDEVICE_MODE_WINDOW);
	camera_->Update();
}

Color4c Color4(int c)
{
	Color4c in_color;
	switch(c)
	{
	case 0:
		in_color.set(255,255,255);
		break;
	case 1:
		in_color.set(255,255,0);
		break;
	case 2:
		in_color.set(0,0,255);
		break;
	case 3:
		in_color.set(128,128,128);
		break;
	case 4:
		in_color.set(0,0,128);
		break;
	case 5:
		in_color.set(128,0,0);
		break;
	case 6:
		in_color.set(0,0,0);
		break;
	default:
		assert(0);
	}
	return in_color;
}

class RotF:public DrawFunctor
{
	int index;
	int component;
	Color4c in_color;
public:
	Interpolator3dxRotation* rot;

	float get(float t,Color4c* color)
	{
		float out[4];
		index=rot->FindIndexRelative(t,index);
		rot->Interpolate(t,out,index);
		*color=in_color;
		return out[component];
	}

	RotF():index(0),component(0){}
	void SetComponent(int c)
	{
		component=c;
		in_color=Color4(c);
	}
};

class MoveF:public DrawFunctor
{
	int index;
	int component;
	Color4c in_color;
public:
	Interpolator3dxPosition* rot;

	float get(float t,Color4c* color)
	{
		float out[4];
		index=rot->FindIndexRelative(t,index);
		rot->Interpolate(t,out,index);
		*color=in_color;
		return out[component];
	}

	MoveF():index(0),component(0){}
	void SetComponent(int c)
	{
		component=c;
		in_color=Color4(c);
	}
};

double function[][4]=
{
  {0.138233, -0.877776, 0.452722, 0.073783,},
  {0.170297, -0.876553, 0.448786, 0.035289,},
  {0.194286, -0.892868, 0.405635, -0.022362,},
  {0.207283, -0.911106, 0.345889, -0.085326,},
  {0.212016, -0.921264, 0.294803, -0.139331,},
  {0.186672, -0.914539, 0.314075, -0.173576,},
  {0.160907, -0.910050, 0.326659, -0.198020,},
  {0.141864, -0.908573, 0.336925, -0.202116,},
  {0.135418, -0.907317, 0.361789, -0.165971,},
  {0.131295, -0.908534, 0.374863, -0.129634,},
  {0.124826, -0.908458, 0.387635, -0.094135,},
  {0.090663, -0.911763, 0.399213, -0.033115,},
  {0.046053, -0.910208, 0.410714, 0.026709,},
  {0.008099, -0.903963, 0.420601, 0.076678,},
  {0.009403, -0.908279, 0.409826, 0.083569,},
  {0.024464, -0.903622, 0.419143, 0.084783,},
  {0.047625, -0.887501, 0.452007, 0.075914,},
  {0.075099, -0.862217, 0.496141, 0.069183,},
  {0.103557, -0.828589, 0.546409, 0.064438,},
  {0.127017, -0.822995, 0.549886, 0.064586,},
};

int function_size=sizeof(function)/sizeof(function[0]);

class RotFf:public DrawFunctor
{
	int component;
	Color4c in_color;
public:

	float get(float t,Color4c* color)
	{
		int i=round(t*function_size)%function_size;
		//*color=in_color;
		*color=Color4(i%7);
		return function[i][component];
	}

	RotFf():component(0){}
	void SetComponent(int c)
	{
		component=c;
		in_color=Color4(c);
	}
};


void View3dx::DrawSpline()
{
	if(!object_)
		return;
/*
	const char* node_name="turret";
	int inode=p3dx->FindNode(node_name);
	if(inode<0)
		return;
/*/
	int inode = selectedGraphicsNode_.index();
	if(inode<0 || inode>=object_->GetNodeNum())
		return;
	const char* node_name=object_->GetNodeName(inode);
/**/
	gb_RenderDevice->OutText(0,20,node_name,Color4f(1,1,1));
	StaticNode& node=object_->GetStatic()->nodes[inode];
	int ichain=object_->GetAnimationGroupChain(0);
	StaticNodeAnimation& chain=node.chains[ichain];

	DrawGraph graph;
	//graph.SetArgumentRange(-0.5f,+0.5f,-1.2f,+1.2f);
	graph.SetCycleHalfShift();

	if(GetKeyState(VK_LSHIFT)&0x8000)
	{
		graph.SetArgumentRange(0,1,-1.2f,+1.2f);

		RotFf functor;

		Color4c c;
		float x=functor.get(0.0f,&c);

		for(int i=0;i<4;i++)
		{
			functor.SetComponent(i);
			graph.Draw(functor);
		}
	}else
	if(true)
	{
		Interpolator3dxRotation& rot=chain.rotation;
		graph.SetArgumentRange(0,1,-1.2f,+1.2f);

		RotF functor;
		functor.rot=&rot;

		Color4c c;
		float x0=functor.get(0.0f,&c);
		float x1=functor.get(1.0f,&c);

		for(int i=0;i<4;i++)
		{
			functor.SetComponent(i);
			graph.Draw(functor);
		}
	}else
	{
		graph.SetArgumentRange(0,1,-2.0e2f,+2.0e2f);
		Interpolator3dxPosition& pos=chain.position;

		MoveF functor;
		functor.rot=&pos;

		Color4c c;
		functor.get(0.69f,&c);

		for(int i=0;i<3;i++)
		{
			functor.SetComponent(i);
			graph.Draw(functor);
		}
	}

	graph.DrawXPosition(framePhase_,Color4c(255,255,255,255));
}

void View3dx::DrawZeroPlane()
{
	if(!object_)
		return;
	MatXf mat=object_->GetPosition();
  	gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
	cQuadBuffer<sVertexXYZDT1>* quad=gb_RenderDevice->GetQuadBufferXYZDT1();
	DWORD old_cullmode=gb_RenderDevice->GetRenderState(RS_CULLMODE);
	gb_RenderDevice->SetRenderState(RS_CULLMODE,1);
	gb_RenderDevice->SetNoMaterial(ALPHA_BLEND, mat);
	quad->BeginDraw(mat);
	int dx=1000,dy=1000;
	Color4c diffuse(0,255,128,128);
	sVertexXYZDT1 *v=quad->Get();
	v[0].pos.x=-dx; v[0].pos.y=-dy; v[0].pos.z=0; v[0].u1()=0; v[0].v1()=0;
	v[1].pos.x=-dx; v[1].pos.y=dy; v[1].pos.z=0; v[1].u1()=0; v[1].v1()=1;
	v[2].pos.x=dx; v[2].pos.y=-dy; v[2].pos.z=0; v[2].u1()=1; v[2].v1()=0;
	v[3].pos.x=dx; v[3].pos.y=dy; v[3].pos.z=0; v[3].u1()=1; v[3].v1()=1;
	v[0].diffuse=v[1].diffuse=v[2].diffuse=v[3].diffuse=diffuse;
	quad->EndDraw();
	gb_RenderDevice->SetRenderState(RS_CULLMODE,old_cullmode);
	gb_RenderDevice3D->SetRenderState(D3DRS_ZFUNC,D3DCMP_ALWAYS);
}

void View3dx::InitFont()
{
	fontManager().releaseFont(pFont);
	const char* fontName = "Scripts\\Resource\\fonts\\default.ttf";
	pFont = fontManager().createFont(fontName, 15);
	xxassert(pFont, (XBuffer() < "Не удалось создать фонт: " < fontName).c_str());
	gb_RenderDevice->SetDefaultFont(pFont);
}

void View3dx::SetDirectLight(float time)
{
	if(!environmentTime_){
		environmentTime_ = new EnvironmentTime(scene_);
		XPrmIArchive ar;
		if(ar.open("Scripts\\Content\\Presets\\global.set") || ar.open("GlobalAttributes") || ar.open("Scripts\\Content\\GlobalAttributes") || ar.open("..\\Scripts\\Content\\GlobalAttributes")){
			if(ar.openStruct(*this, "GlobalAttributes", 0) && 
				ar.openStruct(*this, "environmentColors", 0) &&
				ar.openStruct(*this, "timeColors_", 0)){
					static_cast<EnvironmentTimeColors*>(environmentTime_)->serialize(ar);
				}
		}
		else
			xassert(0 && "Не могу найти файл 'Scripts\\Content\\Presets\\global.set' или 'Scripts\\Content\\GlobalAttributes' для настроек освещения");
	}
	environmentTime_->SetTime(time);
}

void View3dx::destroyDebrises()
{
	for(vector<cSimply3dx*>::iterator it=debrises.begin();it!=debrises.end();++it)
		(*it)->Release();
	debrises.clear();

	if(object_)
		object_->clearAttribute(ATTRUNKOBJ_IGNORE);
}

void View3dx::createDebrises()
{
	scene_->CreateDebrisesDetached(object_,debrises);
	for(vector<cSimply3dx*>::iterator it=debrises.begin();it!=debrises.end();++it)
		scene_->AttachObj(*it);

	Mats pos=object_->GetPositionMats();
	for(vector<cSimply3dx*>::iterator it=debrises.begin();it!=debrises.end();++it){
		Mats m;
		m.mult(pos,(*it)->GetStatic()->debrisPos);
		(*it)->SetPosition(m.se());
		(*it)->SetScale(m.scale());
	}

	object_->setAttribute(ATTRUNKOBJ_IGNORE);
}

void View3dx::serialize(Archive& ar)
{
	if(!ar.isEdit())
		__super::serialize(ar);
	else if(!object_)
		return;

	if(object_ && ar.openBlock("object", "Объект")){
		if(object_->GetStatic()->is_lod)
			ar.serialize(lod_, "lod", "ЛОД");

		Items::iterator i;
		FOR_EACH(visibilitySets_, i)
			ar.serialize(*i, i->name.c_str(), i->name.c_str());

		ar.closeBlock();
	}

	if(ar.openBlock("animation", "Анимация")){
		ar.serialize(chainsSimultaneously_, "chainsSimultaneously", "Все группы");
		if(chainsSimultaneously_)
			ar.serialize(chain_, "chain", "Цепочка");
		else{
			Items::iterator i;
			FOR_EACH(animationGroups_, i)
				ar.serialize(*i, i->name.c_str(), i->name.c_str());
		}

		ar.serialize(HLineDecorator(), "line2", "<");

		ar.serialize(pauseAnimation_, "pauseAnimation", "Пауза");
		if(pauseAnimation_)
			ar.serialize(RangedWrapperf(framePhase_, 0, 1), "framePhase", "Фаза");
		else{
			ar.serialize(reverseAnimation_, "reverseAnimation", "Назад");
			ar.serialize(RangedWrapperf(scaleTime_, 0.001f, 2), "scaleTime", "Скорость анимации");
		}
		
		ar.closeBlock();
	}

	if(ar.openBlock("rendering", "Отрисовка")){
		ar.serialize(wireFrame_, "wireFrame", "Wireframe");
		ar.serialize(bump_, "bump", "Бамп");
		ar.serialize(anisotropic_, "anisotropic", "Анизотропная фильтрация");
		ar.serialize(lighting_, "lihgting", "Освещение");
		ar.serialize(useShadow_, "useShadow", "Тень");
		ar.serialize(useTimeLight_, "useTimeLight", "Освещение по времени");
		if(useTimeLight_)
			ar.serialize(RangedWrapperf(timeLight_, 0, 24), "timeLight_", "Время суток");
		ar.serialize(controlMode_, "controlMode", "Управление");
		ar.closeBlock();
	}

	if(ar.openBlock("nodes", "Узлы")){
		ar.serialize(showGraphicsBound_, "showGraphicsBound", "Показать графический баунд");
		ar.serialize(showLogicBound_, "showLogicBound", "Показать логический баунд");
		ar.serialize(showBoundSpheres_, "showBoundSpheres", "Показать сферические баунды");
		ar.serialize(showLogicNodes_, "showLogicNodes", "Показать логические узлы");
		ar.serialize(showGraphicsNodes_, "showGraphicsNodes", "Показать графические узлы");
		ar.serialize(showSpline_, "showSpline", "Показать сплайн анимации выбранного графического узла");
		ar.serialize(selectedLogicNode_, "selectedLogicNode", "Логический узел");
		ar.serialize(selectedGraphicsNode_, "selectedGraphicsNode", "Графический узел");
		if(!selectedGraphicsNode_.value().empty() || !selectedLogicNode_.value().empty()){
			ar.serialize(RangedWrapperf(userRotation_.x, -180, 180), "userRotationX", "Вращение X");
			ar.serialize(RangedWrapperf(userRotation_.y, -180, 180), "userRotationY", "Вращение Y");
			ar.serialize(RangedWrapperf(userRotation_.z, -180, 180), "userRotationZ", "Вращение Z");
		}
		ar.closeBlock();
	}

	if(ar.openBlock("debug", "Отладочная отрисовка")){
		bool showDebrises = showDebrises_;
		if(object_)
			ar.serialize(showDebrises_, "showDebrises", "Показать осколки");
		if(showDebrises != showDebrises_)
			if(debrises.empty())
				createDebrises();
			else
				destroyDebrises();

		ar.serialize(showLights_, "showLights", "Источники освещения");
		ar.serialize(hideObject_, "hideObject", "Спрятать объект");

		ar.serialize(showFog_, "showFog", "Показать туман");
		ar.serialize(showNormals_, "showNormals", "Показать нормали");
		ar.serialize(showZeroPlane_, "showZeroPlane", "Показать нулевую плоскость");

		int objectsNumber = objectsNumber_;
		ar.serialize(objectsNumber_, "objectsNumber", "Количество объектов");
		if(objectsNumber != objectsNumber_)
			loadObject(fileName_.c_str());

		bool debugMipMap = debugMipMap_;
		ar.serialize(debugMipMap_, "debugMipMap_", "Debug mip map");
		if(debugMipMap_ != debugMipMap)
			GetTexLibrary()->DebugMipMapColor(debugMipMap_);

		int mipMapLevel = mipMapLevel_;
		ar.serialize(RangedWrapperi(mipMapLevel_, 0, 3), "mipMapLevel", "Mip map level");
		if(mipMapLevel_ != mipMapLevel)
			GetTexLibrary()->DebugMipMapForce(mipMapLevel_);

		ar.serialize(skinColor_, "skinColor", "Скин-цвет");
		static ResourceSelector::Options textureOptions("*.tga", "Resource\\Models\\Textures");
		ar.serialize(ResourceSelector(logoTexture_, textureOptions), "logoTexture", "Текстура логотипа");

		ar.serialize(fillColor_, "fillColor", "Цвет фона");

		ar.closeBlock();
	}

	//ar.serialize(HLineDecorator(), "line1", "<");

	ButtonDecorator showModelProperty("Свойства модели");
	ar.serialize(showModelProperty, "showModelProperty", "<");
	if(showModelProperty){
		Static3dxFile file;
		file.load(fileName_.c_str());
		if(kdw::edit(Serializer(file), (string(getenv("TEMP")) + "ModelProperty.cfg").c_str(), kdw::IMMEDIATE_UPDATE, HWND(0), "Model property"))
			file.save(fileName_.c_str());
	}

	ButtonDecorator screenShoot("Скриншот");
	ar.serialize(screenShoot, "screenShoot", "<");
	if(screenShoot)
		OnScreenShoot();

	if(object_){
		static XBuffer title;
		title.init();
		title < "Конверсия (файл: " <= object_->GetStatic()->version < ", текущая: " <= Static3dxBase::VERSION < ")";
		ButtonDecorator conversion(title);
		ar.serialize(conversion, "conversion", "<");
		if(conversion)
			Static3dxFile::convertFile(fileName_.c_str());
	}

	ar.serialize(objectPosition_, "objectPosition", 0);
	ar.serialize(objectOrientation_, "objectOrientation", 0);
	ar.serialize(cameraPose_, "cameraPose", 0);
	ar.serialize(sunAlpha_, "sunAlpha", 0);
	ar.serialize(sunTheta_, "sunTheta", 0);

	if(ar.isInput()){
		SetCameraPosition(0,0,1);
		if(object_){
			applyUserTransform();
			updateObject();
		}
	}
}

void View3dx::applyUserTransform()
{
	/*
	Если есть и логическая и графическая ноды, то вращать графическую по осям логической.
	*/
	bool is_logic_transform = false;
	Mats logic_transform = Mats::ID;

	if(check_graph_by_logic && selectedGraphicsNode_.index() > -1){
		string name = object_->GetNodeName(selectedGraphicsNode_.index());
		name+="_logic";
		int cur_sel_additional = object_->FindNode(name.c_str());
		if(cur_sel_additional>-1){
			int parent;
			object_->GetNodeOffset(cur_sel_additional,logic_transform,parent);
			if(parent == selectedGraphicsNode_.index())
				is_logic_transform=true;
			else
				xassert(0 && "Неправильно прилинкована нода _logic");
		}
	}

	Se3f rotation = Se3f(QuatF(G2R(userRotation_.x), Vect3f::I) % QuatF(G2R(userRotation_.y), Vect3f::J) % QuatF(G2R(userRotation_.z), Vect3f::K), Vect3f::ZERO);
	Se3f logic_rotation = Se3f::ID;

	if(selectedGraphicsNode_.index() > -1){
		//graph*additional_rotation*rot*inv_additional_rotation
		if(is_logic_transform){
			QuatF inv_logic_rotation;
			inv_logic_rotation.invert(logic_transform.rot());
			logic_rotation.rot().mult(logic_transform.rot(),rotation.rot());
			logic_rotation.rot().postmult(inv_logic_rotation);
			logic_rotation.trans()=Vect3f::ZERO;

			Objects::iterator it;
			FOR_EACH(objects_, it)
				(*it)->SetUserTransform(selectedGraphicsNode_.index(),logic_rotation);
		}
		else{
			Objects::iterator it;
			FOR_EACH(objects_, it)
				(*it)->SetUserTransform(selectedGraphicsNode_.index(),rotation);
		}
	}

	if(selectedLogicNode_.index() > -1 && logic_)
		if(is_logic_transform)
			logic_->SetUserTransform(selectedLogicNode_.index(), logic_rotation);
		else
			logic_->SetUserTransform(selectedLogicNode_.index(), rotation);
}

void View3dx::updateObject()
{
	Objects::iterator ito;
	FOR_EACH(objects_, ito){
		cObject3dx* object = *ito;

		object->SetLod(lod_);

		if(chainsSimultaneously_){
			object->SetChain(chain_);
			if(logic_)
				logic_->SetChain(chain_);
			framePeriod_ = object->GetChain(object->GetChainIndex(chain_))->time*1000;
		}
		else
			for(int i = 0; i < animationGroups_.size(); i++){
				object->SetAnimationGroupChain(i, animationGroups_[i].data);
				if(logic_)
					logic_->SetAnimationGroupChain(i, animationGroups_[i].data);
			}

		for(int i = 0; i < visibilitySets_.size(); i++)
			object->SetVisibilityGroup(object->GetVisibilityGroupIndex(visibilitySets_[i].data, VisibilitySetIndex(i)), VisibilitySetIndex(i));

		object->SetSkinColor(skinColor_, logoTexture_.c_str());

		if(lighting_) 
			object->clearAttribute(ATTRUNKOBJ_NOLIGHT);
		else 
			object->setAttribute(ATTRUNKOBJ_NOLIGHT);

		if(!hideObject_ && !showDebrises_)
			object->clearAttribute(ATTRUNKOBJ_IGNORE);
		else
			object->setAttribute(ATTRUNKOBJ_IGNORE);
	}

	if(useShadow_ && !tileMap_)
		tileMap_ = scene_->CreateMap();

	if(tileMap_){
		tileMap_->putAttribute(ATTRUNKOBJ_IGNORE,!useShadow_);
		gb_VisGeneric->SetShadowType(useShadow_, 4);
	}

	if(useTimeLight_)
		SetDirectLight(timeLight_);
	else
		SetSunDirection(Vect3f::ZERO);

	gb_VisGeneric->SetAnisotropic(anisotropic_);
	gb_VisGeneric->SetEnableBump(bump_);

	scene_->HideSelfIllumination(!showLights_);
	scene_->HideAllObjectLights(!showLights_);
}

bool View3dx::Item::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	return ar.serialize(data, name, nameAlt);
}

NodeSerializer::NodeSerializer(const StaticNodes& nodes, bool enableEmpty)
{
	nodes_ = &nodes;
	value_ = nodes.front().name.c_str();
	add(nodes, -1);
	if(enableEmpty){
		comboList_ = string("|") + comboList_;
		value_ = "";
	}
}

void NodeSerializer::add(const StaticNodes& nodes, int parent)
{
	StaticNodes::const_iterator ni;
	FOR_EACH(nodes, ni){
		if(parent != ni->iparent)
			continue;
		if(!comboList_.empty())
			comboList_ += "|";
		for(int p = ni->iparent; p != -1; p = nodes[p].iparent)
			comboList_ += "    ";
		comboList_ += ni->name;
		add(nodes, ni->inode);
	}
}

bool NodeSerializer::serialize(Archive& ar, const char* name, const char* nameAlt)
{
	bool result = ar.serialize((ComboListString&)*this, name, nameAlt);
	if(ar.isInput())
		while(value_[0] == ' ')
			value_.erase(0, 1);
	return result;
}

int NodeSerializer::index() const
{
	//xassert(nodes_);
	if(!nodes_)
		return -1;
	StaticNodes::const_iterator i;
	FOR_EACH(*nodes_, i)
		if(i->name == value_)
			return i - nodes_->begin();

	return -1;
}

