enum KomaType
{
    koma_type_none,
    koma_type_pawn,
    koma_type_rook,
    koma_type_queen,
    koma_type_king
};

enum KomaActionType
{
    koma_action_type_none,
    koma_action_type_attack,
    koma_action_type_defense
};

struct Koma
{
    u64 inner_id;
    u64 outer_id;
//    f3 velocity;
    RigidBody rigid_body;
    int hp;
    u32 max_hp;
    u32 atk_pow;
    KomaType type;
    KomaActionType action_type;
    f32 max_speed;
    f32 friction;
    u32 team_id;//index out of  10
    u32  player_id;
}typedef Koma;

struct FingerPull
{
    f2 start_pull_p;
    f2 end_pull_p;
    float pull_strength;
    bool pull_begin = false;
    Koma* koma;
//    u64 koma_id
};


FMJStretchBuffer komas;
FingerPull finger_pull = {};

PxFilterFlags KomaFilterShader(
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
//    if((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
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
		for (PxU32 i = 0; i < nbPairs; i++)
		{
			const PxContactPair& cp = pairs[i];
            {
                u64 k_i = (u64)pairHeader.actors[0]->userData;
                Koma* k  = fmj_stretch_buffer_check_out(Koma,&komas,k_i);

                u64 k2_i = (u64)pairHeader.actors[1]->userData;                                            
                Koma* k2 = fmj_stretch_buffer_check_out(Koma,&komas,k2_i);
/*
                if(k->player_id == k2->player_id)
                {
                    //simulate physics for own team only
                    //and take damage
                    if(k->action_type == koma_action_type_defense)
                    {
                        u32 atk_pow = k->atk_pow;
                        k2->hp -= max(atk_pow,0);                                        
                    }
                    else if(k2->action_type == koma_action_type_defense)
                    {
                        u32 atk_pow = k2->atk_pow;                        
                        k->hp -= max(atk_pow,0);                                        
                    }
                }
                //               else
                */
                {
                    //take damage but no physics
                    if(k->action_type == koma_action_type_attack)
                    {
                        u32 atk_pow = k->atk_pow;
                        k2->hp -= max(atk_pow,0);                                        
                    }
                    else if(k2->action_type == koma_action_type_attack)
                    {
                        u32 atk_pow = k2->atk_pow;                        
                        k->hp -= max(atk_pow,0);                                        
                    }
                }

                fmj_stretch_buffer_check_in(&komas);
                fmj_stretch_buffer_check_in(&komas);
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



bool CheckValidFingerPull(FingerPull* fp)
{
    ASSERT(fp);
    fp->pull_strength = abs(f2_length(f2_sub(fp->start_pull_p,fp->end_pull_p)));
    if(fp->pull_strength > 2)
    {
        return true;
    }
    fp->pull_strength = 0;
    return false;
}

