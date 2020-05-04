#if !defined(ANIMATION_H)

struct FMJCurve
{
    f2 points[4];
};

struct FMJCurves
{
    FMJStretchBuffer buffer;
    f2 min_max;    
};

f32 fmj_animation_curve_evaluate(FMJCurves curves,f32 test_x)
{
    for(int ci = 0;ci < curves.buffer.fixed.count;++ci)
    {
        FMJCurve curve = fmj_stretch_buffer_get(FMJCurve,&curves.buffer,ci);
        float p1x = curve.points[0].x;
        float p2x = curve.points[3].x;
        f32 test_x_in_global_t = test_x;
        if(p1x < test_x_in_global_t && p2x > test_x_in_global_t)
        {
            f2 p1 = curve.points[0];
            f2 p2 = curve.points[3];
            f2 c1 = curve.points[1];
            f2 c2 = curve.points[2];

            f32 test_x_px_diff  = test_x_in_global_t - p1.x;
            f32 length_of_segment_drawing_cords = p2.x - p1.x;
            f32 tesx_x_in_local_normalized_cords = test_x_px_diff / length_of_segment_drawing_cords;
                 
            ImVec2 v2 = ImBezierCalc(ImVec2(p1.x,p1.y),ImVec2(c1.x,c1.y),ImVec2(c2.x,c2.y),ImVec2(p2.x,p2.y),tesx_x_in_local_normalized_cords);
            return v2.y;
        }
    }
    //TODO(Ray):When trying to access a curve outside the bounds of the global timestep
    //allow for ping pong claming etc... behavior.
    return 0;
}

#define ANIMATION_H
#endif
