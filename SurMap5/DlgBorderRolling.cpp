// DlgBorderRolling.cpp : implementation file
//

#include "stdafx.h"
#include "SurMap5.h"
#include "DlgBorderRolling.h"


// DlgBorderRolling dialog

IMPLEMENT_DYNAMIC(DlgBorderRolling, CDialog)
DlgBorderRolling::DlgBorderRolling(CWnd* pParent /*=NULL*/)
	: CDialog(DlgBorderRolling::IDD, pParent)
	, borderHeight(30)
	, boderAngle(80)
{
}

DlgBorderRolling::~DlgBorderRolling()
{
}

void DlgBorderRolling::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDCE_H, borderHeight);
	DDV_MinMaxUInt(pDX, borderHeight, 0, 511);
	DDX_Text(pDX, IDCE_ANGLE, boderAngle);
	DDV_MinMaxUInt(pDX, boderAngle, 1, 90);
}


BEGIN_MESSAGE_MAP(DlgBorderRolling, CDialog)
END_MESSAGE_MAP()


// DlgBorderRolling message handlers
