add_library(3dx STATIC
    Interpolator3dx.cpp
    Lib3dx.cpp
    Node3DX.cpp
    Saver.cpp
    Simply3dx.cpp
    Static3DX.cpp
    Static3dxBase.cpp
    VoxelBox.cpp
    stdafx.cpp
    umath.cpp
)

target_link_libraries(3dx PUBLIC EngineIncludes Util)
