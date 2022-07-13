#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolEmpty.h"

IMPLEMENT_DYNAMIC(CSurToolEmpty, CSurToolBase)
CSurToolEmpty::CSurToolEmpty(CWnd* pParent)
	: CSurToolBase(getIDD(), pParent)
{
	iconInSurToolTree = IconISTT_FolderTools;
}

CSurToolEmpty::~CSurToolEmpty()
{
}

void CSurToolEmpty::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolEmpty, CSurToolBase)
END_MESSAGE_MAP()