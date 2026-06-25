add_library(Network STATIC
    CommonEvents.cpp
    ConnectionDP.cpp
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

target_link_libraries(Network PUBLIC EngineIncludes Util)
