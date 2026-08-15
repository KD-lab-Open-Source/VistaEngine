# SurMap5Qt — the Qt 6 port of the SurMap5 map editor.
#
# Phase 1 milestone: application shell. Pure Qt, no engine linkage yet — the
# window structure (main window, docks, toolbars, status bar) mirrors the old
# CMainFrame layout (SurMap5/MainFrame.h) so the port can land piece by piece:
#   Phase 2: embed the engine's SDL GPU renderer into RenderViewWidget
#            (SDL_CreateWindowWithProperties on its native HWND).
#   Phase 3: input + game loop (CGeneralView::WindowProc port).
#   Phase 4/5: dialogs and tools.
#
# The old SurMap5/ tree is kept as the reference implementation, like kdw's
# stubs elsewhere in the tree.
#
# Engine flags vs Qt: the engine compiles with EngineIncludes INTERFACE flags
# that Qt translation units must not inherit — `_HAS_STD_BYTE=0` (Windows SDK
# `byte` vs std::byte) makes Qt's qtypes.h #error out, and /FIwindows.h would
# drag Win32 headers into Qt code. So the engine-touching sources live in their
# own static library (EditorEngine) that links EngineIncludes and the engine
# modules; SurMap5Qt links that library and stays Qt-clean.
cmake_minimum_required(VERSION 3.25)
project(SurMap5Qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt owns the UI of the editor. Qt itself is found, never vendored — install
# Qt 6 (see Documents/Build-PORTING.md, "The editor (SurMap5Qt)" section).
find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON)

# The engine-touching half of the editor: device init, the render window
# wrapper, the per-frame draw, the world-list scan. Compiled with the engine's
# flags and includes.
add_library(EditorEngine STATIC
    src/editor/EngineViewport.h
    src/editor/EngineViewport.cpp
    src/editor/WorldList.h
    src/editor/WorldList.cpp
)
# EngineIncludes brings the engine's include roots and (on Windows)
# _HAS_STD_BYTE=0 + /FIwindows.h — exactly what engine headers expect.
# PRIVATE: SurMap5Qt must not inherit any of it (Qt needs std::byte).
# The include roots are duplicated below (PRIVATE) because EngineIncludes is
# PRIVATE here — its include directories would otherwise not reach this
# library's own .cpp files.
target_link_libraries(EditorEngine PRIVATE EngineIncludes)
target_include_directories(EditorEngine PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/Util
    ${CMAKE_SOURCE_DIR}/Util/XMath
    ${CMAKE_SOURCE_DIR}/Util/XTL
    ${CMAKE_SOURCE_DIR}/Render/inc
    ${CMAKE_SOURCE_DIR}/Render/src
    ${CMAKE_SOURCE_DIR}/Render/3dx
    ${CMAKE_SOURCE_DIR}/Render/D3D
    ${CMAKE_SOURCE_DIR}/XLibs.Net/XUtil
    ${CMAKE_SOURCE_DIR}/XLibs.Net/XUtil/CRC
    ${CMAKE_SOURCE_DIR}/XLibs.Net/XUtil/Unicode
    ${CMAKE_SOURCE_DIR}/XLibs.Net/XZip
    ${CMAKE_SOURCE_DIR}/XLibs.Net/Profiler/Profiler
    ${CMAKE_SOURCE_DIR}/Units
    ${CMAKE_SOURCE_DIR}/Environment
    ${CMAKE_SOURCE_DIR}/Physics
    ${CMAKE_SOURCE_DIR}/Terra
    ${CMAKE_SOURCE_DIR}/Game
    ${CMAKE_SOURCE_DIR}/UserInterface
    ${CMAKE_SOURCE_DIR}/Network
    ${CMAKE_SOURCE_DIR}/Sound
)
# The full engine module set, the same order Game/CMakeLists.txt uses (MSVC
# static-library order: a library's dependencies come after it). Phase 3 pulls
# in the game-level modules (Game for CameraManager/Universe/RenderObjects,
# Environment, Water, UserInterface) for the scene graph and camera.
target_link_libraries(EditorEngine PRIVATE
    Util
    AI
    Terra
    Physics
    Sound
    Network
    Units
    Environment
    Water
    UserInterface
    VistaRender
    Render
    SDL3::SDL3
)

add_executable(SurMap5Qt
    src/main.cpp
    src/EditorApplication.h
    src/EditorApplication.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/RenderViewWidget.h
    src/RenderViewWidget.cpp
    src/dialogs/WorldNameDialog.h
    src/dialogs/WorldNameDialog.cpp
    src/dialogs/EnterNameDialog.h
    src/dialogs/EnterNameDialog.cpp
    src/dialogs/ProgressDialog.h
    src/dialogs/ProgressDialog.cpp
    src/dialogs/SelectWorldDialog.h
    src/dialogs/SelectWorldDialog.cpp
    # Phase 5 tools — engine-free behaviour classes (see src/editor/EditorTool.h)
    src/editor/EditorTool.h
    src/tools/ToolManager.h
    src/tools/ToolManager.cpp
    src/tools/SelectTool.h
    src/tools/SelectTool.cpp
    src/tools/TransformTool.h
    src/tools/TransformTool.cpp
    src/tools/MoveTool.h
    src/tools/MoveTool.cpp
    src/tools/RotateTool.h
    src/tools/RotateTool.cpp
    src/tools/ScaleTool.h
    src/tools/ScaleTool.cpp
)

# SurMap5Qt links EditorEngine (which carries the engine modules); Qt code
# itself never sees EngineIncludes flags. Platform/Window.cpp lives in the
# executable (as in Game) — it provides PlatformWindow::nativeHandle()/current()
# that the SDL render device asks for.
target_link_libraries(SurMap5Qt PRIVATE
    Qt6::Widgets
    EditorEngine
    SDL3::SDL3   # Platform/Window.cpp needs SDL3 headers; a clean import target
)

# Include roots for the editor's own sources: src/ for the dialogs' includes
# of "dialogs/..." and "editor/WorldList.h" (a path, not an engine flag).
target_include_directories(SurMap5Qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}
)

target_sources(SurMap5Qt PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/Window.cpp
)
