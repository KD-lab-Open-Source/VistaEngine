#pragma once

#include "kdw\VBox.h"
#include "kdw\ObjectsTree.h"
#include "TriggerEditor\TriggerExport.h"

class TriggerView;

namespace kdw {
	class ObjectsTree;
	class Slider;
	class VBox;
};

class TriggerDebugger : public kdw::VBox
{
public:
	TriggerDebugger(TriggerChain& triggerChain, TriggerView* triggerView);
	
	void onSelectionChanged();
	void onSliderChanged();

private:
	TriggerChain& triggerChain_;
	TriggerView* triggerView_;
	kdw::ObjectsTree* tree_;
	kdw::Slider* slider_;

	kdw::TreeObject* root() { return tree_->root(); } 
};

