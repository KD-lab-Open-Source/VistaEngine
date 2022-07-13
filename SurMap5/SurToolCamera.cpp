#include "stdafx.h"
#include "SurMap5.h"
#include "AttribEditor\AttribEditorCtrl.h"
#include "SurToolCamera.h"
#include "SurToolCameraEditor.h"
#include "SurToolSelect.h"
#include "SelectionUtil.h"
#include "Game\CameraManager.h"
#include "Serialization\Serialization.h"

Vect3f To3D(const Vect2f& pos);

IMPLEMENT_DYNAMIC(CSurToolCamera, CSurToolBase)

CSurToolCamera::CSurToolCamera(CWnd* parent)
: CSurToolEditable(parent)
, cameraSpline_(*new CameraSpline)
{

}

CSurToolCamera::~CSurToolCamera()
{
    delete &cameraSpline_;
}

void CSurToolCamera::DoDataExchange(CDataExchange* pDX)
{
	CSurToolEditable::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolCamera, CSurToolEditable)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CSurToolCamera::OnInitDialog()
{
    CSurToolEditable::OnInitDialog();

	if(cameraManager){
		attribEditor().attachSerializer(Serializer(cameraSpline_));
	}
	return TRUE;
}

bool CSurToolCamera::onOperationOnMap(int x, int y)
{
	using namespace UniverseObjectActions;
    if(cameraManager){
		RadiusExtractor op;
		forEachSelected(op);
		Vect3f position;
		if(op.count())
			position = op.center();
		else
			position = To3D(Vect2f(x, y));

		CameraSpline* spline = new CameraSpline(cameraSpline_);
		CameraCoordinate coord = cameraManager->coordinate();
		spline->setPose(Se3f(QuatF::ID, position), true);
		coord.position() = coord.position() - position;
		spline->push_back(coord);
		cameraManager->addSpline(spline);
		
	
		spline->setSelected(true);
		eventMaster().signalObjectChanged().emit(this);
		pushEditorMode(new CSurToolCameraEditor(spline));
    }
    return true;
}
