#if !defined(ASSETS_H)

#define CGLTF_IMPLEMENTATION
#include "../external/cgltf/cgltf.h"

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

    u64 metallic_roughness_texture_id;
    FMJ3DTrans transform;
    u64 matrix_id;
}typedef FMJMeshAsset;

struct FMJAssetModel
{
    u32 id;
    FMJString model_name;
    FMJStretchBuffer meshes;
}typedef FMJModelAsset;

struct FMJAssetModelLoadResult
{
    bool is_success;
    FMJAssetModel model;
    FMJSceneObject scene_object;
};

void fmj_asset_init(AssetTables* asset_tables);
FMJAssetModel fmj_asset_model_create(FMJAssetContext* ctx);
u64 fmj_asset_texture_add(FMJAssetContext* ctx,LoadedTexture texture);
FMJAssetMesh fmj_asset_create_mesh_from_cgltf_mesh(FMJAssetContext* ctx,cgltf_mesh* ma);
FMJAssetModel fmj_asset_load_model_from_glb(FMJAssetContext* ctx,const char* file_path,u32  material_id);
void fmj_asset_load_meshes_recursively_gltf_node_(FMJAssetModelLoadResult* result,cgltf_node* node,FMJAssetContext* ctx,const char* file_path,u32  material_id,FMJSceneObject* so);
FMJAssetModelLoadResult fmj_asset_load_model_from_glb_2(FMJAssetContext* ctx,const char* file_path,u32  material_id,FMJSceneObjectBuffer* sob);
void fmj_asset_upload_model(AssetTables* asset_tables,FMJAssetContext*ctx,FMJAssetModel* ma);

#define ASSETS_H
#endif

