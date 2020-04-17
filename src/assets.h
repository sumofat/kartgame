#if !defined(ASSETS_H)
#define CGLTF_IMPLEMENTATION
#include "../external/cgltf/cgltf.h"

struct FMJAssetContext
{
    FMJMemoryArena* perm_mem;
    FMJMemoryArena* temp_mem;
}typedef FMJAssetContext;

struct AssetTables
{
    AnyCache materials;
    FMJStretchBuffer sprites;
    FMJStretchBuffer vertex_buffers;
    FMJStretchBuffer index_buffers;    
}typedef AssetTables;

enum FMJAssetVertCompressionType
{
    fmj_asset_vert_compression_none
}typedef FMJAssetVertCompressionType;

enum FMJAssetIndexComponentSize
{
    fmj_asset_index_component_size_32,
    fmj_asset_index_component_size_16
}typedef FMJAssetIndexComponentSize;

struct FMJAssetMesh
{
    u32 id;
    FMJString name;
    FMJAssetVertCompressionType compression_type;
    
    float* vertex_data;
    u64 vertex_data_size;
    u64 vertex_count;
    float* tangent_data;
    u64 tangent_data_size;
    u64 tangent_count;
    float* bi_tangent_data;
    u64 bi_tangent_data_size;
    u64 bi_tangent_count;
    float* normal_data;
    u64 normal_data_size;
    u64 normal_count;
    float* uv_data;
    u64 uv_data_size;
    u64 uv_count;
    //NOTE(Ray):We are only support max two uv sets
    float* uv2_data;
    u64 uv2_data_size;
    u64 uv2_count;
    
    FMJAssetIndexComponentSize index_component_size;
    //TODO(Ray):These are seriously problematic and ugly will be re working these.
    u32* index_32_data;
    u64 index_32_data_size;
    u64 index32_count;
    uint16_t* index_16_data;
    u64 index_16_data_size;
    u64 index16_count;
    GPUMeshResource mesh_resource;    
    u32 material_id;
}typedef FMJMeshAsset;

struct FMJAssetModel
{
    u32 id;
    FMJString model_name;
    FMJStretchBuffer meshes;
}typedef FMJModelAsset;

void fmj_asset_init(AssetTables* asset_tables)
{
    asset_tables->materials = fmj_anycache_init(4096,sizeof(FMJRenderMaterial),sizeof(u64),true);                                  
    asset_tables->sprites = fmj_stretch_buffer_init(1,sizeof(FMJSprite),8);
    //NOTE(RAY):If we want to make this platform independendt we would just make a max size of all platofrms
    //struct and put in this and get out the opaque pointer and cast it to what we need.
    asset_tables->vertex_buffers = fmj_stretch_buffer_init(1,sizeof(D3D12_VERTEX_BUFFER_VIEW),8);
    asset_tables->index_buffers = fmj_stretch_buffer_init(1,sizeof(D3D12_INDEX_BUFFER_VIEW),8);        
}

FMJAssetModel fmj_asset_model_create(FMJAssetContext* ctx)
{
    FMJAssetModel model = {};
    model.meshes = fmj_stretch_buffer_init(1,sizeof(FMJAssetMesh),8);
    return model;
}

FMJAssetMesh fmj_asset_create_mesh_from_cgltf_mesh(FMJAssetContext* ctx,cgltf_mesh* ma)
{
    ASSERT(ma);
    FMJAssetMesh mesh = {};        
    //Extract mesh binary data
    for(int j = 0;j < ma->primitives_count;++j)
    {
        cgltf_primitive prim = ma->primitives[j];
        cgltf_material* mat  = prim.material;

        if(mat->normal_texture.texture)
        {
            cgltf_texture_view tv = mat->normal_texture;
        }

        if(mat->occlusion_texture.texture)
        {
            cgltf_texture_view tv = mat->occlusion_texture;
        }

        if(mat->emissive_texture.texture)
        {
            cgltf_texture_view tv = mat->emissive_texture;
        }

        if(mat->has_pbr_metallic_roughness)
        {
            if(mat->pbr_metallic_roughness.base_color_texture.texture)
            {
                cgltf_texture_view tv = mat->pbr_metallic_roughness.base_color_texture;
            }
            if(mat->pbr_metallic_roughness.metallic_roughness_texture.texture)
            {
                cgltf_texture_view tv = mat->pbr_metallic_roughness.metallic_roughness_texture;
            }

            cgltf_float* bcf = mat->pbr_metallic_roughness.base_color_factor;
            f4 base_color_value = f4_create(bcf[0],bcf[1],bcf[2],bcf[3]);
            cgltf_float* mf = &mat->pbr_metallic_roughness.metallic_factor;
            cgltf_float* rf = &mat->pbr_metallic_roughness.roughness_factor;
        }
            
        if(mat->has_pbr_specular_glossiness)
        {
            if(mat->pbr_specular_glossiness.diffuse_texture.texture)
            {
                cgltf_texture_view tv = mat->pbr_specular_glossiness.diffuse_texture;
            }

            if(mat->pbr_specular_glossiness.specular_glossiness_texture.texture)
            {
                cgltf_texture_view tv = mat->pbr_specular_glossiness.specular_glossiness_texture;
            }

            cgltf_float* dcf = mat->pbr_specular_glossiness.diffuse_factor;
            f4 diffuse_value = f4_create(dcf[0],dcf[1],dcf[2],dcf[3]);
            cgltf_float* sf = mat->pbr_specular_glossiness.specular_factor;
            f3 specular_value = f3_create(sf[0],sf[1],sf[2]);
            cgltf_float* gf = &mat->pbr_specular_glossiness.glossiness_factor;
        }
            
        //alphaCutoff
        //alphaMode
        //emissiveFactor
        //emissiveTexture
        //occlusionTexture
        //normalTexture


        //TODO(Ray):Next we wil lmake append char to char verify that we are treating these primitives
        //as such in the importer and make sure that the propery uvs are being imported on the read side.
        //Something is not quite right.
        mesh.name = fmj_string_create(ma->name,ctx->perm_mem);
        
        mesh.material_id = 0;//AssetSystem::default_mat;
        
        //get buffer data from mesh
        if(prim.type == cgltf_primitive_type_triangles)
        {
            bool has_got_first_uv_set = false;
            for(int k = 0;k < prim.attributes_count;++k)
            {
                cgltf_attribute ac = prim.attributes[k];

                //Get indices
                if(prim.indices->component_type == cgltf_component_type_r_16u)
                {
                    cgltf_buffer_view* ibf = prim.indices->buffer_view;
                    u64 istart_offset = ibf->offset;
                    cgltf_buffer* ibuf = ibf->buffer;
                    uint16_t* indices_buffer = (uint16_t*)((uint8_t*)ibuf->data + istart_offset);
                    mesh.index_16_data = indices_buffer;
                    mesh.index_16_data_size = ibf->size;
                    mesh.index16_count = prim.indices->count;
                }
                
                //Get verts
                cgltf_accessor* acdata = ac.data;
                u64 count = acdata->count;
                cgltf_buffer_view* bf = acdata->buffer_view;
                
                //Get vertex buffer
                u64 start_offset = bf->offset;
                u32 stride = (u32)bf->stride;
                cgltf_buffer* buf = bf->buffer;
                float* buffer = (float*)((uint8_t*)buf->data + start_offset);
                            
                if(ac.type == cgltf_attribute_type_position)
                {
                    mesh.vertex_data = buffer;
                    mesh.vertex_data_size = bf->size;
                    mesh.vertex_count = count * 3;
                }
                else if(ac.type == cgltf_attribute_type_normal)
                {
                    mesh.normal_data = buffer;
                    mesh.normal_data_size = bf->size;
                    mesh.normal_count = count * 3;
                }
                else if(ac.type == cgltf_attribute_type_tangent)
                {
                    mesh.tangent_data = buffer;
                    mesh.tangent_data_size = bf->size;
                    mesh.tangent_count = count * 3;
                }

//NOTE(Ray):only support two set of uv data for now.
                else if(ac.type == cgltf_attribute_type_texcoord && !has_got_first_uv_set)
                {
                    mesh.uv_data = buffer;
                    mesh.uv_data_size = bf->size;
                    mesh.uv_count = count * 2;
                    has_got_first_uv_set = true;
                }
                else if(ac.type == cgltf_attribute_type_texcoord && has_got_first_uv_set)
                {
                    mesh.uv2_data = buffer;
                    mesh.uv2_data_size = bf->size;
                    mesh.uv2_count = count * 2;
                    has_got_first_uv_set = true;                        
                }
            }
        }
    }
    return mesh;    
}

bool fmj_asset_load_model_from_glb(FMJAssetContext* ctx,const char* file_path,FMJAssetModel* result,u32  material_id)
{
    bool is_success = false;
    result->model_name = fmj_string_create((char*)file_path,ctx->perm_mem);

    FMJAssetModel model = {};
    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result aresult = cgltf_parse_file(&options,file_path, &data);
    if (aresult == cgltf_result_success)
    {

//NOTE(Ray):Why multipler buffers?
//        for(int i = 0;i < data->buffers_count;++i)
        if(data->buffers_count > 1)
        {
            //We dont support multiple buffer cgltf yet.
            ASSERT(false);
        }

        model = fmj_asset_model_create(ctx);
        
        cgltf_result rs = cgltf_load_buffers(&options, data, data->buffers[0].uri);
        for(int i = 0;i < data->meshes_count;++i)
        {
            cgltf_mesh mes = data->meshes[i];
            FMJAssetMesh mesh = fmj_asset_create_mesh_from_cgltf_mesh(ctx,&mes);
            mesh.material_id = material_id;
            fmj_stretch_buffer_push(&model.meshes,&mesh);
            is_success = true;            
        }
        if(!is_success)
        {
            //TODO(Ray):Delte created model data.
            ASSERT(false);
        }
        else
        {
            *result = model;
        }
        cgltf_free(data);
    }
    return is_success;        
}

void fmj_asset_upload_model(AssetTables* asset_tables,FMJAssetContext*ctx,FMJAssetModel* ma)
{
    for(int i = 0;i < ma->meshes.fixed.count;++i)
    {
        int is_valid = 0;
        GPUMeshResource mesh_r;
        FMJAssetMesh* mesh = (FMJAssetMesh*)fmj_stretch_buffer_check_out(FMJAssetMesh,&ma->meshes,i);
        f2 id_range;
        if(mesh->vertex_count > 0)
        {
            u64 v_size = 3 * sizeof(f32) * mesh->vertex_count;
            u32 stride = sizeof(f32) * 3;            
            mesh_r.vertex_buff = D12RendererCode::AllocateStaticGPUArena(v_size);
            D12RendererCode::SetArenaToVertexBufferView(&mesh_r.vertex_buff,v_size,stride);

            u32 id = fmj_stretch_buffer_push(&asset_tables->vertex_buffers,(void*)&mesh_r.vertex_buff.buffer_view);
            
            D12RendererCode::UploadBufferData(&mesh_r.vertex_buff,mesh->vertex_data,v_size);

            id_range.x = id;            
            is_valid++;
        }
        else
        {
            ASSERT(false);
        }

        f32 start_range = id_range.x;
        if(mesh->normal_count > 0)
        {
            u64 size = sizeof(f32) * mesh->normal_count * 3;
            u32 stride = sizeof(f32) * 3;                        
            mesh_r.normal_buff = D12RendererCode::AllocateStaticGPUArena(size);            
            D12RendererCode::SetArenaToVertexBufferView(&mesh_r.normal_buff,size,stride);            

            fmj_stretch_buffer_push(&asset_tables->vertex_buffers,(void*)&mesh_r.normal_buff.buffer_view);
            D12RendererCode::UploadBufferData(&mesh_r.normal_buff,mesh->normal_data,size);

            start_range  += 1.0f;
            is_valid++;
        }
        
        if(mesh->uv_count > 0)
        {
            u64 size = sizeof(float) * mesh->uv_count;
            u32 stride = sizeof(f32) * 2;
            mesh_r.uv_buff = D12RendererCode::AllocateStaticGPUArena(size);            
            D12RendererCode::SetArenaToVertexBufferView(&mesh_r.uv_buff,size,stride);            

            fmj_stretch_buffer_push(&asset_tables->vertex_buffers,(void*)&mesh_r.uv_buff.buffer_view);    
            D12RendererCode::UploadBufferData(&mesh_r.uv_buff,mesh->normal_data,size);

            start_range += 1.0f;            
            is_valid++;
        }
        
//Not supporting UV2's for now
#if  0
        if(mesh->uv2_count > 0)
        {
            u64 size = sizeof(float) * mesh->uv2_count;
            u32 stride = sizeof(f32) * 2;
            mesh_r.uv_buff = D12RendererCode::AllocateStaticGPUArena(size);            
            D12RendererCode::SetArenaToVertexBufferView(&mesh_r.uv_buff,size,stride);            

            fmj_stretch_buffer_push(&asset_tables->vertex_buffers,(void*)&mesh_r.uv_buff.buffer_view);    
            D12RendererCode::UploadBufferData(&mesh_r.uv_buff,mesh->normal_data,size);            
            mesh_r.uv2_buff = ogle_gen_buffer(s,size,ResourceStorageModeShared);
            ogle_buffer_data_named(s,size,&mesh_r.uv2_buff,(void*)mesh->uv2_data);

            start_range += 1.0f;
            is_valid++;
        }
#endif
        
        id_range.y = start_range;
        
        if(mesh->index32_count > 0)
        {
            u64 size = sizeof(float) * mesh->index32_count;
            mesh_r.element_buff = D12RendererCode::AllocateStaticGPUArena(size);
            DXGI_FORMAT format;

            mesh->index_component_size = fmj_asset_index_component_size_32;
            format = DXGI_FORMAT_R32_UINT;                
            D12RendererCode::SetArenaToIndexVertexBufferView(&mesh_r.element_buff,size,format);

            u64 index_id = fmj_stretch_buffer_push(&asset_tables->index_buffers,(void*)&mesh_r.element_buff.index_buffer_view);    
            D12RendererCode::UploadBufferData(&mesh_r.element_buff,mesh->index_32_data,size);            

            mesh_r.index_id = index_id;
            is_valid++;
        }
        else if(mesh->index16_count > 0)
        {
            u64 size = sizeof(float) * mesh->index16_count;
            mesh_r.element_buff = D12RendererCode::AllocateStaticGPUArena(size);
            DXGI_FORMAT format;
            mesh->index_component_size = fmj_asset_index_component_size_16;
            
            format = DXGI_FORMAT_R16_UINT;                                
            D12RendererCode::SetArenaToIndexVertexBufferView(&mesh_r.element_buff,size,format);

            u64 index_id = fmj_stretch_buffer_push(&asset_tables->index_buffers,(void*)&mesh_r.element_buff.index_buffer_view);    
            D12RendererCode::UploadBufferData(&mesh_r.element_buff,mesh->index_16_data,size);            

            mesh_r.index_id = index_id;
            is_valid++;
        }
        
        //NOTE(RAY):For now we require that you have met all the data criteria
        if(is_valid >= 1)
        {
            mesh_r.buffer_range = id_range;
            mesh->mesh_resource = mesh_r;
        }
        else
        {
            ASSERT(false);
        }
    }
}

#define ASSETS_H
#endif

