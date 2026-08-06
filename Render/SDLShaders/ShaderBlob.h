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

// ---------------------------------------------------------------------------------------
// Why every vertex input in SDLShaders/*.vert.hlsl is a TEXCOORD
// ---------------------------------------------------------------------------------------
// SDL_GPU describes a vertex attribute by *location* alone -- SDL_GPUVertexAttribute has no
// semantic field, because SPIR-V and MSL have no semantics to fill it with. D3D12 does, so
// its backend has to invent one: D3D12_INTERNAL_ConvertVertexInputState stamps every
// D3D12_INPUT_ELEMENT_DESC with SemanticName = "TEXCOORD" and SemanticIndex = location.
//
// ID3D12Device::CreateInputLayout then rejects any layout that does not name every element
// the vertex shader's input signature reads. So a shader declaring `float3 pos : POSITION`
// fails pipeline creation outright:
//
//   CREATEINPUTLAYOUT_MISSINGELEMENT: The provided input signature expects to read an
//   element with SemanticName/Index: 'POSITION'/0, but the declaration doesn't provide a
//   matching name.
//
// which surfaces from SDL_CreateGPUGraphicsPipeline as a bare E_INVALIDARG (0x80070057).
// The rule is therefore: a vertex shader input's semantic is TEXCOORD<its location>, never
// what the data means. The `#if`-conditional layouts (object3dx) renumber accordingly.
//
// Only the DXIL path cares. Locations in SPIR-V and MSL come from declaration order, which
// the naming does not affect, so the same source keeps its bindings on Vulkan and Metal.
//
// This constrains *inputs* only. Interpolants between the vertex and fragment stages are
// matched by name within our own shader pairs, and keep the semantic that reads best.

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
