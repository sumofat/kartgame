#include "physics.h"

struct CarFrameBuffer
{
    FMJStretchBuffer car_frames;
    FMJAssetContext* asset_ctx;
}typedef CarFrameBuffer;

struct CarFrameDescriptorBuffer
{
    FMJStretchBuffer descriptors;
}typedef CarFrameDescriptorBuffer;

enum CarFrameType
{
    car_frame_type_none,
}typedef CarFrameType;

struct CarFrameDescriptor
{
    CarFrameType type;
//    GameObject prefab;
    int count;
    f32 mass;
    f32 max_thrust;
    f32 maximum_wheel_angle;
    int wheel_count;

}typedef CarFrameDescriptor;
 
struct CarFrame
{
    FMJSceneObject* e;
    f3 v;
    
    f3 thrust_vector;
    f3 lift_vector;
    f3 rudder_lift_vector;
    f3 angular_momentum;
    // Quaternion spin;
    f3 angular_velocity;
    f32 inertia;//typically a 3x3 matrice but we are simpliflying to a cube for now.
    f32 inverse_inertia;

    RigidBody rb;

    f3 total_lift;
    f3 last_v;
    f3 last_accel;
    f32 last_aoa;//angle of attack
    f32 last_aos;//angle of slip

    CarFrameDescriptor desc;
    f32 indicated_ground_speed;
    f3 ground_normal;

    f32 sus_height;
    bool is_grounded;
    f32 repulsive_force_area;

    f3 last_p;
    f3 last_track_normal;
    f3 last_hit_p;
    f32 friction;

    FMJ3DTrans wheel_transforms[4];//add more if we need
    u32 wheel_count;
    
    f32 current_spin_angle = 0;
}typedef CarFrame;

struct AgentGroundedTestResult
{
    f32 distance_from_hit_p;
    bool is_hit;
    bool is_grounded;
    f3 hit_p;
    f3 hit_n;    
}typedef AgentGroundedTestResult;

float dampening = 0.2f;
float sus_height = 1.0f;

void kart_init_car_frames(CarFrameBuffer* buffer,CarFrameDescriptorBuffer buffer_desc,int count)
{
    ASSERT(buffer);
    buffer->car_frames = fmj_stretch_buffer_init(count,sizeof(CarFrame),8);
    for (int i = 0; i < buffer_desc.descriptors.fixed.count; i++)
    {
        CarFrameDescriptor desc = fmj_stretch_buffer_get(CarFrameDescriptor,&buffer_desc.descriptors,i);
//        GameObject prefab_instance = desc.prefab;
        for (int j = 0; j < desc.count; j++)
        {
            CarFrame new_airframe = {};//new CarFrame();
            new_airframe.desc = desc;
//            GameObject go  = Object.Instantiate(prefab_instance) as GameObject;
//            new_airframe.e = AddEntity(repo, go.transform); 
            new_airframe.e->transform.p = f3_create_f(0);
            new_airframe.e->transform.r =  quaternion_identity();
            new_airframe.e->transform.s = f3_create_f(1);
            new_airframe.last_v = f3_create_f(0);
            new_airframe.v = f3_create_f(0);
            new_airframe.total_lift = f3_create(0, 100, 0);
            new_airframe.thrust_vector = f3_create(0, 0, 0);
            new_airframe.lift_vector = f3_create(0, 1, 0);
//            new_airframe.rb = go.AddComponent<Rigidbody>();
            //new_airframe.rb.angularDrag = 3.1f;
            new_airframe.repulsive_force_area = 24;
//            new_airframe.rb.useGravity = false;
//            new_airframe.sus_height = sus_height;
//            new_airframe.rb.constraints = RigidbodyConstraints.FreezePosition;k
//            new_airframe.e.is_rigidbody_rotation = true;
//            new_airframe.wheel_transforms = new List<Transform>();
/*
            for(int w = 0; w < desc.wheel_count; ++w)
            {
                Transform wheel_t = null;
                if(w == 0)
                {
                    wheel_t = FindTransformRecursive(new_airframe.e.t, "SkyCarWheelFrontLeft");
                }
                else if(w == 1)
                {
                    wheel_t = FindTransformRecursive(new_airframe.e.t, "SkyCarWheelFrontRight");
                }else if(w == 2)
                {
                    wheel_t = FindTransformRecursive(new_airframe.e.t, "SkyCarWheelRearLeft");
                }else if(w == 3)
                {
                    wheel_t = FindTransformRecursive(new_airframe.e.t, "SkyCarWheelRearRight");
                }

                if(wheel_t == null)
                {
                    Debug.LogError("WE ARE MISSING A WHEEL!! for now must have 4 wheels with specific transform names for lookup.");
                }
                new_airframe.wheel_transforms.Add(wheel_t);
            }
*/
            
            fmj_stretch_buffer_push(&buffer->car_frames,&new_airframe);
        }
    }
}

f32 air_density = 1.225f;

f3 fmj_physics_calculate_gravity(f32 mass,f3 gravity)
{
    f3 gravity_vector = gravity;
    return f3_mul_s(gravity_vector,(mass/mass));
}

f3 fmj_physics_calculate_drag(CarFrame airframe,FMJCurves curves)
{
    //BEGIN DRAG
    //TODO(Ray):Make a curve editor for saving curves to use for evaultion
    //and fine tuning.
    f32 coeffecient_of_drag = fmj_animation_curve_evaluate(curves,airframe.last_aoa);

    //NOTE(Ray):We do not take into account  changing  areas based on rotation.
    //cross section in relation to the vector of movement        
    f32 area = 4.0f;
    f32 density_of_medium = 1.1839f;

    f32 velocity = f3_length(airframe.v);
    f32 velocity_squared = (velocity * velocity);
    f32 a = (coeffecient_of_drag *  area * density_of_medium) * (velocity_squared);
    f3 b = f3_mul_s(f3_normalize(airframe.v),-1);//safe to zero            
    f3 drag_force = f3_mul_s(b,a);
    if (f3_length(drag_force) < 0.01f)
    {
        drag_force = f3_create_f(0);
    }

    return f3_div_s(drag_force,airframe.desc.mass);
}

f3 fmj_physics_calculate_thrust(CarFrame airframe,f3 stick_input_throttle)
{
    f3 a = f3_mul_s(f3_mul_s(airframe.e->transform.forward,stick_input_throttle.z),airframe.desc.max_thrust);
    return f3_div_s(a,airframe.desc.mass);
}

f3 fmj_physics_calculate_lateral_force(CarFrame cf,FMJCurves slip_angle_curve)
{
    f3 evaluated_vector = f3_create_f(0);
    if (cf.is_grounded)
    {
        //TireForce
        //slip angle

//        float forVel = cf.e.t.InverseTransformDirection(cf.v).normalized.z;
        float forVel = f3_normalize(fmj_3dtrans_world_to_local_dir(&cf.e->transform,cf.v)).z;
//        float forVel = f3_normlalize(f3_mul_s(cf.e.transform.forward,f3_length(cf.v))).z;
//        f3 latVector3 = cf.e.t.InverseTransformDirection(cf.v).normalized;
        f3 latVector3 = f3_normalize(fmj_3dtrans_world_to_local_dir(&cf.e->transform,cf.v));
//                                f3 latDirVector3 = cf.e.t.TransformDirection(new float3(latVector3.x, 0, 0));                                
//        f3 latDirVector3 = cf.e.t.TransformDirection(new float3(latVector3.x, 0, 0));
        f3 latDirVector3 = fmj_3dtrans_local_to_world_dir(&cf.e->transform,f3_create(latVector3.x, 0, 0));                                
        f32 latVel = latVector3.x;
        f32 evaluated_force = 0;
        f32 slipAngle = degrees(atan(latVel / abs(forVel)));// * Mathf.Rad2Deg;
        if (isnan(slipAngle)) slipAngle = 0;

        evaluated_force = fmj_animation_curve_evaluate(slip_angle_curve,abs(slipAngle));

//        f32 sign = Mathf.Sign((int)slipAngle);
        f32 sign = signbit(slipAngle);
        if (isnan(evaluated_force)) evaluated_force = 0;
        if (sign == 0) sign = 1;
        float no_of_tires = 4;

        f3 a = f3_mul_s(f3_mul_s(f3_mul_s(latDirVector3,evaluated_force),no_of_tires),-1);
        evaluated_vector = f3_project_on_plane(a,cf.last_track_normal);
    }
    else
    {

        evaluated_vector = f3_create_f(0);
    }

    f3 lateral_force = evaluated_vector;
    if (fmj_3dtrans_world_to_local_dir(&cf.e->transform,(cf.v)).z < 1)
    {
        lateral_force = f3_create_f(0);
    }
    return f3_div_s(lateral_force,cf.desc.mass);
}



f3 CalculateRudderLift(CarFrame* af,FMJCurves f16lc)
{
    f32 CI = af->last_aos;
    CI = fmj_animation_curve_evaluate(f16lc,CI);

    f3 result = f3_create_f(0);
    f32 velocity = af->indicated_ground_speed;
    f32 velocity_squared = (velocity * velocity);
    f32 dynamic_pressure = 0.5f * (air_density * velocity_squared);
    f32 lift_vector = CI * af->repulsive_force_area * dynamic_pressure;
    f3 cd = af->e->transform.right;
    f3 final_lift_vector = f3_mul_s(cd,lift_vector);
    //Debug.Log("ThrustVector : " + airframe.thrust_vector + "Lift vector : " + final_lift_vector);
    af->rudder_lift_vector = result = f3_div_s(final_lift_vector,af->desc.mass);
    return result;
}

f3 CalculateTorqueForce(f3 p_of_force, f3 force)
{
    return cross(p_of_force, force);
}

void UpdateOrientation(CarFrame* carframe,f3 push_p,f3 push_dir,float force,f3 push_p_roll,f3 roll_dir,float roll_force,f32 delta_time)
{
    ASSERT(carframe)
        if(carframe->is_grounded)
        {
            f3 yaw_torque = f3_mul_s(CalculateTorqueForce(push_p_roll, roll_dir),roll_force);

//            carframe.rb.AddRelativeTorque(yaw_torque);
//            yaw_torque = fmj_3dtrans_world_to_local_dir(&carframe->e->transform,yaw_torque);            
            PhysicsCode::AddRelativeTorqueForce(carframe->rb,&carframe->e->transform,yaw_torque);

                f3 kart_forward_world = carframe->e->transform.forward;
            f3 kart_up_world = carframe->e->transform.up;
            float sus_speed = 5;
            f3 up_v = f3_lerp(kart_up_world, carframe->last_track_normal, f3_create_f(sus_speed * delta_time));
            f3 projected_kart_forward = f3_project_on_plane(kart_forward_world, up_v);
            carframe->e->transform.r = quaternion_look_rotation(projected_kart_forward, up_v);
        }
}

#define FMJ_PHYSICS_RAYCAST_MAX_RESULT 10
AgentGroundedTestResult GroundCast(PhysicsScene scene,CarFrame a, f3 p, f3 up,f32 distance_to_maintain)
{
    AgentGroundedTestResult result = {};
    f3 castp = (p);
    bool is_hit = false;

    float distance_from_hit_p = FLT_MAX;
    float ray_length = 1.1f;
    int max_results = 10;
    PxRaycastBuffer hit;
    f3 neg_up = f3_negate(up);
    PxVec3 pp = {p.x,p.y,p.z};
    PxVec3 upp = {neg_up.x,neg_up.y,neg_up.z};
    bool status = scene.state->raycast(pp,upp,ray_length,hit);        
    if (status && hit.hasBlock && hit.block.shape != NULL)
    {
        result.distance_from_hit_p = hit.block.distance;
        result.hit_p = f3_create(hit.block.position.x, hit.block.position.y, hit.block.position.z);
        result.hit_n = f3_create(hit.block.normal.x, hit.block.normal.y, hit.block.normal.z);
        result.is_hit = true;
        if (result.distance_from_hit_p < a.sus_height)
        {
            result.is_grounded = true;
        }
    }
    return result;
}


f3 fmj_physics_calculate_acceleration(CarFrame airframe,FMJCurves f16lc,FMJCurves f16rlc,FMJCurves sa_curve,f3 stick_input_throttle)
{
    f3 acceleration = f3_create_f(0);
    //Mass check
    if (airframe.desc.mass <= 0)
    {
        airframe.desc.mass = 1;
//        fmj_editor_console_add_entry("CarFrame with zero mass can not exist set to 1.  Check your description!");
//            Debug.LogError("CarFrame with zero mass can not exist set to 1.  Check your description!");
    }
        
    if(!airframe.is_grounded)
    {
        acceleration = f3_add(acceleration,fmj_physics_calculate_gravity(airframe.desc.mass,f3_create(0, -9.806f, 0)));            
    }

    airframe.thrust_vector = fmj_physics_calculate_thrust(airframe, stick_input_throttle);
    acceleration = f3_add(acceleration,airframe.thrust_vector);
    acceleration = f3_add(acceleration,fmj_physics_calculate_drag(airframe, f16rlc));
    //acceleration += CalculateRudderLift(airframe, f16lc);
    acceleration = f3_add(acceleration,fmj_physics_calculate_lateral_force(airframe, sa_curve));
    return acceleration;
}

void UpdateCarFrames(PlatformState* ps,CarFrameBuffer* buffer,FMJCurves f16lc,FMJCurves f16rlc,FMJCurves sa_curve,float timestep,f3 stick_input_throttle,PhysicsScene pscene)
    {
        //NOTE(Ray):This is verlet integration.
        for (int i = 0; i < buffer->car_frames.fixed.count; i++)
        {
            CarFrame* carframe = fmj_stretch_buffer_check_out(CarFrame,&buffer->car_frames,i);
//            f3 localz = carframe.e.t.InverseTransformDirection(carframe.v);
            f3 localz = fmj_3dtrans_world_to_local_dir(&carframe->e->transform,carframe->v);
            float ground_speed = localz.z;
            //NOTE(Ray):If an airframe is flying backwards 0 lift something is really wrong//
            //VTOL aircraft and helo's this does not cover.
            if (ground_speed < 0)
            {
                ground_speed = 0;
            }
            carframe->indicated_ground_speed = ground_speed;
            f32 distance_to_maintain_from_ground = 0.05f;

            AgentGroundedTestResult grounded_result = GroundCast(pscene,*carframe, carframe->e->transform.p, carframe->e->transform.up, distance_to_maintain_from_ground);

            if (grounded_result.is_hit)
            {
                carframe->is_grounded = true;
                carframe->last_track_normal = grounded_result.hit_n;

                carframe->last_hit_p = grounded_result.hit_p;
                f32 d = f3_distance(carframe->e->transform.p, carframe->last_hit_p);
                f32 penetration_distance = (sus_height - 0.01f) - d;
                carframe->e->transform.p.y = (carframe->e->transform.p.y + penetration_distance);
            }
            else
            {
                carframe->is_grounded = false;
            }

//            f3 lsv = carframe->e.t.InverseTransformVector(carframe.v);
            f3 lsv = fmj_3dtrans_world_to_local_dir(&carframe->e->transform,carframe->v);            
//            lsv = nornormalizesafe(lsv, f3(0));
            lsv = f3_normalize(lsv);
            
//            f3 hsv = normalizesafe(f3(lsv.x, 0, lsv.z), f3(0));
            f3 hsv = f3_normalize(f3_create(lsv.x, 0, lsv.z));
    
//            lsv = normalizesafe(f3(0, lsv.y, lsv.z), f3(0));
            lsv = f3_normalize(f3_create(0, lsv.y, lsv.z));
            
//            f3 fsv = carframe->e.t.InverseTransformVector(carframe->e.t.forward);
            f3 fsv =  carframe->e->transform.forward;
//            f3 usv = carframe->e.t.InverseTransformVector(f3.up);
            f3 usv = carframe->e->transform.up;
//            f3 rsv = carframe->e.t.InverseTransformVector(f3.right);
            f3 rsv = carframe->e->transform.right;
            
//            float angle_of_attack = f3.SignedAngle(lsv, fsv, f3.right);
//            float angle_of_slip = f3.SignedAngle(hsv, fsv, f3.up);
            float angle_of_attack = f3_signed_angle(lsv, fsv, f3_create(1,0,0));
            float angle_of_slip = f3_signed_angle(hsv, fsv, f3_create(0,1,0));
            
            //Debug.DrawRay(carframe->e.p, lsv * 10, Color.cyan);
//            Debug.DrawRay(carframe->e.p, hsv * 10, Color.cyan);

            carframe->last_aoa = angle_of_attack;
            carframe->last_aos = angle_of_slip;

            f3 push_p_roll = f3_create(0, 0, 0);
            f3 roll_dir = f3_create(1, 0, 0);
            float turn_power = 2;

            if(ps->input.keyboard.keys[keys.d].down)
//            if (Input.GetKey(KeyCode.D))
            {
                stick_input_throttle.x = -turn_power;
                push_p_roll = f3_create(0.5f, 0, 0.5f);
            }

            if(ps->input.keyboard.keys[keys.a].down)
//            if (Input.GetKey(KeyCode.A))
            {
                stick_input_throttle.x = turn_power;
                push_p_roll = f3_create(-0.5f, 0, 0.5f);
            }

            f2 screen = f2_create(ps->window.dim.x, ps->window.dim.y);
            f2 half_screen = f2_mul_s(screen,0.5f);
            

            if(ps->input.mouse.lmb.released)
//            if (Input.GetMouseButton(0))
            {
//                Debug.Log("Click detected");
            }

            if(ps->input.mouse.lmb.down)
//            if(Input.GetMouseButton(0))
            {
                f2 touch_p;
                touch_p = f2_create(ps->input.mouse.p.x,ps->input.mouse.p.y);
                if (touch_p.x > half_screen.x)
                {
                    //Go right
                    f32 center_x_p = screen.x - half_screen.x;
                    f32 touch_offset_x = touch_p.x - half_screen.x;
                    f32 screen_distance_ratio = touch_offset_x / center_x_p;
                    
                    stick_input_throttle.x = (-turn_power * screen_distance_ratio);
                    push_p_roll = f3_create(0.5f, 0, 0.5f);
                }
                if (touch_p.x < half_screen.x)
                {
                    //Go left
                    f32 touch_offset_x = abs(touch_p.x - half_screen.x);
                    f32 screen_distance_ratio = touch_offset_x / half_screen.x;// - 1.0f;
                    stick_input_throttle.x = (turn_power * screen_distance_ratio);
                    push_p_roll = f3_create(-0.5f, 0, 0.5f);
                }
            }
//            else if (ps->input.touchCount > 1 || Input.touchCount == 0)
            {
                // Go straight
            }

            
            stick_input_throttle = f3_mul_s(stick_input_throttle,5.5f);



//            UpdateOrientation(carframe,push_p,f3 push_dir,float force,f3 push_p_roll,f3 roll_dir,float roll_force,f32 delta_time)            
            UpdateOrientation(carframe,f3_create(0, 1, -1),f3_create(0, -1, 0),-stick_input_throttle.y,push_p_roll,roll_dir,-stick_input_throttle.x,ps->time.delta_seconds);



//Debug.Log("aoa: " + carframe.last_aoa + "aos: " + carframe.last_aos);
            //linear forces

            f3 Acceleration = fmj_physics_calculate_acceleration(*carframe,f16lc,f16rlc,sa_curve, stick_input_throttle);
    
//            f3 Acceleration = CalculateAcceleration(carframe,f16lc,f16rlc,sa_curve, stick_input_throttle);
            carframe->e->transform.p = f3_add(carframe->e->transform.p,f3_mul(f3_s_mul(timestep,f3_add_s(carframe->v,timestep)),f3_mul_s(Acceleration,0.5f)));
            carframe->v = f3_add(carframe->v,f3_s_mul(timestep,Acceleration));
            f3 NewAcceleration = fmj_physics_calculate_acceleration(*carframe,f16lc, f16rlc,sa_curve, stick_input_throttle);
            carframe->v = f3_add(carframe->v,f3_mul_s(f3_s_mul(timestep,f3_sub(NewAcceleration,Acceleration)),0.5f));
            carframe->last_accel = NewAcceleration;
            
            //Update visual only elements
//            UpdateWheelOrientationPosition(carframe,stick_input_throttle.x);

//            Debug.DrawRay(carframe->e.p, normalize(carframe.lift_vector) * 5, Color.red);
//            Debug.DrawRay(carframe->e.p, normalize(carframe.rudder_lift_vector) * 5, Color.yellow);
//            Debug.DrawRay(carframe->e.p, normalize(carframe.thrust_vector) * 5, Color.green);
//            Debug.DrawRay(carframe->e.p, normalize(carframe.v) * 5, Color.black);

//            Debug.DrawLine(carframe->e.p, carframe.last_p, Color.green,1000);
            carframe->last_p = carframe->e->transform.p;
        }
    }

#if 0

    public static void UpdateWheelOrientationPosition(CarFrame cf,float input)//1 to -1
    {
        input = clamp(1, -1, input);
        float max_angle = cf.desc.maximum_wheel_angle;
        float angle = max_angle * -input;
        float spin_angle = 10;
        Quaternion speed_amount = Quaternion.AngleAxis(cf.current_spin_angle, f3(1, 0, 0)); ;
        Quaternion steering_amount = Quaternion.AngleAxis(angle, cf.e.t.up);
        cf.current_spin_angle += cf.indicated_ground_speed;
        for (int w = 0; w < 4; w++)
        {
            Transform t = cf.wheel_transforms[w];
            float distance_to_maintain_from_ground = 0.05f;

            f3 cast_p = f3(0);
            //tire cast p
            float half_body_lenth_forward = 2.6f;
            float half_body_lenght_rear = 3.2f;
            float half_body_width_front = 1.8f;
            float half_body_width_rear = 2.0f;
            switch(w)
            {
                case 0:
                {
                    cast_p = (cf.e.t.position + cf.e.t.forward * half_body_lenth_forward) + cf.e.t.right * half_body_width_front;
                }
                break;
                case 1:
                {
                    cast_p = (cf.e.t.position + cf.e.t.forward * half_body_lenth_forward) + -cf.e.t.right * half_body_width_front;
                }
                break;
                case 2:
                {
                    cast_p = (cf.e.t.position + -cf.e.t.forward * half_body_lenght_rear) + cf.e.t.right * half_body_width_rear;
                }
                break;
                case 3:
                {
                    cast_p = (cf.e.t.position + -cf.e.t.forward * half_body_lenght_rear) + -cf.e.t.right * half_body_width_rear;
                }
                break;
            }

           
            Debug.DrawRay(cast_p,-cf.e.t.up * sus_height,Color.green);
            agent_grounded_test_result grounded_result = GroundCast(cf, cast_p, cf.e.t.up, distance_to_maintain_from_ground);

            if (grounded_result.is_hit)
            {
                f3 hit_p = grounded_result.hit_p;
                float d = distance(t.position, hit_p);
                float penetration_distance = (sus_height - 0.01f) - d;
                //t.position = f3(t.position.x, (t.position.y + penetration_distance),t.position.z);

                //t.position = hit_p;
                float half_wheel_radius = 0.70f;
                t.position = hit_p + f3(0, half_wheel_radius, 0);
            }
            else
            {
             //Max length of spring from rest 
                //t.position = f3(t.position.x, (t.position.y + penetration_distance),t.position.z);
            }
            Quaternion nr = cf.e.t.rotation;
            if (w == 0 || w == 1)
            {
                nr *= steering_amount * speed_amount;
                t.rotation = nr;
            }
            else
            {
                t.rotation = nr * speed_amount;
            }

        }
    }

#endif




