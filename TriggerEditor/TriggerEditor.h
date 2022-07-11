#pragma once

#include "kdw/Dialog.h"

class TriggerChain;
class TriggerView;
class TriggerMiniMap;
class TriggerDebugger;
class ClassTree;

namespace kdw {
	class CommandManager;
	class HSplitter;
	class VSplitter;
	class PropertyTree;
};

class TriggerEditor : public kdw::Dialog {
public:
	TriggerEditor(TriggerChain& triggerChain, HWND hwnd, bool toCopy = false);

	bool edit();

	void onFileDebug();
	void onOpenToCopy();

protected:
	TriggerChain& triggerChain_;
	kdw::CommandManager* commandManager_;
	kdw::VSplitter* vSplitter1_;
	kdw::HSplitter* hSplitter_;
	kdw::VSplitter* vSplitter2_;
	TriggerView* triggerView_;
	TriggerMiniMap* miniMap_;
	TriggerDebugger* debugger_;;
	ClassTree* actionsTree_;
};

