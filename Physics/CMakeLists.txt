add_library(Physics STATIC
    CD/CD2D.cpp
    CD/CDConvex.cpp
    CD/CDSphere.cpp
    # Crash/CrashSystem.cpp and RigidBodyNode.cpp are real (listed in Core.vcproj)
    # but depend on the Render layer (Render/3dx/Simply3dx.h, Render/src/Scene.h,
    # Game/RenderObjects.h) — the D3D9 renderer that is excluded for now (Track B).
    # Restore these once the SDL GPU render backend is in place.
    #
    # Missile.cpp, MultiBodyDispatcher.cpp, Rocket.cpp and PathTracking.cpp are
    # omitted as orphans: none are listed in Core.vcproj (the build's source of
    # truth), and they reference a pre-refactor `RigidBody` class plus headers
    # ("RigidBody.h", "CD/CD-Sphere.h") that exist nowhere. Like AI/AITileMap.cpp.
    FormationController.cpp
    Geom.cpp
    Math/ConstraintHandler.cpp
    Math/ConstraintHandlerSimple.cpp
    Math/PhysicsMath.cpp
    NormalMap.cpp
    RigidBody.cpp
    RigidBodyBase.cpp
    RigidBodyBox.cpp
    RigidBodyCar.cpp
    RigidBodyMissile.cpp
    RigidBodyPhysics.cpp
    RigidBodyUnit.cpp
    RigidBodyUnitRagDoll.cpp
    WhellController.cpp
    WindMap.cpp
)

target_link_libraries(Physics PUBLIC EngineIncludes Util)
