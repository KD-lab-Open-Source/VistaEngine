// Cross-platform entry point.
//
// The game's real entry is WinMain (Game/Runtime.cpp), and every platform reaches it
// through main() — including Windows, where add_executable() carries no WIN32 flag and
// the linker therefore looks for main() too. A real SDL3 entry/bootstrap is a later
// step. (There is no CD-key check in this build; see Game/CMakeLists.txt.)
#include "StdAfx.h"

#include <cstdio>
#include <filesystem>
#include <string_view>
#include <SDL3/SDL_filesystem.h>

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw);

#ifdef __linux__
// XUtil (Console.cpp, XUtilCore.cpp) reads the MSVC globals __argc / __argv, which
// WindowsAPI.h declares here but nothing defined: macOS has Apple's _NSGetArgv() to
// map them onto, and glibc offers no equivalent. main() has them, so capture them.
char** __argv = nullptr;
int    __argc = 0;
#endif

static bool holdsGameData(const std::filesystem::path& dir)
{
	std::error_code ec;
	return std::filesystem::is_directory(dir / "Resource", ec)
		&& std::filesystem::is_directory(dir / "Scripts", ec);
}

// Every data path in the engine is relative -- "Resource\...", "Scripts\..." -- so the
// game has to run with the data directory as its working directory. The original's
// shortcut set that ("Start in"); nothing does for a game Steam starts on Linux, or an
// app bundle double-clicked in the Finder. So look for the data:
//
//   1. in the working directory, if it is already there: a developer's
//      `cd GameData && ../build/Game/Game`, or a launcher that sets it;
//   2. in SDL_GetBasePath(): the executable's directory -- or, for a macOS app bundle
//      whose Info.plist sets SDL_FILESYSTEM_BASE_DIR_TYPE to "parent", the directory
//      the .app sits in, which is where a release installs the data shared by all
//      three platforms.
//
// The path goes through std::filesystem rather than _chdir: SDL hands it over in UTF-8,
// and on Windows only the wide API takes that, for an install path the ANSI code page
// cannot spell. The engine's own opens are relative to it and do not care.
static void enterDataDirectory()
{
	std::error_code ec;
	if(holdsGameData(std::filesystem::current_path(ec)))
		return;

	if(const char* base = SDL_GetBasePath()){
		std::filesystem::path dir(std::u8string_view(reinterpret_cast<const char8_t*>(base)));
		if(holdsGameData(dir)){
			std::filesystem::current_path(dir, ec);
			if(!ec){
				fprintf(stderr, "Data directory: %s\n", base);
				return;
			}
		}
	}

	// Nothing found: stay put. The first file the game cannot open will say what is missing.
	fprintf(stderr, "No game data (Resource/, Scripts/) in the working directory or at %s\n",
		SDL_GetBasePath() ? SDL_GetBasePath() : "(no base path)");
}

int main(int argc, char** argv)
{
#ifdef __linux__
	__argc = argc;
	__argv = argv;
#else
	(void)argc; (void)argv;
#endif
	enterDataDirectory();
	return WinMain(0, 0, (LPSTR)"", 0);
}
