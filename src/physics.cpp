
namespace PhysicsCode
{
static PxDefaultErrorCallback gDefaultErrorCallback;
static PxDefaultAllocator gDefaultAllocatorCallback;

#define PVD_HOST "127.0.0.1"	//Set this to the IP address of the system running the PhysX Visual Debugger that you want to connect to.

    physx::PxRigidStatic* test_rigid_body;
    physx::PxTriangleMeshGeometry test_mesh_geo;
    physx::PxPhysics* physics;
    physx::PxCooking* cooking;

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

        cooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, PxCookingParams(scale));
        if (!cooking)
            ASSERT(false);

	
        PxCookingParams params(scale);
        // disable mesh cleaning - perform mesh validation on development configurations
        params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
        // disable edge precompute, edges are set for each triangle, slows contact generation
        params.meshPreprocessParams |= PxMeshPreprocessingFlag::eDISABLE_ACTIVE_EDGES_PRECOMPUTE;
        // lower hierarchy for internal mesh
        //params.meshCookingHint = PxMeshCookingHint::eCOOKING_PERFORMANCE;
        cooking->setParams(params);
        
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

    PhysicsShapeBox CreateBox(f3 dim,PhysicsMaterial material)
    {
        PhysicsShapeBox result;
        f3 d = f3_div_s(dim,2.0f);
        PxBoxGeometry box(d.x,d.y,d.z);
        result.box = box;
        result.dim = dim;
        result.extents = d;
        ASSERT(box.isValid());
        result.state = physics->createShape(box, *material.state,true);
        return result;
    }

    void SetSimulationFilterData(PxShape* shape,u32 a,u32 b)
    {
        PxFilterData filterData;
        filterData.word0 = a; // word0 = own ID
        filterData.word1 = b;  // word1 = ID mask to filter pairs that trigger a
        shape->setSimulationFilterData(filterData);
    }

    void SetRigidBodyUserData(RigidBody body,void* data)
    {
        PxActor* a = (PxActor*)body.state;
        a->userData = data;
    }
    
    RigidBody CreateDynamicRigidbody(f3 p,PxShape* shape,bool is_kinematic)
    {
        RigidBody result;
        PxTransform pxt = PxTransform::PxTransform(p.x,p.y,p.z);
        PxRigidDynamic* actor = physics->createRigidDynamic(pxt);
        actor->attachShape(*shape);
        actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, is_kinematic);
        PxRigidBodyExt::updateMassAndInertia(*actor,1);

        result.mass = actor->getMass();
        result.type = rigidbody_type_kinematic;
        result.state = (void*)actor;
        result.p = p;
        return result;
    }

    RigidBody CreateStaticRigidbody(f3 p,PxShape* shape)
    {
        RigidBody result;
        PxTransform pxt = PxTransform::PxTransform(p.x,p.y,p.z);
        PxRigidStatic* actor = physics->createRigidStatic(pxt);
        actor->attachShape(*shape);
        result.state = (void*)actor;
        result.p = p;
        result.type = rigidbody_type_static;
        return result;
    }

    void SetMass(RigidBody rbd,float mass)
    {
        ASSERT(rbd.state);
        ASSERT(rbd.type == rigidbody_type_kinematic ||  rbd.type == rigidbody_type_dynamic);        
        ((PxRigidDynamic*)rbd.state)->setMass(mass);    
    }

    void SetRestitutionCombineMode(PhysicsMaterial mat,physx::PxCombineMode::Enum mode)
    {
        mat.state->setRestitutionCombineMode(mode);
    }
    
    void SetFrictionCombineMode(PhysicsMaterial mat,physx::PxCombineMode::Enum mode)
    {
        mat.state->setFrictionCombineMode(mode);
    }
    
    void UpdateRigidBodyMassAndInertia(RigidBody rbd,uint32_t density)
    {
        ASSERT(rbd.state);
        ASSERT(rbd.type == rigidbody_type_kinematic ||  rbd.type == rigidbody_type_dynamic);
        PxRigidBodyExt::updateMassAndInertia(*((PxRigidDynamic*)rbd.state),1);
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

    void AddActorToScene(PhysicsScene scene, RigidBody rb)
    {
        ASSERT(scene.state);
        ASSERT(rb.state);
        PxActor* actor = (PxActor*)rb.state;
        scene.state->addActor(*actor);
    }

    void SetFilterData(PhysicsFilterData filter_data,PxShape* shape)
    {
        ASSERT(shape);
        PxFilterData filterData;
        filterData.word0 = filter_data.word0;//FilterGroup::kart; // word0 = own ID
        filterData.word1 = filter_data.word1;//FilterGroup::track;  // word1 = ID mask to filter pairs that trigger a
        shape->setSimulationFilterData(filterData);
    }

    void SetFlagsForActor(PxActor* actor,PxActorFlag::Enum flags)
    {
        actor->setActorFlags(flags);            
    }

    void SetFlagForActor(PxActor* actor,PxActorFlag::Enum flag,bool state)
    {
        actor->setActorFlag(flag, state);            
    }

    void UnlockRigidAll(RigidBody rbd)
    {
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X,false);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y,false);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z,false);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X,false);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y,false);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z,false);        
    }
    
    void UnlockRigidPosition(RigidBody rbd)
    {
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X,false);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y,false);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z,false);
    }
    
    void LockRigidPosition(RigidBody rbd)
    {
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X,true);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y,true);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z,true);        
    }

    void UnLockRigidRotation(RigidBody rbd)
    {
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X,false);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y,false);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z,false);                
    }

    void LockRigidRotation(RigidBody rbd)
    {
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X,true);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y,true);
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z,true);                
    }
    
    void SetRigidDynamicLockFlag(RigidBody rbd,physx::PxRigidDynamicLockFlag::Enum flag,bool value)
    {
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlag(flag,value);            
    }
    
    void SetRigidDynamicLockFlags(RigidBody rbd,physx::PxRigidDynamicLockFlags flags)
    {
        ((PxRigidDynamic*)rbd.state)->setRigidDynamicLockFlags(flags);            
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

    PhysicsShapeMesh CreatePhyshicsMeshShape(void* vertex_data,u64 vertex_count,void* index_data,u64 index_count,u16 index_type,PhysicsMaterial material)
    {
        PhysicsShapeMesh result = {};
        PxTriangleMeshDesc tmd = PxTriangleMeshDesc();
        PxBoundedData v_data = PxBoundedData();
        v_data.stride = sizeof(PxVec3);
        int mesh_index = 0;
        v_data.data  = vertex_data;
        v_data.count = vertex_count;
        if(index_data)
        {
            PxBoundedData e_data = PxBoundedData();
            ASSERT(index_type < 3);
            if(index_type == 2)//16 bit
            {
                e_data.stride = sizeof(PxU16) * 3;
                e_data.data = index_data;
                e_data.count = index_count / 3;
                tmd.flags = PxMeshFlag::Enum::e16_BIT_INDICES;
            }
            else if(index_type == 1)//32 bit
            {
                e_data.stride = sizeof(PxU32) * 3;
                e_data.data = index_data;
                e_data.count = index_count / 3;
            }
            else
            {
                //NEED VALID INDEX TYPE
                ASSERT(false);
            }
            tmd.triangles = e_data;        
        }
        else
        {
            ASSERT(false);
        }
	
        tmd.points = v_data;

        if (!tmd.isValid())
            ASSERT(false);
	
        // mesh should be validated before cooked without the mesh cleaning
        bool res = cooking->validateTriangleMesh(tmd);
        PX_ASSERT(res);
        if(!res)
        {
//n            ASSERT(false);
        }
        
        PxTriangleMesh* aTriangleMesh = cooking->createTriangleMesh(tmd,physics->getPhysicsInsertionCallback());
        if(!aTriangleMesh)
        {
            ASSERT(false);
        }
        result.tri_mesh = aTriangleMesh;
        physx::PxTriangleMeshGeometry mesh_geo = PxTriangleMeshGeometry(aTriangleMesh);
        result.state = physics->createShape(mesh_geo,&material.state,true);

        //TODO(Ray):Add support for mesh convex geometry types. 
/*
        PxConvexMeshGeometry convexGeom = PxConvexMeshGeometry(convexBox);
        convexShape = PxRigidActorExt::createExclusiveShape(*meshActor, convexGeom,
        defaultMaterial);
*/
        
        return result;        
    }

    void AddTorqueForce(RigidBody rb,f3 dir)
    {
        PxVec3 v3 = {dir.x,dir.y,dir.z};
        ((PxRigidBody*)rb.state)->addTorque(v3);        
    }

    void AddRelativeTorqueForce(RigidBody rb,FMJ3DTrans* transform,f3 dir)
    {
        f3 local_torque = fmj_3dtrans_world_to_local_dir(transform,dir);
        PxVec3 v3 = {local_torque.x,local_torque.y,local_torque.z};        
        ((PxRigidBody*)rb.state)->addTorque(v3);
    }
    
};






