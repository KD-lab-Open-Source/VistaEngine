# CommonEvents.cpp, GS_interface.cpp and ConnectionDP.cpp are omitted as orphans:
# none appear in Core.vcproj or Game.vcproj (the build's sources of truth), and
# CommonEvents.cpp includes a non-existent "CommonEvents.h". Like AI/AITileMap.cpp.
set(NETWORK_SOURCES
    EventBufferDP.cpp
    ExternalTask.cpp
    GameCommands.cpp
    MissionDescriptionNet.cpp
    NetCenterStatic.cpp
    NetCommandBase.cpp
    P2P_interface2Th.cpp
    P2P_interfaceAnyTh.cpp
    PlayerStatisticsConversion.cpp
    StartGameParam.cpp
    UniverseX.cpp
    quantTimeStatistic.cpp
)

# No DirectPlay backend on any platform. P2P_interface{1Th,3Th,2ThDPF,Aux}.cpp call
# DirectPlay8Create and IDirectPlay8Peer, and Microsoft removed DirectPlay from the
# Windows SDK — so <dplay8.h> is gone even there, and those files no longer build
# anywhere. They stay in the tree as the reference for a real cross-platform
# networking backend (Track B, like the renderer); Network/dplay8.h declares the
# types their headers still name.
#
# NetCenterStub gives every platform the no-op PNetCenter methods those files
# otherwise define, so the game links. Networking is silent.
list(APPEND NETWORK_SOURCES NetCenterStub.cpp)

add_library(Network STATIC ${NETWORK_SOURCES})
target_link_libraries(Network PUBLIC EngineIncludes Util)
