cmake_minimum_required(VERSION 3.25)
project(VistaEngine CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 REQUIRED CONFIG)

# Interface target that gives every module the root source tree as an include path
add_library(EngineIncludes INTERFACE)
target_include_directories(EngineIncludes INTERFACE ${CMAKE_SOURCE_DIR})

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
