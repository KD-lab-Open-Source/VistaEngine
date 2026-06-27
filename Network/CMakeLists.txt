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

if(WIN32)
    # The DirectPlay 8 networking backend. Stubbed off-Windows (the fake dplay8.h
    # lets the Network headers parse, but these .cpp call IDirectPlay8Peer methods
    # and DirectPlay8Create). Real cross-platform networking is a later
    # replacement (Track B, like the renderer / DirectSound audio).
    list(APPEND NETWORK_SOURCES
        P2P_interface1Th.cpp
        P2P_interface2ThDPF.cpp
        P2P_interface3Th.cpp
        P2P_interfaceAux.cpp
    )
else()
    # Off-Windows: no-op PNetCenter methods otherwise defined by the gated
    # DirectPlay files above, so the game links (silent networking).
    list(APPEND NETWORK_SOURCES NetCenterStub.cpp)
endif()

add_library(Network STATIC ${NETWORK_SOURCES})
target_link_libraries(Network PUBLIC EngineIncludes Util)
