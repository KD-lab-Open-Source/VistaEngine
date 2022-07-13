#include "stdafx.h"
#include "SurMap5.h"
#include "DebugUtil.h"
#include "SurToolAnchor.h"

#include "AttribEditor\AttribEditorCtrl.h"
#include "Environment\Anchor.h"

#include "SurToolAux.h"
#include "Environment\SourceManager.h"
#include "Serialization\Dictionary.h"
#include "Serialization\BinaryArchive.h"
#include "EventListeners.h"
#include "kdw/LibraryTab.h" // дл€ makeName

Anchor* CSurToolAnchor::anchorOnMouse_ = 0;

IMPLEMENT_DYNAMIC(CSurToolAnchor, CSurToolEditable)
CSurToolAnchor::CSurToolAnchor(CWnd* pParent /*=NULL*/)
: CSurToolEditable(pParent)
, anchor_(new Anchor(true))
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

	if(sourceManager){
        killAnchor();
		if(anchor_) {
			anchor_->setRadius(getBrushRadius());
			attribEditor().attachSerializer(Serializer(*anchor_));
		}
		if(originalAnchor()) {
			anchorOnMouse_ = sourceManager->addAnchor(originalAnchor());
			anchorOnMouse_->setPose(Se3f(QuatF::ID, cursorPosition()), true);
		}
	}
	return FALSE;
}

void CSurToolAnchor::onBrushRadiusChanged()
{
	if(anchor_) {
		attribEditor().attachSerializer(Serializer(*anchor_));
		onPropertyChanged();
	}
}

bool CSurToolAnchor::onOperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded () && sourceManager) {
		xassert(originalAnchor());
		std::string comboList;
		const SourceManager::Anchors& anchors = sourceManager->anchors();
		SourceManager::Anchors::const_iterator it;
		FOR_EACH(anchors, it){
			if(*it != anchorOnMouse()){
				if(it != anchors.begin())
					comboList += "|";
				comboList += (*it)->label();
			}
		}

		Anchor* anchor = sourceManager->addAnchor(originalAnchor());
		anchor->enable();
		std::string label = kdw::makeName(comboList.c_str(), anchor->label());

		anchor->setLabel(label.c_str());
		anchor->setPose(Se3f(QuatF::ID, To3D(Vect2f(x,y))), true);
		eventMaster().signalObjectChanged().emit(this);
	}
	return true;
}

bool CSurToolAnchor::onDrawAuxData()
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
	if(sourceManager && anchorOnMouse_) {
        killAnchor();
	}

	CSurToolEditable::OnDestroy();
}

bool CSurToolAnchor::onTrackingMouse(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	if(sourceManager && anchorOnMouse_) {
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
			anchorOnMouse_ = sourceManager->addAnchor(originalAnchor());
		}
		if(anchorOnMouse_) {
			Se3f old_pose = anchorOnMouse_->pose();
			BinaryOArchive oa;     oa.serialize(*anchor, "anchor", "anchor");
			BinaryIArchive ia(oa); ia.serialize(*anchorOnMouse_, "anchor", "anchor");
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
		SourceManager::Anchors::iterator it = std::find(sourceManager->anchors().begin(),sourceManager->anchors().end(), anchorOnMouse_);
		xassert(it != sourceManager->anchors().end());
		sourceManager->anchors().erase(it);
		anchorOnMouse_ = 0;
	}
}
