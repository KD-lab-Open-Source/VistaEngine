# The TextConverter/* sources are a separate standalone tool (its own
# TextConverter.vcproj); none appear in Core.vcproj, so they are not part of the
# engine Sound library and were dropped from this build.
set(SOUND_SOURCES
    StdAfx.cpp
    # The audio device, on every platform including Windows: miniaudio, reading through a
    # VFS onto the game's own .pak/file streams.
    AudioBackend.cpp
    # SoundSystem / Sound / Channel / SND3DListener: the effects, and where they are in the world.
    SoundSystemMiniaudio.cpp
    # OggPlayer: the music, the speech and the cutscene audio, streamed.
    OggPlayerMiniaudio.cpp
    # Vorbis, which miniaudio does not decode for itself.
    VorbisDecoder.cpp
    # stb_vorbis, from miniaudio's own extras. Built as C — it is C, and the engine's
    # force-included WindowsAPI.h is C++-only, so it compiles clean.
    ${miniaudio_SOURCE_DIR}/extras/stb_vorbis.c
    #
    # The DirectSound backend (SoundSystem.cpp, WaveFile.cpp, C3D.cpp, HardwareBuffer.cpp,
    # SoftwareBuffer.cpp, init.cpp) is not built anywhere, and neither is XLibs.Net/OGG/PlayOgg.
    # It is 1990s DirectSound8 / mmio code that has never been through a 64-bit compiler, and it
    # was silent on the other two platforms regardless — so there was nothing to preserve by
    # keeping Windows on it. Those files stay in the tree as the reference for the miniaudio port,
    # exactly as the D3D9 sources do for the renderer: read the original before porting a
    # behaviour. See Documents/Sound-PORTING.md.
)

add_library(Sound STATIC ${SOUND_SOURCES})
target_link_libraries(Sound PUBLIC EngineIncludes Util PRIVATE miniaudio)

# stb_vorbis.c lives in miniaudio's extras rather than on its include path.
target_include_directories(Sound PRIVATE ${miniaudio_SOURCE_DIR}/extras)
