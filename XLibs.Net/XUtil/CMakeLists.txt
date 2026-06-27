set(XUTIL_SOURCES
    Console.cpp
    Crc/crc.cpp
    Unicode/UnicodeConverter.cpp
    Unicode/Utf8.cpp
    Unicode/WBuffer.cpp
    skey.cpp
    XUtilCore.cpp
    XUTIL/XUTIL.CPP
    # Portable replacement for the x86 __asm cpuid feature probe.
    XUTIL/CPUIDStub.cpp
    XBUFFER/XBCORE.CPP
    XBUFFER/XBCNVOUT.CPP
    XBUFFER/XBCNVIN.CPP
    XBUFFER/XBSEARCH.CPP
    XSTREAM/XSCORE.CPP
    XSTREAM/XSCNVIN.CPP
    XSTREAM/XSCNVOUT.CPP
    XSTREAM/XSINPUT.CPP
    XSTREAM/XSOUTPUT.CPP
    XSTREAM/XSRDWR.CPP
    XSTREAM/XSSERV.CPP
    XSTREAM/XSGETLIN.CPP
    XSTREAM/XSENLV.CPP
    XSTREAM/XSTIME.CPP
)

if(WIN32)
    list(APPEND XUTIL_SOURCES XUtil/XClock.cpp)
else()
    list(APPEND XUTIL_SOURCES XUtil/XClock_posix.cpp)
endif()

if(WIN32)
    list(APPEND XUTIL_SOURCES
        # The Win32/x86 crash handler: SetUnhandledExceptionFilter + CONTEXT
        # register dump + DbgHelp/BugslayerUtil stack trace + MessageBox.
        XERRHAND/XERRHAND.CPP
        XERRHAND/BSUFunctions.cpp
        XERRHAND/CrashHandler.cpp
        XERRHAND/DiagAssert.cpp
        XERRHAND/GetLoadedModules.cpp
        XERRHAND/IsNT.cpp
        XERRHAND/NT4ProcessInfo.cpp
        XERRHAND/TLHELPProcessInfo.cpp
    )
else()
    # Portable XErrorHandler (ErrH global + Abort/Exit/SetPrefix/SetRestore):
    # same fatal-error semantics without the Win32 crash machinery.
    list(APPEND XUTIL_SOURCES XERRHAND/XErrorHandlerStub.cpp)
endif()

add_library(XUtil STATIC ${XUTIL_SOURCES})
target_include_directories(XUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(XUtil PUBLIC EngineIncludes)
