// SDL3 windowing for the non-Windows build. See Platform/Window.h.
#include "Platform/Window.h"

#ifndef _WIN32

// We provide our own main() (Game/PlatformStub.cpp), so prevent SDL from
// remapping main / supplying its own entry point.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cstdio>

// The engine's own WM_MOUSEWHEEL (Util/SystemUtil.h) is WM_MOUSELAST + 1, not the
// native Win32 0x020A. Match that value so the message we synthesize here is the
// same one GameShell::EventParser dispatches on.
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST + 1)
#endif

namespace PlatformWindow {

void* create(const char* title, int width, int height)
{
	if(!SDL_WasInit(SDL_INIT_VIDEO)){
		SDL_SetMainReady();
		if(!SDL_Init(SDL_INIT_VIDEO)){
			fprintf(stderr, "PlatformWindow: SDL_Init(VIDEO) failed: %s\n", SDL_GetError());
			return nullptr;
		}
	}

	if(width  <= 0) width  = 1024;
	if(height <= 0) height = 768;

	SDL_Window* window = SDL_CreateWindow(title && *title ? title : "VistaEngine",
	                                      width, height, 0);
	if(!window){
		fprintf(stderr, "PlatformWindow: SDL_CreateWindow failed: %s\n", SDL_GetError());
		return nullptr;
	}

	// Route keyboard text through SDL_EVENT_TEXT_INPUT so edit fields (profile
	// name, multiplayer IP, ...) receive characters.
	SDL_StartTextInput(window);
	return window;
}

void destroy(void* window)
{
	if(window)
		SDL_DestroyWindow(static_cast<SDL_Window*>(window));
}

namespace {

// Pack window-relative pixel coordinates into an LPARAM the way Win32 mouse
// messages do: LOWORD = x, HIWORD = y. GameShell reads them back with
// LOWORD/HIWORD (see GameShell::convert).
LPARAM packCoords(float x, float y)
{
	int xi = x > 0.f ? int(x) : 0;
	int yi = y > 0.f ? int(y) : 0;
	return LPARAM((WORD(yi) << 16) | WORD(xi));
}

// Current mouse-button + modifier bitmask, as a Win32 MK_* set. Feeds the mouse
// message wParam so drag operations (button held during move) work.
WPARAM currentMouseFlags()
{
	SDL_MouseButtonFlags mb = SDL_GetMouseState(nullptr, nullptr);
	SDL_Keymod mod = SDL_GetModState();
	WPARAM flags = 0;
	if(mb & SDL_BUTTON_LMASK)  flags |= MK_LBUTTON;
	if(mb & SDL_BUTTON_RMASK)  flags |= MK_RBUTTON;
	if(mb & SDL_BUTTON_MMASK)  flags |= MK_MBUTTON;
	if(mod & SDL_KMOD_SHIFT)   flags |= MK_SHIFT;
	if(mod & SDL_KMOD_CTRL)    flags |= MK_CONTROL;
	return flags;
}

// Map an SDL virtual keycode to a Win32 virtual-key code (the engine keys off
// VK_* values throughout). Returns 0 for keys with no VK equivalent.
WPARAM mapKeyToVK(SDL_Keycode key)
{
	// Letters: SDL reports lowercase ASCII; VK codes are the uppercase letter.
	if(key >= SDLK_A && key <= SDLK_Z) return WPARAM(key - 0x20);
	// Digits map straight through (VK '0'..'9' == ASCII).
	if(key >= SDLK_0 && key <= SDLK_9) return WPARAM(key);

	switch(key){
	case SDLK_RETURN:
	case SDLK_KP_ENTER:  return VK_RETURN;
	case SDLK_ESCAPE:    return VK_ESCAPE;
	case SDLK_BACKSPACE: return VK_BACK;
	case SDLK_TAB:       return VK_TAB;
	case SDLK_SPACE:     return VK_SPACE;
	case SDLK_DELETE:    return VK_DELETE;
	case SDLK_INSERT:    return VK_INSERT;
	case SDLK_HOME:      return VK_HOME;
	case SDLK_END:       return VK_END;
	case SDLK_PAGEUP:    return VK_PRIOR;
	case SDLK_PAGEDOWN:  return VK_NEXT;
	case SDLK_LEFT:      return VK_LEFT;
	case SDLK_RIGHT:     return VK_RIGHT;
	case SDLK_UP:        return VK_UP;
	case SDLK_DOWN:      return VK_DOWN;
	case SDLK_PAUSE:     return VK_PAUSE;
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:    return VK_SHIFT;
	case SDLK_LCTRL:
	case SDLK_RCTRL:     return VK_CONTROL;
	case SDLK_LALT:
	case SDLK_RALT:      return VK_MENU;

	// The numeric keypad drives the camera: rotate is bound to KP 4/6/8/2 and
	// zoom to KP +/- (Scripts/Content/Controls), so these are not optional.
	case SDLK_KP_0:        return VK_NUMPAD0;
	case SDLK_KP_1:        return VK_NUMPAD0 + 1;
	case SDLK_KP_2:        return VK_NUMPAD2;
	case SDLK_KP_3:        return VK_NUMPAD0 + 3;
	case SDLK_KP_4:        return VK_NUMPAD4;
	case SDLK_KP_5:        return VK_NUMPAD0 + 5;
	case SDLK_KP_6:        return VK_NUMPAD6;
	case SDLK_KP_7:        return VK_NUMPAD0 + 7;
	case SDLK_KP_8:        return VK_NUMPAD8;
	case SDLK_KP_9:        return VK_NUMPAD9;
	case SDLK_KP_PLUS:     return VK_ADD;
	case SDLK_KP_MINUS:    return VK_SUBTRACT;
	case SDLK_KP_MULTIPLY: return VK_MULTIPLY;
	case SDLK_KP_DIVIDE:   return VK_DIVIDE;
	case SDLK_KP_PERIOD:   return VK_DECIMAL;
	}

	if(key >= SDLK_F1 && key <= SDLK_F12)
		return WPARAM(VK_F1 + (key - SDLK_F1));

	// Remaining printable ASCII (punctuation) passes through unchanged.
	if(key < 0x80) return WPARAM(key);
	return 0;
}

// Decode one UTF-8 sequence at s (bounded by end) into a Unicode code point.
// Advances s past the sequence. Returns 0 on malformed input.
unsigned decodeUtf8(const char*& s, const char* end)
{
	unsigned char c = (unsigned char)*s++;
	if(c < 0x80) return c;
	int extra; unsigned cp;
	if((c & 0xE0) == 0xC0){ extra = 1; cp = c & 0x1F; }
	else if((c & 0xF0) == 0xE0){ extra = 2; cp = c & 0x0F; }
	else if((c & 0xF8) == 0xF0){ extra = 3; cp = c & 0x07; }
	else return 0;
	while(extra-- > 0){
		if(s >= end || (*s & 0xC0) != 0x80) return 0;
		cp = (cp << 6) | (*s++ & 0x3F);
	}
	return cp;
}

} // namespace

bool pumpEvents(WindowEventSink sink)
{
	SDL_Event event;
	while(SDL_PollEvent(&event)){
		switch(event.type){
		case SDL_EVENT_QUIT:
			return false;

		case SDL_EVENT_WINDOW_FOCUS_GAINED:
			sink(WM_ACTIVATEAPP, TRUE, 0);
			break;
		case SDL_EVENT_WINDOW_FOCUS_LOST:
			// SDL delivers no key-ups while unfocused, so anything held at this
			// point would otherwise stay "down" forever (e.g. a camera pan key).
			PlatformClearKeyStates();
			sink(WM_ACTIVATEAPP, FALSE, 0);
			break;

		case SDL_EVENT_MOUSE_MOTION:
			sink(WM_MOUSEMOVE, currentMouseFlags(),
			     packCoords(event.motion.x, event.motion.y));
			break;

		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP: {
			bool down = event.button.down;
			LPARAM lp = packCoords(event.button.x, event.button.y);
			WPARAM wp = currentMouseFlags();
			UINT msg = 0;
			switch(event.button.button){
			case SDL_BUTTON_LEFT:
				msg = down ? (event.button.clicks >= 2 ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN)
				           : WM_LBUTTONUP;
				PlatformSetKeyState(VK_LBUTTON, down);
				break;
			case SDL_BUTTON_RIGHT:
				msg = down ? (event.button.clicks >= 2 ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN)
				           : WM_RBUTTONUP;
				PlatformSetKeyState(VK_RBUTTON, down);
				break;
			case SDL_BUTTON_MIDDLE:
				msg = down ? (event.button.clicks >= 2 ? WM_MBUTTONDBLCLK : WM_MBUTTONDOWN)
				           : WM_MBUTTONUP;
				PlatformSetKeyState(VK_MBUTTON, down);
				break;
			}
			if(msg) sink(msg, wp, lp);
			break;
		}

		case SDL_EVENT_MOUSE_WHEEL:
			// GameShell::MouseWheel keys only off the sign of the delta.
			sink(WM_MOUSEWHEEL, WPARAM(event.wheel.y > 0 ? 120 : -120),
			     packCoords(event.wheel.mouse_x, event.wheel.mouse_y));
			break;

		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP: {
			bool down = (event.type == SDL_EVENT_KEY_DOWN);

			// Take the modifiers from SDL's own state rather than from the key
			// transition: VK_SHIFT/CONTROL/MENU are each shared by two physical
			// keys, so releasing one must not clear the flag while the other is
			// still held.
			SDL_Keymod mod = event.key.mod;
			PlatformSetKeyState(VK_SHIFT,   (mod & SDL_KMOD_SHIFT) != 0);
			PlatformSetKeyState(VK_CONTROL, (mod & SDL_KMOD_CTRL)  != 0);
			PlatformSetKeyState(VK_MENU,    (mod & SDL_KMOD_ALT)   != 0);

			WPARAM vk = mapKeyToVK(event.key.key);
			if(!vk) break;
			if(vk != VK_SHIFT && vk != VK_CONTROL && vk != VK_MENU)
				PlatformSetKeyState(int(vk), down);

			// lParam bit 30 = previous key state (set on auto-repeat), matching
			// what KeyPressed reads for repeat suppression.
			LPARAM lp = (down && event.key.repeat) ? 0x40000000 : 0;
			sink(down ? WM_KEYDOWN : WM_KEYUP, vk, lp);
			break;
		}

		case SDL_EVENT_TEXT_INPUT: {
			const char* s = event.text.text;
			const char* end = s + SDL_strlen(s);
			while(s < end){
				unsigned cp = decodeUtf8(s, end);
				if(cp)
					sink(WM_UNICHAR, WPARAM(cp & 0xFFFF), 0);
			}
			break;
		}
		}
	}
	return true;
}

} // namespace PlatformWindow

#endif // !_WIN32
