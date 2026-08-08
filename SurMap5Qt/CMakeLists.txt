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
# wrapper, the per-frame draw. Compiled with the engine's flags and includes.
add_library(EditorEngine STATIC
    src/editor/EngineViewport.h
    src/editor/EngineViewport.cpp
)
# EngineIncludes brings the engine's include roots and (on Windows)
# _HAS_STD_BYTE=0 + /FIwindows.h — exactly what engine headers expect.
# PRIVATE: SurMap5Qt must not inherit any of it (Qt needs std::byte).
target_link_libraries(EditorEngine PRIVATE EngineIncludes)
target_link_libraries(EditorEngine PRIVATE
    # MSVC static-library order: dependencies after the library that needs them.
    Util
    Terra
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

# Just the include root for Platform/Window.h (a path, not an engine flag).
target_include_directories(SurMap5Qt PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_sources(SurMap5Qt PRIVATE
    ${CMAKE_SOURCE_DIR}/Platform/Window.cpp
)
