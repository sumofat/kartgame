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
    InitAssetTables(&asset_tables);

    FMJStretchBuffer render_command_buffer = fmj_stretch_buffer_init(1,sizeof(FMJRenderCommand),8);
    FMJStretchBuffer matrix_buffer = fmj_stretch_buffer_init(1,sizeof(f4x4),8);

    
    //Load a texture with all mip maps calculated
    //load texture to mem from disk
    LoadedTexture test_texture = get_loaded_image("test.png",4);
    LoadedTexture test_texture_2 = get_loaded_image("test2.png",4);    
    LoadedTexture koma_2 = get_loaded_image("koma_2.png",4);
    LoadedTexture ping_title = get_loaded_image("ping_title.png",4);
    LoadedTexture koma_outer_1 = get_loaded_image("rook_outer_1.png",4);    
    LoadedTexture koma_outer_2 = get_loaded_image("rook_outer_2.png",4);
    LoadedTexture koma_outer_3 = get_loaded_image("rook_outer_3.png",4);

    LoadedTexture koma_pawn_inner = get_loaded_image("pawn.png",4);
    LoadedTexture koma_pawn_outer_1 = get_loaded_image("pawn_outer_1.png",4);
    LoadedTexture koma_pawn_outer_2 = get_loaded_image("pawn_outer_2.png",4);

    LoadedTexture koma_queen_inner = get_loaded_image("queen.png",4);
    LoadedTexture koma_queen_outer_1 = get_loaded_image("queen_outer_1.png",4);
    LoadedTexture koma_queen_outer_2 = get_loaded_image("queen_outer_2.png",4);
    LoadedTexture koma_queen_outer_3 = get_loaded_image("queen_outer_3.png",4);
    LoadedTexture koma_queen_outer_4 = get_loaded_image("queen_outer_4.png",4);
    LoadedTexture koma_queen_outer_5 = get_loaded_image("queen_outer_5.png",4);
    LoadedTexture koma_queen_outer_6 = get_loaded_image("queen_outer_6.png",4);

    LoadedTexture koma_king_inner = get_loaded_image("king.png",4);
    LoadedTexture koma_king_outer_1 = get_loaded_image("king_outer_1.png",4);
    LoadedTexture koma_king_outer_2 = get_loaded_image("king_outer_2.png",4);
    LoadedTexture koma_king_outer_3 = get_loaded_image("king_outer_3.png",4);
    LoadedTexture koma_king_outer_4 = get_loaded_image("king_outer_4.png",4);
    LoadedTexture koma_king_outer_5 = get_loaded_image("king_outer_5.png",4);
    
    //court stuff
    LoadedTexture circle = get_loaded_image("circle.png",4);
    LoadedTexture line   = get_loaded_image("line.png",4);
    
    //upload texture data to gpu
    D12RendererCode::Texture2D(&test_texture,0);
    D12RendererCode::Texture2D(&test_texture_2,1);

    D12RendererCode::Texture2D(&koma_2,2);
    D12RendererCode::Texture2D(&ping_title,3);
    f2 rook_tex_id_range = f2_create(4,7);    //4 is not here used by matrix buffer but we pretend it is.    
    D12RendererCode::Texture2D(&koma_outer_1,5);
    D12RendererCode::Texture2D(&koma_outer_2,6);
    D12RendererCode::Texture2D(&koma_outer_3,7);

    D12RendererCode::Texture2D(&circle,8);
    D12RendererCode::Texture2D(&line,9);        
    u32 current_tex_id = 10;
    f2 pawn_tex_id_range = f2_create(10,12);
    D12RendererCode::Texture2D(&koma_pawn_inner,current_tex_id++);
    D12RendererCode::Texture2D(&koma_pawn_outer_1,current_tex_id++);
    D12RendererCode::Texture2D(&koma_pawn_outer_2,current_tex_id++);

    f2 queen_tex_id_range = f2_create(13,19);
    D12RendererCode::Texture2D(&koma_queen_inner,current_tex_id++);
    D12RendererCode::Texture2D(&koma_queen_outer_1,current_tex_id++);
    D12RendererCode::Texture2D(&koma_queen_outer_2,current_tex_id++);
    D12RendererCode::Texture2D(&koma_queen_outer_3,current_tex_id++);
    D12RendererCode::Texture2D(&koma_queen_outer_4,current_tex_id++);
    D12RendererCode::Texture2D(&koma_queen_outer_5,current_tex_id++);    
    D12RendererCode::Texture2D(&koma_queen_outer_6,current_tex_id++);

    f2 king_tex_id_range = f2_create(20,25);
    D12RendererCode::Texture2D(&koma_king_inner,current_tex_id++);
    D12RendererCode::Texture2D(&koma_king_outer_1,current_tex_id++);
    D12RendererCode::Texture2D(&koma_king_outer_2,current_tex_id++);
    D12RendererCode::Texture2D(&koma_king_outer_3,current_tex_id++);
    D12RendererCode::Texture2D(&koma_king_outer_4,current_tex_id++);    
    D12RendererCode::Texture2D(&koma_king_outer_5,current_tex_id++);
    
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
    
    uint32_t input_layout_count = _countof(input_layout);
    // Create a root/shader signature.

    char* vs_file_name = "vs_test.hlsl";
    char* fs_file_name = "ps_test.hlsl";
    RenderShader rs = D12RendererCode::CreateRenderShader(vs_file_name,fs_file_name);

    char* fs_color_file_name = "fs_color.hlsl";
    RenderShader rs_color = D12RendererCode::CreateRenderShader(vs_file_name,fs_color_file_name);
    
    PipelineStateStream ppss = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout,input_layout_count,rs.vs_blob,rs.fs_blob);
    ID3D12PipelineState* pipeline_state = D12RendererCode::CreatePipelineState(ppss);    

    PipelineStateStream color_ppss = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout,input_layout_count,rs.vs_blob,rs_color.fs_blob);
    ID3D12PipelineState* color_pipeline_state = D12RendererCode::CreatePipelineState(color_ppss);    

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

    //Camera setup
    RenderCamera rc = {};
    rc.ot.p = f3_create(0,0,0);
    rc.ot.r = f3_axis_angle(f3_create(0,0,1),0);
    rc.ot.s = f3_create(1,1,1);

    f32 aspect_ratio = ps->window.dim.x / ps->window.dim.y;
    f2 size = f2_create_f(300);
    size.x = size.x * aspect_ratio;
    rc.projection_matrix = init_ortho_proj_matrix(size,0.0f,1.0f);
    
    rc.fov = 80;
    rc.near_far_planes = f2_create(0,1);
    rc.matrix = f4x4_identity();
    u64 ortho_matrix_id = fmj_stretch_buffer_push(&matrix_buffer,(void*)&rc.projection_matrix);
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
    GPUArena quad_gpu_arena = D12RendererCode::AllocateStaticGPUArena(quad_mem_size);
    GPUArena ui_quad_gpu_arena = D12RendererCode::AllocateStaticGPUArena(quad_mem_size);    
    GPUArena matrix_gpu_arena = D12RendererCode::AllocateGPUArena(matrix_mem_size);

    FMJFixedBuffer fixed_quad_buffer = fmj_fixed_buffer_init(200,sizeof(SpriteTrans),8);
    ui_fixed_quad_buffer = fmj_fixed_buffer_init(200,sizeof(SpriteTrans),8);    
    FMJFixedBuffer matrix_quad_buffer = fmj_fixed_buffer_init(200,sizeof(f4x4),0);
//end memory setup

//ui definition    
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
    komas = fmj_stretch_buffer_init(1,sizeof(Koma),8);
    
    FMJSprite base_koma_sprite_2 = add_sprite_to_stretch_buffer(&asset_tables.sprites,base_render_material.id,2,uvs,white,true);
    FMJSprite koma_sprite_outer_3 = add_sprite_to_stretch_buffer(&asset_tables.sprites,base_render_material.id,rook_tex_id_range.y,uvs,white,true);

    FMJSprite pawn_sprite = add_sprite_to_stretch_buffer(&asset_tables.sprites,base_render_material.id,pawn_tex_id_range.x,uvs,white,true);
    FMJSprite pawn_sprite_outer_2 = add_sprite_to_stretch_buffer(&asset_tables.sprites,base_render_material.id,pawn_tex_id_range.y,uvs,white,true);

    FMJSprite queen_sprite = add_sprite_to_stretch_buffer(&asset_tables.sprites,base_render_material.id,queen_tex_id_range.x,uvs,white,true);
    FMJSprite queen_sprite_outer_6 = add_sprite_to_stretch_buffer(&asset_tables.sprites,base_render_material.id,queen_tex_id_range.y,uvs,white,true);

    FMJSprite king_sprite = add_sprite_to_stretch_buffer(&asset_tables.sprites,base_render_material.id,king_tex_id_range.x,uvs,white,true);
    FMJSprite king_sprite_outer_5 = add_sprite_to_stretch_buffer(&asset_tables.sprites,base_render_material.id,king_tex_id_range.y,uvs,white,true);
    
    FMJSprite circle_sprite = fmj_sprite_init(8,uvs,white,true);
    FMJSprite line_sprite = fmj_sprite_init(9,uvs,white,true);
    FMJSprite court_sprite = fmj_sprite_init(0,uvs,white,true);        
    circle_sprite.material_id = base_render_material.id;
    line_sprite.material_id = base_render_material.id;
    court_sprite.material_id = color_render_material.id;

    u64 circle_sprite_id = fmj_stretch_buffer_push(&asset_tables.sprites,(void*)&circle_sprite);
    u64 line_sprite_id   = fmj_stretch_buffer_push(&asset_tables.sprites,(void*)&line_sprite);
    u64 court_sprite_id   = fmj_stretch_buffer_push(&asset_tables.sprites,(void*)&court_sprite);    

    quaternion def_r = f3_axis_angle(f3_create(0,0,1),0);
    f32 court_size_y = 500;
    f32 court_size_x = line.dim.x;
    
    u64  court_id = sprite_trans_create(f3_create_f(0),f3_create(line.dim.x,court_size_y,1),def_r,white,court_sprite,&matrix_buffer,&fixed_quad_buffer,&sb.arena);
//    u64  court_id = sprite_trans_create(f3_create_f(0),f3_create(100,100,1),def_r,white,court_sprite_id,&matrix_buffer,&fixed_quad_buffer,&sb.arena);        
    u64  circle_id = sprite_trans_create(f3_create_f(0),f3_create(court_size_x*0.8f,court_size_x*0.8f,1),def_r,white,circle_sprite,&matrix_buffer,&fixed_quad_buffer,&sb.arena);    
    u64  line_id = sprite_trans_create(f3_create_f(0),f3_create(line.dim.x,line.dim.y,1),def_r,white,line_sprite,&matrix_buffer,&fixed_quad_buffer,&sb.arena);

    f2  top_right_screen_xy = f2_create(ps->window.dim.x,ps->window.dim.y);
    f2  bottom_left_xy = f2_create(0,0);

    f3 max_screen_p = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,top_right_screen_xy,0);
    f3 lower_screen_p = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,bottom_left_xy,0);

    f32 half_court_x =  (f32)line.dim.x / 2.0f;
    f32 half_court_y = court_size_y / 2.0f;
    f32 court_slot_x = court_size_x / 6.0f;

    f32 x_offset = 0;
    u64 rot_test_st_id;

    f3 koma_bottom_left = f3_create(-half_court_x + court_slot_x,-half_court_y + court_slot_x,0);
    f3 koma_top_right = f3_create(-half_court_x + court_slot_x,half_court_y - court_slot_x,0);        
    f3 start_pos = f3_create_f(0);
    f3 current_pos = f3_create(0,0,-5);
    u32 player_id = 0;
    for(int o = 0;o < 2;++o)
    {
        if(o == 0)
        {
            start_pos = koma_bottom_left;            
        }
        else
        {
            player_id = 1;
            def_r = f3_axis_angle(f3_create(0,0,1),180);
            start_pos = koma_top_right;            
        }

        f3 current_pos = start_pos;
        for(int i = 0;i < 10;++i)
        {
            f32 base_scale = 38;
            f32 scale_mul = 1.0f;
            f3 next_p = current_pos;        
            Koma k = {0};
            
            FMJSprite inner_sprite;
            FMJSprite outer_sprite;
            k.team_id = i;
            k.player_id = player_id;
            if(i > 4)//pawns
            {
                inner_sprite = pawn_sprite;
                outer_sprite = pawn_sprite_outer_2;
                k.atk_pow = 1;
                k.max_hp = 2;
                k.hp = k.max_hp;
                k.type = koma_type_pawn;
            }
            else if(i == 1 || i == 3)//queen
            {
                inner_sprite = queen_sprite;
                outer_sprite = queen_sprite_outer_6;
                scale_mul = 1.3f;
                k.atk_pow = 1;
                k.max_hp = 6;
                k.hp = k.max_hp;
                k.type = koma_type_queen;
            }
            else if(i == 2)//king
            {
                inner_sprite = king_sprite;
                outer_sprite = king_sprite_outer_5;
                scale_mul = 1.0f;
                k.atk_pow = 1;
                k.max_hp = 5;
                k.hp = k.max_hp;
                k.type = koma_type_king;
            }
            else if(i == 0 || i == 4)//rooks
            {
                inner_sprite = base_koma_sprite_2;
                outer_sprite = koma_sprite_outer_3;
                k.atk_pow = 2;
                k.max_hp = 3;
                k.hp = k.max_hp;
                k.type = koma_type_rook;
            }
            else
            {
                ASSERT(false);
            }
            
            f32 scale = base_scale * scale_mul;
            u64  inner_id = sprite_trans_create(next_p,f3_create(scale,scale,1),def_r,white,inner_sprite,&matrix_buffer,&fixed_quad_buffer,&sb.arena);
            k.inner_id = inner_id;

            base_scale += 8;
            scale = base_scale * scale_mul;
            u64  outer_id = sprite_trans_create(next_p,f3_create(scale,scale,1),def_r,white,outer_sprite,&matrix_buffer,&fixed_quad_buffer,&sb.arena);
            k.outer_id = outer_id;

//        filterData.word0 = k.type; // word0 = own ID
//        filterData.word1 = 0xFF;  // word1 = ID mask to filter pairs that trigger a
            PhysicsShapeSphere sphere_shape = PhysicsCode::CreateSphere(scale/2,physics_material);
            PhysicsCode::SetSimulationFilterData(((PxShape*)sphere_shape.shape),k.type,0xFF);
            
            RigidBody rbd = PhysicsCode::CreateDynamicRigidbody(next_p, sphere_shape.shape, false);

            PhysicsCode::AddActorToScene(scene, rbd);
            PhysicsCode::DisableGravity((PxActor*)rbd.state,true);
            k.rigid_body = rbd;
            PhysicsCode::UpdateRigidBodyMassAndInertia(k.rigid_body,1);
            PhysicsCode::SetMass(k.rigid_body,1);

            // contact callback;
            u64 koma_index = fmj_stretch_buffer_push(&komas,(void*)&k);
            u64* kpp = (u64*)koma_index;
            PhysicsCode::SetRigidBodyUserData(rbd,kpp);            

            PhysicsCode::SetRigidDynamicLockFlag(rbd,PxRigidDynamicLockFlag::eLOCK_LINEAR_Z,true);
//            PhysicsCode::LockRigidRotation(rbd);

            if(0 == (i + 1) % 5)
            {
                if(o == 0)
                    current_pos = f3_add(current_pos,f3_create(0,court_slot_x,0));
                else
                    current_pos = f3_sub(current_pos,f3_create(0,court_slot_x,0));
            
                current_pos = f3_create(start_pos.x,current_pos.y,0);
            }
            else
            {
                current_pos = f3_add(current_pos,f3_create(court_slot_x,0,0));
            }        
        }
    }    

//create court bounds physics
    for(int i = 0;i < 4;++i)
    {
        f32 scale = 19;
        f3 box_size;
        f3 next_p; 
        //right left boxes
        if(i % 2 == 0)
        {
            box_size = f3_create(1.0f,court_size_y + 1,scale);
            if(i == 0)//left
                next_p =  f3_create(-(court_size_x / 2.0f),0,0);
            if(i == 2)//right
                next_p =  f3_create((court_size_x / 2.0f),0,0);
        }
        else //top bottom
        {
            box_size = f3_create(court_size_x + 1.0f,1.0f,scale);
            if(i == 1)//top
                next_p =  f3_create(0,-(court_size_y / 2.0f),0);
            if(i == 3)//bottom
                next_p =  f3_create(0,(court_size_y / 2.0f),0);
        }

        PhysicsShapeBox box_shape = PhysicsCode::CreateBox(box_size,physics_material);
        RigidBody rbd = PhysicsCode::CreateStaticRigidbody(next_p, box_shape.shape);
        PhysicsCode::AddActorToScene(scene, rbd);
        PhysicsCode::DisableGravity((PxActor*)rbd.state,true);
    }
//end game object setup

    //ui  evaulation
    fmj_ui_evaluate_node(&base_node,&ui_state.hot_node_state);
    fmj_ui_commit_nodes_for_drawing(&sb_ui.arena,base_node,&ui_fixed_quad_buffer,white,uvs);
//end ui evaulation

    //upload data to gpu
    D12RendererCode::SetArenaToVertexBufferView(&quad_gpu_arena,quad_mem_size,stride);
    D12RendererCode::SetArenaToVertexBufferView(&ui_quad_gpu_arena,quad_mem_size,stride);    
    FMJRenderGeometry geo  = {0};
    geo.id = fmj_stretch_buffer_push(&asset_tables.vertex_buffers,(void*)&quad_gpu_arena.buffer_view);    

    FMJRenderGeometry ui_geo  = {0};    
    ui_geo.id = fmj_stretch_buffer_push(&asset_tables.vertex_buffers,(void*)&ui_quad_gpu_arena.buffer_view);

    fmj_anycache_add_to_free_list(&asset_tables.geometries,(void*)&geo.id,(void*)&geo);
    fmj_anycache_add_to_free_list(&asset_tables.geometries,(void*)&ui_geo.id,(void*)&ui_geo);

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

        f4x4* p_mat_ = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,ortho_matrix_id);
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
        
        if(ps->input.mouse.lmb.down && !game_state.is_phyics_active)
        {
            //Begin the pull
            finger_pull.pull_begin = true;
            if(!finger_pull.koma)
            {
                //find the hovered unit
                PxScene* cs = scene.state;
                f3 origin_3 = f3_screen_to_world_point(p_mat,rc.matrix,ps->window.dim,ps->input.mouse.p,0);                
                PxVec3 origin = PxVec3(origin_3.x,origin_3.y,origin_3.z);
                //PxVec3 origin = PxVec3(ps->input.mouse.p.x,ps->input.mouse.p.y,0);
                f3 unit_dir_ = f3_create(0,0,-1);
                PxVec3 unitDir = PxVec3(unit_dir_.x,unit_dir_.y,unit_dir_.z);
                PxReal maxDistance = 5.0f;                // [in] Raycast max distance
                PxRaycastBuffer hit;                      // [out] Raycast results
                // Raycast against all static & dynamic objects (no filtering)
                // The main result from this call is the closest hit, stored in the 'hit.block' structure
                bool status = cs->raycast(origin, unitDir, maxDistance, hit);
                if(status)
                {
                    for(int i = 0;i < komas.fixed.count;++i)
                    {
                        Koma* koma = fmj_stretch_buffer_check_out(Koma,&komas,i);
                        //For each piece check inside sphere radius if start touch is in side piece radius
//                        Koma* gp = &GamePieceCode::game_pieces[i];
                        if( hit.block.actor == koma->rigid_body.state && koma->hp > 0)
                        {
                            finger_pull.koma = koma;
                            finger_pull.start_pull_p = ps->input.mouse.p;
                            break;
                        }
                        //set gp
                        fmj_stretch_buffer_check_in(&komas);
                    }
                }
            }
            //if no unit hovered ignore pull
            //else show pull graphic over piece
        }
        else
        {
            if(finger_pull.pull_begin)
            {
                finger_pull.end_pull_p  = ps->input.mouse.p;
                if(!CheckValidFingerPull(&finger_pull) || !finger_pull.koma || finger_pull.koma->player_id != game_state.current_player_id)
                {
                    finger_pull.pull_begin = false;
                    finger_pull.koma = nullptr;
                    finger_pull.start_pull_p = f2_create(0,0);
                    finger_pull.end_pull_p = f2_create(0,0);
                    finger_pull.pull_begin = false;                    
                }
                
                else if(finger_pull.pull_begin && finger_pull.koma)
                {
                    //Pull was valid
                    f32 max_pull_length  = 20;
                    finger_pull.koma->action_type = koma_action_type_attack;
                    f2 pull_dif = f2_sub(finger_pull.end_pull_p,finger_pull.start_pull_p);
                    f3 flick_dir = f3_create(pull_dif.x,pull_dif.y,0);
                    f32 flick_dir_length = fmin(f3_length(flick_dir),max_pull_length);
                    flick_dir = f3_normalize(f3_create(flick_dir.x,flick_dir.y,0));
                    f32  scale_down_speed = 12.0f;
                    f32 flick_speed = flick_dir_length * finger_pull.koma->max_speed * scale_down_speed;
                    flick_dir = f3_mul_s(flick_dir,flick_speed);
                    PxVec3 pulldirpx3 = PxVec3(flick_dir.x,flick_dir.y,flick_dir.z);
                    
//                    ((PxRigidDynamic*)finger_pull.koma->rigid_body.state)->setLinearVelocity(pulldirpx3);
                    ((PxRigidDynamic*)finger_pull.koma->rigid_body.state)->addForce(pulldirpx3,	PxForceMode::Enum::eVELOCITY_CHANGE);

                    game_state.flicked_koma = finger_pull.koma;
                    finger_pull.koma = nullptr;
                    finger_pull.start_pull_p = f2_create(0,0);
                    finger_pull.end_pull_p = f2_create(0,0);
                    finger_pull.pull_begin = false;                    
                }
                
                else
                {
                    finger_pull.koma = nullptr;
                    finger_pull.start_pull_p = f2_create(0,0);
                    finger_pull.end_pull_p = f2_create(0,0);
                    finger_pull.pull_begin = false;
                }
            }
        }

        //Updte our Koma game pieces on moving bodies
        f32 komas_total_linear_velocity = 0.0f;        
        for(int i = 0;i < komas.fixed.count;++i)
        {
            Koma* k = fmj_stretch_buffer_check_out(Koma,&komas,i);
            
            if(k->action_type != koma_action_type_attack)
            {
                PhysicsCode::LockRigidPosition(k->rigid_body);                
            }
            else
            {
                PhysicsCode::UnlockRigidPosition(k->rigid_body);
            }

            PxVec3 lv = ((PxRigidDynamic*)k->rigid_body.state)->getLinearVelocity();
            
            f32 koma_vel_len = f3_length(f3_create(lv.x,lv.y,lv.z));

            komas_total_linear_velocity += koma_vel_len;            
            lv *= ground_friction;
            if(k->hp > 0)
                ((PxRigidDynamic*)k->rigid_body.state)->addForce(-lv,	PxForceMode::Enum::eIMPULSE);
//                ((PxRigidDynamic*)k->rigid_body.state)->setLinearVelocity(lv);
            
            u64 outer_sprite_id;
            u32 j = k->team_id;
            u32 t_id;
            f32 rook_max_speed = 30.0f;
            f32 pawn_max_speed = 20.0f;
            f32 queen_max_speed = 38.0f;
            f32 king_max_speed = 10.0f;
//            f32 base_friction = ground_friction;
            
            if(j > 4)//pawn
            {
                u32 start = k->hp + pawn_tex_id_range.x;
                t_id = clamp(start,pawn_tex_id_range.x + 1,pawn_tex_id_range.y);
                k->max_speed = pawn_max_speed;
            }
            else if(j == 1 || j == 3)//queen
            {
                u32 start = k->hp + queen_tex_id_range.x;
                t_id = clamp(start,queen_tex_id_range.x + 1,queen_tex_id_range.y);
                k->max_speed = queen_max_speed;
            }
            else if(j == 2)//king
            {
                u32 start = k->hp + king_tex_id_range.x;
                t_id = clamp(start,king_tex_id_range.x + 1,king_tex_id_range.y);
                k->max_speed = king_max_speed;
            }
            else if(j == 0 || j == 4)
            {
                u32 start = k->hp + rook_tex_id_range.x;
                t_id = clamp(start,rook_tex_id_range.x + 1,rook_tex_id_range.y);
                k->max_speed = rook_max_speed;
            }
            else
            {
                ASSERT(false);
            }
            
            outer_sprite_id = t_id;            

            SpriteTrans* inner_koma_st = fmj_fixed_buffer_get_ptr(SpriteTrans,&fixed_quad_buffer,k->inner_id);
            SpriteTrans* outer_koma_st = fmj_fixed_buffer_get_ptr(SpriteTrans,&fixed_quad_buffer,k->outer_id);
            FMJ3DTrans  new_trans =  inner_koma_st->t;

            f4x4 c_mat = fmj_stretch_buffer_get(f4x4,&matrix_buffer,rc_matrix_id);
            
            f3 p = inner_koma_st->t.p;

            PxTransform pxt = ((PxRigidDynamic*)k->rigid_body.state)->getGlobalPose();
            f3 new_p = f3_create(pxt.p.x,pxt.p.y,pxt.p.z);
            quaternion new_r = quaternion_create(pxt.q.x,pxt.q.y,pxt.q.z,pxt.q.w);
            
            inner_koma_st->t.p = new_p;
            outer_koma_st->t.p = inner_koma_st->t.p;
            outer_koma_st->t.r = new_r;//f3_axis_angle(f3_create(0,0,1),outer_angle_rot);
            outer_angle_rot += 0.01f;
            fmj_3dtrans_update(&outer_koma_st->t);
            fmj_3dtrans_update(&inner_koma_st->t);

            FMJSprite is = inner_koma_st->sprite;//fmj_stretch_buffer_check_out(FMJSprite,&asset_tables.sprites,inner_koma_st->sprite_id);            
            FMJSprite s = outer_koma_st->sprite;//fmj_stretch_buffer_check_out(FMJSprite,&asset_tables.sprites,outer_koma_st->sprite_id);
            if(k->hp <= 0)
            {
                outer_koma_st->sprite.is_visible = false;
                inner_koma_st->sprite.is_visible = false;
            }
            outer_koma_st->sprite.tex_id = outer_sprite_id;

            //TODO(Ray):DOO this when we change turns 
            if(k->hp <= 0)
            {

                PhysicsCode::SetFlagForActor((PxActor*)k->rigid_body.state,PxActorFlag::Enum::eDISABLE_SIMULATION ,true);
            }
            
            f4x4* outer_model_matrix = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,outer_koma_st->model_matrix_id);
            *outer_model_matrix = outer_koma_st->t.m;
            f4x4* inner_model_matrix = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,inner_koma_st->model_matrix_id);
            *inner_model_matrix = inner_koma_st->t.m;
            
        }

        if(komas_total_linear_velocity <= 6.5f)
        {
            if(game_state.is_phyics_active)
            {
                game_state.current_player_id = (game_state.current_player_id + 1) % 2;
                if(game_state.flicked_koma)
                {
                    game_state.flicked_koma->action_type = koma_action_type_defense;
                    game_state.flicked_koma = nullptr;                
                }
            }
            game_state.is_phyics_active = false;
        }
        else
        {
            game_state.is_phyics_active = true;
        }
        
        if(ps->input.mouse.lmb.released)
        {
            show_title = false;
            fmj_ui_evaluate_on_node_recursively(&base_node,SetSpriteNonVisible);
        }
        
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
        for(int i = 0;i < fixed_quad_buffer.count;++i)
        {
            FMJRenderCommand com = {};
            SpriteTrans st = fmj_fixed_buffer_get(SpriteTrans,&fixed_quad_buffer,i);
            FMJSprite s = st.sprite;//fmj_stretch_buffer_get(FMJSprite,&asset_tables.sprites,st.sprite_id);
            if(s.is_visible)
            {
                geo.offset = (i * 6);
                com.geometry = geo;
                com.material_id = s.material_id;
                com.texture_id = s.tex_id;
                com.model_matrix_id = st.model_matrix_id;
                com.camera_matrix_id = rc_matrix_id;
                com.perspective_matrix_id = ortho_matrix_id;
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
                ui_geo.offset = (i * 6);
                com.geometry = ui_geo;
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
                    D3D12_VERTEX_BUFFER_VIEW bv = fmj_stretch_buffer_get(D3D12_VERTEX_BUFFER_VIEW,&asset_tables.vertex_buffers,command.geometry.id);
                    D12RendererCode::AddDrawCommand(command.geometry.offset,6,D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,bv);
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
