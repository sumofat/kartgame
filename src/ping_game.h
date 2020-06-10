
struct GameState
{
    u32  current_player_id;
    u32 is_phyics_active;
};

static SoundClip bgm_soundclip;
PhysicsScene scene;
PhysicsMaterial physics_material;
f32 ground_friction = 0.0032f;
FMJFixedBuffer ui_fixed_quad_buffer;
GameState game_state;

bool prev_lmb_state = false;

void SetSpriteNonVisible(void* node)
{
    FMJUINode* a = (FMJUINode*)node;
//    SpriteTrans* st = fmj_fixed_buffer_get_ptr(SpriteTrans,&ui_fixed_quad_buffer,a->st_id);    
//    st->sprite.is_visible = false;
}





