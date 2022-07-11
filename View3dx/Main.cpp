#include "StdAfx.h"

#include "kdw/Win32/Types.h"

#include "kdw/Window.h"
#include "kdw/Application.h"

#include "XMath/XMathLib.h"

#include "MainWindow.h"

int __stdcall WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand)
{
	kdw::Application application(instance);

	ShareHandle<kdw::Window> mainWindow = createMainWindow(&application);

	return application.run();
}


bool isUnderEditor()
{
	return false;
}
