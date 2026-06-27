// Portable display-capability queries for the cross-platform build.
//
// The real VideoMemoryInformation.cpp queries the D3D9 adapter (device types,
// multisample modes, supported resolutions) and the video memory via WMI — both
// Windows-only. Off-Windows we report a permissive, sane set of defaults until
// the SDL GPU backend (Track B) provides the real device/display enumeration.
#include "VideoMemoryInformation.h"

int GetVideoMemory()
{
	// Report a generous amount (in MB) so memory-budget checks pass.
	return 1024;
}

bool CheckDeviceType(IDirect3D9* /*lpD3D*/, int /*xscr*/, int /*yscr*/,
	bool /*fullscreen*/, bool /*stencil*/, bool /*alpha*/,
	std::vector<DWORD>* multisamplemode)
{
	// No multisample modes advertised; the requested mode is "supported".
	if(multisamplemode)
		multisamplemode->clear();
	return true;
}

bool getSupportedResolutions(IDirect3D9* /*lpD3D*/, bool /*fullscreen*/,
	bool /*stencil*/, bool /*alpha*/, std::vector<Vect2i>& modes)
{
	static const int res[][2] = {
		{800, 600}, {1024, 768}, {1280, 720}, {1280, 1024},
		{1366, 768}, {1600, 900}, {1920, 1080},
	};
	modes.clear();
	for(const auto& r : res)
		modes.push_back(Vect2i(r[0], r[1]));
	return true;
}
