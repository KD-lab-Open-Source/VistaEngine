#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolUnitFolder.h"
#include "SurToolPlayerFolder.h"
#include "SurToolUnit.h"
#include "Serialization\Serialization.h"
#include "Serialization\StringTable.h"
#include "Units\UnitAttribute.h"
#include "Game\Universe.h"

// CSurToolUnitFolder dialog

IMPLEMENT_DYNAMIC(CSurToolUnitFolder, CSurToolEmpty)
CSurToolUnitFolder::CSurToolUnitFolder(CWnd* pParent /*=NULL*/)
	: CSurToolEmpty(/*CSurToolUnitFolder::IDD,*/ pParent)
{
}

CSurToolUnitFolder::~CSurToolUnitFolder()
{
}

void CSurToolUnitFolder::DoDataExchange(CDataExchange* pDX)
{
	CSurToolEmpty::DoDataExchange(pDX);
}

BOOL CSurToolUnitFolder::OnInitDialog ()
{
	CSurToolEmpty::OnInitDialog ();
	
	return FALSE;
}

void CSurToolUnitFolder::serialize(Archive& ar) 
{
	children().clear();
	__super::serialize(ar);
	children().clear();
	FillIn();
}

void CSurToolUnitFolder::FillIn ()
{
    deleteAllChilds();

    if(universe()){
		Player& world_player = *universe()->worldPlayer();
		AttributeLibrary& library (AttributeLibrary::instance());
        PlayerVect& players = universe()->Players;

        PlayerVect::iterator it;
        FOR_EACH(players, it) {
            Player& player = **it;
			if(&player != &world_player){
				CSurToolPlayerFolder* new_folder = new CSurToolPlayerFolder();
				new_folder->setPlayer(&player);
				new_folder->FillIn();
				children_.push_back(new_folder);
			}
        }

        CSurToolPlayerFolder* new_folder = new CSurToolPlayerFolder();
        new_folder->setPlayer(0);
        new_folder->FillIn();
        children_.push_back(new_folder);

		new_folder = new CSurToolPlayerFolder();
        new_folder->setPlayer(&world_player);
        new_folder->FillIn();
        children_.push_back(new_folder);
	}
}

BEGIN_MESSAGE_MAP(CSurToolUnitFolder, CSurToolEmpty)
END_MESSAGE_MAP()


// CSurToolUnitFolder message handlers
