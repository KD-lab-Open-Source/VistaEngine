#include "StdAfx.h"
#include "NoiseParamsDialog.h"
#include "kdw/HBox.h"
#include "kdw/PropertyTree.h"
#include "NoisePreview.h"

NoiseParamsDialog::NoiseParamsDialog(kdw::Window* parent)
: kdw::Dialog(parent)
{
	setTitle(TRANSLATE("Параметры шума"));
	setResizeable(true);
	setDefaultSize(Vect2i(640, 400));
	kdw::HBox* hbox = new kdw::HBox(5);
	add(hbox);
	{
		propertyTree_ = new kdw::PropertyTree();
		hbox->add(propertyTree_, false, true, true);
		propertyTree_->setCompact(true);
		propertyTree_->signalChanged().connect(this, &NoiseParamsDialog::onChange);
		propertyTree_->attach(Serializer(noiseParams_));

		noisePreview_ = new NoisePreview();
		noisePreview_->setRequestSize(Vect2i(200, 100));
		hbox->add(noisePreview_, false, true, true);
	}

	addButton(TRANSLATE("ОК"), kdw::RESPONSE_OK);
	addButton(TRANSLATE("Отмена"), kdw::RESPONSE_CANCEL);
}


void NoiseParamsDialog::set(const NoiseParams& noiseParams)
{
	noiseParams_ = noiseParams;
	propertyTree_->revert();
	noisePreview_->set(noiseParams);
}

void NoiseParamsDialog::onChange()
{
	noisePreview_->set(noiseParams_);
}
