
namespace PhysicsCode
{
static PxDefaultErrorCallback gDefaultErrorCallback;
static PxDefaultAllocator gDefaultAllocatorCallback;

#define PVD_HOST "127.0.0.1"	//Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.

    physx::PxRigidStatic* test_rigid_body;
    physx::PxTriangleMeshGeometry test_mesh_geo;
    physx::PxPhysics* physics;

    float accumulator = 0.0;
    float step_size = 0;
    PxTolerancesScale scale{};
//    MyEventCallback* e;
    
    void Init()
    {
//Connect to Physx visual debugger
        PxFoundation* mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gDefaultErrorCallback);
        if(!mFoundation)
        {
            ASSERT(false);
        }
        accumulator = 0.0f;
        bool recordMemoryAllocations = true;

        PxPvd* mPvd = PxCreatePvd(*mFoundation);
        PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate(PVD_HOST, 5425, 100);
        //PxPvdTransport* transport = PxDefaultPvdFileTransportCreate("pvd");
        bool is_connected_to_pvd = mPvd->connect(*transport, PxPvdInstrumentationFlag::ePROFILE);
        if (!mPvd->isConnected())
        {
            //ASSERT(false);
        }

        if(!scale.isValid())
        {
            ASSERT(false);
        }

        physics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation,
		scale, recordMemoryAllocations);

        if (!physics)
            ASSERT(false);
    }
    
    PxFilterFlags DefaultFilterShader(
        PxFilterObjectAttributes attributes0, PxFilterData filterData0,
        PxFilterObjectAttributes attributes1, PxFilterData filterData1,
        PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
    {
        // let triggers through
        if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
        {
            pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
            return PxFilterFlag::eDEFAULT;
        }
        // generate contacts for all that were not filtered above
        pairFlags = PxPairFlag::eCONTACT_DEFAULT;
        // trigger the contact callback for pairs (A,B) where
        // the filtermask of A contains the ID of B and vice versa.
        if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
        {
            pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
            pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
            pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
            pairFlags |= PxPairFlag::eSOLVE_CONTACT;
            pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
        }
        return PxFilterFlag::eDEFAULT;
    }

    PhysicsShapeSphere CreateSphere(float radius,PhysicsMaterial material)
    {
        PhysicsShapeSphere result;
        PxSphereGeometry sphere(radius);
        result.state = sphere;
        result.radius = radius;
        result.shape = physics->createShape(sphere, *material.state,true);
        return result;
    }

    RigidBodyDynamic CreateDynamicRigidbody(f3 p,PxShape* shape,bool is_kinematic)
    {
        RigidBodyDynamic result;
        PxTransform pxt = PxTransform::PxTransform(p.x,p.y,p.z);
        PxRigidDynamic* actor = physics->createRigidDynamic(pxt);
        actor->attachShape(*shape);
        actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, is_kinematic);
        PxRigidBodyExt::updateMassAndInertia(*actor,1);
        
        result.mass = actor->getMass();
        result.is_kinematic = is_kinematic;
        result.state = actor;
        result.p = p;
        return result;
    }

    void SetMass(RigidBodyDynamic rbd,float mass)
    {
        rbd.state->setMass(mass);    
    }

    void SetRestitutionCombineMode(PhysicsMaterial mat,physx::PxCombineMode::Enum mode)
    {
        mat.state->setRestitutionCombineMode(mode);
    }
    
    void SetFrictionCombineMode(PhysicsMaterial mat,physx::PxCombineMode::Enum mode)
    {
        mat.state->setFrictionCombineMode(mode);
    }
    
    void UpdateRigidBodyMassAndInertia(RigidBodyDynamic rbd,uint32_t density)
    {
        PxRigidBodyExt::updateMassAndInertia(*rbd.state,1);
    }
    
    PhysicsScene CreateScene(FilterShaderCallback callback)
    {
        PhysicsScene result;
        PxSceneDesc scene_desc = PxSceneDesc(scale);
        if (!scene_desc.cpuDispatcher)
        {
            PxDefaultCpuDispatcher* mCpuDispatcher = PxDefaultCpuDispatcherCreate(0);
            if (!mCpuDispatcher)
                ASSERT(false);
            scene_desc.cpuDispatcher = mCpuDispatcher;
        }
        scene_desc.gravity = PxVec3(0, -9.36f, 0);
        scene_desc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;
        scene_desc.staticKineFilteringMode = PxPairFilteringMode::eKEEP;
        if (!scene_desc.filterShader)
            scene_desc.filterShader = callback;
        if(!scene_desc.isValid())
        {
            ASSERT(false);
        }
        result.state = physics->createScene(scene_desc);
        if(!result.state)
        {
            ASSERT(false);
        }
        return result;
    }

    void SetSceneCallback(PhysicsScene* s,PxSimulationEventCallback* e)
    {
        ASSERT(s);
        ASSERT(e);
        s->state->setSimulationEventCallback(e);
    }

    PhysicsMaterial CreateMaterial(float staticFriction/* 	the coefficient of static friction*/,
                                   float dynamicFriction/* 	the coefficient of dynamic friction*/,
                                   float restitution/* 	the coefficient of restitution */)
    {
        PhysicsMaterial result;
        result.state = physics->createMaterial(staticFriction, dynamicFriction, restitution);
        return result;
    }

    void AddActorToScene(PhysicsScene scene, RigidBodyDynamic rb)
    {
        ASSERT(scene.state);
        ASSERT(rb.state);
        scene.state->addActor(*rb.state);
    }

    void SetFilterData(PhysicsFilterData filter_data,PxShape* shape)
    {
        ASSERT(shape);
        PxFilterData filterData;
        filterData.word0 = filter_data.word0;//FilterGroup::kart; // word0 = own ID
        filterData.word1 = filter_data.word1;//FilterGroup::track;  // word1 = ID mask to filter pairs that trigger a
        shape->setSimulationFilterData(filterData);
    }

/*
    void SetFlagsForActor(PxActor* actor,PxActorFlag flags,bool state)
    {
        actor->setActorFlags(flags, state);            
    }
*/

    void SetFlagForActor(PxActor* actor,PxActorFlag::Enum flag,bool state)
    {
        actor->setActorFlag(flag, state);            
    }
    
    void DisableGravity(PxActor* actor,bool enable)
    {
        SetFlagForActor(actor,PxActorFlag::eDISABLE_GRAVITY,enable);
    }

    bool Update(PhysicsScene* scenes,uint32_t scene_count,float dt)
    {
        
        step_size = 1.0f / 60.0f;

        accumulator  += dt;
        if(accumulator < step_size)
            return false;

        accumulator -= step_size;
        for(int i = 0;i < scene_count;++i)
        {
            PhysicsScene* s = scenes + i;
            ASSERT(s);
            s->state->simulate(dt);
            s->state->fetchResults(true);
        }
        //mScene->simulate(step_size);
        return true;
    }

};

