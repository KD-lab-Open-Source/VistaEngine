add_subdirectory(3dx)

set(RENDER_SOURCES
    Util/Runtime3D.cpp
    Util/stack.cpp
    Util/stackwalker.cpp
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
    src/Line3d.cpp
    src/MultiRegion.cpp
    src/NParticle.cpp
    src/NParticleKey.cpp
    src/RenderCubemap.cpp
    src/Scene.cpp
    src/ShadowVolume.cpp
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
    src/particle.cpp
    src/perlin.cpp
)

if(WIN32)
    list(APPEND RENDER_SOURCES
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
target_link_libraries(Render PUBLIC EngineIncludes Util 3dx SDL3::SDL3)
