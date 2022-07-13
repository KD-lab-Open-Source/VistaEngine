#include "stdafx.h"
#include "SurMap5.h"
#include "DebugUtil.h"
#include "SurToolAnchor.h"

#include "..\AttribEditor\AttribEditorCtrl.h"
#include "..\Environment\Anchor.h"

#include "SurToolAux.h"
#include "..\Environment\Environment.h"
#include "EditArchive.h"
#include "Dictionary.h"
#include "EventListeners.h"

Anchor* CSurToolAnchor::anchorOnMouse_ = 0;

IMPLEMENT_DYNAMIC(CSurToolAnchor, CSurToolEditable)
CSurToolAnchor::CSurToolAnchor(CWnd* pParent /*=NULL*/)
: CSurToolEditable(pParent)
, anchor_(new Anchor())
{
}

CSurToolAnchor::~CSurToolAnchor()
{
}

BEGIN_MESSAGE_MAP(CSurToolAnchor, CSurToolEditable)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// CSurToolAnchor message handlers

BOOL CSurToolAnchor::OnInitDialog()
{
    CSurToolEditable::OnInitDialog();

	if(environment){
        killAnchor();
		if(anchor_) {
			anchor_->setRadius(getBrushRadius());
			attribEditor().attachSerializeable(Serializeable(*anchor_));
		}
		if(originalAnchor()) {
			anchorOnMouse_ = environment->addAnchor(originalAnchor());
			anchorOnMouse_->setPose(Se3f(QuatF::ID, cursorPosition()), true);
		}
	}
	return FALSE;
}

void CSurToolAnchor::CallBack_BrushRadiusChanged()
{
	if(anchor_) {
		attribEditor().attachSerializeable(Serializeable(*anchor_));
		onPropertyChanged();
	}
}

std::string makeName(const char* reservedComboList, const char* nameBase);

bool CSurToolAnchor::CallBack_OperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded () && environment) {
		xassert(originalAnchor());
		std::string comboList;
		Environment::Anchors& anchors = environment->anchors();
		Environment::Anchors::iterator it;
		FOR_EACH(anchors, it){
			if(*it != anchorOnMouse()){
				if(it != anchors.begin())
					comboList += "|";
				comboList += (*it)->c_str();
			}
		}

		Anchor* anchor = environment->addAnchor(originalAnchor());
		std::string label = makeName(comboList.c_str(), anchor->c_str());

		anchor->setLabel(label.c_str());
		anchor->setPose(Se3f(QuatF::ID, To3D(Vect2f(x,y))), true);
		eventMaster().eventObjectChanged().emit();
	}
	return true;
}

bool CSurToolAnchor::CallBack_DrawAuxData()
{
	drawCursorCircle();
	return true;
}

void CSurToolAnchor::serialize(Archive& ar)
{
	CSurToolBase::serialize(ar);
	ar.serialize(*anchor_, "anchor", "якорь");
}

void CSurToolAnchor::OnDestroy()
{
	if(environment && anchorOnMouse_) {
        killAnchor();
	}

	CSurToolEditable::OnDestroy();
}

bool CSurToolAnchor::CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	if(environment && anchorOnMouse_) {
		anchorOnMouse_->setPose(Se3f(anchorOnMouse_->orientation(), worldCoord), true);
	}
	return true;
}

void CSurToolAnchor::onPropertyChanged()
{
	if(const Anchor* anchor = originalAnchor()) {
		if(!anchor_ && anchor) {
			if(anchorOnMouse_)
                killAnchor();
			anchorOnMouse_ = environment->addAnchor(originalAnchor());
		}
		if(anchorOnMouse_) {
			Se3f old_pose = anchorOnMouse_->pose();
			EditOArchive oa;     oa.serialize(*anchor, "anchor", "anchor");
			EditIArchive ia(oa); ia.serialize(*anchorOnMouse_, "anchor", "anchor");
			anchorOnMouse_->setPose(old_pose, true);
		}
	}
}

const Anchor* CSurToolAnchor::originalAnchor()
{
	return &*anchor_/* ? &*anchor_ : &*refHolder_.anchor*/;
}

Anchor* CSurToolAnchor::anchorOnMouse()
{
    return anchorOnMouse_;
}

void CSurToolAnchor::setAnchorOnMouse(Anchor* anchor)
{
    anchorOnMouse_ = anchor;
}

void CSurToolAnchor::killAnchor()
{
	if(anchorOnMouse_){
		Environment::Anchors::iterator it = std::find(environment->anchors().begin(),environment->anchors().end(), anchorOnMouse_);
		xassert(it != environment->anchors().end());
		environment->anchors().erase(it);
		anchorOnMouse_ = 0;
	}
}
