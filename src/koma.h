
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
            carframe->v = rv;

            fmj_stretch_buffer_check_in(handle.buffer);                            
            
//            for(PxU32 j=0; j < nbContacts; j++)
            {
//                PxVec3 point = contacts[j].position;
//                PxVec3 impulse = contacts[j].impulse;
//                PxVec3 normal = contacts[j].normal;
//                PxU32 internalFaceIndex0 = contacts[j].internalFaceIndex0;
//                PxU32 internalFaceIndex1 = contacts[j].internalFaceIndex1;
                
                int a =0;

            }            
            
		}
	}
};

struct SpriteTrans
{
    u64 id;
//    u64 sprite_id;//not  using ref sprites in this  game common case is to use
    //every sprite will be different
    FMJSprite sprite;
    u64 material_id;
    u64 model_matrix_id;
    FMJ3DTrans t;
}typedef SpriteTrans;

u64 sprite_trans_create(f3 p,f3 s,quaternion r,f4 color,FMJSprite sprite,FMJStretchBuffer* matrix_buffer,FMJFixedBuffer* buffer,FMJMemoryArena* sb_arena)
{
    f2 stbl = f2_create(0.0f,0.0f);
    f2 stbr = f2_create(1.0f,0.0f);
    f2 sttr = f2_create(1.0f,1.0f);
    f2 sttl = f2_create(0.0f,1.0f);
    f2 uvs[4] = {stbl,stbr,sttr,sttl};
    
    SpriteTrans result = {0};
    FMJ3DTrans t = {0};
    t.p = p;
    t.s = s;
    t.r = r;
    fmj_3dtrans_update(&t);
    result.t = t;
    result.sprite = sprite;
    result.model_matrix_id = fmj_stretch_buffer_push(matrix_buffer,&result.t.m);
    u64 id = fmj_fixed_buffer_push(buffer,(void*)&result);
    //NOTE(Ray):PRS is not used here  just the matrix that is passed above.
    fmj_sprite_add_quad_notrans(sb_arena,p,r,s,color,uvs);
    return id;
}

FMJSprite add_sprite_to_stretch_buffer(FMJStretchBuffer* sprites,u64 material_id,u64 tex_id,f2 uvs[],f4 color,bool is_visible)
{
    FMJSprite result = {0};
    result = fmj_sprite_init(tex_id,uvs,color,is_visible);
    result.material_id = material_id;
    u64 sprite_id = fmj_stretch_buffer_push(sprites,(void*)&result);
    result.id = sprite_id;
    return result;    
}



