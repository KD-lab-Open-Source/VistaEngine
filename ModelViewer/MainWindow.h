#ifndef __VIEW3DX_MAIN_WINDOW_H_INCLUDED__
#define __VIEW3DX_MAIN_WINDOW_H_INCLUDED__

#include "kdw/Window.h"

namespace kdw{
	class Application;
	class ComboBox;
	class Label;
	class Box;
	class Tree;
	class Workspace;
	class CommandManager;
}

class AnimationGroupComboBox;
class MainView;


class MainWindow : public kdw::Window{
public:
	typedef MainWindow Self;

	MainWindow(kdw::Application* application);
	~MainWindow();
	
protected:
	void onModelChanged();

	void onMenuFileOpen();
	void onMenuFileExit();
	void onMenuWorkspaceEditMenus();
	void onMenuViewOption();
	void onMenuHelpAbout();

	kdw::Workspace* workspace_;
	kdw::CommandManager* commandManager_;
};

kdw::Window* createMainWindow(kdw::Application* application);

#endif
