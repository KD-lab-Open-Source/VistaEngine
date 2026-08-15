// EngineViewport.cpp — see header. Compiled with the engine's flags.

#include "EngineViewport.h"

// The engine's headers assume the old StdAfx preamble: `using namespace std`,
// <vector>/<string> (IRenderDevice.h names `vector`/`string` unqualified),
// `xassert` (XLibs.Net/XUtil/xutil.h) and `FOR_EACH` (Util/XTL/my_STL.h) —
// engine headers call all of them.
#include <vector>
#include <string>
using namespace std;
#include "xutil.h"
#include "my_STL.h"

// Engine headers — safe here because EditorEngine compiles with EngineIncludes.
#include "Render/inc/IRenderDevice.h"
#include "Render/inc/Unknown.h"     // RELEASE()
#include "Render/src/VisGeneric.h"   // gb_VisGeneric, CreateScene
#include "Render/src/Scene.h"        // cScene (CreateCamera, Draw)
#include "Render/src/cCamera.h"      // Camera (SetFrustum, SetPosition)
#include "Util/XMath/xmath.h"        // MatXf/Mat3f/Mat2f + X_AXIS/Y_AXIS/Z_AXIS

namespace {
// kMouseMove2Angle from CGeneralView::WindowProc — 0.25 deg per pixel.
const float kMouseMove2Angle = 0.25f * 3.14159265f / 180.f;

// SetCameraPosition from Game/CameraManager.cpp — the axis flips that turn the
// orbit matrix into the engine's camera-space convention.
void setCameraPosition(Camera* camera, const MatXf& matrix)
{
	MatXf ml = MatXf::ID;
	ml.rot()[2][2] = -1;
	MatXf mr = MatXf::ID;
	mr.rot()[1][1] = -1;
	camera->SetPosition(mr * ml * matrix);
}
}

EngineViewport::EngineViewport() = default;

EngineViewport::~EngineViewport()
{
	done();
}

bool EngineViewport::init(int width, int height)
{
	if(inited_)
		return true;
	if(!nativeWindow_)
		return false;

	// The engine's one entry point into the renderer (Render/RenderStub.cpp):
	// builds gb_VisGeneric + the SDL GPU device.
	if(!gb_RenderDevice)
		CreateIRenderDevice(false);
	if(!gb_RenderDevice)
		return false;

	// With no global SDL window (Qt owns the windows), the device is created
	// without a swapchain; createRenderWindow() below wraps this viewport's
	// native handle and becomes the (global) render window.
	if(!gb_RenderDevice->inited() &&
	   !gb_RenderDevice->Initialize(width, height, RENDERDEVICE_MODE_WINDOW, nullptr, 0, nullptr))
		return false;

	renderWindow_ = gb_RenderDevice->createRenderWindow((HWND)nativeWindow_);
	if(!renderWindow_)
		return false;
	gb_RenderDevice->selectRenderWindow(renderWindow_);

	// The scene graph + a camera off it, as initScene() (Game/RenderObjects.cpp)
	// does. No world is loaded yet — the scene holds the engine's globals and
	// the camera renders whatever the world load (Phase 3b) will add.
	scene_ = gb_VisGeneric->CreateScene();
	if(!scene_)
		return false;
	camera_ = scene_->CreateCamera();
	if(!camera_)
		return false;

	applyCamera();
	inited_ = true;
	return true;
}

void EngineViewport::done()
{
	if(!inited_)
		return;
	if(gb_RenderDevice){
		gb_RenderDevice->selectRenderWindow(0);
		if(renderWindow_)
			gb_RenderDevice->DeleteRenderWindow(renderWindow_);
	}
	// Camera is owned by the scene (CreateCamera); releasing the scene drops it.
	if(scene_)
		RELEASE(scene_);
	scene_ = nullptr;
	camera_ = nullptr;
	renderWindow_ = nullptr;
	inited_ = false;
}

void EngineViewport::resize()
{
	if(renderWindow_)
		renderWindow_->ChangeSize();
}

void EngineViewport::tick(float /*dt*/)
{
	// The orbit state is applied immediately on input (like CGeneralView's
	// WindowProc, which sets cameraManager->setCoordinate per event), so there
	// is nothing to integrate per-frame yet. Phase 3b (world + animation) will
	// advance the scene here.
	applyCamera();
}

void EngineViewport::drawFrame()
{
	if(!inited_ || !gb_RenderDevice || !camera_)
		return;

	// The editor viewport's clear (CGeneralView used the environment's fone
	// colour; with no world loaded, a neutral slate).
	gb_RenderDevice->Fill(32, 48, 64, 255);
	gb_RenderDevice->BeginScene();

	// The camera renders whatever the scene holds. With no world, the scene is
	// empty — the clear is all the frame shows until Phase 3b loads terrain.
	if(camera_){
		const Vect2f center(0.5f, 0.5f);
		const sRectangle4f clip(-0.5f, -0.5f, 0.5f, 0.5f);
		const Vect2f focus(orbit_.focus, orbit_.focus);
		const Vect2f zPlane(30.0f, 12000.0f);
		camera_->SetFrustum(&center, &clip, &focus, &zPlane);
		camera_->PreDrawScene();
		camera_->DrawScene();
	}

	gb_RenderDevice->EndScene();
	gb_RenderDevice->Flush();
}

// --- Input ---------------------------------------------------------------

void EngineViewport::mouseWheel(int wheelDelta, int modifiers)
{
	// Port of CGeneralView::WindowProc WM_MOUSEWHEEL.
	//   wheelDelta > 0 = wheel up (zoom in); SHIFT moves focus; ALT rolls fi.
	const float sign = wheelDelta > 0 ? 1.f : -1.f;
	const bool shift = (modifiers & 1) != 0;
	const bool ctrl  = (modifiers & 2) != 0;
	const bool alt   = (modifiers & 4) != 0;

	if(shift){
		if(alt)
			orbit_.focus = clamp(orbit_.focus + sign * 0.04f, 0.1f, 10.0f);
		else
			orbit_.pz += -sign * (ctrl ? 0.0025f : 0.01f) * orbit_.distance;
	}
	else{
		if(alt)
			orbit_.fi -= sign * 0.04f;
		else
			orbit_.distance = max(orbit_.distance, 2.0f) * (1.0f - ((ctrl ? 0.01f : 0.2f) * sign));
	}
	applyCamera();
}

void EngineViewport::mouseButton(int button, bool pressed, int x, int y)
{
	// button: 1=left, 2=middle, 4=right — CGeneralView's capture model.
	if(button == 2){
		if(pressed){
			mouseMiddle_ = true;
			dragStartX_ = x; dragStartY_ = y;
			dragStartPsi_ = orbit_.psi;
			dragStartTheta_ = orbit_.theta;
		}
		else
			mouseMiddle_ = false;
	}
	else if(button == 4){
		if(pressed){
			mouseRight_ = true;
			dragStartX_ = x; dragStartY_ = y;
			dragStartPx_ = orbit_.px; dragStartPy_ = orbit_.py; dragStartPz_ = orbit_.pz;
		}
		else
			mouseRight_ = false;
	}
	else if(button == 1){
		mouseLeft_ = pressed;
		if(pressed){
			dragStartX_ = x; dragStartY_ = y;
		}
	}
}

void EngineViewport::mouseMove(int x, int y)
{
	if(mouseMiddle_){
		const float dx = float(x - dragStartX_);
		const float dy = float(y - dragStartY_);
		orbit_.psi = dragStartPsi_ + dx * kMouseMove2Angle;
		orbit_.theta = dragStartTheta_ - dy * kMouseMove2Angle;
		applyCamera();
	}
	else if(mouseRight_){
		// Pan: move the orbit centre in the camera plane (CGeneralView's
		// RMB branch, minus the terrain-height coupling).
		const float dx = float(x - dragStartX_);
		const float dy = float(y - dragStartY_);
		// The pan scales with distance like the original.
		const float scale = max(orbit_.distance, 100.f) / 800.f;
		// Camera-plane basis: psi rotates the yaw; a screen delta maps to
		// world x/y through the yaw rotation.
		const float c = cosf(orbit_.psi);
		const float s = sinf(orbit_.psi);
		orbit_.px = dragStartPx_ - (dx * c - dy * s) * scale;
		orbit_.py = dragStartPy_ - (dx * s + dy * c) * scale;
		applyCamera();
	}
}

// --- Camera --------------------------------------------------------------

void EngineViewport::applyCamera()
{
	if(!camera_)
		return;

	// The orbit matrix, as CameraManager::quant builds it:
	//   R(theta,X)*R(fi,Y)*R(pi/2-psi,Z) translated by -position.
	// Position sits on the orbit sphere around the centre.
	Vect3f position(
		orbit_.px + orbit_.distance * cosf(orbit_.theta) * cosf(orbit_.psi),
		orbit_.py + orbit_.distance * cosf(orbit_.theta) * sinf(orbit_.psi),
		orbit_.pz + orbit_.distance * sinf(orbit_.theta));

	MatXf matrix = MatXf::ID;
	matrix.rot() = Mat3f(orbit_.theta, X_AXIS) * Mat3f(orbit_.fi, Y_AXIS) * Mat3f(M_PI_2 - orbit_.psi, Z_AXIS);
	matrix *= MatXf(Mat3f::ID, -position);
	setCameraPosition(camera_, matrix);
}
