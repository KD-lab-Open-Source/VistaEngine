add_library(AI STATIC
    AiBuilding.cpp
    AITileMap.cpp
    PFTrap.cpp
)

target_link_libraries(AI PUBLIC EngineIncludes Util)
