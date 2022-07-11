#include "StdAfx.h"
#include "MainWindow.h"

#include "ShowLog.h"

#include "Serialization\Serialization.h"
#include "Serialization\XPrmArchive.h"

#include "kdw\Application.h"
#include "kdw\CommandManager.h"

#include "kdw\VBox.h"
#include "kdw\HBox.h"
#include "kdw\hsplitter.h"
#include "kdw\HLine.h"
#include "kdw\MenuBar.h"
#include "kdw\ImageStore.h"
#include "kdw\Toolbar.h"

#include "resource.h"

kdw::Window* createMainWindow(kdw::Application* application)
{
	return new MainWindow(application);
}

MainWindow::MainWindow(kdw::Application* application)
: commandManager_(new kdw::CommandManager)
{
	signalClose().connect(application, &kdw::Application::quit);

	setTitle("Vista RPC Server");
	setIconFromResource("MAIN");
	setDefaultPosition(kdw::POSITION_CENTER);
	setDefaultSize(Vect2i(800, 600));			
	setBorder(0);

	kdw::CommandManager& commands = *commandManager_;

	commands.get("main.file.exit").connect(this, &Self::onMenuFileExit);
	commands.get("main.log.clear").connect(this, &Self::onMenuLogClear);

	commands.get("main.help.about").connect(this, &Self::onMenuHelpAbout);

	kdw::VBox* vbox = new kdw::VBox;
	add(vbox);

	vbox->add(new kdw::HLine(), true, false, false);

	kdw::HBox* menuBox = new kdw::HBox();
	menuBox->setClipChildren(true);
	vbox->add(menuBox);
	{
		kdw::HSplitter* menuSplitter = new kdw::HSplitter(0, 0);
		menuBox->add(menuSplitter, true, true, true);

		kdw::MenuBar* menuBar = new kdw::MenuBar(commandManager_, "main");
		menuSplitter->add(menuBar, .15f);
		kdw::Toolbar* toolbar = new kdw::Toolbar(commandManager_);
		kdw::ImageStore* imageStore = new kdw::ImageStore(24, 24);

		imageStore->addFromResource("TOOLBAR", RGB(255, 0, 255));
		toolbar->setImageStore(imageStore);

		toolbar->addButton("main.log.clear", 0);
		toolbar->addSeparator();
		menuSplitter->add(toolbar);
	}
	vbox->add(new kdw::HLine(), true, false, false);

    vbox->add(view_ = new ShowLog, true, true, true);

	showAll();
}

MainWindow::~MainWindow()
{
	delete commandManager_;
	commandManager_ = 0;
}

void MainWindow::onMenuFileExit()
{
	onClose();
}

void MainWindow::onMenuHelpAbout()
{
	view()->addRecord("Vista RPC Server");
}

void MainWindow::onMenuLogClear()
{
	view()->clearLog();
}