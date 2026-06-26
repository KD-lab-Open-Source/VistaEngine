# The Water module is a visual/render module: nearly every source includes the
# D3D9 render headers (Render/D3D/D3DRender.h, Render/shader/shaders.h) and calls
# into the IDirect3DDevice9 interface. Like the renderer, it is stubbed out on
# non-Windows platforms and will be restored on top of the eventual SDL GPU
# backend (Track B) / Render stub LIB.
set(WATER_SOURCES
    WaterStub.cpp
)

if(WIN32)
    list(APPEND WATER_SOURCES
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
        wind.cpp
    )
endif()

add_library(Water STATIC ${WATER_SOURCES})

target_link_libraries(Water PUBLIC EngineIncludes Util Terra)
