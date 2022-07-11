#include "StdAfx.h"

#include "kdw\Win32/Types.h"

#include "kdw/Window.h"
#include "kdw/Application.h"

#include "XMath/XMathLib.h"

kdw::Window* createMainWindow(kdw::Application* application);

int __stdcall WinMain(HINSTANCE instance, HINSTANCE previousInstance, char* commandLine, int showCommand)
{
	kdw::Application application(instance);

	ShareHandle<kdw::Window> mainWindow = createMainWindow(&application);

	return application.run();
}


bool isUnderEditor()
{
	return true;
}
