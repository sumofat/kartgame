#include<windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <xinput.h>

extern "C"{
#include "fmj/src/fmj_types.h"
}

#include "directx12.h"
#include "util.h"

#include "sound_interface.h"
#include "sound_interface.cpp"

#include "physics.h"
#include "physics.cpp"

#include "platform_win32.h"

#include "assets.h"

#include "koma.h"
#include "ping_game.h"

AssetTables asset_tables = {0};
u64 material_count = 0;

enum RenderCameraProjectionType
{
    perspective,
    orthographic,
    screen_space
};

struct RenderCamera
{
    FMJ3DTrans ot;//perspective and ortho only
    f4x4 matrix;
    f4x4 projection_matrix;
    f4x4 spot_light_shadow_projection_matrix;
    f4x4 point_light_shadow_projection_matrix;
    RenderCameraProjectionType projection_type;
    f32 size;//ortho only
    f32 fov;//perspective only
    f2 near_far_planes;
};

//draw nodes
//TODO(Ray):Not for sure what convention to use here before we pull this into FMJ
//either each node is associate with a FMJ3DTrans or it has an ID or some other way
//for the user to connect rendering to the  ui nodes.
void fmj_ui_commit_nodes_for_drawing(FMJMemoryArena* arena,FMJUINode base_node,FMJFixedBuffer* quad_buffer,f4 color,f2 uvs[])
{
    for(int i = 0;i < base_node.children.fixed.count;++i)
    {
        FMJUINode* child_node = fmj_stretch_buffer_check_out(FMJUINode,&base_node.children,i);
        if(child_node)
        {
            if(child_node->type == fmj_ui_node_sprite)
            {
                SpriteTrans st= {0};
                st.sprite = child_node->sprite;
                child_node->st_id = fmj_fixed_buffer_push(quad_buffer,(void*)&st);
                fmj_sprite_add_rect_with_dim(arena,child_node->rect.dim,0,child_node->rect.color,uvs);
            }
            else
            {
                ASSERT(false);//no othe rtypes supported yet.
            }
            fmj_ui_commit_nodes_for_drawing(arena,*child_node,quad_buffer,color,uvs);                            
        }
    }
}
//end draw nodes

int WINAPI WinMain(HINSTANCE h_instance,HINSTANCE h_prev_instance, LPSTR lp_cmd_line, int n_show_cmd )
{
    printf("Starting Window...");
    PlatformState* ps = &gm.ps;
    ps->is_running  = true;

//WINDOWS SETUP

    if(!PlatformInit(ps,f2_create(1024,1024),f2_create_f(0),n_show_cmd))
    {
        //Could not start or create window
        ASSERT(false);
    }
    
    //Lets init directx12
    CreateDeviceResult result = D12RendererCode::Init(&ps->window.handle,ps->window.dim);
    if (result.is_init)
    {
        //Do some setup if needed
    }
    else
    {
        //Could not initialize graphics device.
        ASSERT(false);
        return 0;
    }
    ID3D12Device2* device = D12RendererCode::GetDevice();
    
    //init tables
    fmj_asset_init(&asset_tables);

    FMJStretchBuffer render_command_buffer = fmj_stretch_buffer_init(1,sizeof(FMJRenderCommand),8);
    FMJStretchBuffer matrix_buffer = fmj_stretch_buffer_init(1,sizeof(f4x4),8);

    
    //Load a texture with all mip maps calculated
    //load texture to mem from disk
    LoadedTexture ping_title = get_loaded_image("ping_title.png",4);
    D12RendererCode::Texture2D(&ping_title,3);
    
    FMJMemoryArena permanent_strings = fmj_arena_allocate(FMJMEGABYTES(1));
    FMJMemoryArena temp_strings = fmj_arena_allocate(FMJMEGABYTES(1));    
    FMJString base_path_to_data = fmj_string_create("../data/Desktop/",&permanent_strings);
    
    //setup audio system and load sound assets.
    SoundAssetCode::Init();
    SoundCode::Init();
    SoundCode::SetDefaultListener();
    PhysicsCode::Init();
    char* file_name = "Master.strings.bank";
    FMJString mat_final_path = fmj_string_append_char(base_path_to_data, file_name,&temp_strings);
        
    SoundAssetCode::CreateSoundBank(mat_final_path.string);
        
    char* bsfile_name = "Master.bank";
    mat_final_path = fmj_string_append_char(base_path_to_data, bsfile_name,&temp_strings);
    SoundAssetCode::CreateSoundBank(mat_final_path.string);
        
    char* afile_name = "bgm.bank";
    mat_final_path = fmj_string_append_char(base_path_to_data, afile_name, &temp_strings);
    SoundAssetCode::CreateSoundBank(mat_final_path.string);
    SoundAssetCode::LoadBankSampleData();
    //SoundAssetCode::GetBus("bus:/bgmgroup");
    SoundAssetCode::GetEvent("event:/bgm",&bgm_soundclip);
    //end audio setup

    //BEGIN Setup physics stuff
    physics_material = PhysicsCode::CreateMaterial(0.1f, 0.1f, 1.0f);
    PhysicsCode::SetRestitutionCombineMode(physics_material,physx::PxCombineMode::eMULTIPLY);
    PhysicsCode::SetFrictionCombineMode(physics_material,physx::PxCombineMode::eMIN);
    scene = PhysicsCode::CreateScene(KomaFilterShader);
    GamePiecePhysicsCallback* e = new GamePiecePhysicsCallback();
    PhysicsCode::SetSceneCallback(&scene, e);
    //END PHYSICS SETUP

    D12RendererCode::CreateDefaultRootSig();
    D12RendererCode::CreateDefaultDepthStencilBuffer(ps->window.dim);
    
    D3D12_INPUT_ELEMENT_DESC input_layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR"   , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    
    D3D12_INPUT_ELEMENT_DESC input_layout_mesh[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT,   1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },        
//        { "COLOR"   , 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    
    uint32_t input_layout_count = _countof(input_layout);
    uint32_t input_layout_count_mesh = _countof(input_layout_mesh);    
    // Create a root/shader signature.

    char* vs_file_name = "vs_test.hlsl";
    char* vs_mesh_file_name = "vs_test_mesh.hlsl";
    char* fs_file_name = "ps_test.hlsl";
    char* fs_color_file_name = "fs_color.hlsl";
    RenderShader rs = D12RendererCode::CreateRenderShader(vs_file_name,fs_file_name);
    RenderShader mesh_rs = D12RendererCode::CreateRenderShader(vs_mesh_file_name,fs_file_name);

    RenderShader rs_color = D12RendererCode::CreateRenderShader(vs_file_name,fs_color_file_name);
    
    PipelineStateStream ppss = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout,input_layout_count,rs.vs_blob,rs.fs_blob);
    ID3D12PipelineState* pipeline_state = D12RendererCode::CreatePipelineState(ppss);    

    PipelineStateStream color_ppss = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout,input_layout_count,rs.vs_blob,rs_color.fs_blob);
    ID3D12PipelineState* color_pipeline_state = D12RendererCode::CreatePipelineState(color_ppss);
    
    PipelineStateStream color_ppss_mesh = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout_mesh,input_layout_count_mesh,mesh_rs.vs_blob,mesh_rs.fs_blob,true);
    ID3D12PipelineState* color_pipeline_state_mesh = D12RendererCode::CreatePipelineState(color_ppss_mesh);    
    
    FMJRenderMaterial base_render_material = {0};
    base_render_material.pipeline_state = (void*)pipeline_state;
    base_render_material.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    base_render_material.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);
    base_render_material.id = material_count;
    fmj_anycache_add_to_free_list(&asset_tables.materials,(void*)&material_count,&base_render_material);
    material_count++;

    FMJRenderMaterial color_render_material = base_render_material;
    color_render_material.pipeline_state = (void*)color_pipeline_state;
    color_render_material.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    color_render_material.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);    
    color_render_material.id = material_count;
    fmj_anycache_add_to_free_list(&asset_tables.materials,(void*)&material_count,&color_render_material);    
    material_count++;

    FMJRenderMaterial color_render_material_mesh = color_render_material;
    color_render_material_mesh.pipeline_state = (void*)color_pipeline_state_mesh;
    color_render_material_mesh.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    color_render_material_mesh.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);    
    color_render_material_mesh.id = material_count;
    fmj_anycache_add_to_free_list(&asset_tables.materials,(void*)&material_count,&color_render_material_mesh);    
    material_count++;

    //Camera setup
    RenderCamera rc = {};
    rc.ot.p = f3_create(0,0,0);
    rc.ot.r = f3_axis_angle(f3_create(0,0,1),0);
    rc.ot.s = f3_create(1,1,1);

    f32 aspect_ratio = ps->window.dim.x / ps->window.dim.y;
    f2 size = f2_create_f(300);
    size.x = size.x * aspect_ratio;
//    rc.projection_matrix = init_ortho_proj_matrix(size,0.0f,1.0f);
    rc.fov = 80;
    rc.near_far_planes = f2_create(0.1,1000);
    rc.projection_matrix = init_pers_proj_matrix(ps->window.dim,rc.fov,rc.near_far_planes);
    rc.matrix = f4x4_identity();
    
    u64 projection_matrix_id = fmj_stretch_buffer_push(&matrix_buffer,(void*)&rc.projection_matrix);
    u64 rc_matrix_id = fmj_stretch_buffer_push(&matrix_buffer,(void*)&rc.matrix);
    
    RenderCamera rc_ui = {};
    rc_ui.ot.p = f3_create(0,0,0);
    rc_ui.ot.r = f3_axis_angle(f3_create(0,0,1),0);
    rc_ui.ot.s = f3_create(1,1,1);
    rc_ui.projection_matrix = init_screen_space_matrix(ps->window.dim);
    rc_ui.matrix = f4x4_identity();
    
    u64 screen_space_matrix_id = fmj_stretch_buffer_push(&matrix_buffer,(void*)&rc_ui.projection_matrix);
    u64 identity_matrix_id = fmj_stretch_buffer_push(&matrix_buffer,(void*)&rc_ui.matrix);
//end camera setup

    //some memory setup
    u64 quad_mem_size = SIZE_OF_SPRITE_IN_BYTES * 100;
    u64 matrix_mem_size = (sizeof(f32) * 16) * 100;
    GPUArena kart_gpu_arena = D12RendererCode::AllocateStaticGPUArena(quad_mem_size);
    
    GPUArena quad_gpu_arena = D12RendererCode::AllocateStaticGPUArena(quad_mem_size);
    GPUArena ui_quad_gpu_arena = D12RendererCode::AllocateStaticGPUArena(quad_mem_size);    
    GPUArena matrix_gpu_arena = D12RendererCode::AllocateGPUArena(matrix_mem_size);

    FMJFixedBuffer fixed_quad_buffer = fmj_fixed_buffer_init(200,sizeof(SpriteTrans),8);
    ui_fixed_quad_buffer = fmj_fixed_buffer_init(200,sizeof(SpriteTrans),8);    
    FMJFixedBuffer matrix_quad_buffer = fmj_fixed_buffer_init(200,sizeof(f4x4),0);
//end memory setup

//TITLE ui definition    
    FMJSpriteBatch sb = {0};
    FMJSpriteBatch sb_ui = {0};
    sb.arena = fmj_arena_allocate(quad_mem_size);
    sb_ui.arena = fmj_arena_allocate(quad_mem_size);
    
    uint64_t stride = sizeof(float) * (3 + 4 + 2);
    f2 stbl = f2_create(0.0f,0.0f);
    f2 stbr = f2_create(1.0f,0.0f);
    f2 sttr = f2_create(1.0f,1.0f);
    f2 sttl = f2_create(0.0f,1.0f);
    f2 uvs[4] = {stbl,stbr,sttr,sttl};
    f4 white = f4_create(1,1,1,1);

    FMJUIState ui_state = {0};
    FMJUINode base_node = {0};
    base_node.rect.dim = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    base_node.children = fmj_stretch_buffer_init(1,sizeof(FMJUINode),8);

    ui_state.parent_node = base_node;
    FMJSprite ui_sprite = {0};
    ui_sprite = fmj_sprite_init(3,uvs,white,true);

    FMJUINode bkg_child = {0};
    bkg_child.use_anchor = true;
    bkg_child.rect.anchor = f4_create(0.0f, 0.0f, 1.0f, 1.0f);
    bkg_child.rect.offset = f4_create(0,0,0,0);
    bkg_child.rect.color = f4_create(0,0,1,1);
    bkg_child.rect.highlight_color = f4_create(1,0,1,1);
    bkg_child.rect.current_color = bkg_child.rect.color;
    bkg_child.interactable = false;
    bkg_child.rect.z = 0.1f;
    bkg_child.children = fmj_stretch_buffer_init(1,sizeof(FMJUINode),8);
    bkg_child.type = fmj_ui_node_sprite;

    ui_sprite.material_id = color_render_material.id;
    bkg_child.sprite = ui_sprite;
    
    FMJUINode title_child = {0};
    title_child.use_anchor = true;
    title_child.rect.anchor = f4_create(0.25f, 0.25f, 0.75f, 0.75f);
    title_child.rect.offset = f4_create(0,0,0,0);
    title_child.rect.color = f4_create(1,1,1,1);
    title_child.rect.highlight_color = f4_create(1,0,1,1);
    title_child.rect.current_color = title_child.rect.color;
    title_child.interactable = false;
    title_child.rect.z = 0;
    title_child.type = fmj_ui_node_sprite;
    ui_sprite.tex_id = 3;
    ui_sprite.material_id = base_render_material.id;
    title_child.sprite = ui_sprite;
    
    u64 ping_ui_id = fmj_stretch_buffer_push(&bkg_child.children,&title_child);
    u64 bkg_ui_id = fmj_stretch_buffer_push(&base_node.children,&bkg_child);    
//end ui definition
    
//game object setup pieces/komas

    quaternion def_r = f3_axis_angle(f3_create(0,0,1),0);

    f2  top_right_screen_xy = f2_create(ps->window.dim.x,ps->window.dim.y);
    f2  bottom_left_xy = f2_create(0,0);

    f3 max_screen_p = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,top_right_screen_xy,0);
    f3 lower_screen_p = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,bottom_left_xy,0);

    //test gltlf aquisition
    FMJAssetContext asset_ctx = {};
    asset_ctx.asset_tables = &asset_tables;
    asset_ctx.perm_mem = &permanent_strings;
    asset_ctx.temp_mem = &temp_strings;    

//    FMJAssetModel test_model = fmj_asset_load_model_from_glb(&asset_ctx,"../data/models/BoxTextured.glb",color_render_material_mesh.id);
//    FMJAssetModel test_model = fmj_asset_load_model_from_glb(&asset_ctx,"../data/models/Box.glb",color_render_material_mesh.id);
//    FMJAssetModel test_model = fmj_asset_load_model_from_glb(&asset_ctx,"../data/models/Avocado.glb",color_render_material_mesh.id);//scale is messed up and uvs
//    FMJAssetModel test_model = fmj_asset_load_model_from_glb(&asset_ctx,"../data/models/Fox.glb",color_render_material_mesh.id);        //no indices 16
//    FMJAssetModel test_model = fmj_asset_load_model_from_glb(&asset_ctx,"../data/models/Duck.glb",color_render_material_mesh.id);//scale is messed up and uvs
//    FMJAssetModel test_model = fmj_asset_load_model_from_glb(&asset_ctx,"../data/models/2CylinderEngine.glb",color_render_material_mesh.id);//crashes
//    FMJAssetModel test_model = fmj_asset_load_model_from_glb(&asset_ctx,"../data/models/BarramundiFish.glb",color_render_material_mesh.id);//
    FMJAssetModel test_model = fmj_asset_load_model_from_glb(&asset_ctx,"../data/models/Lantern.glb",color_render_material_mesh.id);//crashes        

    FMJAssetMesh* test_mesh = fmj_stretch_buffer_check_out(FMJAssetMesh,&test_model.meshes,0);
    
    fmj_asset_upload_model(&asset_tables,&asset_ctx,&test_model);
    FMJ3DTrans test_mesh_t;
    fmj_3dtrans_init(&test_mesh_t);
    test_mesh_t.p = f3_create(0,0,-5);
    test_mesh_t.s = f3_create_f(1);
    fmj_3dtrans_update(&test_mesh_t);    
    
    u64 test_mesh_model_matrix_id = fmj_stretch_buffer_push(&matrix_buffer,&test_mesh_t.m);
//end game object setup

    //ui  evaulation
    fmj_ui_evaluate_node(&base_node,&ui_state.hot_node_state);
    fmj_ui_commit_nodes_for_drawing(&sb_ui.arena,base_node,&ui_fixed_quad_buffer,white,uvs);
//end ui evaulation

    //upload data to gpu
    D12RendererCode::SetArenaToVertexBufferView(&quad_gpu_arena,quad_mem_size,stride);
    D12RendererCode::SetArenaToVertexBufferView(&ui_quad_gpu_arena,quad_mem_size,stride);    
    u64 sprite_buffer_id = fmj_stretch_buffer_push(&asset_tables.vertex_buffers,(void*)&quad_gpu_arena.buffer_view);    
    u64 ui_sprite_buffer_id = fmj_stretch_buffer_push(&asset_tables.vertex_buffers,(void*)&ui_quad_gpu_arena.buffer_view);

    D12RendererCode::UploadBufferData(&quad_gpu_arena,sb.arena.base,quad_mem_size);    
    D12RendererCode::UploadBufferData(&ui_quad_gpu_arena,sb_ui.arena.base,quad_mem_size);
    
    u64 m_stride = sizeof(f32) * 16;
    D12RendererCode::SetArenaToConstantBuffer(&matrix_gpu_arena,4);
    void* mapped_matrix_data;
    matrix_gpu_arena.resource->Map(0,NULL,&mapped_matrix_data);
    
#if 1
    quad_gpu_arena.resource->SetName(L"QUADS_GPU_ARENA");
    ui_quad_gpu_arena.resource->SetName(L"UI_QUADS_GPU_ARENA");    
#endif
//end up load data to gpu

    //game global vars
    u32 tex_index = 0;
    f32 angle = 0;    
    f2 cam_pitch_yaw = f2_create(0.0f,0.0f);
    f32 x_move = 0;
    bool show_title = true;
    f32 outer_angle_rot = 0;    
    u32 tex_id;
    f32 size_sin = 0;
    game_state.current_player_id = 0;
    //end game global  vars

    //game loop
    while(ps->is_running)
    {
//pull this frames info from devices//api
        PullDigitalButtons(ps);
        PullMouseState(ps);
        PullGamePads(ps);
        PullTimeState(ps);
//End pull
        
//Game Code 
        if(ps->input.keyboard.keys[keys.s].down)
        {
            ps->is_running = false;
        }

        //stop showing title  after mouse button is pressed
        if(ps->input.mouse.lmb.released)
        {
            show_title = false;
            fmj_ui_evaluate_on_node_recursively(&base_node,SetSpriteNonVisible);
        }

        test_mesh_t.r = f3_axis_angle(f3_create(0,1,0),angle);
        fmj_3dtrans_update(&test_mesh_t);    
        angle += 0.01f;
        
        f4x4* tmt = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,test_mesh_model_matrix_id);        
        *tmt = test_mesh_t.m;

        //modify camera matrix
        f4x4* p_mat_ = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,projection_matrix_id);
#if 0
        size.x = abs(sin(size_sin)) * 300;
        size.y = abs(cos(size_sin)) * 300;
        
        size.x = clamp(size.x,150,300);
        size.y = clamp(size.x,150,300);
        size.x = size.x * aspect_ratio;
        fmj_stretch_buffer_check_in(&matrix_buffer);
        size_sin += 0.001f;        
        *p_mat_ = init_ortho_proj_matrix(size,0.0f,1.0f);
#endif
        f4x4 p_mat = *p_mat_;
        p_mat_ = nullptr;
        fmj_stretch_buffer_check_in(&matrix_buffer);
//end render camera modify

        SoundCode::ContinousPlay(&bgm_soundclip);
//Render camera stated etc..  is finalized        
 //Free cam
#if 0
        cam_pitch_yaw = f2_add(cam_pitch_yaw,ps->input.mouse.delta_p);
        quaternion pitch = f3_axis_angle(f3_create(1, 0, 0), cam_pitch_yaw.y);
        quaternion yaw   = f3_axis_angle(f3_create(0, 1, 0), cam_pitch_yaw.x * -1);
        quaternion turn_qt = quaternion_mul(pitch,yaw);        
        rc.ot.r = turn_qt;
#endif
        rc.matrix = fmj_3dtrans_set_cam_view(&rc.ot);
        f4x4* rc_mat = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,rc_matrix_id);
        *rc_mat = rc.matrix;
        fmj_stretch_buffer_check_in(&matrix_buffer);
//End game code

//Render command creation(Render pass setup)

        for(int i = 0;i < test_model.meshes.fixed.count;++i)
        {
            FMJRenderCommand com = {};
            FMJAssetMesh m = fmj_stretch_buffer_get(FMJAssetMesh,&test_model.meshes,i);
            FMJRenderGeometry geo = {};
            if(m.index32_count > 0)
            {
                com.is_indexed = true;
                geo.buffer_id_range = m.mesh_resource.buffer_range;
                geo.index_id = m.mesh_resource.index_id;
                geo.index_count = m.index32_count;                
            }
            if(m.index16_count > 0)
            {
                com.is_indexed = true;
                geo.buffer_id_range = m.mesh_resource.buffer_range;
                geo.index_id = m.mesh_resource.index_id;
                geo.index_count = m.index16_count;                
            }
            else
            {
                ASSERT(false);
            }
            geo.offset = 0;
            com.geometry = geo;
            com.material_id = m.material_id;
            com.texture_id = m.metallic_roughness_texture_id;
            com.model_matrix_id = test_mesh_model_matrix_id;
            com.camera_matrix_id = rc_matrix_id;
            com.perspective_matrix_id = projection_matrix_id;
            fmj_stretch_buffer_push(&render_command_buffer,(void*)&com);            
        }
                
        for(int i = 0;i < fixed_quad_buffer.count;++i)
        {
            FMJRenderCommand com = {};
            SpriteTrans st = fmj_fixed_buffer_get(SpriteTrans,&fixed_quad_buffer,i);
            FMJSprite s = st.sprite;
            if(s.is_visible)
            {
                FMJRenderGeometry geo = {};                            
                geo.offset = (i * 6);
                geo.buffer_id_range = f2_create(sprite_buffer_id,sprite_buffer_id);
                geo.count = 6;                
                com.geometry = geo;
                com.material_id = s.material_id;
                com.texture_id = s.tex_id;
                com.model_matrix_id = st.model_matrix_id;
                com.camera_matrix_id = rc_matrix_id;
                com.perspective_matrix_id = projection_matrix_id;
                fmj_stretch_buffer_push(&render_command_buffer,(void*)&com);
            }            
        }

        for(int i = 0;i < ui_fixed_quad_buffer.count;++i)
        {
            FMJRenderCommand com = {};
            SpriteTrans st = fmj_fixed_buffer_get(SpriteTrans,&ui_fixed_quad_buffer,i);
            FMJSprite s = st.sprite;//fmj_stretch_buffer_get(FMJSprite,&asset_tables.sprites,st.sprite_id);
            if(s.is_visible)
            {
                FMJRenderGeometry geo = {};                                            
                geo.offset = (i * 6);
                geo.buffer_id_range = f2_create(ui_sprite_buffer_id,ui_sprite_buffer_id);
                geo.count = 6;
                com.geometry = geo;

                com.material_id = s.material_id;
                com.texture_id = s.tex_id;
                com.model_matrix_id = identity_matrix_id;
                com.camera_matrix_id = identity_matrix_id;
                com.perspective_matrix_id = screen_space_matrix_id;
                fmj_stretch_buffer_push(&render_command_buffer,(void*)&com);
            }
        }

//Render command setup end

//Render commands issued to the render api
        bool has_update = false;

        if(render_command_buffer.fixed.count > 0)
        {
            D12RendererCode::AddStartCommandListCommand();                            
            //screen space cam
            for(int i = 0;i < render_command_buffer.fixed.count;++i)
            {
                FMJRenderCommand command = fmj_stretch_buffer_get(FMJRenderCommand,&render_command_buffer,i);
                {
                    f4x4 m_mat = fmj_stretch_buffer_get(f4x4,&matrix_buffer,command.model_matrix_id);
                    f4x4 c_mat = fmj_stretch_buffer_get(f4x4,&matrix_buffer,command.camera_matrix_id);
                    f4x4 p_mat = fmj_stretch_buffer_get(f4x4,&matrix_buffer,command.perspective_matrix_id);
                    f4x4 world_mat = f4x4_mul(c_mat,m_mat);
                    f4x4 finalmat = f4x4_mul(p_mat,world_mat);
                    m_mat.c0.x = (f32)(matrix_quad_buffer.count * sizeof(f4x4));

                    fmj_fixed_buffer_push(&matrix_quad_buffer,(void*)&finalmat);        

                    D12RendererCode::AddRootSignatureCommand(D12RendererCode::root_sig);
                    D12RendererCode::AddViewportCommand(f4_create(0,0,ps->window.dim.x,ps->window.dim.y));
                    D12RendererCode::AddScissorRectCommand(CD3DX12_RECT(0,0,LONG_MAX,LONG_MAX));

                    FMJRenderMaterial material = fmj_anycache_get(FMJRenderMaterial,&asset_tables.materials,(void*)&command.material_id);
                    D12RendererCode::AddPipelineStateCommand((ID3D12PipelineState*)material.pipeline_state);                    
                    D12RendererCode::AddGraphicsRoot32BitConstant(0,16,&m_mat,0);                    
                    D12RendererCode::AddGraphicsRoot32BitConstant(2,16,&finalmat,0);
                    
                    tex_index = command.texture_id;
                    D12RendererCode::AddGraphicsRoot32BitConstant(4,4,&tex_index,0);
                    D12RendererCode::AddGraphicsRootDescTable(1,D12RendererCode::default_srv_desc_heap,D12RendererCode::default_srv_desc_heap->GetGPUDescriptorHandleForHeapStart());

                    int slot = 0;
                    for(int i = command.geometry.buffer_id_range.x;i <= command.geometry.buffer_id_range.y;++i)
                    {
                        D3D12_VERTEX_BUFFER_VIEW bv = fmj_stretch_buffer_get(D3D12_VERTEX_BUFFER_VIEW,&asset_tables.vertex_buffers,i);
                        D12RendererCode::AddSetVertexBufferCommand(slot++,bv);
                    }
                    
                    if(command.is_indexed)
                    {
                        D3D12_INDEX_BUFFER_VIEW ibv = fmj_stretch_buffer_get(D3D12_INDEX_BUFFER_VIEW,&asset_tables.index_buffers,command.geometry.index_id);
                        D12RendererCode::AddDrawIndexedCommand(command.geometry.index_count,command.geometry.offset,D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,ibv);
                    }
                    else
                    {
                        D12RendererCode::AddDrawCommand(command.geometry.offset,command.geometry.count,D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);                        
                    }

                    has_update = true;
                }
            }
            D12RendererCode::AddEndCommandListCommand();                            
        }

        if(has_update)
        {
            memcpy((void*)mapped_matrix_data,(void*)matrix_quad_buffer.mem_arena.base,matrix_quad_buffer.mem_arena.used);
            fmj_fixed_buffer_clear(&matrix_quad_buffer);
        }

//        fmj_fixed_buffer_clear(&ui_fixed_quad_buffer);        
//Command are finalized and rendering is started.        
 // TODO(Ray Garner): Add render targets commmand
//        if(show_title)
 
//        DrawTest test = {cube,rc,gpu_arena,test_desc_heap};
//Post frame work is done.
        D12RendererCode::EndFrame();

        fmj_stretch_buffer_clear(&render_command_buffer);
//END Render commands issued to the render api        

        PhysicsCode::Update(&scene,1,ps->time.delta_seconds);                
        SoundCode::Update();

        HandleWindowsMessages(ps);        
    }
    return 0;
}
