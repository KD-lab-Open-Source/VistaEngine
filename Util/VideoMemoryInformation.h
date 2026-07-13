#ifndef __VIDEO_MEMORY_INFORMATION_H_INCLUDED__
#define __VIDEO_MEMORY_INFORMATION_H_INCLUDED__

#include <vector>
#include <cstdint>
#include "XMath/xmath.h"

//Вызывать только после вызова CoInitializeEx(0, COINIT_MULTITHREADED);
int GetVideoMemory();
struct IDirect3D9;

// uint32_t, not DWORD: this header is included by every platform (the implementation is
// VideoMemoryInformationStub.cpp everywhere), and DWORD only exists where windows.h has
// been included first — which is nowhere, in the stub's translation unit.
bool CheckDeviceType(IDirect3D9* lpD3D, int xscr, int yscr, bool fullscreen, bool stencil, bool alpha, std::vector<uint32_t>* multisamplemode = 0);
bool getSupportedResolutions(IDirect3D9* lpD3D, bool fullscreen, bool stencil, bool alpha, std::vector<Vect2i>& modes);

#endif