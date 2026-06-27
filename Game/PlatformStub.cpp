// Cross-platform entry point + licensing stub.
//
// The game's real entry is WinMain (Game/Runtime.cpp); on macOS/Linux the C
// runtime calls main(), so we forward to WinMain. VerifyCDKey is an external
// licensing DLL on Windows (__declspec(dllimport)); off-Windows there is no key
// check, so it always succeeds. A real SDL3 entry/bootstrap is a later step.
#include "StdAfx.h"

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw);

int main(int /*argc*/, char** /*argv*/)
{
	return WinMain(0, 0, (LPSTR)"", 0);
}

bool VerifyCDKey(const char* /*String*/)
{
	return true;
}
