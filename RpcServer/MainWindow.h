#pragma once
#ifndef __VISTARPC_MAIN_WINDOW_H_INCLUDED__
#define __VISTARPC_MAIN_WINDOW_H_INCLUDED__

#include "kdw/Window.h"

class ShowLog;

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

	ShowLog* view() const { return view_; }

protected:
	void onMenuFileExit();

	void onMenuHelpAbout();
	void onMenuLogClear();

	kdw::CommandManager* commandManager_;
	ShowLog* view_;
};

kdw::Window* createMainWindow(kdw::Application* application);

#endif //__VISTARPC_MAIN_WINDOW_H_INCLUDED__
