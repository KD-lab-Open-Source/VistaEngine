add_library(Physics STATIC
    CD/CD2D.cpp
    CD/CDConvex.cpp
    CD/CDSphere.cpp
    Crash/CrashSystem.cpp
    FormationController.cpp
    Geom.cpp
    Math/ConstraintHandler.cpp
    Math/ConstraintHandlerSimple.cpp
    Math/PhysicsMath.cpp
    Missile.cpp
    MultiBodyDispatcher.cpp
    NormalMap.cpp
    PathTracking.cpp
    RigidBody.cpp
    RigidBodyBase.cpp
    RigidBodyBox.cpp
    RigidBodyCar.cpp
    RigidBodyMissile.cpp
    RigidBodyNode.cpp
    RigidBodyPhysics.cpp
    RigidBodyUnit.cpp
    RigidBodyUnitRagDoll.cpp
    Rocket.cpp
    WhellController.cpp
    WindMap.cpp
)

target_link_libraries(Physics PUBLIC EngineIncludes Util)
