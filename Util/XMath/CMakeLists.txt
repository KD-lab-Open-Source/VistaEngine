add_library(XMath STATIC
    Box6f.cpp
    Colors.cpp
    ComboListColor.cpp
    KeysBase.cpp
    Mat4f.cpp
    Plane.cpp
    Rectangle4f.cpp
    XMath.cpp
    mats.cpp
    stdafx.cpp
)

target_include_directories(XMath PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(XMath PUBLIC EngineIncludes)
