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
# XUtil, because XMath.cpp serializes through XStream (operator<= / operator>=). The
# dependency was always real, just never declared — Apple's linker resolves archives in
# any order, GNU ld scans each one once, in the order given, and never goes back.
target_link_libraries(XMath PUBLIC EngineIncludes XUtil)
