// CameraDlg.cpp : implementation file
//


#include "stdafx.h"
#include "SurMap5.h"
#include "CameraDlg.h"

#include "MainFrm.h"
#include "EnterNameDlg.h"

#include "..\game\CameraManager.h"
#include "SurToolSelect.h"

// CCameraDlg dialog

IMPLEMENT_DYNAMIC(CCameraDlg, CDialog)
CCameraDlg::CCameraDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCameraDlg::IDD, pParent)
	, time(0)
	, name(_T(""))
	, cycling(FALSE)
{
}

CCameraDlg::~CCameraDlg()
{
}

void CCameraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON1, createCameraButton);
	DDX_Control(pDX, IDC_BUTTON2, deleteCameraButton);
	DDX_Control(pDX, IDC_BUTTON3, playCameraButton);
	DDX_Control(pDX, IDC_BUTTON4, setCPButton);
	DDX_Control(pDX, IDC_BUTTON5, setPCButton);
	DDX_Control(pDX, IDC_BUTTON6, deletePointButton);
	DDX_Control(pDX, IDC_LIST1, CameraList);
	DDX_Text(pDX, IDC_EDIT2, time);
	DDV_MinMaxInt(pDX, time, 1, 1000000);
	DDX_Text(pDX, IDC_EDIT1, name);
	DDX_Check(pDX, IDC_CHECK1, cycling);
	DDX_Control(pDX, IDC_BUTTON7, addPoint);
}


BEGIN_MESSAGE_MAP(CCameraDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, OnBnClickedButton6)
	ON_LBN_SELCHANGE(IDC_LIST1, OnLbnSelchangeList1)
	ON_EN_CHANGE(IDC_EDIT1, OnEnChangeEdit1)
	ON_EN_CHANGE(IDC_EDIT2, getSpl)
	ON_BN_CLICKED(IDC_CHECK1, OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_BUTTON7, OnBnClickedButton7)
END_MESSAGE_MAP()


// CCameraDlg message handlers

// createCamera
void CCameraDlg::OnBnClickedButton1()
{
	CEnterNameDlg dlg;
	int result = dlg.DoModal ();

	if (result = IDOK) {
		RefreshList();
		mouseMode = CREATE_POINTS;
		cameraManager->spline().name = dlg.buf;
		cameraManager->setSelectedSpline(-1);
		cameraManager->setSelectedPointToNull();
		DisablePointsButtons();
	}

	CSurToolSelect::objectChangeNotify();
}

void CCameraDlg::OnMapLBClick(const Vect3f& coord)
{
	if(vMap.isWorldLoaded () && cameraManager) {
		CameraCoordinate cc;
		switch (mouseMode) {
			case CREATE_POINTS:
				cc = cameraManager->coordinate();
				cc.setPosition(Vect3f(coord.x,coord.y,0.0f));
				cameraManager->addPositionToPath(cc);
				break;
			case EDIT_POINTS:
				cameraManager->findNearestCoordinate(coord.x, coord.y);
				if (cameraManager->getSelectedPoint()!=NULL) EnablePointsButtons();
				else DisablePointsButtons();
				break;
			case SELECT_POINTS:
				cameraManager->addPoint(coord.x, coord.y);
				cameraManager->setSelectedPointToNull();
				cameraManager->setSelectedSpline(cameraManager->getSelectedSplineIndex());
				mouseMode = EDIT_POINTS;
				break;
		}
	}
}

void CCameraDlg::OnMapRBClick(const Vect3f& coord)
{

	if (cameraManager->isCameraSplineEmpty()) {
//		cameraManager->findNearestCoordinate(coord.x, coord.y);

	} else {
		cameraManager->savePath(cameraManager->spline().name.c_str());
		cameraManager->erasePath();
		mouseMode = EDIT_POINTS;
		RefreshList();
		CSurToolSelect::objectChangeNotify();
	}
}

void CCameraDlg::RefreshList()
{
	CameraList.ResetContent();

	CameraSplines::const_iterator si;

	FOR_EACH(cameraManager->splines(), si)
		CameraList.AddString(si->name.c_str());
}


void CCameraDlg::OnLbnSelchangeList1()
{
	cameraManager->setSelectedSpline(CameraList.GetCurSel());
	mouseMode = EDIT_POINTS;
//	cameraManager->setSelectedPointToNull();
	EnablePointsButtons();
	deleteCameraButton.EnableWindow();
	playCameraButton.EnableWindow();
	EnablePropGroup();
}

void CCameraDlg::EnablePointsButtons()
{
	setPCButton.EnableWindow();
	setCPButton.EnableWindow();
	deletePointButton.EnableWindow();
	addPoint.EnableWindow();
}

void CCameraDlg::DisablePointsButtons()
{
	setPCButton.EnableWindow(FALSE);
	setCPButton.EnableWindow(FALSE);
	deletePointButton.EnableWindow(FALSE);
	addPoint.EnableWindow(FALSE);
}

// deleteCamera
void CCameraDlg::OnBnClickedButton2()
{
	mouseMode = NONE;
	cameraManager->deleteSelectedSpline();
	RefreshList();
	deleteCameraButton.EnableWindow(FALSE);
	playCameraButton.EnableWindow(FALSE);
	DisablePointsButtons();
	DisablePropGroup();
	CSurToolSelect::objectChangeNotify();
}

void CCameraDlg::OnBnClickedButton6()
{
	cameraManager->deleteSelectedPoint();
	RefreshList();
	DisablePointsButtons();
	CSurToolSelect::objectChangeNotify();
}


void CCameraDlg::OnBnClickedButton4()
{
	CameraCoordinate* cc;
	cc = cameraManager->getSelectedPoint();
	*cc = cameraManager->coordinate();
}

void CCameraDlg::OnBnClickedButton5()
{
	cameraManager->setCoordinate(*(cameraManager->getSelectedPoint()));
}

void CCameraDlg::OnBnClickedButton3()
{
	CameraSpline* cs = cameraManager->getSelectedSpline();

	if(cs!=NULL) {
		cameraManager->loadPath(cs->name.c_str(),false);
		cameraManager->startReplayPath(cs->stepDuration, 1);
	}

	cameraManager->setSelectedSpline(-1);
	CameraList.SetCurSel(-1);
	playCameraButton.EnableWindow(FALSE);
	DisablePropGroup();
}


BOOL CCameraDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	
	// TODO:  Add extra initialization here
	DisablePropGroup();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CCameraDlg::OnEnChangeEdit1()
{
	getSplineProp();
	RefreshList();
}

void CCameraDlg::EnablePropGroup()
{
	setSplineProp();
	GetDlgItem(IDC_EDIT1)->EnableWindow();
	GetDlgItem(IDC_EDIT2)->EnableWindow();
//	GetDlgItem(IDC_STATIC)->EnableWindow();
//	GetDlgItem(IDC_STATIC2)->EnableWindow();
//	GetDlgItem(IDC_STATIC3)->EnableWindow();
	GetDlgItem(IDC_CHECK1)->EnableWindow();
}

void CCameraDlg::DisablePropGroup()
{
	GetDlgItem(IDC_EDIT1)->EnableWindow(FALSE);
	GetDlgItem(IDC_EDIT2)->EnableWindow(FALSE);
//	GetDlgItem(IDC_STATIC)->EnableWindow(FALSE);
//	GetDlgItem(IDC_STATIC2)->EnableWindow(FALSE);
//	GetDlgItem(IDC_STATIC3)->EnableWindow(FALSE);
	GetDlgItem(IDC_CHECK1)->EnableWindow(FALSE);
}

void CCameraDlg::setSplineProp()
{
	CameraSpline* spline = cameraManager->getSelectedSpline();
	if (spline != NULL) {
		name = spline->name.c_str();
		time = spline->stepDuration;
		cycling = spline->cycled;
		UpdateData(FALSE);
	}
}

void CCameraDlg::getSplineProp()
{
	CameraSpline* spline = cameraManager->getSelectedSpline();
	if (spline != NULL) {
		UpdateData(TRUE);
		spline->name = name;
		spline->stepDuration = time;
		spline->cycled = cycling;
	}
}

void CCameraDlg::getSpl()
{
	getSplineProp();
}

void CCameraDlg::OnBnClickedCheck1()
{
	getSplineProp();
}

void CCameraDlg::OnBnClickedButton7()
{
	mouseMode = SELECT_POINTS;
}
