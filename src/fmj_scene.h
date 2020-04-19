
enum FMJSceneState
{
    fmj_scene_state_default,//default state nothing happened
    fmj_scene_state_loading,
    fmj_scene_state_loaded,
    fmj_scene_state_unloading,
    fmj_scene_state_unloaded,
    fmj_scene_state_serializing,
    fmj_scene_state_serialized
};

struct FMJSceneStatus
{
    uint32_t state_flags;
    uint32_t object_count;
};

struct FMJSceneObjectBuffer
{
    FMJStretchBuffer buffer;
}typedef FMJSceneObjectBuffer;

struct FMJSceneObject
{
    FMJ3DTrans transform;
    FMJSceneObjectBuffer children;
    FMJSceneObject* parent;
    u32 type;//user defined type
    void* data;//user defined data typically ptr to a game object etcc...
}typedef FMJSceneObject;

struct FMJScene
{
    FMJSceneObjectBuffer buffer;
    FMJSceneState state_flags;    
}typedef FMJScene;

struct FMJSceneBuffer
{
    FMJStretchBuffer buffer;
};

void fmj_scene_object_buffer_init(FMJSceneObjectBuffer* buffer)
{
    ASSERT(buffer);
    buffer->buffer = fmj_stretch_buffer_init(1,sizeof(FMJSceneObject),false);
}

u64 AddSceneObject(FMJSceneObjectBuffer* so,FMJ3DTrans ot)
{
    ASSERT(so);
    FMJSceneObject new_so = {};
    new_so.transform = ot;
    uint32_t handle = fmj_stretch_buffer_push(&so->buffer,&new_so);
    return handle;
}

u64 AddSceneObject(FMJSceneObjectBuffer* so,f3 p,quaternion r,f3 s)
{
    ASSERT(so);
    FMJ3DTrans ot;
    fmj_3dtrans_init(&ot);
    ot.p = p;
    ot.r = r;
    ot.s = s;
    return AddSceneObject(so,ot);
}

void UpdateSceneObject(FMJSceneObject* so,f3 p,quaternion r,f3 s)
{
    ASSERT(so);
    so->transform.p = p;
    so->transform.r = r;
    so->transform.s = s;                            
}
    
//NOTE(Ray):When adding a chid ot p is local position and p is offset from parents ot p.
u64 AddChildToSceneObject(FMJSceneObject* so,FMJ3DTrans* new_child,void** data)
{
    ASSERT(so);
//    FMJSceneObject* so = fmj_stretch_buffer_check_out(SceneObject,&buffer->buffer,so_index);
    if(so->children.buffer.fixed.count == 0)
    {
        fmj_scene_object_buffer_init(&so->children);
//        so->children.buffer = YoyoInitVector(10, SceneObject, false);
    }

    
    //and the local p is the absolute p relative to the parent p.
    new_child->local_p = new_child->p;
    new_child->local_s = new_child->s;
    new_child->local_r = new_child->r;


    
    //New child p in this contex is the local p relative to the parent p reference frame
//		new_child->p  += rotate(so->transform.r, new_child->p);
    //Rotations add
//		new_child->r = so->transform.r * new_child->r;
//		new_child->s = so->transform.s * new_child->s;
    //SceneObject* new_so = AddSceneObject(&so->children, new_child);


    
    FMJSceneObject new_so = {};
    new_so.children = {};
    new_so.transform = *new_child;
    new_so.parent = so;
    new_so.data = *data;
    int handle = fmj_stretch_buffer_push(&so->children.buffer,&new_so);
    return handle;
}

//NOTE(Ray):When adding a chid ot p is local position and p is offset from parents ot p.
u64 AddChildToSceneObject(FMJSceneObject* so,f3 p,quaternion r,f3 s,void** data)
{
    ASSERT(so);
    FMJ3DTrans new_child;
    fmj_3dtrans_init(&new_child);
    new_child.p = p;
    new_child.r = r;
    new_child.s = s;
    return AddChildToSceneObject(so,&new_child,data);
}
    
void UpdateChildren(FMJSceneObject* parent_so,f3* position_sum,quaternion* rotation_product)
{
    FMJSceneObject* child_so;
    for(int i = 0;i < parent_so->children.buffer.fixed.count;++i)
    {
        child_so = fmj_stretch_buffer_check_out(FMJSceneObject,&parent_so->children.buffer,i);
        FMJ3DTrans *ot = &child_so->transform;
        f3 current_p_sum = *position_sum;
        quaternion current_r_product = *rotation_product;
        ot->p = current_p_sum = f3_add(ot->p,f3_rotate((current_r_product), ot->local_p));
        ot->r = current_r_product = quaternion_mul(current_r_product,ot->local_r);        

        ot->s = f3_mul(parent_so->transform.s,ot->local_s);
        fmj_3dtrans_update(ot);

        UpdateChildren(child_so, &current_p_sum, &current_r_product);        
    }
}

void UpdateSceneObjects(FMJSceneObjectBuffer* buffer,f3* position_sum,quaternion* rotation_product)
{
    FMJSceneObject* so;
    for(int i = 0;i < buffer->buffer.fixed.count;++i)
    {
        so = fmj_stretch_buffer_check_out(FMJSceneObject,&buffer->buffer,i);
        FMJ3DTrans* parent_ot = &so->transform;
        fmj_3dtrans_update(parent_ot);
        f3 current_p_sum = *position_sum;
        current_p_sum = f3_add(current_p_sum,parent_ot->p);
        *rotation_product = parent_ot->r;
        UpdateChildren(so, &current_p_sum, rotation_product);        
    }
}

void SetWorldP(FMJSceneObject* so,f3 p)
{
    ASSERT(so);
    f3 new_local_p = f3_sub(p,so->parent->transform.p);
    so->transform.p = p;
    so->transform.local_p = new_local_p;
//    so->flags |= 0x01;
}
    
void SetWorldR(FMJSceneObject* so,quaternion r)
{
    ASSERT(so);
    so->transform.r = r;
//    so->flags |= 0x02;
}

///SCENE CODE
void InitSceneBuffer(FMJSceneBuffer* buffer)
{
    ASSERT(buffer);
    buffer->buffer = fmj_stretch_buffer_init(1,sizeof(FMJScene),8);
}

//NOTE(Ray):All empty scenes in fact all scenes must have one scene object that is the root object
u64 CreateEmptyScene(FMJSceneBuffer* buffer)
{
    FMJScene scene = {};
    fmj_scene_object_buffer_init(&scene.buffer);
    scene.state_flags = fmj_scene_state_default;
    u64 id = fmj_stretch_buffer_push(&buffer->buffer,&scene);
    return id;
}

//NOTE(Ray):For updating all scenes?
void UpdateSceneBuffer(FMJSceneBuffer* buffer)
{
    FMJScene* scene;
    quaternion product = quaternion_identity();
    for(int i = 0; i < buffer->buffer.fixed.count;++i)
    {
        scene = fmj_stretch_buffer_check_out(FMJScene,&buffer->buffer,i);
        f3 sum = f3_create_f(0);
        UpdateSceneObjects(&scene->buffer,&sum,&product);
        fmj_stretch_buffer_check_in(&buffer->buffer);
    }
}

