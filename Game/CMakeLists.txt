set(GAME_SOURCES
    CDKey.cpp
    GameContext.cpp
    Runtime.cpp
    StdAfx.cpp
)

if(WIN32)
    list(APPEND GAME_SOURCES PlayBink.cpp)
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
    SDL3::SDL3
)
