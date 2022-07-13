#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolUnit.h"
#include "ObjectsManagerWindow.h"
#include "SurToolSelect.h"

#include "Units\IronLegion.h"
#include "Units\UnitAttribute.h"
#include "Units\Squad.h"
#include "Game\Player.h"

BEGIN_MESSAGE_MAP(CSurToolUnit, CSurToolBase)
	ON_WM_HSCROLL()
	ON_WM_DESTROY()
	ON_WM_SIZE()
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CSurToolUnit, CSurToolBase)

CSurToolUnit::CSurToolUnit(CWnd* pParent /*=NULL*/)
: CSurToolBase(getIDD(), pParent)
, unitOnMouse_(0)
{
	seed_ = GetTickCount();
	flag_repeatOperationEnable=false;
    player_ = 0;
	angleSlider_.SetRange (0, 360);
}

CSurToolUnit::~CSurToolUnit()
{
}

void CSurToolUnit::DoDataExchange(CDataExchange* pDX)
{
	CSurToolBase::DoDataExchange(pDX);
}


static UnitBase* createUnit(const AttributeReference& attributeReference, const Se3f& pose, Player* player, bool auxiliary)
{
	Vect3f unitPosition(pose.trans());
	if(UnitBase* unit = player->buildUnit(attributeReference)){
		unit->setAuxiliary(auxiliary);
		if(unit->attr().isLegionary()){
			UnitLegionary* legionary = safe_cast<UnitLegionary*>(unit);
			UnitSquad* squad = safe_cast<UnitSquad*>(player->buildUnit(&*legionary->attr().squad));
			squad->setAuxiliary(auxiliary);
			squad->setPose(Se3f(QuatF::ID, unitPosition), true);
			squad->addUnit(legionary);
		}
		unit->setPose(pose, true);
		return unit;
	}
	return 0;
}

bool CSurToolUnit::onOperationOnMap(int x, int y)
{
    if(player_){
		Se3f pose;
		calculateUnitPose(pose);
		unitAttribute_->setCurrentAttribute(unitAttribute_);
		if(!unitAttribute_->model())
			unitAttribute_->createModel(unitAttribute_->modelName.c_str());
		if(unitAttribute_->model()){
			createUnit(unitAttribute_, pose, player_, false);
			eventMaster().signalObjectChanged().emit(this);
			updateSeed();
		}
		else{
			MessageBox(TRANSLATE("Юнит не может быть создан: в аттрибутах не указана модель, или файл модели не может быть загружен!"), TRANSLATE("Внимание!"), MB_OK | MB_ICONSTOP);
		}
    }
    return true;
}

void CSurToolUnit::onReleaseScene ()
{
	if(unitOnMouse_){
		unitOnMouse_ = 0;
	}
}

bool CSurToolUnit::onTrackingMouse(const Vect3f& worldCoord, const Vect2i& scrCoord)
{
    if (player_)
		updateUnitOnMouse();
    return true;
}

void CSurToolUnit::updateSeed()
{
	seed_ = rand();
}

BOOL CSurToolUnit::OnInitDialog()
{
	CSurToolBase::OnInitDialog();

	angleSlider_.Create(this, IDC_ANGLE_SLIDER, IDC_ANGLE_EDIT, IDC_ANGLE_LESS_BUTTON, IDC_ANGLE_MORE_BUTTON);
	angleDeltaSlider_.Create(this, IDC_D_ANGLE_SLIDER, IDC_D_ANGLE_EDIT, IDC_D_ANGLE_LESS_BUTTON, IDC_D_ANGLE_MORE_BUTTON);

	if(unitOnMouse_)
		unitOnMouse_ = 0;

	layout_.init(this);
	layout_.add(1, 0, 1, 0, IDC_H_LINE);
	layout_.add(1, 0, 1, 0, IDC_ANGLE_SLIDER);
	layout_.add(1, 0, 1, 0, IDC_D_ANGLE_SLIDER);

	updateSeed();
	Se3f pose;
	calculateUnitPose(pose);
	unitAttribute_->setCurrentAttribute(unitAttribute_);
	if(!unitAttribute_->model())
		unitAttribute_->createModel(unitAttribute_->modelName.c_str());
	if(unitAttribute_->model())
		unitOnMouse_ = createUnit(unitAttribute_, pose, player_, true);
	else
		unitOnMouse_ = 0;
	return FALSE;
}

void CSurToolUnit::calculateUnitPose(Se3f& result)
{
	random_.set(seed_);
	float angle = float(angleSlider_.value) + random_.frnd(float(angleDeltaSlider_.value));

	Vect3f worldCoord = cursorPosition();
	result = Se3f(QuatF(angle * (M_PI / 180.0f), Vect3f(0.0f, 0.0f, 1.0f), 0), worldCoord);
}

void CSurToolUnit::updateUnitOnMouse()
{
	Se3f pose;
	calculateUnitPose(pose);

	if(unitOnMouse_){
		unitOnMouse_->setPose(pose, false);
		if(unitOnMouse_->rigidBody())
			unitOnMouse_->rigidBody()->setPose(pose);
	}
}

void CSurToolUnit::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	int id = pScrollBar->GetDlgCtrlID();

	switch(id){
	case IDC_D_ANGLE_SLIDER:
	case IDC_ANGLE_SLIDER:
		updateUnitOnMouse();
		break;
	}

	CSurToolBase::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CSurToolUnit::OnDestroy()
{
	CSurToolBase::OnDestroy();

	if(unitOnMouse_){
		unitOnMouse_->Kill();
		unitOnMouse_ = 0;
	}
}

void CSurToolUnit::setUnitID(const AttributeBase* unitAttribute)
{
    unitAttribute_ = unitAttribute;
	setName(AttributeReference(unitAttribute).unitAttributeID().unitName().c_str());
}

void CSurToolUnit::quant()
{
	if(unitOnMouse_ && !unitOnMouse_->alive()){
		unitOnMouse_ = 0;
		AfxMessageBox(TRANSLATE("Юнит на мышке сдох!"), MB_OK | MB_ICONEXCLAMATION);
	}
}

void CSurToolUnit::OnSize(UINT nType, int cx, int cy)
{
	CSurToolBase::OnSize(nType, cx, cy);
	layout_.onSize (cx, cy);

	Invalidate();
}
