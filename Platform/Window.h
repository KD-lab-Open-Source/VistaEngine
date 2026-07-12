#ifndef VISTA_PLATFORM_WINDOW_H
#define VISTA_PLATFORM_WINDOW_H

// SDL3 windowing and input, on every platform.
//
// This is the engine's only window: the SDL GPU device (cSDLRenderDevice) can only
// claim a swapchain on an SDL_Window, so SDL creates the window everywhere, and SDL's
// event queue is the only source of input. The Win32 window class and message loop
// (RegisterClassEx / CreateWindow / PeekMessage / DispatchMessage) are gone -- not
// because D3D9 left, but because SDL owns the window now, and its own pump drains the
// OS queue; a second message loop alongside it would steal messages from it.
//
// What did NOT go is the WM_* message *vocabulary*: Runtime::eventHandler and
// GameShell::EventParser still dispatch on WM_LBUTTONDOWN / WM_KEYDOWN / ... with
// Win32 WPARAM/LPARAM packing, and pumpEvents() below synthesizes those from SDL
// events. Teaching the engine's input dispatch to speak SDL natively is a separate
// refactor (GameShell, ControlManager's key bindings, the kdw dialogs); translating
// is the cheap path, and it costs nothing at runtime.
//
// Two handles come out of the same window, and they are not interchangeable:
//
//   current()      the SDL_Window*, as an opaque void*. This is what SDL GPU
//                  claims a swapchain on, and nothing else wants it.
//   nativeHandle() the OS window handle, i.e. the engine's HWND. A real HWND on
//                  Windows (DirectSound, DirectInput and the kdw editor dialogs
//                  all need one); off-Windows, where HWND is itself a void*
//                  (Platform/WindowsAPI.h) and the Win32 calls are no-op shims,
//                  the SDL_Window* stands in for it.
//
// Keeping them apart is what lets the engine hold an HWND that still means
// something to Win32 while the renderer holds the SDL window.

#include "Platform/WindowsAPI.h"

namespace PlatformWindow {

// Create the main game window via SDL3, initializing the video subsystem on the
// first call. Returns the SDL_Window* as a void*, or nullptr on failure.
void* create(const char* title, int width, int height);

// Destroy a window previously returned by create().
void destroy(void* window);

// The window created above, as an SDL_Window* -- for the SDL GPU device.
void* current();

// The OS window handle of the window created above. See the note on HWND above.
HWND nativeHandle();

// Resize the window and re-centre it on the display (what the options screen does
// when the resolution changes).
void setSize(int width, int height);

// Raise the window and give it keyboard focus.
void focus();

// Minimize the window -- the error handler does this before putting a dialog up.
void minimize();

// Sink for translated input events. Parameters mirror the Win32 window-procedure
// message triple (uMsg, wParam, lParam) so the SDL events can be fed straight
// into the engine's existing message path (runtimeWndProc -> eventHandler).
typedef void (*WindowEventSink)(UINT uMsg, WPARAM wParam, LPARAM lParam);

// Drain pending SDL events, translating mouse/keyboard/focus input into Win32
// window messages delivered through `sink`. Returns false when the user
// requested quit (SDL_EVENT_QUIT), true otherwise.
bool pumpEvents(WindowEventSink sink);

// Block until an event arrives (or the timeout elapses), leaving it queued for
// the next pumpEvents. What the game idles on when it isn't running a frame.
void waitEvents();

} // namespace PlatformWindow

#endif // VISTA_PLATFORM_WINDOW_H
