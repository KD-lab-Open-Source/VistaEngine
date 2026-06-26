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
    LocString.cpp
    ObjectSpreader.cpp
    ScanningShape.cpp
    SynchroTimer.cpp
    TextDB.cpp
    Timers.cpp
    ZipConfig.cpp
)

if(WIN32)
    list(APPEND UTIL_SOURCES
        ConsoleWindow.cpp
        DebugUtil.cpp
        FormulaString.cpp
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
endif()

add_library(Util STATIC ${UTIL_SOURCES})
target_include_directories(Util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Util PUBLIC EngineIncludes XUtil XZip XMath Serialization XmlRpc)
