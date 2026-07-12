add_subdirectory(3dx)

# ---------------------------------------------------------------------------
# Shaders
# ---------------------------------------------------------------------------
# Every SDL renderer below `#include`s a "SDLShaders/<module>_shaders.h" holding the
# compiled bytecode for its pipelines. Those headers are generated here, into the build
# tree, by compiling the HLSL in SDLShaders/ to the host's one native shader format —
# DXIL, SPIR-V or MSL, see the toolchain block in the root CMakeLists. Each entry point
# becomes a `<var>_blob[]` / `<var>_blob_len` pair; SDLShaders/ShaderBlob.h pairs those
# with the right SDL_GPUShaderFormat at compile time.
set(SHADER_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDLShaders")
set(SHADER_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/SDLShaders")
file(MAKE_DIRECTORY "${SHADER_GEN_DIR}")

set(SHADER_HEADERS)

# Compile a set of HLSL entry points into one header.
#
#   vista_shader_module(<module> <var>=<source.hlsl>:<stage>[:<DEF=1,DEF2=0>] ...)
#
# <stage> is vertex / fragment / compute; the entry function is always `main`. Defines
# are comma-separated. Entries in one module may come from different .hlsl sources (the
# shadow passes do), and one source may appear under several defines (the water and
# object3dx variants do) — each entry is compiled independently.
function(vista_shader_module MODULE)
    set(_out_hdr "${SHADER_GEN_DIR}/${MODULE}_shaders.h")

    # The .hlsl each entry names, deduplicated, so editing a source rebuilds the header.
    set(_sources)
    foreach(_e IN LISTS ARGN)
        string(REGEX REPLACE "^[^=]+=([^:]+):.*$" "\\1" _src "${_e}")
        list(APPEND _sources "${SHADER_SRC_DIR}/${_src}")
    endforeach()
    list(REMOVE_DUPLICATES _sources)

    # Join with `|`, not `;`: CMake splits a semicolon-bearing COMMAND argument into
    # separate argv entries, so a `;`-separated list would reach bin2h.cmake truncated
    # to its first entry.
    string(REPLACE ";" "|" _entries_arg "${ARGN}")

    if(SHADER_COMPILER)
        set(_compiler_arg "-DCOMPILER=${SHADER_COMPILER}")
        set(_extra_deps)
    else()
        # No prebuilt CLI: use the shadercross this build produces, resolved late.
        set(_compiler_arg "-DCOMPILER=$<TARGET_FILE:shadercross>")
        set(_extra_deps   shadercross)
    endif()

    add_custom_command(
        OUTPUT "${_out_hdr}"
        COMMAND "${CMAKE_COMMAND}"
            "-DOUT_HEADER=${_out_hdr}"
            "-DMODE=${SHADER_MODE}"
            "${_compiler_arg}"
            "-DFORMAT_EXT=${SHADER_FORMAT_EXT}"
            "-DSRC_DIR=${SHADER_SRC_DIR}"
            "-DGEN_DIR=${SHADER_GEN_DIR}"
            "-DENTRIES=${_entries_arg}"
            -P "${CMAKE_SOURCE_DIR}/cmake/bin2h.cmake"
        DEPENDS ${_sources} ${_extra_deps} "${CMAKE_SOURCE_DIR}/cmake/bin2h.cmake"
        COMMENT "Compiling ${MODULE} shaders (${SHADER_MODE} -> ${SHADER_FORMAT_EXT})"
        VERBATIM
    )
    set(SHADER_HEADERS ${SHADER_HEADERS} "${_out_hdr}" PARENT_SCOPE)
endfunction()

vista_shader_module(ui
    "ui_vert=ui.vert.hlsl:vertex"
    "ui_frag=ui.frag.hlsl:fragment")

vista_shader_module(minimap
    "minimap_vert=minimap.vert.hlsl:vertex"
    "minimap_frag=minimap.frag.hlsl:fragment"
    "minimap_border_frag=minimap_border.frag.hlsl:fragment")

vista_shader_module(grass
    "grass_vert=grass.vert.hlsl:vertex"
    "grass_frag=grass.frag.hlsl:fragment")

vista_shader_module(cloudshadow
    "cloudshadow_vert=cloudshadow.vert.hlsl:vertex"
    "cloudshadow_frag=cloudshadow.frag.hlsl:fragment")

vista_shader_module(worldquad
    "worldquad_vert=worldquad.vert.hlsl:vertex"
    "worldquad_frag=worldquad.frag.hlsl:fragment")

vista_shader_module(worldtri
    "worldtri_vert=worldtri.vert.hlsl:vertex"
    "worldtri_frag=worldtri.frag.hlsl:fragment")

# The terrain, and the depth-only pass it casts shadows with.
vista_shader_module(tilemap
    "tilemap_vert=tilemap.vert.hlsl:vertex"
    "tilemap_frag=tilemap.frag.hlsl:fragment"
    "tilemap_shadow_vert=tilemap_shadow.vert.hlsl:vertex"
    "tilemap_shadow_frag=tilemap_shadow.frag.hlsl:fragment")

# REFLECTION=1 is the original's water_linear (samples the planar-reflection target);
# REFLECTION=0 is water_easy.
vista_shader_module(water
    "water_vert=water.vert.hlsl:vertex:REFLECTION=0"
    "water_frag=water.frag.hlsl:fragment:REFLECTION=0"
    "water_reflect_vert=water.vert.hlsl:vertex:REFLECTION=1"
    "water_reflect_frag=water.frag.hlsl:fragment:REFLECTION=1")

# SKINNED mirrors the original's `#if(WEIGHT>1)`: a lod binding one bone per vertex
# carries no weight bytes, so its vertex layout — and its shader — differ from the
# skinned one. BUMP mirrors the original's two vertex shaders (object_scene_light.vsl /
# object_scene_bump.vsl) and its psl BUMP define; a bump vertex also carries a tangent
# frame. Shadow casters are depth-only: one fragment shader, no bump variants.
vista_shader_module(object3dx
    "object3dx_rigid_vert=object3dx.vert.hlsl:vertex:SKINNED=0,BUMP=0"
    "object3dx_skin_vert=object3dx.vert.hlsl:vertex:SKINNED=1,BUMP=0"
    "object3dx_rigid_bump_vert=object3dx.vert.hlsl:vertex:SKINNED=0,BUMP=1"
    "object3dx_skin_bump_vert=object3dx.vert.hlsl:vertex:SKINNED=1,BUMP=1"
    "object3dx_frag=object3dx.frag.hlsl:fragment:BUMP=0"
    "object3dx_bump_frag=object3dx.frag.hlsl:fragment:BUMP=1"
    "object3dx_shadow_rigid_vert=object3dx_shadow.vert.hlsl:vertex:SKINNED=0"
    "object3dx_shadow_skin_vert=object3dx_shadow.vert.hlsl:vertex:SKINNED=1"
    "object3dx_shadow_frag=object3dx_shadow.frag.hlsl:fragment")

add_custom_target(VistaShaders DEPENDS ${SHADER_HEADERS})

set(RENDER_SOURCES
    shader/ShaderStorage.cpp
    shader/shaders.cpp
    src/AccessTexture.cpp
    src/CChaos.cpp
    src/ClippingMesh.cpp
    src/FT_Font.cpp
    src/FileImage.cpp
    src/FogOfWar.cpp
    src/Grass.cpp
    src/Leaves.cpp
    src/LensFlare.cpp
    # src/Line3d.cpp — orphan: not in Core.vcproj, references the pre-refactor
    # class hierarchy (cAnimUnkObj, sColor4c, cCamera, MTDECLARE, GetFrame).
    src/MultiRegion.cpp
    src/NParticle.cpp
    src/NParticleKey.cpp
    src/RenderCubemap.cpp
    src/Scene.cpp
    # src/ShadowVolume.cpp — orphan: not in Core.vcproj, references a missing
    # header (MeshTri.h) and the pre-refactor hierarchy (cUnknownClass, sPlane4f,
    # cCamera).
    src/TexLibrary.cpp
    src/Texture.cpp
    src/TextureAtlas.cpp
    src/TextureMiniDetail.cpp
    src/TileMap.cpp
    src/UnkLight.cpp
    src/UnkObj.cpp
    src/VisError.cpp
    src/VisGeneric.cpp
    src/VisGrid2d.cpp
    src/cCamera.cpp
    src/cZPlane.cpp
    src/filter.c
    src/ftrender.cpp
    src/lighting.cpp
    src/observer.cpp
    # src/particle.cpp — orphan: not in Render.vcproj (superseded by NParticle.cpp);
    # references the pre-refactor hierarchy (cAnimUnkObj, sParticleKey, cCamera).
    src/perlin.cpp

    # The renderer: SDL GPU, on every platform. The D3D9 backend it replaces
    # (Render/D3D/D3DRender*.cpp, DrawType, PoolManager, OcclusionQuery, ...) is
    # retired — Windows runs this one too, so there is one renderer to port a
    # feature to, not two.
    SDLRenderDevice.cpp
    SDLUIRenderer.cpp
    SDLMinimapRenderer.cpp
    SDLTileMapRenderer.cpp
    SDLObject3dxRenderer.cpp
    SDLWaterRenderer.cpp
    SDLWorldQuadRenderer.cpp
    SDLGrassRenderer.cpp
    SDLCloudShadowRenderer.cpp
    # Symbols the engine still names that the D3D backend used to define
    # (gb_RenderDevice, the sampler constants, the sPtr buffer handles).
    RenderStub.cpp
    # Vertex-format declaration tables (BEGIN_VERTEX_DECLARATION). Pure data — no
    # device calls — feeding vertex::declaration layouts to the SDL backend.
    D3D/VertexDeclaration.cpp
    # Portable DDS decoder (D3DX is Windows-only); model textures ship as DDS.
    src/DDSImage.cpp
)

if(WIN32)
    # DbgHelp stack walkers: Windows-only debug tooling, nothing to do with the
    # renderer.
    list(APPEND RENDER_SOURCES
        Util/stack.cpp
        Util/stackwalker.cpp
    )
endif()

add_library(Render STATIC ${RENDER_SOURCES})
# FreeType (FetchContent) — used by the font rasterizer (FT_Font.cpp, ftrender.cpp).
# Replaces the old vendored include dir + FreeTypeStub.cpp; the freetype target
# propagates its own include paths.
target_link_libraries(Render PUBLIC EngineIncludes Util 3dx SDL3::SDL3)
target_link_libraries(Render PRIVATE freetype)

# The generated "SDLShaders/<module>_shaders.h" live in the build tree, so that has to
# be an include root too — the renderers include them by the same path they used when
# the headers were checked in beside the HLSL.
target_include_directories(Render PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
# Compile every shader before any Render source: a target-level dependency, because the
# .cpp -> header edge only exists once the compiler has generated a depfile, which it
# cannot do on a clean tree where the header does not exist yet.
add_dependencies(Render VistaShaders)
