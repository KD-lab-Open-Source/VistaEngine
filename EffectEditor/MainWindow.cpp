#include "StdAfx.h"
#include "MainWindow.h"

#include "Serialization/Serialization.h"
#include "kdw/Serialization.h"
#include "kdw/Application.h"
#include "kdw/Workspace.h"
#include "kdw/VBox.h"
#include "kdw/HBox.h"
#include "kdw/HLine.h"
#include "kdw/MenuBar.h"
#include "kdw/Toolbar.h"
#include "kdw/ImageStore.h"
#include "kdw/CommandManager.h"
#include "kdw/FileDialog.h"
#include "kdw/PropertyEditor.h"

#include "EffectDocument.h"
#include "Environment/Environment.h"

#include "EditWindow.h"
#include "ToolPlace.h"

// --------------------------------------------------------------------------------

class EffectWorkspace : public kdw::Workspace{
public:
	void serialize(Archive& ar);
};

void EffectWorkspace::serialize(Archive& ar)
{
	if(ar.filter(kdw::SERIALIZE_STATE))
		ar.serialize(*globalDocument, "document", "Документ");
	__super::serialize(ar);
}

// --------------------------------------------------------------------------------

kdw::Window* createMainWindow(kdw::Application* application)
{
	return new MainWindow(application);
}

#ifndef TRANSLATE
# define TRANSLATE(x) x
#endif


namespace{
	const char* globalToolNames[] = {
		typeid(kdw::ToolNavigator).name(),
		typeid(kdw::ToolSelect).name(),
		typeid(kdw::ToolTranslate).name(),
		typeid(ToolPlace).name()
	};
	int globalToolCount = sizeof(globalToolNames) / sizeof(globalToolNames[0]);
}

MainWindow::MainWindow(kdw::Application* application)
: commandManager_(new kdw::CommandManager)
{
	globalDocument = new EffectDocument();
	application->signalIdle().connect((kdw::ModelTimed*)globalDocument, &kdw::ModelTimed::onIdle);
	kdw::Model::set(globalDocument);
	globalDocument->history().signalChanged().connect(this, &Self::onHistoryChanged);

	signalClose().connect(application, &kdw::Application::quit);

	setTitle("EffectEditor - Vista Game Engine");
	setIconFromResource("MAIN");
	setDefaultPosition(kdw::POSITION_CENTER);
	setDefaultSize(Vect2i(800, 600));			
	setBorder(0);

	workspace_ = new EffectWorkspace();
	kdw::CommandManager& commands = *commandManager_;

	commands.get("main.file.new").connect(this, &Self::onMenuFileNew);
	commands.get("main.file.reset").connect(this, &Self::onMenuFileReset);
	commands.get("main.file.open").connect(this, &Self::onMenuFileOpen);
	commands.get("main.file.save").connect(this, &Self::onMenuFileSave);
	commands.get("main.file.saveAll").connect(this, &Self::onMenuFileSaveAll);
	commands.get("main.file.saveAs").connect(this, &Self::onMenuFileSaveAs);
	commands.get("main.file.exit").connect(this, &Self::onMenuFileExit);

	commands.get("main.edit.undo").enable(false).connect(this, &Self::onMenuEditUndo);
	commands.get("main.edit.redo").enable(false).connect(this, &Self::onMenuEditRedo);

	commands.get("main.terrain.load").connect(this, &Self::onMenuTerrainLoad);
	commands.get("main.terrain.environmentParams").connect(this, &Self::onMenuTerrainEnvironmentParams);

	commands.get("main.workspace.revert").connect(workspace_, &kdw::Workspace::revertState);
	commands.get("main.workspace.save").connect(workspace_, &kdw::Workspace::saveState);
	commands.get("main.workspace.editMenus").connect(this, &Self::onMenuWorkspaceEditMenus);

	commands.get("main.help.help");
	commands.get("main.help.about").connect(this, &Self::onMenuHelpAbout);

	updateToolMenu();

	kdw::VBox* box = new kdw::VBox;
	add(box);

	box->add(new kdw::HLine(), true, false, false);

	kdw::HBox* menuBox = new kdw::HBox();
	menuBox->setClipChildren(true);
	box->add(menuBox);
	{
		kdw::MenuBar* menuBar = new kdw::MenuBar(commandManager_, "main");
		menuBox->add(menuBar);
		kdw::Toolbar* toolbar = new kdw::Toolbar(commandManager_);
		kdw::ImageStore* imageStore = new kdw::ImageStore(24, 24);

		imageStore->addFromResource("FX_PAN_256", RGB(255, 0, 255));
		toolbar->setImageStore(imageStore);

		toolbar->addButton("main.file.open", 0);
		toolbar->addButton("main.file.save", 1);
		toolbar->addButton("main.file.saveAll", 2);
		toolbar->addSeparator();
		toolbar->addButton("main.edit.undo", 6);
		toolbar->addButton("main.edit.redo", 7);
		toolbar->addSeparator();
		toolbar->addButton("main.terrain.load", 8);
		toolbar->addSeparator();
		for(int i = 0; i < globalToolCount; ++i){
			const char* name = globalToolNames[i];
			std::string commandName = "main.tool.";
			commandName += name;
			toolbar->addButton(commandName.c_str(), i + 19);
		}
		menuBox->add(toolbar, true, true, true);
	}

	box->add(new kdw::HLine(), true, false, false);

	box->add(workspace_, true, true, true);
	
	workspace_->setStateFile("Scripts\\TreeControlSetups\\EffectEditor.workspace");

	kdw::CommandTemplateManager::instance().setStateFile("Scripts\\Engine\\UI\\EffectEditor.commands");
	showAll();
}

void MainWindow::updateToolMenu()
{
	kdw::CommandManager& commands = *commandManager_;
	for(int i = 0; i < globalToolCount; ++i){
		std::string commandName = "main.tool.";
		commandName += globalToolNames[i];
		kdw::Tool* currentTool = globalDocument->currentTool();
		bool checked = strcmp(typeid(*currentTool).name(), globalToolNames[i]) == 0;
		commands.get(commandName.c_str())
			.check(checked)
			.connect(this, &MainWindow::onMenuViewTool, globalToolNames[i]);
	}
}

MainWindow::~MainWindow()
{
	delete globalDocument;
    globalDocument = 0;
	delete commandManager_;
	commandManager_ = 0;
}

void MainWindow::onMenuWorkspaceEditMenus()
{
	EditWindow editWindow(Serializer(kdw::CommandTemplateManager::instance()), this);
	editWindow.showModal();
}

void MainWindow::onModelChanged()
{
}

void MainWindow::onMenuFileReset()
{
	int count = globalDocument->root()->childrenCount();
	for(int i = 0; i < count; ++i){
		NodeEffect* effect = safe_cast<NodeEffect*>(globalDocument->root()->child(i));
		effect->onMenuUnload();
	}
}

void MainWindow::onMenuFileNew()
{
	NodeEffectRoot* root = safe_cast<NodeEffectRoot*>(globalDocument->root());
	root->onMenuNewEffect();
}

void MainWindow::onMenuFileSave()
{
	if(NodeEffect* effect = globalDocument->activeEffect())
		effect->onMenuSave(this);
}

void MainWindow::onMenuFileSaveAll()
{
	int count = globalDocument->root()->childrenCount();
	for(int i = 0; i < count; ++i){
		NodeEffect* effect = safe_cast<NodeEffect*>(globalDocument->root()->child(i));
		effect->onMenuSave(this);
	}
}

void MainWindow::onMenuFileSaveAs()
{
	if(NodeEffect* effect = globalDocument->activeEffect())
		effect->onMenuSaveAs(this);
}

void MainWindow::onMenuFileOpen()
{
	NodeEffectRoot* root = safe_cast<NodeEffectRoot*>(globalDocument->root());
	root->onMenuOpenEffect(this);
}

void MainWindow::onMenuFileExit()
{
	onClose();
}

void MainWindow::onHistoryChanged()
{
	kdw::CommandManager& commands = *commandManager_;
	commands.get("main.edit.undo").enable(globalDocument->history().canBeUndone());
	commands.get("main.edit.redo").enable(globalDocument->history().canBeRedone());
}

void MainWindow::onMenuEditUndo()
{
    globalDocument->history().undo();
}

void MainWindow::onMenuEditRedo()
{
    globalDocument->history().redo();
}


void MainWindow::onMenuTerrainLoad()
{
	const char* filter[] = { "World (.spg)", "*.spg", 0 };
	kdw::FileDialog fileDialog(this, false, filter, globalDocument->worldsDirectory());
	if(fileDialog.showModal()){
		globalDocument->setWorldName(fileDialog.fileName());
	}
}

void MainWindow::onMenuTerrainEnvironmentParams()
{
	kdw::edit(Serializer(*environment, "", "", SERIALIZE_PRESET_DATA), "Scripts\\TreeControlSetups\\editEnvironment", kdw::IMMEDIATE_UPDATE | kdw::ONLY_TRANSLATED, this);
}

void MainWindow::onMenuHelpAbout()
{

}

void MainWindow::onMenuViewTool(const char* name)
{
	kdw::CommandManager& commands = *commandManager_;
	kdw::Tool* tool = kdw::ToolManager::instance().get(name);
	if(tool){
		if(commands.toggling() && tool == globalDocument->currentTool())
			commands.delayToggle();
		else
			globalDocument->toggleTool(tool, commands.stickyLocked());
	}
	else
		xassert(0);

	updateToolMenu();
}
