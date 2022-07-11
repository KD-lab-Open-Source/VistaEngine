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
#include "kdw/PropertyTree.h"
#include "kdw/PropertyTreeModel.h"
#include "kdw/PropertyRow.h"
#include "kdw/Viewport.h"
#include "kdw/Label.h"
#include "Serialization\XPrmArchive.h"
#include "Render\src\TexLibrary.h"

#include "XMath/ComboListColor.h"
#include "View3dx.h"
#include "resource.h"

kdw::Window* createMainWindow(kdw::Application* application)
{
	return new MainWindow(application);
}

MainWindow::MainWindow(kdw::Application* application)
: commandManager_(new kdw::CommandManager)
{
	signalClose().connect(application, &kdw::Application::quit);

	setTitle("View 3dx");
	setIconFromResource("MAIN");
	setDefaultPosition(kdw::POSITION_CENTER);
	setDefaultSize(Vect2i(1024, 768));			
	setBorder(0);

	kdw::CommandManager& commands = *commandManager_;

	commands.get("main.file.open").connect(this, &Self::onMenuFileOpen);
	commands.get("main.file.exit").connect(this, &Self::onMenuFileExit);

	commands.get("main.help.help");
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

		toolbar->addButton("main.file.open", 0);
		toolbar->addSeparator();
		menuSplitter->add(toolbar);
	}
	vbox->add(new kdw::HLine(), true, false, false);

	kdw::HSplitter* splitter = new kdw::HSplitter(0, 0);
	vbox->add(splitter, true, true, true);

	propertyTree_ = new kdw::PropertyTree;
	propertyTree_->setSelectFocused(true);
	propertyTree_->setHideUntranslated(true);
	propertyTree_->setCompact(true);
	splitter->add(propertyTree_, 0.25);

	view_ = new View3dx;
    splitter->add(view_);

	propertyTree_->attach(Serializer(*view_));
	propertyTree_->revert();

	vbox->add(new kdw::HLine(), true, false, false);
	vbox->add(view_->statusLine(), true, false, false);

	kdw::CommandTemplateManager::instance().setStateFile("Scripts\\Engine\\UI\\EffectEditor.commands");
	showAll();

	XPrmIArchive ar;
	if(ar.open("View3dx.cfg"))
		serialize(ar);

	if(__argc > 1) 
		loadObject(__argv[1]);
}

MainWindow::~MainWindow()
{
	delete commandManager_;
	commandManager_ = 0;
}

void MainWindow::serialize(Archive& ar)
{
	ar.setFilter(SERIALIZE_STATE);
	
	__super::serialize(ar);
	
	ar.serialize(*view_, "view", 0);
	ar.serialize(*propertyTree_, "propertyTree", 0);
}

void MainWindow::onClose()
{
	__super::onClose();

	serialize(XPrmOArchive("View3dx.cfg"));
}

void MainWindow::onSetFocus()
{
	__super::onSetFocus();

	GetTexLibrary()->ReloadAllTexture();
}

void MainWindow::onMenuFileOpen()
{
	const char* masks[] = {
		"3dx (.3dx)", "*.3dx",
			0
	};

	kdw::FileDialog dialog(0, false, masks, "");
	if(dialog.showModal())
		loadObject(dialog.fileName());
}

void MainWindow::loadObject(const char* fname)
{
	view_->loadObject(fname);
	propertyTree_->revert();
	setTitle((string("View 3dx - ") + fname).c_str());
}

void MainWindow::onMenuFileExit()
{
	onClose();
}

void MainWindow::onMenuHelpAbout()
{
}

void ComboListColor::serialize (Archive& ar)
{
	ar.serialize(index_, "index", 0);
}

