# The Water module is a visual/render module: nearly every source includes the
# D3D9 render headers (Render/D3D/D3DRender.h, Render/shader/shaders.h) and calls
# into the IDirect3DDevice9 interface. The render-internal calls resolve against
# the expanded fake d3d9.h headers (parse-only) and the Render stub LIB at link.
set(WATER_SOURCES
    WaterStub.cpp
    CircleManager.cpp
    Circles.cpp
    CloudShadow.cpp
    CoastSprites.cpp
    FallLeaves.cpp
    Fallout.cpp
    OrCircle.cpp
    SkyObject.cpp
    SpringDamping.cpp
    Water.cpp
    WaterGarbage.cpp
    WaterWalking.cpp
    Waves.cpp
    bound.cpp
    ice.cpp
    # wind.cpp + Wind.h excluded: pre-refactor orphan (in no .vcproj; references
    # the old cCamera/cBaseGraphObject/cStaticNode/cSimply3dx/NodeObj type names).
    # Wind.h is included only by wind.cpp, so both drop out together.
)

add_library(Water STATIC ${WATER_SOURCES})

target_link_libraries(Water PUBLIC EngineIncludes Util Terra)
