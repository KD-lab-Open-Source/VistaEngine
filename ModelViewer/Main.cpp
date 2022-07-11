#include "StdAfx.h"

#include "Win32/Types.h"

#include "kdw/Window.h"
#include "kdw/Application.h"

kdw::Window* createMainWindow(kdw::Application* application);

int __stdcall WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand)
{
	kdw::Application application(instance);

	ShareHandle<kdw::Window> mainWindow = createMainWindow(&application);

	//EditArchive ea;
	///ea.edit(*mainWindow);
	return application.run();
}


