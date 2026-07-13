# The TextConverter/* sources are a separate standalone tool (its own
# TextConverter.vcproj); none appear in Core.vcproj, so they are not part of the
# engine Sound library and were dropped from this build.
set(SOUND_SOURCES
    StdAfx.cpp
    # No-op audio on every platform, Windows included: real C++ stubs for
    # SoundSystem/Sound/Channel/OggPlayer and the SND* functions.
    #
    # The DirectSound backend (SoundSystem.cpp, WaveFile.cpp, C3D.cpp,
    # HardwareBuffer.cpp, SoftwareBuffer.cpp, init.cpp) is not built anywhere. It is
    # 1990s DirectSound8 / mmio code that has never been through a 64-bit compiler,
    # and audio is silent on the other two platforms regardless — so there is nothing
    # to preserve by keeping Windows on a separate path. Those files stay in the tree
    # as the reference for the SDL3 audio backend, which will then land once, for
    # everyone (Track B, like the renderer did).
    SoundStub.cpp
)

add_library(Sound STATIC ${SOUND_SOURCES})
target_link_libraries(Sound PUBLIC EngineIncludes Util)
