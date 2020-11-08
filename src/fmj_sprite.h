#if !defined(FMJ_SPRITE_H)

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

#define FMJ_SPRITE_H
#endif
