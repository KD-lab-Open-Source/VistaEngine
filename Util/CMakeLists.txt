add_subdirectory(XMath)
add_subdirectory(Serialization)
add_subdirectory(XmlRpc)

if(WIN32)
    add_subdirectory(ShaderToH2)
    add_subdirectory(kdw)
endif()

set(UTIL_SOURCES
    ComboVectorString.cpp
    ConsoleWindow.cpp
    DebugPrm.cpp
    DebugUtil.cpp
    FileUtils/FileTime.cpp
    FileUtils/FileUtils.cpp
    FileUtils/XGUID.cpp
    FormulaString.cpp
    LocString.cpp
    ObjStatistic.cpp
    ObjectSpreader.cpp
    ScanningShape.cpp
    SplashScreen.cpp
    SynchroTimer.cpp
    SystemUtil.cpp
    TextDB.cpp
    Timers.cpp
    VideoMemoryInformation.cpp
    ZipConfig.cpp
    joystick.cpp
)

if(WIN32)
    list(APPEND UTIL_SOURCES
        mfc/PopupMenu.cpp
        Win32/DebugSymbolManager.cpp
        Win32/KeyboardDriver.cpp
    )
endif()

add_library(Util STATIC ${UTIL_SOURCES})
target_include_directories(Util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Util PUBLIC EngineIncludes XUtil XZip XMath Serialization XmlRpc)
