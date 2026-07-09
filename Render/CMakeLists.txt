add_subdirectory(3dx)

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
)

if(WIN32)
    list(APPEND RENDER_SOURCES
        # The D3D9 backend: these .cpp drive IDirect3DDevice9 / IDirect3D9 (device
        # creation, GDI text) directly. Off-Windows the fake d3d9.h only makes the
        # headers parse, not the device-creation call sites, so the real backend is
        # Windows-only. The cross-platform renderer is the SDL GPU backend (Track B);
        # off-Windows the referenced symbols come from RenderStub.cpp below.
        D3D/AdapterMode.cpp
        D3D/D3DRender.cpp
        D3D/D3DRenderDraw.cpp
        D3D/D3DRenderTexture.cpp
        D3D/D3DRenderTilemap.cpp
        D3D/DrawType.cpp
        D3D/OcclusionQuery.cpp
        D3D/PoolManager.cpp
        D3D/RenderDevice.cpp
        D3D/StdAfxRD.cpp
        D3D/VertexDeclaration.cpp

        # Win32 application host (WinMain + message loop) and DbgHelp stack
        # walkers. The cross-platform entry point / windowing is the eventual
        # SDL3 replacement (Track B).
        Util/Runtime3D.cpp
        Util/stack.cpp
        Util/stackwalker.cpp
        src/WinVideo.cpp
    )
else()
    # Off-Windows: no-op definitions for the render-backend symbols the engine
    # references (gb_RenderDevice + the D3D device/buffer classes), so the game
    # links with a silent renderer until the SDL GPU backend (Track B).
    list(APPEND RENDER_SOURCES RenderStub.cpp)
    # The cross-platform SDL GPU render device (replaces the D3D9 cD3DRender), plus
    # the renderer classes it forwards drawing to (2D/UI so far).
    list(APPEND RENDER_SOURCES SDLRenderDevice.cpp)
    list(APPEND RENDER_SOURCES SDLUIRenderer.cpp)
    # Vertex-format declaration tables (BEGIN_VERTEX_DECLARATION). Pure data — no
    # device calls — so it's portable: it feeds vertex::declaration layouts to the
    # SDL backend via the portable cD3DRender::RegisterVertexDeclaration (RenderStub).
    list(APPEND RENDER_SOURCES D3D/VertexDeclaration.cpp)
    # Portable DDS decoder (D3DX is Windows-only); model textures ship as DDS.
    list(APPEND RENDER_SOURCES src/DDSImage.cpp)
endif()

add_library(Render STATIC ${RENDER_SOURCES})
# FreeType (FetchContent) — used by the font rasterizer (FT_Font.cpp, ftrender.cpp).
# Replaces the old vendored include dir + FreeTypeStub.cpp; the freetype target
# propagates its own include paths.
target_link_libraries(Render PUBLIC EngineIncludes Util 3dx SDL3::SDL3)
target_link_libraries(Render PRIVATE freetype)
