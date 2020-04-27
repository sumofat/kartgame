#if !defined (EDITOR_H)

#define EDITOR_CONSOLE_ENTRY_MAX_TEXT_SIZE 1024
struct FMJBezierCubicCurve
{
    f2 points[4];
};

struct FMJEditorConsole
{
    bool is_showing;
    FMJStretchBuffer commands;
    FMJMemoryArena string_mem;
    FMJStretchBuffer items;
//Testing
    FMJStretchBuffer points;
    FMJBezierCubicCurve unfinished_point;
    bool point_in_progress;
    int selected_points_index;
    f2 p1;
    f2 p2;
    f2 p3;
    f2 p4;
    f2 p5;
    f2 p6;
    f2 p7;    
};

void fmj_editor_console_init(FMJEditorConsole* console)
{
    ASSERT(console);
    if(console)
    {
        console->is_showing = false;
//        console->commands = fmj_stretch_buffer_init(1,)
        console->string_mem = fmj_arena_allocate(FMJMEGABYTES(2));
        console->items = fmj_stretch_buffer_init(1,sizeof(FMJString),8);
        console->points = fmj_stretch_buffer_init(1,sizeof(FMJBezierCubicCurve),8);
        console->selected_points_index = -1;
    }
}

void fmj_editor_console_add_entry_(FMJEditorConsole* console,char* a)
{
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
        //if (ImGui::Selectable("Clear")) ClearLog();
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
    
    ImGui::SliderFloat2("p1",console->p1.xy,min ,max);
    ImGui::SliderFloat2("p2",console->p2.xy,min ,max);
    ImGui::SliderFloat2("p3",console->p3.xy,min ,max);
    ImGui::SliderFloat2("p4",console->p4.xy,min ,max);

    ImGui::SliderFloat2("p5",console->p5.xy,min ,max);
    ImGui::SliderFloat2("p6",console->p6.xy,min ,max);
    ImGui::SliderFloat2("p7",console->p7.xy,min ,max);
    
//IMGUI_API bool          SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
//IMGUI_API bool          SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
//IMGUI_API bool          SliderFloat2(const char* label, float v[2], float v_min, float v_max, const char* format = "%.3f", float power = 1.0f);
    ImGui::Text("Custom Drawing test");

    const ImVec2 p = ImGui::GetCursorScreenPos();

    static ImVec4 colf = ImVec4(1.0f, 1.0f, 0.4f, 1.0f);
     const ImU32 col = ImColor(colf);
     ImDrawList* draw_list = ImGui::GetWindowDrawList();
//AddNgonFilled(const ImVec2& center, float radius, ImU32 col, int num_segments);

//     draw_list->AddRectFilledMultiColor(canvas_pos, ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y), IM_COL32(50, 50, 50, 255), IM_COL32(50, 50, 60, 255), IM_COL32(60, 60, 70, 255), IM_COL32(50, 50, 60, 255));     
     draw_list->AddNgonFilled(ImVec2(p.x + console->p1.x,p.y + console->p1.y),10.0f, col, 10);

     //IMGUI_API void  AddBezierCurve(const ImVec2& p1, const ImVec2& p2, const ImVec2& p3, const ImVec2& p4, ImU32 col, float thickness, int num_segments = 0);
     f2 p1 = f2_create(p.x + console->p1.x,p.y + console->p1.y);
     f2 p2 = f2_create(p.x + console->p2.x,p.y + console->p2.y);
     f2 p3 = f2_create(p.x + console->p3.x,p.y + console->p3.y);
     f2 p4 = f2_create(p.x + console->p4.x,p.y + console->p4.y);

     f2 p5 = f2_create(p.x + console->p4.x,p.y + console->p4.y);
     f2 p6 = f2_create(p.x + console->p5.x,p.y + console->p5.y);
     f2 p7 = f2_create(p.x + console->p6.x,p.y + console->p6.y);
     
     draw_list->AddBezierCurve(ImVec2(p1.x,p1.y), ImVec2(p2.x,p2.y), ImVec2(p3.x,p3.y), ImVec2(p4.x,p4.y), col, 3,20);
     draw_list->AddBezierCurve(ImVec2(p4.x,p4.y), ImVec2(p5.x,p5.y), ImVec2(p6.x,p6.y), ImVec2(p7.x,p7.y), col, 3,20);

     ImVec2 a = ImBezierCalc(ImVec2(p1.x,p1.y), ImVec2(p2.x,p2.y), ImVec2(p3.x,p3.y), ImVec2(p4.x,p4.y),0.0f);
     ImVec2 b = ImBezierCalc(ImVec2(p4.x,p4.y), ImVec2(p5.x,p5.y), ImVec2(p6.x,p6.y), ImVec2(p7.x,p7.y),0.0f);


     static ImVec4 grey = ImVec4(0.8f, 0.8f, 0.8f, 0.5f);
     const ImU32 grey_col = ImColor(grey);

     int lines_count = 20;
     int width = max;
     int height = max;
     ImVec2 canvas_size = ImVec2(max,max);//ImGui::GetContentRegionAvail();     
     int offset = width / lines_count;          
     for(int x =0;x < lines_count;++x)
     {
         draw_list->AddLine(ImVec2(p.x + (offset * x),p.y), ImVec2(p.x + (offset * x),p.y + height), grey_col,1.0f);             
     }
     
     //horizontal lines
     for(int y = 0;y < lines_count;++y)
     {
         draw_list->AddLine(ImVec2(p.x,p.y + (offset * y)), ImVec2(p.x + width,p.y + (offset * y)), grey_col,2.0f);             
     }

//     ImGui::Dummy(ImVec2(300,400));

     ImGui::InvisibleButton("canvas", canvas_size);
     ImVec2 mouse_pos_in_canvas = ImVec2(ImGui::GetIO().MousePos.x - p.x, ImGui::GetIO().MousePos.y - p.y);     
     if (ImGui::IsItemHovered())
     {
         draw_list->AddNgonFilled(ImVec2(p.x + mouse_pos_in_canvas.x,p.y + mouse_pos_in_canvas.y),10.0f, col, 4);
         if(ImGui::IsMouseDown(0))
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
                     f32 distance = abs(f2_length(f2_sub(test_p,mp)));
                     if(distance < 5)
                     {
                         console->selected_points_index = i;
                         f2 new_p = f2_create(mp.x,mp.y);
                         curve->points[i] = new_p;
                         fmj_editor_console_add_entry(console,"found point at x:%f y:%f ",new_p.x,new_p.y);
                         break;
                     }
                 }
                 fmj_stretch_buffer_check_in(&console->points);                 
             }
         }
         else
         {
            console->selected_points_index = -1;             
         }
         
         if (ImGui::IsMouseReleased(0) && console->selected_points_index == -1)
         {
             f2 point = console->unfinished_point.points[0] = f2_create(mouse_pos_in_canvas.x,mouse_pos_in_canvas.y);
             /*
             f2 point = 

             */
             console->point_in_progress = true;             
             fmj_editor_console_add_entry(console,"added start end point at x:%f y:%f ",point.x,point.y);             
         }
         else if(ImGui::IsMouseClicked(1) && console->point_in_progress)
         {
     
             f2 point = console->unfinished_point.points[3] = f2_create(mouse_pos_in_canvas.x,mouse_pos_in_canvas.y);
             fmj_editor_console_add_entry(console,"added finished end point at x:%f y:%f ",point.x,point.y);
             f2 dir = f2_mul_s(f2_normalize(f2_sub(console->unfinished_point.points[3],console->unfinished_point.points[0])),10);
             f2 opdir = f2_mul_s(f2_normalize(f2_sub(console->unfinished_point.points[0],console->unfinished_point.points[3])),10);             
             
             console->unfinished_point.points[1] = f2_add(console->unfinished_point.points[0],dir);
             console->unfinished_point.points[2] = f2_add(console->unfinished_point.points[3],opdir);
             
             fmj_stretch_buffer_push(&console->points,&console->unfinished_point);
             console->unfinished_point.points[0] = f2_create_f(0);
             console->unfinished_point.points[1] = f2_create_f(0);
             console->unfinished_point.points[2] = f2_create_f(0);
             console->unfinished_point.points[3] = f2_create_f(0);             
         }
     }
     
     for(int i = 0;i < console->points.fixed.count;++i)
     { 
         FMJBezierCubicCurve curve = fmj_stretch_buffer_get(FMJBezierCubicCurve,&console->points,i);
         f2 p1 = curve.points[0];
         f2 p2 = curve.points[3];
         f2 c1 = curve.points[1];
         f2 c2 = curve.points[2];
         
         draw_list->AddNgonFilled(ImVec2(p.x + p1.x,p.y + p1.y),4.0f, col, 10);
         draw_list->AddNgonFilled(ImVec2(p.x + p2.x,p.y + p2.y),4.0f, col, 10);

         draw_list->AddNgonFilled(ImVec2(p.x + c1.x,p.y + c1.y),8.0f, col, 4);
         draw_list->AddNgonFilled(ImVec2(p.x + c2.x,p.y + c2.y),8.0f, col, 4);

         draw_list->AddBezierCurve(ImVec2(p.x + p1.x,p.y + p1.y), ImVec2(p.x + c1.x,p.y + c1.y), ImVec2(p.x + c2.x,p.y + c2.y), ImVec2(p.x + p2.x,p.y + p2.y), col, 3,20);         
     }

     
     ImGui::End();    
}



#define EDITOR_H
#endif


