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

# Interface target that gives every module common include paths
add_library(EngineIncludes INTERFACE)
target_include_directories(EngineIncludes INTERFACE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/XLibs.Net/XUtil
    ${CMAKE_SOURCE_DIR}/XLibs.Net/XZip
    ${CMAKE_SOURCE_DIR}/XLibs.Net/Profiler/Profiler
    ${CMAKE_SOURCE_DIR}/Util/XTL
)

if(NOT WIN32)
    # fake Windows headers must come before any system paths
    target_include_directories(EngineIncludes BEFORE INTERFACE
        ${CMAKE_SOURCE_DIR}/Platform/fake_includes
    )
    target_compile_definitions(EngineIncludes INTERFACE _CROSS_PLATFORM_)
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
add_subdirectory(Game)
