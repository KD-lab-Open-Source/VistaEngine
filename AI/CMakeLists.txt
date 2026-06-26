add_library(AI STATIC
    AiBuilding.cpp
    # AITileMap.cpp omitted: orphan code that #includes a non-existent
    # "ClusterFind.h" and was never part of the original MSVC project.
    PFTrap.cpp
)

target_link_libraries(AI PUBLIC EngineIncludes Util)
