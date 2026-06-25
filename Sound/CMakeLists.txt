set(SOUND_SOURCES
    SoundSystem.cpp
    StdAfx.cpp
    TextConverter/MessageBlock.cpp
    TextConverter/StdAfx.cpp
    TextConverter/TextConverter.cpp
    WaveFile.cpp
)

if(WIN32)
    list(APPEND SOUND_SOURCES
        C3D.cpp
        HardwareBuffer.cpp
        SoftwareBuffer.cpp
        TextConverter/Win2Dos.cpp
        init.cpp
    )
endif()

add_library(Sound STATIC ${SOUND_SOURCES})
target_link_libraries(Sound PUBLIC EngineIncludes Util)
