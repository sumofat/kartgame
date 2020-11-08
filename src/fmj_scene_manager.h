#if !defined(FMJSceneManager)

struct FMJSceneManager
{
    FMJScene* current_scene;
    u64 root_node_id;
}typedef FMJSceneManager;

FMJSceneManager scene_manager;

#endif FMJSceneManager
