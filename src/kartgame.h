#if !defined(KART_GAME_H)
#include "carframe.h"
struct GameState
{
    u32  current_player_id;
    u32 is_phyics_active;
};

GameState game_state;
PhysicsScene scene;

u64 track_physics_id;
RigidBody track_rbd;
FMJSceneObject* track_so;
CarFrameBuffer car_frame_buffer;
CarFrame player_car;
FMJSceneObject track_physics_so_ = {};

PxFilterFlags CarFrameFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    // let triggers through
    if(PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
    {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
    }
    // generate contacts for all that were not filtered above
    pairFlags = PxPairFlag::eCONTACT_DEFAULT;

    // trigger the contact callback for pairs (A,B) where
    // the filtermask of A contains the ID of B and vice versa.
    if((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
    {
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
		pairFlags |= PxPairFlag::eSOLVE_CONTACT;
		pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
    }
    return PxFilterFlag::eDEFAULT;
}

class GamePiecePhysicsCallback : public PxSimulationEventCallback
{
	void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override
	{
		int a = 0;
	}
	void onWake(PxActor** actors, PxU32 count) override
	{
		int a = 0;
	}
	void onSleep(PxActor** actors, PxU32 count) override
	{
		int a = 0;
	}
	void onTrigger(PxTriggerPair* pairs, PxU32 count) override
	{
		int a = 0;
	}
	void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override
	{
		int a = 0;
	}

	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override
	{

        const PxU32 bufferSize = 64;
        PxContactPairPoint contacts[bufferSize];
        
		for (PxU32 i = 0; i < nbPairs; i++)
		{
			const PxContactPair& cp = pairs[i];
            u64 k_i = (u64)pairHeader.actors[0]->userData;
            FMJSceneObject* so = fmj_stretch_buffer_check_out(FMJSceneObject,&asset_ctx.scene_objects,k_i);
            
            u64 k2_i = (u64)pairHeader.actors[1]->userData;
            FMJSceneObject* so2 = fmj_stretch_buffer_check_out(FMJSceneObject,&asset_ctx.scene_objects,k2_i);

            PxU32 nbContacts = pairs[i].extractContacts(contacts, bufferSize);
            PxVec3 normal = contacts[0].normal;
            f3 n = f3_create(normal.x,normal.y,normal.z);                            
            f3 v = {0};
            CarFrame* carframe;
            GOHandle handle;
            if((GameObjectType*)so->data == (GameObjectType*)go_type_kart)
            {
                handle = fmj_anycache_get(GOHandle,&asset_ctx.so_to_go,so);
                carframe = fmj_stretch_buffer_check_out(CarFrame,handle.buffer,handle.index);
                v =  carframe->v;
//                fmj_stretch_buffer_check_in(handle.buffer);
            }
            else if((GameObjectType*)so2->data == (GameObjectType*)go_type_kart)
            {
                handle = fmj_anycache_get(GOHandle,&asset_ctx.so_to_go,so);
                carframe = fmj_stretch_buffer_check_out(CarFrame,handle.buffer,handle.index);
                v = carframe->v;
//                fmj_stretch_buffer_check_in(handle.buffer);                
            }
            else
            {
                ASSERT(false);
            }
            f3 rv = f3_reflect(v,n);

            if(isnan(rv.x))
            {
                ASSERT(false);
            }
            
            carframe->v = rv;

            fmj_stretch_buffer_check_in(handle.buffer);                            
            
//            for(PxU32 j=0; j < nbContacts; j++)
            {
//                PxVec3 point = contacts[j].position;
//                PxVec3 impulse = contacts[j].impulse;
//                PxVec3 normal = contacts[j].normal;
//                PxU32 internalFaceIndex0 = contacts[j].internalFaceIndex0;
//                PxU32 internalFaceIndex1 = contacts[j].internalFaceIndex1;

            }            
            
		}
	}
};

void kart_game_setup(PlatformState* ps,RenderCamera rc,AssetTables* asset_tables,FMJMemoryArena permanent_strings,FMJMemoryArena temp_strings)
{
    //BEGIN Setup physics stuff
    physics_material = PhysicsCode::CreateMaterial(0.5f, 0.5f, 0.5f);
    PhysicsCode::SetRestitutionCombineMode(physics_material,physx::PxCombineMode::eMIN);
    PhysicsCode::SetFrictionCombineMode(physics_material,physx::PxCombineMode::eMIN);
    scene = PhysicsCode::CreateScene(CarFrameFilterShader);
    GamePiecePhysicsCallback* e = new GamePiecePhysicsCallback();
    PhysicsCode::SetSceneCallback(&scene, e);
    //END PHYSICS SETUP

//game object setup 
    quaternion def_r = f3_axis_angle(f3_create(0,0,1),0);

    f2  top_right_screen_xy = f2_create(ps->window.dim.x,ps->window.dim.y);
    f2  bottom_left_xy = f2_create(0,0);

    f3 max_screen_p = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,top_right_screen_xy,0);
    f3 lower_screen_p = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,bottom_left_xy,0);


    
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/Box.glb",color_render_material_mesh.id);    
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/Fox.glb",vu_render_material_mesh.id);
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/Lantern.glb",color_render_material_mesh.id);
    FMJRenderMaterial track_material = fmj_asset_get_material_by_id(&asset_ctx,2);
    FMJRenderMaterial kart_material = fmj_asset_get_material_by_id(&asset_ctx,2);    
    FMJAssetModelLoadResult track_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/track.glb",track_material.id);
    FMJAssetModelLoadResult kart_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/kart.glb",kart_material.id);        
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/Duck.glb",color_render_material_mesh.id);
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/Avocado.glb",color_render_material_mesh.id);//scale is messed up and uvs    
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/BarramundiFish.glb",color_render_material_mesh.id);//    
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/BoxTextured.glb",color_render_material_mesh.id);
    
    //FMJAssetModel test_model = test_model_result.model;
    //fmj_asset_upload_model(&asset_tables,&asset_ctx,&duck_model_result.model);

    u64 track_instance_id = fmj_asset_create_model_instance(&asset_ctx,&track_model_result);
    u64 kart_instance_id = fmj_asset_create_model_instance(&asset_ctx,&kart_model_result);    

    FMJ3DTrans track_trans;
    fmj_3dtrans_init(&track_trans);
    track_trans.p = f3_create(0,0,0);
    track_trans.r = f3_axis_angle(f3_create(0,0,1),0);
    
    FMJ3DTrans kart_trans;
    fmj_3dtrans_init(&kart_trans);
    kart_trans.p = f3_create(0,0.4f,-1);    
    quaternion start_r = kart_trans.r;
//    AddModelToSceneObjectAsChild(&asset_ctx,root_node_id,kart_instance_id,kart_trans);
    
    FMJ3DTrans duck_trans;
    fmj_3dtrans_init(&duck_trans);
    duck_trans.p = f3_create(-10,0,0);

    fmj_3dtrans_init(&duck_trans);
    duck_trans.p = f3_create(10,0,0);

    fmj_3dtrans_init(&duck_trans);
    duck_trans.p = f3_create(0,8,0);
    
    track_so = fmj_stretch_buffer_check_out(FMJSceneObject,&asset_ctx.scene_objects,track_instance_id);
    track_so->name = fmj_string_create("track so",asset_ctx.perm_mem);
    track_so->data = (u32*)go_type_kart;        

    FMJSceneObject* kart_so = fmj_stretch_buffer_check_out(FMJSceneObject,&asset_ctx.scene_objects,kart_instance_id);
    kart_so->name = fmj_string_create("kart so",asset_ctx.perm_mem);
    
    u64 mesh_id;
    u64 kart_mesh_id;
    if(!fmj_asset_get_mesh_id_by_name("track",&asset_ctx,track_so,&mesh_id))
    {
        ASSERT(false);    
    }
    
    if(!fmj_asset_get_mesh_id_by_name("kart",&asset_ctx,kart_so,&kart_mesh_id))
    {
        ASSERT(false);    
    }
    
    FMJAssetMesh track_mesh = fmj_stretch_buffer_get(FMJAssetMesh,&asset_ctx.asset_tables->meshes,mesh_id);
    FMJAssetMesh track_collision_mesh = track_mesh;
    PhysicsShapeMesh track_physics_mesh = CreatePhysicsMeshShape(&track_mesh,physics_material);
    PhysicsCode::SetQueryFilterData((PxShape*)track_physics_mesh.state,(u32)go_type_track);    
    PhysicsCode::SetSimulationFilterData((PxShape*)track_physics_mesh.state,go_type_track,0xFF);
            
	track_collision_mesh.vertex_data   = (f32*)track_physics_mesh.tri_mesh->getVertices();
	track_collision_mesh.vertex_count  = track_physics_mesh.tri_mesh->getNbVertices() * 3;

    physx::PxTriangleMeshFlags mesh_flags = track_physics_mesh.tri_mesh->getTriangleMeshFlags();
    if(mesh_flags & PxTriangleMeshFlag::Enum::e16_BIT_INDICES)
    {
        track_collision_mesh.index_component_size = fmj_asset_index_component_size_16;
        track_collision_mesh.index_16_data = (u16*)track_physics_mesh.tri_mesh->getTriangles();
        track_collision_mesh.index16_count = track_physics_mesh.tri_mesh->getNbTriangles() * 3;
        track_collision_mesh.index_16_data_size = track_collision_mesh.index16_count * sizeof(u16);        
    }
    else
    {
        track_collision_mesh.index_component_size = fmj_asset_index_component_size_32;
        track_collision_mesh.index_32_data = (u32*)track_physics_mesh.tri_mesh->getTriangles();
        track_collision_mesh.index32_count = track_physics_mesh.tri_mesh->getNbTriangles() * 3;
        track_collision_mesh.index_32_data_size = track_collision_mesh.index32_count * sizeof(u32);                
    }

    u64 tcm_id = fmj_stretch_buffer_push(&asset_ctx.asset_tables->meshes,&track_collision_mesh);
    fmj_asset_upload_meshes(&asset_ctx,f2_create(tcm_id,tcm_id));

    track_physics_so_.name = fmj_string_create("Track Physics",asset_ctx.perm_mem);
    track_physics_so_.transform = kart_trans;
    track_physics_so_.children.buffer = fmj_stretch_buffer_init(1,sizeof(u64),8);
    track_physics_so_.m_id = track_so->m_id;
    track_physics_so_.data = 0;
    track_physics_so_.type = 1;
    track_physics_so_.primitives_range = f2_create_f(tcm_id);
    
    track_physics_id = fmj_stretch_buffer_push(&asset_ctx.scene_objects,&track_physics_so_);
    
    AddModelToSceneObjectAsChild(&asset_ctx,scene_manager.root_node_id,track_instance_id,track_physics_so_.transform);

    PhysicsShapeBox phyx_box_shape = PhysicsCode::CreateBox(f3_create(1.2f,0.2f,1.2f),physics_material);
        
    track_rbd = PhysicsCode::CreateStaticRigidbody(track_trans.p,track_physics_mesh.state);
    u64* instance_id_ptr = (u64*)track_instance_id;
    PhysicsCode::SetRigidBodyUserData(track_rbd,instance_id_ptr);
    PhysicsCode::AddActorToScene(scene, track_rbd);    
    
//end game object setup

    //game global vars
    CarFrameDescriptorBuffer car_descriptor_buffer;
    CarFrameDescriptor d = {};
    d.mass = 400;
    d.max_thrust = 15000;
    d.maximum_wheel_angle = 33;//degrees
    //d.count = 1;
    d.physics_material = physics_material;
    d.parent_id = scene_manager.root_node_id;
    d.physics_scene = scene;
    d.model = kart_model_result;

    car_descriptor_buffer.descriptors = fmj_stretch_buffer_init(1,sizeof(CarFrameDescriptor),8);
    fmj_stretch_buffer_push(&car_descriptor_buffer.descriptors,&d);

    car_frame_buffer = {};
    kart_init_car_frames(&car_frame_buffer,car_descriptor_buffer,1,&asset_ctx);

    player_car = fmj_stretch_buffer_get(CarFrame,&car_frame_buffer.car_frames,0);

    game_state.current_player_id = 0;    
}


f32 aangle = 0;    
f32 x_move = 0;
bool show_title = true;
f32 outer_angle_rot = 0;    
u32 tex_id;
f32 size_sin = 0;
f32 tset_num = 0.005f;
//end game global  vars

void kart_game_update(PlatformState* ps,RenderCamera rc,AssetTables* asset_tables,FMJMemoryArena permanent_strings,FMJMemoryArena temp_strings)
{
            
//Game Code 
        if(ps->input.keyboard.keys[keys.s].down)
        {
            ps->is_running = false;
        }

        FMJSceneObject* track_physics_so = fmj_stretch_buffer_check_out(FMJSceneObject,&asset_ctx.scene_objects,track_physics_id);
        
        PxTransform pxt = ((PxRigidDynamic*)track_rbd.state)->getGlobalPose();
        f3 new_p = f3_create(pxt.p.x,pxt.p.y,pxt.p.z);
        quaternion new_r = quaternion_create(pxt.q.x,pxt.q.y,pxt.q.z,pxt.q.w);

        track_physics_so->transform.local_p = new_p;
        track_physics_so->transform.local_r = new_r;
        fmj_stretch_buffer_check_in(&asset_ctx.scene_objects);
        track_so->transform = track_physics_so->transform;

#if 1
        FMJCurves f16lc;
        FMJCurves f16rlc;
        FMJCurves sa_curve;
        f3 input_axis = f3_create_f(0);

        UpdateCarFrames(ps,&car_frame_buffer,f16lc,f16rlc,sa_curve,ps->time.delta_seconds,input_axis,scene,ps->time.delta_seconds);
#endif

        aangle += 0.01f;

//modify camera matrix
        f4x4* p_mat_ = fmj_stretch_buffer_check_out(f4x4,&asset_ctx.asset_tables->matrix_buffer,rc.projection_matrix_id);
#if 0
        size.x = abs(sin(size_sin)) * 300;
        size.y = abs(cos(size_sin)) * 300;
        
        size.x = clamp(size.x,150,300);
        size.y = clamp(size.x,150,300);
        size.x = size.x * aspect_ratio;
        fmj_stretch_buffer_check_in(&asset_ctx.asset_tables->matrix_buffer);
        size_sin += 0.001f;        
        *p_mat_ = init_ortho_proj_matrix(size,0.0f,1.0f);
#endif
        f4x4 p_mat = *p_mat_;
        p_mat_ = nullptr;
        fmj_stretch_buffer_check_in(&asset_ctx.asset_tables->matrix_buffer);
//end render camera modify

//        SoundCode::ContinousPlay(&bgm_soundclip);
//Render camera stated etc..  is finalized        

//        fmj_camera_orbit(&asset_ctx,&rc,ps->input,ps->time.delta_seconds,kart_physics_so->transform,2);
//        fmj_camera_orbit(&asset_ctx,&rc,ps->input,ps->time.delta_seconds,player_car.e->transform,2);
//        f3 f = fmj_3dtrans_world_to_local_dir(&player_car.e->transform,player_car.e->transform.forward);

//        f3 go = fmj_3dtrans_local_to_world_pos(&player_car.e->transform,f3_create(0,3,-3));
//        f3 f = f3_mul_s(f3_negate(quaternion_forward(player_car.e->transform.r),3);
        fmj_camera_chase(&asset_ctx,&rc,ps->input,ps->time.delta_seconds,player_car.e->transform,f3_create(0,3,0));
        
//        fmj_camera_chase(&asset_ctx,&rc,ps->input,ps->time.delta_seconds,player_car.e->transform,f3_create(f.x,f.y + 3,-f.z - 3));
//        fmj_camera_free(&asset_ctx,&rc,ps->input,ps->time.delta_seconds);
//End game code

        //Compositable engine code.
        PhysicsCode::Update(&scene,1,ps->time.delta_seconds);                
}

void kart_game_kart_properties_imgui(FMJAssetContext* ctx,FMJSceneObject* so)
{
    if(ImGui::TreeNode("KART"))
    {
        GOHandle handle = fmj_anycache_get(GOHandle,&ctx->so_to_go,so);
        CarFrame cf = fmj_stretch_buffer_get(CarFrame,handle.buffer,handle.index);
                    
        ImGui::Text("KART Properties");
        ImGui::InputFloat3("thrust",cf.thrust_vector.xyz);
        ImGui::InputFloat3("velocity",cf.v.xyz);

        ImGui::Text("is_grounded %d ",(u32)cf.is_grounded);
        ImGui::InputFloat3("ground normal",cf.last_track_normal.xyz);
        ImGui::InputFloat3("foward",cf.e->transform.forward.xyz);
                    
        ImGui::Text("ground speed %f",cf.indicated_ground_speed);
        ImGui::Text("last aos  %f",cf.last_aos);
        ImGui::Text("last aoa  %f",cf.last_aoa);
        ImGui::Text("repulse force area  %f",cf.repulsive_force_area);

        PxVec3 lin_vel = ((PxRigidBody*)cf.rb.state)->getLinearVelocity();
                    
        ImGui::InputFloat3("phsyx LinVel",f3_create(lin_vel.x,lin_vel.y,lin_vel.z).xyz);                    
        ImGui::TreePop();                                        
    }                
}

#define KART_GAME_H
#endif
