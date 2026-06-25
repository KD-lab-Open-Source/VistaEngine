set(NETWORK_SOURCES
    CommonEvents.cpp
    EventBufferDP.cpp
    ExternalTask.cpp
    GS_interface.cpp
    GameCommands.cpp
    MissionDescriptionNet.cpp
    NetCenterStatic.cpp
    NetCommandBase.cpp
    P2P_interface1Th.cpp
    P2P_interface2Th.cpp
    P2P_interface2ThDPF.cpp
    P2P_interface3Th.cpp
    P2P_interfaceAnyTh.cpp
    P2P_interfaceAux.cpp
    PlayerStatisticsConversion.cpp
    StartGameParam.cpp
    UniverseX.cpp
    quantTimeStatistic.cpp
)

if(WIN32)
    list(APPEND NETWORK_SOURCES ConnectionDP.cpp)
endif()

add_library(Network STATIC ${NETWORK_SOURCES})
target_link_libraries(Network PUBLIC EngineIncludes Util)
