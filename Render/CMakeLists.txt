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
