cmake_minimum_required(VERSION 3.25)
project(VistaEngine CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3)

# FreeType — font rasterizer for the UI text (Render/src/FT_Font.cpp, ftrender.cpp).
# Replaces the vendored 2.2 tree (XLibs.Net/FreeType) and the link-time stub.
# Disable optional external deps so it's self-contained and needs no system libs.
set(FT_DISABLE_HARFBUZZ ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG      ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_ZLIB     ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2    ON  CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI   ON  CACHE BOOL "" FORCE)
FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG        VER-2-13-3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(freetype)

# Interface target that gives every module common include paths
add_library(EngineIncludes INTERFACE)
target_include_directories(EngineIncludes INTERFACE
    # Util's precompiled-header StdAfx.h lives here; list it first so bare
    # "StdAfx.h" includes from Util/*.cpp resolve to it rather than to a
    # same-named StdAfx.h in one of the module dirs added below.
    ${CMAKE_SOURCE_DIR}/Util/XMath
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Util
    # Module source dirs, so legacy unqualified cross-module includes
    # (e.g. "UnitLink.h", "BaseUniverseObject.h") resolve as they did
    # under the original MSVC build, where every module dir was a search root.
    ${CMAKE_SOURCE_DIR}/Units
    ${CMAKE_SOURCE_DIR}/Environment
    ${CMAKE_SOURCE_DIR}/Physics
    ${CMAKE_SOURCE_DIR}/Terra
    ${CMAKE_SOURCE_DIR}/Game
    ${CMAKE_SOURCE_DIR}/UserInterface
    ${CMAKE_SOURCE_DIR}/Network
    ${CMAKE_SOURCE_DIR}/Sound
    ${CMAKE_SOURCE_DIR}/Render/inc
    ${CMAKE_SOURCE_DIR}/Render/src
    ${CMAKE_SOURCE_DIR}/Render/3dx
    ${CMAKE_SOURCE_DIR}/Render/D3D
    ${CMAKE_SOURCE_DIR}/XLibs.Net/XUtil
    ${CMAKE_SOURCE_DIR}/XLibs.Net/XUtil/CRC
    ${CMAKE_SOURCE_DIR}/XLibs.Net/XUtil/Unicode
    ${CMAKE_SOURCE_DIR}/XLibs.Net/XZip
    ${CMAKE_SOURCE_DIR}/XLibs.Net/Profiler/Profiler
    ${CMAKE_SOURCE_DIR}/XLibs.Net/OGG/PlayOgg
    ${CMAKE_SOURCE_DIR}/Util/XTL
)

if(NOT WIN32)
    # fake Windows headers must come before any system paths
    target_include_directories(EngineIncludes BEFORE INTERFACE
        ${CMAKE_SOURCE_DIR}/Platform/fake_includes
    )
    target_compile_definitions(EngineIncludes INTERFACE _CROSS_PLATFORM_)
    # This is MSVC-origin code: -fms-extensions lets clang accept MSVC-isms that
    # are pervasive here (notably the __super keyword, used in ~180 files).
    target_compile_options(EngineIncludes INTERFACE
        "$<$<COMPILE_LANGUAGE:CXX>:-fms-extensions>"
        # The 'register' keyword (removed in C++17) is used in ~19 legacy files.
        "$<$<COMPILE_LANGUAGE:CXX>:-Wno-register>"
        # MSVC allowed taking the address of a temporary (e.g. f(&UI_TextFormat(...)));
        # this is pervasive in the UI render code. The temporary lives to the end of
        # the full-expression, so the calls are well-defined; downgrade to a warning.
        "$<$<COMPILE_LANGUAGE:CXX>:-Wno-error=address-of-temporary>"
        # Force-include WindowsAPI.h as the very first header in every C++ translation unit
        "$<$<COMPILE_LANGUAGE:CXX>:-include;${CMAKE_SOURCE_DIR}/Platform/WindowsAPI.h>"
    )
endif()

add_subdirectory(XLibs.Net)
add_subdirectory(Util)
add_subdirectory(AI)
add_subdirectory(Terra)
add_subdirectory(Physics)
add_subdirectory(Sound)
add_subdirectory(Network)
add_subdirectory(Units)
add_subdirectory(Environment)
add_subdirectory(Water)
add_subdirectory(UserInterface)
add_subdirectory(Render)
add_subdirectory(VistaRender)
add_subdirectory(Game)
