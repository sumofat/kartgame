struct FMJRender
{
    FMJStretchBuffer command_buffer;
}typedef FMJRender;

FMJRender fmj_render_init(PlatformState* ps)
{
    FMJRender result = {0};
    result.command_buffer = fmj_stretch_buffer_init(1,sizeof(FMJRenderCommand),8);

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
    fmj_asset_material_add(&asset_ctx,base_render_material);

    FMJRenderMaterial color_render_material = base_render_material;
    color_render_material.pipeline_state = (void*)color_pipeline_state;
    color_render_material.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    color_render_material.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);    
    fmj_asset_material_add(&asset_ctx,color_render_material);

    FMJRenderMaterial color_render_material_mesh = color_render_material;
    color_render_material_mesh.pipeline_state = (void*)color_pipeline_state_mesh;
    color_render_material_mesh.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    color_render_material_mesh.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);    
    fmj_asset_material_add(&asset_ctx,color_render_material_mesh);

    FMJRenderMaterial vu_render_material_mesh = color_render_material;
    vu_render_material_mesh.pipeline_state = (void*)vu_pipeline_state_mesh;
    vu_render_material_mesh.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    vu_render_material_mesh.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);    
    fmj_asset_material_add(&asset_ctx,vu_render_material_mesh);

    FMJRenderMaterial pn_render_material_mesh = color_render_material;
    pn_render_material_mesh.pipeline_state = (void*)pn_pipeline_state_mesh;
    pn_render_material_mesh.viewport_rect = f4_create(0,0,ps->window.dim.x,ps->window.dim.y);
    pn_render_material_mesh.scissor_rect = f4_create(0,0,LONG_MAX,LONG_MAX);    
    fmj_asset_material_add(&asset_ctx,pn_render_material_mesh);

    
    return result;
}



