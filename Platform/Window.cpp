// SDL3 windowing for the non-Windows build. See Platform/Window.h.
#include "Platform/Window.h"

#ifndef _WIN32

// We provide our own main() (Game/PlatformStub.cpp), so prevent SDL from
// remapping main / supplying its own entry point.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cstdio>

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
	return window;
}

void destroy(void* window)
{
	if(window)
		SDL_DestroyWindow(static_cast<SDL_Window*>(window));
}

bool pumpEvents()
{
	SDL_Event event;
	while(SDL_PollEvent(&event)){
		if(event.type == SDL_EVENT_QUIT)
			return false;
	}
	return true;
}

} // namespace PlatformWindow

#endif // !_WIN32
