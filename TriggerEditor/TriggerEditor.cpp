#include "stdafx.h"
#include "TriggerEditor.h"
#include "TriggerView.h"
#include "TriggerMiniMap.h"
#include "TriggerDebugger.h"
#include "ClassTree.h"
#include "kdw\CommandManager.h"
#include "kdw\VBox.h"
#include "kdw\HBox.h"
#include "kdw\HLine.h"
#include "kdw\MenuBar.h"
#include "kdw\ToolBar.h"
#include "kdw\HSplitter.h"
#include "kdw\VSplitter.h"
#include "kdw\ImageStore.h"
#include "kdw\PropertyTree.h"
#include "kdw\FileDialog.h"
#include "kdw\Label.h"
#include "kdw\Win32\Rectangle.h"
#include "kdw\Win32\Window.h"
#include "Serialization\SerializationFactory.h"

FORCE_SEGMENT(ConditionEditor)

TriggerEditor::TriggerEditor(TriggerChain& triggerChain, HWND hwnd, bool toCopy)
: triggerChain_(triggerChain)
, Dialog(hwnd)
, commandManager_(new kdw::CommandManager)
, debugger_(0)
{
	setTitle((string("TriggerEditor  -  ") + triggerChain_.name + (toCopy ? " (открыт для копирования)" : "")).c_str());

	setDefaultPosition(kdw::POSITION_CENTER);
	setDefaultSize(Vect2i(800, 600));
	setResizeable(true);
	setMaximized(true);

	commandManager_->get("main.Menu.Debug (Ctrl+D)").connect(this, &TriggerEditor::onFileDebug);
	commandManager_->get("main.Menu.Undo (Ctrl+Z)").connect(this, &TriggerEditor::onFileDebug);
	commandManager_->get("main.Menu.Redo (Ctrl+Shift+Z, Ctrl+Y)").connect(this, &TriggerEditor::onFileDebug);
	commandManager_->get("main.Menu.Поиск (Ctrl+F)").connect(this, &TriggerEditor::onFileDebug);
	commandManager_->get("main.Menu.Открыть для копирования").connect(this, &TriggerEditor::onOpenToCopy);

	kdw::HBox* menuBox = new kdw::HBox;
	menuBox->setClipChildren(true);
	add(menuBox, false, false, false);
	{
		kdw::MenuBar* menuBar = new kdw::MenuBar(commandManager_, "main");
		menuBox->add(menuBar);
		kdw::Toolbar* toolbar = new kdw::Toolbar(commandManager_);
		kdw::ImageStore* imageStore = new kdw::ImageStore(24, 24);

		imageStore->addFromResource("FX_PAN_256", RGB(255, 0, 255));
		toolbar->setImageStore(imageStore);

		toolbar->addButton("main.Menu.Debug (Ctrl+D)", 8);
		toolbar->addSeparator();
		toolbar->addButton("main.Menu.Undo (Ctrl+Z)", 6);
		toolbar->addButton("main.Menu.Redo (Ctrl+Shift+Z, Ctrl+Y)", 7);
		toolbar->addButton("main.Menu.Поиск (Ctrl+F)", 10);
		toolbar->addButton("main.Menu.Открыть для копирования", 0);
		toolbar->addSeparator();
		menuBox->add(toolbar, true, true, true);
	}

	add(new kdw::HLine(), false, false, false);

	add(vSplitter1_ = new kdw::VSplitter, false, true, true);

	vSplitter1_->add(hSplitter_ = new kdw::HSplitter, .85f);
	
	hSplitter_->add(vSplitter2_ = new kdw::VSplitter, 0.2f);

	typedef FactorySelector<Action>::Factory Factory;
	Factory& factory = Factory::instance();
	vSplitter2_->add(actionsTree_ = new ClassTree(factory.comboStrings(), factory.comboStringsAlt()), 0.33f);

	vSplitter2_->add(miniMap_ = new TriggerMiniMap(triggerChain), .66f);

	kdw::PropertyTree* propertyTree = new kdw::PropertyTree;
	propertyTree->setSelectFocused(true);
	propertyTree->setHideUntranslated(true);
	propertyTree->setCompact(true);
	propertyTree->setImmediateUpdate(true);
	vSplitter2_->add(propertyTree);
	
	hSplitter_->add(triggerView_ = new TriggerView(triggerChain, miniMap_, propertyTree), 0.7f);

	commandManager_->get("main.Menu.Undo (Ctrl+Z)").connect(triggerView_, &TriggerView::undo);
	commandManager_->get("main.Menu.Redo (Ctrl+Shift+Z, Ctrl+Y)").connect(triggerView_, &TriggerView::redo);
	commandManager_->get("main.Menu.Поиск (Ctrl+F)").connect(triggerView_, &TriggerView::find);
	
	hotkeyContext()->signalPressed(sKey('D' | sKey::CONTROL)).connect(this, &TriggerEditor::onFileDebug);
	hotkeyContext()->signalPressed(sKey('Z' | sKey::CONTROL)).connect(triggerView_, &TriggerView::undo);
	hotkeyContext()->signalPressed(sKey('Z' | sKey::CONTROL | sKey::SHIFT)).connect(triggerView_, &TriggerView::redo);
	hotkeyContext()->signalPressed(sKey('Y' | sKey::CONTROL)).connect(triggerView_, &TriggerView::redo);
	hotkeyContext()->signalPressed(sKey('F' | sKey::CONTROL)).connect(triggerView_, &TriggerView::find);

	add(new kdw::HLine(), true, false, false);

	if(toCopy)
		addButton(TRANSLATE("Закрыть"), kdw::RESPONSE_OK);
	else{
		addButton(TRANSLATE("ОК"),     kdw::RESPONSE_OK);
		addButton(TRANSLATE("Отмена"), kdw::RESPONSE_CANCEL);
	}

	if(!triggerChain_.logData().empty())
		onFileDebug();
}

void TriggerEditor::onFileDebug()
{
	if(debugger_){
		vSplitter1_->remove(1, false);
		debugger_ = 0;
		triggerView_->setDebug(false);
		miniMap_->setDebug(false);
	}
	else{
		vSplitter1_->add(debugger_ = new TriggerDebugger(triggerChain_, triggerView_), 0.15f);
		triggerView_->setDebug(true);
		miniMap_->setDebug(true);
	}
	showAll();
}

bool TriggerEditor::edit()
{
	if(showModal() != kdw::RESPONSE_CANCEL)
		return true;
	if(!triggerView_->changed())
		return false;
	kdw::Dialog dialog(this);
	dialog.add(new kdw::Label("Записать изменения?"));
	dialog.addButton(TRANSLATE("Да"), kdw::RESPONSE_OK);
	dialog.addButton(TRANSLATE("Нет"), kdw::RESPONSE_CANCEL);
	return dialog.showModal() != kdw::RESPONSE_CANCEL;
}

void TriggerEditor::onOpenToCopy()
{
	const char* masks[] = {
		"scr (.scr)", "*.scr",
			0
	};

	kdw::FileDialog dialog(this, false, masks, "Scripts\\Content\\Triggers");
	if(!dialog.showModal())
		return;

	TriggerChain triggerChain;
	triggerChain.load(dialog.fileName());
	TriggerEditor(triggerChain, _window()->get(), true).edit();
}