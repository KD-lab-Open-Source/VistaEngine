add_library(Sound STATIC
    C3D.cpp
    HardwareBuffer.cpp
    SoftwareBuffer.cpp
    SoundSystem.cpp
    StdAfx.cpp
    TextConverter/MessageBlock.cpp
    TextConverter/StdAfx.cpp
    TextConverter/TextConverter.cpp
    TextConverter/Win2Dos.cpp
    WaveFile.cpp
    init.cpp
)

target_link_libraries(Sound PUBLIC EngineIncludes Util)
