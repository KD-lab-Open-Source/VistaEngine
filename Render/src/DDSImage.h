#ifndef __DDS_IMAGE_H__
#define __DDS_IMAGE_H__

// Portable DDS reader for the cross-platform (SDL GPU) backend. The original
// engine decoded DDS via D3DX (cTexture::loadDDS / reloadDDS), which is
// Windows-only; off-Windows the 3D model textures ship *only* as cached DDS
// (DXT1/DXT3/DXT5), so we decode the base mip to BGRA here and feed it through
// the normal cFileImage -> cInterfaceRenderDevice::CreateTexture path.

#include "FileImage.h"
#include <vector>
#include <cstdint>

class cDDSImage : public cFileImage
{
public:
	cDDSImage() { length = 1; }
	virtual int load(const char* fname);
	virtual int load(void* pointer, int size);
	virtual int GetTexture(void* pointer, int time, int xSize, int ySize);

private:
	std::vector<uint8_t> bgra_;   // GetX()*GetY()*4, top-down, B,G,R,A
};

#endif // __DDS_IMAGE_H__
