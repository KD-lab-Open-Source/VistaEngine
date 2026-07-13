add_subdirectory(XMath)
add_subdirectory(Serialization)
add_subdirectory(XmlRpc)

# Neither of the two Util tools is built any more:
#   ShaderToH2 compiled the old .vsl/.psl shaders into headers against d3dx9.h, and
#   both the D3D9 backend and that toolchain are gone — CMake compiles the HLSL now
#   (see cmake/bin2h.cmake).
#   kdw is the Win32 widget toolkit behind the editors' property dialogs. It is 32-bit
#   Win32 GUI code (its own mini-windows.h had UINT_PTR as a 32-bit int) and the game
#   does not need it: kdwStub.cpp below satisfies every kdw entry point the game
#   references, on every platform, as it already did off-Windows.

set(UTIL_SOURCES
    ComboVectorString.cpp
    DebugPrm.cpp
    FileUtils/FileTime.cpp
    FileUtils/FileUtils.cpp
    FileUtils/XGUID.cpp
    # Parameter-formula evaluator; hand-rolled recursive descent since the
    # original Boost.Spirit grammar doesn't build under clang/C++20.
    FormulaString.cpp
    LocString.cpp
    ObjectSpreader.cpp
    ScanningShape.cpp
    SynchroTimer.cpp
    TextDB.cpp
    Timers.cpp
    ZipConfig.cpp
    # Portable debug-draw geometry helpers (clampWorldPosition, clip_circle_3D,
    # show_terrain_line, G2S, To3D); includes render/Terra interfaces only.
    DebugUtil.cpp
)

# One source list, Windows included. Each of these replaces a Win32 leaf that either
# cannot build 64-bit or that the game does not need — the real sources stay in the
# tree as the reference for whoever ports them properly:
#   SystemUtilPortable  the Win32 file dialogs / popup menus / editText boxes are
#                       no-ops; the portable half (time formatting, loc-data path)
#                       is what the game actually calls.
#   ProfilerStub        the real Profiler (XLibs.Net/Profiler) is compiled nowhere,
#                       on any platform — Windows was linking against nothing.
#   VideoMemoryInformationStub  the real one asks D3D9 (and WMI) how much video
#                       memory there is, and D3D9 is gone.
#   joystickStub        joystick.cpp enumerates DirectInput8 devices; SDL3 is the
#                       input layer now.
#   kdwStub             kdw is the editors' 32-bit Win32 widget toolkit.
#   ConsoleWindowStub   the Win32 debug console (and the only user of mfc/PopupMenu,
#                       which despite the directory name is plain USER32 menu code —
#                       nothing in this build has ever included an MFC header).
#   Win32/*Stub         DbgHelp symbol resolver and the raw keyboard driver.
# SplashScreen.cpp and ObjStatistic.cpp are simply gone: nothing references them.
list(APPEND UTIL_SOURCES
    SystemUtilPortable.cpp
    ProfilerStub.cpp
    VideoMemoryInformationStub.cpp
    joystickStub.cpp
    kdwStub.cpp
    ConsoleWindowStub.cpp
    Win32/DebugSymbolManagerStub.cpp
    Win32/KeyboardDriverStub.cpp
)

add_library(Util STATIC ${UTIL_SOURCES})
target_include_directories(Util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Util PUBLIC EngineIncludes XUtil XZip XMath Serialization XmlRpc)
