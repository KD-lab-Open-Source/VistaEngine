#pragma once
// DirectDraw — stub for non-Windows builds. Only the DDS file-header structures
// (DDSURFACEDESC2 et al.) are modelled, since Texture.cpp casts a raw .dds file
// header onto DDSURFACEDESC2 to read its dimensions/caps. The layout below
// matches the on-disk DDS header so the cast stays correct cross-platform.
#include "../WindowsAPI.h"

struct DDPIXELFORMAT {
    DWORD dwSize, dwFlags, dwFourCC, dwRGBBitCount;
    DWORD dwRBitMask, dwGBitMask, dwBBitMask, dwRGBAlphaBitMask;
};

struct DDSCAPS2 {
    DWORD dwCaps, dwCaps2, dwCaps3, dwCaps4;
};

struct DDSURFACEDESC2 {
    DWORD dwSize, dwFlags, dwHeight, dwWidth;
    DWORD dwPitchOrLinearSize, dwDepth, dwMipMapCount;
    DWORD dwReserved1[11];
    DDPIXELFORMAT ddpfPixelFormat;
    DDSCAPS2 ddsCaps;
    DWORD dwReserved2;
};

#define DD_OK 0

// DDSCAPS2 flags.
#define DDSCAPS2_CUBEMAP            0x00000200
#define DDSCAPS2_CUBEMAP_POSITIVEX 0x00000400
#define DDSCAPS2_CUBEMAP_NEGATIVEX 0x00000800
#define DDSCAPS2_CUBEMAP_POSITIVEY 0x00001000
#define DDSCAPS2_CUBEMAP_NEGATIVEY 0x00002000
#define DDSCAPS2_CUBEMAP_POSITIVEZ 0x00004000
#define DDSCAPS2_CUBEMAP_NEGATIVEZ 0x00008000
#define DDSCAPS2_VOLUME            0x00200000
