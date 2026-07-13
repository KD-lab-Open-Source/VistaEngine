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

# 3dx and Render are mutually dependent: Render links 3dx, and 3dx's Node3DX.cpp /
# Static3DX.cpp call into Render (Leaves, cAccessTexture). Declaring the cycle is what
# makes CMake repeat both archives on the link line — Apple's linker resolves it either
# way, but GNU ld scans each archive once, in order, and never goes back.
target_link_libraries(3dx PUBLIC EngineIncludes Util Render)
