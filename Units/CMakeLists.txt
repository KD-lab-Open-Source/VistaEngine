add_library(Units STATIC
    AbnormalStateAttribute.cpp
    Animation.cpp
    AttributeCache.cpp
    AttributeSquad.cpp
    BaseUnit.cpp
    BaseUniverseObject.cpp
    CommandsQueue.cpp
    # Economic.cpp omitted: orphan (in neither Core.vcproj nor Game.vcproj) and
    # includes a non-existent "Config.h". Like AI/AITileMap.cpp.
    EffectController.cpp
    EnginePrm.cpp
    EnvironmentSimple.cpp
    GlobalAttributes.cpp
    Installer.cpp
    Inventory.cpp
    IronBuilding.cpp
    IronBullet.cpp
    IronLegion.cpp
    LabelObject.cpp
    MicroAI.cpp
    Nature.cpp
    Object3dxInterface.cpp
    Parameters.cpp
    RealUnit.cpp
    ShowChangeController.cpp
    ShowChangeParameters.cpp
    SoundAttribute.cpp
    Squad.cpp
    StateBase.cpp
    Triggers.cpp
    UnitActing.cpp
    UnitAttribute.cpp
    UnitCommand.cpp
    UnitEnvironment.cpp
    UnitID.cpp
    UnitInterface.cpp
    UnitItemInventory.cpp
    UnitItemResource.cpp
    UnitObjective.cpp
    UnitPad.cpp
    Weapon.cpp
    WeaponAttribute.cpp
    stdafx.cpp
)

target_include_directories(Units PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Units PUBLIC EngineIncludes Util Physics)
