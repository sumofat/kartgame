#if !defined (EDITOR_H)

#define EDITOR_CONSOLE_ENTRY_MAX_TEXT_SIZE 1024
#define EDITOR_CONSOLE_ENTRY_MAX_BEFORE_RESET 5000

struct FMJBezierCubicCurve
{
    f2 points[4];
    f2 min_max;
    f2 min_max_range;
    //NOTE(Ray):These handles are just for the user to input the desired
    //tangent angle and than a cp is calculated to reach the desired tangent.
    //the user never sees the actual control points.
    f2 handle_a;//position of the visual handles
    f2 handle_b;//
};

struct FMJEditorConsole
{
    bool is_showing;
    FMJStretchBuffer commands;
    FMJMemoryArena string_mem;
    FMJMemoryArena scratch_mem;
    FMJStretchBuffer items;
//Testing
    FMJStretchBuffer points;
    FMJBezierCubicCurve unfinished_point;
    bool point_in_progress;
    int selected_points_index;

    float test_x;
    f2 last_result = f2_create_f(0);
};

float bezier_tangent(float t, f32 a, f32 b, f32 c, f32 d)
{
    f32 C1 = ( d - (3.0 * c) + (3.0 * b) - a );
    f32 C2 = ( (3.0 * c) - (6.0 * b) + (3.0 * a) );
    f32 C3 = ( (3.0 * b) - (3.0 * a) );
    f32 C4 = ( a );
    return ( ( 3.0 * C1 * t* t ) + ( 2.0 * C2 * t ) + C3 );
}

void fmj_editor_update_cp_and_handle(f2* handle_p_out,f2* cp_out,f32 handle_size,f2 mp,f2 p1,f2 p3,bool is_move_handle)
{
    f32 length = f2_distance(p3,p1);
    length = length * 0.333f;                                     
    if(is_move_handle)
    {
        f2 new_dir = f2_mul_s(f2_normalize(f2_sub(mp,p1)),handle_size);
        *handle_p_out = f2_add(p1,new_dir);
    }

    f2 dir = f2_mul_s(f2_normalize(f2_sub(p3,p1)),length);
    f32 new_x = p1.x + dir.x;
    f2 local_handle_p = f2_sub(*handle_p_out,p1);
    f32 new_y = (local_handle_p.y / local_handle_p.x) * dir.x;
    f2 new_p = f2_create(new_x,p1.y + new_y);
    *cp_out = new_p;    
}

void fmj_editor_console_init(FMJEditorConsole* console)
{
    ASSERT(console);
    if(console)
    {
        console->is_showing = false;
//        console->commands = fmj_stretch_buffer_init(1,)
        console->string_mem = fmj_arena_allocate(FMJMEGABYTES(2));
        console->scratch_mem = fmj_arena_allocate(FMJKILOBYTES(512));        
        console->items = fmj_stretch_buffer_init(1,sizeof(FMJString),8);
        console->points = fmj_stretch_buffer_init(1,sizeof(FMJBezierCubicCurve),8);
        console->selected_points_index = -1;
    }
}

void fmj_editor_console_add_entry_(FMJEditorConsole* console,char* a)
{
    if(console->items.fixed.count > EDITOR_CONSOLE_ENTRY_MAX_BEFORE_RESET)
    {
        fmj_arena_deallocate(&console->string_mem,false);
        fmj_stretch_buffer_clear(&console->items);        
    }

    FMJString test = fmj_string_create(a,&console->string_mem);    
    fmj_stretch_buffer_push(&console->items,&test);    
}

void fmj_editor_console_add_entry(FMJEditorConsole* console,const char* fmt, ...)
{
    char buf[EDITOR_CONSOLE_ENTRY_MAX_TEXT_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf)-1] = 0;
    va_end(args);
    char* a = &buf[0];
    fmj_editor_console_add_entry_(console,a);    
}
 
f32 mix(f32 a, f32 b, f32 t)
{
    // degree 1
    return a * (1.0f - t) + b*t;
}
 
f32 BezierQuadratic(f32 A, f32 B, f32 C, f32 t)
{
    // degree 2
    f32 AB = mix(A, B, t);
    f32 BC = mix(B, C, t);
    return mix(AB, BC, t);
}
 
f32 BezierCubic(f32 A, f32 B, f32 C, f32 D, f32 t)
{
    // degree 3
    f32 ABC = BezierQuadratic(A, B, C, t);
    f32 BCD = BezierQuadratic(B, C, D, t);
    return mix(ABC, BCD, t);
}

void fmj_editor_console_show(FMJEditorConsole* console)
{
    ImGui::SetNextWindowSize(ImVec2(520,600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("KartLog", &console->is_showing))
    {
        ImGui::End();
        return;
    }

    // As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar. So e.g. IsItemHovered() will return true when hovering the title bar.
    // Here we create a context menu only available from the title bar.
    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Close Console"))
            console->is_showing = false;
        ImGui::EndPopup();
    }
    
    ImGui::TextWrapped("This example implements a console with basic coloring, completion and history. A more elaborate implementation may want to store entries along with extra data such as timestamp, emitter, etc.");
    ImGui::TextWrapped("Enter 'HELP' for help, press TAB to use text completion.");

    ImGui::SameLine();

    ImGui::Separator();

    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
    if (ImGui::BeginPopupContextWindow())
    {
        ImGui::EndPopup();
    }
    
    ImGui::Separator();
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4,1)); // Tighten spacing
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f)); 
    for(int i = 0;i < console->items.fixed.count;++i)
    {
        FMJString item = fmj_stretch_buffer_get(FMJString,&console->items,i);
        ImGui::TextUnformatted(item.string);
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();

    f32 min = 0;
    f32 max = 400;

    ImGui::Text("Curve Editor");

    ImVec2 p = ImGui::GetCursorScreenPos();

    static ImVec4 colf = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
     const ImU32 col = ImColor(colf);
     ImDrawList* draw_list = ImGui::GetWindowDrawList();

     static ImVec4 grey = ImVec4(0.8f, 0.8f, 0.8f, 0.5f);
     const ImU32 grey_col = ImColor(grey);

     int lines_count = 20;
     int width = max;
     int height = max;
     ImVec2 canvas_size = ImVec2(max,max);//ImGui::GetContentRegionAvail();     
     int offset = width / lines_count;
     f2 grid_top_left = f2_create(50,0);
     f2 final_start_grid_pos = f2_create_f(0);
     f32 max_scale_factor = 1.0f;
     f32 increment = max_scale_factor / (f32)lines_count;
     f32 next_text_num = 0;
     for(int x =0;x < lines_count;++x)
     {
         draw_list->AddLine(ImVec2(p.x + (offset * x) + grid_top_left.x,p.y + grid_top_left.y), ImVec2(p.x + (offset * x) + grid_top_left.x,p.y + height + grid_top_left.y), grey_col,1.0f);
         if(x == 0)
         {
             final_start_grid_pos.x = p.x + (offset * x) + grid_top_left.x;
         }
         if(x % 2 == 0)
         {
             FMJString out_string = fmj_string_create_formatted("%.2f",&console->scratch_mem,&console->scratch_mem,next_text_num);
             f2 bottom = f2_create(p.x + (offset * x),p.y + height);
             draw_list->AddText(ImVec2(bottom.x + grid_top_left.x,bottom.y + grid_top_left.y), col, out_string.string);                      
         }
         next_text_num += increment;         
     }
     next_text_num = max_scale_factor;     

     //horizontal lines
     for(int y = 0;y < lines_count;++y)
     {
         draw_list->AddLine(ImVec2(p.x + grid_top_left.x,p.y + (offset * y) + grid_top_left.y), ImVec2(p.x + width + grid_top_left.x,p.y + (offset * y) + grid_top_left.y), grey_col,2.0f);
         if(y == 0)
         {
             final_start_grid_pos.y = p.y + (offset * y) + grid_top_left.y;
         }
         if(y % 2 == 0)
         {
             FMJString out_string = fmj_string_create_formatted("%.2f",&console->scratch_mem,&console->scratch_mem,next_text_num);             
             f2 left = f2_create(p.x,p.y + (offset * y));         
             draw_list->AddText(ImVec2(left.x - 40 + grid_top_left.x,left.y + grid_top_left.y), col, out_string.string);                  
         }
         next_text_num -= increment;         
     }

     ImGui::SetCursorScreenPos(ImVec2(final_start_grid_pos.x,final_start_grid_pos.y));

     p = ImGui::GetCursorScreenPos();     
     ImGui::InvisibleButton("canvas", canvas_size);
     ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - p.x, ImGui::GetIO().MousePos.y - p.y);     
     if (ImGui::IsItemHovered())
     {
         draw_list->AddNgonFilled(ImVec2(p.x + mouse_pos_in_canvas.x,p.y + mouse_pos_in_canvas.y),10.0f, col, 4);
         if(ImGui::IsMouseDoubleClicked(0))
         {
             if(console->points.fixed.count == 0 && console->point_in_progress == false)//special case for first point added
             {
                 f2 point = console->unfinished_point.points[0] = f2_create(mouse_pos_in_canvas.x,mouse_pos_in_canvas.y);
                 console->point_in_progress = true;             
                 fmj_editor_console_add_entry(console,"added start point at x:%f y:%f ",point.x,point.y);                              
             }
             else if(console->point_in_progress == true)
             {
                 f2 point = f2_create(mouse_pos_in_canvas.x,mouse_pos_in_canvas.y);
                 console->unfinished_point.points[3] = point;
                 
                 f32 length = f2_distance(console->unfinished_point.points[3],console->unfinished_point.points[0]);
                 length = length * 0.333f;
                 f2 dir = f2_mul_s(f2_normalize(f2_sub(console->unfinished_point.points[3],console->unfinished_point.points[0])),length);
                 f2 opdir = f2_mul_s(f2_normalize(f2_sub(console->unfinished_point.points[0],console->unfinished_point.points[3])),length);             
             
                 console->unfinished_point.points[1] = f2_add(console->unfinished_point.points[0],dir);
                 console->unfinished_point.points[2] = f2_add(console->unfinished_point.points[3],opdir);

                 console->unfinished_point.handle_a = f2_add(console->unfinished_point.points[0],f2_mul_s(dir,0.5f));
                 console->unfinished_point.handle_b = f2_add(console->unfinished_point.points[3],f2_mul_s(opdir,0.5f));
                 
                 console->unfinished_point.min_max_range = f2_create(0,max);             
                 fmj_stretch_buffer_push(&console->points,&console->unfinished_point);
                 console->unfinished_point.points[0] = f2_create_f(0);
                 console->unfinished_point.points[1] = f2_create_f(0);
                 console->unfinished_point.points[2] = f2_create_f(0);
                 console->unfinished_point.points[3] = f2_create_f(0);
                 console->point_in_progress = false;
             }
             else if(console->points.fixed.count > 0 && console->point_in_progress == false)
             {
//add new curve after first curve is full complete
                 ASSERT(console->points.fixed.count > 0);
                 
                 FMJBezierCubicCurve prev_curve = fmj_stretch_buffer_get(FMJBezierCubicCurve,&console->points,console->points.fixed.count - 1);
                 console->unfinished_point.points[0] = prev_curve.points[3];
                 
                 f2 point = f2_create(mouse_pos_in_canvas.x,mouse_pos_in_canvas.y);
                 console->unfinished_point.points[3] = point;

                 f32 length = f2_distance(console->unfinished_point.points[3],console->unfinished_point.points[0]);
                 length = length * 0.333f;
                 f2 dir = f2_mul_s(f2_normalize(f2_sub(console->unfinished_point.points[3],console->unfinished_point.points[0])),length);
                 f2 opdir = f2_mul_s(f2_normalize(f2_sub(console->unfinished_point.points[0],console->unfinished_point.points[3])),length);             
             
                 console->unfinished_point.points[1] = f2_add(console->unfinished_point.points[0],dir);
                 console->unfinished_point.points[2] = f2_add(console->unfinished_point.points[3],opdir);

                 console->unfinished_point.handle_a = f2_add(console->unfinished_point.points[0],f2_mul_s(dir,0.5f));
                 console->unfinished_point.handle_b = f2_add(console->unfinished_point.points[3],f2_mul_s(opdir,0.5f));

                 console->unfinished_point.min_max_range = f2_create(0,max);
                 fmj_stretch_buffer_push(&console->points,&console->unfinished_point);
                 console->unfinished_point.points[0] = f2_create_f(0);
                 console->unfinished_point.points[1] = f2_create_f(0);
                 console->unfinished_point.points[2] = f2_create_f(0);
                 console->unfinished_point.points[3] = f2_create_f(0);
                 console->point_in_progress = false;
             }
         }
         else
         {
             console->selected_points_index = -1;             
         }

         if(ImGui::IsMouseDown(0))
         {
             if(console->point_in_progress)
             {
                 f2 mp = f2_create(mouse_pos_in_canvas.x,mouse_pos_in_canvas.y);                 
                 f2 test_p = console->unfinished_point.points[0];
                 f32 distance = abs(f2_length(f2_sub(test_p,mp)));
                 if(distance < 5)
                 {
                     f2 new_p = f2_create(mp.x,mp.y);                     
                     console->unfinished_point.points[0] = new_p;
                     fmj_editor_console_add_entry(console,"found point at x:%f y:%f ",new_p.x,new_p.y);
                 }                 
             }
             else
             {
                 for(int i = 0;i < console->points.fixed.count;++i)
                 { 
                     FMJBezierCubicCurve* curve = fmj_stretch_buffer_check_out(FMJBezierCubicCurve,&console->points,i);
                     f2 p1 = curve->points[0];
                     f2 p2 = curve->points[3];
                     f2 c1 = curve->points[1];
                     f2 c2 = curve->points[2];
                 
                     f2 mp = f2_create(mouse_pos_in_canvas.x,mouse_pos_in_canvas.y);
                     for(int i = 0;i < 4;++i)
                     {
                         f2 test_p = curve->points[i];
                         if(i == 1 || i == 2)//control points
                         {
                             if(i == 1)
                                 test_p = curve->handle_a;
                             if(i == 2)
                                 test_p = curve->handle_b;
                             
                             f32 distance = abs(f2_length(f2_sub(test_p,mp)));
//                             f32 tan = 
                             if(distance < 5)
                             {
                                 f32 length = f2_distance(curve->points[3],curve->points[0]);
                                 length = length * 0.333f;                                 
                                 if(i == 1)
                                 {
                                     fmj_editor_update_cp_and_handle(&curve->handle_a,&curve->points[1],50,mp,curve->points[0],curve->points[3],true);
#if 0                                     
                                     f2 new_dir = f2_mul_s(f2_normalize(f2_sub(mp,curve->points[0])),50);
                                     curve->handle_a = f2_add(curve->points[0],new_dir);

                                     f2 dir = f2_mul_s(f2_normalize(f2_sub(curve->points[3],curve->points[0])),length);
                                     f32 new_x = curve->points[0].x + dir.x;
                                     f2 local_handle_p = f2_sub(curve->handle_a,curve->points[0]);
                                     f32 new_y = (local_handle_p.y / local_handle_p.x) * dir.x;
                                     f2 new_p = f2_create(new_x,curve->points[0].y + new_y);
                                     curve->points[1] = new_p;
#endif
                                 }
                                 else if(i == 2)
                                 {
                                     fmj_editor_update_cp_and_handle(&curve->handle_b,&curve->points[2],50,mp,curve->points[3],curve->points[0],true);
                                     
#if 0
                                     f2 new_dir = f2_mul_s(f2_normalize(f2_sub(mp,curve->points[3])),50);
                                     curve->handle_b = f2_add(curve->points[3],new_dir);

                                     f2 oppdir = f2_mul_s(f2_normalize(f2_sub(curve->points[0],curve->points[3])),length);                                     
                                     f32 new_x = curve->points[3].x + oppdir.x;
                                     f2 local_handle_p = f2_sub(curve->handle_b,curve->points[3]);
                                     f32 new_y = (local_handle_p.y / local_handle_p.x) * oppdir.x;
                                     f2 new_p = f2_create(new_x,curve->points[3].y + new_y);
                                     curve->points[2] = new_p;
#endif
                                 }

                                 console->selected_points_index = i;
                                 break;
                             }                                                          
                         }
                         else
                         {
                             f32 distance = abs(f2_length(f2_sub(test_p,mp)));
                             if(distance < 5)
                             {
                                 console->selected_points_index = i;
                                 f2 new_p = f2_create(mp.x,mp.y);
                                 f2 move_diff = f2_sub(new_p,curve->points[i]);
                                 if(i == 0)
                                 {
                                     curve->handle_a = f2_add(move_diff,curve->handle_a);
                                 }
                                 else if(i == 3)
                                 {
                                     curve->handle_b = f2_add(move_diff,curve->handle_b);                                     
                                 }
                                 
                                 curve->points[i] = new_p;//f2_add(move_diff,curve->points[i]);//new_p;
                                 fmj_editor_console_add_entry(console,"found point at x:%f y:%f ",new_p.x,new_p.y);
                                 break;
                             }                             
                         }
                     }


                     fmj_editor_update_cp_and_handle(&curve->handle_a,&curve->points[1],50,mp,curve->points[0],curve->points[3],false);
                     fmj_editor_update_cp_and_handle(&curve->handle_b,&curve->points[2],50,mp,curve->points[3],curve->points[0],false);
                     
                     fmj_stretch_buffer_check_in(&console->points);                 
                 }                                  
             }
         }
     }

     if(console->points.fixed.count == 0 && console->point_in_progress)
     {
         f2 p1 = console->unfinished_point.points[0];
         draw_list->AddNgonFilled(ImVec2(p.x + p1.x,p.y + p1.y),4.0f, col, 10);         
     }
     else
     {
         for(int i = 0;i < console->points.fixed.count;++i)
         { 
             FMJBezierCubicCurve curve = fmj_stretch_buffer_get(FMJBezierCubicCurve,&console->points,i);
             f2 p1 = curve.points[0];
             f2 p2 = curve.points[3];
             f2 c1 = curve.points[1];
             f2 c2 = curve.points[2];

             draw_list->AddNgonFilled(ImVec2(p.x + p1.x,p.y + p1.y),4.0f, col, 10);
             draw_list->AddNgonFilled(ImVec2(p.x + p2.x,p.y + p2.y),4.0f, col, 10);
#if 0
             draw_list->AddNgonFilled(ImVec2(p.x + c1.x,p.y + c1.y),8.0f, col, 4);
             draw_list->AddNgonFilled(ImVec2(p.x + c2.x,p.y + c2.y),8.0f, col, 4);
#else
             
             draw_list->AddNgonFilled(ImVec2(p.x + curve.handle_a.x,p.y + curve.handle_a.y),8.0f, col, 4);
             draw_list->AddNgonFilled(ImVec2(p.x + curve.handle_b.x,p.y + curve.handle_b.y),8.0f, col, 4);
             
#endif
             
             draw_list->AddLine(ImVec2(p.x + p1.x,p.y + p1.y), ImVec2(p.x + (curve.handle_a.x),p.y + curve.handle_a.y), col,1.0f);
             draw_list->AddLine(ImVec2(p.x + p2.x,p.y + p2.y), ImVec2(p.x + curve.handle_b.x,p.y + curve.handle_b.y)  , col,1.0f);
         
             draw_list->AddBezierCurve(ImVec2(p.x + p1.x,p.y + p1.y), ImVec2(p.x + c1.x,p.y + c1.y), ImVec2(p.x + c2.x,p.y + c2.y), ImVec2(p.x + p2.x,p.y + p2.y), col, 3,20);
         }
     }

     
     draw_list->AddNgonFilled(ImVec2(p.x + console->last_result.x,p.y + console->last_result.y),8.0f, col, 4);
//extra spacing

     f2 extra_size = f2_create_f(100);
     ImGui::InvisibleButton("spacing",ImVec2(extra_size.x,extra_size.y));

     ImGui::SliderFloat("test_x",&console->test_x,0,1.0f);     
//     ImGui::SliderFloat2("test_v",t, float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);     
     if(ImGui::Button("Save"))
     {
         
         //teste evaluate
         //first find the curve we are in against scale x
         //than evalute the curve  point at t to get y
         //than map y pos to our scale y

         for(int i = 0;i < console->points.fixed.count;++i)
         {
             FMJBezierCubicCurve curve = fmj_stretch_buffer_get(FMJBezierCubicCurve,&console->points,i);
             float p1x = curve.points[0].x;//minx
             float p2x = curve.points[3].x;//maxx
             f32 test_x_in_time_t = console->test_x;
             float test_x_in_drawing_coridinates = (test_x_in_time_t * curve.min_max_range.y);
             if((p1x) < test_x_in_drawing_coridinates && (p2x) > test_x_in_drawing_coridinates)
             {
                 f2 p1 = curve.points[0];
                 f2 p2 = curve.points[3];
                 f2 c1 = curve.points[1];
                 f2 c2 = curve.points[2];

                 f32 t_of_x_in_drawing_cords = lerp(0,curve.min_max_range.y,test_x_in_time_t);

                 f32 test_x_px_diff_in_drawing_cords  = test_x_in_drawing_coridinates - p1.x;                 
                 f32 length_of_segment_drawing_cords = p2.x - p1.x;
                 f32 tesx_x_in_local_normalized_cords = test_x_px_diff_in_drawing_cords / length_of_segment_drawing_cords;

                 float tangent = bezier_tangent(0.333f,p1.x,c1.x,c2.x,p2.x);

// float Evaluate(float t, Keyframe keyframe0, Keyframe keyframe1)
 {
//     float dt = keyframe1.time - keyframe0.time;
     float t = test_x_in_time_t;
     float dt = p2.x - p1.x;

     float cy = c1.y - p1.y;
     float c2y = c2.y - p2.y;
     float out_tan = cy/c1.x;
     float in_tan  = c2y/c2.x;
     
//     float m0 = keyframe0.outTangent * dt;
     float m0 = out_tan * dt;     
     float m1 = in_tan * dt;
 
     float t2 = t * t;
     float t3 = t2 * t;
     
     float a = 2 * t3 - 3 * t2 + 1;
     float b = t3 - 2 * t2 + t;
     float c = t3 - t2;
     float d = -2 * t3 + 3 * t2;
     
//     float finalval = a * keyframe0.value + b * m0 + c * m1 + d * keyframe1.value;
     float finalval = a * p1.y + b * m0 + c * m1 + d * p2.y;
     int aaaaa = 0;
 }
 
                 fmj_editor_console_add_entry(console,"curve tangent for  p1 %f ",tangent);
                 
                 ImVec2 v2 = ImBezierCalc(ImVec2(p1.x,p1.y),ImVec2(c1.x,c1.y),ImVec2(c2.x,c2.y),ImVec2(p2.x,p2.y),tesx_x_in_local_normalized_cords);
                 //v2.y should be the value we are after in drawing cords.
                 f2 result_c = f2_create(v2.x,v2.y);
                 console->last_result = result_c;

//                 fmj_editor_console_add_entry(console,"test eval point at x:%f y:%f ",result.x,result.y);                              
                 int a =0;
                 break;
             }
         }

         fmj_editor_console_add_entry(console,"saving curve");
     }

     ImGui::End();
     fmj_arena_deallocate(&console->scratch_mem,false);
}

#define EDITOR_H
#endif


