#ifndef __VIEW3DX_MAIN_WINDOW_H_INCLUDED__
#define __VIEW3DX_MAIN_WINDOW_H_INCLUDED__

#include "kdw/Window.h"

class View3dx;

namespace kdw{
	class Application;
	class Workspace;
	class CommandManager;
	class PropertyTree;
}

class MainWindow : public kdw::Window{
public:
	typedef MainWindow Self;

	MainWindow(kdw::Application* application);
	~MainWindow();

	void loadObject(const char* fname);

protected:
	void onMenuFileOpen();
	void onMenuFileExit();

	void serialize(Archive& ar);
	void onClose();
	void onSetFocus();
	void onMenuHelpAbout();

	kdw::CommandManager* commandManager_;
	kdw::PropertyTree* propertyTree_;
	View3dx* view_;
};

kdw::Window* createMainWindow(kdw::Application* application);

#endif
