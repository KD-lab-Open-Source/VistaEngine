# The TextConverter/* sources are a separate standalone tool (its own
# TextConverter.vcproj); none appear in Core.vcproj, so they are not part of the
# engine Sound library and were dropped from this build.
set(SOUND_SOURCES
    StdAfx.cpp
    # The audio device, on every platform including Windows: miniaudio, reading through a
    # VFS onto the game's own .pak/file streams.
    AudioBackend.cpp
    SoundSystemMiniaudio.cpp
    # Still silent: Sound, Channel, OggPlayer and SND3DListener — the sample pool, the
    # voices, the music/voice streams and the 3D listener. They land slice by slice.
    SoundStub.cpp
    #
    # The DirectSound backend (SoundSystem.cpp, WaveFile.cpp, C3D.cpp, HardwareBuffer.cpp,
    # SoftwareBuffer.cpp, init.cpp) is not built anywhere. It is 1990s DirectSound8 / mmio
    # code that has never been through a 64-bit compiler, and it was silent on the other two
    # platforms regardless — so there was nothing to preserve by keeping Windows on it. Those
    # files stay in the tree as the reference for the miniaudio port, exactly as the D3D9
    # sources do for the renderer: read the original before porting a behaviour.
)

add_library(Sound STATIC ${SOUND_SOURCES})
target_link_libraries(Sound PUBLIC EngineIncludes Util PRIVATE miniaudio)
