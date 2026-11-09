#ifndef VISTA_PLATFORM_SERVICES_H
#define VISTA_PLATFORM_SERVICES_H

// The services of the store the game is sold through -- Steam's, for one -- as the engine
// sees them: a few calls at fixed points in the program's life.
//
// The engine has no store of its own. Platform/ServicesNone.cpp implements these as
// nothing at all, and every build of the engine uses it unless the project around the
// engine sets VISTA_PLATFORM_SERVICES_EXTERNAL and adds its own implementation to the Game
// target (Game/CMakeLists.txt). That keeps a store's SDK, and the code that calls it, with
// the game that is sold there.

namespace PlatformServices {

// First thing in WinMain; the data directory is already the working directory. False means
// the program must exit straight away: the store is starting it again, the way it wants it
// started (Steam relaunching a game that was run outside it).
bool startup();

// Once per main-loop iteration, focused or not. False means the store asked the program to
// quit (Steam is shutting down); the loop treats that like the window being closed.
bool update();

// The store's overlay is up, over the game. A single-player game pauses while it is
// (GameShell::graphicsQuant).
bool overlayActive();

// Last thing before the program exits.
void shutdown();

} // namespace PlatformServices

#endif // VISTA_PLATFORM_SERVICES_H
