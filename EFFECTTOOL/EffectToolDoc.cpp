// EffectToolDoc.cpp : implementation of the CEffectToolDoc class
//

#include "stdafx.h"
#include "EffectTool.h"

#include "EffectToolDoc.h"
#include "DlgLoadSprite.h"
#include "ControlView.h"
#include "EffectTreeView.h"
#include "OptTree.h"

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#undef THIS_FILE
//static char THIS_FILE[] = __FILE__;
//#endif


CEffectToolDoc* _pDoc = 0;
void RepaintGraph();
extern CDrawGraph* _pWndGraph;

/////////////////////////////////////////////////////////////////////////////
// CEffectToolDoc

IMPLEMENT_DYNCREATE(CEffectToolDoc, CDocument)

BEGIN_MESSAGE_MAP(CEffectToolDoc, CDocument)
	//{{AFX_MSG_MAP(CEffectToolDoc)
	ON_COMMAND(ID_TOOLBAR_SPRITE, OnToolbarSprite)
	ON_UPDATE_COMMAND_UI(ID_TOOLBAR_SPRITE, OnUpdateToolbarSprite)
	ON_COMMAND(ID_TB_PAUSE, OnTbPause)
	ON_COMMAND(ID_TB_FIRST, OnTbFirst)
	ON_UPDATE_COMMAND_UI(ID_TB_FIRST, OnUpdateTbFirst)
	ON_COMMAND(ID_TB_LAST, OnTbLast)
	ON_UPDATE_COMMAND_UI(ID_TB_LAST, OnUpdateTbLast)
	ON_COMMAND(ID_TOOLBAR_SAVE, OnToolbarSave)
	ON_UPDATE_COMMAND_UI(ID_TOOLBAR_SAVE, OnUpdateToolbarSave)
	ON_COMMAND(ID_TOOLBAR_OPEN, OnToolbarOpen)
	ON_COMMAND(ID_TB_NEXT, OnTbNext)
	ON_UPDATE_COMMAND_UI(ID_TB_NEXT, OnUpdateTbNext)
	ON_COMMAND(ID_TOOLBAR_EXPORT, OnToolbarExport)
	ON_UPDATE_COMMAND_UI(ID_TOOLBAR_EXPORT, OnUpdateToolbarExport)
	ON_COMMAND(ID_TOOLBAR_SAVEALL, OnToolbarSaveall)
	ON_UPDATE_COMMAND_UI(ID_TOOLBAR_SAVEALL, OnUpdateToolbarSave)
	//}}AFX_MSG_MAP
/*	ON_UPDATE_COMMAND_UI(IDC_COMBO_POS_TYPE, OnUpdateGeneral)
	ON_UPDATE_COMMAND_UI(IDC_COMBO_OUT_TYPE, OnUpdateGeneral)
	ON_UPDATE_COMMAND_UI(IDC_SHOW, OnUpdateGeneral)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_X, OnUpdateLabel)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_Y, OnUpdateLabel)
	ON_UPDATE_COMMAND_UI(IDC_STATIC_Z, OnUpdateLabel)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_X, OnUpdateLabel)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_Y, OnUpdateLabel)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_Z, OnUpdateLabel)*/
	ON_UPDATE_COMMAND_UI(IDC_EMITTER_LIFE_TIME, OnUpdateTime)
	ON_UPDATE_COMMAND_UI(ID_TOOLBAR_OPEN, OnUpdateToolbarOpen)
	ON_UPDATE_COMMAND_UI(ID_SET3DMODEL, OnUpdateSet3dmodel)
	ON_COMMAND(ID_SET3DMODEL, OnSet3dmodel)
	ON_UPDATE_COMMAND_UI(ID_SHOW3DMODEL, OnUpdateShow3dmodel)
	ON_COMMAND(ID_SHOW3DMODEL, OnShow3dmodel)
	ON_UPDATE_COMMAND_UI(ID_SET3DBACKGROUND, OnUpdateSet3dbackground)
	ON_COMMAND(ID_SET3DBACKGROUND, OnSet3dbackground)
	ON_COMMAND(ID_SHOW3DBACKGROUND, OnShow3dbackground)
	ON_UPDATE_COMMAND_UI(ID_SHOW3DBACKGROUND, OnUpdateShow3dbackground)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND_RANGE(ID_VISGROUP_NUM,ID_VISGROUP_NUM+1000,OnVisibleGroupNum)
	ON_COMMAND_RANGE(ID_NODE_NUM,ID_NODE_NUM+1000,OnNodeNum)
	ON_COMMAND_RANGE(ID_CHAIN_NUM,ID_CHAIN_NUM+1000,OnChainNum)
	ON_COMMAND(ID_BUTTON_UNDO, TestUndo)
	ON_COMMAND(ID_BUTTON_PUT, TestPut)
	ON_COMMAND(ID_FILE_SAVE_AS, SaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, UpdSaveAs)
	ON_UPDATE_COMMAND_UI(ID_TOOL_MERGE_FX, OnUpdateToolbarOpen)
	ON_COMMAND(ID_TOOL_MERGE_FX, OnToolbarMergeFx)
	ON_COMMAND(ID_TB_VERTICAL, OnTbVertical)
	ON_UPDATE_COMMAND_UI(ID_TB_VERTICAL, OnUpdateTbVertical)
END_MESSAGE_MAP()

afx_msg void CEffectToolDoc::TestPut()
{
		History().Redo();
}
afx_msg void CEffectToolDoc::TestUndo()
{
	History().Undo();
}

/////////////////////////////////////////////////////////////////////////////
// CEffectToolDoc construction/destruction

CEffectToolDoc::CEffectToolDoc()
: m_StorePath(_T(""))
, m_CurPath3DModel(_T(""))
{
	SetActiveEffect(NULL);
	SetActiveEmitter(NULL);
	SetActiveGroup(NULL);

	m_nCurrentGenerationPoint = 0;
	m_nCurrentParticlePoint = 0;
	m_nCurrentNode = 0;
	m_nCurrentChain = 0;
//	m_nCurrentGroup = 0;

	theApp.scene.SetDocument(this);

//	theApp.scene.LoadWorld();

//	if(!LoadRegistry())
//	InitEmptyDocument();

	m_StorePath = "";

	_pDoc = this;

	m_EffectCenter = Vect3f::ZERO;
	m_NullCenter = Vect3f::ZERO;

	LoadRegistry();
	edit = false;
	v_control = NULL;
	m_pEffTree = NULL;
}

CEffectToolDoc::~CEffectToolDoc()
{
	Init();
 	SaveRegistry();
	Release3DModel();
}

void CEffectToolDoc::Init(void)
{
//	for(int i=0; i<m_group.size(); i++)
//		delete m_group[i];
	m_group.clear();

	SetActiveGroup(NULL);
}

void CEffectToolDoc::InitEmptyDocument()
{
	m_group.clear();//intelligence pointers
	SetActiveGroup(AddGroup());
	SetActiveEffect(ActiveGroup()->Effect(0));
	SetActiveEmitter(ActiveEffect()->Emitter(0));
//	GetEffTree()->UpdateGroup(_pDoc->ActiveGroup());
}

void CEffectToolDoc::LoadRegistry()
{
	CString temp;

	temp = theApp.GetProfileString("EffectCenter", "x", "");
	float fx = atof(temp);
	temp = theApp.GetProfileString("EffectCenter", "y", "");
	float fy = atof(temp);
	temp = theApp.GetProfileString("EffectCenter", "z", "");
	float fz = atof(temp);

	m_EffectCenter.set(fx, fy, fz);
}
void CEffectToolDoc::SaveRegistry()
{
	CString temp;

	temp.Format("%f", m_EffectCenter.x);
	theApp.WriteProfileString("EffectCenter", "x", temp);
	temp.Format("%f", m_EffectCenter.y);
	theApp.WriteProfileString("EffectCenter", "y", temp);
	temp.Format("%f", m_EffectCenter.z);
	theApp.WriteProfileString("EffectCenter", "z", temp);
}

/*
void CEffectToolDoc::SaveRegistry()
{
	CString s;

	EffectStorageType::iterator i;
	FOR_EACH(m_effects, i)
	{
		s += (*i)->name.c_str();
		s += ";";
	}

	theApp.WriteProfileString("State", "Last", s);
}
bool CEffectToolDoc::LoadRegistry()
{
	CString s = theApp.GetProfileString("State", "Last");
	if(s.IsEmpty())
		return false;

	bool bRet = false;

	char cb[255];
	strncpy(cb, s, 255);

	char* token = strtok(cb, ";");
	while(token)
	{
		if(LoadEffect(theApp.m_szDir + token))
			bRet = true;

		token = strtok(0, ";");
	}
	return bRet;
}
*/
/*

bool CEffectToolDoc::EmittersCheckDirty()
{
	EffectStorageType::iterator i;
	for(int g=0; g<m_group.GetSize(); g++)
	{
		FOR_EACH(((CGroupData*)m_group[g])->m_effects, i)
		{
			if((*i)->GetDirty())
				return true;

			EmitterListType::iterator j;
			FOR_EACH((*i)->emitters, j)
				if((*j)->GetDirty())
					return true;
		}
	}
	return false;
}
*/
/*
void CEffectToolDoc::SaveAll()
{
	GroupListType::iterator group;
//	FOR_EACH(m_group, group)
//		(*group)->Save(m_StorePath);//temp
/*
	for(int g=0; g<m_group.GetSize(); g++)
	{
		CString group_path = m_StorePath + "\\" + ((CGroupData*)m_group[g])->m_name;
		_mkdir((LPCTSTR)group_path);
		EffectStorageType::iterator i;
		FOR_EACH(((CGroupData*)m_group[g])->m_effects, i)
		{
			CString effect_path = group_path + "\\" + (*i)->name.c_str();
			_mkdir((LPCTSTR)effect_path);
			SaveEffect(((CGroupData*)m_group[g])->m_name, *i);
		}
	}
*/
//}

void CEffectToolDoc::SetPause()
{
	theApp.scene.bPause = true;

	POSITION pos = GetFirstViewPosition();
	while(pos)
	{
		CControlView* pView = dynamic_cast<CControlView*>(GetNextView(pos));
		if(pView)
			pView->SetPause();
	}   
}
MatXf& CEffectToolDoc::GetSplineMatrix(MatXf& mat)
{
	float fTime = ActiveEmitter()->Effect2EmitterTime(theApp.scene.GetEffect()->GetTime());

	if(ActiveEmitter()->IsBase())
		((EmitterKeyBase*)ActiveEmitter()->emitter())->GetPosition(fTime, mat);
//	else
//		ActiveEmitter()->data_light->GetPosition(fTime, mat);

	return mat;
}
/*
bool CEffectToolDoc::CheckEffectName(LPCTSTR lpsz)
{

	ASSERT(ActiveGroup());
	EffectStorageType::iterator i;
	FOR_EACH(ActiveGroup()->m_effects, i)
		if((*i)->name == lpsz)
			return false;

	return true;

}
*/
bool  CEffectToolDoc::Load3DModel(int mode, int type)
{
	if(!ActiveEffect()||!ActiveGroup())
		return false;
	if(mode==MODE_FIND)
	{
		if(ActiveEffect()->Path3Dmodel.IsEmpty()||!ActiveEffect()->Show3Dmodel)
			RELEASE(theApp.scene.m_pModel);
		if(ActiveGroup()->m_Path3DBack.IsEmpty()||!ActiveGroup()->m_bShow3DBack)
			RELEASE(theApp.scene.m_pBackModel);

		if(type==TYPE_3DMODEL&&!ActiveEffect()->Show3Dmodel)
			return false;
		if(type==TYPE_3DBACK&&!ActiveGroup()->m_bShow3DBack)
			return false;

		CString path;
		if(type==TYPE_3DMODEL)
			path = ActiveEffect()->Path3Dmodel;
		else
			path = ActiveGroup()->m_Path3DBack;

		CFileFind finder;
		if(!path.IsEmpty()&&finder.FindFile(path))
		{
			theApp.scene.LoadParticleModel(path, type);
			m_CurPath3DModel = ActiveEffect()->Path3Dmodel;

			if(type == TYPE_3DMODEL)
			{
				theApp.scene.m_pModel->SetVisibilityGroup(C3dxVisibilityGroup(ActiveEffect()->GetVisibleGroup()));
				theApp.scene.m_pModel->SetChain(ActiveEffect()->GetChain());
			}
			else if(type == TYPE_3DBACK)
			{
				theApp.scene.m_pBackModel->SetVisibilityGroup(C3dxVisibilityGroup(ActiveEffect()->GetVisibleGroup()));
				theApp.scene.m_pModel->SetChain(ActiveEffect()->GetChain());
			}

			return true;
		}
	}
	else{
	char cb[MAX_PATH];
		_getcwd(cb, 100);
		std::string s = theApp.GetProfileString("Paths","3DModelPath",cb);
		CFileDialog dlg(TRUE, 0, 0, OFN_HIDEREADONLY, "Models (*.3dx)|*.3dx|Models (*.m3d)|*.m3d|All files|*.*||");
		dlg.m_ofn.lpstrInitialDir = (LPSTR)s.c_str();

		if(dlg.DoModal() == IDOK){
			CString name = dlg.GetPathName();
			if(type==TYPE_3DMODEL){
				if(ActiveEmitter())
					return theApp.scene.LoadModelInsideEmitter(name);
				ActiveEffect()->Path3Dmodel = name;
				m_CurPath3DModel = ActiveEffect()->Path3Dmodel;
			}else{
				ActiveGroup()->m_Path3DBack = name;
			}
			theApp.scene.LoadParticleModel(name, type);
			theApp.WriteProfileString("Paths","3DModelPath",name);
			return true;
		}
	}
	return false;
}
/*
void  CEffectToolDoc::ResetEmitterType(EMITTER_CLASS cls)
{
	ASSERT(ActiveEmitter());

	if(ActiveEmitter()->Class()!=cls)
		ActiveEmitter()->Reset(cls);
	m_nCurrentGenerationPoint = 0;
}

float CEffectToolDoc::get_gen_point_time(int nPoint, EmitterKeyBase* pEmitter)
{
	if(pEmitter == 0)	//very strange
		pEmitter = (EmitterKeyBase*)ActiveEmitter()->emitter();

	if(!pEmitter)
		return 0;

	float sc = 0;
	sc = pEmitter->emitter_life_time;
	return pEmitter->num_particle[nPoint].time*sc;
}

void  CEffectToolDoc::set_gen_point_time(int nPoint, float tm, CEmitterData* pEmitter)
{
	if (!pEmitter)  pEmitter =  ActiveEmitter();
	float sc = pEmitter->emitter_life_time();

	sc = tm/sc;
	if(sc > 1.0f)
		sc = 1.0f;

	pEmitter->SetGenerationPointTime(nPoint, sc);
}

float CEffectToolDoc::get_particle_key_time(int nGenPoint, int nParticleKey)
{
	float f = 0;
	if (ActiveEmitter()->IsBase())
	{
		f = ActiveEmitter()->num_particle()[nGenPoint].time*ActiveEmitter()->emitter_life_time() + 
			ActiveEmitter()->GenerationLifeTime(nGenPoint)*ActiveEmitter()->p_size()[nParticleKey].time;
	}else
		f = ActiveEmitter()->emitter_life_time()*ActiveEmitter()->emitter_size()[nParticleKey].time;

	return f;
}
void  CEffectToolDoc::set_particle_key_time(int nGenPoint, int nParticleKey, float tm)
{
	if (ActiveEmitter()->IsBase())
	{
		tm -= ActiveEmitter()->num_particle()[nGenPoint].time*ActiveEmitter()->emitter_life_time();
		tm /= ActiveEmitter()->GenerationLifeTime(nGenPoint);
	}else
		tm /= ActiveEmitter()->emitter_life_time();

	if(tm > 1.0f) tm = 1.0f;
	if(tm < 0)	  tm = 0;

	ActiveEmitter()->SetParticleKeyTime(nParticleKey, tm);
}

void CEffectToolDoc::change_particle_life_time(int nGenPoint, float tm)
{
	if (ActiveEmitter()->IsBase())
	{
		tm -= ActiveEmitter()->num_particle()[nGenPoint].time*ActiveEmitter()->emitter_life_time();
		if(tm > 100.0f)	tm = 100.0f;
		if(tm < 0.001f)	tm = 0.001f;
		ActiveEmitter()->ChangeGenerationLifetime(nGenPoint, tm);
	}
	else
	{
		if(tm > 100.0f)	tm = 100.0f;
		if(tm < 0.001f)	tm = 0.001f;
		ActiveEmitter()->emitter_life_time() = tm;
	}
}
*/

/////////////////////////////////////////////////////////////////////////////
// CEffectToolDoc commands

BOOL CEffectToolDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
//	Init();
	ASSERT(v_control);
	int ret = v_control->KillTimer(1);

	History().ClearHistory();
	m_group.clear();
	bool res = false;
	res = CDocument::OnOpenDocument(lpszPathName); //Serialize() insert empty groups
	sform = true;
	xassert(!m_group.empty());
	SetActiveGroup(m_group.front());
	SetActiveEffect(m_group.front()->Effect(0));
	SetActiveEmitter(NULL);

	Update();
//	LoadProject(lpszPathName);
	v_control->SetTimer(1, 10, 0);
	return res;
}

CString CEffectToolDoc::CopySprite(LPCTSTR name, LPCTSTR export_path)
{
	SetWorkingDir();
	CString from(name);
	CString filename(from.Right(from.GetLength() - from.ReverseFind('\\') -1));
	CString to;
	if(export_path==NULL){
		to = m_StorePath;
		to += "\\";
		to += FOLDER_SPRITE;
	}else{
		to = export_path;
	}
	//
	if(_chdir(to)==-1){
		_mkdir(to);
	}
	//
//	to += "\\";
	to += filename;

//	if(from==to)return to;

	CopyFile(from,to, false);
	SetWorkingDir();
	return filename;
}

void CEffectToolDoc::OnToolbarSprite() 
{
	ASSERT(ActiveEmitter());

	CEmitterData*  pActEmitter = ActiveEmitter();
	CDlgLoadSprite dlg;
	dlg.SetStrTexture(pActEmitter->texture_name().c_str());
	if (ActiveEmitter()->Class()==EMC_COLUMN_LIGHT)
	{
		dlg.SetStrTexture2(pActEmitter->texture2_name().c_str());
		dlg.Texture2Visible() = true;
	}
	char cb[MAX_PATH];
	_getcwd(cb, 100);
	std::string s = theApp.GetProfileString("Paths","SpritePath",cb);
	dlg.m_ofn.lpstrInitialDir = (LPSTR)s.c_str();
	if(dlg.DoModal() == IDOK)
	{
		// Copy Sprite to Project/Image dir
		if (m_StorePath.IsEmpty())
		{
			pActEmitter->texture_name() = dlg.GetStrTexture();
			if (ActiveEmitter()->IsCLight())
				pActEmitter->texture2_name() = dlg.GetStrTexture2();
		}else
		{
			CString name = CopySprite(dlg.GetStrTexture());
			if (name!="")
				name = FOLDER_SPRITE+name;
			pActEmitter->texture_name() = name;
			if (ActiveEmitter()->Class()==EMC_COLUMN_LIGHT)
			{
				name = CopySprite(dlg.GetStrTexture2());
				if (name!="")
					name = FOLDER_SPRITE+name;
				pActEmitter->texture2_name() = name;
			}
		}
		pActEmitter->SetDirty(true);
		theApp.scene.InitEmitters();
		theApp.WriteProfileString("Paths","SpritePath",dlg.GetStrTexture());
	}
}
void CEffectToolDoc::OnUpdateToolbarSprite(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(ActiveEmitter()!=NULL);
//	pCmdUI->Enable(ActiveEmitter() != 0 && !m_StorePath.IsEmpty());
}

void CEffectToolDoc::OnUpdateGeneral(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(ActiveEmitter() && ActiveEmitter()->IsBase());
}

void CEffectToolDoc::OnUpdateLabel(CCmdUI* pCmdUI)
{
	OnUpdateGeneral(pCmdUI);

/*	if(ActiveEmitter())
	{
		CString s;
		switch(pCmdUI->m_nID)
		{
		case IDC_EDIT_X:
			s.Format("%.2f", ActiveEmitter()->particle_position.size.x);
			break;
		}

		pCmdUI->m_pOther->SetWindowText(s);
	}
*/
}

void CEffectToolDoc::OnUpdateTime(CCmdUI* pCmdUI)
{
	if(theApp.scene.GetEffect())
	{
		CString s;
		s.Format("%.2f", theApp.scene.GetEffect()->GetSummaryTime());
		pCmdUI->m_pOther->SetWindowText(s);
	}
}
void CEffectToolDoc::OnTbVertical()
{
	if (_pWndGraph)
		_pWndGraph->SetVerticalDrag(!_pWndGraph->IsVertiacal());
}
void CEffectToolDoc::OnUpdateTbVertical(CCmdUI *pCmdUI)
{
	if(ActiveEmitter() == NULL)
	{
		pCmdUI->Enable(FALSE);
	}else
	if (_pWndGraph)
	{
		pCmdUI->SetCheck(_pWndGraph->IsVertiacal());
		pCmdUI->Enable();
	}else
	{
		pCmdUI->SetCheck(FALSE);
		pCmdUI->Enable(FALSE);
	}
}

void CEffectToolDoc::OnTbPause() 
{
	theApp.scene.bPause = !theApp.scene.bPause;

	RepaintGraph();
	//UpdateAllViews(0);
}

void CEffectToolDoc::OnTbFirst() 
{
	theApp.scene.InitEmitters();
}
void CEffectToolDoc::OnTbLast() 
{
	if(ActiveEmitter())
		theApp.scene.GetEffect()->MoveToTime(ActiveEmitter()->ParticleLifeTime() - 0.001f);
	else
		theApp.scene.GetEffect()->MoveToTime(theApp.scene.GetEffect()->GetSummaryTime() - 0.001f);
}
void CEffectToolDoc::OnTbNext() 
{
	theApp.scene.GetEffect()->MoveToTime(theApp.scene.GetEffect()->GetTime() + 0.01f);
}
void CEffectToolDoc::OnUpdateTbFirst(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(theApp.scene.bPause);
}
void CEffectToolDoc::OnUpdateTbLast(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(theApp.scene.bPause);
}
void CEffectToolDoc::OnUpdateTbNext(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(theApp.scene.bPause);
}

static LPCTSTR lpszFilter = "Effect files|*.effect|All files|*.*||";
void CEffectToolDoc::OnToolbarSave() 
{
	CString s;
	if(ActiveEmitter())
	{
		s.Format("Сохранить эмиттер %s?", ActiveEmitter()->name().c_str());
		if(AfxMessageBox(s, MB_YESNO|MB_ICONQUESTION) == IDYES)
			xassert(0);
//			SaveEmitter(ActiveEmitter(), theApp.m_szDirEmitters);
	}
	else if(ActiveEffect())
	{
		s.Format("Сохранить эффект %s?", ActiveEffect()->name.c_str());
		if(AfxMessageBox(s, MB_YESNO|MB_ICONQUESTION) == IDYES)
			xassert(0);
//			SaveEffect(ActiveGroup()->m_name, ActiveEffect());
	}
}


struct BrowseCBDataType
{
	LPCTSTR lpszPath;

	BrowseCBDataType(LPCTSTR lpsz){
		lpszPath = lpsz; 
	}
};

int __stdcall BrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lp, LPARAM pData) 
{
	BrowseCBDataType* pCBData = (BrowseCBDataType*)pData;

	switch(uMsg) 
	{
	case BFFM_INITIALIZED:
		if(_tcslen((LPCTSTR)pData)) 
			SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)pCBData->lpszPath);
		break;
	}

	return 0;
}
bool BrowseFolder(LPCTSTR szTitle, CString& s, CString& path, int mode)
{
	bool           bRet = false;
	BROWSEINFO     bri;
	//IMalloc*       pMalloc;
	LPMALLOC	   pMalloc;
	LPITEMIDLIST   idlist;
	TCHAR          cbBuf[MAX_PATH];

	CString sDef;
	if(mode)
		sDef = theApp.GetProfileString("Browse", "LastFXPath", path);
	else
		sDef = theApp.GetProfileString("Browse", "LastSpritePath", path);

	if(::SHGetMalloc(&pMalloc) == NOERROR)
	{
		BrowseCBDataType cb_data(sDef);

		memset(&bri, 0, sizeof(bri));
		bri.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();
		bri.lpszTitle = szTitle;
		bri.ulFlags = BIF_RETURNONLYFSDIRS|BIF_BROWSEINCLUDEFILES;
		bri.lpfn = BrowseCallbackProc;
		bri.lParam = (LPARAM)&cb_data;

		if(idlist = ::SHBrowseForFolder(&bri))
		{
			if(::SHGetPathFromIDList(idlist, cbBuf))
			{
				s = cbBuf;
				bRet = true;

				if(mode)
				{
					theApp.WriteProfileString("Browse", "LastFXPath", cbBuf);
				}
				else
					theApp.WriteProfileString("Browse", "LastSpritePath", cbBuf);
			}
			pMalloc->Free(idlist);
		}

		pMalloc->Release();
	}

	return bRet;
}
void CEffectToolDoc::OnToolbarOpen() 
{
	CString fx, sprite;
	if(BrowseFolder("Выберите эффект или эмиттер", fx, m_StorePath, 1))
	{
		if(GetFileAttributes(fx) != FILE_ATTRIBUTE_DIRECTORY&&(fx.Find(".effect")>=0))
		{
			while(_pDoc->GroupsSize())
				_pDoc->GetEffTree()->DeleteGroupView(_pDoc->Group(0));
			_pDoc->SetActiveGroup(_pDoc->AddGroup());
			_pDoc->ActiveGroup()->Clear();
			LoadEffectFromFile(fx);
			_pDoc->ActiveGroup()->m_name = _pDoc->ActiveGroup()->Effect(0)->name.c_str();
			_pDoc->SetActiveEffect(_pDoc->ActiveGroup()->Effect(0));
			UpdateAllViews(0, HINT_UPDATE_TREE);
			GetEffTree()->UpdateGroup(ActiveGroup());
			History().ClearHistory();
		}
/*		else
			if(BrowseFolder("Укажите библиотеку спрайтов", sprite, m_StorePath, 0))
			{
				if(GetFileAttributes(fx) == FILE_ATTRIBUTE_DIRECTORY) 
					LoadEffect(fx, sprite);
				else
					LoadEmitter(ActiveEffect(), fx, sprite);
			}
*/
	}
/*	char cb[MAX_PATH];
	_getcwd(cb, 100);

	CFileDialog dlg(TRUE, 0, 0, OFN_HIDEREADONLY, lpszFilter);
	if(dlg.DoModal() == IDOK)
	{
		m_effects.clear();

		OnOpenDocument(dlg.GetPathName());
		UpdateAllViews(0, HINT_UPDATE_TREE);
	}

	_chdir(cb);
*/
}

///// Load World /////

bool BrowseWorldFolder(CString& s)
{
	BROWSEINFO     bri;
	IMalloc*       pMalloc;
	LPITEMIDLIST   idlist;
	TCHAR          cbBuf[MAX_PATH];

	bool ret=true;
	CFileFind ff;
	CString sDef;

	if(ff.FindFile(s))
		return true;

	if(::SHGetMalloc(&pMalloc) == NOERROR)
	{
		BrowseCBDataType cb_data(s);

		memset(&bri, 0, sizeof(bri));
		bri.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();
		bri.lpszTitle = "Выберите папку ресурсов мира";
		bri.ulFlags = BIF_RETURNONLYFSDIRS|BIF_BROWSEINCLUDEFILES;
		bri.lpfn = BrowseCallbackProc;
		bri.lParam = (LPARAM)&cb_data;

		if(idlist = ::SHBrowseForFolder(&bri))
		{
			if(::SHGetPathFromIDList(idlist, cbBuf))
				s = cbBuf;
			else ret = false;
			pMalloc->Free(idlist);
		}
		else ret = false;
		pMalloc->Release();
	}

	return ret;
}

void CEffectToolDoc::LoadWorld(bool select)
{
	CString cur_path = theApp.GetProfileString("Worlds", "LastWorldsPath", "");
	m_WorldPath = cur_path;
	if (select)
		m_WorldPath = m_WorldPath + "sjkdlfdsasafkj";
	while(BrowseWorldFolder(m_WorldPath))
		if (theApp.scene.LoadWorld(m_WorldPath))
		{
			theApp.WriteProfileString("Worlds", "LastWorldsPath", m_WorldPath);
			break;
		}
}

//////////////////////

void CEffectToolDoc::OnUpdateToolbarSave(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(ActiveGroup()&&m_group.size() && ActiveGroup()->EffectsSize());
}

/*
void CEffectToolDoc::SaveCurrentEffect()
{
	ASSERT(ActiveEffect());
	ASSERT(ActiveGroup());

	CString s;
	s.Format("Сохранить эффект %s?", ActiveEffect()->name.c_str());
	if(AfxMessageBox(s, MB_YESNO|MB_ICONQUESTION) == IDYES)
		SaveEffect(ActiveGroup()->m_name, ActiveEffect());
}
*/
/*
void CEffectToolDoc::SaveCurrentEmitter()
{
	ASSERT(ActiveEmitter());

	CString s;
	s.Format("Сохранить эмиттер %s?", ActiveEmitter()->name().c_str());

	if(AfxMessageBox(s, MB_YESNO|MB_ICONQUESTION) == IDYES)
		SaveEmitter(ActiveEmitter(), theApp.m_szDirEmitters);
}
*/
/*
void CEffectToolDoc::SaveEffect(LPCTSTR strGroup, CEffectData* pEffect)
{

//	CString sFolder = theApp.m_szDir + pEffect->name.c_str();
	CString sFolder(m_StorePath);
	sFolder += "\\";
	sFolder += strGroup;
	sFolder += "\\";
	sFolder += pEffect->name.c_str();
	sFolder += "\\";

	RMDIR(sFolder);
	_mkdir(sFolder);

	EmitterListType::iterator i;
	FOR_EACH(pEffect->emitters, i)
		(*i)->Save(sFolder);
//		SaveEmitter(*i, sFolder);

	pEffect->SetDirty(false);

}
*/
/*
void CEffectToolDoc::SaveEmitter(CEmitterData* pEmitter, LPCTSTR szPath)
{

	CString s(szPath);
	s += pEmitter->name().c_str();
	if (!pEmitter->IsSave())
	{
		string &tex1 = pEmitter->texture_name();
		if (!tex1.empty()) 
			tex1 = FOLDER_SPRITE + CopySprite(tex1.c_str());
		if (pEmitter->IsCLight() && !pEmitter->texture2_name().empty())
			pEmitter->texture2_name() = FOLDER_SPRITE + CopySprite(pEmitter->texture2_name().c_str());
	}
	CSaver sv(s);
	pEmitter->Save(sv);
	pEmitter->SetDirty(false);
	pEmitter->SetSave(true);

}
*/
class EffectLibrary//Для редактора спецэффектов, для игры нужно пользовать EffectLibrary2
{
	vector<EffectKey*> lst;
	string filename;
	bool isLoaded_;

	static bool enable_delete_effect_key;
public:
	EffectLibrary();
	~EffectLibrary();
	static bool IsEnableDeleteEffectKey();

	bool Load(const char* filename,const char* texture_path=NULL);
	bool isLoaded() const { return isLoaded_; }
	static bool LoadEffectNames(const char* filename,list<string>& effect_names);
	EffectKey* Get(const char* name) const;

	const char* GetFileName()const{return filename.c_str();}
	
	typedef vector<EffectKey*>::iterator iterator;
	inline iterator begin(){return lst.begin();}
	inline iterator end(){return lst.end();}
	void ClearNoDelete();

};

////////////////////////////EffectLibrary//////////////////////////////
bool EffectLibrary::enable_delete_effect_key=false;
EffectLibrary::EffectLibrary()
{
	enable_delete_effect_key=false;
	isLoaded_ = false;
}

EffectLibrary::~EffectLibrary()
{
	enable_delete_effect_key=true;
	iterator it;
	FOR_EACH(lst,it)
	{
		delete *it;
	}
	enable_delete_effect_key=false;
}

bool EffectLibrary::IsEnableDeleteEffectKey()
{
	return enable_delete_effect_key;
}

void EffectLibrary::ClearNoDelete()
{
	lst.clear();
}

EffectKey* EffectLibrary::Get(const char* name) const
{
	vector<EffectKey*>::const_iterator it;
	FOR_EACH(lst,it)
	{
		if(stricmp((*it)->name.c_str(),name)==0)
			return *it;
	}

	return NULL;
}

bool EffectLibrary::Load(const char* fname,const char* texture_path)
{
	isLoaded_ = true;

	CLoadDirectoryFile s;
	if(!s.Load(fname))
		return false;
	filename=fname;

	while(CLoadData* ld=s.next())
	if(ld->id==IDS_EFFECTKEY)
	{
		EffectKey* ek=new EffectKey;
		ek->filename=filename;
		lst.push_back(ek);
		ek->Load(ld);

		ek->changeTexturePath(texture_path);
		ek->preloadTexture();
	}

//	xassert(lst.size()==1);
	return true;
}

bool EffectLibrary::LoadEffectNames(const char* filename,list<string>& effect_names)
{
	CLoadDirectoryFile s;
	if(!s.Load(filename))
		return false;
	while(CLoadData* ld=s.next())
	if(ld->id==IDS_EFFECTKEY)
	{
		string effect_name;
		CLoadDirectory rd(ld);
		while(CLoadData* ld=rd.next())
		switch(ld->id)
		{
		case IDS_EFFECTKEY_HEADER:
			{
				CLoadIterator rd(ld);
				rd>>effect_name;
			}
			break;
		}

		if(!effect_name.empty())
			effect_names.push_back(effect_name);
	}

	return true;
}

bool CEffectToolDoc::LoadEffectFromFile(LPCTSTR szFolder)
{
	if(GetFileAttributes(szFolder) == 0xFFFFFFFF)
		return false;
	ASSERT(ActiveGroup());
	CEffectData* pEffect = NULL;
	CString strFXPath(szFolder);
	strFXPath = strFXPath.Left(strFXPath.ReverseFind('\\'));

	EffectLibrary eflib;
	if(eflib.Load(szFolder, (string(strFXPath)+"\\Textures\\").c_str()))
	{
		EffectLibrary::iterator i;
		FOR_EACH(eflib, i)
		{
			SetActiveEffect(ActiveGroup()->AddEffect(new CEffectData(*i)));
			//// Copy Sprites to TEXTURES
/*			EmitterListType::iterator i_e;
			FOR_EACH(pEffect->emitters, i_e)
			{
				CString sprite_name;
				sprite_name = ((*i_e)->texture_name().c_str());

				if(sprite_name.Find('\\')){
					sprite_name = sprite_name.Right(sprite_name.GetLength() - sprite_name.ReverseFind('\\') -1);
				}
				if ((*i_e)->IsSave())
				{
					CString from(strFXPath);
					from += "\\";
					from += FOLDER_TEXTURES;
					from += "\\";
					from += sprite_name;

					CopySprite(from);
				}else
					(*i_e)->texture_name() = strFXPath + "\\Textures\\" + sprite_name;
				if ((*i_e)->IsCLight())
				{
					CString sprite_name;
					sprite_name = ((*i_e)->texture2_name().c_str());

					if(sprite_name.Find('\\')){
						sprite_name = sprite_name.Right(sprite_name.GetLength() - sprite_name.ReverseFind('\\') -1);
					}
					if ((*i_e)->IsSave())
					{
						CString from(strFXPath);
						from += "\\";
						from += FOLDER_TEXTURES;
						from += "\\";
						from += sprite_name;
						CopySprite(from);
					}else
						(*i_e)->texture2_name() = strFXPath + "\\Textures\\" + sprite_name;
				}
			}
			////*/
		}
	}
	return true;
}

/*
bool CEffectToolDoc::LoadEffect(LPCTSTR szFolder, LPCTSTR szFolderSprite)
{

	if(GetFileAttributes(szFolder) == 0xFFFFFFFF)
		return false;
	ASSERT(ActiveGroup());

	ActiveGroup()->m_effects.push_back(SetActiveEffect(new CEffectData));
	
	CEffectData*	pEffect = ActiveEffect();

	CString sFolder(szFolder);

	int n = sFolder.ReverseFind('\\');
	pEffect->name = sFolder.Right(sFolder.GetLength() - n - 1);

	sFolder += "\\*.";

	WIN32_FIND_DATA ff;
	HANDLE fh = ::FindFirstFile(sFolder, &ff);
	if(fh != INVALID_HANDLE_VALUE)
	{
		do
		{
			if((ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				CString s(szFolder);
				s += "\\";
				s += ff.cFileName;

				LoadEmitter(pEffect, s, szFolderSprite);

			}
		}
		while(::FindNextFile(fh, &ff));
		::FindClose(fh);
	}

	UpdateAllViews(0, HINT_UPDATE_TREE);

	return true;

}*/
string CEffectToolDoc::CopyTexture(string path, string& name)
{
	CString sprite_name;
	sprite_name = (name.c_str());

	if(sprite_name.Find('\\')){
		sprite_name = sprite_name.Right(sprite_name.GetLength() - sprite_name.ReverseFind('\\') -1);
	}

	CString from(path.c_str());
	from += "\\";
	from += sprite_name;

	CopySprite(from);
	return (string)sprite_name;
}
/*
void CEffectToolDoc::LoadEmitter(CEffectData* pEffect, LPCTSTR szPath, LPCTSTR szFolderSprite, int mode)
{
	ASSERT(pEffect);

	CLoadDirectoryFile rd;

	if(rd.Load(szPath))
	{
		CEmitterData* p = 0;

		CLoadData* ID = rd.next();
		switch(ID->id)
		{
		case IDS_BUILDKEY_Z:
			p = pEffect->add_emitter(EMC_INTEGRAL_Z);
			break;
		case IDS_BUILDKEY_LIGHT:
			p = pEffect->add_emitter(EMC_LIGHT);
			break;
		case IDS_BUILDKEY_COLUMN_LIGHT:
			p = pEffect->add_emitter(EMC_COLUMN_LIGHT);
			break;
		case IDS_BUILDKEY_SPL:
			p = pEffect->add_emitter(EMC_SPLINE);
			break;
		case IDS_BUILDKEY_INT:
			p = pEffect->add_emitter(EMC_INTEGRAL);
			break;
		case IDS_BUILDKEY_LIGHTING:
			p = pEffect->add_emitter(EMC_LIGHTING);
			break;
		default:
			xassert(0);
			return;
			break;
		}

		if(p)
			p->Load(ID);
		
		// Copy Sprite
		CString sprite_path(szFolderSprite);
		if(!sprite_path.IsEmpty()){
			if(GetFileAttributes(sprite_path) != FILE_ATTRIBUTE_DIRECTORY){
				sprite_path = sprite_path.Left(sprite_path.ReverseFind('\\'));
			}
			p->texture_name() = FOLDER_SPRITE + CopyTexture((string)sprite_path, p->texture_name());
			if (p->IsCLight())
				p->texture2_name() = FOLDER_SPRITE + CopyTexture((string)sprite_path, p->texture2_name());
		}
		//

		if(p){
			int n = 1;
			char cb[255];
			
			strcpy(cb, p->name().c_str());
			while(!pEffect->CheckName(cb, p))
				sprintf(cb, "%s%d", p->name().c_str(), n++);
			p->name() = cb;
		}
	}

	if(mode)
		UpdateAllViews(0, HINT_UPDATE_TREE);

}
*/
struct Texs
{ 
	string tex1, tex2;
	string textures[10];
};

void CEffectToolDoc::OnToolbarExport() //temp rewrite
{
	ASSERT(ActiveGroup());
	char cb[MAX_PATH];
	_getcwd(cb, 100);
	CFileDialog dlg(FALSE, "effect", ActiveGroup()->m_name, OFN_HIDEREADONLY, lpszFilter);
	std::string s = theApp.GetProfileString("Paths","ExportPath",cb);
	dlg.m_ofn.lpstrInitialDir = (LPSTR)s.c_str();
	if(dlg.DoModal() == IDOK)
	{
		FileSaver saver;
		saver.Init(dlg.GetPathName());
		saver.SetData(EXPORT_TO_GAME);

		CString export_path(dlg.GetPathName());
		export_path = export_path.Left(export_path.ReverseFind('\\'));
		CString Texture_path = m_StorePath;
		m_StorePath = export_path;
		for(int i=0;i<ActiveGroup()->EffectsSize();i++)
		{
			vector<Texs> paths;
			CEffectData* eff = ActiveGroup()->Effect(i);
			for(int ie = 0;ie<eff->EmittersSize();ie++)
			{
				CEmitterData *em = eff->Emitter(ie);
				Texs path;
				path.tex1 = em->texture_name().c_str();
				CString from;
				if (em->IsSave())
					from = Texture_path + "\\" + em->texture_name().c_str();
				else from = em->texture_name().c_str();
				CString to(export_path);
				to += "\\";
				to += FOLDER_TEXTURES;

				if (!em->texture_name().empty())
					em->texture_name() = CString(FOLDER_TEXTURES) + CopySprite(from, to);
				for(int i=0; i<10; i++)
				{
					path.textures[i] = em->textureName(i);
					CString from;
					if (em->IsSave())
						from = Texture_path + "\\" + em->textureName(i).c_str();
					else from = em->textureName(i).c_str();
					CString to(export_path);
					to += "\\";
					to += FOLDER_TEXTURES;

					if (!em->textureName(i).empty())
						em->textureName(i) = CString(FOLDER_TEXTURES) + CopySprite(from, to);
				}

				if (em->IsCLight())
				{
					path.tex2 = em->texture2_name().c_str();
					CString from;
					if (em->IsSave())
						from = Texture_path + "\\" + em->texture2_name().c_str();
					else 
						from = em->texture2_name().c_str();
					CString to(export_path);
					to += "\\";
					to += FOLDER_TEXTURES;
					if (!em->texture2_name().empty())
						em->texture2_name() = CString(FOLDER_TEXTURES) + CopySprite(from, to);
				}
				paths.push_back(path);
			}
			eff->prepare_effect_data(NULL,true);
			eff->Save(saver);
			for(int ie = 0;ie<eff->EmittersSize();ie++)
			{
				CEmitterData *em = eff->Emitter(ie);
				em->texture_name() = paths[ie].tex1;
				for(int i=0; i<10; i++)
				{
					em->textureName(i) = paths[ie].textures[i];
				}
				if (em->IsCLight())
					em->texture2_name() = paths[ie].tex2;
			}

		}
		theApp.WriteProfileString("Paths","ExportPath",dlg.GetPathName());
		m_StorePath="";
	}
	_chdir(cb);
	SetWorkingDir();
}

CGroupData* CEffectToolDoc::AddGroup()
{
	CString name = "Group";
	int i = 1;
	do
		name.Format("Group %d", i++);
	while(!CheckGroupName(name));
	m_group.push_back(new CGroupData(name));
	CEffectData* eff = m_group.back()->AddEffect();
//	eff->name = "effect";
	eff->add_emitter();
//	eff->SetExpand(true);
	return m_group.back();
}
int CEffectToolDoc::GroupIndex(CGroupData* group)
{
	int ix = 0;
	GroupListType::iterator it; 
	FOR_EACH(m_group, it)
	{
		if (*it==group)
			return ix;
		ix++;
	}
	ASSERT(0&&"group not found");
	return -1;
}

CGroupData* CEffectToolDoc::Group(int ix)
{
	ASSERT((UINT)ix<m_group.size());
	return m_group[ix];
}

void CEffectToolDoc::DeleteGroup(CGroupData* group)
{
	GroupListType::iterator it = find(m_group.begin(), m_group.end(), group);
	ASSERT(it!=m_group.end());
	m_group.erase(it);//delete *it;
}

void CEffectToolDoc::OnUpdateToolbarExport(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(true);
}

BOOL CEffectToolDoc::CanCloseFrame(CFrameWnd* pFrame) 
{
//	if(EmittersCheckDirty())
	{
//		SetModifiedFlag();
		/*
		switch(AfxMessageBox(IDS_CLOSEFRAMEDIRTY, MB_YESNOCANCEL|MB_ICONQUESTION))
		{
		case IDYES:
//			SaveRegistry();
//			SaveAll();
			break;

		case IDNO:
			return TRUE;

		default:
			return FALSE;
		}
		*/
	}
//	else
//		SaveRegistry();
	if (!History().IsSaved())
		switch(AfxMessageBox(IDS_CLOSEFRAMEDIRTY, MB_YESNOCANCEL|MB_ICONQUESTION))
		{
		case IDYES:
		{
			CString name = GetPathName();
			if (name.IsEmpty())
			{
				CFileDialog dlg(false, "fxp", 0, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, "*.fxp|*.fxp||");
				dlg.m_ofn.lpstrInitialDir = m_StorePath;
				if(dlg.DoModal() == IDOK)
					name = dlg.GetPathName();
				else 
					return FALSE;
			}
			OnSaveDocument(name);
		}
			break;
		case IDNO:
			return TRUE;

		default:
			return FALSE;
		}
	
	return CDocument::CanCloseFrame(pFrame);
}

void CEffectToolDoc::OnToolbarSaveall() 
{
	int t=0;
//	SaveAll();	
}

///////////////////////////
//// Project Serialize ////
///////////////////////////
/*
// Emitter
void CEmitterDeskr::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{	// storing code
		ar << m_name;
		// dummy values
		ar << d1 << d2 << d3;
		ar << i1 << i2 << i3;
	}
	else
	{	// loading code
		ar >> m_name;
		// dummy values
		ar >> d1 >> d2 >> d3;
		ar >> i1 >> i2 >> i3;
	}
}
// Effect
void CEffectDeskr::AddEmitter(LPCTSTR name)
{
	m_emitter.SetSize(m_emitter.GetSize()+1);
	m_emitter[m_emitter.GetSize()-1].m_name = name;
}
void CEffectDeskr::Serialize(CArchive& ar)
{
	int count = 0;
	if (ar.IsStoring())
	{	// storing code
		ar << m_name;
		// dummy values
		ar << m_Path3DModel << d2 << d3;
		ar << m_bExpand << i2 << i3;		
		// array
		count = m_emitter.GetSize();
		ar << count;
		for(int i=0; i<count; i++)
			m_emitter[i].Serialize(ar);
	}
	else
	{	// loading code
		ar >> m_name;
		// dummy values
		ar >> m_Path3DModel >> d2 >> d3;
		ar >> m_bExpand >> i2 >> i3;
		// array
		ar >> count;
		m_emitter.SetSize(count);
		for(int i=0; i<count; i++)
			m_emitter[i].Serialize(ar);
	}
}
// Project
void CEmitterDeskr::operator =(const CEmitterDeskr &source)
{
	memcpy(this, &source, sizeof(CEmitterDeskr));
//	m_name = source.m_name;
//	d1 = source.d1;
//	d2 = source.d2;
//	d3 = source.d3;
//	i1 = source.i1;
//	i2 = source.i2;
//	i3 = source.i3;
}
void CEffectDeskr::operator =(const CEffectDeskr &source)
{
	m_name = source.m_name;
	m_Path3DModel = source.m_Path3DModel;
	d2 = source.d2;
	d3 = source.d3;
	i2 = source.i2;
	i3 = source.i3;
	m_emitter.RemoveAll();
	int size = source.m_emitter.GetSize();
	m_emitter.SetSize(size);
	for(int i=size-1;i>=0;i--)
		m_emitter[i] = source.m_emitter[i];
}
*/
/*
void CGroupData::Reset(CGroupData* group)
{
	m_name			=group->m_name;
	m_Path3DBack	=group->m_Path3DBack;
	m_bShow3DBack	=group->m_bShow3DBack;
	d2				=group->d2;
	d3				=group->d3;
	m_bExpand		=group->m_bExpand;
	i2				=group->i2;
	i3				=group->i3;
	id				=group->id;
	m_effects.resize(group->m_effects.size());
	for(int i=m_effects.size()-1; i>=0; i--)
		m_effects[i] = new CEffectData(group->m_effects[i]);

//	m_effect.RemoveAll();
//	m_effect.SetSize(group->m_effect.GetSize());
//	for(int i=0;i<group->m_effect.GetSize();i++)
//		m_effect[i] = group->m_effect[i];
}

void CGroupData::AddEmitter(LPCTSTR name)
{
//	m_effect[m_effect.GetSize()-1].AddEmitter(name);
}
void CGroupData::AddEffect(LPCTSTR name, LPCTSTR model_name, bool bExpand)
{
	CEffectData* pNewEffect;
	m_effects.push_back(pNewEffect = new CEffectData);
	pNewEffect->add_emitter();

//	m_effect.SetSize(m_effect.GetSize()+1);
//	m_effect[m_effect.GetSize()-1].m_name = name;
//	m_effect[m_effect.GetSize()-1].m_Path3DModel = model_name;
//	m_effect[m_effect.GetSize()-1].m_bExpand = bExpand;
}
void CGroupData::Serialize(CArchive& ar)
{
	int count = 0;
	if (ar.IsStoring())
	{
		ar << m_name;
		ar << m_Path3DBack << d2 << d3;
		ar << m_bExpand << i2 << i3;
		count = m_effects.size();
		ar << count;
		for(int i=0; i<count; i++)
			m_effects[i]->Serialize(ar);
	}
	else
	{	// loading code
		ar >> m_name;
		ar >> m_Path3DBack >> d2 >> d3;
		ar >> m_bExpand >> i2 >> i3;
		ar >> count;
		Clear();
		m_effects.resize(count);
		for(int i=0; i<count; i++)
		{
			m_effects[i] = new CEffectData();
			m_effects[i]->Serialize(ar);
		}
	}
}
void CGroupData::Clear()
{
	EffectStorageType::iterator fx;
	FOR_EACH(m_effects, fx)
		(*fx)->Clear();
	m_effects.clear();
}
*/
void CEffectToolDoc::Serialize(CArchive& ar)
{
	m_StorePath = ar.m_strFileName;
	m_StorePath = m_StorePath.Left(m_StorePath.Find(".fxp"));
	int version = 2;
	int count = 0;
	if (ar.IsStoring())
	{	
		count = m_group.size();
		ar << (version+10000);
		ar << count;
		for(int i=0; i<count; i++)
			((CGroupData*)m_group[i])->Serialize(ar, m_StorePath,version);
	}
	else
	{	// loading code
		
		ar >> version;
		if ((version-10000) < 0)
		{
			count = version;
			version = -1;
		}else
		{
			ar >> count;
			version -= 10000;
		}
		
		for(int i=0; i<count; i++)
		{
			m_group.push_back(new CGroupData);
			((CGroupData*)m_group[i])->Serialize(ar, m_StorePath,version);
		}
	}
}

BOOL CEffectToolDoc::OnNewDocument()
{
	InitEmptyDocument();
	return CDocument::OnNewDocument();
}
/*
bool CEffectToolDoc::LoadProject(CString strProject)
{
	CString cur_path(strProject.Left(strProject.ReverseFind('.')));
	m_StorePath = cur_path;
	SetWorkingDir();

	for(int i=0; i<m_group.GetSize(); i++)
	{
		CString group_path;
		group_path = cur_path + "\\";
		group_path += ((CGroupData*)m_group[i])->m_name;

		EffectStorageType::iterator fx;
		FOR_EACH(((CGroupData*)m_group[i])->m_effects, fx)
//		for(int fx=0; fx<((CGroupData*)m_group[i])->m_effects.GetSize(); fx++)
		{
//			((CGroupData*)m_group[i])->m_effects.push_back(SetActiveEffect(new CEffectData));
//			ActiveEffect()->name =			((CGroupData*)m_group[i])->m_effect[fx].m_name;
//			ActiveEffect()->Path3Dmodel =	((CGroupData*)m_group[i])->m_effect[fx].m_Path3DModel;
//			ActiveEffect()->bExpand =	((CGroupData*)m_group[i])->m_effect[fx].m_bExpand;

//			if(!ActiveEffect()->Path3Dmodel.IsEmpty())
//				Load3DModel(1);

			CString fx_path(group_path);
			fx_path += "\\";
			fx_path += (*fx)->name.c_str();

			EmitterListType::iterator em;
			FOR_EACH((*fx)->emitters, em)
//			for(int mt=0; mt<((CGroupData*)m_group[i])->m_effects[fx].emitters.size(); mt++)
			{
				CString mt_path(fx_path);
				mt_path += "\\";
				mt_path += (*em)->GetSerName();			
				(*em)->Load(mt_path, "");
			}
		}
	}

	History().ClearHistory();
	return true;
}
*/
bool CEffectToolDoc::CheckGroupName(LPCTSTR lpsz)
{
	GroupListType::iterator group;
	FOR_EACH(m_group, group)
		if ((*group)->m_name==lpsz)
			return false;
	return true;
}
/*
CGroupData* CEffectToolDoc::AddGroup(void)
{
	CString name("Group1");
	int i = 1;
	while(!CheckGroupName(name))
	{
		i++;
		CString proto;
		name.Format("Group%d", i);
	}

	CGroupData* pg;
	m_group.Add(pg = new CGroupData(name));
	return pg;
}
*/
/*
void CEffectToolDoc::DeleteCurEffect()
{
	ASSERT(ActiveGroup()->m_effects.size()>1);
	EffectStorageType & effects = ActiveGroup()->m_effects;
	EffectStorageType::iterator it = find(effects.begin(), effects.end(),  pActiveEffect);
	ASSERT(it!=effects.end());
	effects.erase(it);
	delete pActiveEffect;	
	pActiveEffect = ActiveGroup()->m_effects.front();
	Release3DModel();
}
void CEffectToolDoc::DeleteCurrentGroup(void)
{
//	History().Push(ActiveGroup(), true);
	ASSERT(m_group.size()>1);
	GroupListType::iterator it = find(m_group.begin(), m_group.end(), ActiveGroup());
	ASSERT(it!=m_group.end());
	ASSERT(*it == ActiveGroup());
	m_group.erase(it);
	delete ActiveGroup();
	SetActiveGroup(m_group.front());
	Release3DModel();
}
*/
/*
void CEffectToolDoc::SaveProject(void)
{
	for(int i=0; i<m_group.GetSize(); i++)
	{
		// make dir
		CString group_path(m_StorePath);
		group_path += "\\";
		group_path += ((CGroupData*)m_group[i])->m_name;
		_mkdir((LPCTSTR)group_path);
		//
		((CGroupData*)m_group[i])->Init();
		EffectStorageType::iterator it;
		FOR_EACH(((CGroupData*)m_group[i])->m_effects, it)
		{
			// make dir
			CString effect_path(group_path);
			effect_path += "\\";
			effect_path += (*it)->name.c_str();
			_mkdir((LPCTSTR)effect_path);
			//
//			((CGroupData*)m_group[i])->AddEffect((*it)->name.c_str(), (*it)->Path3Dmodel, (*it)->bExpand);
			
//			EmitterListType::iterator i_e;
//			FOR_EACH((*it)->emitters, i_e)
//			{
//				((CGroupData*)m_group[i])->AddEmitter((*i_e)->name().c_str());
//			}
		}
	}
}
*/
BOOL CEffectToolDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	CString path(lpszPathName);

//	CopySprite(theApp.m_szDir + DEFAULT_TXT_FIRE);
//	CopySprite(theApp.m_szDir + DEFAULT_TXT_LIGHT);

//	SaveProject();

//	SaveAll();

	m_StorePathOld = m_StorePath;
	m_StorePath = path.Left(path.ReverseFind('.'));
	SetWorkingDir();
	bool r =  CDocument::OnSaveDocument(lpszPathName);
	SetWorkingDir();
	SetPathName(lpszPathName);
	History().SetSave();
	return r;
}

void CEffectToolDoc::OnUpdateToolbarOpen(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(true);//!m_StorePath.IsEmpty());
}

void CEffectToolDoc::OnUpdateSet3dmodel(CCmdUI *pCmdUI)
{
	if (ActiveEmitter())
		pCmdUI->Enable(ActiveEmitter()->IsBase() && 
						ActiveEmitter()->particle_position().type == EMP_3DMODEL_INSIDE);
	else 
		pCmdUI->Enable(ActiveEffect()!=NULL);
}

void CEffectToolDoc::OnSet3dmodel()
{
	Load3DModel(MODE_LOAD, TYPE_3DMODEL);
	theApp.scene.InitEmitters();
	tr->ShowOptEmiter();
}
void CEffectToolDoc::OnVisibleGroupNum(UINT nID)
{
	int visGroupNum = nID-ID_VISGROUP_NUM;
	if(theApp.scene.m_pModel)
	{
		theApp.scene.m_pModel->SetVisibilityGroup(C3dxVisibilityGroup(visGroupNum));	
		ActiveEffect()->SetVisibleGroup(visGroupNum);
	}
}
void CEffectToolDoc::OnChainNum(UINT nID)
{
	int chainID = nID-ID_CHAIN_NUM;
	if(theApp.scene.m_pModel)
	{
		m_nCurrentChain = chainID;
		theApp.scene.m_pModel->SetChain(m_nCurrentChain);
		ActiveEffect()->SetChain(m_nCurrentChain);
	}
}

void CEffectToolDoc::OnNodeNum(UINT nID)
{
	int visGroupNum = nID-ID_NODE_NUM;
	if(theApp.scene.m_pModel)
	{
		m_nCurrentNode = visGroupNum;
		//theApp.scene.m_pModel->SetVisibilityGroup(visGroupNum);	
		//ActiveEffect()->SetVisibleGroup(visGroupNum);
	}
}

void CEffectToolDoc::OnUpdateShow3dmodel(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(ActiveEffect()!=NULL&&!ActiveEffect()->Path3Dmodel.IsEmpty());
	pCmdUI->SetCheck(ActiveEffect()!=NULL&&ActiveEffect()->Show3Dmodel);
}

void CEffectToolDoc::OnShow3dmodel()
{
	if(ActiveEffect()){
		if(ActiveEffect()->Show3Dmodel)
			ActiveEffect()->Show3Dmodel = false;
		else
			ActiveEffect()->Show3Dmodel = true;
	}
	UpdateAllViews(0);
	theApp.scene.InitEmitters();
	tr->ShowOptEmiter();
}

void CEffectToolDoc::OnUpdateSet3dbackground(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(ActiveGroup()!=NULL);
}

void CEffectToolDoc::OnSet3dbackground()
{
	Load3DModel(MODE_LOAD, TYPE_3DBACK);
}

void CEffectToolDoc::OnShow3dbackground()
{
	if(ActiveGroup()){
		if(ActiveGroup()->m_bShow3DBack)
			ActiveGroup()->m_bShow3DBack = false;
		else
			ActiveGroup()->m_bShow3DBack = true;
	}
	UpdateAllViews(0);
}

void CEffectToolDoc::OnUpdateShow3dbackground(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(ActiveGroup()!=NULL&&!ActiveGroup()->m_Path3DBack.IsEmpty());
	pCmdUI->SetCheck(ActiveGroup()!=NULL&&ActiveGroup()->m_bShow3DBack);
}

void CEffectToolDoc::OnFileOpen()
{
	char cb[MAX_PATH];
	_getcwd(cb, 100);
	std::string s = theApp.GetProfileString("Paths","ProjectPath",cb);
	CFileDialog dlg(true,NULL,NULL,0,"Project files|*.fxp||");
	dlg.m_ofn.lpstrInitialDir = (LPSTR)s.c_str();
	if(dlg.DoModal() == IDOK)
	{
		AfxGetApp()->OpenDocumentFile(dlg.GetPathName());
		theApp.WriteProfileString("Paths","ProjectPath",dlg.GetPathName());
	}
}
void CEffectToolDoc::SetWorkingDir(void)
{
	if(!m_StorePath.IsEmpty()&&_chdir(m_StorePath)==-1){
		_mkdir(m_StorePath);
	}
	if(!m_StorePath.IsEmpty())
		_chdir(m_StorePath);
}

int CEffectToolDoc::Release3DModel(void)
{
	RELEASE(theApp.scene.m_pModel);
	RELEASE(theApp.scene.m_pBackModel);
	return 0;
}

#include "..\Terra\terra.h"

void CEffectToolDoc::MoveEffectCenter(float dx, float dy)
{
	float x, y;
	x = m_EffectCenter.x += dx;
	y = m_EffectCenter.y += dy;

	if(m_EffectCenter.x<0) m_EffectCenter.x = 0;
	if(m_EffectCenter.y<0) m_EffectCenter.y = 0;
	if(m_EffectCenter.x>=vMap.H_SIZE) m_EffectCenter.x = vMap.H_SIZE -1;
	if(m_EffectCenter.y>=vMap.V_SIZE) m_EffectCenter.y = vMap.V_SIZE -1;

//	m_EffectCenter.z = vMap.GetApproxAlt(x, y) + 50;
}


void CEffectToolDoc::Update()
{
//	GetEffTree()->Clear();
	UpdateAllViews(0,0,0);
//	GroupListType::iterator group;
//	FOR_EACH(m_group, group)
		GetEffTree()->UpdateGroup(ActiveGroup());

//	if (v_control)
//		v_control->Update(true);
}


void CEffectToolDoc::SaveAs()
{
	CFileDialog dlg(false, "fxp", 0, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, "*.fxp|*.fxp||");
	dlg.m_ofn.lpstrInitialDir = m_StorePath;
	if(dlg.DoModal() == IDOK)
	{
		CString name = dlg.GetPathName();
		OnSaveDocument(name);
	}
}
void CEffectToolDoc::UpdSaveAs(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(true);
}

void CEffectToolDoc::OnToolbarMergeFx()
{
	CString fx, sprite;
	if(BrowseFolder("Выберите эффект или эмиттер", fx, m_StorePath, 1))
	{
		if(GetFileAttributes(fx) != FILE_ATTRIBUTE_DIRECTORY&&(fx.Find(".effect")>=0))
		{
			LoadEffectFromFile(fx);
			UpdateAllViews(0, HINT_UPDATE_TREE);
			GetEffTree()->UpdateGroup(ActiveGroup());
//			History().ClearHistory();
		}
	}
}
