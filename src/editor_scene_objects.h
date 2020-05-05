
struct FMJEditorSceneTree
{
    FMJSceneObject* root;
    bool is_showing;
    
}typedef FMJSceneTreeViewer;

u64 node_id;

void fmj_editor_scene_tree_init(FMJEditorSceneTree* tree)
{
    ASSERT(tree);
}

void fmj_editor_scene_tree_display_recursively(FMJSceneObject* so,FMJAssetContext* ctx,u64 id)
{
    if(so)
    {
        bool is_tree_node = false;
        if(so->name.length > 0)
        {
            u64* node_id = (u64*)id;
            is_tree_node = ImGui::TreeNode((void*)node_id,"Selectable Node %s",so->name.string);            
        }
        else
        {
            is_tree_node = ImGui::TreeNode(&id,"Selectable Node");            
        }

        if(is_tree_node)
        {

            if(ImGui::TreeNode("matrix"))
            {
                static float vec4f[4] = { 0.10f, 0.20f, 0.30f, 0.44f };
                ImGui::InputFloat4("input float4", vec4f);
                ImGui::InputFloat4("input float4", vec4f);
                ImGui::InputFloat4("input float4", vec4f);
                ImGui::InputFloat4("input float4", vec4f);
                ImGui::TreePop();
            }

            ImGui::InputFloat3("position",so->transform.p.xyz);
            ImGui::InputFloat4("rotation",so->transform.r.xyzw);
            ImGui::InputFloat3("scale",so->transform.s.xyz);
            if(so->type == 1)
            {
                if(ImGui::TreeNode("mesh data"))
                {
                    for(int m = so->primitives_range.x;m <= so->primitives_range.y;++m)
                    {
                        u64 mesh_id = (u64)m;                                    
                        FMJAssetMesh mesh = fmj_stretch_buffer_get(FMJAssetMesh,&ctx->asset_tables->meshes,mesh_id);
                        //show some mesh properties.
                        ImGui::Text("mesh properties");
                        ImGui::Text("vert count %d",mesh.vertex_count);
                        ImGui::Text("vert data size %d",mesh.vertex_data_size);
                        if(mesh.index_component_size == fmj_asset_index_component_size_16)
                        {
                            ImGui::Text("index component size 16 bit");                                                                                        
                            ImGui::Text("index count %d",mesh.index16_count);
                            ImGui::Text("index size %d",mesh.index_16_data_size);                                                                                        
                        }
                        else if(mesh.index_component_size == fmj_asset_index_component_size_16)
                        {
                            ImGui::Text("index component size 32 bit");
                            ImGui::Text("index count %d",mesh.index32_count);
                            ImGui::Text("index size %d",mesh.index_32_data_size);                                                                                                            
                        }

                    }
                    ImGui::TreePop();                                        

                }

            }
            for(int i = 0;i < so->children.buffer.fixed.count;++i)
            {
                u64 child_so_id = fmj_stretch_buffer_get(u64,&so->children.buffer,i);            
                FMJSceneObject child = fmj_stretch_buffer_get(FMJSceneObject,&ctx->scene_objects,child_so_id);
                fmj_editor_scene_tree_display_recursively(&child,ctx,child_so_id);
            }            
                
            ImGui::TreePop();
        }
    }
}

void fmj_editor_scene_tree_show(FMJEditorSceneTree* tree,FMJScene* s,FMJAssetContext* ctx)
{
    ASSERT(tree);
    ImGui::SetNextWindowSize(ImVec2(520,600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Scene Objects", &tree->is_showing))
    {
        ImGui::End();
        return;
    }

    if (ImGui::TreeNode("SceneObjects"))
    {
        ImGui::Text("Start Of Tree!");
        for(int i = 0;i < s->buffer.buffer.fixed.count;++i)
        {
            u64 so_id = fmj_stretch_buffer_get(u64,&s->buffer.buffer,i);        
            FMJSceneObject* so = fmj_stretch_buffer_check_out(FMJSceneObject,&ctx->scene_objects,so_id);
            fmj_editor_scene_tree_display_recursively(so,ctx,so_id);
            fmj_stretch_buffer_check_in(&ctx->scene_objects);
        }        

        ImGui::TreePop();        
    }

    ImGui::End();    
}

