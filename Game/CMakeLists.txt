set(GAME_SOURCES
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

# Not built on any platform:
#   PlayBink.cpp draws Bink frames through d3d9.h / d3dx9.h, which went with the D3D9
#   backend. Nothing constructs a PlayBink (GameShell.cpp only includes the header),
#   so there is nothing to stub — video playback needs a real port.
#   CDKey.cpp is the CD-key check. It links verifier.lib, a 32-bit VC7.1 import
#   library that an x64 build cannot use, and Runtime.cpp's call to it has been
#   commented out for years. There is no key check in this build.
list(APPEND GAME_SOURCES
    # main() forwarding to WinMain, on Windows too: add_executable() has no WIN32
    # flag, so the linker looks for main() there as well.
    PlatformStub.cpp
    # The TriggerEditor dialog is built on kdw, which nothing builds now.
    ../TriggerEditor/TriggerEditorStub.cpp
    # Out-of-line bodies for the force-included WindowsAPI.h shim (path/dir-iteration
    # helpers). Compiled into the executable itself so the single definition is always
    # present at the final link, regardless of static-archive order. The file compiles
    # to nothing on Windows, where the real API is used.
    ${CMAKE_SOURCE_DIR}/Platform/WindowsAPI.cpp
)

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

if(WIN32)
    # comctl32, for _TrackMouseEvent: Runtime.cpp's window procedure asks to be told when
    # the cursor leaves the window. (user32 exports the plain TrackMouseEvent; the
    # underscored one, which this code calls, is comctl32's.)
    target_link_libraries(Game PRIVATE comctl32)
endif()
