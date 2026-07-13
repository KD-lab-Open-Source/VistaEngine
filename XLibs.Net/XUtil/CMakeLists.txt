set(XUTIL_SOURCES
    Console.cpp
    CRC/crc.cpp
    Unicode/UnicodeConverter.cpp
    Unicode/Utf8.cpp
    Unicode/WBuffer.cpp
    skey.cpp
    XUtilCore.cpp
    XUTIL/XUTIL.CPP
    XUTIL/XClock.cpp
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

# The Win32/x86 crash handler (XERRHAND.CPP, CrashHandler.cpp, DiagAssert.cpp,
# BSUFunctions.cpp, GetLoadedModules.cpp, IsNT.cpp, {NT4,TLHELP}ProcessInfo.cpp) is
# not built on any platform. It dumps the CONTEXT registers by their 32-bit names —
# Eip, Esp, Ebp — which do not exist in a 64-bit CONTEXT, and this engine is 64-bit
# everywhere, Windows included. It walks the stack with x86 assumptions besides.
# The files remain as the reference for a real crash reporter.
#
# XErrorHandlerStub carries the fatal-error semantics (the ErrH global, Abort/Exit/
# SetPrefix/SetRestore) plus a portable DiagAssert for xassert().
list(APPEND XUTIL_SOURCES XERRHAND/XErrorHandlerStub.cpp)

add_library(XUtil STATIC ${XUTIL_SOURCES})
target_include_directories(XUtil PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(XUtil PUBLIC EngineIncludes)
