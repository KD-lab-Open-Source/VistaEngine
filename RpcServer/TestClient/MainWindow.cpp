#include "StdAfx.h"
#include "MainWindow.h"

#include "XMath\XMath.h"

#include "ShowLog.h"
#include "UserInterface\XmlRpc\RpcTypes.h"
#include "Client.h"
#include "GameTest.h"

#include "XmlRpc\XmlRpcUtil.h"

#include "Serialization\Serialization.h"
#include "Serialization\Serializer.h"
#include "Serialization\XPrmArchive.h"

#include "kdw\Application.h"
#include "kdw\CommandManager.h"

#include "kdw\VBox.h"
#include "kdw\HBox.h"
#include "kdw\hsplitter.h"
#include "kdw\HLine.h"
#include "kdw\Label.h"
#include "kdw\MenuBar.h"
#include "kdw\ImageStore.h"
#include "kdw\Toolbar.h"
#include "kdw\PropertyTree.h"

#include "resource.h"


kdw::Window* createMainWindow(kdw::Application* application)
{
	return new MainWindow(application);
}

MainWindow::MainWindow(kdw::Application* application)
: commandManager_(new kdw::CommandManager)
{
	signalClose().connect(application, &kdw::Application::quit);

	setTitle("Vista RPC Client");
	setIconFromResource("MAIN");
	setDefaultPosition(kdw::POSITION_CENTER);
	setDefaultSize(Vect2i(800, 600));			
	setBorder(0);

	kdw::CommandManager& commands = *commandManager_;

	commands.get("main.file.exit").connect(this, &Self::onMenuFileExit);

	commands.get("main.test.sum").connect(this, &Self::onMenuTestSum);

	commands.get("main.help.help").connect(this, &Self::onMenuHelpAbout);;
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

		toolbar->addButton("main.test.sum", 0);
		toolbar->addSeparator();
		menuSplitter->add(toolbar);
	}
	vbox->add(new kdw::HLine(), true, false, false);

	kdw::HSplitter* splitter_ = new kdw::HSplitter(0, 0);
	vbox->add(splitter_, true, true, true);

	propertyTree_ = new kdw::PropertyTree;
	propertyTree_->setSelectFocused(true);
	propertyTree_->setHideUntranslated(true);
	propertyTree_->setCompact(true);
	splitter_->add(propertyTree_, 0.4f);

	splitter_->add(view_ = new ClientLog, true, true, true);

	vbox->add(new kdw::Label("Работаю", false, 2), true, false, false);

	propertyTree_->attach(Serializer(*this));
	showAll();
}

MainWindow::~MainWindow()
{
	delete commandManager_;
	commandManager_ = 0;
}

void MainWindow::serialize(Archive& ar)
{
	client->game()->serialize(ar);
}

void MainWindow::onMenuFileExit()
{
	onClose();
}

void MainWindow::onMenuHelpAbout()
{
	view_->addRecord("Vista RPC Client");
}

void MainWindow::addLogRecord(const char* text)
{
	view_->addRecord(text);
}

struct SumHandler
{
	SumHandler(int r) : ret(r) {}
	int ret;

	void handler(int status, const int* tst)
	{
		if(tst && *tst == ret)
			XmlRpc::XmlRpcUtil::log(0, "OK");
		else
			XmlRpc::XmlRpcUtil::log(0, "ERROR!!!");
			
		delete this;
	}
};

void MainWindow::onMenuTestSum()
{
	for(int calls = 0; calls < 10; ++calls){
		RpcType::SumType param;
		int count = 1 + random(9);
		for(int i = 0; i < count; ++i)
			param.data_.push_back(random(9999));
		
		
		typedef RpcMethodAsynchCall<SumHandler, RpcType::SumType, int> MethodSum;
		MethodSum call("SUM", new SumHandler(param.sum()), &SumHandler::handler, param);
		client->rpcAsynchCall(&call);
	}

	Sleep(20);

	RpcType::SumType param;
	int count = 1 + random(9);
	for(int i = 0; i < count; ++i)
		param.data_.push_back(random(9999));

	int sum;
	int status;
	bool ret = client->doRemoteCall("SUM", param, status, sum);

	if(!ret || param.sum() != sum)
		XmlRpc::XmlRpcUtil::log(0, "ERROR!!!");
	else
		XmlRpc::XmlRpcUtil::log(0, (XBuffer() <= count < " ints, sum " <= ret < " OK"));
}
