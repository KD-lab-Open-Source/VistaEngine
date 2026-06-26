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
        # Win32 application host (WinMain + message loop) and DbgHelp stack
        # walkers. The cross-platform entry point / windowing is the eventual
        # SDL3 replacement (Track B).
        Util/Runtime3D.cpp
        Util/stack.cpp
        Util/stackwalker.cpp
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
        src/WinVideo.cpp
    )
endif()

add_library(Render STATIC ${RENDER_SOURCES})
# FreeType (vendored) — used by the font rasterizer (FT_Font.cpp, ftrender.cpp).
target_include_directories(Render PRIVATE ${CMAKE_SOURCE_DIR}/XLibs.Net/FreeType/include)
target_link_libraries(Render PUBLIC EngineIncludes Util 3dx SDL3::SDL3)
