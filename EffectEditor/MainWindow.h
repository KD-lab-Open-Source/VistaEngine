#ifndef __VIEW3DX_MAIN_WINDOW_H_INCLUDED__
#define __VIEW3DX_MAIN_WINDOW_H_INCLUDED__

#include "kdw/Window.h"

namespace kdw{
	class Application;
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
	void onHistoryChanged();

	void updateToolMenu();
	void onMenuViewTool(const char* name);

	void onMenuFileNew();
	void onMenuFileReset();
	void onMenuFileOpen();
	void onMenuFileSave();
	void onMenuFileSaveAll();
	void onMenuFileSaveAs();
	void onMenuFileExit();

	void onMenuEditUndo();
	void onMenuEditRedo();

	void onMenuTerrainLoad();
	void onMenuTerrainEnvironmentParams();

	void onMenuWorkspaceEditMenus();

	void onMenuHelpAbout();

	kdw::Workspace* workspace_;
	kdw::CommandManager* commandManager_;
};

kdw::Window* createMainWindow(kdw::Application* application);

#endif
