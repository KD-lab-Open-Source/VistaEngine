add_library(Environment STATIC
    ActionsEnvironmental.cpp
    Anchor.cpp
    ChainLightningController.cpp
    Environment.cpp
    EnvironmentColors.cpp
    SourceBase.cpp
    SourceBlast.cpp
    SourceCameraShaking.cpp
    SourceDeleteGrass.cpp
    SourceFlash.cpp
    SourceFlock.cpp
    SourceImpulse.cpp
    SourceLight.cpp
    SourceLightning.cpp
    SourceManager.cpp
    SourceShield.cpp
    SourceTeleport.cpp
    SourceTerTool.cpp
    SourceTornado.cpp
    SourceZone.cpp
    Sources.cpp
)

target_link_libraries(Environment PUBLIC EngineIncludes Util Units)
