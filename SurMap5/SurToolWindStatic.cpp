//

#include "stdafx.h"
#include "SurMap5.h"
#include ".\SurToolWindStatic.h"

#include "MainFrm.h"
#include "..\Environment\Environment.h"
#include "..\Water\Wind.h"
#include "..\Game\CameraManager.h"
#include "SurToolSelect.h"


IMPLEMENT_DYNAMIC(CSurToolWindStatic, CSurToolBase)
CSurToolWindStatic::CSurToolWindStatic(CWnd* pParent /*=NULL*/)
	: CSurToolBase(getIDD(), pParent)

{
	flag_repeatOperationEnable=false;
	mouse_down = false;
}

CSurToolWindStatic::~CSurToolWindStatic()
{
}

void CSurToolWindStatic::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolWindStatic, CSurToolBase)
ON_EN_CHANGE(IDC_EDIT_WINDZ, OnEnChangeEditWindZ)
ON_WM_DESTROY()
ON_BN_CLICKED(IDC_CH_CLEAR_WIND, OnBnClickedChClearWind)
END_MESSAGE_MAP()


// CSurToolWindStatic message handlers


BOOL CSurToolWindStatic::OnInitDialog()
{
	CSurToolBase::OnInitDialog();
	if (environment)
		environment->ShowWindArrow(true);
	GetDlgItem(IDC_EDIT_WINDZ)->SetWindowText("100");
	cur_z = 100;
	is_clear = false;
	return TRUE;
}

bool CSurToolWindStatic::CallBack_LMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	mouse_down = true;
	return true;
}
bool CSurToolWindStatic::CallBack_LMBUp (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	mouse_down = false;
	return true;
}

bool CSurToolWindStatic::CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	static Vect3f prev = worldCoord;
	if (mouse_down && vMap.isWorldLoaded() && cameraManager)
	{
		float z = worldCoord.z + cur_z;
		sPlane4f plane(Vect3f(0,0,z), Vect3f(1,0,z), Vect3f(1,1,z));
		Vect3f camera_pos = cameraManager->GetCamera()->GetPos();
		float t = plane.GetCross(camera_pos, worldCoord);
		Vect3f pos = (worldCoord - camera_pos)*t + camera_pos; 

		CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd ();
		float radius = static_cast<float>(getBrushRadius());;
		if (is_clear)
			environment->getWind()->ClearStaticWind(pos, radius);
		else
		{
			StaticWindQuantInfo dat;
			dat.r = radius;
			dat.type = LINEAR_WIND;
			dat.maxz = cur_z;
			dat.pos.set(pos.x, pos.y);
			Vect3f v = prev - worldCoord;
			v.z = 0; 
			static float k = -5;
			v.Normalize(k);
			dat.vel_wind.set(v.x, v.y);
			environment->getWind()->CreateStaticQuant(dat);
		}
		CSurToolSelect::objectChangeNotify();
	}
	prev = worldCoord;
	return true;
}


bool CSurToolWindStatic::DrawAuxData(void)
{
	drawCursorCircle ();
	return true;
}


void CSurToolWindStatic::OnEnChangeEditWindZ()
{
	CString str;
	GetDlgItem(IDC_EDIT_WINDZ)->GetWindowText(str);
	cur_z = atof(str);
}

void CSurToolWindStatic::OnDestroy()
{
	CSurToolBase::OnDestroy();

	if (environment)
		environment->ShowWindArrow(false);
}

void CSurToolWindStatic::OnBnClickedChClearWind()
{
	is_clear = ((CButton*)GetDlgItem(IDC_CH_CLEAR_WIND))->GetCheck();
	GetDlgItem(IDC_EDIT_WINDZ)->EnableWindow(!is_clear);
}
