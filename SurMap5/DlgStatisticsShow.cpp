// DlgStatisticsShow.cpp : implementation file
//

#include "stdafx.h"
#include "DlgStatisticsShow.h"
#include "DlgTexturesStatistics.h"
#include "..\3dx\Lib3dx.h"
#include "..\Game\CameraManager.h"
#include ".\dlgstatisticsshow.h"

// CDlgStatisticsShow dialog
extern class cScene* terScene;
extern CameraManager* cameraManager;

IMPLEMENT_DYNAMIC(CDlgStatisticsShow, CDialog)
CDlgStatisticsShow::CDlgStatisticsShow(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgStatisticsShow::IDD, pParent)
	, m_EditStat(_T(""))
	, m_TotalObjects(_T(""))
	, m_UniqueObjects(_T(""))
	, m_TotalTextures(_T(""))
	, m_TexturesSize(_T(""))
	, m_VertexSize(_T(""))
	, m_bVisibleObjects(FALSE)
{
	totalVertexSize = 0;
	totalTextureSize = 0;
	totalObjects = 0;
}

CDlgStatisticsShow::~CDlgStatisticsShow()
{
}

void CDlgStatisticsShow::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_TOTAL_OBJ, m_TotalObjects);
	DDX_Text(pDX, IDC_UNIQE_OBJ, m_UniqueObjects);
	DDX_Text(pDX, IDC_TOTAL_TEXTURES, m_TotalTextures);
	DDX_Text(pDX, IDC_TOTAL_TEXTURES_SIZE, m_TexturesSize);
	DDX_Text(pDX, IDC_VERTEX_SIZE, m_VertexSize);
	DDX_Control(pDX, IDC_OBJECTS_LIST, m_ObjectsList);
	DDX_Control(pDX, IDC_TEXTURES_LIST, m_TexturesList);
	DDX_Check(pDX, IDC_ONLY_VISIBLE_OBJECTS, m_bVisibleObjects);
	DDX_Control(pDX, IDC_TAB_OBJECTS, m_TabObjects);
	DDX_Control(pDX, IDC_SIMPLY_LIST, m_SimplyList);
}


BEGIN_MESSAGE_MAP(CDlgStatisticsShow, CDialog)
//	ON_WM_SIZE()
ON_NOTIFY(LVN_ITEMCHANGED, IDC_OBJECTS_LIST, OnLvnItemchangedObjectsList)
ON_BN_CLICKED(IDC_ONLY_VISIBLE_OBJECTS, OnBnClickedOnlyVisibleObjects)
ON_BN_CLICKED(IDC_ALL_TEXTURES, OnBnClickedAllTextures)
ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_OBJECTS, OnTcnSelchangeTabObjects)
END_MESSAGE_MAP()

cTexture* CDlgStatisticsShow::GetTexture(string name)
{
	cTexLibrary* texLab  = GetTexLibrary();
	for (int i=0; i<texLab->GetNumberTexture(); i++)
	{
		cTexture* cur;
		cur = texLab->GetTexture(i);
		if (cur->GetName() == name)
			return cur;
	}
	return NULL;
}
void CDlgStatisticsShow::PrepareUniqueObjects()
{
	cCamera* camera = cameraManager->GetCamera();
	for(int i=0; i<objects.size(); i++)
	{
		cObject3dx* obj = objects[i];
		bool ident = false;
		if(!camera->TestVisible(obj->GetPosition(),obj->GetStatic()->bound_box.min,obj->GetStatic()->bound_box.max) && m_bVisibleObjects)
			continue;
		totalObjects ++;
		for (int j=0; j<uniqueObjects.size(); j++)
		{
			UniqObj& uObj = uniqueObjects[j];
			if(strcmp(obj->GetFileName(),uObj.obj->GetFileName()) == 0)
			{
				vector<string> textures1;
				vector<string> textures2;
				obj->GetAllTextureName(textures1);
				uObj.obj->GetAllTextureName(textures2);
				if (textures1 == textures2)
				{
					ident = true;
				}
			}

			if (ident)
			{
				uObj.count++;
				break;
			}
		}
		if(!ident)
		{
			UniqObj addObj;
			addObj.obj = obj;
			addObj.count = 1;
			uniqueObjects.push_back(addObj);
		}
	}
}

void CDlgStatisticsShow::CalcTotalVertexSize()
{
	for (int i=0; i<uniqueObjects.size(); i++)
	{
		cObject3dx* obj = uniqueObjects[i].obj;
		totalVertexSize += obj->GetStatic()->vb.size*obj->GetStatic()->vb_size;
	}
}
CString CDlgStatisticsShow::GetFormatedSize(DWORD size)
{
	CString str;
	str.Format("%d",size);
	for (int j=str.GetLength(); j>0; j-=3)
		str.Insert(j,' ');
	return str;
}

void CDlgStatisticsShow::CalcUniqueTextureSize()
{
	set<string>::iterator it;
	for(it = uniqueTextures.begin(); it != uniqueTextures.end(); ++it)
	{
		cTexture* texture =  GetTexture(*it);
		if (texture)
			totalTextureSize += texture->CalcTextureSize();
	}

}
void CDlgStatisticsShow::SetGeneralInfo()
{
	m_TotalObjects.Format("%d",totalObjects);
	m_UniqueObjects.Format("%d",uniqueObjects.size());
	m_TotalTextures.Format("%d",uniqueTextures.size());
	m_TexturesSize = GetFormatedSize(totalTextureSize);
	m_VertexSize = GetFormatedSize(totalVertexSize);
}
void CDlgStatisticsShow::SetObjectsList()
{
	CString str;
	m_TexturesList.DeleteAllItems();
	m_ObjectsList.DeleteAllItems();
	m_SimplyList.DeleteAllItems();
	for(int i=0; i<uniqueObjects.size(); i++)
	{
		UniqObj& uObj = uniqueObjects[i];
		m_ObjectsList.InsertItem(i,uObj.obj->GetFileName());
		m_ObjectsList.SetItemData(i,i);
		str.Format("%d",uObj.count);
		m_ObjectsList.SetItem(i,1,LVIF_TEXT,str,0,0,0,0);
		SetTextureInfo(uObj.obj,i);
		str.Format("%d",uObj.obj->GetStatic()->vb_size);
		m_ObjectsList.SetItem(i,4,LVIF_TEXT,str,0,0,0,0);
		str.Format("%d",uObj.obj->GetStatic()->vb_size);
		m_ObjectsList.SetItem(i,5,LVIF_TEXT,GetFormatedSize(uObj.obj->GetStatic()->vb.size*uObj.obj->GetStatic()->vb_size),0,0,0,0);
		str.Format("%d",uObj.obj->GetNodeNum());
		m_ObjectsList.SetItem(i,6,LVIF_TEXT,str,0,0,0,0);
		str.Format("%d",uObj.obj->GetMaterialNum());
		m_ObjectsList.SetItem(i,7,LVIF_TEXT,str,0,0,0,0);
	}

	for(int i = 0; i<simply_objects.size(); i++)
	{
		m_SimplyList.InsertItem(i,simply_objects[i].pStatic->file_name.c_str());
		m_SimplyList.SetItemData(i,i);
		
		m_SimplyList.SetItem(i,1,LVIF_TEXT,GetFormatedSize(simply_objects[i].objects.size()),0,0,0,0);

		m_SimplyList.SetItem(i,2,LVIF_TEXT,GetFormatedSize(simply_objects[i].pStatic->GetTexture()->CalcTextureSize()),0,0,0,0);

		m_SimplyList.SetItem(i,3,LVIF_TEXT,GetFormatedSize(simply_objects[i].pStatic->vb_size),0,0,0,0);
		
		m_SimplyList.SetItem(i,4,LVIF_TEXT,GetFormatedSize(simply_objects[i].pStatic->vb.size*simply_objects[i].pStatic->vb_size),0,0,0,0);

		m_SimplyList.SetItem(i,5,LVIF_TEXT,GetFormatedSize(simply_objects[i].objects[0]->GetNodeNum()),0,0,0,0);
	}
}
void CDlgStatisticsShow::SetTextureInfo(cObject3dx* obj, int numInList)
{
	int size = 0;
	CString str;
	vector<string> textures;
	obj->GetAllTextureName(textures);
	str.Format("%d",textures.size());
	m_ObjectsList.SetItem(numInList,2,LVIF_TEXT,str,0,0,0,0);
	for(int i=0; i<textures.size(); i++)
	{
		cTexture* texture =  GetTexture(textures[i]);
		if (texture)
		{
			size += texture->CalcTextureSize();
		}
		uniqueTextures.insert(textures[i]);
	}
	m_ObjectsList.SetItem(numInList,3,LVIF_TEXT,GetFormatedSize(size),0,0,0,0);
}
void CDlgStatisticsShow::SetTextureList(cObject3dx* obj)
{
	m_TexturesList.DeleteAllItems();
	int size = 0;
	CString str;
	vector<string> textures;
	obj->GetAllTextureName(textures);
	for(int i=0; i<textures.size(); i++)
	{
		m_TexturesList.InsertItem(i,textures[i].c_str());
		cTexture* texture =  GetTexture(textures[i]);
		if (texture)
		{
			size = texture->CalcTextureSize();
			m_TexturesList.SetItem(i,1,LVIF_TEXT,GetFormatedSize(size),0,0,0,0);
		}
	}

}

void CDlgStatisticsShow::CalcTotalSimplyObjects()
{
	cCamera* camera = cameraManager->GetCamera();
	for (int i=0; i<simply_objects.size(); i++)
	{
		cStaticSimply3dx* pStatic = simply_objects[i].pStatic;
		totalSimplyVertexSize += pStatic->vb.size*pStatic->vb_size;
		totalSimplyVertexCount += pStatic->vb_size;
		totalSimplyObjectCount += simply_objects[i].objects.size();
	}

}
void CDlgStatisticsShow::Recalculate()
{
	objects.clear();
	uniqueObjects.clear();
	totalObjects = 0;
	totalTextureSize = 0;
	totalVertexSize = 0;
	totalSimplyVertexSize = 0;
	totalSimplyVertexCount = 0;
	totalSimplyObjectCount = 0;
	terScene->GetAllObject3dx(objects);
	simply_objects = terScene->GetAllSimply3dxList();
	PrepareUniqueObjects();
	CalcTotalSimplyObjects();
	SetObjectsList();
	CalcUniqueTextureSize();
	CalcTotalVertexSize();
	SetGeneralInfo();
	UpdateData(FALSE);
}

BOOL CDlgStatisticsShow::OnInitDialog()
{
	CDialog::OnInitDialog();
	ShowObjects3dx = true;
	DWORD style = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
	m_ObjectsList.SetExtendedStyle(style);
	m_SimplyList.SetExtendedStyle(style);
	m_TexturesList.SetExtendedStyle(style);
	m_ObjectsList.InsertColumn(0,"Имя",LVCFMT_LEFT,300);
	m_ObjectsList.InsertColumn(1,"Кол-во",LVCFMT_RIGHT,50);
	m_ObjectsList.InsertColumn(2,"Текстуры",LVCFMT_RIGHT,70);
	m_ObjectsList.InsertColumn(3,"Размер Текстур",LVCFMT_RIGHT,80);
	m_ObjectsList.InsertColumn(4,"Вершины",LVCFMT_RIGHT,70);
	m_ObjectsList.InsertColumn(5,"Размер вершин",LVCFMT_RIGHT,80);
	m_ObjectsList.InsertColumn(6,"Nodes",LVCFMT_RIGHT,50);
	m_ObjectsList.InsertColumn(7,"Материалы",LVCFMT_RIGHT,80);
	m_TexturesList.InsertColumn(0,"Имя",LVCFMT_LEFT,300);
	m_TexturesList.InsertColumn(1,"Размер",LVCFMT_RIGHT,80);

	m_SimplyList.InsertColumn(0,"Имя",LVCFMT_LEFT,300);
	m_SimplyList.InsertColumn(2,"Кол-во",LVCFMT_RIGHT,50);
	m_SimplyList.InsertColumn(3,"Размер Текстур",LVCFMT_RIGHT,80);
	m_SimplyList.InsertColumn(4,"Вершины",LVCFMT_RIGHT,70);
	m_SimplyList.InsertColumn(5,"Размер вершин",LVCFMT_RIGHT,80);
	m_SimplyList.InsertColumn(6,"Nodes",LVCFMT_RIGHT,50);

	m_TabObjects.InsertItem(TCIF_TEXT,0,"Objects 3dx",0,NULL,0,0);
	m_TabObjects.InsertItem(TCIF_TEXT,1,"Simply 3dx",0,NULL,0,0);

	Recalculate();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgStatisticsShow::OnLvnItemchangedObjectsList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	CString str = m_ObjectsList.GetItemText(pNMLV->iItem,0);
	//cObject3dx* obj = (cObject3dx*)m_ObjectsList.GetItemData(pNMLV->iItem);
	for (int i=0; i<uniqueObjects.size(); i++)
	{
		cObject3dx* obj = uniqueObjects[i].obj;
		if (!lstrcmp(obj->GetFileName(),str))
		{
			SetTextureList(obj);
			break;

		}

	}
	//if (obj)
	//{
	//	SetTextureList(obj);
	//}
	*pResult = 0;
}

void CDlgStatisticsShow::OnBnClickedOnlyVisibleObjects()
{
	UpdateData(TRUE);
	Recalculate();
}

void CDlgStatisticsShow::OnBnClickedAllTextures()
{
	CDlgTexturesStatistics dlg;
	dlg.DoModal();
}

void CDlgStatisticsShow::OnTcnSelchangeTabObjects(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	int itm = m_TabObjects.GetCurSel();
	switch (itm) 
	{
		case 0:
		{
			m_ObjectsList.ShowWindow(SW_SHOW);
			m_SimplyList.ShowWindow(SW_HIDE);
			break;
		}
		case 1:
		{
			m_ObjectsList.ShowWindow(SW_HIDE);
			m_SimplyList.ShowWindow(SW_SHOW);
			break;
		}
	}
	*pResult = 0;
}
