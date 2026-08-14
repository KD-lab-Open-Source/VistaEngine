#ifndef VISTA_PLATFORM_CURSOR_H
#define VISTA_PLATFORM_CURSOR_H

// The mouse cursor, on every platform.
//
// The game's cursors are Windows cursor files under Resource/Cursors, named in
// Scripts/Content/UI_CursorLibrary. Two formats sit behind the one .cur extension:
// eight are plain CUR (an ICO with a hotspot), the other twenty-five are RIFF/ACON
// animated cursors -- 16 frames for the pointer, 25 for the hourglass. Windows read
// both through LoadImage(IMAGE_CURSOR) and showed them with SetCursor, and that is
// what the engine used to call; neither exists elsewhere, so off-Windows the shims in
// Platform/WindowsAPI.h returned nothing and set nothing, and the game had no cursor
// at all.
//
// This decodes both formats itself and hands the frames to SDL, which has a cursor on
// every platform we build for. One path everywhere, rather than a Win32 one and a
// second one beside it: the file format is the game's, not the OS's, and SDL_SetCursor
// does on Windows exactly what SetCursor did.
//
// Animation is ours to drive -- SDL holds one image per cursor -- so a loaded cursor
// keeps its whole frame list and animate() swaps them on the wall clock. Call it once
// a frame; it costs nothing for the static cursors and for a cursor already showing
// the right frame.

namespace PlatformCursor {

/// A loaded cursor: every frame of it, and the timing that walks them.
typedef void* Handle;

/// Decode a .cur/.ani file. The path is the engine's own, backslashes and all.
/// Returns null if the file is missing or not a cursor, having said so on stderr.
Handle load(const char* fileName);

/// Release a handle from load(). Safe on null; clears the active cursor if it is
/// the one being destroyed.
void destroy(Handle cursor);

/// Show this cursor from its first frame. Null hides the pointer, which is what
/// the engine means by SetCursor(NULL) -- direct control, and the cutscenes.
void set(Handle cursor);

/// Advance the active cursor to the frame its rate list says the clock has reached.
/// A no-op unless an animated cursor is showing.
void animate();

} // namespace PlatformCursor

#endif // VISTA_PLATFORM_CURSOR_H
