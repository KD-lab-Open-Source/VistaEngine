add_library(XUtil STATIC
    Console.cpp
    Crc/crc.cpp
    Unicode/UnicodeConverter.cpp
    Unicode/Utf8.cpp
    Unicode/WBuffer.cpp
    skey.cpp
    XERRHAND/BSUFunctions.cpp
    XERRHAND/CrashHandler.cpp
    XERRHAND/DiagAssert.cpp
    XERRHAND/GetLoadedModules.cpp
    XERRHAND/IsNT.cpp
    XERRHAND/NT4ProcessInfo.cpp
    XERRHAND/TLHELPProcessInfo.cpp
    XUtilCore.cpp
    XUtil/XClock.cpp
)

target_include_directories(XUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(XUtil PUBLIC EngineIncludes)
