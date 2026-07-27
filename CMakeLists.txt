cmake_minimum_required(VERSION 3.25)
project(VistaEngine CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(FetchContent)

# The engine only ever calls SDL_Init(SDL_INIT_VIDEO) (Platform/Window.cpp): SDL is
# our window, our input, and our GPU device, and nothing else. Audio is miniaudio's
# (see below), joystick is DirectInput, and the rest we never
# touch — so don't compile them. Besides the build time, each subsystem drags in
# system dependencies of its own that CI would otherwise have to install.
# SDL3_test is off for the same reason: it builds by default and we never link it.
# (SDL_TESTS, the test *programs*, is already off — it defaults to "am I the main
# project", and we are a FetchContent subproject.)
#
# SDL_JOYSTICK stays ON despite being just as unused: with it off, SDL's macOS build
# does not link the GameController framework, yet SDL_cocoamouse.m still references
# GCMouse from it, and libSDL3 fails to link. Leave the subsystem in rather than
# carry a patch for it.
set(SDL_AUDIO        OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA       OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG       OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC       OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI       OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR       OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(SDL3)

# ---------------------------------------------------------------------------
# Shader toolchain
# ---------------------------------------------------------------------------
# The HLSL in Render/SDLShaders is compiled at build time to the one shader
# format the host's SDL_GPU backend actually speaks, and embedded as bytes in
# a generated header (see cmake/bin2h.cmake and Render/CMakeLists.txt):
#
#   Windows : D3D12  -> DXIL   via the Windows SDK's dxc.exe
#   Linux   : Vulkan -> SPIR-V via SDL_shadercross
#   macOS   : Metal  -> MSL    via SDL_shadercross
#
# Windows needs no third-party shader compiler: DXIL is the one target the SDK's
# dxc.exe ships codegen for, and it is the only format the D3D12 backend loads.
# SDL_shadercross is the HLSL -> SPIR-V/MSL route everywhere else, and it wants
# DXC plus SPIRV-Cross underneath it.
#
# Setting SHADERCROSS_EXECUTABLE to an already-built shadercross CLI skips
# fetching and building the whole thing — that is the fast path for a dev box
# that has one lying around, and how CI reuses a cached build.
if(WIN32)
    # dxc.exe ships in the Windows SDK under Windows Kits\10\bin\<ver>\x64. Inside a
    # VS developer prompt its directory is $WindowsSdkVerBinPath, but a plain
    # cmake-from-Explorer inherits no such thing, so glob the install root and take
    # the newest SDK (a descending sort puts the highest version first).
    if(NOT DXC_EXECUTABLE)
        file(GLOB _winsdk_dxc_candidates
            "C:/Program Files (x86)/Windows Kits/10/bin/*/x64/dxc.exe"
            "C:/Program Files/Windows Kits/10/bin/*/x64/dxc.exe")
        if(_winsdk_dxc_candidates)
            list(SORT _winsdk_dxc_candidates ORDER DESCENDING)
            list(GET _winsdk_dxc_candidates 0 DXC_EXECUTABLE)
        endif()
    endif()
    if(NOT DXC_EXECUTABLE)
        find_program(DXC_EXECUTABLE
            NAMES dxc.exe dxc
            HINTS
                "$ENV{WindowsSdkVerBinPath}x64"
                "$ENV{WindowsSdkDir}bin/x64"
            DOC "Windows SDK DirectX Shader Compiler (HLSL -> DXIL)")
    endif()
    if(NOT DXC_EXECUTABLE)
        message(FATAL_ERROR
            "dxc.exe not found. Install the Windows 10/11 SDK (it ships dxc.exe under "
            "'C:/Program Files (x86)/Windows Kits/10/bin/<ver>/x64'), or pass "
            "-DDXC_EXECUTABLE=<path/to/dxc.exe>.")
    endif()
    set(DXC_EXECUTABLE "${DXC_EXECUTABLE}" CACHE FILEPATH
        "Windows SDK DirectX Shader Compiler (HLSL -> DXIL)" FORCE)
    message(STATUS "Shaders: dxc.exe (DXIL) at ${DXC_EXECUTABLE}")
elseif(SHADERCROSS_EXECUTABLE)
    if(NOT EXISTS "${SHADERCROSS_EXECUTABLE}")
        message(FATAL_ERROR
            "SHADERCROSS_EXECUTABLE=${SHADERCROSS_EXECUTABLE} does not exist. Point it at a "
            "prebuilt shadercross CLI, or leave it unset to build one from source.")
    endif()
    set(SHADERCROSS_EXECUTABLE "${SHADERCROSS_EXECUTABLE}" CACHE FILEPATH
        "Prebuilt SDL_shadercross CLI (skips building DXC + SPIRV-Cross from source)" FORCE)
    message(STATUS "Shaders: prebuilt SDL_shadercross at ${SHADERCROSS_EXECUTABLE}")
else()
    # Build SDL_shadercross from source. Its two dependencies are handled very
    # differently per platform, because DXC is an LLVM fork and building it is by
    # far the most expensive thing in this project's configure+build:
    #
    #   Linux: Microsoft publishes a prebuilt DXC, so download that instead of
    #          spending ~25 min compiling LLVM. SPIRV-Cross still gets built from
    #          the submodule (~1-2 min); distro packages of it are too old for
    #          SDL_shadercross's C API.
    #   macOS: no prebuilt DXC is published for it, so the vendored one is built
    #          (~14 min). CI caches the resulting CLI and feeds it back in via
    #          SHADERCROSS_EXECUTABLE above.
    #
    # Pinned rather than tracking main: this is the revision the checked-in shaders
    # were developed against, and CI keys its build cache on it.
    #
    # SOURCE_SUBDIR names a directory with no CMakeLists.txt, which makes
    # FetchContent_MakeAvailable download the sources *without* immediately calling
    # add_subdirectory. The Linux path has to run SDL_shadercross's own DXC-download
    # script out of the fetched tree before its CMakeLists is processed, and the
    # options below have to be in the cache before it reads them.
    if(LINUX)
        set(_shadercross_submodules "external/SPIRV-Cross")
    else()
        set(_shadercross_submodules "")   # empty list => fetch them all, DXC included
    endif()
    FetchContent_Declare(
        SDL_shadercross
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_shadercross.git
        GIT_TAG        6b06e55c7c5d7e7a09a8a14f76e866dcfad5ab99
        GIT_SUBMODULES ${_shadercross_submodules}
        SOURCE_SUBDIR  cmake-entry-point-suppressed
    )
    FetchContent_MakeAvailable(SDL_shadercross)

    set(SDLSHADERCROSS_DXC        ON  CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_STATIC     ON  CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_SHARED     OFF CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_CLI        ON  CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_INSTALL    OFF CACHE BOOL "" FORCE)
    set(SDLSHADERCROSS_TESTS      OFF CACHE BOOL "" FORCE)

    if(LINUX)
        # Extracts a prebuilt DXC under <src>/external/DirectXShaderCompiler-binaries/linux
        # and points DirectXShaderCompiler_ROOT at it; SDL_shadercross's own
        # FindDirectXShaderCompiler.cmake then picks it up off that root.
        include("${sdl_shadercross_SOURCE_DIR}/build-scripts/download-prebuilt-DirectXShaderCompiler.cmake")

        # One knob, SDLSHADERCROSS_VENDORED, governs *both* dependencies. We want
        # prebuilt DXC but vendored SPIRV-Cross, so turn it off and add SPIRV-Cross
        # ourselves. SDL_shadercross guards its find_package with
        # `if(NOT TARGET spirv_cross_c)`, so a pre-existing target makes it skip.
        set(SDLSHADERCROSS_VENDORED          OFF CACHE BOOL "" FORCE)
        set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE BOOL "" FORCE)

        # SPIRV_CROSS_SKIP_INSTALL stays OFF, and not because anything is installed:
        # SDL_shadercross calls export(TARGETS SDL3_shadercross-static) unconditionally,
        # and CMake refuses to export a static library whose PRIVATE dependency
        # (spirv-cross-c) belongs to no export set. SPIRV-Cross's install macro is what
        # registers those targets, so skipping it fails the configure outright.
        set(SPIRV_CROSS_SKIP_INSTALL     OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_CLI              OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_ENABLE_TESTS     OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_SHARED           OFF CACHE BOOL "" FORCE)
        set(SPIRV_CROSS_STATIC           ON  CACHE BOOL "" FORCE)
        # spirv-cross-c ends up inside static SDL3_shadercross, which ends up inside
        # the CLI; PIC keeps that relocatable link happy.
        set(CMAKE_POSITION_INDEPENDENT_CODE ON)
        add_subdirectory("${sdl_shadercross_SOURCE_DIR}/external/SPIRV-Cross"
                         "${sdl_shadercross_BINARY_DIR}/external/SPIRV-Cross" EXCLUDE_FROM_ALL)
        # SPIRV-Cross names the target with hyphens; SDL_shadercross's existence check
        # spells it with underscores. The alias satisfies the check — it links the
        # hyphenated name directly, which is the real target.
        add_library(spirv_cross_c ALIAS spirv-cross-c)
        message(STATUS "Shaders: SDL_shadercross + prebuilt DXC + vendored SPIRV-Cross (Linux)")
    else()
        set(SDLSHADERCROSS_VENDORED          ON  CACHE BOOL "" FORCE)
        set(SDLSHADERCROSS_SPIRVCROSS_SHARED OFF CACHE BOOL "" FORCE)
        # HLSL -> MSL runs through DXC's SPIR-V codegen, which DXC leaves off by
        # default. Force it on in the cache so the vendored DXC's option() sees it.
        set(ENABLE_SPIRV_CODEGEN             ON  CACHE BOOL "" FORCE)
        message(STATUS "Shaders: SDL_shadercross + vendored DXC (this build is slow; cache the CLI and pass -DSHADERCROSS_EXECUTABLE)")
    endif()

    add_subdirectory("${sdl_shadercross_SOURCE_DIR}" "${sdl_shadercross_BINARY_DIR}" EXCLUDE_FROM_ALL)
endif()

# What the shader rules in Render/CMakeLists.txt feed to cmake/bin2h.cmake. An empty
# SHADER_COMPILER means "resolve $<TARGET_FILE:shadercross> at build time", i.e. use the
# CLI this build is about to produce.
if(WIN32)
    set(SHADER_MODE       "dxc")
    set(SHADER_COMPILER   "${DXC_EXECUTABLE}")
    set(SHADER_FORMAT_EXT "dxil")
else()
    set(SHADER_MODE       "shadercross")
    set(SHADER_COMPILER   "${SHADERCROSS_EXECUTABLE}")
    if(APPLE)
        set(SHADER_FORMAT_EXT "msl")
    else()
        set(SHADER_FORMAT_EXT "spv")
    endif()
endif()

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

# miniaudio — the audio device on every platform, Windows included (Sound/AudioBackend.cpp).
# It replaces DirectSound, and SDL's audio subsystem stays off (see the SDL block above).
#
# SDL3 was the obvious candidate and does not fit: it mixes streams bound to a device and
# stops there, whereas this game's sound API (SoundSystem/Sound/Channel/SND3DListener) is a
# skin over DirectSound3D — per-emitter min/max distance, rolloff, doppler, fades, a voice
# pool — plus Vorbis for the music. miniaudio's ma_engine covers that set almost 1:1.
#
# Runtime linking is miniaudio's default: it dlopen()s ALSA/PulseAudio/JACK rather than
# linking them, so a Linux build (and CI) needs no audio -dev packages. Leave
# MINIAUDIO_NO_RUNTIME_LINKING off. The libvorbis/libopus decoders are separate targets of
# its own that we do not build; Vorbis arrives with the music slice, via stb_vorbis.
set(MINIAUDIO_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_TESTS     OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_BUILD_TOOLS     OFF CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_LIBVORBIS    ON  CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_LIBOPUS      ON  CACHE BOOL "" FORCE)
set(MINIAUDIO_NO_EXTRA_NODES  ON  CACHE BOOL "" FORCE)   # reverb, vocoder, … : unused
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.25
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(miniaudio)

# ffmpeg — the Bink/AVI decoder behind Video/. Built from source with all but five of its
# components disabled; see the module for what we keep and why it is not a FetchContent.
include(FFmpeg)

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

if(WIN32)
    # The Windows SDK declares a global `byte` (rpcndr.h), C++17 added std::byte, and
    # this codebase says `using namespace std` in its shared headers — so every SDK
    # header that names `byte` is ambiguous (C2872). _HAS_STD_BYTE=0 is the microsoft
    # standard-library's own switch for exactly this; nothing here uses std::byte.
    target_compile_definitions(EngineIncludes INTERFACE _HAS_STD_BYTE=0)

    # windows.h in every C++ translation unit — the mirror of the WindowsAPI.h
    # force-include below, and the same reason. This is MSVC-origin code that assumes
    # the Win32 API is simply *there*: its headers name HWND, HANDLE and DWORD freely
    # (Util/joystick.h, Util/Win32/DebugSymbolManager.h, Sound/Sound.h, the kdw
    # headers, ~18 of them), and each module's StdAfx.h used to drag windows.h in for
    # them. The stub translation units have no StdAfx, so on Windows they saw none of
    # it. Forcing the header is what the original build did in effect, and it settles
    # the whole class rather than one header at a time.
    target_compile_options(EngineIncludes INTERFACE
        "$<$<COMPILE_LANGUAGE:CXX>:/FIwindows.h>")
endif()

# D3D9 declarations, on every platform. The backend is retired and none of its .cpp are
# built, but the render headers still name its types, so the declarations have to exist
# — and on Windows d3dx9.h ships in no modern SDK, so it needs them as much as anyone.
# BEFORE the system paths: on Windows this shadows the SDK's own d3d9.h, deliberately,
# so all three platforms parse the same declarations for a backend nobody calls.
target_include_directories(EngineIncludes BEFORE INTERFACE
    ${CMAKE_SOURCE_DIR}/Platform/d3d9_compat
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

# ---------------------------------------------------------------------------
# AddressSanitizer
# ---------------------------------------------------------------------------
# -DVISTA_ASAN=ON builds the engine — and only the engine — instrumented.
#
# The position in this file is the whole trick: compile and link options set on a
# directory reach the subdirectories added *after* them, and every third-party
# dependency is fetched above. So SDL, FreeType, miniaudio and (on a cold build)
# the vendored DXC keep compiling clean, while everything in the
# add_subdirectory() list below — XLibs.Net included, since XZip and XBuffer are
# exactly where a buffer bug would hide — is instrumented. ffmpeg is out of reach
# either way: it is an ExternalProject with a configure script of its own.
#
# Instrumenting only part of a program is what ASan is built for. The clean
# libraries simply report nothing of their own, and the final link of Game is
# what pulls the runtime in.
#
# macOS only for now. Windows would need MSVC's /fsanitize=address plus its
# clang_rt DLL staged next to the executable, and Linux its own libasan on the
# link line; neither has been tried, and a silently ignored flag is worse than a
# refused one.
option(VISTA_ASAN "Build the engine with AddressSanitizer (macOS only for now)" OFF)
if(VISTA_ASAN)
    if(NOT APPLE)
        message(FATAL_ERROR
            "VISTA_ASAN is macOS-only for now. See the comment above this check in "
            "CMakeLists.txt for what the other two platforms would need.")
    endif()
    # -fno-omit-frame-pointer because the build type we actually run is
    # RelWithDebInfo: -O2 drops the frame pointer, and with it the readable half
    # of every ASan report.
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    message(STATUS "AddressSanitizer: ON (engine targets only; dependencies stay uninstrumented)")
endif()

add_subdirectory(XLibs.Net)
add_subdirectory(Util)
add_subdirectory(AI)
add_subdirectory(Terra)
add_subdirectory(Physics)
add_subdirectory(Sound)
add_subdirectory(Video)
add_subdirectory(Network)
add_subdirectory(Units)
add_subdirectory(Environment)
add_subdirectory(Water)
add_subdirectory(UserInterface)
add_subdirectory(Render)
add_subdirectory(VistaRender)
add_subdirectory(Game)
