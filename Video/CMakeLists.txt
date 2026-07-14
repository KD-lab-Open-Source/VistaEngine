# The video decoder and player: Bink 1 (.bik) and the raw-DIB .avi textures, through ffmpeg.
# What ffmpeg we build, and why it is only five components of it, is in cmake/FFmpeg.cmake.
set(VIDEO_SOURCES
    # The decoder: demux + decode to BGRA, read through the game's VFS with a custom AVIOContext.
    # Serves both the player below and the animated .avi textures (Render/src/FileImage.cpp).
    VideoFile.cpp
    # The player: the soundtrack through miniaudio, the picture pulled against the audio clock.
    VideoPlayer.cpp
)

add_library(Video STATIC ${VIDEO_SOURCES})

# Sound, for the audio device the soundtrack plays on (audio::engine/group). That is what makes
# Render -> Video -> Sound an edge, which is new but not a cycle: Sound knows nothing of either.
target_link_libraries(Video PUBLIC EngineIncludes Util PRIVATE Sound miniaudio FFmpeg)

# ffmpeg is an ExternalProject, so its headers do not exist until it has been built.
add_dependencies(Video ffmpeg_external)
