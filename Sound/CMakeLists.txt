# The TextConverter/* sources are a separate standalone tool (its own
# TextConverter.vcproj); none appear in Core.vcproj, so they are not part of the
# engine Sound library and were dropped from this build.
set(SOUND_SOURCES
    StdAfx.cpp
)

if(NOT WIN32)
    # No-op audio backend (real C++ stubs for SoundSystem/Sound/Channel/OggPlayer
    # + the SND* functions). The DirectSound backend below is WIN32-only.
    list(APPEND SOUND_SOURCES SoundStub.cpp)
endif()

if(WIN32)
    # The DirectSound / Windows-multimedia backend. Stubbed out on other
    # platforms (no-op audio); real cross-platform audio is the eventual SDL3
    # audio replacement (Track B, like the renderer). The fake dsound.h /
    # mmsystem.h headers exist only so the Sound headers parse for callers.
    list(APPEND SOUND_SOURCES
        SoundSystem.cpp     # DirectSound: DirectSoundCreate8, IDirectSound8, ...
        WaveFile.cpp        # mmio: mmioOpen/Read/Descend, MMCKINFO, ...
        C3D.cpp
        HardwareBuffer.cpp
        SoftwareBuffer.cpp
        init.cpp
    )
endif()

add_library(Sound STATIC ${SOUND_SOURCES})
target_link_libraries(Sound PUBLIC EngineIncludes Util)
