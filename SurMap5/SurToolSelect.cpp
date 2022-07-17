#include "stdafx.h"

#include "SurMap5.h"
#include ".\SurToolSelect.h"

/// zzz
#include "MainFrame.h"
#include "GeneralView.h"
// ^^^

#include "Serialization.h"

#include "..\Render\inc\IVisGeneric.h"
#include "..\Game\RenderObjects.h"
#include "..\Units\BaseUniverseObject.h"

#include "SurToolPathEditor.h"
#include "SurToolEnvironmentEditor.h"
#include "SurToolCameraEditor.h"

#include "..\AttribEditor\AttribEditorCtrl.h"

#include "SerializeableUniverseObject.h"
#include "EventListeners.h"

#include "SelectionUtil.h"
#include "SystemUtil.h"

#ifdef __UNIT_ATTRIBUTE_H__
# error "UnitAttribute.h included!"
#endif

std::vector<CSurToolSelect*> CSurToolSelect::instances_;

// _VISTA_ENGINE_EXTERNAL_

IMPLEMENT_DYNAMIC(CSurToolSelect, CSurToolBase)
CSurToolSelect::CSurToolSelect(CWnd* pParent /*=NULL*/)
: CSurToolBase(getIDD(), pParent)
, attribEditor_(0)
, selectionStart_(Vect3f::ZERO)
, randomSelectionChance_(0.5f)

, isSelectionEditable_(false)
, isSelectionMovable_(false)
{
    iconInSurToolTree=IconISTT_FolderTools;
}

CSurToolSelect::~CSurToolSelect()
{
}

void CSurToolSelect::DoDataExchange(CDataExchange* pDX)
{
    CSurToolBase::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSurToolSelect, CSurToolBase)
    ON_WM_DESTROY()
    ON_WM_SIZE()
	ON_BN_CLICKED(IDC_EDIT_BUTTON, OnEditButton)
	ON_BN_CLICKED(IDC_RANDOM_SEL_BUTTON, OnRandomSelectionClicked)
	ON_EN_CHANGE(IDC_RANDOM_SEL_PERCENT_EDIT, OnRandomSelPercentChange)
END_MESSAGE_MAP()


// CSurToolSelect message handlers

BOOL CSurToolSelect::OnInitDialog()
{
    CSurToolBase::OnInitDialog();

	eventMaster().eventObjectChanged().registerListener(this);
	eventMaster().eventSelectionChanged().registerListener(this);

    CRect attribEditorRect;
    GetDlgItem(IDC_ATTRIB_EDITOR)->GetWindowRect(&attribEditorRect);
    GetParent()->ScreenToClient(&attribEditorRect);
	SetDlgItemInt(IDC_RANDOM_SEL_PERCENT_EDIT, round(randomSelectionChance_ * 100.0f));
    
    attribEditor_ = new CAttribEditorCtrl();
    attribEditor_->setStyle(CAttribEditorCtrl::AUTO_SIZE |
                            CAttribEditorCtrl::COMPACT | 
                            CAttribEditorCtrl::HIDE_ROOT_NODE);
    VERIFY(attribEditor_->Create(WS_CHILD, attribEditorRect, this, 0));

	instances_.push_back(this);

    flag_mouseLBDown=false;

	layout_.init(this);
	layout_.add(1, 1, 1, 1, attribEditor_);
	layout_.add(1, 1, 1, 0, IDC_EDIT_BUTTON);
	layout_.add(1, 1, 1, 0, IDC_LABEL);
	layout_.add(1, 1, 1, 1, IDC_SELECTED_INFO_LABEL);
	layout_.add(1, 0, 1, 1, IDC_RANDOM_SEL_BUTTON);
	layout_.add(0, 0, 1, 1, IDC_RANDOM_SEL_PERCENT_EDIT);
	layout_.add(0, 0, 1, 1, IDC_RANDOM_SEL_PERCENT_LABEL);

	if(vMap.isWorldLoaded())
        CallBack_SelectionChanged();
    
	updateLayout();
	return FALSE;
}

void CSurToolSelect::OnDestroy()
{
    CSurToolBase::OnDestroy();

	instances_.erase(std::find(instances_.begin(), instances_.end(), this));

    layout_.reset();
	if(attribEditor_ && ::IsWindow(attribEditor_->GetSafeHwnd()))
		attribEditor_->DestroyWindow();
    attribEditor_ = 0;
    flag_mouseLBDown = false;
}


namespace UniverseObjectActions{

struct AttachToAttribEditor : public UniverseObjectAction{
    AttachToAttribEditor(CAttribEditorCtrl& ctrl)
    : attribEditor_(ctrl)
    {}
	
	void operator()(BaseUniverseObject& object){
		UniverseObjectClass objectClass = object.objectClass();
		if(objectClass == UNIVERSE_OBJECT_SOURCE || objectClass == UNIVERSE_OBJECT_UNIT || objectClass == UNIVERSE_OBJECT_ENVIRONMENT)
			attribEditor_.attachSerializeable(SerializeableUniverseObject(UnitLink<BaseUniverseObject>(&object)));
		else
			attribEditor_.attachSerializeable(Serializeable(object));
	}
	
	CAttribEditorCtrl& attribEditor_;
};

struct CollectSerializeablesAndCount : UniverseObjectAction{
	CollectSerializeablesAndCount(CSurToolSelect::Serializeables& serializeables, CSurToolSelect::SelectionCount& selectionCount)
	: serializeables_(serializeables)
	, selectionCount_(selectionCount)
	{
		selectionCount_ = CSurToolSelect::SelectionCount();
	}

	void operator()(BaseUniverseObject& object){
		switch(object.objectClass()){
		case UNIVERSE_OBJECT_ENVIRONMENT:
			++selectionCount_.numEnvironment;
			serializeables_.push_back(SerializeableUniverseObject(UnitLink<BaseUniverseObject>(&object)));
			return;
		case UNIVERSE_OBJECT_UNIT:
			++selectionCount_.numUnits;
			serializeables_.push_back(SerializeableUniverseObject(UnitLink<BaseUniverseObject>(&object)));
			return;
		case UNIVERSE_OBJECT_SOURCE:
			++selectionCount_.numSources;
			serializeables_.push_back(SerializeableUniverseObject(UnitLink<BaseUniverseObject>(&object)));
			return;
		case UNIVERSE_OBJECT_CAMERA_SPLINE:
			++selectionCount_.numCameras;
			serializeables_.push_back(Serializeable(object, "", ""));
			return;
		case UNIVERSE_OBJECT_ANCHOR:
			++selectionCount_.numAnchors;
			serializeables_.push_back(Serializeable(object, "", ""));
			return;
		}
	}

	CSurToolSelect::Serializeables& serializeables_;
	CSurToolSelect::SelectionCount& selectionCount_;
};

};

bool CSurToolSelect::CallBack_LMBDown(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	flag_mouseLBDown = true;
	
	selectionStart_ = worldCoord;
	if(::selectByScreenRectangle(scrCoord, scrCoord, !isControlPressed(), !isControlPressed() && !isShiftPressed())){
		eventMaster().eventSelectionChanged().emit();
		CallBack_SelectionChanged();
	}
	return false;
}

bool CSurToolSelect::CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	if(flag_mouseLBDown){
		Vect2i screenStart = worldToScreen(selectionStart_);
		Vect2i screenEnd = scrCoord;//worldToScreen(worldCoord);

		bool selectionChanged = ::selectByScreenRectangle(screenStart, screenEnd, 
														  !::isControlPressed(),
														  !::isShiftPressed() && !::isControlPressed());

		eventMaster().eventSelectionChanged().emit();
	}
    return true;
}


bool CSurToolSelect::CallBack_LMBUp (const Vect3f& coord, const Vect2i& scrCoord)
{
	if(flag_mouseLBDown){
		bool selectionChanged = false;
		if(Vect2i(selectionStart_) == Vect2i(coord.xi(), coord.yi())){
			CMainFrame* mainFrame = (CMainFrame*)AfxGetMainWnd();
			CRect rt;
			mainFrame->view().GetClientRect(&rt);
			Vect2f mousePosition(float(scrCoord.x) / float(rt.Width()), float(scrCoord.y) / float(rt.Height()));
			if(BaseUniverseObject* unitHover = unitHoverAll(mousePosition - Vect2f(0.5f, 0.5f), false)){
				if(isShiftPressed()){
					selectionChanged = !unitHover->selected();
					unitHover->setSelected(true);
				}
				else if(isControlPressed()){
					selectionChanged = unitHover->selected();
					unitHover->setSelected(false);
				}
				else{
					deselectAll();
					selectionChanged = true;
					unitHover->setSelected(true);
				}
			}
		}
		else
			selectionChanged = ::selectByScreenRectangle(worldToScreen(selectionStart_), scrCoord, !isControlPressed(), !isControlPressed() && !isShiftPressed());

		if(selectionChanged)
			eventMaster().eventSelectionChanged().emit();
	}
	flag_mouseLBDown = false;
    return true;
}

bool CSurToolSelect::CallBack_RMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
    return false;
}

bool CSurToolSelect::CallBack_Delete(void)
{
	::deleteSelectedUniverseObjects();
	return true;
}

void drawCircle (const Se3f& position, float radius, const sColor4c& color);
void drawFilledArc (const Se3f& position, float radius, float start_angle, float end_angle, const sColor4c& start_color, const sColor4c& end_color);

bool CSurToolSelect::CallBack_DrawAuxData()
{
    if(flag_mouseLBDown){
		Vect2i currentPos = cursorScreenPosition();
		Vect2i startPos = worldToScreen(selectionStart_);
        Vect2i size = currentPos - startPos;
        gb_RenderDevice->DrawRectangle(startPos.x, startPos.y, size.x,  size.y, sColor4c(0, 255, 0, 255), 1);
    }
    return true;
}


bool CSurToolSelect::CallBack_KeyDown(unsigned int keyCode, bool shift, bool control, bool alt)
{
    if(!vMap.isWorldLoaded())
        return true;

    return false;
}

void CSurToolSelect::CallBack_SelectionChanged()
{
	using namespace UniverseObjectActions;
	if(!attribEditor_ || !::IsWindow(attribEditor_->GetSafeHwnd()))
		return;

	serializeables_.clear();
	forEachSelected(CollectSerializeablesAndCount(serializeables_, selectionCount_));

	selectionDescription_ = "";
	isSelectionMovable_ = false;
	if(serializeables_.size() == 1 && forFirstSelected(AttachToAttribEditor(*attribEditor_))){
		GetSelectionInfo getSelectionInfo(isSelectionEditable_, isSelectionMovable_, selectionDescription_);
		forFirstSelected(getSelectionInfo);
	}
	else{
		isSelectionMovable_ = true;
		isSelectionEditable_ = false;
		attribEditor_->detachData();
	}
	updateLayout();
}

void CSurToolSelect::calcSubEditorRect(CRect& rect)
{
	CRect windowRect;
	GetClientRect(&windowRect);
	windowRect.top = windowRect.bottom;
	rect = windowRect;
}

void CSurToolSelect::showControls(bool multiSelect)
{
	if(multiSelect){
		attribEditor_->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_BUTTON)->ShowWindow(SW_SHOWNOACTIVATE);
		GetDlgItem(IDC_LABEL)->ShowWindow(SW_HIDE);

		GetDlgItem(IDC_SELECTED_INFO_LABEL)->ShowWindow(SW_SHOWNOACTIVATE);
		GetDlgItem(IDC_RANDOM_SEL_BUTTON)->ShowWindow(SW_SHOWNOACTIVATE);
		GetDlgItem(IDC_RANDOM_SEL_PERCENT_EDIT)->ShowWindow(SW_SHOWNOACTIVATE);
	}
	else{
		attribEditor_->ShowWindow(SW_SHOWNOACTIVATE);
		GetDlgItem(IDC_EDIT_BUTTON)->ShowWindow(isSelectionEditable_ ? SW_SHOWNOACTIVATE : SW_HIDE);
		GetDlgItem(IDC_LABEL)->ShowWindow(isSelectionEditable_ ? SW_HIDE : SW_SHOWNOACTIVATE);

		GetDlgItem(IDC_SELECTED_INFO_LABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RANDOM_SEL_BUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_RANDOM_SEL_PERCENT_EDIT)->ShowWindow(SW_HIDE);
	}
}

void CSurToolSelect::updateLayout()
{
	if(!attribEditor_)
		return;

	CRect rt;
	calcSubEditorRect(rt);

	const char* text = TRANSLATE(serializeables_.empty() ? "Ничего не выбрано" : (isSelectionEditable_ ? "Редактировать" : "Редактировать общие свойства"));

	if(serializeables_.size() == 1 && !selectionDescription_.empty())
		text = selectionDescription_.c_str();

	SetDlgItemText(IDC_LABEL, text);
	SetDlgItemText(IDC_EDIT_BUTTON, text);

	if(serializeables_.size() > 1){
		XBuffer buf(256, 1);
		buf < TRANSLATE("Выделенеие") < ":\n";
		
		if(selectionCount_.numSources)
			buf < "\t" < TRANSLATE("Источники") < ": " <= selectionCount_.numSources < "\n";
		if(selectionCount_.numEnvironment)
			buf < "\t" < TRANSLATE("Окружение") < ": " <= selectionCount_.numEnvironment < "\n";
		if(selectionCount_.numUnits)
			buf < "\t" < TRANSLATE("Юниты") < ": " <= selectionCount_.numUnits < "\n";
		if(selectionCount_.numCameras)
			buf < "\t" < TRANSLATE("Камеры") < ": " <= selectionCount_.numCameras < "\n";
		if(selectionCount_.numAnchors)
			buf < "\t" < TRANSLATE("Якори") < ": " <= selectionCount_.numAnchors < "\n";

		SetDlgItemText(IDC_SELECTED_INFO_LABEL, buf);
		showControls(true);
	}
	else{

		//GetDlgItem(IDC_EDIT_BUTTON)->EnableWindow(isSelectionEditable_);
		showControls(false);
	}
	enableTransformTools(isSelectionMovable_);
}




void CSurToolSelect::OnSize(UINT nType, int cx, int cy)
{
    CSurToolBase::OnSize(nType, cx, cy);

	updateLayout();
	
    layout_.onSize(cx, cy);
}

void CSurToolSelect::onSelectionChanged()
{
	CallBack_SelectionChanged();
}

void CSurToolSelect::onObjectChanged()
{
	CallBack_SelectionChanged();
}

namespace UniverseObjectActions{
struct RandomDeselect : UniverseObjectAction
{
    RandomDeselect(float chance)
    : chance_(chance)
    {
    }

    void operator()(BaseUniverseObject& object){ 
        float random_value = float(rand()) / (RAND_MAX + 1.0f);
        if(random_value > 1.0f - chance_)
            object.setSelected(false);
    }

    float chance_;
};
};

void CSurToolSelect::serialize(Archive& ar)
{
    CSurToolBase::serialize(ar);
    ar.serialize(randomSelectionChance_, "randomSelectionChance", 0);
}

void CSurToolSelect::OnRandomSelectionClicked()
{
	using namespace UniverseObjectActions;
    forEachSelected(RandomDeselect(1.0f - randomSelectionChance_));
      
	eventMaster().eventSelectionChanged().emit();
}

void CSurToolSelect::OnRandomSelPercentChange()
{
    CString percent;
    GetDlgItemText(IDC_RANDOM_SEL_PERCENT_EDIT, percent);
    randomSelectionChance_ = clamp(float(atoi(percent)) / 100.0f, 0.0f, 1.0f);
}


void CSurToolSelect::OnEditButton()
{
	using namespace UniverseObjectActions;
	xassert(attribEditor_);

	if(serializeables_.size() == 1){
		ShareHandle<CSurToolBase> editor;
		forFirstSelected(EditorCreator(editor));

		if(editor)
			pushEditorMode(editor);
	}
	else if(serializeables_.size() > 1){
		attribEditor_->detachData();

		Serializeables::iterator it;
		FOR_EACH(serializeables_, it){
			Serializeable& ser = *it;
            attribEditor_->mixIn(ser);
		}
		attribEditor_->showMix();
		serializeables_.clear();

		updateLayout();
	}
	else{
	}
}
