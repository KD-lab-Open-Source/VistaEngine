#include "stdafx.h"
#include <Windows.h>
#include "TriggerDebugger.h"
#include "TriggerView.h"
#include "kdw\Slider.h"
#include "kdw\HBox.h"
#include "kdw\Button.h"

TriggerDebugger::TriggerDebugger(TriggerChain& triggerChain, TriggerView* triggerView)
: triggerChain_(triggerChain)
, triggerView_(triggerView)
{
	kdw::HBox* hBox = new kdw::HBox;
	add(hBox, false, false, false);

	hBox->add(slider_ = new kdw::Slider, false, true, true);
	kdw::Button* button = new kdw::Button("Легенда");
	hBox->add(button, false, false, false);
	button->signalPressed().connect(triggerView, &TriggerView::showLegend);
	
	add(tree_ = new kdw::ObjectsTree, false, true, true);
	TriggerChain::TriggerEventList::const_iterator i;
	FOR_EACH(triggerChain_.logData(), i)
		root()->add(new kdw::TreeObject(i->event.c_str()));
	tree_->update();

	tree_->signalSelectionChanged().connect(this, &TriggerDebugger::onSelectionChanged);
	slider_->signalChanged().connect(this, &TriggerDebugger::onSliderChanged);
	slider_->setValue(1.f);
	onSliderChanged();
}

void TriggerDebugger::onSelectionChanged()
{
	int index = root()->find(tree_->focusedObject()) - root()->begin();
	slider_->setValue(root()->size() > 1 ? float(index)/(root()->size() - 1) : 1);
	triggerChain_.setLogRecord(index);
	triggerView_->redraw();
}

void TriggerDebugger::onSliderChanged()
{
	tree_->model()->deselectAll();
	if(root()->size()){
		int index = round(slider_->value()*(root()->size() - 1));
		tree_->selectObject(safe_cast<kdw::TreeObject*>(&*root()->begin()[index]));
		triggerChain_.setLogRecord(index);
		triggerView_->redraw();
	}
}

