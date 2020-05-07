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

    FMJStretchBuffer points;
    FMJBezierCubicCurve unfinished_point;
    bool point_in_progress;
    int selected_points_index;
    int selected_curve_index;
    
    float test_x;
    f2 last_result = f2_create_f(0);
    f2 prev_mp;
    f2 mp;
    f2 grid_pos;
    bool is_moving_point;
    FMJBezierCubicCurve* moving_curve;

    //test save output
    FMJCurves curves_output;
};

f32 max_scale_factor = 1.0f;
f32 max_scale_factor_x = 1.0f;

void fmj_editor_update_handle(f2* handle_p_out,f2 handle_size,f2 mp,f2 p1_with_offset,f2 p1,f2 p3)
{
    f32 length = f2_distance(p3,p1);
    length = length * 0.333f;                                     
    f2 new_dir = f2_mul(f2_normalize(f2_sub(mp,p1)),handle_size);
    *handle_p_out = f2_add(p1,new_dir);
}

void fmj_editor_update_cp(f2 handle_p,f2* cp_out,f2 handle_size,f2 p1,f2 p3)
{
    f32 length = f2_distance(p3,p1);
    length = length * 0.333f;                                     
    f2 dir = f2_mul_s(f2_normalize(f2_sub(p3,p1)),length);
    f32 new_x = p1.x + dir.x;
    f2 local_handle_p = f2_sub(handle_p,p1);
    f32 new_y = (local_handle_p.y / local_handle_p.x) * dir.x;
    f2 new_p = f2_create(new_x,p1.y + new_y);
    *cp_out = new_p;    
}

f2 fmj_editor_apply_scale_f2(f2 p,f2 point,f2 scale_factors,f2 dim)
{
    f32 scaled_x = p.x + (point.x * scale_factors.x);
    f32 scaled_y = ((dim.y - point.y) - ((dim.y - point.y) * scale_factors.y)) + (p.y + point.y);
    return f2_create(scaled_x,scaled_y);
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
        console->selected_curve_index = -1;
        console->grid_pos = f2_create_f(0);
        console->curves_output.buffer = fmj_stretch_buffer_init(1,sizeof(FMJBezierCubicCurve),8);
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

ImVec2 canvas_size;// = ImVec2(max,max);
void fmj_editor_draw_grid(FMJEditorConsole* console,f2 dim)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    f2 mpdiff = f2_sub(console->mp,console->prev_mp);

    static ImVec4 colf = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
    const ImU32 col = ImColor(colf);
    static ImVec4 grey = ImVec4(0.8f, 0.8f, 0.8f, 0.5f);
    const ImU32 grey_col = ImColor(grey);

    int lines_count = 20;
    int width = dim.x;
    int height = dim.y;

    int offset = width / lines_count;
    int base_offset = offset;    
    f2 grid_top_left = f2_create(50,0);
    f2 final_start_grid_pos = f2_create_f(0);

    f32 increment = max_scale_factor / (f32)lines_count;
    f32 increment_x = max_scale_factor_x / (f32)lines_count;     
    f32 next_text_num = 0;
    offset = base_offset;
    next_text_num = console->grid_pos.x;;
    
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
        next_text_num += increment_x;         
    }

    next_text_num = max_scale_factor;
    
    offset = base_offset;// + mpdiff.y;
    next_text_num += console->grid_pos.y;
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
}

void fmj_editor_add_end_point_at_mp(FMJEditorConsole* console,f2 mp,f2 sfinverse,f2 sf,f2 grid_dim)
{
    f2 prev_p_scaled  = fmj_editor_apply_scale_f2(f2_create_f(0),console->unfinished_point.points[0],sfinverse,f2_create(grid_dim.x,grid_dim.y));
    f2 ps_diff = f2_sub(mp,prev_p_scaled);
    f2 scaled_diff  = f2_mul(ps_diff,sf);
    f2 point = f2_add(console->unfinished_point.points[0],scaled_diff);
    console->unfinished_point.points[3] = point;

    f2 offset = f2_mul(console->grid_pos,grid_dim);
    offset = f2_create(offset.x,-offset.y);
    f2 p1_offset = f2_sub(console->unfinished_point.points[0],offset);
            
    fmj_editor_update_handle(&console->unfinished_point.handle_a,
                         f2_create_f(50),
                         console->unfinished_point.points[3],
                         p1_offset,
                         console->unfinished_point.points[0],
                         console->unfinished_point.points[3]);
    
    fmj_editor_update_cp(console->unfinished_point.handle_a,
                         &console->unfinished_point.points[1],
                         f2_create_f(50),
                         console->unfinished_point.points[0],
                         console->unfinished_point.points[3]);
            
    fmj_editor_update_handle(&console->unfinished_point.handle_b,
                         f2_create_f(50),
                         console->unfinished_point.points[0],
                         p1_offset,
                         console->unfinished_point.points[3],
                         console->unfinished_point.points[0]);
    
    fmj_editor_update_cp(console->unfinished_point.handle_b,
                         &console->unfinished_point.points[2],
                         f2_create_f(50),
                         console->unfinished_point.points[3],
                         console->unfinished_point.points[0]);

//    fmj_editor_update_cp_and_handle(&console->unfinished_point.handle_b,&console->unfinished_point.points[2],f2_create_f(50),console->unfinished_point.points[0],console->unfinished_point.points[3],console->unfinished_point.points[0],true);    
//    fmj_editor_update_cp_and_handle(&console->unfinished_point.handle_b,&console->unfinished_point.points[2],f2_create_f(50),console->unfinished_point.points[0],console->unfinished_point.points[3],console->unfinished_point.points[0],true);

    console->unfinished_point.min_max_range = f2_create(0,grid_dim.y);
                 
    fmj_stretch_buffer_push(&console->points,&console->unfinished_point);

    console->unfinished_point.points[0] = f2_create_f(0);
    console->unfinished_point.points[1] = f2_create_f(0);
    console->unfinished_point.points[2] = f2_create_f(0);
    console->unfinished_point.points[3] = f2_create_f(0);    
}

void fmj_editor_add_point_at_mp(FMJEditorConsole* console,f2 mp,f2 sfinverse,f2 sf,f2 grid_dim)
{
    FMJBezierCubicCurve prev_curve = fmj_stretch_buffer_get(FMJBezierCubicCurve,&console->points,console->points.fixed.count - 1);
    console->unfinished_point.points[0] = prev_curve.points[3];
    fmj_editor_add_end_point_at_mp(console,mp,sfinverse,sf,grid_dim);
}

void fmj_editor_check_add_points(FMJEditorConsole* console,f2 mp,f2 grid_dim,f2 sf,f2 sfinverse)
{
    if(ImGui::IsMouseDoubleClicked(0))
    {
        if(console->points.fixed.count == 0 && console->point_in_progress == false)//special case for first point added
        {
            f2 point = console->unfinished_point.points[0] = mp;
            console->point_in_progress = true;             
        }
        else if(console->point_in_progress == true)
        {
            fmj_editor_add_end_point_at_mp(console,mp,sfinverse,sf,grid_dim);
            console->point_in_progress = false;
        }
        else if(console->points.fixed.count > 0 && console->point_in_progress == false)
        {
//add new curve after first curve is full complete
            ASSERT(console->points.fixed.count > 0);
            fmj_editor_add_point_at_mp(console,mp,sfinverse,sf,grid_dim);
            console->point_in_progress = false;
        }
    }
}    

void fmj_editor_move_unfinished_point(FMJEditorConsole* console,f2 mp)
{
    f2 test_p = console->unfinished_point.points[0];
    f32 distance = abs(f2_length(f2_sub(test_p,mp)));
    if(distance < 5)
    {
        f2 new_p = f2_create(mp.x,mp.y);                     
        console->unfinished_point.points[0] = new_p;
    }                 
}

void fmj_editor_move_point_at_mp(FMJEditorConsole* console,f2 mp,f2 scale_factor,f2 scale_factor_inverted,f2 grid_dim)
{
    f2 sf_inv = scale_factor_inverted;
    
    for(int i = 0;i < console->points.fixed.count;++i)
    {
        FMJBezierCubicCurve* curve = fmj_stretch_buffer_check_out(FMJBezierCubicCurve,&console->points,i);

        f2 offset = f2_mul(console->grid_pos,grid_dim);
        offset = f2_create(offset.x,-offset.y);

        f2 p1 = curve->points[0];
        f2 p2 = curve->points[3];
        f2 c1 = curve->points[1];
        f2 c2 = curve->points[2];

        f2 handle_scale = f2_create_f(10);                     
        for(int j = 0;j < 4;++j)
        {
            f2 test_p = curve->points[j];
            f2 non_offset_test_p = test_p;

            if(j == 1 || j == 2)//control points
            {
                if(j == 1)
                    test_p = curve->handle_a;
                if(j == 2)
                    test_p = curve->handle_b;
 
                f2 scaled_test_p = f2_sub(test_p,offset);
                test_p = fmj_editor_apply_scale_f2(f2_create_f(0),scaled_test_p,scale_factor_inverted,f2_create_f(grid_dim.y));
                f32 distance = abs(f2_length(f2_sub(test_p,mp)));
                if(distance < 5)
                {
                     console->selected_points_index = j;
                     console->selected_curve_index = i;                     
                     console->is_moving_point = true;
                     console->moving_curve = curve;
                    break;
                }                                                          
            }
            else
            {
                f2 scaled_test_p = f2_sub(test_p,offset);
                test_p = fmj_editor_apply_scale_f2(f2_create_f(0),scaled_test_p,scale_factor_inverted,f2_create_f(grid_dim.y));
                f32 distance = abs(f2_length(f2_sub(test_p,mp)));
                if(distance < 5)
                {
                    console->selected_points_index = j;
                    console->selected_curve_index = i;                                         
                    console->is_moving_point = true;
                    console->moving_curve = curve;                    
                    break;
                }                             
            }
        }
    
        fmj_editor_update_cp(curve->handle_a,
                             &curve->points[1],
                             handle_scale,
                             curve->points[0],
                             curve->points[3]);
    
        fmj_editor_update_cp(curve->handle_b,
                             &curve->points[2],
                             handle_scale,
                             curve->points[3],
                             curve->points[0]);
                     
        fmj_stretch_buffer_check_in(&console->points);                 
    }                                      
}

void fmj_editor_draw_curves(FMJEditorConsole* console,f2 grid_dim,f2 p)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();        
    static ImVec4 colf = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);    
    const ImU32 col = ImColor(colf);

    f32 b = 1.0f/max_scale_factor_x;
    f32 c = 1.0f/max_scale_factor;
    f2 sfinverse = f2_create(b,c);    
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
            f2 offset = f2_mul(console->grid_pos,grid_dim);
            offset = f2_create(offset.x,-offset.y);
            f2 p1 = f2_sub(curve.points[0],offset);
            f2 p2 = f2_sub(curve.points[3],offset);
            f2 c1 = f2_sub(curve.points[1],offset);
            f2 c2 = f2_sub(curve.points[2],offset);
            f2 h1 = f2_sub(curve.handle_a,offset);
            f2 h2 = f2_sub(curve.handle_b,offset);

            f32 max = grid_dim.y;
            p1 = fmj_editor_apply_scale_f2(p,p1,sfinverse,f2_create(grid_dim.x,grid_dim.y));             
            p2 = fmj_editor_apply_scale_f2(p,p2,sfinverse,f2_create(grid_dim.x,grid_dim.y));
            c1 = fmj_editor_apply_scale_f2(p,c1,sfinverse,f2_create(grid_dim.x,grid_dim.y));
            c2 = fmj_editor_apply_scale_f2(p,c2,sfinverse,f2_create(grid_dim.x,grid_dim.y));
            h1 = fmj_editor_apply_scale_f2(p,h1,sfinverse,f2_create(grid_dim.x,grid_dim.y));
            h2 = fmj_editor_apply_scale_f2(p,h2,sfinverse,f2_create(grid_dim.x,grid_dim.y));
            
            draw_list->AddNgonFilled(ImVec2(p1.x,p1.y),4.0f, col, 10);             
            draw_list->AddNgonFilled(ImVec2(p2.x,p2.y),4.0f, col, 10);
            draw_list->AddNgonFilled(ImVec2(h1.x,h1.y),8.0f, col, 4);
            draw_list->AddNgonFilled(ImVec2(h2.x,h2.y),8.0f, col, 4);

            draw_list->AddLine(ImVec2(p1.x,p1.y), ImVec2(h1.x,h1.y), col,1.0f);
            draw_list->AddLine(ImVec2(p2.x,p2.y), ImVec2(h2.x,h2.y), col,1.0f);
         
            draw_list->AddBezierCurve(ImVec2(p1.x,p1.y), ImVec2(c1.x,c1.y), ImVec2(c2.x,c2.y), ImVec2(p2.x,p2.y), col, 3,20);

        }
    }
            f2 offset = f2_mul(console->grid_pos,grid_dim);
            offset = f2_create(offset.x,-offset.y);    
            f2 last_result_offset = f2_sub(console->last_result,offset);
    f2 out_p = fmj_editor_apply_scale_f2(p,last_result_offset,f2_create(b,c),grid_dim);
    
    draw_list->AddNgonFilled(ImVec2(out_p.x,out_p.y),8.0f, col, 4);    
}

void fmj_editor_console_show(FMJEditorConsole* console,FMJAssetContext* asset_ctx)
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
    ImVec2 p = ImGui::GetCursorScreenPos();

    static ImVec4 colf = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
    const ImU32 col = ImColor(colf);
    
    f2 grid_dim = f2_create(400,400);
    ImVec2 canvas_size = ImVec2(grid_dim.x,grid_dim.y);
    
    ImGui::Text("Curve Editor");
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    f2 scale_factor = f2_create(max_scale_factor_x,max_scale_factor);
    f2 scale_factor_inverted = f2_create(1.0f/scale_factor.x,1.0f/scale_factor.y);
    
    fmj_editor_draw_grid(console,grid_dim);

//move points from user input
    p = ImGui::GetCursorScreenPos();    
    ImGui::InvisibleButton("canvas", canvas_size);
    ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - p.x, ImGui::GetIO().MousePos.y - p.y);
    f2 mp = f2_create(mouse_pos_in_canvas.x,mouse_pos_in_canvas.y);
    console->mp = mp;
     
    if (ImGui::IsItemHovered())
    {
        fmj_editor_check_add_points(console,mp,grid_dim,scale_factor,scale_factor_inverted);
         
        if(ImGui::IsMouseDown(0))
        {
            if(console->point_in_progress)
            {
                fmj_editor_move_unfinished_point(console,mp);
            }
            else
            {
                fmj_editor_move_point_at_mp(console,mp,scale_factor,scale_factor_inverted,grid_dim);
            }
        }//end check if down
        else
        {
            console->is_moving_point = false;
            console->moving_curve = nullptr;
        }
    }

    if(console->is_moving_point && console->moving_curve && console->selected_points_index >= 0)
    {
        f2 offset = f2_mul(console->grid_pos,grid_dim);
        offset = f2_create(offset.x,-offset.y);

        int i = console->selected_points_index;
        FMJBezierCubicCurve* curve = console->moving_curve;
        f2 test_p = curve->points[i];
        f2 non_offset_test_p = test_p;

        f2 p1 = curve->points[0];
        f2 p2 = curve->points[3];
        f2 c1 = curve->points[1];
        f2 c2 = curve->points[2];

        f2 scaled_p1 = fmj_editor_apply_scale_f2(f2_create_f(0),p1,scale_factor_inverted,f2_create_f(grid_dim.y));
        f2 scaled_p3 = fmj_editor_apply_scale_f2(f2_create_f(0),p2,scale_factor_inverted,f2_create_f(grid_dim.y));
        
        f32 length = f2_distance(p2,p1);
        length = length * 0.333f;
        f2 scaled_test_p = f2_sub(test_p,offset);
        test_p = fmj_editor_apply_scale_f2(f2_create_f(0),scaled_test_p,scale_factor_inverted,f2_create_f(grid_dim.y));
        if(i == 1 || i == 2)//control points
        {
            if(i == 1)
                test_p = curve->handle_a;
            if(i == 2)
                test_p = curve->handle_b;

            scaled_test_p = f2_sub(test_p,offset);
            test_p = fmj_editor_apply_scale_f2(f2_create_f(0),scaled_test_p,scale_factor_inverted,f2_create_f(grid_dim.y));            
            if(i == 1)
            {
                f2 diff_v2 = f2_sub(mp,test_p);
                f2 new_p = f2_add(curve->handle_a,diff_v2);
                curve->handle_a = new_p;

                fmj_editor_update_cp(curve->handle_a,
                                     &curve->points[1],
                                     f2_create_f(50),
                                     scaled_p1,
                                     scaled_p3);
            }
            else if(i == 2)
            {
                f2 diff_v2 = f2_sub(mp,test_p);
                f2 new_p = f2_add(curve->handle_b,diff_v2);
                curve->handle_b = new_p;
    
                fmj_editor_update_cp(curve->handle_b,
                                     &curve->points[2],
                                     f2_create_f(50),
                                     scaled_p3,
                                     scaled_p1);
            }                        
        }
        else
        {
            FMJBezierCubicCurve* other_curve_at_point = nullptr;
            int prev_or_next_index = -1;
            if(console->selected_curve_index > 0  && console->selected_points_index == 0)
            {
                FMJBezierCubicCurve* prev_curve = fmj_stretch_buffer_check_out(FMJBezierCubicCurve,&console->points,console->selected_curve_index - 1);
                other_curve_at_point = prev_curve;
                prev_or_next_index = 3;
            }
            else if(console->selected_curve_index > 0  && console->selected_points_index == 3)
            {
                FMJBezierCubicCurve* next_curve = fmj_stretch_buffer_check_out(FMJBezierCubicCurve,&console->points,console->selected_curve_index +  1);
                other_curve_at_point = next_curve;
                prev_or_next_index = 0;                
            }
            
            test_p = fmj_editor_apply_scale_f2(f2_create_f(0),scaled_test_p,scale_factor_inverted,f2_create_f(grid_dim.y));
            f2 diff_v2 = f2_sub(mp,test_p);
                    
            f2 new_p = f2_add(non_offset_test_p,diff_v2);
            f2 move_diff = f2_sub(new_p,non_offset_test_p);
            if(i == 0)
            {
                curve->handle_a = f2_add(move_diff,curve->handle_a);
                if(other_curve_at_point)
                {
                    other_curve_at_point->handle_b = f2_add(move_diff,other_curve_at_point->handle_b);
                }                
            }
            else if(i == 3)
            {
                curve->handle_b = f2_add(move_diff,curve->handle_b);
                if(other_curve_at_point)
                {
                    other_curve_at_point->handle_a = f2_add(move_diff,other_curve_at_point->handle_a);
                }                
            }
                                 
            curve->points[i] = new_p;
            if(other_curve_at_point)
            {
                other_curve_at_point->points[prev_or_next_index] = new_p;
                fmj_stretch_buffer_check_in(&console->points);                            
            }

        }

        fmj_editor_update_cp(curve->handle_a,
                             &curve->points[1],
                             f2_create_f(50),
                             p1,
                             p2);
    
        fmj_editor_update_cp(curve->handle_b,
                             &curve->points[2],
                             f2_create_f(50),
                             p2,
                             p1);                
    }
    
    fmj_editor_draw_curves(console,grid_dim,f2_create(p.x,p.y));

//Test bed 
     
     f2 extra_size = f2_create_f(100);
     ImGui::InvisibleButton("spacing",ImVec2(extra_size.x,extra_size.y));
     f2 min_max_scale = f2_create(1.0f,10000.f);
     
     ImGui::SliderFloat("scale_x",&max_scale_factor_x,min_max_scale.x, min_max_scale.y);          
     ImGui::SliderFloat("scale_y",&max_scale_factor,min_max_scale.x,min_max_scale.y);
     
     ImGui::SliderFloat("test_x",&console->test_x,0,min_max_scale.y);     

     ImGui::SliderFloat("pos_x",&console->grid_pos.x,-min_max_scale.y,min_max_scale.y);
     ImGui::SliderFloat("pos_y",&console->grid_pos.y,-min_max_scale.y,min_max_scale.y);

     static char name[32] = "File Name";
     char buf[64]; sprintf(buf, "%s", name); // ### operator override ID ignoring the preceding label
     ImGui::InputText("File Name", name, IM_ARRAYSIZE(name));     
     if(ImGui::Button("Save"))
     {
         f2 offset = f2_mul(console->grid_pos,grid_dim);
         offset = f2_create(offset.x,-offset.y);
         //teste evaluate
         //first find the curve we are in against scale x
         //than evalute the curve  point at t to get y
         //than map y pos to our scale y
         for(int i = 0;i < console->points.fixed.count;++i)
         {
             FMJBezierCubicCurve curve = fmj_stretch_buffer_get(FMJBezierCubicCurve,&console->points,i);
             float p1x = curve.points[0].x / max_scale_factor_x;
             float p2x = curve.points[3].x / max_scale_factor_x;

             f32 test_x_in_time_t = console->test_x / max_scale_factor_x;
             float test_x_in_drawing_coridinates = (test_x_in_time_t * curve.min_max_range.y);
             if((p1x) < test_x_in_drawing_coridinates && (p2x) > test_x_in_drawing_coridinates)
             {
                 f2 p1 = curve.points[0];
                 f2 p2 = curve.points[3];
                 f2 c1 = curve.points[1];
                 f2 c2 = curve.points[2];
                 f2 ha = curve.handle_a;
                 f2 hb = curve.handle_b;
                 
                 f2 p1s = fmj_editor_apply_scale_f2(f2_create_f(0),p1,scale_factor_inverted,f2_create(canvas_size.x,canvas_size.y));
                 f2 p2s = fmj_editor_apply_scale_f2(f2_create_f(0),p2,scale_factor_inverted,f2_create(canvas_size.x,canvas_size.y));
                 
                 f32 test_x_px_diff_in_drawing_cords  = test_x_in_drawing_coridinates - p1s.x;                 
                 f32 length_of_segment_drawing_cords = p2s.x - p1s.x;
                 f32 tesx_x_in_local_normalized_cords = test_x_px_diff_in_drawing_cords / length_of_segment_drawing_cords;
                 
                 ImVec2 v2 = ImBezierCalc(ImVec2(p1.x,p1.y),ImVec2(c1.x,c1.y),ImVec2(c2.x,c2.y),ImVec2(p2.x,p2.y),tesx_x_in_local_normalized_cords);
                 
                 f2 result_c = f2_create(v2.x,v2.y);
                 console->last_result = result_c;

                 break;
             }
         }

         fmj_stretch_buffer_clear(&console->curves_output.buffer);
         
         for(int ci = 0;ci < console->points.fixed.count;++ci)
         {
             FMJBezierCubicCurve curve = fmj_stretch_buffer_get(FMJBezierCubicCurve,&console->points,ci);
             f2 p1 = curve.points[0];
             f2 p2 = curve.points[3];
             f2 c1 = curve.points[1];
             f2 c2 = curve.points[2];
             f2 h1 = curve.handle_a;
             f2 h2 = curve.handle_b;
                 
             FMJBezierCubicCurve newcurve = {};
             p1.x = newcurve.points[0].x = p1.x / curve.min_max_range.y;
             p1.y = newcurve.points[0].y = (curve.min_max_range.y - p1.y) / curve.min_max_range.y;

             c1.x = newcurve.points[1].x = c1.x / curve.min_max_range.y;
             c1.y = newcurve.points[1].y = (curve.min_max_range.y - c1.y) / curve.min_max_range.y;

             c2.x = newcurve.points[2].x = c2.x / curve.min_max_range.y;
             c2.y = newcurve.points[2].y = (curve.min_max_range.y - c2.y) / curve.min_max_range.y;

             p2.x = newcurve.points[3].x = p2.x / curve.min_max_range.y;
             p2.y = newcurve.points[3].y = (curve.min_max_range.y - p2.y) / curve.min_max_range.y;

             newcurve.handle_a.x = h1.x / curve.min_max_range.y;
             newcurve.handle_a.y = (curve.min_max_range.y - h1.y) / curve.min_max_range.y;
             
             newcurve.handle_b.x = h2.x / curve.min_max_range.y;
             newcurve.handle_b.y = (curve.min_max_range.y - h2.y) / curve.min_max_range.y;

             newcurve.min_max_range = curve.min_max_range;
             
             fmj_stretch_buffer_push(&console->curves_output.buffer,&newcurve);
         }

         FMJFilePointer file = {};
         char* file_name = name;//"curve1.crv";
         FMJString file_name_with_ext = fmj_string_append_char_to_char(name,".crv", &console->scratch_mem);
         fmj_file_platform_write_memory(&file,file_name_with_ext.string,&console->curves_output.buffer,sizeof(FMJStretchBuffer),false,"wb");
         fmj_file_platform_write_memory(&file,nullptr,console->curves_output.buffer.fixed.mem_arena.base,console->curves_output.buffer.fixed.mem_arena.size,true,"wb");
     }

     FMJFileDirInfoResult all_files = fmj_file_platform_get_all_files_in_dir("../Build",&console->scratch_mem);

     static ImGuiComboFlags flags = 0;
     static const char* item_current = "none";//items[0];            // Here our selection is a single pointer stored outside the object.
     if (ImGui::BeginCombo("Choose Curve", item_current, flags)) // The second parameter is the label previewed before opening the combo.
     {
         for(int i = 0;i < all_files.files.count;++i)
//         for (int n = 0; n < IM_ARRAYSIZE(items); n++)
         {
             FMJFileInfo* info = fmj_fixed_buffer_get_ptr(FMJFileInfo,&all_files.files,i);
             bool is_selected = (item_current == info->name.string);

//             if (ImGui::Selectable(items[n], is_selected))
             if (ImGui::Selectable(info->name.string, is_selected))             
             {
                 item_current = info->name.string;//items[n];
                 FMJString ext = fmj_string_get_extension(fmj_string_create((char*)item_current,&console->scratch_mem),&console->scratch_mem,false);
                 if(fmj_string_cmp_char(ext,"crv"))
                 {
                     FMJFileReadResult result = fmj_file_platform_read_entire_file(info->name.string);
                     FMJStretchBuffer* curves = (FMJStretchBuffer*)result.content;
                     void* source = (u8*)result.content + sizeof(FMJStretchBuffer);
                     u64 memsize = curves->fixed.mem_arena.size;
                     u64 used = curves->fixed.mem_arena.used;
                     curves->fixed.mem_arena = fmj_arena_allocate(memsize);
                     void* dest = curves->fixed.mem_arena.base;                     
                     fmj_memory_copy(dest,source,memsize);

                     curves->fixed.mem_arena.used = used;
                     curves->fixed.base = curves->fixed.mem_arena.base;                     
                     ASSERT(result.content_size > 0);
                     console->points = *curves;

                     for(int ci = 0;ci < console->points.fixed.count;++ci)
                     {
                         FMJBezierCubicCurve* curve = fmj_stretch_buffer_check_out(FMJBezierCubicCurve,&console->points,ci);
                         f2 p1 = curve->points[0];
                         f2 p2 = curve->points[3];
                         f2 c1 = curve->points[1];
                         f2 c2 = curve->points[2];
                         f2 h1 = curve->handle_a;
                         f2 h2 = curve->handle_b;
                 
                         FMJBezierCubicCurve newcurve = {};
                         p1.x = newcurve.points[0].x = p1.x * curve->min_max_range.y;
                         p1.y = newcurve.points[0].y = curve->min_max_range.y - (p1.y * curve->min_max_range.y);
                         
                         c1.x = newcurve.points[1].x = c1.x * curve->min_max_range.y;
                         c1.y = newcurve.points[1].y = curve->min_max_range.y - (c1.y * curve->min_max_range.y);
                         

                         c2.x = newcurve.points[2].x = c2.x * curve->min_max_range.y;
                         c2.y = newcurve.points[2].y = curve->min_max_range.y - (c2.y * curve->min_max_range.y);
                         
                         p2.x = newcurve.points[3].x = p2.x * curve->min_max_range.y;
                         p2.y = newcurve.points[3].y = curve->min_max_range.y - (p2.y * curve->min_max_range.y);
                         

                         newcurve.handle_a.x = h1.x * curve->min_max_range.y;
                         newcurve.handle_a.y = curve->min_max_range.y - (h1.y * curve->min_max_range.y);
                       
                         newcurve.handle_b.x = h2.x * curve->min_max_range.y;
                         newcurve.handle_b.y = curve->min_max_range.y - (h2.y * curve->min_max_range.y);
                         

                         newcurve.min_max_range = curve->min_max_range;
                         
                         *curve = newcurve;
                         fmj_stretch_buffer_check_in(&console->points);             
                     }

                     fmj_file_free_result(&result);
                 }                 
             }

             if (is_selected)
             {
                 ImGui::SetItemDefaultFocus();   // Set the initial focus when opening the combo (scrolling + for keyboard navigation support in the upcoming navigation branch)

             }
         }
         ImGui::EndCombo();
     }
     
     console->prev_mp = console->mp;
     ImGui::End();
     fmj_arena_deallocate(&console->scratch_mem,false);

}

#define EDITOR_H
#endif


