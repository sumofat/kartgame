#define KART_GAME

#include<windows.h>
#include <Xinput.h>
#include <dsound.h>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <xinput.h>

extern "C"{
#include "fmj/src/fmj_types.h"
}

#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_widgets.cpp"
#include "../external/imgui/imgui.cpp"
#include "../external/imgui/imgui_demo.cpp"
#include "../external/imgui/imgui_draw.cpp"
#include "../external/imgui/examples/imgui_impl_win32.h"
#include "../external/imgui/examples/imgui_impl_win32.cpp"
#include "../external/imgui/examples/imgui_impl_dx12.h"
#include "../external/imgui/examples/imgui_impl_dx12.cpp"

#include "directx12.h"
#include "util.h"

#include "sound_interface.h"
#include "sound_interface.cpp"

struct GOHandle
{
    u64 index;
    FMJStretchBuffer* buffer;
    u32 type;//0 none 1 car
}typedef;

struct AssetTables
{
    AnyCache materials;
    u64 material_count;
    FMJStretchBuffer sprites;
    FMJStretchBuffer textures;
    FMJStretchBuffer vertex_buffers;
    FMJStretchBuffer index_buffers;
    FMJStretchBuffer matrix_buffer;
    FMJStretchBuffer meshes;

}typedef AssetTables;

struct FMJAssetContext
{
    FMJMemoryArena* perm_mem;
    FMJMemoryArena* temp_mem;
    AssetTables* asset_tables;

    FMJStretchBuffer scene_objects;
    AnyCache so_to_go;
}typedef FMJAssetContext;

#include "fmj_scene.h"
#include "fmj_scene_manager.h"
#include "assets.h"

#include "physics.h"
#include "physics.cpp"

PhysicsShapeMesh CreatePhysicsMeshShape(FMJAssetMesh* mesh,PhysicsMaterial material)
{
    PhysicsShapeMesh result;
    if(mesh->index_component_size == fmj_asset_index_component_size_none)
    {
        result = PhysicsCode::CreatePhyshicsMeshShape(mesh->vertex_data,mesh->vertex_count,0,0,0,material);           
    }
    else if(mesh->index_component_size == fmj_asset_index_component_size_32)
    {
        result = PhysicsCode::CreatePhyshicsMeshShape(mesh->vertex_data,mesh->vertex_count,mesh->index_32_data,mesh->index32_count,fmj_asset_index_component_size_32,material);
    }
    else if(mesh->index_component_size == fmj_asset_index_component_size_16)
    {
        result = PhysicsCode::CreatePhyshicsMeshShape(mesh->vertex_data,mesh->vertex_count,mesh->index_16_data,mesh->index16_count,fmj_asset_index_component_size_16,material);
    }
    else
    {
        ASSERT(false);
    }
    return result;
}


#include "platform_win32.h"

FMJAssetContext asset_ctx = {};

FMJFixedBuffer ui_fixed_quad_buffer;


void SetSpriteNonVisible(void* node)
{
    FMJUINode* a = (FMJUINode*)node;
//    SpriteTrans* st = fmj_fixed_buffer_get_ptr(SpriteTrans,&ui_fixed_quad_buffer,a->st_id);    
//    st->sprite.is_visible = false;
}

#include "assets.cpp"
#include "camera.h"
#include "animation.h"
#include "fmj_sprite.h"
#include "renderer.h"
//Game files

RenderCamera rc = {};
RenderCamera rc_ui = {};

enum GameObjectType
{
    go_type_none,
    go_type_kart,
    go_type_track,
};

static SoundClip bgm_soundclip;
PhysicsMaterial physics_material;
f32 ground_friction = 0.0032f;
bool prev_lmb_state = false;

#ifdef KART_GAME
#include "kartgame.h"
#else
#include "two_d_test_game.h"
#endif

//End game files

#include "editor.h"
#include "editor_scene_objects.h"

//BEGIN FMJSCENE TEST

FMJSceneBuffer scenes;
FMJRender render;

void fmj_scene_process_children_recrusively(FMJSceneObject* so,u64 c_mat,u64 p_mat,FMJAssetContext* ctx)
{
    for(int i = 0;i < so->children.buffer.fixed.count;++i)
    {
        u64 child_so_id = fmj_stretch_buffer_get(u64,&so->children.buffer,i);
        FMJSceneObject* child_so = fmj_stretch_buffer_check_out(FMJSceneObject,&ctx->scene_objects,child_so_id);
        fmj_3dtrans_update(&child_so->transform);

        if(child_so->type != 0)//non mesh type
        {
            f4x4* final_mat = fmj_stretch_buffer_check_out(f4x4,&ctx->asset_tables->matrix_buffer,child_so->m_id);
            *final_mat = child_so->transform.m;
            fmj_stretch_buffer_check_in(&ctx->asset_tables->matrix_buffer);
            
            for(int m = child_so->primitives_range.x;m <= child_so->primitives_range.y;++m)
            {
                u64 mesh_id = (u64)m;            
                FMJAssetMesh* m_ = fmj_stretch_buffer_check_out(FMJAssetMesh,&ctx->asset_tables->meshes,mesh_id);
                if(m_)
                {
                    FMJAssetMesh  mesh = *m_;
                    //get mesh issue render command
                    FMJRenderCommand com = {};        
                    FMJRenderGeometry geo = {};
                    if(mesh.index32_count > 0)
                    {
                        com.is_indexed = true;
                        geo.buffer_id_range = mesh.mesh_resource.buffer_range;
                        geo.index_id = mesh.mesh_resource.index_id;
                        geo.index_count = mesh.index32_count;                
                    }
                    else if(mesh.index16_count > 0)
                    {
                        com.is_indexed = true;
                        geo.buffer_id_range = mesh.mesh_resource.buffer_range;
                        geo.index_id = mesh.mesh_resource.index_id;
                        geo.index_count = mesh.index16_count;                
                    }
                    else
                    {
                        com.is_indexed = false;
                        geo.buffer_id_range = mesh.mesh_resource.buffer_range;
                        geo.index_id = mesh.mesh_resource.index_id;
                        geo.count = mesh.vertex_count;
                    }
            
                    geo.offset = 0;
                    geo.base_color = mesh.base_color;
                    com.geometry = geo;

                    com.material_id = mesh.material_id;
                    com.texture_id = mesh.metallic_roughness_texture_id;
                    com.model_matrix_id = child_so->m_id;
                    com.camera_matrix_id = c_mat;
                    com.perspective_matrix_id = p_mat;
                    fmj_stretch_buffer_push(&render.command_buffer,(void*)&com);
                    fmj_stretch_buffer_check_in(&ctx->asset_tables->meshes);                    
                }                
            }
        }

        fmj_scene_process_children_recrusively(child_so,c_mat,p_mat,ctx);
        fmj_stretch_buffer_check_in(&ctx->scene_objects);
    }
}

void fmj_scene_issue_render_commands(FMJScene* s,FMJAssetContext* ctx,u64 c_mat,u64 p_mat)
{
    //Start at root node
    for(int i = 0;i < s->buffer.buffer.fixed.count;++i)
    {
        u64 so_id = fmj_stretch_buffer_get(u64,&s->buffer.buffer,i);        
        FMJSceneObject* so = fmj_stretch_buffer_check_out(FMJSceneObject,&ctx->scene_objects,so_id);
//        FMJSceneObject* so = fmj_stretch_buffer_check_out(FMJSceneObject,&s->buffer.buffer,i);
//        if(so->type == scene_object_type_mesh)

        if(so->children.buffer.fixed.count > 0)
        {
            fmj_scene_process_children_recrusively(so,c_mat,p_mat,ctx);
        }
        fmj_stretch_buffer_check_in(&ctx->scene_objects);
    }
}
//END FMJSCENETEST

AssetTables asset_tables = {0};
u64 material_count = 0;

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


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer bindings
    ImGui_ImplWin32_Init(ps->window.handle);
    
    
#define NUM_FRAMES_IN_FLIGHT 2

    ID3D12DescriptorHeap* g_pd3dSrvDescHeap = NULL;
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
    }

    ImGui_ImplDX12_Init(device, NUM_FRAMES_IN_FLIGHT,
                        DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());
    
    //init tables
    fmj_asset_init(&asset_tables);


    
    //Load a texture with all mip maps calculated
    //load texture to mem from disk
    LoadedTexture ping_title = get_loaded_image("ping_title.png",4);
    D12RendererCode::Texture2D(&ping_title,3);
    
    FMJMemoryArena permanent_strings = fmj_arena_allocate(FMJMEGABYTES(1));
    FMJMemoryArena temp_strings = fmj_arena_allocate(FMJMEGABYTES(1));    
    FMJString base_path_to_data = fmj_string_create("../data/Desktop/",&permanent_strings);

    asset_ctx.scene_objects = fmj_stretch_buffer_init(100,sizeof(FMJSceneObject),8);
    //NOTE(Ray):We should make it the largest size of al the gameobjects to use it with any game object.
    asset_ctx.so_to_go = fmj_anycache_init(1024,sizeof(GOHandle),sizeof(FMJSceneObject*),false);

    //TODO(Ray):The pointers here are not good we need to init this somehwere else.
    asset_ctx.asset_tables = &asset_tables;
    asset_ctx.perm_mem = &permanent_strings;
    asset_ctx.temp_mem = &temp_strings;    

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

    render = fmj_render_init(ps);    

    //Camera setup
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

    FMJStretchBuffer* matrix_buffer = &asset_tables.matrix_buffer;
    u64 projection_matrix_id = fmj_stretch_buffer_push(matrix_buffer,(void*)&rc.projection_matrix);
    u64 rc_matrix_id = fmj_stretch_buffer_push(matrix_buffer,(void*)&rc.matrix);
    rc.projection_matrix_id = projection_matrix_id;
    rc.matrix_id = rc_matrix_id;

    rc_ui.ot.p = f3_create(0,0,0);
    rc_ui.ot.r = f3_axis_angle(f3_create(0,0,1),0);
    rc_ui.ot.s = f3_create(1,1,1);
    rc_ui.projection_matrix = init_screen_space_matrix(ps->window.dim);
    rc_ui.matrix = f4x4_identity();
    
    u64 screen_space_matrix_id = fmj_stretch_buffer_push(matrix_buffer,(void*)&rc_ui.projection_matrix);
    u64 identity_matrix_id = fmj_stretch_buffer_push(matrix_buffer,(void*)&rc_ui.matrix);
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

    FMJRenderMaterial ui_sprite_material = fmj_asset_get_material_by_id(&asset_ctx,1);
    ui_sprite.material_id = ui_sprite_material.id;
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

    FMJRenderMaterial base_render_material = fmj_asset_get_material_by_id(&asset_ctx,0);
    ui_sprite.material_id = base_render_material.id;
    title_child.sprite = ui_sprite;
    
    u64 ping_ui_id = fmj_stretch_buffer_push(&bkg_child.children,&title_child);
    u64 bkg_ui_id = fmj_stretch_buffer_push(&base_node.children,&bkg_child);    
//end ui definition

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

//BEGIN SCENE Management
    InitSceneBuffer(&scenes);
    u64 scene_id = CreateEmptyScene(&scenes);
    FMJScene* test_scene = fmj_stretch_buffer_check_out(FMJScene,&scenes.buffer,scene_id);
    FMJString object_name = fmj_string_create("Root Node",asset_ctx.perm_mem);
    u64 root_node_id = AddSceneObject(&asset_ctx,&test_scene->buffer,f3_create_f(0),quaternion_identity(),f3_create_f(1),object_name);
    FMJSceneObject* root_node = fmj_stretch_buffer_check_out(FMJSceneObject,&asset_ctx.scene_objects,root_node_id);
    fmj_scene_object_buffer_init(&root_node->children);
    FMJ3DTrans root_t;
    fmj_3dtrans_init(&root_t);
    root_node->transform = root_t;    
    fmj_stretch_buffer_check_in(&asset_ctx.scene_objects);

    scene_manager = {0};
    scene_manager.current_scene = test_scene;
    scene_manager.root_node_id = root_node_id;
//End Scene management

    bool show_demo_window = true;
    bool show_kart_editor = true;
    bool show_editor_scene = true;
    FMJEditorSceneTree scene_tree;

    fmj_editor_scene_tree_init(&scene_tree);
    FMJEditorConsole console;
    fmj_editor_console_init(&console);
    console.is_showing = true;    
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

#ifdef KART_GAME
    kart_game_setup(ps,rc,&asset_tables,permanent_strings,temp_strings);
#else
    two_d_test_game_setup(ps,rc,&asset_tables,permanent_strings,temp_strings);
#endif

    u32 tex_index = 0;    

    //game loop
    while(ps->is_running)
    {
//pull this frames info from devices//api
        PullDigitalButtons(ps);
        PullMouseState(ps);
        PullGamePads(ps);
        PullTimeState(ps);
//End pull
#ifdef KART_GAME
        kart_game_update(ps,rc,&asset_tables,permanent_strings,temp_strings);
#else
 two_d_test_game_update(ps,rc,&asset_tables,permanent_strings,temp_strings);
#endif

 #if 0
        //stop showing title  after mouse button is pressed
        if(ps->input.mouse.lmb.released)
        {
            show_title = false;
//            fmj_ui_evaluate_on_node_recursively(&base_node,SetSpriteNonVisible);
        }
#endif
        
        //EDITOR / IMGUI
 // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

         // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);            
        }

        if(show_kart_editor)
        {
            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("Windows"))
                {
                    if (ImGui::MenuItem("SceneObjects"))
                    {
                        fmj_editor_console_add_entry(&console,"Start showing scene objects window");
                        scene_tree.is_showing = true;
                    }

                    if(ImGui::MenuItem("Log"))
                    {
                        console.is_showing = true;
                    }                    
                    ImGui::EndMenu();                
                }
                ImGui::EndMainMenuBar();            
            }
        }

        if(scene_tree.is_showing)
        {
//            FMJSceneObject* start_node = fmj_stretch_buffer_check_out(FMJSceneObject,&asset_ctx.scene_objects,root_node_id);                
            fmj_editor_scene_tree_show(&scene_tree,scene_manager.current_scene,&asset_ctx);
//            fmj_stretch_buffer_check_in(&asset_ctx.scene_objects);
        }
        
        if(console.is_showing)
        {
            fmj_editor_console_show(&console,&asset_ctx);
        }

        FMJFileReadResult read_result = fmj_file_platform_read_entire_file("curve1");
        if(read_result.content_size > 0)
        {
            FMJCurves* read_back_curves = (FMJCurves*)read_result.content;
            f32 acr = fmj_animation_curve_evaluate(console.curves_output,-4.0f);
            int a = 0;
        }
        //END EDITOR IMGUI

//Render command creation(Render pass setup)
        UpdateScene(&asset_ctx,test_scene);
        fmj_scene_issue_render_commands(test_scene,&asset_ctx,rc_matrix_id,projection_matrix_id);
        
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
                fmj_stretch_buffer_push(&render.command_buffer,(void*)&com);
            }            
        }

#if 0 
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
                fmj_stretch_buffer_push(&render.command_buffer,(void*)&com);
            }
        }
#endif
//Render command setup end

//Render commands issued to the render api
        bool has_update = false;

        if(render.command_buffer.fixed.count > 0)
        {
            D12RendererCode::AddStartCommandListCommand();                            
            //screen space cam
            for(int i = 0;i < render.command_buffer.fixed.count;++i)
            {
                FMJRenderCommand command = fmj_stretch_buffer_get(FMJRenderCommand,&render.command_buffer,i);
                {
                    f4x4 m_mat = fmj_stretch_buffer_get(f4x4,matrix_buffer,command.model_matrix_id);
                    f4x4 c_mat = fmj_stretch_buffer_get(f4x4,matrix_buffer,command.camera_matrix_id);
                    f4x4 proj_mat = fmj_stretch_buffer_get(f4x4,matrix_buffer,command.perspective_matrix_id);
                    f4x4 world_mat = f4x4_mul(c_mat,m_mat);
                    f4x4 finalmat = f4x4_mul(proj_mat,world_mat);
                    m_mat.c0.x = (f32)(matrix_quad_buffer.count * sizeof(f4x4));
                    m_mat.c0.y = 5.0f;
                    
                    f4 base_color = command.geometry.base_color;
                    
                    m_mat.c1 = base_color;
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
                    for(int j = command.geometry.buffer_id_range.x;j <= command.geometry.buffer_id_range.y;++j)
                    {
                        D3D12_VERTEX_BUFFER_VIEW bv = fmj_stretch_buffer_get(D3D12_VERTEX_BUFFER_VIEW,&asset_tables.vertex_buffers,j);
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

//Command are finalized and rendering is started.        
 // TODO(Ray Garner): Add render targets commmand
//        if(show_title)
 
//        DrawTest test = {cube,rc,gpu_arena,test_desc_heap};
//Post frame work is done.
        D12RendererCode::EndFrame(g_pd3dSrvDescHeap);

        fmj_stretch_buffer_clear(&render.command_buffer);
//END Render commands issued to the render api        


        SoundCode::Update();

        HandleWindowsMessages(ps);        
    }
    return 0;
}

