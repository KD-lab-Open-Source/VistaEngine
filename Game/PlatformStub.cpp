// Cross-platform entry point.
//
// The game's real entry is WinMain (Game/Runtime.cpp), and every platform reaches it
// through main() — including Windows, where add_executable() carries no WIN32 flag and
// the linker therefore looks for main() too. A real SDL3 entry/bootstrap is a later
// step. (There is no CD-key check in this build; see Game/CMakeLists.txt.)
#include "StdAfx.h"

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw);

#ifdef __linux__
// XUtil (Console.cpp, XUtilCore.cpp) reads the MSVC globals __argc / __argv, which
// WindowsAPI.h declares here but nothing defined: macOS has Apple's _NSGetArgv() to
// map them onto, and glibc offers no equivalent. main() has them, so capture them.
char** __argv = nullptr;
int    __argc = 0;
#endif

int main(int argc, char** argv)
{
#ifdef __linux__
	__argc = argc;
	__argv = argv;
#else
	(void)argc; (void)argv;
#endif
	return WinMain(0, 0, (LPSTR)"", 0);
}
