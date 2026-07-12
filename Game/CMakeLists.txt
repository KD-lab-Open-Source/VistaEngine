set(GAME_SOURCES
    CDKey.cpp
    GameContext.cpp
    Runtime.cpp
    StdAfx.cpp
    Actions.cpp
    CameraManager.cpp
    CheatManager.cpp
    Conditions.cpp
    GameOptions.cpp
    GameOptionsSerialization.cpp
    IniFile.cpp
    MissionDescription.cpp
    Player.cpp
    PlayerStatistics.cpp
    RenderObjects.cpp
    ShowHead.cpp
    SoundApp.cpp
    SoundTrack.cpp
    StreamCommand.cpp
    TransparentTracking.cpp
    Universe.cpp
    # Core.vcproj engine source that happens to live in the TriggerEditor dir
    # (defines TriggerChain runtime methods load/save/buildLinks/serialize/...).
    ../TriggerEditor/TriggerExport.cpp

    # SDL3 windowing and input — the engine's only window, on every platform, and
    # the one the SDL GPU device claims its swapchain on.
    ${CMAKE_SOURCE_DIR}/Platform/Window.cpp
)

if(WIN32)
    list(APPEND GAME_SOURCES PlayBink.cpp)
else()
    # main() entry forwarding to WinMain + portable licensing stub.
    list(APPEND GAME_SOURCES PlatformStub.cpp)
    # TriggerEditor (kdw editor dialog) is Win32-only; no-op off-Windows.
    list(APPEND GAME_SOURCES ../TriggerEditor/TriggerEditorStub.cpp)
    # Out-of-line bodies for the force-included WindowsAPI.h shim (path/dir-iteration
    # helpers). Compiled into the executable itself so the single definition is
    # always present at the final link, regardless of static-archive order.
    list(APPEND GAME_SOURCES ${CMAKE_SOURCE_DIR}/Platform/WindowsAPI.cpp)
endif()

add_executable(Game ${GAME_SOURCES})
target_link_libraries(Game PRIVATE
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
    Render
    VistaRender
    SDL3::SDL3
)
