set(XUTIL_SOURCES
    Console.cpp
    Crc/crc.cpp
    Unicode/UnicodeConverter.cpp
    Unicode/Utf8.cpp
    Unicode/WBuffer.cpp
    skey.cpp
    XUtilCore.cpp
    XUtil/XClock.cpp
)

if(WIN32)
    list(APPEND XUTIL_SOURCES
        XERRHAND/BSUFunctions.cpp
        XERRHAND/CrashHandler.cpp
        XERRHAND/DiagAssert.cpp
        XERRHAND/GetLoadedModules.cpp
        XERRHAND/IsNT.cpp
        XERRHAND/NT4ProcessInfo.cpp
        XERRHAND/TLHELPProcessInfo.cpp
    )
endif()

add_library(XUtil STATIC ${XUTIL_SOURCES})
target_include_directories(XUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(XUtil PUBLIC EngineIncludes)
