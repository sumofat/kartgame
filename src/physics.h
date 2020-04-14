#if !defined(PHYSICS_H)
#if OSX || IOS
//#define PHYSX_WARNINGS_SUPRESSED_CLANG 
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#endif

//#define _DEBUG
#include "PxPhysicsAPI.h"

#if OSX || IOS
#pragma clang diagnostic pop
#endif

using namespace physx;

struct Physics
{
};

struct PhysicsShapeSphere
{
    float radius;
    PxSphereGeometry state;
    PxShape* shape;
};

struct RigidBodyDynamic
{
    f3 p;
    PxRigidDynamic* state;
    float mass;
    bool is_kinematic;
};

struct PhysicsScene
{
    PxScene* state;
};

struct PhysicsMaterial
{
    PxMaterial* state;    
};

struct PhysicsFilterData
{
    uint32_t word0;
    uint32_t word1;
};

typedef PxFilterFlags (*FilterShaderCallback)(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
                                                  PxFilterObjectAttributes attributes1, PxFilterData filterData1,
                                                  PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);

namespace PhysicsCode
{
//Base functions
    void Init();
    bool Update(PhysicsScene* scenes,uint32_t scene_count,float dt);

    //Shape functions
    PhysicsShapeSphere CreateSphere(float radius,PhysicsMaterial material);

    //built in "shader" Callback/s
    PxFilterFlags DefaultFilterShader(
        PxFilterObjectAttributes attributes0, PxFilterData filterData0,
        PxFilterObjectAttributes attributes1, PxFilterData filterData1,
        PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);    

    RigidBodyDynamic CreateDynamicRigidbody(f3 p,PxShape* shape,bool is_kinematic);    
    PhysicsScene CreateScene(FilterShaderCallback callback);
    void SetSceneCallback(PhysicsScene* s,PxSimulationEventCallback* e);
    PhysicsMaterial CreateMaterial(float staticFriction/* 	the coefficient of static friction*/,
                                   float dynamicFriction/* 	the coefficient of dynamic friction*/,
                                   float restitution/* 	the coefficient of restitution */);
    void AddActorToScene(PhysicsScene scene, RigidBodyDynamic rb);
    void SetFilterData(PhysicsFilterData filter_data,PxShape* shape);        
    void DisableGravity(PxActor* actor,bool enable);
    void SetFlagForActor(PxActor* actor,PxActorFlag flag,bool state);
    void UpdateRigidBodyMassAndInertia(RigidBodyDynamic rbd,uint32_t density);
    void SetMass(RigidBodyDynamic rbd,float mass);
    void SetRestitutionCombineMode(PhysicsMaterial mat,physx::PxCombineMode::Enum mode);    
    void SetFrictionCombineMode(PhysicsMaterial mat,physx::PxCombineMode::Enum mode);
};

#define PHYSICS_H
#endif
