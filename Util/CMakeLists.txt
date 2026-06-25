add_subdirectory(XMath)
add_subdirectory(Serialization)
add_subdirectory(XmlRpc)
add_subdirectory(ShaderToH2)
add_subdirectory(kdw)

# Core util files compiled into the game (from Core.vcproj and Game.vcproj)
add_library(Util STATIC
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
    mfc/PopupMenu.cpp
)

target_include_directories(Util PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Util PUBLIC EngineIncludes XUtil XZip XMath Serialization XmlRpc)
