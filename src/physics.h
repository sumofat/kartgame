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

struct PhysicsShapeMesh
{
    PxShape* state;
    PxTriangleMesh* tri_mesh;
};

struct PhysicsShapeBox
{
    f3 dim;
    f3 extents;
    PxBoxGeometry box;
    PxShape* state;
};

enum RigidBodyType
{
    rigidbody_type_none,    
    rigidbody_type_dynamic,
    rigidbody_type_kinematic,
    rigidbody_type_static,    
};

struct RigidBody
{
    f3 p;
    void* state;
    float mass;
    RigidBodyType  type;
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
    PhysicsShapeBox CreateBox(f3 dim,PhysicsMaterial material);
    void SetSimulationFilterData(PxShape* shape,u32 a,u32 b);
    void SetQueryFilterData(PxShape* shape,u32 a);
    
    //built in "shader" Callback/s
    PxFilterFlags DefaultFilterShader(
        PxFilterObjectAttributes attributes0, PxFilterData filterData0,
        PxFilterObjectAttributes attributes1, PxFilterData filterData1,
        PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);    

    RigidBody CreateDynamicRigidbody(f3 p,PxShape* shape,bool is_kinematic);
    RigidBody CreateStaticRigidbody(f3 p,PxShape* shape);
    void SetRigidBodyUserData(RigidBody body,void* data);
    
    PhysicsScene CreateScene(FilterShaderCallback callback);
    void SetSceneCallback(PhysicsScene* s,PxSimulationEventCallback* e);
    PhysicsMaterial CreateMaterial(float staticFriction/* 	the coefficient of static friction*/,
                                   float dynamicFriction/* 	the coefficient of dynamic friction*/,
                                   float restitution/* 	the coefficient of restitution */);
    void AddActorToScene(PhysicsScene scene, RigidBody rb);
    void SetFilterData(PhysicsFilterData filter_data,PxShape* shape);        
    void DisableGravity(PxActor* actor,bool enable);

    void UnlockRigidAll(RigidBody rbd);
    void UnlockRigidPosition(RigidBody rbd);
    void UnlockRigidRotation(RigidBody rbd);        
    void LockRigidPosition(RigidBody rbd);
    void LockRigidRotation(RigidBody rbd);
    
    void SetRigidDynamicLockFlags(RigidBody rbd,physx::PxRigidDynamicLockFlags flags);
    void SetRigidDynamicLockFlag(RigidBody rbd,physx::PxRigidDynamicLockFlag flag,bool value);    
    void SetFlagsForActor(PxActor* actor,PxActorFlag flags);
    void SetFlagForActor(PxActor* actor,PxActorFlag flag,bool state);
    void UpdateRigidBodyMassAndInertia(RigidBody rbd,uint32_t density);
    void SetMass(RigidBody rbd,float mass);
    void SetRestitutionCombineMode(PhysicsMaterial mat,physx::PxCombineMode::Enum mode);    
    void SetFrictionCombineMode(PhysicsMaterial mat,physx::PxCombineMode::Enum mode);

    PhysicsShapeMesh CreatePhyshicsMeshShape(void* vertex_data,u64 vertex_count,void* index_data,u64 index_count,u16 index_type,PhysicsMaterial material);
    void SetAngularDampening(RigidBody rb,f32 a);
    void AddTorqueForce(RigidBody rb,f3 dir);
    void AddRelativeTorqueForce(RigidBody rb,FMJ3DTrans* transform,f3 dir);
    void SetGlobalPoseP(RigidBody rb,f3 p);
    void SetGlobalPose(RigidBody rb,f3 p,quaternion r);
    FMJ3DTrans GetGlobalPose(RigidBody rb);
    void Simulate(PhysicsScene* scenes,uint32_t scene_count,float dt);
    void FetchResults(PhysicsScene* s,bool blocking);

};

#define PHYSICS_H
#endif
