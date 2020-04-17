
struct AssetTables
{
    AnyCache materials;
    AnyCache geometries;
    FMJStretchBuffer sprites;
    FMJStretchBuffer vertex_buffers;
}typedef;


void InitAssetTables(AssetTables* asset_tables)
{
    asset_tables->materials = fmj_anycache_init(4096,sizeof(FMJRenderMaterial),sizeof(u64),true);                                  
    asset_tables->sprites = fmj_stretch_buffer_init(1,sizeof(FMJSprite),8);
    //NOTE(RAY):If we want to make this platform independendt we would just make a max size of all platofrms
    //struct and put in this and get out the opaque pointer and cast it to what we need.
    asset_tables->vertex_buffers = fmj_stretch_buffer_init(1,sizeof(D3D12_VERTEX_BUFFER_VIEW),8);    
    asset_tables->geometries = fmj_anycache_init(4096,sizeof(FMJRenderGeometry),sizeof(u64),true);
}
