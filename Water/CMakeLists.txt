add_library(Water STATIC
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

target_link_libraries(Water PUBLIC EngineIncludes Util Terra)
