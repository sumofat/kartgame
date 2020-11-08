#if !defined(CAMERA_H)

f2 cam_pitch_yaw = f2_create(0.0f,0.0f);

enum RenderCameraProjectionType
{
    perspective,
    orthographic,
    screen_space
};

struct RenderCamera
{
    FMJ3DTrans ot;//perspective and ortho only
    f4x4 matrix;
    f4x4 projection_matrix;
    f4x4 spot_light_shadow_projection_matrix;
    f4x4 point_light_shadow_projection_matrix;
    RenderCameraProjectionType projection_type;
    f32 size;//ortho only
    f32 fov;//perspective only
    f2 near_far_planes;
    u64 matrix_id;
    u64 projection_matrix_id;
};

void fmj_camera_free(FMJAssetContext*ctx,RenderCamera* rc,Input input,f32 delta_seconds)
{
    float move_speed = 20.0f;
    f3 move_dir = f3_create_f(0);

    fmj_3dtrans_update(&rc->ot);
    if (input.keyboard.keys[keys.i].down)
    {
        move_dir = f3_add(move_dir,rc->ot.forward);
    }
    if (input.keyboard.keys[keys.k].down)
    {
        move_dir = f3_add(move_dir,f3_mul_s(rc->ot.forward,-1));
    }
    if (input.keyboard.keys[keys.j].down)
    {
        move_dir = f3_add(move_dir,rc->ot.right);
    }
    if (input.keyboard.keys[keys.l].down)
    {
        move_dir = f3_add(move_dir,f3_mul_s(rc->ot.right,-1));
    }

    rc->ot.p = f3_add(rc->ot.p, f3_mul_s(f3_mul_s(f3_mul_s(move_dir,-1),move_speed),delta_seconds));
            
    cam_pitch_yaw = f2_add(cam_pitch_yaw,input.mouse.delta_p);
    quaternion pitch = f3_axis_angle(f3_create(1, 0, 0), cam_pitch_yaw.y);
    quaternion yaw   = f3_axis_angle(f3_create(0, 1, 0), cam_pitch_yaw.x * -1);
    quaternion turn_qt = quaternion_mul(pitch,yaw);        
    rc->ot.r = turn_qt;

    rc->matrix = fmj_3dtrans_set_cam_view(&rc->ot);
    f4x4* rc_mat = fmj_stretch_buffer_check_out(f4x4,&ctx->asset_tables->matrix_buffer,rc->matrix_id);
    *rc_mat = rc->matrix;
    fmj_stretch_buffer_check_in(&ctx->asset_tables->matrix_buffer);
}

void fmj_camera_chase(FMJAssetContext*ctx,RenderCamera* rc,Input input,f32 delta_seconds,FMJ3DTrans target,f3 offset)
{
    fmj_3dtrans_update(&target);
    fmj_3dtrans_update(&rc->ot);
    f3 t_p = target.p;
    f3 n_f = f3_negate(target.forward);
    
    f3 target_f = f3_mul_s(n_f,3);
    target_f = f3_add(target_f,offset);
    f3 p = f3_add(t_p,target_f);
    f3 f = target.forward;
    f3 look_point_world = f3_add(t_p,f);
    f3 cam_dir = f3_sub(p,look_point_world);

    
    rc->ot.p = p;
    rc->ot.r = quaternion_look_rotation(cam_dir,f3_create(0,1,0));
    
    rc->matrix = fmj_3dtrans_set_cam_view(&rc->ot);
    f4x4* rc_mat = fmj_stretch_buffer_check_out(f4x4,&ctx->asset_tables->matrix_buffer,rc->matrix_id);
    *rc_mat = rc->matrix;
    fmj_stretch_buffer_check_in(&ctx->asset_tables->matrix_buffer);    
}

void fmj_camera_orbit(FMJAssetContext*ctx,RenderCamera* rc,Input input,f32 delta_seconds,FMJ3DTrans target,float radius)
{
    fmj_3dtrans_update(&rc->ot);
    cam_pitch_yaw = f2_add(cam_pitch_yaw,input.mouse.delta_p);
    quaternion pitch = f3_axis_angle(f3_create(1, 0, 0), cam_pitch_yaw.y);
    quaternion yaw   = f3_axis_angle(f3_create(0, 1, 0), cam_pitch_yaw.x * -1);
    quaternion turn_qt = quaternion_mul(pitch,yaw);        
    
    rc->ot.p = f3_add(target.p,f3_mul_s(quaternion_forward(turn_qt),radius));
    rc->ot.r = turn_qt;
    
    rc->matrix = fmj_3dtrans_set_cam_view(&rc->ot);
    f4x4* rc_mat = fmj_stretch_buffer_check_out(f4x4,&ctx->asset_tables->matrix_buffer,rc->matrix_id);
    *rc_mat = rc->matrix;
    fmj_stretch_buffer_check_in(&ctx->asset_tables->matrix_buffer);
}

#if 0
#endif

#define CAMERA_H
#endif
