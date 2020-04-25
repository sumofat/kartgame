
void fmj_asset_init(AssetTables* asset_tables)
{
    asset_tables->materials = fmj_anycache_init(4096,sizeof(FMJRenderMaterial),sizeof(u64),true);                                  
    asset_tables->sprites = fmj_stretch_buffer_init(1,sizeof(FMJSprite),8);
    asset_tables->textures = fmj_stretch_buffer_init(1,sizeof(LoadedTexture),8);    
    //NOTE(RAY):If we want to make this platform independendt we would just make a max size of all platofrms
    //struct and put in this and get out the opaque pointer and cast it to what we need.
    asset_tables->vertex_buffers = fmj_stretch_buffer_init(1,sizeof(D3D12_VERTEX_BUFFER_VIEW),8);
    asset_tables->index_buffers = fmj_stretch_buffer_init(1,sizeof(D3D12_INDEX_BUFFER_VIEW),8);
    asset_tables->matrix_buffer = fmj_stretch_buffer_init(1,sizeof(f4x4),8);

    asset_tables->meshes = fmj_stretch_buffer_init(1,sizeof(FMJAssetMesh),8);
}

FMJAssetModel fmj_asset_model_create(FMJAssetContext* ctx)
{
    FMJAssetModel model = {};
    model.meshes = fmj_stretch_buffer_init(1,sizeof(u64),8);
    return model;
}

u64 fmj_asset_texture_add(FMJAssetContext* ctx,LoadedTexture texture)
{

    u64 tex_id = fmj_stretch_buffer_push(&ctx->asset_tables->textures,&texture);    
    D12RendererCode::Texture2D(&texture,tex_id);
    //TODO(ray):Add assert to how many textures are allowed in acertain heap that
    //we are using to store the texture on the gpu.
//texid is a slot on the gpu heap
//    ASSERT(tex_id < MAX_TEX_ID_FOR_HEAP)
    return tex_id;
}

f2 fmj_asset_create_mesh_from_cgltf_mesh(FMJAssetContext* ctx,cgltf_mesh* ma,u64 material_id)
{
    ASSERT(ma);

    u32 mesh_id = ctx->asset_tables->meshes.fixed.count;    
    f2 result = f2_create(mesh_id,mesh_id);

    for(int j = 0;j < ma->primitives_count;++j)
    {
        
        FMJAssetMesh mesh = {};
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
                {
                    u64 offset = (u64)tv.texture->image->buffer_view->offset;
                    void* tex_data = (uint8_t*)tv.texture->image->buffer_view->buffer->data + offset;
                    if(*(u8*)tex_data)
                    {
                        uint64_t data_size = tv.texture->image->buffer_view->size;                
                        LoadedTexture tex =  get_loaded_image_from_mem(tex_data,data_size,4);                
                        u64 id = fmj_asset_texture_add(ctx,tex);                
                        mesh.metallic_roughness_texture_id = id;                    
                    }                    
                }
            }
            
            if(mat->pbr_metallic_roughness.metallic_roughness_texture.texture)
            {
                cgltf_texture_view tv = mat->pbr_metallic_roughness.metallic_roughness_texture;
            }

            cgltf_float* bcf = mat->pbr_metallic_roughness.base_color_factor;
            mesh.base_color = f4_create(bcf[0],bcf[1],bcf[2],bcf[3]);
            
//            cgltf_float* mf = &mat->pbr_metallic_roughness.metallic_factor;
//            cgltf_float* rf = &mat->pbr_metallic_roughness.roughness_factor;
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

        mesh.name = fmj_string_create(ma->name,ctx->perm_mem);
        mesh.material_id = 0;

        if(prim.type == cgltf_primitive_type_triangles)
        {
            bool has_got_first_uv_set = false;

            if(prim.indices)
            {
                u64 istart_offset = prim.indices->offset + prim.indices->buffer_view->offset;
                cgltf_buffer* ibuf = prim.indices->buffer_view->buffer;                
                if(prim.indices->component_type == cgltf_component_type_r_16u)
                {
                    u64 indices_size =  (u64)prim.indices->count * sizeof(u16);
                    u16* indices_buffer = (u16*)((uint8_t*)ibuf->data + istart_offset);
                    u16* outindex_f = (u16*)malloc(indices_size);
                    
                    memcpy(outindex_f,indices_buffer,indices_size);
                    mesh.index_16_data = outindex_f;
                    mesh.index_16_data_size = sizeof(u16) * prim.indices->count;
                    mesh.index16_count = prim.indices->count;                    
                }

                else if(prim.indices->component_type == cgltf_component_type_r_32u)
                {
                    u64 indices_size =  (u64)prim.indices->count * sizeof(u32);
                    u32* indices_buffer = (u32*)((uint8_t*)ibuf->data + istart_offset);
                    u32* outindex_f = (u32*)malloc(indices_size);

                    memcpy(outindex_f,indices_buffer,indices_size);
                    mesh.index_32_data = outindex_f;
                    mesh.index_32_data_size = sizeof(u32) * prim.indices->count;
                    mesh.index32_count = prim.indices->count;                    
                }
            }
            
            for(int k = 0;k < prim.attributes_count;++k)
            {
                cgltf_attribute ac = prim.attributes[k];

                cgltf_accessor* acdata = ac.data;
                u64 count = acdata->count;
                cgltf_buffer_view* bf = acdata->buffer_view;
                {
                    u64 start_offset = bf->offset;
                    u32 stride = (u32)bf->stride;
                    cgltf_buffer* buf = bf->buffer;
                    float* buffer = (float*)((uint8_t*)buf->data + start_offset);

                    if (acdata->is_sparse)
                    {
                        ASSERT(false);
                    }

                    cgltf_size num_floats = acdata->count * cgltf_num_components(acdata->type);
                    cgltf_size num_bytes = sizeof(f32) * num_floats;                
                    cgltf_float* outf = (cgltf_float*)malloc(num_bytes);
                    cgltf_size csize = cgltf_accessor_unpack_floats(acdata,outf,num_floats);

                    if(ac.type == cgltf_attribute_type_position)
                    {
                        mesh.vertex_data = outf;
                        mesh.vertex_data_size = num_bytes;
                        mesh.vertex_count = count;
                    }

                    else if(ac.type == cgltf_attribute_type_normal)
                    {
                        mesh.normal_data = outf;
                        mesh.normal_data_size = num_bytes;
                        mesh.normal_count = count;
                    }

                    else if(ac.type == cgltf_attribute_type_tangent)
                    {
                        mesh.tangent_data = outf;
                        mesh.tangent_data_size = num_bytes;
                        mesh.tangent_count = count;
                    }

//NOTE(Ray):only support two set of uv data for now.
                    else if(ac.type == cgltf_attribute_type_texcoord && !has_got_first_uv_set)
                    {
                        mesh.uv_data = outf;
                        mesh.uv_data_size = num_bytes;
                        mesh.uv_count = count;
                        has_got_first_uv_set = true;
                    }
                
                    else if(ac.type == cgltf_attribute_type_texcoord && has_got_first_uv_set)
                    {
                        mesh.uv2_data = outf;
                        mesh.uv2_data_size = num_bytes;
                        mesh.uv2_count = count;
                        has_got_first_uv_set = true;                        
                    }                    
                }
            }
        }

        mesh.material_id = material_id;
        u32 last_id = fmj_stretch_buffer_push(&ctx->asset_tables->meshes,&mesh);
        result.y = last_id;
    }
    
    return result;    
}

void fmj_asset_load_meshes_recursively_gltf_node_(FMJAssetModelLoadResult* result,cgltf_node* node,FMJAssetContext* ctx,const char* file_path,u32  material_id,FMJSceneObject* so)
{
    for(int i = 0;i < node->children_count;++i)
    {
        cgltf_node* child = node->children[i];
        FMJ3DTrans trans = {};
        fmj_3dtrans_init(&trans);

        f4x4 out_mat = f4x4_identity();
        if(child->has_matrix)
        {
                    
            out_mat = f4x4_create(child->matrix[0],child->matrix[1],child->matrix[2],child->matrix[3],
                                  child->matrix[4],child->matrix[5],child->matrix[6],child->matrix[7],
                                  child->matrix[8],child->matrix[9],child->matrix[10],child->matrix[11],
                                  child->matrix[12],child->matrix[13],child->matrix[14],child->matrix[15]);                                        
        }
        else
        {
            cgltf_node_transform_world(child, (cgltf_float*)&out_mat);
        }

        trans.p = f3_create(out_mat.c3.x,out_mat.c3.y,out_mat.c3.z);
        trans.s = f3_create(f3_length(f3_create(out_mat.c0.x,out_mat.c0.y,out_mat.c0.z)),
                            f3_length(f3_create(out_mat.c1.x,out_mat.c1.y,out_mat.c1.z)),
                            f3_length(f3_create(out_mat.c2.x,out_mat.c2.y,out_mat.c2.z)));
        trans.r = quaternion_create_f4x4(out_mat);
        trans.m = out_mat;                    

        s32 mesh_id = -1;
        u32 type = 0;
        f2 mesh_range = f2_create_f(0);
        if(child->mesh)
        {
            //result->model.model_name = fmj_string_create((char*)file_path,ctx->perm_mem);                
            mesh_range = fmj_asset_create_mesh_from_cgltf_mesh(ctx,child->mesh,material_id);
            type = 1;
            fmj_asset_upload_meshes(ctx,mesh_range);            
        }
        void* mptr = (void*)mesh_id;
//add node to parent        
        u64 child_id = AddChildToSceneObject(ctx,so,&trans,&mptr);
        FMJSceneObject* child_so = fmj_stretch_buffer_check_out(FMJSceneObject,&ctx->scene_objects,child_id);
        child_so->type = type;
        child_so->primitives_range = mesh_range;
        fmj_asset_load_meshes_recursively_gltf_node_(result,child,ctx,file_path, material_id,child_so);
        fmj_stretch_buffer_check_in(&ctx->scene_objects);                    
    }
}

//TODO(Ray):We dont need rendering info such as matrices pushes for this as its
//just a template to use to build a model "instance"
FMJAssetModelLoadResult fmj_asset_load_model_from_glb_2(FMJAssetContext* ctx,const char* file_path,u32  material_id)
{
    FMJAssetModelLoadResult result = {};
    bool is_success = false;

    cgltf_options options = {};
    cgltf_data* data = NULL;
    cgltf_result aresult = cgltf_parse_file(&options,file_path, &data);
    ASSERT(aresult == cgltf_result_success);
    if (aresult == cgltf_result_success)
    {
        for(int i = 0;i < data->buffers_count;++i)
        {
            cgltf_result rs = cgltf_load_buffers(&options, data, data->buffers[i].uri);
            ASSERT(rs == cgltf_result_success);
        }

        //TODO(ray):If we didnt  get a mesh release any memory allocated.
        if(data->nodes_count > 0)
        {
            //result.model = fmj_asset_model_create(ctx);
            FMJ3DTrans parent_trans = {};
            fmj_3dtrans_init(&parent_trans);

            u64 p_matrix_id = fmj_stretch_buffer_push(&ctx->asset_tables->matrix_buffer,&parent_trans.m);
            void* p_mptr = (void*)p_matrix_id;

            FMJSceneObject model_root_so_ = {};
            fmj_scene_object_buffer_init(&model_root_so_.children);
            u64 model_root_so_id = fmj_stretch_buffer_push(&ctx->scene_objects,&model_root_so_);
            FMJSceneObject* model_root = fmj_stretch_buffer_check_out(FMJSceneObject,&ctx->scene_objects,model_root_so_id);

            ASSERT(data->scenes_count == 1);
            for(int i = 0;i < data->scenes[0].nodes_count;++i)
            {
                cgltf_node* root_node = data->scenes[0].nodes[i];
            
                FMJ3DTrans trans = {};
                fmj_3dtrans_init(&trans);

                f4x4 out_mat = f4x4_identity();
                if(root_node->has_matrix)
                {
                    out_mat = f4x4_create(root_node->matrix[0],root_node->matrix[1],root_node->matrix[2],root_node->matrix[3],
                                          root_node->matrix[4],root_node->matrix[5],root_node->matrix[6],root_node->matrix[7],
                                          root_node->matrix[8],root_node->matrix[9],root_node->matrix[10],root_node->matrix[11],
                                          root_node->matrix[12],root_node->matrix[13],root_node->matrix[14],root_node->matrix[15]);                                        
                }
                else
                {
                    cgltf_node_transform_world(root_node, (cgltf_float*)&out_mat);
                }

                trans.p = f3_create(out_mat.c3.x,out_mat.c3.y,out_mat.c3.z);
                trans.s = f3_create(f3_length(f3_create(out_mat.c0.x,out_mat.c0.y,out_mat.c0.z)),
                                    f3_length(f3_create(out_mat.c1.x,out_mat.c1.y,out_mat.c1.z)),
                                    f3_length(f3_create(out_mat.c2.x,out_mat.c2.y,out_mat.c2.z)));
                trans.r = quaternion_create_f4x4(out_mat);
                trans.m = out_mat;

                s32 mesh_id = -1;
                u32 type = 0;
                f2 mesh_range = f2_create_f(0);                
                if(root_node->mesh)
                {
                    //result.model.model_name = fmj_string_create((char*)file_path,ctx->perm_mem);                
                    mesh_range = fmj_asset_create_mesh_from_cgltf_mesh(ctx,root_node->mesh,material_id);
                    type = 1;
                    fmj_asset_upload_meshes(ctx,mesh_range);
                }

                void* mptr = (void*)mesh_id;
                u64 child_id = AddChildToSceneObject(ctx,model_root,&trans,&mptr);
                FMJSceneObject* child_so = fmj_stretch_buffer_check_out(FMJSceneObject,&ctx->scene_objects,child_id);
                child_so->type = type;
                child_so->primitives_range = mesh_range;
                fmj_asset_load_meshes_recursively_gltf_node_(&result,root_node,ctx,file_path,material_id,child_so);
                fmj_stretch_buffer_check_in(&ctx->scene_objects);
                
            }
            fmj_stretch_buffer_check_in(&ctx->scene_objects);
            result.scene_object_id = model_root_so_id;
        }
        
        if(!is_success)
        {
//            ASSERT(false);
        }
//        cgltf_free(data);
    }
    return result;        
}

void fmj_asset_upload_meshes(FMJAssetContext*ctx,f2 range)
{
    for(int i = range.x;i <= range.y;++i)
    {
        int is_valid = 0;
        GPUMeshResource mesh_r;
        FMJAssetMesh* mesh = (FMJAssetMesh*)fmj_stretch_buffer_check_out(FMJAssetMesh,&ctx->asset_tables->meshes,i);        
        f2 id_range = f2_create_f(0);
        if(mesh->vertex_count > 0)
        {
            u64 v_size = mesh->vertex_data_size;//3 * sizeof(f32) * mesh->vertex_count;
            u32 stride = sizeof(f32) * 3;            
            mesh_r.vertex_buff = D12RendererCode::AllocateStaticGPUArena(v_size);
            D12RendererCode::SetArenaToVertexBufferView(&mesh_r.vertex_buff,v_size,stride);
            D12RendererCode::UploadBufferData(&mesh_r.vertex_buff,mesh->vertex_data,v_size);
            
            u32 id = fmj_stretch_buffer_push(&ctx->asset_tables->vertex_buffers,(void*)&mesh_r.vertex_buff.buffer_view);            

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
            u64 size = mesh->normal_data_size;
            u32 stride = sizeof(f32) * 3;
            mesh_r.normal_buff = D12RendererCode::AllocateStaticGPUArena(size);            
            D12RendererCode::SetArenaToVertexBufferView(&mesh_r.normal_buff,size,stride);            

            D12RendererCode::UploadBufferData(&mesh_r.normal_buff,mesh->normal_data,size);
            fmj_stretch_buffer_push(&ctx->asset_tables->vertex_buffers,(void*)&mesh_r.normal_buff.buffer_view);
            start_range  += 1.0f;
            is_valid++;
        }
        
        if(mesh->uv_count > 0)
        {
            u64 size = mesh->uv_data_size;
            u32 stride = sizeof(f32) * 2;
            mesh_r.uv_buff = D12RendererCode::AllocateStaticGPUArena(size);            
            D12RendererCode::SetArenaToVertexBufferView(&mesh_r.uv_buff,size,stride);            

            D12RendererCode::UploadBufferData(&mesh_r.uv_buff,mesh->uv_data,size);
            fmj_stretch_buffer_push(&ctx->asset_tables->vertex_buffers,(void*)&mesh_r.uv_buff.buffer_view);    
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

            D12RendererCode::UploadBufferData(&mesh_r.uv_buff,mesh->uv2_data,size);
            fmj_stretch_buffer_push(&ctx->asset_tables->vertex_buffers,(void*)&mesh_r.uv_buff.buffer_view);

            start_range += 1.0f;
            is_valid++;
        }
#endif
        
        id_range.y = start_range;
        
        if(mesh->index32_count > 0)
        {
            u64 size = mesh->index_32_data_size;
            mesh_r.element_buff = D12RendererCode::AllocateStaticGPUArena(size);
            DXGI_FORMAT format;

            mesh->index_component_size = fmj_asset_index_component_size_32;
            format = DXGI_FORMAT_R32_UINT;                
            D12RendererCode::SetArenaToIndexVertexBufferView(&mesh_r.element_buff,size,format);

            D12RendererCode::UploadBufferData(&mesh_r.element_buff,mesh->index_32_data,size);            
            u64 index_id = fmj_stretch_buffer_push(&ctx->asset_tables->index_buffers,(void*)&mesh_r.element_buff.index_buffer_view);
            
            mesh_r.index_id = index_id;
            is_valid++;
        }
        
        else if(mesh->index16_count > 0)
        {
            u64 size = mesh->index_16_data_size;
            mesh_r.element_buff = D12RendererCode::AllocateStaticGPUArena(size);
            DXGI_FORMAT format;
            mesh->index_component_size = fmj_asset_index_component_size_16;
            
            format = DXGI_FORMAT_R16_UINT;                                
            D12RendererCode::SetArenaToIndexVertexBufferView(&mesh_r.element_buff,size,format);

            D12RendererCode::UploadBufferData(&mesh_r.element_buff,mesh->index_16_data,size);            
            u64 index_id = fmj_stretch_buffer_push(&ctx->asset_tables->index_buffers,(void*)&mesh_r.element_buff.index_buffer_view);
            
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

        fmj_stretch_buffer_check_in(&ctx->asset_tables->meshes);        
    }
}

FMJStretchBuffer fmj_asset_copy_buffer(FMJStretchBuffer* in,bool copy_contents = false)
{
    ASSERT(in);
    FMJStretchBuffer result = {};
    if(in->fixed.mem_arena.size > 0)
    {
        result = fmj_stretch_buffer_init(in->fixed.count,in->fixed.unit_size,in->fixed.alignment);;
        if(copy_contents)
        {
            result.fixed.count = in->fixed.count;
            result.fixed.total_size = in->fixed.total_size;
            memcpy(result.fixed.mem_arena.base,in->fixed.mem_arena.base,in->fixed.mem_arena.size);                            
        }
    }
    return result;
}

FMJSceneObject fmj_asset_copy_scene_object(FMJAssetContext* ctx,FMJSceneObject* so)
{
    ASSERT(so);
    FMJSceneObject result = *so;
    result.children.buffer = fmj_asset_copy_buffer(&so->children.buffer);
    result.transform = so->transform;
    result.m_id = fmj_stretch_buffer_push(&ctx->asset_tables->matrix_buffer,&result.transform.m);
//    result.data = so->data;
//    result.type = so->type;
//    result.primitives_range = so->primitives_range;
    return result;
}

void fmj_asset_copy_model_data_recursively_(FMJAssetContext* ctx,u64 dest_id,u64  src_id)
{

    FMJSceneObject src = fmj_stretch_buffer_get(FMJSceneObject,&ctx->scene_objects,src_id);
    for(int i = 0;i < src.children.buffer.fixed.count;++i)
    {
        FMJSceneObject* dest = fmj_stretch_buffer_check_out(FMJSceneObject,&ctx->scene_objects,dest_id);
           
        u64 child_so_id = fmj_stretch_buffer_get(u64,&src.children.buffer,i);
        FMJSceneObject child_so = fmj_stretch_buffer_get(FMJSceneObject,&ctx->scene_objects,child_so_id);
        FMJSceneObject new_child_dest = fmj_asset_copy_scene_object(ctx,&child_so);

        fmj_stretch_buffer_check_in(&ctx->scene_objects);
        u64 new_dest_id = fmj_stretch_buffer_push(&ctx->scene_objects,&new_child_dest);
        dest = fmj_stretch_buffer_check_out(FMJSceneObject,&ctx->scene_objects,dest_id);        
        fmj_stretch_buffer_push(&dest->children.buffer,&new_dest_id);
        fmj_stretch_buffer_check_in(&ctx->scene_objects);
        
        if(child_so.type == 1)
        {
            int a = 0;
        }            
        fmj_asset_copy_model_data_recursively_(ctx,new_dest_id,child_so_id);
    }
}

u64 fmj_asset_create_model_instance(FMJAssetContext*ctx,FMJAssetModelLoadResult* model)
{
    ASSERT(model);
    FMJSceneObject src = fmj_stretch_buffer_get(FMJSceneObject,&ctx->scene_objects,model->scene_object_id);
    FMJSceneObject dest_ = fmj_asset_copy_scene_object(ctx,&src);
    u64 dest_id = fmj_stretch_buffer_push(&ctx->scene_objects,&dest_);

    for(int i = 0;i < src.children.buffer.fixed.count;++i)
    {
        FMJSceneObject* dest = fmj_stretch_buffer_check_out(FMJSceneObject,&ctx->scene_objects,dest_id);            
        u64 src_child_so_id = fmj_stretch_buffer_get(u64,&src.children.buffer,i);
        FMJSceneObject src_child_so = fmj_stretch_buffer_get(FMJSceneObject,&ctx->scene_objects,src_child_so_id);
        FMJSceneObject dest_child_so_ = fmj_asset_copy_scene_object(ctx,&src_child_so);

        fmj_stretch_buffer_check_in(&ctx->scene_objects);
        u64 dest_child_so_id = fmj_stretch_buffer_push(&ctx->scene_objects,&dest_child_so_);
        dest = fmj_stretch_buffer_check_out(FMJSceneObject,&ctx->scene_objects,dest_id);            
        fmj_stretch_buffer_push(&dest->children.buffer,&dest_child_so_id);
        fmj_stretch_buffer_check_in(&ctx->scene_objects);        

        
        fmj_asset_copy_model_data_recursively_(ctx,dest_child_so_id,src_child_so_id);
    }
    return dest_id;
}

bool fmj_asset_get_mesh_id_by_name(char* name,FMJAssetContext* ctx,FMJSceneObject* start,u64* result)
{
    for(int i = 0;i <= start->primitives_range.y;++i)
    {
        FMJAssetMesh mesh = fmj_stretch_buffer_get(FMJAssetMesh,&ctx->asset_tables->meshes,i);
        if(fmj_string_cmp_char(mesh.name,name))
        {
            *result = (u64)i;
            return true;
        }
    }

    bool is_found = false;
    for(int c = 0;c < start->children.buffer.fixed.count;++c)
    {
        u64 c_id = fmj_stretch_buffer_get(u64,&start->children.buffer,c);
        FMJSceneObject child_so = fmj_stretch_buffer_get(FMJSceneObject,&ctx->scene_objects,c_id);
        is_found = fmj_asset_get_mesh_id_by_name(name,ctx,&child_so,result);
        if(is_found)
        {
            break;
        }
    }
    
    return is_found;
}

