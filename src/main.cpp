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

#include "physics.h"
#include "physics.cpp"

#include "platform_win32.h"

struct AssetTables
{
    AnyCache materials;
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
}typedef FMJAssetContext;

#include "fmj_scene.h"
#include "assets.h"

#include "koma.h"
#include "ping_game.h"

#include "assets.cpp"

#include "camera.h"

#include "animation.h"

#include "editor.h"
#include "editor_scene_objects.h"

//Game files
#include "carframe.h"
//End game files

//BEGIN FMJSCENE TEST

FMJSceneBuffer scenes;
FMJStretchBuffer render_command_buffer;

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
                    fmj_stretch_buffer_push(&render_command_buffer,(void*)&com);
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

    render_command_buffer = fmj_stretch_buffer_init(1,sizeof(FMJRenderCommand),8);
    
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
    physics_material = PhysicsCode::CreateMaterial(0.5f, 0.5f, 0.5f);
    PhysicsCode::SetRestitutionCombineMode(physics_material,physx::PxCombineMode::eMIN);
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
    
    D3D12_INPUT_ELEMENT_DESC input_layout_vu_mesh[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    
    D3D12_INPUT_ELEMENT_DESC input_layout_pn_mesh[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL"  , 0, DXGI_FORMAT_R32G32B32_FLOAT,   1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },        
    };
    
    uint32_t input_layout_count = _countof(input_layout);
    uint32_t input_layout_count_mesh = _countof(input_layout_mesh);
    uint32_t input_layout_vu_count_mesh = _countof(input_layout_vu_mesh);
    uint32_t input_layout_pn_count_mesh = _countof(input_layout_pn_mesh);            
    // Create a root/shader signature.

    char* vs_file_name = "vs_test.hlsl";
    char* vs_mesh_vu_name = "vs_vu.hlsl";
    char* vs_mesh_file_name = "vs_test_mesh.hlsl";
    char* vs_pn_file_name = "vs_pn.hlsl";
    
    char* fs_file_name = "ps_test.hlsl";
    char* fs_color_file_name = "fs_color.hlsl";
    RenderShader rs = D12RendererCode::CreateRenderShader(vs_file_name,fs_file_name);
    RenderShader mesh_rs = D12RendererCode::CreateRenderShader(vs_mesh_file_name,fs_file_name);
    RenderShader mesh_vu_rs = D12RendererCode::CreateRenderShader(vs_mesh_vu_name,fs_file_name);
    RenderShader rs_color = D12RendererCode::CreateRenderShader(vs_file_name,fs_color_file_name);
    RenderShader mesh_pn_color = D12RendererCode::CreateRenderShader(vs_pn_file_name,fs_color_file_name);
    
    PipelineStateStream ppss = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout,input_layout_count,rs.vs_blob,rs.fs_blob);
    ID3D12PipelineState* pipeline_state = D12RendererCode::CreatePipelineState(ppss);    

    PipelineStateStream color_ppss = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout,input_layout_count,rs.vs_blob,rs_color.fs_blob);
    ID3D12PipelineState* color_pipeline_state = D12RendererCode::CreatePipelineState(color_ppss);
    
    PipelineStateStream color_ppss_mesh = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout_mesh,input_layout_count_mesh,mesh_rs.vs_blob,mesh_rs.fs_blob,true);
    ID3D12PipelineState* color_pipeline_state_mesh = D12RendererCode::CreatePipelineState(color_ppss_mesh);

    PipelineStateStream vu_ppss_mesh = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout_vu_mesh,input_layout_vu_count_mesh,mesh_vu_rs.vs_blob,mesh_vu_rs.fs_blob,true);
    ID3D12PipelineState* vu_pipeline_state_mesh = D12RendererCode::CreatePipelineState(vu_ppss_mesh);        

    PipelineStateStream pn_ppss_mesh = D12RendererCode::CreateDefaultPipelineStateStreamDesc(input_layout_pn_mesh,input_layout_pn_count_mesh,mesh_pn_color.vs_blob,mesh_pn_color.fs_blob,true);
    ID3D12PipelineState* pn_pipeline_state_mesh = D12RendererCode::CreatePipelineState(pn_ppss_mesh);        
    
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

    FMJRenderMaterial vu_render_material_mesh = color_render_material;
    vu_render_material_mesh.pipeline_state = (void*)vu_pipeline_state_mesh;
    vu_render_material_mesh.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    vu_render_material_mesh.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);    
    vu_render_material_mesh.id = material_count;
    fmj_anycache_add_to_free_list(&asset_tables.materials,(void*)&material_count,&vu_render_material_mesh);    
    material_count++;

    FMJRenderMaterial pn_render_material_mesh = color_render_material;
    pn_render_material_mesh.pipeline_state = (void*)pn_pipeline_state_mesh;
    pn_render_material_mesh.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    pn_render_material_mesh.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);    
    pn_render_material_mesh.id = material_count;
    fmj_anycache_add_to_free_list(&asset_tables.materials,(void*)&material_count,&pn_render_material_mesh);    
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

    FMJStretchBuffer* matrix_buffer = &asset_tables.matrix_buffer;
    u64 projection_matrix_id = fmj_stretch_buffer_push(matrix_buffer,(void*)&rc.projection_matrix);
    u64 rc_matrix_id = fmj_stretch_buffer_push(matrix_buffer,(void*)&rc.matrix);
    rc.matrix_id = rc_matrix_id;
    
    RenderCamera rc_ui = {};
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
    
//game object setup 
    quaternion def_r = f3_axis_angle(f3_create(0,0,1),0);

    f2  top_right_screen_xy = f2_create(ps->window.dim.x,ps->window.dim.y);
    f2  bottom_left_xy = f2_create(0,0);

    f3 max_screen_p = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,top_right_screen_xy,0);
    f3 lower_screen_p = f3_screen_to_world_point(rc.projection_matrix,rc.matrix,ps->window.dim,bottom_left_xy,0);

    FMJAssetContext asset_ctx = {};
    asset_ctx.scene_objects = fmj_stretch_buffer_init(100,sizeof(FMJSceneObject),8);
    
    asset_ctx.asset_tables = &asset_tables;
    asset_ctx.perm_mem = &permanent_strings;
    asset_ctx.temp_mem = &temp_strings;    
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

//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/BrainStem.glb",pn_render_material_mesh.id);
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/2CylinderEngine.glb",pn_render_material_mesh.id);
    
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/Box.glb",color_render_material_mesh.id);    
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/Fox.glb",vu_render_material_mesh.id);
//    FMJAssetModelLoadResult test_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/Lantern.glb",color_render_material_mesh.id);
    FMJAssetModelLoadResult track_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/track.glb",color_render_material_mesh.id);
    FMJAssetModelLoadResult kart_model_result = fmj_asset_load_model_from_glb_2(&asset_ctx,"../data/models/kart.glb",color_render_material_mesh.id);        
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
    AddModelToSceneObjectAsChild(&asset_ctx,root_node_id,kart_instance_id,kart_trans);
    
    FMJ3DTrans duck_trans;
    fmj_3dtrans_init(&duck_trans);
    duck_trans.p = f3_create(-10,0,0);

    fmj_3dtrans_init(&duck_trans);
    duck_trans.p = f3_create(10,0,0);

    fmj_3dtrans_init(&duck_trans);
    duck_trans.p = f3_create(0,8,0);
    
    FMJSceneObject* track_so = fmj_stretch_buffer_check_out(FMJSceneObject,&asset_ctx.scene_objects,track_instance_id);
    track_so->name = fmj_string_create("track so",asset_ctx.perm_mem);
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

    FMJSceneObject track_physics_so_ = {};
    track_physics_so_.name = fmj_string_create("Track Physics",asset_ctx.perm_mem);
    track_physics_so_.transform = kart_trans;
    track_physics_so_.children.buffer = fmj_stretch_buffer_init(1,sizeof(u64),8);
    track_physics_so_.m_id = track_so->m_id;
    track_physics_so_.data = 0;
    track_physics_so_.type = 1;
    track_physics_so_.primitives_range = f2_create_f(tcm_id);
    
    u64 track_physics_id = fmj_stretch_buffer_push(&asset_ctx.scene_objects,&track_physics_so_);
    
    AddModelToSceneObjectAsChild(&asset_ctx,root_node_id,track_instance_id,track_physics_so_.transform);

    PhysicsShapeBox phyx_box_shape = PhysicsCode::CreateBox(f3_create(1.2f,0.2f,1.2f),physics_material);
        
    RigidBody track_rbd = PhysicsCode::CreateStaticRigidbody(track_trans.p,track_physics_mesh.state);        
    RigidBody kart_rbd = PhysicsCode::CreateDynamicRigidbody(kart_trans.p,phyx_box_shape.state,false);//kart_physics_mesh.state,true);

    PhysicsCode::AddActorToScene(scene, track_rbd);    
    PhysicsCode::AddActorToScene(scene, kart_rbd);

    PhysicsCode::UpdateRigidBodyMassAndInertia(kart_rbd,1);
    PhysicsCode::SetMass(kart_rbd,1);
    
//end game object setup

    //ui  evaulation
    fmj_ui_evaluate_node(&base_node,&ui_state.hot_node_state);
//    fmj_ui_commit_nodes_for_drawing(&sb_ui.arena,base_node,&ui_fixed_quad_buffer,white,uvs);
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
    CarFrameDescriptorBuffer car_descriptor_buffer;
    CarFrameDescriptor d = {};
    d.mass = 400;
    d.max_thrust = 2000;
    d.maximum_wheel_angle = 33;//degrees
    d.count = 1;
//    d.type = CarFrameType.test;
//    d.prefab = test_car_prefab;
    car_descriptor_buffer.descriptors = fmj_stretch_buffer_init(1,sizeof(CarFrameDescriptor),8);
    fmj_stretch_buffer_push(&car_descriptor_buffer.descriptors,&d);
    

//    kart_init_car_frames(&car_frame_buffer,car_descriptor_buffer,1);
    
    u32 tex_index = 0;
    f32 angle = 0;    

    f32 x_move = 0;
    bool show_title = true;
    f32 outer_angle_rot = 0;    
    u32 tex_id;
    f32 size_sin = 0;
    game_state.current_player_id = 0;

    bool show_demo_window = true;
    bool show_kart_editor = true;
    bool show_editor_scene = true;
    FMJEditorSceneTree scene_tree;

    fmj_editor_scene_tree_init(&scene_tree);
    FMJEditorConsole console;
    fmj_editor_console_init(&console);
    console.is_showing = true;    
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    f32 tset_num = 0.005f;
    //end game global  vars

    CarFrameDescriptorBuffer desc = {};
    desc.descriptors = fmj_stretch_buffer_init(1,sizeof(CarFrameDescriptor),8);
    
    CarFrameDescriptor cdd = {};
    cdd.mass = 400;
    cdd.max_thrust = 2000;
    cdd.maximum_wheel_angle = 33;//degrees
    cdd.count = 1;
    cdd.type = car_frame_type_none;
    //cdd.prefab = test_car_prefab;
    fmj_stretch_buffer_push(&desc.descriptors,&cdd);

#if 0
    CarFrameBuffer car_frame_buffer = {};
    kart_init_car_frames(&car_frame_buffer,desc,1);
#endif    
    
    //game loop
    while(ps->is_running)
    {
//pull this frames info from devices//api
        PullDigitalButtons(ps);
        PullMouseState(ps);
        PullGamePads(ps);
        PullTimeState(ps);
//End pull
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
            fmj_editor_scene_tree_show(&scene_tree,test_scene,&asset_ctx);
//            fmj_stretch_buffer_check_in(&asset_ctx.scene_objects);
        }
        if(console.is_showing)
        {
            fmj_editor_console_show(&console,&asset_ctx);
        }

//        if(console.curves_output.buffer.fixed.count > 0)
        {
            FMJFileReadResult read_result = fmj_file_platform_read_entire_file("curve1");
            if(read_result.content_size > 0)
            {
                FMJCurves* read_back_curves = (FMJCurves*)read_result.content;
                f32 acr = fmj_animation_curve_evaluate(console.curves_output,-4.0f);
                int a = 0;
            }
            
            int a = 0;
        }
        
        //END EDITOR IMGUI
        
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

//        test_model_result.scene_object.transform.local_p = f3_create(0,-10,-25);
//        test_model_result.scene_object.transform.local_r = f3_axis_angle(f3_create(0,0,1),angle);
//        test_model_result.scene_object->transform.p = f3_create(0,-10,-25);
//        root_node->transform.local_r = f3_axis_angle(f3_create(0,0,1),angle);
        FMJSceneObject* track_physics_so = fmj_stretch_buffer_check_out(FMJSceneObject,&asset_ctx.scene_objects,track_physics_id);
        PxTransform pxt = ((PxRigidDynamic*)track_rbd.state)->getGlobalPose();
        f3 new_p = f3_create(pxt.p.x,pxt.p.y,pxt.p.z);
        quaternion new_r = quaternion_create(pxt.q.x,pxt.q.y,pxt.q.z,pxt.q.w);
        track_physics_so->transform.local_p = new_p;
        track_physics_so->transform.local_r = new_r;
        fmj_stretch_buffer_check_in(&asset_ctx.scene_objects);

        FMJSceneObject* kart_physics_so = fmj_stretch_buffer_check_out(FMJSceneObject,&asset_ctx.scene_objects,kart_instance_id);
        pxt = ((PxRigidDynamic*)kart_rbd.state)->getGlobalPose();
        new_p = f3_create(pxt.p.x,pxt.p.y,pxt.p.z);
        new_r = quaternion_create(pxt.q.x,pxt.q.y,pxt.q.z,pxt.q.w);
        kart_physics_so->transform.local_p = new_p;
        kart_physics_so->transform.local_r = new_r;        
        fmj_stretch_buffer_check_in(&asset_ctx.scene_objects);

#if 0
        FMJCurves f16lc;
        FMJCurves f16rlc;
        FMJCurves sa_curve;
        f3 input_axis = f3_create_f(0);        
        UpdateCarFrames(ps,&car_frame_buffer,f16lc,f16rlc,sa_curve,ps->time.delta_seconds,input_axis,scene);
#endif

//        track_so->transform.local_r = quaternion_mul(f3_axis_angle(f3_create(0,1,0),angle),start_r);
//        track_so->transform.local_s = f3_create_f(100);        
        //duck_child_3->transform.local_r = f3_axis_angle(f3_create(0,1,0),angle);
        
        //      track_so_2->transform.local_r = f3_axis_angle(f3_create(0,1,0),angle);        
//        track_so->transform. = f3_create(-10,-10,-25);        
//        track_so->transform.r = 
        angle += 0.01f;
        
//        f4x4* tmt = fmj_stretch_buffer_check_out(f4x4,&matrix_buffer,test_mesh_model_matrix_id);        
//        *tmt = test_mesh_t.m;

        //modify camera matrix
        f4x4* p_mat_ = fmj_stretch_buffer_check_out(f4x4,matrix_buffer,projection_matrix_id);
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

        fmj_camera_orbit(&asset_ctx,&rc,ps->input,ps->time.delta_seconds,kart_physics_so->transform,2);
//        fmj_camera_chase(&asset_ctx,&rc,ps->input,ps->time.delta_seconds,kart_physics_so->transform,f3_create(0,3,-3));
//        fmj_camera_free(&asset_ctx,&rc,ps->input,ps->time.delta_seconds);
        
//End game code

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

//        fmj_fixed_buffer_clear(&ui_fixed_quad_buffer);        
//Command are finalized and rendering is started.        
 // TODO(Ray Garner): Add render targets commmand
//        if(show_title)
 
//        DrawTest test = {cube,rc,gpu_arena,test_desc_heap};
//Post frame work is done.
        D12RendererCode::EndFrame(g_pd3dSrvDescHeap);

        fmj_stretch_buffer_clear(&render_command_buffer);
//END Render commands issued to the render api        

        PhysicsCode::Update(&scene,1,ps->time.delta_seconds);                
        SoundCode::Update();

        HandleWindowsMessages(ps);        
    }
    return 0;
}
