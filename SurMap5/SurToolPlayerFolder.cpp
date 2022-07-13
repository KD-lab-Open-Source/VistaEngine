#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolPlayerFolder.h"
#include "SurToolUnit.h"
#include "Serialization\Serialization.h"
#include "Game\Universe.h"
#include "Serialization\StringTable.h"
#include "Units\UnitAttribute.h"
#include "UnicodeConverter.h"

// CSurToolPlayerFolder dialog
IMPLEMENT_DYNAMIC(CSurToolPlayerFolder, CSurToolEmpty)
CSurToolPlayerFolder::CSurToolPlayerFolder(CWnd* pParent /*=NULL*/)
	: CSurToolEmpty(/*CSurToolPlayerFolder::IDD,*/ pParent)
{
	subfolderExpandPermission=SEP_Permission;
	iconInSurToolTree=IconISTT_FolderTools;
    setName("Undefined player");
}

CSurToolPlayerFolder::~CSurToolPlayerFolder()
{
}

void CSurToolPlayerFolder::DoDataExchange(CDataExchange* pDX)
{
	CSurToolEmpty::DoDataExchange(pDX);
	//DDX_Control (pDX, IDC_TABLEPATH_EDIT, m_editTablePath);
}

void CSurToolPlayerFolder::serialize(Archive& ar) 
{
	ar.serialize(name_, "NodeName", 0);
	ar.serialize(popUpMenuRestriction, "popUpMenuRestriction", 0);
	//ar.serialize(treeSDTB, "treeSDTB", 0);Запрещаем сериал-ю дочерних эл-ов
}

BOOL CSurToolPlayerFolder::OnInitDialog ()
{
	CSurToolEmpty::OnInitDialog ();
	
	return FALSE;
}


void CSurToolPlayerFolder::FillIn()
{
    CWaitCursor wait_cursor;
	AttributeLibrary& library = AttributeLibrary::instance();

	deleteAllChilds();

	const AttributeLibrary::Map& objects_map = library.map();
	AttributeLibrary::Map::const_iterator it;

	if (player_ == universe()->worldPlayer()) {
		FOR_EACH(objects_map, it){
			if (!it->get())
				continue;
			AttributeBase& object = *it->get();
			if ((object.isBuilding() || object.isLegionary()) && !object.internal) {
				CSurToolUnit* tool = new CSurToolUnit();
				tool->setUnitID(&object);
				tool->setPlayer(player_);
				children_.push_back(tool);
			}
		}
	} else if (player_ == 0) {
		FOR_EACH(objects_map, it){
			if (!it->get())
				continue;
			AttributeBase& object = *it->get();
			if((object.isResourceItem() || object.isInventoryItem()) && !object.internal) {
				CSurToolUnit* tool = new CSurToolUnit ();
				tool->setUnitID(&object);
				tool->setPlayer(universe()->worldPlayer());
				children_.push_back(tool);
			}
		}
	} else {
		FOR_EACH(objects_map, it){
			if (!it->get())
				continue;
			AttributeBase& object = *it->get();
			if(!(object.isResourceItem() || object.isInventoryItem()) && it->key().race() == player_->race() && !object.internal){
				CSurToolUnit* tool = new CSurToolUnit ();
				tool->setUnitID(&object);
				tool->setPlayer(player_);
				children_.push_back(tool);
			}
		}
	}
}

void CSurToolPlayerFolder::setPlayer (Player* _player) {
    player_ = _player;
	
	if (player_) {
		std::string label(w2a(player_->name()));
		if (player_ != universe()->worldPlayer()) {
			label += " [";
			label += player_->race().c_str();
			label += "]";
		}
		setName(label.c_str());
	} else {
		setName(TRANSLATE("Предметы"));
	}
}

BEGIN_MESSAGE_MAP(CSurToolPlayerFolder, CSurToolEmpty)
END_MESSAGE_MAP()