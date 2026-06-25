add_executable(Game
    CDKey.cpp
    GameContext.cpp
    PlayBink.cpp
    Runtime.cpp
    StdAfx.cpp
)

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
