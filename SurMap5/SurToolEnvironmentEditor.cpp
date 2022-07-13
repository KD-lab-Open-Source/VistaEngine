#include "stdafx.h"

#include "SurMap5.h"
#include "SurToolEnvironmentEditor.h"

#include "Game\Universe.h"
#include "Units\Nature.h"
#include "Units\EnvironmentSimple.h"
#include "Game\RenderObjects.h"
#include "Game\CameraManager.h"

#include "DebugUtil.h"

IMPLEMENT_DYNAMIC(CSurToolEnvironmentEditor, CSurToolBase)

BEGIN_MESSAGE_MAP(CSurToolEnvironmentEditor, CSurToolBase)
    ON_WM_SIZE()
END_MESSAGE_MAP()

CSurToolEnvironmentEditor::CSurToolEnvironmentEditor(BaseUniverseObject* object)
: CSurToolBase(0, 0)
, unit_(safe_cast<UnitEnvironment*>(object))
{

}

CSurToolEnvironmentEditor::~CSurToolEnvironmentEditor()
{

}


void CSurToolEnvironmentEditor::DoDataExchange(CDataExchange* pDX)
{
    CSurToolBase::DoDataExchange(pDX);
}


BOOL CSurToolEnvironmentEditor::OnInitDialog()
{
    CSurToolBase::OnInitDialog();

    layout_.init(this);
	layout_.add(1, 1, 1, 0, IDC_MODEL_NAME_LABEL);

	std::string modelName;
	if(unit_->attr().isEnvironmentBuilding()){
		UnitEnvironmentBuilding* building = safe_cast<UnitEnvironmentBuilding*>(unit_);
		modelName = building->modelName();
	}
	if(unit_->attr().isEnvironmentSimple()){
		UnitEnvironmentSimple* simple = safe_cast<UnitEnvironmentSimple*>(unit_);
		modelName = simple->modelName();
	}
	if(!modelName.empty()){
		std::string::size_type pos = modelName.rfind ("\\") + 1;
		modelName = std::string(modelName.begin() + pos, modelName.end());
	}
   	SetDlgItemText(IDC_MODEL_NAME_LABEL, modelName.c_str());
    return FALSE;
}


void CSurToolEnvironmentEditor::OnSize(UINT nType, int cx, int cy)
{
    CSurToolBase::OnSize(nType, cx, cy);

    layout_.onSize(cx, cy);
}
