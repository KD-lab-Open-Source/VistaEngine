#include "stdafx.h"

#include "SurToolTransform.h"

#include "Serialization.h"

#include "..\Render\inc\IRenderDevice.h"
#include "..\Game\RenderObjects.h"

#include "..\Units\BaseUniverseObject.h"

#include "SelectionUtil.h"
#include "EventListeners.h"

// CSurToolTransform dialog

IMPLEMENT_DYNAMIC(CSurToolTransform, CSurToolBase)
CSurToolTransform::CSurToolTransform(CWnd* pParent /*=NULL*/)
: CSurToolBase(getIDD(), pParent)
, localTransform_(false)
, startPoint_(Vect3f::ZERO)
, endPoint_(Vect3f::ZERO)
, selectionCenter_(Vect3f::ZERO)
, selectionRadius_(0.0f)
, buttonPressed_(false)
{
	transformAxis_[0] = false;
	transformAxis_[1] = false;
	transformAxis_[2] = true;
	iconInSurToolTree = IconISTT_FolderTools;
}

CSurToolTransform::~CSurToolTransform()
{
}

void CSurToolTransform::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolTransform, CSurToolBase)
	ON_WM_DESTROY()
	ON_WM_SIZE()

	ON_COMMAND(IDC_TO_SURFACE_LEVEL_BUTTON, OnToSurfaceLevel)
	ON_COMMAND(IDC_TO_SURFACE_NORMAL_BUTTON, OnToSurfaceNormal)

	ON_COMMAND(IDC_X_AXIS_CHECK, OnXAxisCheck)
	ON_COMMAND(IDC_Y_AXIS_CHECK, OnYAxisCheck)
	ON_COMMAND(IDC_Z_AXIS_CHECK, OnZAxisCheck)
END_MESSAGE_MAP()


// CSurToolTransform message handlers


BOOL CSurToolTransform::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	buttonPressed_=false;

	CheckDlgButton(IDC_X_AXIS_CHECK, transformAxis_[0]);
	CheckDlgButton(IDC_Y_AXIS_CHECK, transformAxis_[1]);
	CheckDlgButton(IDC_Z_AXIS_CHECK, transformAxis_[2]);

	if (vMap.isWorldLoaded())
		CallBack_SelectionChanged();

	layout_.init(this);
	layout_.add(1, 0, 1, 0, IDC_TO_SURFACE_LEVEL_BUTTON);
	layout_.add(1, 0, 1, 0, IDC_TO_SURFACE_NORMAL_BUTTON);

	return FALSE;
}

void CSurToolTransform::OnDestroy()
{
	CSurToolBase::OnDestroy();

	layout_.reset();
	buttonPressed_ = false;
}

namespace UniverseObjectActions{

struct ResetRotation : UniverseObjectAction{
	void operator() (BaseUniverseObject& unit) {
		unit.setPose(Se3f(QuatF::ID, unit.position()), true);
	}
};

struct LevelToSurface : UniverseObjectAction{
	void operator() (BaseUniverseObject& unit) {
		Vect3f pos = unit.position();
		pos.z = float(vMap.GetApproxAlt (pos.xi(), pos.yi()));
		unit.setPose(Se3f(unit.orientation(), pos), true);
	}
};

struct ToSurfaceNormal : UniverseObjectAction{
	//ToSurfaceNormal (){}
	void operator()(BaseUniverseObject& unit) {
		UniverseObjectClass objectClass = unit.objectClass();
		if(objectClass == UNIVERSE_OBJECT_ENVIRONMENT || objectClass == UNIVERSE_OBJECT_UNIT){
			Se3f pose (unit.pose());

			Vect3f normal;
			float radius = unit.radius();
			Vect2f analyze_pos (clamp (pose.trans().x, radius, float(vMap.H_SIZE) - radius),
								clamp (pose.trans().y, radius, float(vMap.V_SIZE) - radius));
			pose.trans().z = vMap.analyzeArea(analyze_pos, radius, normal);

			pose.rot() = QuatF(0 * (M_PI/180.0f), Vect3f::K);

			Vect3f cross = Vect3f::K % normal;
			float len = cross.norm();

			if(len > FLT_EPS)
				pose.rot().premult(QuatF(Acos(dot(Vect3f::K, normal)/(normal.norm() + 1e-5)), cross));

			unit.setPose(pose, true);
		}
	}
};



};

/*
void CSurToolTransform::select(Vect2i p1, Vect2i p2)
{
	if(p1.x > p2.x) swap(p1.x,p2.x);
	if(p1.y > p2.y) swap(p1.y,p2.y);
	environment->select(p1, p2);
	universe()->select(p1, p2);

	forEachSelected(StorePose (poses_));
    RadiusExtractor extractor;
	forEachSelected(extractor);
    selectionCenter_ = extractor.center();
    selectionRadius_ = extractor.radius();
}
*/

bool CSurToolTransform::CallBack_LMBDown(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
    buttonPressed_ = true;
    beginTransformation();
	return false;
}

bool CSurToolTransform::CallBack_LMBUp (const Vect3f& coord, const Vect2i&)
{
    if (buttonPressed_) {
        finishTransformation ();
    }
	buttonPressed_ = false;
	return false;
}


bool CSurToolTransform::CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	cursorCoord_ = scrCoord;
	return true;
}


bool CSurToolTransform::CallBack_RMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	cancelTransformation ();
	return false;
}

bool CSurToolTransform::CallBack_Delete(void)
{
	::deleteSelectedUniverseObjects();
	eventMaster().eventObjectChanged().emit();
	return true;
}

void CSurToolTransform::drawCircle (const Se3f& position, float radius, const sColor4c& color)
{
	Vect3f point;

	int count = 36;
	float angle_step = M_PI * 2.0f / float(count);
	Vect3f lastPoint (Vect3f::ZERO);
	for (int i = 0; i <= count; ++i) {
		float angle = angle_step * float(i);
		point.set (sinf(angle) * radius, cosf(angle) * radius, 0.0f);
		position.xformPoint (point);
		if (i) {
			gb_RenderDevice->DrawLine (lastPoint, point, color);
		}
		lastPoint = point;
	}
}

void drawFilledArc (const Se3f& position, float radius, float start_angle, float end_angle, const sColor4c& start_color, const sColor4c& end_color)
{
	Vect3f point;

	int count = 36;
	float angle_step = M_PI * 2.0f / float(count);
	Vect3f last_point (Vect3f::ZERO);

	typedef sVertexXYZD Vertex;
	cVertexBuffer<Vertex>* buffer = gb_RenderDevice->GetBufferXYZD ();
	Vertex* vertices = buffer->Lock (count * 3 - 3);

	for (int i = 0; i < count; ++i) {
		float angle = start_angle + float(i) * angle_step;
		point.set (sin (angle) * radius, cos (angle) * radius, 0.0f);
		position.xformPoint (point);
		if (i) {
			sColor4c color;

			vertices [0].pos = position.trans();
			vertices [0].diffuse = sColor4c (0, 0, 0, 255);
			vertices [1].pos = last_point;
			color.interpolate (start_color, end_color, float(i - 1) / count);
			vertices [1].diffuse = color;
			vertices [2].pos = point;
			vertices [2].diffuse = end_color;
			color.interpolate (start_color, end_color, float(i) / count);
			vertices += 3;
		}
		last_point = point;
	}
	buffer->Unlock (count * 3 - 3);
}

void CSurToolTransform::drawAxis(const Vect3f& point, float radius, bool axis[3])
{
	gb_RenderDevice->DrawLine(point, point + Vect3f(0.f, 0.f, radius), sColor4c (0, 0, 255, 63 + 192 * axis[2]));
	gb_RenderDevice->DrawLine(point, point + Vect3f(radius, 0.f, 0.f), sColor4c (255, 0, 0, 63 + 192 * axis[0]));
	gb_RenderDevice->DrawLine(point, point + Vect3f(0.f, radius, 0.f), sColor4c (0, 255, 0, 63 + 192 * axis[1]));
}

bool CSurToolTransform::CallBack_DrawAuxData(void)
{/*
	if(mode() == ROTATE) {
		Vect3f position = selectionCenter_;

		drawAxis (position, selectionRadius_, transformAxis_);
		
		Vect3f unitPoint = selectionCenter_;
		Vect3f a = startPoint_ - unitPoint;
		Vect3f b = endPoint_ - unitPoint;
		a.normalize(selectionRadius_);
		b.normalize(selectionRadius_);

		gb_RenderDevice->DrawLine (unitPoint, unitPoint + a, sColor4c(255, 255, 255));
		gb_RenderDevice->DrawLine (unitPoint, unitPoint + b, sColor4c(255, 255, 255));
		Se3f circle_pose (Se3f (QuatF::ID, selectionCenter_));
		if (transformAxisIndex () == 1) {
			circle_pose.rot().set (M_PI * 0.5f, Vect3f::I, 0);
		} else if (transformAxisIndex () == 0) {
			circle_pose.rot().set (M_PI * 0.5f, Vect3f::J, 0);
		} else {
			circle_pose.rot().set (0, Vect3f::I, 0);
		}
		drawCircle (circle_pose, selectionRadius_, sColor4c(255 * transformAxis_[0],
															255 * transformAxis_[1],
															255 * transformAxis_[2], 255));
	} else if (mode() == SCALE) {
		drawAxis (selectionCenter_, selectionRadius_, transformAxis_);

		gb_RenderDevice->DrawLine (selectionCenter_, selectionCenter_ + startPoint_, sColor4c(0, 0, 255));
		gb_RenderDevice->DrawLine (selectionCenter_, selectionCenter_ + endPoint_, sColor4c(255, 255, 255));
	}
	*/
	return true;
}

void CSurToolTransform::cancelTransformation ()
{
	using namespace UniverseObjectActions;
	forEachSelected(RestorePose(poses_, true));

    RadiusExtractor extractor;
	forEachSelected (extractor);
    selectionCenter_ = extractor.center();
    selectionRadius_ = extractor.radius();
}

void CSurToolTransform::finishTransformation ()
{
	using namespace UniverseObjectActions;
	forEachSelected (StorePose (poses_));

	RadiusExtractor extractor;
	forEachSelected (extractor);
    selectionCenter_ = extractor.center();
    selectionRadius_ = extractor.radius();
}


bool CSurToolTransform::CallBack_KeyDown(unsigned int keyCode, bool shift, bool control, bool alt)
{
	if(!vMap.isWorldLoaded())
		return true;

    if (keyCode == VK_ESCAPE) {
        cancelTransformation ();
	} else if (keyCode == VK_RETURN) {
		finishTransformation ();
	} else if (keyCode == 'X') {
		transformAxis_[0] = !transformAxis_[0];
		onTransformAxisChanged(0);
		CheckDlgButton(IDC_X_AXIS_CHECK, transformAxis_[0]);
		CheckDlgButton(IDC_Y_AXIS_CHECK, transformAxis_[1]);
		CheckDlgButton(IDC_Z_AXIS_CHECK, transformAxis_[2]);
	} else if (keyCode == 'Y') {
		transformAxis_[1] = !transformAxis_[1];
		onTransformAxisChanged(1);
		CheckDlgButton(IDC_X_AXIS_CHECK, transformAxis_[0]);
		CheckDlgButton(IDC_Y_AXIS_CHECK, transformAxis_[1]);
		CheckDlgButton(IDC_Z_AXIS_CHECK, transformAxis_[2]);
 	} else if (keyCode == 'Z') {
		transformAxis_[2] = !transformAxis_[2];
		onTransformAxisChanged(2);
		CheckDlgButton(IDC_X_AXIS_CHECK, transformAxis_[0]);
		CheckDlgButton(IDC_Y_AXIS_CHECK, transformAxis_[1]);
		CheckDlgButton(IDC_Z_AXIS_CHECK, transformAxis_[2]);
	} else if (keyCode == 'O') {
		localTransform_ = !localTransform_;
	} else if (keyCode == 'V') {
		OnToSurfaceLevel();
	} else if (keyCode == 'N') {
		OnToSurfaceNormal();
	} else if (keyCode == 'C') {
		/*
		cancelTransformation ();
		forEachSelected (ResetRotation ());
	    forEachSelected (StorePose (poses_));
		*/
    } else {
        return false;
    }
    return true;
}

void CSurToolTransform::CallBack_SelectionChanged ()
{
	using namespace UniverseObjectActions;
	poses_.clear();
	forEachSelected (StorePose (poses_));
    RadiusExtractor extractor;
	forEachSelected (extractor);
    selectionCenter_ = extractor.center();
    selectionRadius_ = extractor.radius();
}

void CSurToolTransform::OnSize(UINT nType, int cx, int cy)
{
	CSurToolBase::OnSize(nType, cx, cy);

	HWND wnd = GetSafeHwnd();
	if(::IsWindow(wnd) && ::IsWindowVisible(wnd)) {
		CRect xRect, yRect, zRect;
		GetDlgItem(IDC_X_AXIS_CHECK)->GetWindowRect(&xRect);
		GetDlgItem(IDC_Y_AXIS_CHECK)->GetWindowRect(&yRect);
		GetDlgItem(IDC_Z_AXIS_CHECK)->GetWindowRect(&zRect);
		ScreenToClient(&xRect);
		ScreenToClient(&yRect);
		ScreenToClient(&zRect);
		int width = zRect.right - xRect.left;
		int left = (cx - width) / 2;
		CPoint offset(left - xRect.left, 0);
		xRect.OffsetRect(offset);
		yRect.OffsetRect(offset);
		zRect.OffsetRect(offset);
		GetDlgItem(IDC_X_AXIS_CHECK)->MoveWindow(&xRect);
		GetDlgItem(IDC_Y_AXIS_CHECK)->MoveWindow(&yRect);
		GetDlgItem(IDC_Z_AXIS_CHECK)->MoveWindow(&zRect);
	}
	layout_.onSize(cx, cy);
}
void CSurToolTransform::OnXAxisCheck()
{
	transformAxis_[0] = IsDlgButtonChecked(IDC_X_AXIS_CHECK);
	onTransformAxisChanged(0);
	CheckDlgButton(IDC_X_AXIS_CHECK, transformAxis_[0]);
	CheckDlgButton(IDC_Y_AXIS_CHECK, transformAxis_[1]);
	CheckDlgButton(IDC_Z_AXIS_CHECK, transformAxis_[2]);
}
void CSurToolTransform::OnYAxisCheck()
{
	transformAxis_[1] = IsDlgButtonChecked(IDC_Y_AXIS_CHECK);
	onTransformAxisChanged(1);
	CheckDlgButton(IDC_X_AXIS_CHECK, transformAxis_[0]);
	CheckDlgButton(IDC_Y_AXIS_CHECK, transformAxis_[1]);
	CheckDlgButton(IDC_Z_AXIS_CHECK, transformAxis_[2]);
}
void CSurToolTransform::OnZAxisCheck()
{
	transformAxis_[2] = IsDlgButtonChecked(IDC_Z_AXIS_CHECK);
	onTransformAxisChanged(2);
	CheckDlgButton(IDC_X_AXIS_CHECK, transformAxis_[0]);
	CheckDlgButton(IDC_Y_AXIS_CHECK, transformAxis_[1]);
	CheckDlgButton(IDC_Z_AXIS_CHECK, transformAxis_[2]);
}

void CSurToolTransform::OnToSurfaceLevel()
{
	using namespace UniverseObjectActions;
	cancelTransformation ();
	forEachSelected (LevelToSurface ());
	forEachSelected (StorePose (poses_));

	RadiusExtractor extractor;
	forEachSelected (extractor);
    selectionCenter_ = extractor.center();
    selectionRadius_ = extractor.radius();
}

void CSurToolTransform::OnToSurfaceNormal()
{
	using namespace UniverseObjectActions;
	cancelTransformation ();
	forEachSelected (ToSurfaceNormal ());
	forEachSelected (StorePose (poses_));

	RadiusExtractor extractor;
	forEachSelected (extractor);
    selectionCenter_ = extractor.center();
    selectionRadius_ = extractor.radius();
}
