add_subdirectory(XMath)
add_subdirectory(Serialization)
add_subdirectory(XmlRpc)

if(WIN32)
    add_subdirectory(ShaderToH2)
    add_subdirectory(kdw)
endif()

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

if(WIN32)
    list(APPEND UTIL_SOURCES
        ConsoleWindow.cpp
        SplashScreen.cpp
        mfc/PopupMenu.cpp
        Win32/DebugSymbolManager.cpp
        Win32/KeyboardDriver.cpp
        # Win32-GUI / Game-dependent helpers (file dialogs, popup menus, dialog
        # boxes) and D3D9/WMI display queries. Unused by the lower-level libs;
        # revisit when porting the higher layers / SDL GPU backend.
        SystemUtil.cpp
        VideoMemoryInformation.cpp
        # Win32 debug statistics window (CommCtrl/HWND); non-_FINAL_VERSION_ tooling.
        ObjStatistic.cpp
        # DirectInput8 joystick; SDL3 provides cross-platform joystick input.
        joystick.cpp
    )
else()
    # Portable display-capability queries (the real VideoMemoryInformation.cpp
    # uses D3D9 + WMI): permissive defaults until the SDL GPU backend (Track B).
    list(APPEND UTIL_SOURCES VideoMemoryInformationStub.cpp)
    # Portable SystemUtil subset (time formatting, loc-data path); the Win32 GUI
    # dialogs (file/popup/editText) are no-op off-Windows.
    list(APPEND UTIL_SOURCES SystemUtilPortable.cpp)
    # No-op stubs for the Win32 tooling / DirectInput leaves whose real sources
    # are gated above (debug console, symbol resolver, joystick, keyboard,
    # profiler).
    list(APPEND UTIL_SOURCES
        ConsoleWindowStub.cpp
        ProfilerStub.cpp
        joystickStub.cpp
        Win32/DebugSymbolManagerStub.cpp
        Win32/KeyboardDriverStub.cpp
        # kdw editor toolkit (Win32 widgets): property-editor entry points and
        # the editor dialogs the game references are no-op off-Windows.
        kdwStub.cpp
    )
endif()

add_library(Util STATIC ${UTIL_SOURCES})
target_include_directories(Util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Util PUBLIC EngineIncludes XUtil XZip XMath Serialization XmlRpc)
