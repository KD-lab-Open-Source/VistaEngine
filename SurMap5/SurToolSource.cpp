#include "stdafx.h"
#include "SurMap5.h"
#include "SurToolSource.h"
#include "DebugUtil.h"

#include "..\AttribEditor\AttribEditorCtrl.h"

#include "SurToolAux.h"
#include "EditArchive.h"
#include "Dictionary.h"
#include "EventListeners.h"

#include "..\Environment\Environment.h"
#include "..\Environment\SourceBase.h"

struct SourceReferenceHolder{
	SourceReference source;
	void serialize(Archive& ar);
};

SourceBase* CSurToolSource::sourceOnMouse_ = 0;

IMPLEMENT_DYNAMIC(CSurToolSource, CSurToolEditable)
CSurToolSource::CSurToolSource(CWnd* pParent /*=NULL*/)
: CSurToolEditable(pParent)
, source_(0)
, refHolder_(new SourceReferenceHolder)
{
}

CSurToolSource::~CSurToolSource()
{
	if(source_)
		source_->setActivity(false);
}

BEGIN_MESSAGE_MAP(CSurToolSource, CSurToolEditable)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

// CSurToolSource message handlers

BOOL CSurToolSource::OnInitDialog()
{
    CSurToolEditable::OnInitDialog();

	if(environment){
		if(sourceOnMouse_) {
			sourceOnMouse_->kill();
			sourceOnMouse_ = 0;
		}

		setLabels(TRANSLATE("Создать источник"), "");

		if(source_){
			const char* typeNameAlt = TRANSLATE(ClassCreatorFactory<SourceBase>::instance().find(source_).nameAlt());

			setLabels(0, typeNameAlt);
		} 
		if(originalSource()) {
			sourceOnMouse_ = environment->addSource(originalSource());
			sourceOnMouse_->setPose(Se3f(QuatF::ID, cursorPosition()), true);
			sourceOnMouse_->setRadius(getBrushRadius());
			sourceOnMouse_->setActivity(sourceActive_);
			//sourceOnMouse_->setActivity(originalSource()->active());
		}

		if(source_)
			attribEditor().attachSerializeable(Serializeable(*sourceOnMouse_));
		else
			attribEditor().attachSerializeable(Serializeable(*refHolder_));
	}
	return FALSE;
}

void CSurToolSource::CallBack_BrushRadiusChanged()
{
	onPropertyChanged();
}

bool CSurToolSource::CallBack_OperationOnMap(int x, int y)
{
	if(vMap.isWorldLoaded () && sourceOnMouse() && originalSource()) {
		SourceBase* src = environment->addSource(sourceOnMouse()); //originalSource()
		src->setPose(Se3f(QuatF::ID, To3D(Vect2f(x,y))), true);
		//src->setActivity(originalSource()->active());
		eventMaster().eventObjectChanged().emit();
	}
	return true;
}

bool CSurToolSource::CallBack_DrawAuxData(void)
{
	drawCursorCircle();
	return true;
}

void CSurToolSource::serialize(Archive& ar)
{
	CSurToolBase::serialize(ar);
	ar.serialize(source_, "source", "Источник");
	ar.serialize(sourceActive_, "sourceActive", "Источник активен");
	ar.serialize(refHolder_->source, "sourceReference", "Ссылка на источник");
}

void CSurToolSource::OnDestroy()
{
	attribEditor().detachData();
	if(environment && sourceOnMouse_) {
		sourceOnMouse_->setActivity(false);
		sourceOnMouse_->kill();
		sourceOnMouse_ = 0;
	}
	
	CSurToolEditable::OnDestroy();
}

bool CSurToolSource::CallBack_TrackingMouse(const Vect3f& worldCoord, const Vect2i& screenCoord)
{
	if(environment && sourceOnMouse_) {
		sourceOnMouse_->setPose(Se3f(sourceOnMouse_->orientation(), worldCoord), true);
	}
	return true;
}

void CSurToolSource::onPropertyChanged()
{
	if(const SourceBase* source = originalSource()){
		if(!source_ && source) {
			if(sourceOnMouse_)
				sourceOnMouse_->kill();
			sourceOnMouse_ = environment->addSource(originalSource());
			sourceOnMouse_->setRadius(getBrushRadius());
		}
		if(source_ && sourceOnMouse_){
			Se3f old_pose = sourceOnMouse_->pose();
			EditOArchive oa;     oa.serialize(*sourceOnMouse_, "source", "source");
			EditIArchive ia(oa); ia.serialize(*source_, "source", "source");
			sourceActive_ = sourceOnMouse_->active();
			sourceOnMouse_->setRadius(getBrushRadius());
		}
	}
}

const SourceBase* CSurToolSource::originalSource()
{
	return &*source_ ? &*source_ : &*refHolder_->source;
}

void SourceReferenceHolder::serialize(Archive& ar)
{
	ar.serialize(source, "source", "Источник");
}

void CSurToolSource::quant()
{
	if(CSurToolSource::sourceOnMouse() && !CSurToolSource::sourceOnMouse()->isAlive()){
		CSurToolSource::setSourceOnMouse(0);
		AfxMessageBox(TRANSLATE("Источник на мышке сдох!"), MB_OK | MB_ICONEXCLAMATION);
	}
}
