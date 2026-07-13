#pragma once

#include <SDL3/SDL.h>

// Per-platform shader blob.
//
// Each entry point in SDLShaders/*.hlsl is compiled at build time to exactly one
// SDL_GPU-native shader format, chosen by platform:
//
//   Windows : DXIL  (Windows SDK dxc.exe)
//   Linux   : SPIRV (SDL_shadercross)
//   macOS   : MSL   (SDL_shadercross)
//
// cmake/bin2h.cmake emits a `<name>_blob[]` / `<name>_blob_len` pair per entry into the
// generated SDLShaders/<module>_shaders.h. VISTA_SHADER(name) wraps that pair; the
// format is not part of the header, it is whatever this platform builds, so the
// renderers no longer choose a format at runtime — there is only ever one to choose.
//
// vista::kShaderFormat is that same format as a constant. Pass it to
// SDL_CreateGPUDevice, so the device is only ever created on a backend we hold bytecode
// for (see cSDLRenderDevice::Initialize).
namespace vista {

struct ShaderBlob {
	const unsigned char* code;
	unsigned int         len;
};

#if defined(_WIN32)
inline constexpr SDL_GPUShaderFormat kShaderFormat     = SDL_GPU_SHADERFORMAT_DXIL;
inline constexpr const char*         kShaderEntryPoint = "main";
#elif defined(__APPLE__)
inline constexpr SDL_GPUShaderFormat kShaderFormat     = SDL_GPU_SHADERFORMAT_MSL;
// SPIRV-Cross renames the entry point on the way to MSL: `main` is reserved in the
// C++-derived Metal Shading Language, so it emits `main0`.
inline constexpr const char*         kShaderEntryPoint = "main0";
#else
inline constexpr SDL_GPUShaderFormat kShaderFormat     = SDL_GPU_SHADERFORMAT_SPIRV;
inline constexpr const char*         kShaderEntryPoint = "main";
#endif

// Fill in the fields of an SDL_GPUShaderCreateInfo that are the same for every shader we
// build: the bytecode, its format, and its entry point. The caller still sets the stage
// and the resource counts, which differ per shader.
inline SDL_GPUShaderCreateInfo shaderCreateInfo(const ShaderBlob& blob)
{
	SDL_GPUShaderCreateInfo info = {};
	info.code       = blob.code;
	info.code_size  = blob.len;
	info.format     = kShaderFormat;
	info.entrypoint = kShaderEntryPoint;
	return info;
}

} // namespace vista

// Build a ShaderBlob for one compiled entry. Requires `<name>_blob` / `<name>_blob_len`
// to be in scope — i.e. the generated SDLShaders/<module>_shaders.h to be included.
#define VISTA_SHADER(name) ::vista::ShaderBlob{ (name##_blob), (name##_blob_len) }
