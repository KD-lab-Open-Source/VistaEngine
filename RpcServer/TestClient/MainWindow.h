#pragma once
#ifndef __VISTARPC_MAIN_WINDOW_H_INCLUDED__
#define __VISTARPC_MAIN_WINDOW_H_INCLUDED__

#include "kdw/Window.h"

class ClientLog;

namespace kdw{
	class Application;
	class CommandManager;
	class PropertyTree;
}

class MainWindow : public kdw::Window{
public:
	typedef MainWindow Self;

	MainWindow(kdw::Application* application);
	~MainWindow();

	void serialize(Archive& ar);

	void addLogRecord(const char* text);

protected:
	void onMenuFileExit();

	void onMenuHelpAbout();
	void onMenuTestSum();

	kdw::CommandManager* commandManager_;

	kdw::PropertyTree* propertyTree_;
	ClientLog* view_;
};

kdw::Window* createMainWindow(kdw::Application* application);

#endif //__VISTARPC_MAIN_WINDOW_H_INCLUDED__
