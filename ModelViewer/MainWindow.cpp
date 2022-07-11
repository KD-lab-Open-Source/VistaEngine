#include "StdAfx.h"
#include "MainWindow.h"

#include "XMath/XMathLib.h"

#include "Win32/Window.h"

#include "kdw/Application.h"

#include "kdw/HSplitter.h"
#include "kdw/VSplitter.h"

#include "kdw/ScrolledWindow.h"

#include "kdw/VBox.h"
#include "kdw/HBox.h"

#include "kdw/ComboBox.h"
#include "kdw/Button.h"
#include "kdw/Label.h"
#include "kdw/Serialization.h"
#include "kdw/MenuBar.h"
#include "kdw/CommandManager.h"
#include "kdw/Workspace.h"

#include "Serialization/EditArchive.h"

#include "kdw/PropertyTree.h"


#include "FileTree.h"
#include "MainView.h"
#include "EditWindow.h"


// --------------------------------------------------------------------------------


kdw::Window* createMainWindow(kdw::Application* application)
{
	return new MainWindow(application);
}

#ifndef TRANSLATE
# define TRANSLATE(x) x
#endif


MainWindow::MainWindow(kdw::Application* application)
: commandManager_(new kdw::CommandManager)
{
	globalDocument = new MainDocument();
	application->registerIdleListener(globalDocument);
	kdw::Navigator::setDocument(globalDocument);

	signalClose().connect(application, &kdw::Application::quit);

	setTitle("ModelViewer - Vista Game Engine");
	setDefaultPosition(kdw::POSITION_CENTER);
	setDefaultSize(Vect2i(800, 600));			
	setBorder(0);

	workspace_ = new kdw::Workspace();
	kdw::CommandManager& commands = *commandManager_;
	commands.add("main.file.open").connect(this, &Self::onMenuFileOpen);
	commands.add("main.file.exit").connect(this, &Self::onMenuFileExit);
	
	commands.add("main.workspace.revert").connect(workspace_, &kdw::Workspace::revertState);
	commands.add("main.workspace.save").connect(workspace_, &kdw::Workspace::saveState);
	//commands.add("main.workspace.reset");
	//commands.add("main.workspace.lock");
	commands.add("main.workspace.openFloating");
	commands.add("main.workspace.editMenus").connect(this, &Self::onMenuWorkspaceEditMenus);

	commands.add("main.help.help");
	commands.add("main.help.about").connect(this, &Self::onMenuHelpAbout);



	kdw::VBox* box = new kdw::VBox;
	add(box);

	kdw::MenuBar* menuBar = new kdw::MenuBar(commandManager_, "main");
	box->pack(menuBar);

	box->pack(workspace_, false, true, true);
	workspace_->setStateFile("Scripts\\TreeControlSetups\\ModelViewer.workspace");
	//globalDocument->signalModelChanged().connect(this, &MainWindow::onModelChanged);
	showAll();
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

void MainWindow::onMenuFileOpen()
{

}

void MainWindow::onMenuViewOption()
{
	commandManager_->add("main.view.bump_mapping").enable(false);
}

void MainWindow::onMenuFileExit()
{
	onClose();
}

void MainWindow::onMenuHelpAbout()
{

}
