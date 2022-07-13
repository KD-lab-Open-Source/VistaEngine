#include "stdafx.h"

#include "SurMap5.h"
#include "SurToolSelect.h"
#include "MainFrame.h"
#include "GeneralView.h"
#include "Serialization\Serialization.h"
#include "Game\RenderObjects.h"
#include "Units\BaseUniverseObject.h"

#include "SurToolPathEditor.h"
#include "SurToolEnvironmentEditor.h"
#include "SurToolCameraEditor.h"

#include "AttribEditor\AttribEditorCtrl.h"

#include "SerializerUniverseObject.h"
#include "EventListeners.h"

#include "SelectionUtil.h"
#include "SystemUtil.h"

#ifdef __UNIT_ATTRIBUTE_H__
# error "UnitAttribute.h included!"
#endif

std::vector<CSurToolSelect*> CSurToolSelect::instances_;

class CSurToolSelectAttribEditor : public CAttribEditorCtrl {
	void onChanged(){
		CSurToolSelect* tool = safe_cast<CSurToolSelect*>(GetParent());
		tool->onPropertyChanged();
	}
};

IMPLEMENT_DYNAMIC(CSurToolSelect, CSurToolBase)
CSurToolSelect::CSurToolSelect(CWnd* pParent /*=NULL*/)
: CSurToolBase(getIDD(), pParent)
, attribEditor_(0)
, selectionStart_(Vect3f::ZERO)
, randomSelectionChance_(0.5f)

, isSelectionEditable_(false)
, isSelectionMovable_(false)
, objectChangedEmitted_(false)
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

	eventMaster().signalObjectChanged().connect(this, &CSurToolSelect::onObjectChanged);
	eventMaster().signalSelectionChanged().connect(this, &CSurToolSelect::onSelectionChanged);

    CRect attribEditorRect;
    GetDlgItem(IDC_ATTRIB_EDITOR)->GetWindowRect(&attribEditorRect);
    GetParent()->ScreenToClient(&attribEditorRect);
	SetDlgItemInt(IDC_RANDOM_SEL_PERCENT_EDIT, round(randomSelectionChance_ * 100.0f));
    
	attribEditor_ = new CSurToolSelectAttribEditor();
    attribEditor_->setStyle(CAttribEditorCtrl::AUTO_SIZE |
                            //CAttribEditorCtrl::COMPACT | 
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
        onSelectionChanged();
    
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
			attribEditor_.attachSerializer(SerializerUniverseObject(UnitLink<BaseUniverseObject>(&object)));
		else
			attribEditor_.attachSerializer(Serializer(object));
	}
	
	CAttribEditorCtrl& attribEditor_;
};

struct CollectSerializersAndCount : UniverseObjectAction{
	CollectSerializersAndCount(CSurToolSelect::Serializers& serializeables, CSurToolSelect::SelectionCount& selectionCount)
	: serializeables_(serializeables)
	, selectionCount_(selectionCount)
	{
		selectionCount_ = CSurToolSelect::SelectionCount();
	}

	void operator()(BaseUniverseObject& object){
		switch(object.objectClass()){
		case UNIVERSE_OBJECT_ENVIRONMENT:
			++selectionCount_.numEnvironment;
			serializeables_.push_back(SerializerUniverseObject(UnitLink<BaseUniverseObject>(&object)));
			return;
		case UNIVERSE_OBJECT_UNIT:
			++selectionCount_.numUnits;
			serializeables_.push_back(SerializerUniverseObject(UnitLink<BaseUniverseObject>(&object)));
			return;
		case UNIVERSE_OBJECT_SOURCE:
			++selectionCount_.numSources;
			serializeables_.push_back(SerializerUniverseObject(UnitLink<BaseUniverseObject>(&object)));
			return;
		case UNIVERSE_OBJECT_CAMERA_SPLINE:
			++selectionCount_.numCameras;
			serializeables_.push_back(Serializer(object, "", ""));
			return;
		case UNIVERSE_OBJECT_ANCHOR:
			++selectionCount_.numAnchors;
			serializeables_.push_back(Serializer(object, "", ""));
			return;
		}
	}

	CSurToolSelect::Serializers& serializeables_;
	CSurToolSelect::SelectionCount& selectionCount_;
};

};

bool CSurToolSelect::onLMBDown(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	flag_mouseLBDown = true;
	
	selectionStart_ = Vect3f(scrCoord.x, scrCoord.y, 0.f); 
	if(::selectByScreenRectangle(scrCoord, scrCoord, !isControlPressed(), !isControlPressed() && !isShiftPressed())){
		eventMaster().signalSelectionChanged().emit(this);
		onSelectionChanged();
	}
	return false;
}

bool CSurToolSelect::onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
	if(flag_mouseLBDown){
		Vect2i screenStart = selectionStart_;
		Vect2i screenEnd = scrCoord;

		bool selectionChanged = ::selectByScreenRectangle(screenStart, screenEnd, 
														  !::isControlPressed(),
														  !::isShiftPressed() && !::isControlPressed());

		eventMaster().signalSelectionChanged().emit(this);
	}
    return true;
}


bool CSurToolSelect::onLMBUp (const Vect3f& coord, const Vect2i& scrCoord)
{
	if(flag_mouseLBDown){
		bool selectionChanged = false;
		if(Vect2i(selectionStart_) == scrCoord){
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
			selectionChanged = ::selectByScreenRectangle(Vect2i(selectionStart_), scrCoord, !isControlPressed(), !isControlPressed() && !isShiftPressed());

		if(selectionChanged)
			eventMaster().signalSelectionChanged().emit(this);
	}
	flag_mouseLBDown = false;
    return true;
}

bool CSurToolSelect::onRMBDown (const Vect3f& worldCoord, const Vect2i& screenCoord)
{
    return false;
}

bool CSurToolSelect::onDelete(void)
{
	::deleteSelectedUniverseObjects();
	return true;
}

void drawCircle (const Se3f& position, float radius, const Color4c& color);
void drawFilledArc (const Se3f& position, float radius, float start_angle, float end_angle, const Color4c& start_color, const Color4c& end_color);

bool CSurToolSelect::onDrawAuxData()
{
    if(flag_mouseLBDown){
		Vect2i currentPos = cursorScreenPosition();
		Vect2i startPos = selectionStart_;
        Vect2i size = currentPos - startPos;
        gb_RenderDevice->DrawRectangle(startPos.x, startPos.y, size.x,  size.y, Color4c(0, 255, 0, 255), 1);
    }
    return true;
}


bool CSurToolSelect::onKeyDown(unsigned int keyCode, bool shift, bool control, bool alt)
{
    if(!vMap.isWorldLoaded())
        return true;

    return false;
}

void CSurToolSelect::onSelectionChanged()
{
	using namespace UniverseObjectActions;
	if(!attribEditor_ || !::IsWindow(attribEditor_->GetSafeHwnd()))
		return;

	serializeables_.clear();
	forEachSelected(CollectSerializersAndCount(serializeables_, selectionCount_));

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
		buf < TRANSLATE("Выделение") < ":\n";
		
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

void CSurToolSelect::onSelectionChanged(SelectionObserver* changer)
{
	onSelectionChanged();
}

void CSurToolSelect::onObjectChanged(ObjectObserver* changer)
{
	if(!objectChangedEmitted_)
		onSelectionChanged();
}

void CSurToolSelect::onPropertyChanged()
{
	objectChangedEmitted_ = true;
	eventMaster().signalObjectChanged().emit(this);
	objectChangedEmitted_ = true;
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
      
	eventMaster().signalSelectionChanged().emit(this);
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

		Serializers::iterator it;
		FOR_EACH(serializeables_, it){
			Serializer& ser = *it;
            attribEditor_->mixIn(ser);
		}
		attribEditor_->showMix();
		serializeables_.clear();

		updateLayout();
	}
	else{
	}
}
