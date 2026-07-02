#ifndef VISTA_PLATFORM_WINDOW_H
#define VISTA_PLATFORM_WINDOW_H

// Cross-platform (SDL3) windowing for the non-Windows build.
//
// On Windows the engine uses its native Win32 window (Runtime::createWindow);
// off-Windows there is no real OS window yet (the Win32 calls hit the no-op
// shims in Platform/WindowsAPI.h). This module stands up a real SDL3 window so
// the SDL GPU device (cSDLRenderDevice) has something to claim a swapchain on.
//
// The window handle is returned as an opaque void* so it travels through the
// engine's HWND (== void*, see Platform/WindowsAPI.h) without leaking SDL into
// engine headers; the render device casts it back to SDL_Window*.
#ifndef _WIN32

namespace PlatformWindow {

// Create the main game window via SDL3, initializing the video subsystem on the
// first call. Returns the SDL_Window* as a void*, or nullptr on failure.
void* create(const char* title, int width, int height);

// Destroy a window previously returned by create().
void destroy(void* window);

// Sink for translated input events. Parameters mirror the Win32 window-procedure
// message triple (uMsg, wParam, lParam) so the SDL events can be fed straight
// into the engine's existing message path (runtimeWndProc -> eventHandler).
typedef void (*WindowEventSink)(UINT uMsg, WPARAM wParam, LPARAM lParam);

// Drain pending SDL events, translating mouse/keyboard/focus input into Win32
// window messages delivered through `sink`. Returns false when the user
// requested quit (SDL_EVENT_QUIT), true otherwise.
bool pumpEvents(WindowEventSink sink);

} // namespace PlatformWindow

#endif // !_WIN32
#endif // VISTA_PLATFORM_WINDOW_H
