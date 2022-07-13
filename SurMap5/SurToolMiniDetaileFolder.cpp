#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolMiniDetaile.h"
#include "SurToolMiniDetaileFolder.h"
#include "Serialization\Serialization.h"
#include "Game\Universe.h"
#include "Serialization\StringTable.h"
#include "Units\UnitAttribute.h"

#include "Game\RenderObjects.h"
#include "Render\src\TileMap.h"

// CSurToolMiniDetaleFolder dialog
IMPLEMENT_DYNAMIC(CSurToolMiniDetaileFolder, CSurToolEmpty)
CSurToolMiniDetaileFolder::CSurToolMiniDetaileFolder(CWnd* pParent /*=NULL*/)
	: CSurToolEmpty(/*CSurToolPlayerFolder::IDD,*/ pParent)
{
	subfolderExpandPermission=SEP_Permission;
	iconInSurToolTree=IconISTT_FolderTools;
    setName("MiniDetaile Textures");
}

CSurToolMiniDetaileFolder::~CSurToolMiniDetaileFolder()
{
}

void CSurToolMiniDetaileFolder::DoDataExchange(CDataExchange* pDX)
{
	CSurToolEmpty::DoDataExchange(pDX);
	//DDX_Control (pDX, IDC_TABLEPATH_EDIT, m_editTablePath);
}

void CSurToolMiniDetaileFolder::serialize(Archive& ar) 
{
	ar.serialize(name_, "NodeName", 0);
	ar.serialize(popUpMenuRestriction, "popUpMenuRestriction", 0);
	//ar.serialize(treeSDTB, "treeSDTB", 0);Запрещаем сериал-ю дочерних эл-ов
}

BOOL CSurToolMiniDetaileFolder::OnInitDialog ()
{
	CSurToolEmpty::OnInitDialog ();
	
	return FALSE;
}


void CSurToolMiniDetaileFolder::FillIn()
{
    CWaitCursor wait_cursor;
	deleteAllChilds();

	if(tileMap){
		for(int i=0; i<cTileMap::miniDetailTexturesNumber; i++){
			children_.push_back(new CSurToolMiniDetail(i));
		}
	}
}

BEGIN_MESSAGE_MAP(CSurToolMiniDetaileFolder, CSurToolEmpty)
END_MESSAGE_MAP()